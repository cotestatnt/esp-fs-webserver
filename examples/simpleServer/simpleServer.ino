#include <FS.h>
#include <LittleFS.h>
#include "FSWebServer.h"

FSWebServer server(80, LittleFS, "myServer");
uint16_t testInt = 150;
float testFloat = 123.456f;

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
const uint8_t ledPin = LED_BUILTIN;


////////////////////////////////  Filesystem  /////////////////////////////////////////
bool startFilesystem() {
  if (LittleFS.begin()) {
    server.printFileList(LittleFS, "/", 1);
    Serial.println();
    return true;
  } else {
    Serial.println("ERROR on mounting filesystem. It will be reformatted!");
    LittleFS.format();
    ESP.restart();
  }
  return false;
}

////////////////////////////  Load custom options //////////////////////////////////////
bool loadApplicationConfig() {
  if (LittleFS.exists(server.getConfiFileName())) {
    // Test "options" values
    server.getOptionValue("Test int variable", testInt);
    server.getOptionValue("Test float variable", testFloat);
    return true;
  }
  return false;
}


/*
* Getting FS info (total and free bytes) is strictly related to
* filesystem library used (LittleFS, FFat, SPIFFS etc etc) and ESP framework
*/
#ifdef ESP32
void getFsInfo(fsInfo_t* fsInfo) {
  fsInfo->fsName = "LittleFS";
  fsInfo->totalBytes = LittleFS.totalBytes();
  fsInfo->usedBytes = LittleFS.usedBytes();
}
#endif


//---------------------------------------
void handleLed(AsyncWebServerRequest* request) {
  static int value = false;
  // http://xxx.xxx.xxx.xxx/led?val=1
  if (request->hasParam("val")) {
    value = request->arg("val").toInt();
    digitalWrite(ledPin, value);
  }
  String reply = "LED is now ";
  reply += value ? "ON" : "OFF";
  request->send(200, "text/plain", reply);
}


void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  delay(1000);
  if (startFilesystem()) {
    // Load application config options
    if (loadApplicationConfig()) {
      Serial.printf("Stored \"testInt\" value: %d\n", testInt);
      Serial.printf("Stored \"testFloat\" value: %3.3f\n", testFloat);
    }
  } else
    Serial.println("LittleFS error!");

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    server.startCaptivePortal("ESP_AP", "123456789", "/setup");
  }

  // Add custom application options tab and set custom title
  server.addOptionBox("Custom options");
  server.addOption("Test int variable", testInt);
  server.addOption("Test float variable", (double)testFloat, 0.0, 100.0, 0.001);
  server.setSetupPageTitle("Simple Async ESP FS WebServer");

  // Enable ACE FS file web editor and add FS info callback function
#ifdef ESP32
  server.enableFsCodeEditor(getFsInfo);
#else
  server.enableFsCodeEditor();
#endif

  // Custom endpoint handler
  server.on("/led", HTTP_GET, handleLed);

  // Start server
  server.begin();
  Serial.print(F("\nAsync ESP Web Server started on IP Address: "));
  Serial.println(server.getServerIP());
  Serial.println(F(
    "This is \"simpleServer.ino\" example.\n"
    "Open /setup page to configure optional parameters.\n"
    "Open /edit page to view, edit or upload example or your custom webserver source files."));
}

void loop() {
  // This delay is required in order to avoid loopTask() WDT reset on ESP32
  server.handleClient();
  delay(10);
}
