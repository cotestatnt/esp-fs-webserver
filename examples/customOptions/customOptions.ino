#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver

#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Test "options" values
bool boolVar = true;
uint32_t longVar = 1234567890;
String stringVar = "Test option String";
uint8_t ledPin = LED_BUILTIN;
float floatVar = 15.5F;

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
struct tm Time;

#ifdef ESP8266
ESP8266WebServer server(80);
#elif defined(ESP32)
WebServer server(80);
#endif

FSWebServer myWebServer(FILESYSTEM, server);


////////////////////////////////  Filesystem  /////////////////////////////////////////
void startFilesystem() {
  // FILESYSTEM INIT
  if ( FILESYSTEM.begin()) {
    File root = FILESYSTEM.open("/", "r");
    File file = root.openNextFile();
    while (file) {
      const char* fileName = file.name();
      size_t fileSize = file.size();
      Serial.printf("FS File: %s, size: %lu\n", fileName, (long unsigned)fileSize);
      file = root.openNextFile();
    }
    Serial.println();
  }
  else {
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    FILESYSTEM.format();
    ESP.restart();
  }
}


////////////////////  Load application options from filesystem  ////////////////////
bool loadOptions() {

  if (FILESYSTEM.exists("/config.json")) {
    myWebServer.getOptionValue("LED pin", ledPin);
    myWebServer.getOptionValue("A bool var", boolVar);
    myWebServer.getOptionValue("A long var", longVar);
    myWebServer.getOptionValue("A float var", floatVar);
    myWebServer.getOptionValue("A String var", stringVar);

    Serial.println();
    Serial.printf("LED pin value: %d\n", ledPin);
    Serial.printf("Bool value: %d\n", boolVar);
    Serial.printf("Long value: %l\n",longVar);
    Serial.println(floatVar);
    Serial.println(stringVar);
    return true;
  }
  else
    Serial.println(F("File \"config.json\" not exist"));

  return false;
}


void setup() {
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to flash stored SSID, start AP if fails after timeout
  IPAddress myIP = myWebServer.startWiFi(15000, "ESP8266_AP", "123456789" );

  // Load configuration (if not present, default will be created when webserver will start)
  if (loadOptions())
    Serial.println(F("Application option loaded"));
  else
    Serial.println(F("Application options NOT loaded!"));

  // Configure /setup page and start Web Server
  myWebServer.addOption("LED Pin", ledPin);
  myWebServer.addOption("A long var", longVar);
  myWebServer.addOption("A float var", floatVar, 0.0, 100.0, 0.01);
  myWebServer.addOption("A String var", stringVar.c_str());
  myWebServer.addOption("A bool var", boolVar);

  if (myWebServer.begin()) {
    Serial.print(F("ESP Web Server started on IP Address: "));
    Serial.println(myIP);
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
    Serial.println(F("Open /update page to upload firmware and filesystem updates"));
  }
}


void loop() {
  myWebServer.run();
}
