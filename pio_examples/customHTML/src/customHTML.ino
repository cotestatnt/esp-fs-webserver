#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <AsyncFsWebServer.h>   // https://github.com/cotestatnt/async-esp-fs-webserver

const char* hostname = "myserver";
#define FILESYSTEM LittleFS
AsyncFsWebServer server(80, FILESYSTEM, hostname);

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Var labels as shown in /setup webpage
#define LED_LABEL "The LED pin number"
#define BOOL_LABEL "A bool variable"
#define LONG_LABEL "A long variable"
#define FLOAT_LABEL "A float variable"
#define STRING_LABEL "A String variable"
#define DROPDOWN_TEST "A dropdown listbox"
#define BRIGHTNESS_LABEL "A slider control"

#define TB_DEVICE_NAME "Device Name"
#define TB_DEVICE_LAT "Device Latitude"
#define TB_DEVICE_LON "Device Longitude"
#define TB_SERVER "ThingsBoard server address"
#define TB_PORT "ThingsBoard server port"
#define TB_DEVICE_TOKEN "ThingsBoard device token"
#define TB_DEVICE_KEY "Provisioning device key"
#define TB_SECRET_KEY "Provisioning secret key"

// Test "options" values
uint8_t ledPin = LED_BUILTIN;
bool boolVar = true;
bool boolVar2 = false;
uint32_t longVar = 1234567890;
float floatVar = 15.5F;
String stringVar = "Test option String";
String dropdownSelected = "Item1";
AsyncFsWebServer::Slider brightness{ BRIGHTNESS_LABEL, 0.0, 100.0, 1.0, 50.0 };

// ThingsBoard variables
String tb_deviceName = "ESP Sensor";
double tb_deviceLatitude = 41.88505;
double tb_deviceLongitude = 12.50050;
String tb_deviceToken = "xxxxxxxxxxxxxxxxxxx";
String tb_device_key = "xxxxxxxxxxxxxxxxxxx";
String tb_secret_key = "xxxxxxxxxxxxxxxxxxx";
String tb_serverIP = "thingsboard.cloud";
uint16_t tb_port = 80;

/*
* Include the custom HTML, CSS and Javascript to be injected in /setup webpage.
* HTML code will be injected according to the order of options declaration.
* CSS and JavaScript will be appended to the end of body in order to work properly.
* In this manner, is also possible override the default element styles
* like for example background color, margins, padding etc etc
*/
#include "customElements.h"
#include "thingsboard.h"

// Callback: notify user when the configuration file is saved
void onConfigSaved(const char* path) {
  Serial.printf("\n[Config] File salvato: %s\n", path);
}

////////////////////////////////  Filesystem  /////////////////////////////////////////
bool startFilesystem() {
  if (FILESYSTEM.begin()){
    server.printFileList(FILESYSTEM, "/", 1);
    return true;
  }
  else {
    Serial.println("ERROR on mounting filesystem. It will be reformatted!");
    FILESYSTEM.format();
    ESP.restart();
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


////////////////////  Load application options from filesystem  ////////////////////
bool loadOptions() {
  if (FILESYSTEM.exists(server.getConfiFileName())) {
    // Test "options" values
    server.getOptionValue(LED_LABEL, ledPin);
    server.getOptionValue(BOOL_LABEL, boolVar);
    server.getOptionValue(BOOL_LABEL "2", boolVar2);
    server.getOptionValue(LONG_LABEL, longVar);
    server.getOptionValue(FLOAT_LABEL, floatVar);
    server.getOptionValue(STRING_LABEL, stringVar);
    server.getOptionValue(DROPDOWN_TEST, dropdownSelected);
    server.getSliderValue(brightness);

    // ThingsBoard variables
    server.getOptionValue(TB_DEVICE_NAME, tb_deviceName);
    server.getOptionValue(TB_DEVICE_LAT, tb_deviceLatitude);
    server.getOptionValue(TB_DEVICE_LON, tb_deviceLongitude);
    server.getOptionValue(TB_DEVICE_TOKEN, tb_deviceToken);
    server.getOptionValue(TB_DEVICE_KEY, tb_device_key);
    server.getOptionValue(TB_SECRET_KEY, tb_secret_key);
    server.getOptionValue(TB_SERVER, tb_serverIP);
    server.getOptionValue(TB_PORT, tb_port);
    server.closeSetupConfiguration();  // Close configuration to free resources

    Serial.println("\nThis are the current values stored: \n");
    Serial.printf("LED pin value: %d\n", ledPin);
    Serial.printf("Bool value 1: %s\n", boolVar ? "true" : "false");
    Serial.printf("Bool value 2: %s\n", boolVar2 ? "true" : "false");
    Serial.printf("Long value: %u\n", longVar);
    Serial.printf("Float value: %d.%d\n", (int)floatVar, (int)(floatVar*1000)%1000);
    Serial.printf("String value: %s\n", stringVar.c_str());
    Serial.printf("Dropdown selected: %s\n", dropdownSelected.c_str());
    Serial.print("Slider value: "); Serial.println(brightness.value);
    return true;
  }
  else {
      Serial.println("Failed to parse configuration file");            
      return false;
  }
  return true;
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  // FILESYSTEM INIT
  if (startFilesystem()){
    // Load configuration (if not present, default will be created when webserver will start)
    loadOptions();      
  }

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    server.startCaptivePortal("ESP_AP", "123456789", "/setup");
  }

  // Add custom HTTP request handlers to webserver
  server.on("/reload", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Options loaded");
    loadOptions();
    Serial.println("Application option loaded after web request");
  });

  // Add a new options box
  server.addOptionBox("My Options");
  server.addOption(LED_LABEL, ledPin);
  server.addOption(LONG_LABEL, longVar);
  // Float fields can be configured with min, max and step properties
  server.addOption(FLOAT_LABEL, floatVar, 0.0, 100.0, 0.01);
  server.addOption(STRING_LABEL, stringVar);
  server.addOption(BOOL_LABEL, boolVar);
  server.addOption(BOOL_LABEL "2", boolVar2);
  static const char* dropItem[] = {"Item1", "Item2", "Item3"};
  AsyncFsWebServer::DropdownList dropdownDef{ DROPDOWN_TEST, dropItem, 3, 0 };
  server.addDropdownList(dropdownDef);
  server.addSlider(brightness);  

  // Add a new options box with custom code injected
  server.addOptionBox("Custom HTML");
  // How many times you need (for example one in different option box)
  server.addHTML(custom_html, "fetch-test", /*overwrite*/ false);

  // Add a new options box
  server.addOptionBox("ThingsBoard");
  server.addOption(TB_DEVICE_NAME, tb_deviceName);
  server.addOption(TB_DEVICE_LAT, tb_deviceLatitude, -180.0, 180.0, 0.00001);
  server.addOption(TB_DEVICE_LON, tb_deviceLongitude, -180.0, 180.0, 0.00001);
  server.addOption(TB_SERVER, tb_serverIP);
  server.addOption(TB_PORT, tb_port);
  server.addOption(TB_DEVICE_KEY, tb_device_key);
  server.addOption(TB_SECRET_KEY, tb_secret_key);
  server.addOption(TB_DEVICE_TOKEN, tb_deviceToken);
  server.addHTML(thingsboard_htm, "ts", /*overwrite file*/ false);

  // CSS will be appended to HTML head
  server.addCSS(custom_css, "fetch", /*overwrite file*/ false);
  // Javascript will be appended to HTML body
  server.addJavascript(custom_script, "fetch", /*overwrite file*/ false);
  server.addJavascript(thingsboard_script, "ts", /*overwrite file*/ false);

  // Add custom page title to /setup
  server.setSetupPageTitle("Custom HTML Web Server");
  // Add custom logo to /setup page with custom size
  server.setLogoBase64(base64_logo, "128", "128", /*overwrite file*/ false);

  // Enable ACE FS file web editor and add FS info callback function    
#ifdef ESP32
  server.enableFsCodeEditor(getFsInfo);
#else
  server.enableFsCodeEditor();
#endif

  // Inform user when config.json is saved via /edit or /upload
  server.setConfigSavedCallback(onConfigSaved);

  // Start web server
  if (server.init()) {
    Serial.print(F("\n\nWeb Server started on IP Address: "));
    Serial.println(server.getServerIP());
    Serial.println(F(
      "\nThis is \"customHTML.ino\" example.\n"
      "Open /setup page to configure optional parameters.\n"
      "Open /edit page to view, edit or upload example or your custom webserver source files."
    ));
    Serial.printf("Ready! Open http://%s.local in your browser\n", hostname);
    if (server.isAccessPointMode())
      Serial.print(F("Captive portal is running"));
  }

}


void loop() {
  server.handleClient();
  if (server.isAccessPointMode())
    server.updateDNS();
  
  // Nothing to do here, just a small delay for task yield
  delay(10);  
}
