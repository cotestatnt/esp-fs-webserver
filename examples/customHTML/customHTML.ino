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

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
struct tm Time;

#ifdef ESP8266
ESP8266WebServer server(80);
#elif defined(ESP32)
WebServer server(80);
#endif

FSWebServer myWebServer(FILESYSTEM, server);

static const char logo_svg[] PROGMEM = R"EOF(
<svg style="width:128px;height:128px" viewBox="0 0 24 24">
    <path fill="currentColor" d="M16.36,14C16.44,13.34 16.5,12.68 16.5,12C16.5,11.32 16.44,10.66 16.36,10H19.74C19.9,10.64 20,11.31 20,12C20,12.69 19.9,13.36 19.74,14M14.59,19.56C15.19,18.45 15.65,17.25 15.97,16H18.92C17.96,17.65 16.43,18.93 14.59,19.56M14.34,14H9.66C9.56,13.34 9.5,12.68 9.5,12C9.5,11.32 9.56,10.65 9.66,10H14.34C14.43,10.65 14.5,11.32 14.5,12C14.5,12.68 14.43,13.34 14.34,14M12,19.96C11.17,18.76 10.5,17.43 10.09,16H13.91C13.5,17.43 12.83,18.76 12,19.96M8,8H5.08C6.03,6.34 7.57,5.06 9.4,4.44C8.8,5.55 8.35,6.75 8,8M5.08,16H8C8.35,17.25 8.8,18.45 9.4,19.56C7.57,18.93 6.03,17.65 5.08,16M4.26,14C4.1,13.36 4,12.69 4,12C4,11.31 4.1,10.64 4.26,10H7.64C7.56,10.66 7.5,11.32 7.5,12C7.5,12.68 7.56,13.34 7.64,14M12,4.03C12.83,5.23 13.5,6.57 13.91,8H10.09C10.5,6.57 11.17,5.23 12,4.03M18.92,8H15.97C15.65,6.75 15.19,5.55 14.59,4.44C16.43,5.07 17.96,6.34 18.92,8M12,2C6.47,2 2,6.5 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z" />
</svg>
)EOF";

static const char button_html[] PROGMEM = R"EOF(
<button class='button' onclick=window.open('/restart')>Restart ESP</button>
<button class='button' style= 'background-color: crimson;' onclick=window.open('/config')>Open link</button>
)EOF";

static const char info_html[] PROGMEM = R"EOF(
<br><br><div>Save options if needed, then insert WiFi credentials and connect to SSID</div><hr>
)EOF";

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
  myWebServer.addOption(FILESYSTEM, "A bool var", boolVar);
  myWebServer.addOption(FILESYSTEM, "LED Pin", ledPin);
  myWebServer.addOption(FILESYSTEM, "A long var", longVar);
  myWebServer.addOption(FILESYSTEM, "A String var", stringVar);

  // Add custom HTML to setup page
  myWebServer.addOption(FILESYSTEM, "raw-html-button", button_html);
  myWebServer.addOption(FILESYSTEM, "raw-html-info", info_html);

  // Add hidden fields to customize your setup page
  myWebServer.addOption(FILESYSTEM, "logo-name", "Custom HTML example", true);
  myWebServer.addOption(FILESYSTEM, "logo-svg", logo_svg, true);
  
  if (myWebServer.begin()) {
    Serial.print(F("ESP Web Server started on IP Address: "));
    Serial.println(myIP);
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
    Serial.println(F("Open /update page to upload firmware and filesystem updates"));
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
