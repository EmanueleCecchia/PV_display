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

PowerData data;
int peak;

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
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

void printData(PowerData data, int peak) {
  int paddingBottom = 40;
  int distanceYLabelValue = 40;
  int totHeight = tft.height();
  int totWidth = tft.width();

  int topLeftX = totWidth / 4;
  int topLeftY = totHeight / 4 - paddingBottom;
  
  int topRightX = totWidth / 4 * 3;
  int topRightY = totHeight / 4 - paddingBottom;
  
  int bottomCenterX = totWidth / 2;
  int bottomCenterY = totHeight / 4 * 3 - paddingBottom;

  int bottomLeftX = totWidth / 4;
  int bottomLeftY = totHeight / 4 * 3 - paddingBottom;
  
  int bottomRightX = totWidth / 4 * 3;
  int bottomRightY = totHeight / 4 * 3 - paddingBottom;

  tft.fillScreen(TFT_BLACK);

  // Fotovoltaico
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(topLeftX - tft.textWidth("FOTOVOLTAICO", 4) / 2, topLeftY, 4);
  tft.println("FOTOVOLTAICO");

  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(topLeftX - tft.textWidth(String(data.photovoltaic), 7) / 2, topLeftY + distanceYLabelValue, 7);
  tft.println(String(data.photovoltaic));

  // Utilizzo
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(topRightX - tft.textWidth("UTILIZZO", 4) / 2, topRightY, 4);
  tft.println("UTILIZZO");

  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(topRightX - tft.textWidth(String(data.use), 7) / 2, topRightY + distanceYLabelValue, 7);
  tft.println(String(data.use));

  // Disponibile
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(bottomLeftX - tft.textWidth("DISPONIBILE", 4) / 2, bottomLeftY, 4);
  tft.println("DISPONIBILE");

  // Change text color based on the value of "disponibile"
  if (data.available < 0) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  }

  tft.setTextSize(1);
  tft.setCursor(bottomLeftX - tft.textWidth(String(data.available), 7) / 2, bottomLeftY + distanceYLabelValue, 7);
  tft.println(String(data.available));

  // Picco
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(bottomRightX - tft.textWidth("PICCO", 4) / 2, bottomRightY, 4);
  tft.println("PICCO");

  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(bottomRightX - tft.textWidth(String(peak), 7) / 2, bottomRightY + distanceYLabelValue, 7);
  tft.println(String(peak));
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

