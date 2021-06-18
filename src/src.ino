#include <ArduinoJson.h>
#include <esp_webserver.h>

#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

// In order to set SSID and password open the /setup webserver page
// const char* ssid;
// const char* password;

bool apMode = false;
String hostname = "fsbrowser";

// Test "config" values
String option1 = "Test option String";
uint32_t option2 = 1234567890;
uint8_t ledPin = LED_BUILTIN;

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

ESP8266WebServer server(80);
WebServer myWebServer(FILESYSTEM, server);
struct tm Time;

////////////////////////////////  NTP Time  /////////////////////////////////////
void getUpdatedtime(const uint32_t timeout)
{
  uint32_t start = millis();
  Serial.print("Sync time...");
  while (millis() - start < timeout  && Time.tm_year <= (1970 - 1900)) {
    time_t now = time(nullptr);
    Time = *localtime(&now);
    delay(5);
  }
  Serial.println(" done.");
}

////////////////////////////////  WiFi  /////////////////////////////////////////
IPAddress startWiFi(){
  IPAddress myIP;
  Serial.printf("Connecting to %s\n", WiFi.SSID().c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  // WiFi.begin(ssid, password);
  uint32_t startTime = millis();
  while (WiFi.status() != WL_CONNECTED ){
    delay(500);
    Serial.print(".");

    // If no connection (or specifically activated) go in Access Point mode
    if( millis() - startTime > 10000 || apMode ) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP("ESP8266_AP", "123456789");
      myIP = WiFi.softAPIP();
      Serial.print(F("\nAP mode.\nServer IP address: "));
      Serial.println(myIP);
      break;
    }
  }

  if(WiFi.status() == WL_CONNECTED) {
    myIP = WiFi.localIP();
    Serial.print(F("\nConnected! IP address: "));
    Serial.println(myIP);

    // Set hostname, timezone and NTP servers
  #ifdef ESP8266
    WiFi.hostname(hostname);
    configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  #elif defined(ESP32)
    WiFi.setHostname(hostname);
    configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  #endif

    // Sync time with NTP. Blocking, but with timeout (0 == no timeout)
    getUpdatedtime(10000);
    char buffer[30];
    strftime (buffer, 30, "%d/%m/%Y - %X", &Time);
    Serial.printf("Synced time: %s\n", buffer);

    // Start MDNS responder
    if (MDNS.begin(hostname)) {
      Serial.println(F("MDNS responder started."));
      Serial.printf("You should be able to connect with address\t http://%s.local/\n", hostname.c_str());
      // Add service to MDNS-SD
      MDNS.addService("http", "tcp", 80);
    }
  }
  return myIP;
}

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
}


////////////////////  Load and save application configuration from filesystem  ////////////////////
void saveApplicationConfig(){
  StaticJsonDocument<1024> doc;
  File file = FILESYSTEM.open("/config.json", "w");
  doc["hostname"] = hostname;
  doc["Option 1"] = option1;
  doc["Option 2"] = option2;
  doc["AP mode"] = apMode;
  doc["LED Pin"] = ledPin;
  serializeJsonPretty(doc, file);
  file.close();
  delay(1000);
  ESP.restart();
}

void loadApplicationConfig() {
  StaticJsonDocument<1024> doc;
  File file = FILESYSTEM.open("/config.json", "r");
  if (file) {
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (!error) {
      Serial.println(F("Deserializing JSON.."));
      hostname = doc["hostname"].as<String>();
      apMode = doc["AP mode"];
      option1 = doc["Option 1"].as<String>();
      option2 = doc["Option 2"];
      ledPin = doc["LED Pin"];
    }
    else {
      Serial.println(F("Failed to deserialize JSON. File could be corrupted"));
      Serial.println(error.c_str());
      saveApplicationConfig();
    }
  }
  else {
    saveApplicationConfig();
    Serial.println(F("New file created with default values"));
  }
}


void handleLed() {
  // If new led state is specifically setted - http://xxx.xxx.xxx.xxx/led?val=1
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

  // Load configuration (if not present, default will be created)
  loadApplicationConfig();

  // WiFi INIT
  IPAddress myIP = startWiFi();

  // Configure /setup page and start Web Server
  myWebServer.addOption(FILESYSTEM, "hostname", hostname);
  myWebServer.addOption(FILESYSTEM, "AP mode", apMode);
  myWebServer.addOption(FILESYSTEM, "LED Pin", ledPin);
  myWebServer.addOption(FILESYSTEM, "Option 1", option1.c_str());
  myWebServer.addOption(FILESYSTEM, "Option 2", option2);
  // Add custom page handlers
  myWebServer.webserver->on("/led", HTTP_GET, handleLed);
  if (myWebServer.begin()) {
    Serial.print(F("ESP Web Server started on IP Address"));
    Serial.println(myIP);
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
  }

  pinMode(LED_BUILTIN, OUTPUT);
}


void loop() {
  myWebServer.run();

  if(WiFi.status() == WL_CONNECTED) {
    MDNS.update();
  }
}