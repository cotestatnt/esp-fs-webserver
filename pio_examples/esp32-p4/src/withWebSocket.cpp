#include "ESP_HostedOTA.h"
#include <FS.h>
#include <LittleFS.h>
#include "FSWebServer.h"

#include "index_htm.h"

#define FILESYSTEM LittleFS
const char* hostname = "fsbrowser";

// If you edit server port, remember to change also websocket port in index_htm.h
// By default websocket port with F,WebServer library is server port + 1
FSWebServer server(FILESYSTEM, 80, hostname);

// Log messages both on Serial and WebSocket clients
void wsLogPrintf(bool toSerial, const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 128, format, args);
    va_end(args);
    server.broadcastWebSocket(buffer);
    if (toSerial)
        Serial.println(buffer);
}

/////////////////////////   WebSocket event callback////////////////////////////////   WebSocket Handler  /////////////////////////////
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
                IPAddress ip = server.getWebSocketServer()->remoteIP(num);
                server.getWebSocketServer()->sendTXT(num, "{\"Connected\": true}");

                // Print welcome message to all clients and to Serial
                wsLogPrintf(true, "Hello to client #%d [%s]\n", (int)num, ip.toString().c_str());
            }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] got Text: %s\n", num, payload);   // Got text message from a client
            break;
        case WStype_BIN:
            Serial.printf("[%u] got binary length: %u\n", num, length); // Got binary message from a client
            break;
        default:
            break;
    }
}

// Test "config" values
String optionString = "Test option String";
uint32_t optionULong = 1234567890;

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
struct tm Time;


////////////////////////////////  NTP Time  /////////////////////////////////////
void getUpdatedtime(const uint32_t timeout) {
    uint32_t start = millis();
    Serial.print("Sync time...");
    while (millis() - start < timeout && Time.tm_year <= (1970 - 1900)) {
        time_t now = time(nullptr);
        Time = *localtime(&now);
        delay(5);
    }
    Serial.println(" done.");
}


////////////////////////////////  Filesystem  /////////////////////////////////////////
bool startFilesystem() {
    if (FILESYSTEM.begin()) {
        server.printFileList(FILESYSTEM, "/", 1, Serial);
        return true;
    } else {
        Serial.println("ERROR on mounting filesystem. It will be reformatted!");
        FILESYSTEM.format();
        ESP.restart();
    }
    return false;
}


////////////////////  Load and save application configuration from filesystem  ////////////////////
bool loadApplicationConfig() {
    if (FILESYSTEM.exists(server.getConfiFileName())) {
        server.getOptionValue("Option 1", optionString);
        server.getOptionValue("Option 2", optionULong);
        server.closeSetupConfiguration();  // Close configuration to free resources
        return true;
    }
    return false;
}


void setup() {

    Serial.begin(115200);
    delay(3000);

    // FILESYSTEM INIT
    if (startFilesystem()) {
        // Load configuration (if not present, default will be created when webserver will start)
        if (loadApplicationConfig()) {
            Serial.println(F("\nApplication option loaded"));
            Serial.printf("  Option 1: %s\n", optionString.c_str());
            Serial.printf("  Option 2: %u\n", optionULong);
        } else
            Serial.println(F("Application options NOT loaded!"));
    }

    // Try to connect to WiFi (will start AP if not connected after timeout)
    if (!server.startWiFi(30000)) {
        Serial.println("\nWiFi not connected! Starting AP mode...");
        server.startCaptivePortal("ESP_AP", "123456789", "/setup");
    }

    // Update C6 firmware if required (must run before WiFi connect attempt)
    if (updateEspHostedSlave()) {
        Serial.println("ESP-Hosted slave updated, restarting...");
        ESP.restart();
    }

    // Configure /setup page
    server.addOptionBox("My Options");
    server.addOption("Option 1", optionString.c_str());
    server.addOption("Option 2", optionULong);

    // Add custom page handlers
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", homepage);
    });

    // Enable ACE FS file web editor
    server.enableFsCodeEditor();

    // Init with WebSocket event handler and start server
    server.begin(webSocketEvent);

    Serial.print(F("ESP Web Server started on IP Address: "));
    Serial.println(server.getServerIP());
    Serial.println(F(
                       "This is \"withWebSocket.ino\" example.\n"
                       "Open /setup page to configure optional parameters.\n"
                       "Open /edit page to view, edit or upload example or your custom webserver source files."
                   ));

    // Set hostname
    WiFi.setHostname(hostname);
    configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
}


void loop() {
    server.run();  // Handle client requests

    // Send ESP system time (epoch) to WS client
    static uint32_t sendToClientTime;
    if (millis() - sendToClientTime > 1000) {
        sendToClientTime = millis();
        time_t now = time(nullptr);
        wsLogPrintf(false, "{\"esptime\": %d}", (int)now);
    }

    delay(1);
}