#ifndef esp_fs_webserver_H
#define esp_fs_webserver_H

#include <Arduino.h>
#include <memory>
#include <typeinfo>
#include <base64.h>
#include <FS.h>

#define INCLUDE_EDIT_HTM
#ifdef INCLUDE_EDIT_HTM
#include "edit_htm.h"
#endif

#define INCLUDE_SETUP_HTM
#ifdef INCLUDE_SETUP_HTM
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include "setup_htm.h"
#endif

#if defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266mDNS.h>
	#include <ESP8266HTTPUpdateServer.h> // from Arduino core, OTA update via webbrowser
    using WebServerClass = ESP8266WebServer;
    using UpdateServerClass = ESP8266HTTPUpdateServer;
#elif defined(ESP32)
    #include <esp_wifi.h>
    #include <WebServer.h>
    #include <WiFi.h>
    #include <ESPmDNS.h>
	#include <HTTPUpdateServer.h> // from Arduino core, OTA update via webbrowser
    using WebServerClass = WebServer;
    using UpdateServerClass = HTTPUpdateServer;
#endif
#include <DNSServer.h>

#ifndef DEBUG_ESP_PORT
#define DBG_OUTPUT_PORT Serial
#define DEBUG_MODE_WS true
#else
#define DBG_OUTPUT_PORT DEBUG_ESP_PORT
#endif

#if DEBUG_MODE_WS
#define DebugPrint(x) DBG_OUTPUT_PORT.print(x)
#define DebugPrintln(x) DBG_OUTPUT_PORT.println(x)
#define DebugPrintf(fmt, ...) DBG_OUTPUT_PORT.printf(fmt, ##__VA_ARGS__)
#define DebugPrintf_P(fmt, ...) DBG_OUTPUT_PORT.printf_P(fmt, ##__VA_ARGS__)
#else
#define DebugPrint(x)
#define DebugPrintln(x)
#define DebugPrintf(x)
#define DebugPrintf_P(x)
#endif



enum { MSG_OK, CUSTOM, NOT_FOUND, BAD_REQUEST, ERROR };
#define TEXT_PLAIN "text/plain"
#define FS_INIT_ERROR "FS INIT ERROR"
#define FILE_NOT_FOUND "FileNotFound"

class FSWebServer{

// using CallbackF = std::function<void(void)>;

public:
    WebServerClass* webserver;

    FSWebServer(fs::FS& fs, WebServerClass& server);

    bool begin();

    void run();

    void addHandler(const Uri &uri, HTTPMethod method, WebServerClass::THandlerFunction fn) ;

    void addHandler(const Uri &uri, WebServerClass::THandlerFunction handler);

  	void setCaptiveWebage(const char* url);

    IPAddress setAPmode(const char* ssid, const char* psk);

    IPAddress startWiFi(uint32_t timeout, const char* apSSID, const char* apPsw);

    WebServerClass* getRequest();

#ifdef INCLUDE_SETUP_HTM


    // Only for backward-compatibility
    template <typename T>
    inline void addOption(fs::FS& fs, const char* label, T val, bool hidden = false) {
        addOption(label, val, hidden);
    }

    #define MIN_F -3.4028235E+38
    #define MAX_F 3.4028235E+38

    // Add custom option to config webpage (type of parameter will be deduced from variable itself)
    template <typename T>
    inline void addOption(const char* label, T val, bool hidden = false,  double min=MIN_F, double max=MAX_F, double step=1) {
        StaticJsonDocument<2048> doc;
        File file = m_filesystem->open("/config.json", "r");
        if (file) {
            // If file is present, load actual configuration
            DeserializationError error = deserializeJson(doc, file);
            if (error) {
                DebugPrintln(F("Failed to deserialize file, may be corrupted"));
                DebugPrintln(error.c_str());
                file.close();
                return;
            }
            file.close();
        }
        else {
            DebugPrintln(F("File not found, will be created new configuration file"));
        }

        String key = label;
        if (hidden)
            key += "-hidden";

        // if min, max, step != from default, treat this as object in order to set other properties
        if (min != MIN_F || max != MAX_F || step != 1.0) {
            JsonObject obj  = doc.createNestedObject(key);
            obj["value"] = static_cast<T>(val);
            obj["min"] = min;
            obj["max"] = max;
            obj["step"] = step;
        }
        else
            doc[key] = static_cast<T>(val);

        file =  m_filesystem->open("/config.json", "w");
        if (serializeJsonPretty(doc, file) == 0) {
            DebugPrintln(F("Failed to write to file"));
		}
        file.close();
    }

    // Add custom "numeric-only" option to config webpage
    template <typename T>
    inline void addOption(const char* label, T val, double min, double max, double step) {
        addOption( label, val, false,  min, max, step);
    }

    // Get current value for a specific custom option (true on success)
    template <typename T>
    bool getOptionValue(const char* label, T& variable) {
        StaticJsonDocument<2048> doc;
        File file = m_filesystem->open("/config.json", "r");
        if (file) {
            DeserializationError error = deserializeJson(doc, file);
            if (error) {
                DebugPrintf_P(F("Failed to deserialize file, may be corrupted %s\n"), error.c_str());
                file.close();
                return false;
            }
            file.close();
        }
        else
            return false;

        if (doc[label]["value"])
            variable = doc[label]["value"].as<T>();
        else
            variable = doc[label].as<T>();
        return true;
    }
#endif

private:
    UpdateServerClass m_httpUpdater;
    DNSServer   m_dnsServer;
    fs::FS*     m_filesystem;
    File        m_uploadFile;
    bool        m_fsOK = false;
    bool        m_apmode = false;
	char* 		m_apWebpage = (char*) "/setup";
	uint32_t	m_timeout = 10000;

    // Default handler for all URIs not defined above, use it to read files from filesystem
    void doWifiConnection();
    void doRestart();
    void replyOK();
    void getIpAddress();
    void handleRequest();
    void handleSetup();
    void handleIndex();
    bool handleFileRead(const String &path);
    void handleFileUpload();
    void replyToCLient(int msg_type, const char* msg);
    void checkForUnsupportedPath(String &filename, String &error);
    void setCrossOrigin();
    void handleScanNetworks();
    const char* getContentType(const char* filename);
    bool captivePortal();

    // edit page, in usefull in some situation, but if you need to provide only a web interface, you can disable
#ifdef INCLUDE_EDIT_HTM
    void handleGetEdit();
    void handleFileCreate();
    void handleFileDelete();
    void handleStatus();
    void handleFileList();
#endif // INCLUDE_EDIT_HTM

/*
#ifdef INCLUDE_SETUP_HTM
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
	const char* getTypes(String a) { return "string"; }
	const char* getTypes(const char* a) { return "string"; }
	const char* getTypes(char *a) {return "string";}
	const char* getTypes(int a) { return "number"; }
	const char* getTypes(unsigned int a) { return "number"; }
	const char* getTypes(long a) { return "number"; }
	const char* getTypes(unsigned long a) { return "number"; }
	const char* getTypes(float a) { return "float";}
	const char* getTypes(bool a) { return "boolean"; }
#pragma GCC diagnostic pop
#endif // INCLUDE_SETUP_HTM
*/


};

#endif