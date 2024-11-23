#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <TFT_eSPI.h> 
#include "credentials.h"

WiFiClient wifiClient;
HttpClient httpClient = HttpClient(wifiClient, shelly_ip, 80);

// Initialize the TFT display
TFT_eSPI tft = TFT_eSPI();  // Create an object for the TFT display

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

void drawProgressBar(int progress) {
  int barWidth = tft.width() - 4;  // Width of the progress bar (leaving some margin)
  int barHeight = 10;              // Height of the progress bar
  int x = 2;                       // X position for the progress bar
  int y = tft.height() - barHeight - 2;  // Y position near the bottom

  // Draw a background bar
  tft.fillRect(x, y, barWidth, barHeight, TFT_DARKGREY);

  // Draw the filled progress bar
  int filledWidth = map(progress, 0, 100, 0, barWidth);
  tft.fillRect(x, y, filledWidth, barHeight, TFT_WHITE);
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
  // Initialize TFT display
  tft.init();
  tft.setRotation(3);  // Set rotation to 1 for horizontal orientation
  tft.fillScreen(TFT_BLACK);  // Clear the screen
  tft.setTextColor(TFT_WHITE);  // Set text color to white

  // Initialize Wi-Fi
  initWiFi();
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
}

void loop() {
  int progress = 0;  // Progress variable (0 to 100)
  
  // Make sure WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    // Prepare the request
    String url = "/status";
    httpClient.beginRequest();  // Start the request
    httpClient.get(url);  // Use GET request

    // If using authentication, set the username and password
    if (String(shelly_username) != "" && String(shelly_password) != "") {
      httpClient.sendBasicAuth(shelly_username, shelly_password);
    }

    // End the request
    httpClient.endRequest();

    // Read the response
    int statusCode = httpClient.responseStatusCode();
    String responseBody = httpClient.responseBody();

    if (statusCode > 0) {
      // Simple way to extract power values from JSON response (basic string search)
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

      // Display the power values on the TFT screen
      tft.fillScreen(TFT_BLACK);  // Clear the screen

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

      // Show progress bar incrementally
      for (progress = 0; progress <= 100; progress++) {
        drawProgressBar(progress);
        delay(50);  // Small delay to visualize progress
      }
    } else {
      Serial.println("Error on HTTP request: " + String(statusCode));
    }
  }

  // Wait before sending another request
  //delay(5000);
}
