#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <TFT_eSPI.h> 
#include "credentials.h"

WiFiClient wifiClient;
HttpClient httpClient = HttpClient(wifiClient, shelly_ip, 80);

TFT_eSPI tft = TFT_eSPI();

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

void initWiFi() {
  WiFi.mode(WIFI_STA);

  // Scan for available networks
  Serial.println("Scanning for available networks...");
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.println("Scanning WiFi...");

  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No networks found");
    tft.println("No networks found");
  } else {
    Serial.printf("%d networks found:\n", n);
    tft.printf("%d networks found:\n", n);

    for (int i = 0; i < n; ++i) {
      Serial.printf("%d: %s (%d dBm)\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));

      String ssidStr = WiFi.SSID(i);
      tft.printf("%s (%d)\n", ssidStr.c_str(), WiFi.RSSI(i));

      delay(100);
    }
  }

  delay(5000);

  // Connect to your configured WiFi
  WiFi.begin(ssid, password);
  //Serial.print("Connecting to WiFi ..");
  tft.print("\n");
  tft.print("Connecting to: "); tft.println(ssid);
  tft.print("Shelly IP: "); tft.println(shelly_ip);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void shellyHttpRequest() {
    String url = "/status";
    httpClient.beginRequest();
    httpClient.get(url);

    if (String(shelly_username) != "" && String(shelly_password) != "") {
      httpClient.sendBasicAuth(shelly_username, shelly_password);
    }

    httpClient.endRequest();
}

PowerData getPowerData(String responseBody) {
  PowerData data;

  // Extract "available"
  int disponibile_start = responseBody.indexOf("\"power\":", responseBody.indexOf("\"emeters\":"));
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

void progressBarAndDelay() {
    int progress = 0;
    tft.fillRect(2, tft.height() - 12, tft.width() - 4, 10, TFT_DARKGREY);  // Background bar
    for (progress = 0; progress <= 100; progress++) {
      drawProgressBarSmooth(progress);
      delay(50);
    }
}

void drawProgressBarSmooth(int progress) {
  int barWidth = tft.width() - 4;  // Width of the progress bar (leaving some margin)
  int barHeight = 10;              // Height of the progress bar
  int x = 2;                       // X position for the progress bar
  int y = tft.height() - barHeight - 2;  // Y position near the bottom

  // Draw the filled portion of the progress bar
  int filledWidth = map(progress, 0, 100, 0, barWidth);
  tft.fillRect(x, y, filledWidth, barHeight, TFT_WHITE);
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
  
  // Initialize TFT display
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);

  // Initialize Wi-Fi
  initWiFi();
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
  data = {0}; // Initializes all members to 0
  peak = 0;
}

void loop() {
  // Make sure WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {

    shellyHttpRequest();
    int statusCode = httpClient.responseStatusCode();
    String responseBody = httpClient.responseBody();

    if (statusCode > 0) {

      data = getPowerData(responseBody);
      updatePeak(data);      
      printData(data, peak);

      progressBarAndDelay();
      
    } else {
      Serial.println("Error on HTTP request: " + String(statusCode));
    }
  }

  // Wait before sending another request
  //delay(5000);
}

