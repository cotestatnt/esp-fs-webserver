#include <WebSocketsServer.h>   // https://github.com/Links2004/arduinoWebSockets
#include <esp-fs-webserver.h>  // https://github.com/cotestatnt/esp-fs-webserver
#include <ArduinoJson.h>

#include <FS.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS
ESP8266WebServer server(80);
#elif defined(ESP32)
#include <WiFi.h>
#include <FFat.h>
#define FILESYSTEM FFat
WebServer server(80);
#endif

FSWebServer myWebServer(FILESYSTEM, server);
WebSocketsServer webSocket = WebSocketsServer(81);


// Define a struct for store all info about each gpio
struct gpio_type {
  const char* type;
  const char* label;
  int pin;
  bool level = LOW;
};

// Define an array of struct GPIO and initialize with values
#define NUM_GPIOS 6
/* (ESP32-C3)
gpio_type gpios[NUM_GPIOS] = {
  {"input", "INPUT 2", 2}, 
  {"input", "INPUT 4", 4},
  {"input", "INPUT 5", 5},
  {"output", "OUTPUT 6", 6},
  {"output", "OUTPUT 7", 7},
  {"output", "LED BUILTIN", 3} // Led ON with signal HIGH (ESP32-C3)
};
*/

/* ESP8266 - Wemos D1-mini */
gpio_type gpios[NUM_GPIOS] = {
  {"input", "INPUT 5", D5},
  {"input", "INPUT 6", D6},
  {"input", "INPUT 7", D7},
  {"output", "OUTPUT 2", D2},
  {"output", "OUTPUT 3", D3},
  {"output", "LED BUILTIN", LED_BUILTIN} 
};

////////////////////////////////   WebSocket Handler  /////////////////////////////
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      Serial.printf("[%u] Client connected\n", num);
      break;
    case WStype_TEXT: {
        Serial.printf("[%u] get Text: %s\n", num, payload);
        // Copy content of payload to a temporary char array
        String json = (char*)payload;
        parseMessage(json);
        break;
      }
    default:
      break;
  }
}


void parseMessage(String& json) {
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, json);

  if (!error) {
    // If this is a "writeOut" command, set the pin level to value
    const char* cmd = doc["cmd"];
    if (strcmp(cmd, "writeOut") == 0) {
      int pin = doc["pin"];
      int level = doc["level"];
      for (int i = 0; i < NUM_GPIOS; i++) {
        if (gpios[i].pin == pin) {
          Serial.printf("Set pin %d to %d\n", pin, level);
          gpios[i].level = level ;
          digitalWrite(pin, level);
          updateGpioList();
          return;
        }
      }
    }
  }
  Serial.print(F("deserializeJson() failed: "));
  Serial.println(error.f_str());
}

void updateGpioList() {
  StaticJsonDocument<512> doc;
  JsonArray array = doc.to<JsonArray>();

  for (int i = 0; i < NUM_GPIOS; i++) {
    JsonObject obj = array.createNestedObject();
    obj["type"] = gpios[i].type;
    obj["pin"] = gpios[i].pin;
    obj["label"] = gpios[i].label;
    obj["level"] = gpios[i].level;
  }

  String json;
  serializeJson(doc, json);
  webSocket.broadcastTXT(json);

  // If this is a reply to HTPP request, get reference to client request.
  WebServerClass* webRequest = myWebServer.getRequest();
  if (webRequest->client())
    webRequest->send(200, "text/plain", json);
}

bool updateGpioState() {
  // Iterate the array of GPIO struct and check level of inputs
  for (int i = 0; i < NUM_GPIOS; i++) {
    if (strcmp(gpios[i].type, "input") == 0) {
      // Input value != from last read
      if (digitalRead(gpios[i].pin) != gpios[i].level) {
        gpios[i].level = digitalRead(gpios[i].pin);
        return true;
      }
    }
  }
  return false;
}


void setup() {
  Serial.begin(115200);

  // FILESYSTEM initialization
  if ( !FILESYSTEM.begin() ) {
    Serial.println("ERROR on mounting filesystem.");
    //FILESYSTEM.format();
    ESP.restart();
  }
  // Set your Static IP address
  IPAddress local_IP(192, 168, 2, 64);
  // Set your Gateway IP address
  IPAddress gateway(192, 168, 2, 1);
  IPAddress subnet(255, 255, 255, 0);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }

  IPAddress myIP = myWebServer.startWiFi(15000, "ESP32-AP", "123456789");  //timeout, AP SSID, AP password
  if (WiFi.status() == WL_CONNECTED) {
    myIP = WiFi.localIP();
    Serial.print(F("\nConnected! IP address: "));
    Serial.println(myIP);
  }

  // Add custom page handlers
  myWebServer.webserver->on("/getGpioList", HTTP_GET, updateGpioList);

  // Start webserver
  if (myWebServer.begin()) {
    Serial.println(F("ESP Web Server started"));
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
  }

  // Start WebSocket server on port 81
  Serial.println("Start WebSocket server");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // GPIO configuration
  for (int i=0; i<NUM_GPIOS; i++) {
    if (strcmp(gpios[i].type, "input") == 0)
      pinMode(gpios[i].pin, INPUT_PULLUP);
    else
      pinMode(gpios[i].pin, OUTPUT);
  }
}

void loop() {
  myWebServer.run();
  webSocket.loop();

  // True on pin state change
  if (updateGpioState()) {
    updateGpioList();   // Push new state to web clients via websocket
  }
}
