#include <esp-fs-webserver.h>  // https://github.com/cotestatnt/esp-fs-webserver

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

// In order to set SSID and password open the /setup webserver page
// const char* ssid;
// const char* password;

uint8_t ledPin = LED_BUILTIN;
bool apMode = false;
char* hostname = "fsbrowser";

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

////////////////////////////  HTTP Request Handlers  ////////////////////////////////////
void handleLed() {
  // If new led state is specified - http://xxx.xxx.xxx.xxx/led?val=1
  if(myWebServer.webserver->hasArg("val")) {
    int value = myWebServer.webserver->arg("val").toInt();
    digitalWrite(ledPin, value);
  }
  // else simple toggle the actual state
  else {
    digitalWrite(ledPin, !digitalRead(ledPin));
  }
  String reply = "LED is now ";
  reply += digitalRead(ledPin) ? "OFF" : "ON";
  myWebServer.webserver->send(200, "text/plain", reply);
}


void setup(){
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to flash stored SSID, start AP if fails after timeout
  IPAddress myIP = myWebServer.startWiFi(15000, "ESP8266_AP", "123456789" );

  // Add custom page handlers to webserver
  myWebServer.addHandler("/led", HTTP_GET, handleLed);

  // Start webserver
  if (myWebServer.begin()) {
    Serial.print(F("ESP Web Server started on IP Address: "));
    Serial.println(myIP);
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
  }

  pinMode(LED_BUILTIN, OUTPUT);
}


void loop() {
  myWebServer.run();
  
}