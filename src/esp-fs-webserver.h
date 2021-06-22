#ifndef esp_fs_webserver_H
#define esp_fs_webserver_H

#include <Arduino.h>
#include <DNSServer.h>
#include <memory>
#include <typeinfo>
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


#ifdef ESP8266
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266mDNS.h>
    using WebServerClass = ESP8266WebServer;
#elif defined(ESP32)
    #include <WebServer.h>
    #include <WiFi.h>
    #include <ESPmDNS.h>
    using WebServerClass = WebServer;
#endif

#define DBG_OUTPUT_PORT Serial

enum { MSG_OK, CUSTOM, NOT_FOUND, BAD_REQUEST, ERROR };
#define TEXT_PLAIN "text/plain"
#define FS_INIT_ERROR "FS INIT ERROR"
#define FILE_NOT_FOUND "FileNotFound"

class FSWebServer{

using CallbackF = std::function<void(void)>;

public:
    FSWebServer(fs::FS& fs, WebServerClass& server);
    bool begin();

    inline void run(){
        webserver->handleClient();
    }

    inline void addHandler(const Uri &uri, HTTPMethod method, WebServerClass::THandlerFunction fn) {
        webserver->on(uri, method, fn);
    }

    inline void addHandler(const Uri &uri, WebServerClass::THandlerFunction handler) {
        webserver->on(uri, HTTP_ANY, handler);
    }


#ifdef INCLUDE_SETUP_HTM
    // Add custom option to config webpage
    template <typename T>
    inline void addOption(fs::FS& fs, const char* label, T value) {
        StaticJsonDocument<2048> doc;
        File file = fs.open("/config.json", "r");
        if (file) {
            // If file is present, load actual configuration
            DeserializationError error = deserializeJson(doc, file);
            if (error) {
                Serial.println(F("Failed to deserialize file, may be corrupted"));
                Serial.println(error.c_str());
                file.close();
                return;
            }
            file.close();
        }
        else {
            Serial.println(F("File not found, will be created new configuration file"));
        }
        doc[label] = static_cast<T>(value);
        file = fs.open("/config.json", "w");
        if (serializeJsonPretty(doc, file) == 0) {
            Serial.println(F("Failed to write to file"));
        }
        file.close();
    }
#endif
    WebServerClass* webserver;

private:
    fs::FS*     m_filesystem;
    File        m_uploadFile;
    bool        m_fsOK = false;

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

    // edit page, in usefull in some situation, but if you need to provide only a web interface, you can disable
#ifdef INCLUDE_EDIT_HTM
    void handleGetEdit();
    void handleFileCreate();
    void handleFileDelete();
    void handleStatus();
    void handleFileList();
#endif // INCLUDE_EDIT_HTM

};

#endif