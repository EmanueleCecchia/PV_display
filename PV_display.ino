#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <TFT_eSPI.h> 
#include "credentials.h"

WiFiClient wifiClient;
HttpClient httpClient = HttpClient(wifiClient, shelly_ip, 80);

TFT_eSPI tft = TFT_eSPI();

struct PowerData {
  int disponibile;
  int fotovoltaico;
};


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

PowerData getPowerData(String responseBody) {
  PowerData data;

  // Extract "disponibile"
  int disponibile_start = responseBody.indexOf("\"power\":", responseBody.indexOf("\"emeters\":"));
  int disponibile_end = responseBody.indexOf(",", disponibile_start);
  String disponibile_str = responseBody.substring(disponibile_start + 8, disponibile_end);
  data.disponibile = (int)disponibile_str.toFloat();

  // Extract "fotovoltaico"
  int fotovoltaico_start = responseBody.indexOf("\"power\":", disponibile_end);
  int fotovoltaico_end = responseBody.indexOf(",", fotovoltaico_start);
  String fotovoltaico_str = responseBody.substring(fotovoltaico_start + 8, fotovoltaico_end);
  float fotovoltaico_float = fotovoltaico_str.toFloat();
  data.fotovoltaico = (fotovoltaico_float < 0) ? 0 : (int)fotovoltaico_float;

  return data;
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
  tft.fillScreen(TFT_BLACK);
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
      PowerData powerData = getPowerData(responseBody);

      int disponibile = powerData.disponibile;
      int fotovoltaico = powerData.fotovoltaico;
      int utilizzo = fotovoltaico - disponibile;

      int textHeight = 40;
      int totHeight = tft.height();
      int totWidth = tft.width();

      int topLeftX = 120;
      int topLeftY = totHeight / 4 - 40;
      
      int topRightX = totWidth / 4 * 3;
      int topRightY = totHeight / 4 - 40;
      
      int bottomCenterX = totWidth / 2;
      int bottomCenterY = totHeight / 4 * 3 - 40;

      tft.fillScreen(TFT_BLACK);

      // Fotovoltaico
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE);
      tft.setCursor(topLeftX - tft.textWidth("FOTOVOLTAICO", 4) / 2, topLeftY, 4);
      tft.println("FOTOVOLTAICO");

      tft.setTextSize(1);
      tft.setTextColor(TFT_YELLOW);
      tft.setCursor(topLeftX - tft.textWidth(String(fotovoltaico), 7) / 2, topLeftY + 40, 7);
      tft.println(String(fotovoltaico));

      // Utilizzo
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE);
      tft.setCursor(topRightX - tft.textWidth("UTILIZZO", 4) / 2, topRightY, 4);
      tft.println("UTILIZZO");

      tft.setTextSize(1);
      tft.setTextColor(TFT_YELLOW);
      tft.setCursor(topRightX - tft.textWidth(String(utilizzo), 7) / 2, topRightY + 40, 7);
      tft.println(String(utilizzo));

      // Disponibile
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE);
      tft.setCursor(bottomCenterX - tft.textWidth("DISPONIBILE", 4) / 2, bottomCenterY, 4);
      tft.println("DISPONIBILE");

      // Change text color based on the value of "disponibile"
      if (disponibile < 0) {
        tft.setTextColor(TFT_RED, TFT_BLACK);
      } else {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
      }

      tft.setTextSize(1);
      tft.setCursor(bottomCenterX - tft.textWidth(String(disponibile), 7) / 2, bottomCenterY + 40, 7);
      tft.println(String(disponibile));

      // Draw progress bar and delay
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
  //delay(5000);
}

