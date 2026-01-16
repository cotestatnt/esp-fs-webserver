/**
 * @file leanWebserver.ino
 * @brief Example of a lean ESP webserver using FSWebServer and LittleFS.
 *
 * This sketch demonstrates how to configure the FSWebServer library for minimal resource usage
 * by disabling unneeded features such as the built-in file editor, setup page, setup HTML, and websocket server.
 * It connects to a WiFi network using provided SSID and password, mounts the LittleFS filesystem,
 * and starts a web server on port 80.
 *
 * Features:
 * - Minimal webserver configuration for ESP boards.
 * - Serves files from LittleFS.
 * - HTTP endpoint `/led` to control the built-in LED via GET requests (e.g., `/led?val=1`).
 * - Serial output for debugging and status.
 *
 * Usage:
 * - Update `ssid` and `password` with your WiFi credentials.
 * - Access the device's IP address in a browser to interact with the server.
 * - Use `/led?val=1` or `/led?val=0` to turn the LED on or off.
 *
 * Note:
 * - The built-in `/setup` and `/edit` pages are disabled for a leaner firmware.
 * - If the filesystem fails to mount, it will be formatted and the device will restart.
 * - mDNS is kept enabled for captive portal redirection, but can be disabled if not needed.
 *
 * @author cotestatnt
 * @see https://github.com/cotestatnt/async-esp-fs-webserver
 */

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

// Configuration to disable unneeded features for a lean webserver

// #define ESP_FS_WS_MDNS 0     // Disable mDNS responder
// Keep MDNS active so OS captive portal redirection works (even if useless without setup page)

#define ESP_FS_WS_EDIT 0        // Disable built-in file editor
#define ESP_FS_WS_SETUP 0       // Disable built-in setup page handlers
#define ESP_FS_WS_SETUP_HTM 0   // Disable built-in setup page HTML 
#define ESP_FS_WS_WEBSOCKET 0   // Disable built-in websocket server

#include <FSWebServer.h>  //https://github.com/cotestatnt/async-esp-fs-webserver

#define FILESYSTEM LittleFS
FSWebServer server(FILESYSTEM, 80, "myserver");

// Define built-in LED if not defined by board (eg. generic dev boards)
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

/*
  In this example the built-in /setup page is disabled as for /edit page.
  You can still try to start the captive portal, but it will redirect to inesistent /setup 
  (or other URL you provide), so user will see 404 error until you load your own pages.

  For this reason, this example uses the usual WiFi connection method.
*/
const char* ssid = "your-ssid";
const char* password = "your-password";

////////////////////////////////  Filesystem  /////////////////////////////////////////
bool startFilesystem() {
  if (FILESYSTEM.begin()){
    server.printFileList(FILESYSTEM, "/", 2);
    return true;
  }
  else {
    Serial.println("ERROR on mounting filesystem. It will be reformatted!");
    FILESYSTEM.format();
    ESP.restart();
  }
  return false;
}

///////////////////////////////  HTTP endpoint  ///////////////////////////////////////
void handleLed() {
  static int value = false;
  // http://xxx.xxx.xxx.xxx/led?val=1
  if(server.hasArg("val")) {
    value = server.arg("val").toInt();
    digitalWrite(LED_BUILTIN, value);
  }
  String reply = "LED is now ";
  reply += value ? "ON" : "OFF";
  server.send(200, "text/plain", reply);
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();
  
  WiFi.begin("your-ssid", "your-password");
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  // Try to connect to WiFi (will start AP if not connected after timeout)
  // if (!server.startWiFi(10000)) {
  //   Serial.println("\nWiFi not connected! Starting AP mode...");
  //   server.startCaptivePortal("ESP_AP", "123456789", "/setup");
  // }

  // Define HTTP endpoints
  server.on("/led", HTTP_GET, handleLed);
  
  // Start server
  server.begin();
  Serial.print(F("\nESP Web Server started on IP Address: "));
  Serial.println(server.getServerIP());
  Serial.println(F(
      "\nThis is \"customOptions.ino\" example.\n"
      "Open /setup page to configure optional parameters.\n"
      "Open /edit page to view, edit or upload example or your custom webserver source files."
  ));

  Serial.print(F("Compile time (default firmware version): "));
  Serial.println(BUILD_TIMESTAMP);
}

void loop() {
  // Handle client requests
  server.run(); 

  // Nothing to do here, just a small delay for task yield
  delay(10);  
}
