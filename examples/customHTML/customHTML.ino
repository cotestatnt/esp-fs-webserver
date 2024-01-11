#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver

#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Set this to 1 if you want clear the /config.json file at startup
#define CLEAR_OTIONS 0

struct tm sysTime;

// Test "options" values
uint8_t ledPin = LED_BUILTIN;
bool boolVar = true;
uint32_t longVar = 1234567890;
float floatVar = 15.5F;
String stringVar = "Test option String";

// ThingsBoard varaibles
String tb_deviceToken = "xxxxxxxxxxxxxxxxxxx";
String tb_device_key = "xxxxxxxxxxxxxxxxxxx";
String tb_secret_key = "xxxxxxxxxxxxxxxxxxx";
String tb_serverIP = "192.168.1.1";
uint16_t tb_port = 8181;

// Var labels (in /setup webpage)
#define LED_LABEL "The LED pin number"
#define BOOL_LABEL "A bool variable"
#define LONG_LABEL "A long variable"
#define FLOAT_LABEL "A float variable"
#define STRING_LABEL "A String variable"

#define TB_SERVER "ThingsBoard server address"
#define TB_PORT "ThingsBoard server port"
#define TB_DEVICE_TOKEN "ThingsBoard device token"
#define TB_DEVICE_KEY "Provisioning device key"
#define TB_SECRET_KEY "Provisioning secret key"

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
struct tm Time;

#ifdef ESP8266
ESP8266WebServer server(80);
#elif defined(ESP32)
WebServer server(80);
#endif

FSWebServer myWebServer(FILESYSTEM, server);


/*
* Include the custom HTML, CSS and Javascript to be injected in /setup webpage.
* HTML code will be injected according to the order of options declaration.
* CSS and JavaScript will be appended to the end of body in order to work properly.
* In this manner, is also possible override the default element styles
* like for example background color, margins, paddings etc etc
*/
#include "customElements.h"

////////////////////////////////  Filesystem  /////////////////////////////////////////
void startFilesystem(){
  // FILESYSTEM INIT
  if ( !FILESYSTEM.begin()){
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    FILESYSTEM.format();
    ESP.restart();
  }
}


////////////////////  Load application options from filesystem  ////////////////////
/*
* Unlike what was done in customOptions.ino, in this example
* the variables are read (and written) all at once using the ArduinoJon library
*/
bool loadOptions() {
  if (FILESYSTEM.exists(myWebServer.getConfigFile())) {
    File file = FILESYSTEM.open(myWebServer.getConfigFile(), "r");
    DynamicJsonDocument doc(file.size() * 1.33);
    if (!file)
      return false;

    DeserializationError error = deserializeJson(doc, file);
    if (error)
      return false;

    ledPin = doc[LED_LABEL];
    boolVar = doc[BOOL_LABEL];
    longVar = doc[LONG_LABEL];
    floatVar = doc[FLOAT_LABEL]["value"];
    stringVar = doc[STRING_LABEL].as<String>();

    tb_deviceToken = doc[TB_DEVICE_TOKEN].as<String>();
    tb_device_key = doc[TB_DEVICE_KEY].as<String>();
    tb_secret_key = doc[TB_SECRET_KEY].as<String>();
    tb_serverIP = doc[TB_SERVER].as<String>();
    tb_port = doc[TB_PORT];

    file.close();

    Serial.println();
    Serial.printf("LED pin value: %d\n", ledPin);
    Serial.printf("Bool value: %d\n", boolVar);
    Serial.printf("Long value: %d\n", longVar);
    Serial.printf("Float value: %d.%d\n", (int)floatVar, (int)(floatVar*1000)%1000);
    Serial.println(stringVar);
    return true;
  }
  else
    Serial.println(F("Configuration file not exist"));
  return false;
}

// Call this if you need to save parameters from the sketch side
// bool saveOptions() {
//   if (FILESYSTEM.exists(myWebServer.configFile())) {
//     File file = FILESYSTEM.open(myWebServer.configFile(), "w");
//     if (!file)
//       return false;

//     DynamicJsonDocument doc(file.size() * 1.33);
//     // Deserialize first, otherwise unhandled or hidden options will be lost
//     DeserializationError error = deserializeJson(doc, file);
//     if (error)
//       return false;

//     doc[LED_LABEL] = ledPin;
//     doc[BOOL_LABEL] = boolVar;
//     doc[LONG_LABEL] = longVar;
//     doc[FLOAT_LABEL]["value"] = floatVar;
//     doc[STRING_LABEL] = stringVar;
//     serializeJsonPretty(doc, file);
//     file.close();
//     return true;
//   }
//   else
//     Serial.println(F("Configuration file not exist"));
//   return false;
// }


void setup() {
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();

  // Load configuration (if not present, default will be created when webserver will start)
#if CLEAR_OPTIONS
  if (myWebServer.clearOptions())
    ESP.restart();
#endif
  if (loadOptions())
    Serial.println(F("Application option loaded\n\n"));
  else
    Serial.println(F("Application options NOT loaded!\n\n"));

  // Configure /setup page and start Web Server

  // Add a new options box
  myWebServer.addOptionBox("My Options");
  myWebServer.addOption(LED_LABEL, ledPin);
  myWebServer.addOption(LONG_LABEL, longVar);
  // Float fields can be configured with min, max and step properties
  myWebServer.addOption(FLOAT_LABEL, floatVar, 0.0, 100.0, 0.01);
  myWebServer.addOption(STRING_LABEL, stringVar);
  myWebServer.addOption(BOOL_LABEL, boolVar);

  // Add a new options box
  myWebServer.addOptionBox("ThingsBoard");
  myWebServer.addOption(TB_SERVER, tb_serverIP);
  myWebServer.addOption(TB_PORT, tb_port);
  myWebServer.addOption(TB_DEVICE_KEY, tb_device_key);
  myWebServer.addOption(TB_SECRET_KEY, tb_secret_key);
  myWebServer.addOption(TB_DEVICE_TOKEN, tb_deviceToken);

  // Add a new options box with custom code injected
  myWebServer.addOptionBox("Custom HTML");
  // How many times you need (for example one in different option box)
  myWebServer.addHTML(custom_html, "fetch-test", /*overwite*/ true);
  // Only once (CSS and Javascript will be appended to head and body)
  myWebServer.addCSS(custom_css, "css", /*overwite*/ false);
  myWebServer.addJavascript(custom_script, "script", /*overwite*/ false);

  // Try to connect to stored SSID, start AP if fails after timeout
  myWebServer.setAP("ESP_AP", "123456789");
  IPAddress myIP = myWebServer.startWiFi(15000);

  // Start webserver
  if (myWebServer.begin()) {
    Serial.print(F("\nESP Web Server started on IP Address: "));
    Serial.println(myIP);
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
  }
}


void loop() {
  myWebServer.run();
}
