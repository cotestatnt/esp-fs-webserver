
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

// Disable optional features of FSWebServer
#define ESP_FS_WS_MDNS 0
#define ESP_FS_WS_EDIT 0
#define ESP_FS_WS_SETUP 0
#define ESP_FS_WS_SETUP_HTM 0
#define ESP_FS_WS_WEBSOCKET 0
#include <FSWebServer.h>  //https://github.com/cotestatnt/async-esp-fs-webserver

#define FILESYSTEM LittleFS
FSWebServer server(80, FILESYSTEM, "myserver");

// Define built-in LED if not defined by board (eg. generic dev boards)
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#ifndef BOOT_PIN
#define BOOT_PIN    0
#endif


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
  pinMode(BOOT_PIN, INPUT_PULLUP);
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    server.startCaptivePortal("ESP_AP", "123456789", "/setup");
  }

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
  server.handleClient();
  if (server.isAccessPointMode())
    server.updateDNS();

  // Nothing to do here, just a small delay for task yield
  delay(10);  
}
