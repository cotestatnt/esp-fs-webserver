# ESP-FS-WebServer

A library for ESP8266/ESP32 that provides a web server with an integrated file system browser, WiFi configuration manager, and support for WebSockets. This library is based on the synchronous `WebServer` class.

![WiFi Manager](docs/wifi_manager.png)

## Features

-   **File System Management**: Browse, view, upload, and delete files on your ESP's file system (LittleFS, SPIFFS, etc.) directly from a web browser.
-   **WiFi Configuration**: An integrated setup page (`/setup`) allows you to scan for WiFi networks and connect the ESP to your local network.
-   **Customizable Options**: Easily add your own configuration parameters (text boxes, checkboxes, sliders) to the setup page, which are saved to a JSON file.
-   **WebSocket Support**: Built-in support for WebSocket communication.
-   **OTA Updates**: Allows for firmware updates through the web interface.

## Documentation

For more detailed information, please refer to the documentation in the `docs` folder:

-   **[API Reference](docs/API.md)** – Detailed overview of the public methods.
-   **[Setup and WiFi](docs/SetupAndWiFi.md)** – Guide to `startWiFi()`, captive portal, and the `/setup` page.
-   **[Filesystem and Editor](docs/FileEditorAndFS.md)** – How to serve static files and use the `/edit` page.
-   **[WebSocket](docs/WebSocket.md)** – Information on using the WebSocket server.

## Dependencies

-   ESP8266/ESP32 Core for Arduino
-   [WebSockets](https://github.com/Links2004/arduino-WebSockets) by Markus Sattler

## Basic Usage

```cpp
#include <FS.h>
#include <LittleFS.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include "FSWebServer.h"

// Use LittleFS
fs::FS& filesystem = LittleFS;

FSWebServer server(80, filesystem, "esphost");

void setup() {
  Serial.begin(115200);

  // Initialize Filesystem
  if (!filesystem.begin()) {
    Serial.println("Error mounting file system.");
    return;
  }

  // Start WiFi (or captive portal if no credentials)
  server.startWiFi(10000);

  // Add a handler for the root page
  server.on("/", []() {
    server.send(200, "text/html", "<h1>Hello from FSWebServer!</h1>");
  });

  // Start the server
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.run();
}
```
