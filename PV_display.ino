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
      int emeter0_power_start = responseBody.indexOf("\"power\":", responseBody.indexOf("\"emeters\":"));
      int emeter0_power_end = responseBody.indexOf(",", emeter0_power_start);
      String emeter0_power_str = responseBody.substring(emeter0_power_start + 8, emeter0_power_end);
      float emeter0_power_float = emeter0_power_str.toFloat();
      int emeter0_power = (int)emeter0_power_float;  // Convert to int to remove decimals

      int emeter1_power_start = responseBody.indexOf("\"power\":", emeter0_power_end);
      int emeter1_power_end = responseBody.indexOf(",", emeter1_power_start);
      String emeter1_power_str = responseBody.substring(emeter1_power_start + 8, emeter1_power_end);
      float emeter1_power_float = emeter1_power_str.toFloat();
      int emeter1_power = (int)emeter1_power_float;  // Convert to int to remove decimals

      tft.fillScreen(TFT_BLACK);

      // Set cursor position for "Differenza" label
      int textHeight = 40; // Height of the text line (can be adjusted based on font size)
      int totalHeight = tft.height();
      int diffLabelY = (totalHeight / 4) - (textHeight / 2);  // Vertical center for first label
      tft.setTextSize(3);
      tft.setCursor((tft.width() - tft.textWidth("Differenza:", 1)) / 2, diffLabelY, 1);
      tft.println("Differenza:");

      // Set cursor position for emeter0_power value
      int emeter0PowerY = diffLabelY + textHeight;
      tft.setTextSize(1);
      tft.setCursor((tft.width() - tft.textWidth(String(emeter0_power) + " W", 7)) / 2, emeter0PowerY, 7);
      tft.println(String(emeter0_power) + " W");

      // Set cursor position for "Produzione" label
      int prodLabelY = (totalHeight * 2.5 / 4) - (textHeight / 2);  // Vertical center for second label
      tft.setTextSize(3);
      tft.setCursor((tft.width() - tft.textWidth("Produzione:", 1)) / 2, prodLabelY, 1);
      tft.println("Produzione:");

      // Set cursor position for emeter1_power value
      int emeter1PowerY = prodLabelY + textHeight;
      tft.setTextSize(1);
      tft.setCursor((tft.width() - tft.textWidth(String(emeter1_power) + " W", 7)) / 2, emeter1PowerY, 7);
      tft.println(String(emeter1_power) + " W");

      tft.fillRect(2, tft.height() - 12, tft.width() - 4, 10, TFT_DARKGREY);  // Sfondo barra
      for (progress = 0; progress <= 100; progress++) {
        drawProgressBarSmooth(progress);
        delay(50);
      }
    } else {
      Serial.println("Error on HTTP request: " + String(statusCode));
    }
  }

  // Wait before sending another request
  //delay(5000);
}
