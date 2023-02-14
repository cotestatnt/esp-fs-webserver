#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver

#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#define BTN_SAVE  5

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
#define FLOAT_LABEL "A float varible"
#define STRING_LABEL "A String variable"

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
struct tm Time;

#ifdef ESP8266
ESP8266WebServer server(80);
#elif defined(ESP32)
WebServer server(80);
#endif

FSWebServer myWebServer(FILESYSTEM, server);


static const char save_btn_htm[] PROGMEM = R"EOF(
<div class="btn-bar">
  <a class="btn" onclick="reload();">Reload options</a>
</div>
)EOF";

static const char button_script[] PROGMEM = R"EOF(
function reload() {
  console.log('Reload configuration options');
  fetch('/reload')
  .then((response) => {
    if (response.ok) {
      openModalMessage('Options loaded', 'Options was reloaded from configuration file');
      return;
    }
    throw new Error('Something goes wrong with fetch');
  })
  .catch((error) => {
    openModalMessage('Error', 'Something goes wrong with your request');
  });
}
)EOF";


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
    myWebServer.getOptionValue(LED_LABEL, ledPin);
    myWebServer.getOptionValue(BOOL_LABEL, boolVar);
    myWebServer.getOptionValue(LONG_LABEL, longVar);
    myWebServer.getOptionValue(FLOAT_LABEL, floatVar);
    myWebServer.getOptionValue(STRING_LABEL, stringVar);

    Serial.println();
    Serial.printf("LED pin value: %d\n", ledPin);
    Serial.printf("Bool value: %d\n", boolVar);
    Serial.printf("Long value: %ld\n",longVar);
    Serial.printf("Float value: %d.%d\n", (int)floatVar, (int)(floatVar*1000)%1000);
    Serial.println(stringVar);
    return true;
  }
  else
    Serial.println(F("File \"config.json\" not exist"));
  return false;
}

void saveOptions() {
  myWebServer.saveOptionValue(LED_LABEL, ledPin);
  myWebServer.saveOptionValue(BOOL_LABEL, boolVar);
  myWebServer.saveOptionValue(LONG_LABEL, longVar);
  myWebServer.saveOptionValue(FLOAT_LABEL, floatVar);
  myWebServer.saveOptionValue(STRING_LABEL, stringVar);
  Serial.println(F("Application options saved."));
}

////////////////////////////  HTTP Request Handlers  ////////////////////////////////////
void handleLoadOptions() {
  WebServerClass* webRequest = myWebServer.getRequest();
  loadOptions();
  Serial.println(F("Application option loaded after web request"));
  webRequest->send(200, "text/plain", "Options loaded");
}


void setup() {
  Serial.begin(115200);
  pinMode(BTN_SAVE, INPUT_PULLUP);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to stored SSID, start AP if fails after timeout
  IPAddress myIP = myWebServer.startWiFi(15000, "ESP8266_AP", "123456789" );

  // Load configuration (if not present, default will be created when webserver will start)
  if (loadOptions())
    Serial.println(F("Application option loaded"));
  else
    Serial.println(F("Application options NOT loaded!"));

  // Add custom page handlers to webserver
  myWebServer.addHandler("/reload", HTTP_GET, handleLoadOptions);

  // Configure /setup page and start Web Server
  myWebServer.addOptionBox("My Options");
  myWebServer.addOption(LED_LABEL, ledPin);
  myWebServer.addOption(LONG_LABEL, longVar);
  myWebServer.addOption(FLOAT_LABEL, floatVar, 0.0, 100.0, 0.01);
  myWebServer.addOption(STRING_LABEL, stringVar);
  myWebServer.addOption(BOOL_LABEL, boolVar);
  myWebServer.addOption("raw-html-button", save_btn_htm);
  myWebServer.addJavascript(button_script);

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

  if (! digitalRead(BTN_SAVE)) {
    saveOptions();
    delay(1000);
  }
}
