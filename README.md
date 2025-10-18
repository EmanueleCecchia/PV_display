# PV_display
esp32 and ILI9488 with Shelly API to display the photovoltaic data

add a file called credentials.h with:

#ifndef CREDENTIALS_H
#define CREDENTIALS_H

// WiFi credentials
const char* ssid = "xxx";
const char* password = "xxx";

// Shelly device credentials
const char* shelly_ip = "xxx";
const char* shelly_username = "xxx";
const char* shelly_password = "xxx";

#endif
