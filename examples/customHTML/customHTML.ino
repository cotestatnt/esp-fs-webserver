#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver

#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

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


static const char custom_html[] PROGMEM = R"EOF(
<style>
  pre {
      font-family: Monaco,Menlo,Consolas,"Courier New",monospace;
      color: #333;
      line-height: 20px;
      background-color: #f5f5f5;
      border: 1px solid rgba(0,0,0,0.15);
      border-radius: 6px;
      overflow-y: scroll;
      min-height: 350px;
      font-size: 85%;
  }
  .select {
    width: 15%;
    height:40px;
    padding-top: 10px;
    padding-left: 20px;
    border:1px solid #ccc;
    border-radius: 6px;
    box-shadow: 0 1px 2px 0 rgba(220, 220, 230, 0.5);
  }
</style>

<form class=form>
  <div class=tf-wrapper>
    <label for=httpmethod class=input-label>Method</label>
    <select class="select" id="httpmethod">
      <option>GET</option>
      <option>POST</option>
    </select>
    <label for=url class=input-label>Endpoint</label>
    <input type=text placeholder="https://httpbin.org/" id=url value="https://httpbin.org/" />
  </div>
  <br>
  <a id=fetch class="btn">
    <span>Fecth url</span>
  </a>
  <br>
  <pre id=payload></pre>
</form>
)EOF";

static const char custom_script[] PROGMEM = R"EOF(
function fetchEndpoint() {
  var mt = $('httpmethod').options[$('httpmethod').selectedIndex].text
  var url = $('url').value + mt.toLowerCase();
  var bd = (mt != 'GET') ? 'body: ""' : '';

  var options = {
    method: mt,
    bd
  };

  fetch(url, options)
  .then(response => response.text())
  .then(txt => {
    $('payload').innerHTML = txt;
  });
}

$('fetch').addEventListener('click', fetchEndpoint);
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


void setup() {
  Serial.begin(115200);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to stored SSID, start AP if fails after timeout
  IPAddress myIP = myWebServer.startWiFi(15000, "ESP_AP", "123456789" );

  // Load configuration (if not present, default will be created when webserver will start)
  if (loadOptions())
    Serial.println(F("Application option loaded"));
  else
    Serial.println(F("Application options NOT loaded!"));

  // Configure /setup page and start Web Server
  myWebServer.addOptionBox("My Options");
  myWebServer.addOption(LED_LABEL, ledPin);
  myWebServer.addOption(LONG_LABEL, longVar);
  myWebServer.addOption(FLOAT_LABEL, floatVar, 0.0, 100.0, 0.01);
  myWebServer.addOption(STRING_LABEL, stringVar);
  myWebServer.addOption(BOOL_LABEL, boolVar);
  myWebServer.addOptionBox("Custom HTML");
  myWebServer.addHTML(custom_html, "custom-html");
  myWebServer.addJavascript(custom_script);

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
