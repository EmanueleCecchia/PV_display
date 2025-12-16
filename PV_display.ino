#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <TFT_eSPI.h> 
#include <Preferences.h> // Library to save data to flash memory
#include "credentials.h"

// --- Global Objects ---
WiFiClient wifiClient;
// We will initialize HttpClient dynamically, not here, because the IP might change.
HttpClient* httpClient = nullptr; 

TFT_eSPI tft = TFT_eSPI();
Preferences preferences; // Create Preferences object

// --- Variables ---
String currentShellyIp;  // Holds the mutable IP address
bool isScanning = false;

struct PowerData {
  int available;
  int photovoltaic;
  int use;
  int peak;
};

struct Position {
   int x;
   int y;
};

struct Positions {
   struct Position topLeft;
   struct Position topRight;
   struct Position bottomCenter;
   struct Position bottomLeft;
   struct Position bottomRight;
};

// --- Helper Functions ---

struct Positions calculatePositions(int width, int height, int paddingBottom) {
   struct Positions pos;
   pos.topLeft.x = width / 4;
   pos.topLeft.y = height / 4 - paddingBottom;
   pos.topRight.x = width / 4 * 3;
   pos.topRight.y = height / 4 - paddingBottom;
   pos.bottomCenter.x = width / 2;
   pos.bottomCenter.y = height / 4 * 3 - paddingBottom;
   pos.bottomLeft.x = width / 4;
   pos.bottomLeft.y = height / 4 * 3 - paddingBottom;
   pos.bottomRight.x = width / 4 * 3;
   pos.bottomRight.y = height / 4 * 3 - paddingBottom;
   return pos;
}

PowerData data;
int peak;

// Initialize the HttpClient with the current IP
void initHttpClient() {
  if (httpClient != nullptr) {
    delete httpClient; // Free old memory if it exists
  }
  // Convert String to const char* for the library
  httpClient = new HttpClient(wifiClient, currentShellyIp.c_str(), 80);
}

void initWiFi() {
  WiFi.mode(WIFI_STA);

  // Visuals for scanning
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.println("Initializing WiFi...");

  // Load saved IP or use default
  preferences.begin("shelly-cfg", false); // Namespace "shelly-cfg"
  currentShellyIp = preferences.getString("saved_ip", shelly_ip); // Default to credentials.h if empty
  preferences.end();

  WiFi.begin(ssid, password);
  
  tft.print("Connecting to: "); tft.println(ssid);
  tft.print("Target IP: "); tft.println(currentShellyIp);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println("");
  Serial.println(WiFi.localIP());
  
  // Setup the HTTP client with the loaded IP
  initHttpClient();
}

// Function to scan the network if connection is lost
void scanForShelly() {
  isScanning = true;
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("LOST CONNECTION!");
  tft.println("Scanning 192.168.1.x ...");
  
  String baseIP = "192.168.1.";
  
  // Set a very short timeout for scanning to speed it up
  wifiClient.setTimeout(100); 

  for (int i = 1; i < 255; i++) {
    String testIP = baseIP + i;
    
    // Optional: Visual update every 10 IPs to reduce flicker
    if (i % 5 == 0) {
      tft.setCursor(0, 40);
      tft.fillRect(0, 40, tft.width(), 20, TFT_RED); // Clear line
      tft.print("Checking: "); tft.println(testIP);
    }

    // Try to connect strictly to port 80 first
    if (wifiClient.connect(testIP.c_str(), 80)) {
      Serial.print("Found open port at: "); Serial.println(testIP);
      wifiClient.stop(); // Close the raw test connection
      
      // Now verify it is actually the Shelly by asking for status
      HttpClient testHttp(wifiClient, testIP.c_str(), 80);
      testHttp.beginRequest();
      testHttp.get("/status");
      if (String(shelly_username) != "" && String(shelly_password) != "") {
         testHttp.sendBasicAuth(shelly_username, shelly_password);
      }
      testHttp.endRequest();

      int statusCode = testHttp.responseStatusCode();
      if (statusCode > 0 && statusCode < 400) {
        // FOUND IT!
        Serial.println("Shelly confirmed at: " + testIP);
        tft.fillScreen(TFT_GREEN);
        tft.setCursor(0, 0);
        tft.println("FOUND NEW IP!");
        tft.println(testIP);
        
        // Save to memory
        preferences.begin("shelly-cfg", false);
        preferences.putString("saved_ip", testIP);
        preferences.end();
        
        // Update global variable and client
        currentShellyIp = testIP;
        initHttpClient();
        
        delay(2000); // Show success message
        isScanning = false;
        
        // Reset timeout to normal
        wifiClient.setTimeout(5000); 
        return; 
      }
    }
  }
  
  // If we finish the loop and find nothing
  tft.fillScreen(TFT_BLACK);
  tft.println("Scan failed. Retrying...");
  wifiClient.setTimeout(5000); // Reset timeout
  delay(2000);
}

void shellyHttpRequest() {
    if(httpClient == nullptr) return;

    String url = "/status";
    httpClient->beginRequest(); // Use -> because it is a pointer now
    httpClient->get(url);

    if (String(shelly_username) != "" && String(shelly_password) != "") {
      httpClient->sendBasicAuth(shelly_username, shelly_password);
    }

    httpClient->endRequest();
}

PowerData getPowerData(String responseBody) {
  PowerData data = {0,0,0,0}; // Initialize safety

  if (responseBody.length() < 10) return data; // Basic validation

  // Extract "available"
  int disponibile_start = responseBody.indexOf("\"power\":", responseBody.indexOf("\"emeters\":"));
  if(disponibile_start == -1) return data; // Error parsing
  
  int disponibile_end = responseBody.indexOf(",", disponibile_start);
  String disponibile_str = responseBody.substring(disponibile_start + 8, disponibile_end);
  data.available = (int)disponibile_str.toFloat();

  // Extract "photovoltaic"
  int fotovoltaico_start = responseBody.indexOf("\"power\":", disponibile_end);
  int fotovoltaico_end = responseBody.indexOf(",", fotovoltaico_start);
  String fotovoltaico_str = responseBody.substring(fotovoltaico_start + 8, fotovoltaico_end);
  float fotovoltaico_float = fotovoltaico_str.toFloat();
  data.photovoltaic = (fotovoltaico_float < 0) ? 0 : (int)fotovoltaico_float;

  data.use = data.photovoltaic - data.available;

  return data;
}

void drawProgressBarSmooth(int progress) {
  int barWidth = tft.width() - 4;  
  int barHeight = 10;              
  int x = 2;                       
  int y = tft.height() - barHeight - 2; 

  int filledWidth = map(progress, 0, 100, 0, barWidth);
  tft.fillRect(x, y, filledWidth, barHeight, TFT_WHITE);
}

void progressBarAndDelay() {
    int progress = 0;
    tft.fillRect(2, tft.height() - 12, tft.width() - 4, 10, TFT_DARKGREY); 
    for (progress = 0; progress <= 100; progress++) {
      drawProgressBarSmooth(progress);
      delay(50);
    }
}

void updatePeak(PowerData data) {
  if (data.photovoltaic > peak) { peak = data.photovoltaic; }
}

void printVariable(char* name,int value, Position position, int distanceYLabelValue, bool isColorDynamic = false) {
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(position.x - tft.textWidth(name, 4) / 2, position.y, 4);
  tft.println(name);

  if (isColorDynamic == true && data.available < 0) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  }

  tft.setTextSize(1);
  if (isColorDynamic == false) { tft.setTextColor(TFT_YELLOW); }
  tft.setCursor(position.x - tft.textWidth(String(value), 7) / 2, position.y + distanceYLabelValue, 7);
  tft.println(String(value));
}

void printData(PowerData data, int peak) {
  struct Positions positions = calculatePositions(tft.width(), tft.height(), 50);
  int distanceYLabelValue = 40;

  tft.fillScreen(TFT_BLACK);

  printVariable("FOTOVOLTAICO", data.photovoltaic, positions.topLeft, distanceYLabelValue);
  printVariable("UTILIZZO", data.use, positions.bottomLeft, distanceYLabelValue);
  printVariable("DISPONIBILE", data.available, positions.bottomRight, distanceYLabelValue, true);
  printVariable("Fvp", peak, positions.topRight, distanceYLabelValue);
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);

  initWiFi();
  
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
  data = {0, 0, 0, 0}; 
  peak = 0;
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {

    shellyHttpRequest();
    
    // Note: httpClient is now a pointer, so we use arrow -> 
    int statusCode = httpClient->responseStatusCode();
    String responseBody = httpClient->responseBody();

    // Check if request was successful
    if (statusCode > 0) {
      data = getPowerData(responseBody);
      updatePeak(data);      
      printData(data, peak);
      progressBarAndDelay();
    } else {
      Serial.println("Error on HTTP request: " + String(statusCode));
      Serial.println("Could not reach Shelly at " + currentShellyIp);
      
      // TRIGGER SCAN
      scanForShelly();
    }
  } else {
    // Reconnect WiFi if lost
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }
  }
}