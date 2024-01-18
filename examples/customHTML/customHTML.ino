#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver

#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

FSWebServer myWebServer(FILESYSTEM, 80);
struct tm sysTime;

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Test "options" values
uint8_t ledPin = LED_BUILTIN;
bool boolVar = true;
uint32_t longVar = 1234567890;
float floatVar = 15.5F;
String stringVar = "Test option String";

// Var labels (in /setup webpage)
#define LED_LABEL "The LED pin number"
#define BOOL_LABEL "A bool variable"
#define LONG_LABEL "A long variable"
#define FLOAT_LABEL "A float variable"
#define STRING_LABEL "A String variable"

// ThingsBoard variables
double tb_deviceLatitude = 41.88505;
double tb_deviceLongitude = 12.50050;
String tb_deviceName = "ESP Sensor";
String tb_deviceToken = "xxxxxxxxxxxxxxxxxxx";
String tb_device_key = "xxxxxxxxxxxxxxxxxxx";
String tb_secret_key = "xxxxxxxxxxxxxxxxxxx";
String tb_serverIP = "thingsboard.cloud";
uint16_t tb_port = 80;

#define TB_DEVICE_NAME "Device Name"
#define TB_DEVICE_LAT "Device Latitude"
#define TB_DEVICE_LON "Device Longitude"
#define TB_SERVER "ThingsBoard server address"
#define TB_PORT "ThingsBoard server port"
#define TB_DEVICE_TOKEN "ThingsBoard device token"
#define TB_DEVICE_KEY "Provisioning device key"
#define TB_SECRET_KEY "Provisioning secret key"

/*
* Include the custom HTML, CSS and Javascript to be injected in /setup webpage.
* HTML code will be injected according to the order of options declaration.
* CSS and JavaScript will be appended to the end of body in order to work properly.
* In this manner, is also possible override the default element styles
* like for example background color, margins, paddings etc etc
*/
#include "customElements.h"
#include "thingsboard.h"


////////////////////////////////  Filesystem  /////////////////////////////////////////
void startFilesystem() {
  // FILESYSTEM INIT
  if ( !FILESYSTEM.begin()) {
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    FILESYSTEM.format();
    ESP.restart();
  }
  myWebServer.printFileList(LittleFS, Serial, "/", 2);
}

/*
* Getting FS info (total and free bytes) is strictly related to
* filesystem library used (LittleFS, FFat, SPIFFS etc etc) and ESP framework
* ESP8266 FS implementation has methods for total and used bytes (only label is missing)
*/
#ifdef ESP32
void getFsInfo(fsInfo_t* fsInfo) {
	fsInfo->fsName = "LittleFS";
	fsInfo->totalBytes = LittleFS.totalBytes();
	fsInfo->usedBytes = LittleFS.usedBytes();
}
#else
void getFsInfo(fsInfo_t* fsInfo) {
	fsInfo->fsName = "LittleFS";
}
#endif


////////////////////  Load application options from filesystem  ////////////////////
////////////////////  Load application options from filesystem  ////////////////////
bool loadOptions() {
  if (FILESYSTEM.exists(myWebServer.getConfigFilepath())) {
    // Config file will be opened on the first time we call this method
    myWebServer.getOptionValue(LED_LABEL, ledPin);
    myWebServer.getOptionValue(BOOL_LABEL, boolVar);
    myWebServer.getOptionValue(LONG_LABEL, longVar);
    myWebServer.getOptionValue(FLOAT_LABEL, floatVar);
    myWebServer.getOptionValue(STRING_LABEL, stringVar);
    // Close configuration file and release memory
    myWebServer.closeConfiguration(false);

    Serial.println("\nThis are the current values stored: \n");
    Serial.printf("LED pin value: %d\n", ledPin);
    Serial.printf("Bool value: %s\n", boolVar ? "true" : "false");
    Serial.printf("Long value: %d\n",longVar);
    Serial.printf("Float value: %d.%d\n", (int)floatVar, (int)(floatVar*1000)%1000);
    Serial.printf("String value: %s\n", stringVar.c_str());
    return true;
  }
  else
    Serial.println(F("Config file not exist"));
  return false;
}


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

  // Try to connect to stored SSID, start AP if fails after timeout
  myWebServer.setAP("ESP_AP", "123456789");
  IPAddress myIP = myWebServer.startWiFi(15000);

  // Configure /setup page and start Web Server
  myWebServer.addOptionBox("My Options");
  myWebServer.addOption(LED_LABEL, ledPin);
  myWebServer.addOption(LONG_LABEL, longVar);
  // Float fields can be configured with min, max and step properties
  myWebServer.addOption(FLOAT_LABEL, floatVar, 0.0, 100.0, 0.01);
  myWebServer.addOption(STRING_LABEL, stringVar);
  myWebServer.addOption(BOOL_LABEL, boolVar);

  // Add a new options box with custom code injected
  myWebServer.addOptionBox("Custom HTML");
  // Add HTML block where you need (for example one in different option box)
  myWebServer.addHTML(custom_html, "fetch", /*overwite*/ true);

  // Add a new options box
  myWebServer.addOptionBox("ThingsBoard");
  myWebServer.addOption(TB_DEVICE_NAME, tb_deviceName);
  myWebServer.addOption(TB_DEVICE_LAT, tb_deviceLatitude, -180.0, 180.0, 0.00001);
  myWebServer.addOption(TB_DEVICE_LON, tb_deviceLongitude, -180.0, 180.0, 0.00001);
  myWebServer.addOption(TB_SERVER, tb_serverIP);
  myWebServer.addOption(TB_PORT, tb_port);
  myWebServer.addOption(TB_DEVICE_KEY, tb_device_key);
  myWebServer.addOption(TB_SECRET_KEY, tb_secret_key);
  myWebServer.addOption(TB_DEVICE_TOKEN, tb_deviceToken);
  // Add HTML block where you need (for example one in different option box)
  myWebServer.addHTML(thingsboard_htm, "ts", /*overwrite file*/ false);

  // CSS and Javascript will be appended to head and body
  // (add as last to prevent wrong /setup page  aspect)
  myWebServer.addJavascript(thingsboard_script, "ts", /*overwrite file*/ false);
  myWebServer.addCSS(custom_css, "fetch", /*overwite*/ false);
  myWebServer.addJavascript(custom_script, "fetch", /*overwite*/ false);

  // Add custom logo to /setup page with custom size
  myWebServer.setLogoBase64(base64_logo, "128", "128", /*overwrite file*/ false);

  // set /setup and /edit page authentication
  // myWebServer.setAuthentication("admin", "admin");

  // Enable ACE FS file web editor and add FS info callback function
  myWebServer.enableFsCodeEditor(getFsInfo);

  // Start webserver
  myWebServer.begin();
  Serial.print(F("\nESP Web Server started on IP Address: "));
  Serial.println(myIP);
  Serial.println(F("Open /setup page to configure optional parameters"));
  Serial.println(F("Open /edit page to view and edit files"));
}


void loop() {
  myWebServer.run();
}