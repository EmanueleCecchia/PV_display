#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <TFT_eSPI.h> 
#include "credentials.h"

WiFiClient wifiClient;
HttpClient httpClient = HttpClient(wifiClient, shelly_ip, 80);

TFT_eSPI tft = TFT_eSPI();

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  tft.println("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
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

void setup() {
  Serial.begin(115200);
  delay(10);
  
  // Initialize TFT display
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);  // Clear the screen
  tft.setTextColor(TFT_WHITE);

  // Initialize Wi-Fi
  initWiFi();
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
}

void loop() {
  int progress = 0;

  // Make sure WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    String url = "/status";
    httpClient.beginRequest();
    httpClient.get(url);

    if (String(shelly_username) != "" && String(shelly_password) != "") {
      httpClient.sendBasicAuth(shelly_username, shelly_password);
    }

    httpClient.endRequest();

    int statusCode = httpClient.responseStatusCode();
    String responseBody = httpClient.responseBody();

    if (statusCode > 0) {
      int disponibile_start = responseBody.indexOf("\"power\":", responseBody.indexOf("\"emeters\":"));
      int disponibile_end = responseBody.indexOf(",", disponibile_start);
      String disponibile_str = responseBody.substring(disponibile_start + 8, disponibile_end);
      float disponibile_float = disponibile_str.toFloat();
      int disponibile = (int)disponibile_float;

      int fotovoltaico_start = responseBody.indexOf("\"power\":", disponibile_end);
      int fotovoltaico_end = responseBody.indexOf(",", fotovoltaico_start);
      String fotovoltaico_str = responseBody.substring(fotovoltaico_start + 8, fotovoltaico_end);
      float fotovoltaico_float = fotovoltaico_str.toFloat();
      int fotovoltaico = (int)fotovoltaico_float;

      // Calculate Utilizzo
      int utilizzo_power = fotovoltaico - disponibile;

      tft.fillScreen(TFT_BLACK);

      // Set cursor position for "Fotovoltaico" label (top left)
      int textHeight = 50;
      int totalHeight = tft.height();
      int prodLabelY = (totalHeight / 4) - (textHeight / 2);
      tft.setTextSize(3);
      tft.setCursor((tft.width() - tft.textWidth("Fotovoltaico:", 1)) / 4, prodLabelY, 1);
      tft.println("Fotovoltaico");

      // Set cursor position for "Fotovoltaico" value
      int prodValueX = (tft.width() - tft.textWidth("Fotovoltaico", 1)) / 4;
      tft.setTextSize(1);
      tft.setCursor(prodValueX, prodLabelY + textHeight / 2, 7);
      tft.println(String(fotovoltaico));

      // Set cursor position for "Utilizzo" label (on the same row as "Fotovoltaico")
      int utilLabelX = prodValueX + tft.textWidth(String(fotovoltaico), 7) + 100;
      tft.setTextSize(3);
      tft.setCursor(utilLabelX, prodLabelY, 1);
      tft.println("Utilizzo");

      // Set cursor position for "Utilizzo" value
      int utilValueX = utilLabelX;
      tft.setTextSize(1);
      tft.setCursor(utilValueX, prodLabelY + textHeight / 2, 7);
      tft.println(String(utilizzo_power));

      // Set cursor position for "Disponibile" label (bottom center)
      int dispLabelY = (totalHeight * 2.5 / 4) - (textHeight / 2);
      tft.setTextSize(3);
      tft.setCursor((tft.width() - tft.textWidth("Disponibile:", 1)) / 2, dispLabelY, 1);
      tft.println("Disponibile");

      // Set cursor position for "Disponibile" value
      int dispValueY = dispLabelY + textHeight;
      tft.setTextSize(1);
      tft.setCursor((tft.width() - tft.textWidth(String(disponibile), 7)) / 2, dispValueY, 7);
      tft.println(String(disponibile));

      // Draw progress bar
      tft.fillRect(2, tft.height() - 12, tft.width() - 4, 10, TFT_DARKGREY);  // Background bar
      for (progress = 0; progress <= 100; progress++) {
        drawProgressBarSmooth(progress);
        delay(50);
      }
    } else {
      Serial.println("Error on HTTP request: " + String(statusCode));
    }
  }

  // Wait before sending another request
  delay(5000);
}

