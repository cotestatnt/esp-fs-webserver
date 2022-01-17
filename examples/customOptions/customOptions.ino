#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver

#include <FS.h>
#ifdef ESP8266
  #include <LittleFS.h>
  #define FILESYSTEM LittleFS
#elif defined(ESP32)
  #include <FFat.h>
  #define FILESYSTEM FFat
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Test "options" values
bool boolVar = true;
uint32_t longVar = 1234567890;
String stringVar = "Test option String";
uint8_t ledPin = LED_BUILTIN;

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
void startFilesystem(){
  // FILESYSTEM INIT
  if ( FILESYSTEM.begin()){
    File root = FILESYSTEM.open("/", "r");
    File file = root.openNextFile();
    while (file){
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
bool loadApplicationConfig() {
  StaticJsonDocument<1024> doc;
  File file = FILESYSTEM.open("/config.json", "r");
  if (file) {
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (!error) {
      Serial.println(F("Deserializing config JSON.."));
      boolVar = doc["A bool var"];
      stringVar = doc["A String var"].as<String>();
      longVar = doc["A long var"];
      ledPin = doc["LED Pin"];
      serializeJsonPretty(doc, Serial);
      Serial.println();
      return true;
    }
    else {
      Serial.println(F("Failed to deserialize JSON. File could be corrupted"));
      Serial.println(error.c_str());
    }
  }
  return false;
}


void setup(){
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to flash stored SSID, start AP if fails after timeout
  IPAddress myIP = myWebServer.startWiFi(15000, "ESP8266_AP", "123456789" );

  // Configure /setup page and start Web Server
  myWebServer.addOption(FILESYSTEM, "LED Pin", ledPin);
  myWebServer.addOption(FILESYSTEM, "A long var", longVar);
  myWebServer.addOption(FILESYSTEM, "A String var", stringVar.c_str());
  myWebServer.addOption(FILESYSTEM, "A bool var", boolVar);
  
  if (myWebServer.begin()) {
    Serial.print(F("ESP Web Server started on IP Address: "));
    Serial.println(myIP);
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
  }

  // Load configuration (if not present, default will be created when webserver will start)
  if (loadApplicationConfig()) {
    Serial.println(F("Application option loaded"));
  } 
  else {
     Serial.println(F("Application NOT loaded!"));
     Serial.print(F("Open http://"));
     Serial.print(myIP);
     Serial.println(F("/setup to configure parameters"));
  }
  
}


void loop() {
  myWebServer.run();
}
