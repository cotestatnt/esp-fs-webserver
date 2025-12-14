#ifndef BUILTIN_LED
#define BUILTIN_LED 2
#endif

#ifndef BOOT_BUTTON
#define BOOT_BUTTON 0
#endif

#include <FS.h>
#include <LittleFS.h>
#include <FSWebServer.h>  // https://github.com/cotestatnt/esp-fs-webserver

#define FILESYSTEM LittleFS

char const* hostname = "fsbrowser";
FSWebServer server(80, FILESYSTEM, hostname);
long unsigned int timer = 0; // used to determine when to send websocket packets

// type declarations for websocket packet

// data type
typedef struct trig_t {
  float sin;
  float cos;
  float tan;
  uint16_t angle; // storing angle (x1000) as an integer, just to be different!
  uint16_t X; // column on chart
};

// union of data type and a byte array for websockets
typedef union trig_u {
  trig_t data;
  uint8_t byteArray[sizeof(trig_t)];
};

trig_u trig; // union to store trig data to be transmitted
uint16_t X=0; // chart column

// HTML of homepage:
static const char homepage[] PROGMEM = R"EOF(
<!DOCTYPE html>
<html>

<body onload="onBodyLoad()">
  <canvas id="chart" width="500" height="202"></canvas>
  <span>Angle (radians): </span><span id="angle"></span>
  <script type="text/javascript">

    var ws = null; // reference to websocket
    var v16; // 16-bit view of incoming data
    var v32; // 32-bit view of incoming data

    var ctxChart = null; // reference to chart canvas 2d context
    var angle; // angle (radians)
    var sin = 0; // sin of angle
    var cos = 1; // cos of angle
    var tan = 0; // tan of angle

    // Handles incoming messages
    function rxMessage(msg) {
      // console.log(msg);
      v16 = new Uint16Array(msg);
      v32 = new Float32Array(msg);
      angle = v16[6] / 1000; // (1000x) angle is stored as an int in the 7th pair of bytes in the websocket packet
      var X = v16[7]; // current column is stored as an int in the 8th pair of bytes in the websocket packet
      document.getElementById("angle").innerHTML = angle;
      // Draw a line from previous sin point to current sin point
      ctxChart.strokeStyle = "red";
      ctxChart.beginPath();
      ctxChart.moveTo(X-1, 100 - 100*sin);
      sin = v32[0]; // sin is stored as a float in the first four bytes of the websocket packet
      ctxChart.lineTo(X, 100 - 100*sin);
      ctxChart.stroke();
      // Draw a line from previous cos point to current sin point
      ctxChart.strokeStyle = "green";
      ctxChart.beginPath();
      ctxChart.moveTo(X-1, 100 - 100*cos);
      cos = v32[1];
      ctxChart.lineTo(X, 100 - 100*cos);
      ctxChart.stroke();
      // Draw a line from previous tan point to current tan point
      ctxChart.strokeStyle = "blue";
      ctxChart.beginPath();
      ctxChart.moveTo(X-1, 100 - 100*tan);
      tan = v32[2];
      ctxChart.lineTo(X, 100 - 100*tan);
      ctxChart.stroke();
      // increment X and wrap if necessary
      if (++X >= 500) {
        X = 0;
        ctxChart.clearRect(0,0,500,202);
      }
    }

    // Configure and start WebSocket client
    function startSocket() {
      ws = new WebSocket('ws://' + document.location.host + '/ws', ['arduino']);
      ws.binaryType = "arraybuffer";
      ws.onopen = function (e) {
        console.log("WebSocket client connected to " + 'ws://' + document.location.host + '/ws');
      };
      ws.onclose = function (e) {
        addMessage("WebSocket client disconnected");
      };
      ws.onerror = function (e) {
        console.log("ws error", e);
        addMessage("Error");
      };
      ws.onmessage = function (e) {
        rxMessage(e.data)
      };
    }

    // When page is fully loaded start connection
    function onBodyLoad() {
      startSocket();
      ctxChart = document.getElementById("chart").getContext("2d");
    }
  </script>
</body>

</html>
)EOF";

// In this example a custom websocket event handler is used instead default
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      client->printf("{\"Websocket connected\": true, \"clients\": %u}", client->id());
      Serial.printf("Websocket client %u connected\n", client->id());
      break;

    case WS_EVT_DISCONNECT:
      Serial.printf("Websocket client %u connected\n", client->id());
      break;

    case WS_EVT_DATA:
      {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->opcode == WS_TEXT) {
          char msg[len+1];
          msg[len] = '\0';
          memcpy(msg, data, len);
          Serial.printf("Received message \"%s\"\n", msg);
        }
      }
      break;

    default:
      break;
  }
}

////////////////////////////////  Filesystem  /////////////////////////////////////////
bool startFilesystem() {
  if (LittleFS.begin()){
    server.printFileList(LittleFS, "/", 1);
    Serial.println();
    return true;
  }
  else {
    Serial.println("ERROR on mounting filesystem. It will be reformatted!");
    LittleFS.format();
    ESP.restart();
  }
  return false;
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);

  Serial.begin(115200);
  while (!Serial) {
    if (millis() > 3000) break; // 3 second timeout to start Serial
  }

  digitalWrite(BUILTIN_LED, LOW);
  if (Serial) { // flash LED if Serial is available
    delay(200);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(200);
    digitalWrite(BUILTIN_LED, LOW);
  }

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to WiFi (will start AP if not connected after timeout)
  if (!server.startWiFi(10000)) {
    Serial.println("\nWiFi not connected! Starting AP mode...");
    char ssid[21];
#ifdef defined(ESP8266)
    snprintf(ssid, sizeof(ssid), "ESP-%dX", ESP.getChipId());
#elif defined(ESP32)
    snprintf(ssid, sizeof(ssid), "ESP-%llX", ESP.getEfuseMac());
#endif
    server.startCaptivePortal(ssid, "123456789", "/setup");
  }
  

  // Add custom page handlers
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", homepage);
  });

  // Enable ACE FS file web editor and add FS info callback function
  server.enableFsCodeEditor();

#ifdef ESP32
  server.setFsInfoCallback([](fsInfo_t* fsInfo) {
    fsInfo->fsName = "LittleFS";
    fsInfo->totalBytes = LittleFS.totalBytes();
    fsInfo->usedBytes = LittleFS.usedBytes();  
  });
#endif

  // Init with custom WebSocket event handler and start server
  server.begin(onWsEvent);

  Serial.print(F("ESP Web Server started on IP Address: "));
  Serial.println(server.getServerIP());
  Serial.println(F(
    "This is \"BinaryWebSocket.ino\" example.\n"
    "Open /setup page to configure optional parameters.\n"
    "Open /edit page to view, edit or upload example or your custom webserver source files."
  ));
}

void loop() {
  server.handleClient();

  // send websocket every 50ms
  if (millis() > 50 * timer) {
    timer++;
    trig.data.angle = 1000.0 * 2 * PI * X/500; // angle (x1000) is stored as a 16-bit integer, just to be different!
    trig.data.sin = sin(trig.data.angle/1000.0);
    trig.data.cos = cos(trig.data.angle/1000.0);
    trig.data.tan = tan(trig.data.angle/1000.0);
    trig.data.X = X;
    if (++X >= 500) X = 0;
    server.broadcastWebSocket((uint8_t*)trig.byteArray, sizeof(trig.byteArray));
  }
}