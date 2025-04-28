#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver
#include <ArduinoJson.h>

#include <FS.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

FSWebServer myWebServer(FILESYSTEM, 80);
WebSocketsServer webSocket = WebSocketsServer(81);


// Define a struct for store all info about each gpio
struct gpio_type {
  const char* type;
  const char* label;
  int pin;
  bool level;
};

// Define an array of struct GPIO and initialize with values

/* (ESP32-C3) */
/*
gpio_type gpios[NUM_GPIOS] = {
  {"input", "INPUT 2", 2}, 
  {"input", "INPUT 4", 4},
  {"input", "INPUT 5", 5},
  {"output", "OUTPUT 6", 6},
  {"output", "OUTPUT 7", 7},
  {"output", "LED BUILTIN", 3} // Led ON with signal HIGH
};
*/

/* ESP8266 - Wemos D1-mini */
/*
  gpio_type gpios[] = {
  {"input", "INPUT 5", D5},
  {"input", "INPUT 6", D6},
  {"input", "INPUT 7", D7},
  {"output", "OUTPUT 2", D2},
  {"output", "OUTPUT 3", D3},
  {"output", "LED BUILTIN", LED_BUILTIN} // Led ON with signal LOW usually
  };
*/

/* ESP32 - NodeMCU-32S */
gpio_type gpios[] = {
  {"input", "INPUT 18", 18},  
  {"input", "INPUT 19", 19},
  {"input", "INPUT 21", 21},  
  {"output", "OUTPUT 4", 4},
  {"output", "OUTPUT 5", 5},
  {"output", "LED BUILTIN", 2} // Led ON with signal HIGH
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
#if ARDUINOJSON_VERSION_MAJOR > 6
  JsonDocument doc;
#else
 DynamicJsonDocument doc(512);
#endif
  DeserializationError error = deserializeJson(doc, json);

  if (!error) {
    // If this is a "writeOut" command, set the pin level to value
    const char* cmd = doc["cmd"];
    if (strcmp(cmd, "writeOut") == 0) {
      int pin = doc["pin"];
      int level = doc["level"];
      for (gpio_type &gpio : gpios) {
        if (gpio.pin == pin) {
          Serial.printf("Set pin %d to %d\n", pin, level);
          gpio.level = level ;
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
#if ARDUINOJSON_VERSION_MAJOR > 6
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();
#else
  DynamicJsonDocument doc(512);
  JsonArray array = doc.to<JsonArray>();
#endif

  for (gpio_type &gpio : gpios) {
#if ARDUINOJSON_VERSION_MAJOR > 6
    JsonObject obj = array.add<JsonObject>();
#else
    JsonObject obj = array.createNestedObject();
#endif
    obj["type"] = gpio.type;
    obj["pin"] = gpio.pin;
    obj["label"] = gpio.label;
    obj["level"] = gpio.level;
  }

  String json;
  serializeJson(doc, json);
  webSocket.broadcastTXT(json);
  
  if (myWebServer.client())
    myWebServer.send(200, "text/plain", json);
}

bool updateGpioState() {
  // Iterate the array of GPIO struct and check level of inputs
  for (gpio_type &gpio : gpios) {
    if (strcmp(gpio.type, "input") == 0) {
      // Input value != from last read
      if (digitalRead(gpio.pin) != gpio.level) {
        gpio.level = digitalRead(gpio.pin);
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
  
  // Try to connect to stored SSID, start AP if fails after timeout
  myWebServer.setAP("ESP_AP", "123456789");
  IPAddress myIP = myWebServer.startWiFi(15000);
  
  if (WiFi.status() == WL_CONNECTED) {
    myIP = WiFi.localIP();
    Serial.print(F("\nConnected! IP address: "));
    Serial.println(myIP);
  }

  // Add custom page handlers
  myWebServer.on("/getGpioList", HTTP_GET, updateGpioList);

  // set /setup and /edit page authentication
  // myWebServer.setAuthentication("admin", "admin");

  // Start webserver
  myWebServer.begin();
  Serial.println(F("ESP Web Server started.\n IP address: "));
  Serial.println(myIP);
  Serial.println(F("Open /setup page to configure optional parameters"));
  Serial.println(F("Open /edit page to view and edit files"));
  Serial.println(F("Open /update page to upload firmware and filesystem updates"));


  // Start WebSocket server on port 81
  Serial.println("Start WebSocket server");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // GPIOs configuration
  for (gpio_type &gpio : gpios) {
    if (strcmp(gpio.type, "input") == 0)     
        pinMode(gpio.pin, INPUT_PULLUP);
    else 
      pinMode(gpio.pin, OUTPUT); 
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