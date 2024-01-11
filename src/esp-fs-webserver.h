#ifndef esp_fs_webserver_H
#define esp_fs_webserver_H

#include <Arduino.h>
#include <memory>
#include <typeinfo>
#include <base64.h>
#include <FS.h>

//default values
#ifndef ESP_FS_WS_EDIT
    #define ESP_FS_WS_EDIT              1   //has edit methods
    #ifndef ESP_FS_WS_EDIT_HTM
        #define ESP_FS_WS_EDIT_HTM      1   //included from progmem
    #endif
#endif

#ifndef ESP_FS_WS_SETUP
    #define ESP_FS_WS_SETUP             1   //has setup methods
    #ifndef ESP_FS_WS_SETUP_HTM
        #define ESP_FS_WS_SETUP_HTM     1   //included from progmem
    #endif
#endif

#if ESP_FS_WS_EDIT_HTM
    #include "edit_htm.h"
#endif

#if ESP_FS_WS_SETUP
    #define ARDUINOJSON_USE_LONG_LONG 1
    #include <ArduinoJson.h>
    #define ESP_FS_WS_CONFIG_FOLDER "/setup"
    #define ESP_FS_WS_CONFIG_FILE ESP_FS_WS_CONFIG_FOLDER "/config.json"
    #include "setup_htm.h"
#endif


#define DBG_OUTPUT_PORT     Serial
#define LOG_LEVEL           2         // (0 disable, 1 error, 2 info, 3 debug)
#include "SerialLog.h"
#include "SetupConfig.hpp"


#if defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266mDNS.h>
    #include <ESP8266HTTPUpdateServer.h> // from Arduino core, OTA update via webbrowser
    using WebServerClass = ESP8266WebServer;
    using UpdateServerClass = ESP8266HTTPUpdateServer;
#elif defined(ESP32)
    #include <esp_wifi.h>
    #include "esp_task_wdt.h"
    #include "sys/stat.h"

    #include <WebServer.h>
    #include <WiFi.h>
    #include <ESPmDNS.h>
    #include <HTTPUpdateServer.h> // from Arduino core, OTA update via webbrowser
    using WebServerClass = WebServer;
    using UpdateServerClass = HTTPUpdateServer;
#endif
#include <DNSServer.h>

// #define ESP_FS_WS_DEBUG_MODE 1
// #if ESP_FS_WS_DEBUG_MODE
//     #define DebugPrint(...) ESP_FS_WS_DEBUG_OUTPUT.print(__VA_ARGS__)
//     #define DebugPrintln(...) ESP_FS_WS_DEBUG_OUTPUT.println(__VA_ARGS__)
//     #define DebugPrintf(...) ESP_FS_WS_DEBUG_OUTPUT.printf(__VA_ARGS__)
//     #define DebugPrintf_P(...) ESP_FS_WS_DEBUG_OUTPUT.printf_P(__VA_ARGS__)
// #else
//     #define DebugPrint(...) ((void)0)
//     #define DebugPrintln(...) ((void)0)
//     #define DebugPrintf(...) ((void)0)
//     #define DebugPrintf_P(...) ((void)0)
// #endif

// #if ESP_FS_WS_INFO_MODE
//     #define InfoPrint(...) ESP_FS_WS_INFO_OUTPUT.print(__VA_ARGS__)
//     #define InfoPrintln(...) ESP_FS_WS_INFO_OUTPUT.println(__VA_ARGS__)
// #else
//     #define InfoPrint(...) ((void)0)
//     #define InfoPrintln(...) ((void)0)
// #endif

enum
{
    MSG_OK,
    CUSTOM,
    NOT_FOUND,
    BAD_REQUEST,
    ERROR
};
#define TEXT_PLAIN "text/plain"
#define FS_INIT_ERROR "FS INIT ERROR"
#define FILE_NOT_FOUND "FileNotFound"

class FSWebServer
{
    using CallbackF = std::function<void(void)>;

public:
    SetupConfigurator setup;

    FSWebServer(fs::FS &fs, WebServerClass &server): m_filesystem(&fs), webserver(&server), setup(&fs) {;}

    ~FSWebServer() { webserver->stop(); }

    bool begin();

    void run();

    void addHandler(const Uri &uri, HTTPMethod method, WebServerClass::THandlerFunction fn);

    void addHandler(const Uri &uri, WebServerClass::THandlerFunction handler);

    void setAPWebPage(const char *url);                 // point a custom setup webpage
    void setAP(const char *ssid, const char *psk);      // set AP SSID and password

    IPAddress startAP();

    IPAddress startWiFi(
        uint32_t timeout            // if 0 - do not wait for wifi connections
        , bool apFlag = false       // if true, start AP only if no credentials was found
        , CallbackF fn = nullptr    // execute callback function during wifi connection
        );

    void clearWifiCredentials();

    inline IPAddress startWiFi(uint32_t timeout, const char* ssid, const char* psk, CallbackF fn = nullptr) {
        setAP(ssid, psk);
        return startWiFi(timeout, true, fn);
    }

    /*
    * Set current firmware version (shown in /setup webpage)
    */
    inline void setFirmwareVersion(char* version) {
      strncpy(m_version, version, sizeof(m_version));
    }

    inline WebServerClass*  getRequest()            { return webserver; }
    inline uint32_t         getTimeout()    const   { return m_timeout; }
    inline bool             getAPMode()     const   { return m_apmode;  }

#if ESP_FS_WS_SETUP
    void addHTML(const char* h, const char* id, bool ow = false) {setup.addHTML(h, id, ow);}
    void addCSS(const char* c, const char* id, bool ow = false){setup.addCSS(c, id, ow);}
    void addJavascript(const char* s, const char* id, bool ow = false) {setup.addJavascript(s, id, ow);}
    void addDropdownList(const char *l, const char** a, size_t size){setup.addDropdownList(l, a, size);}
    void addOptionBox(const char* title) { setup.addOption("param-box", title); }
    void setLogoBase64(const char* logo, const char* w = "128", const char* h = "128", bool ow = false) {
      setup.setLogoBase64(logo, w, h, ow);
    }

    template <typename T>
    void addOption(const char *lbl, T val, double min, double max, double st){
      setup.addOption(lbl, val, false, min, max, st);
    }

    template <typename T>
    void addOption(const char *lbl, T val, bool hd = false,  double min = MIN_F,
      double max = MAX_F, double st = 1.0) {
      setup.addOption(lbl, val, hd, min, max, st);
    }

    template <typename T>
    bool getOptionValue(const char *lbl, T &var) { return setup.getOptionValue(lbl, var);}

    template <typename T>
    bool saveOptionValue(const char *lbl, T val) {return setup.saveOptionValue(lbl, val);}
#endif

private:
    WebServerClass *webserver = nullptr;
    fs::FS *m_filesystem = nullptr;

    DNSServer m_dnsServer;
    File m_uploadFile;
    bool m_fsOK = false;
    bool m_apmode = false;
    String m_apWebpage = "/setup";
    String m_apSsid = "ESP_AP";
    String m_apPsk = "123456789";
    uint32_t m_timeout = 10000;

    char m_version[16] = {__TIME__};

    // Default handler for all URIs not defined above, use it to read files from filesystem
    bool checkDir(const char *dirname);
    void doWifiConnection();
    void doRestart();
    void replyOK();
    void getIpAddress();
    void handleRequest();

#if ESP_FS_WS_SETUP
    void getStatus() ;
    void getConfigFile();
    bool optionToFile(const char* filename, const char* str, bool overWrite = false);
    void removeWhiteSpaces(String& str);
    void handleSetup();
    uint8_t numOptions = 0;
    void update_second();
    void update_first() ;
    bool createDirFromPath(const String& path);
#endif

    void handleIndex();
    bool handleFileRead(const char* path);
    void handleFileUpload();
    void replyToCLient(int msg_type, const char *msg);
    void checkForUnsupportedPath(String &filename, String &error);
    void setCrossOrigin();
    void handleScanNetworks();
    const char *getContentType(const char *filename);
    bool captivePortal();

    // edit page, in usefull in some situation, but if you need to provide only a web interface, you can disable
#if ESP_FS_WS_EDIT
    void deleteContent(String& path) ;
    void handleGetEdit();
    void handleFileCreate();
    void handleFileDelete();
    void handleStatus();
    void handleFileList();
#endif
};

// List all files
void PrintDir(fs::FS& fs, Print& p, const char* dirName, uint8_t level = 1);

#endif
