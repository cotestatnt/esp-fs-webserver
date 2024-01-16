#ifndef esp_fs_webserver_H
#define esp_fs_webserver_H

#include <Arduino.h>
#include <memory>
#include <typeinfo>
#include <base64.h>
#include <FS.h>

#define DBG_OUTPUT_PORT     Serial
#define LOG_LEVEL           2         // (0 disable, 1 error, 2 info, 3 debug)
#include "SerialLog.h"


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

#if ESP_FS_WS_SETUP_HTM
    #define ARDUINOJSON_USE_LONG_LONG 1
    #include <ArduinoJson.h>
#if ARDUINOJSON_VERSION_MAJOR > 6
    #define JSON_DOC(x) JsonDocument doc
#else
    #define JSON_DOC(x) DynamicJsonDocument doc((size_t)x)
#endif
    #define ESP_FS_WS_CONFIG_FOLDER "/setup"
    #define ESP_FS_WS_CONFIG_FILE ESP_FS_WS_CONFIG_FOLDER "/config.json"
    #define LIB_URL "https://github.com/cotestatnt/esp-fs-webserver/"

    #include "setup_htm.h"
    #include "SetupConfig.hpp"
#endif


#if defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266mDNS.h>
    #include <ESP8266HTTPUpdateServer.h> // from Arduino core, OTA update via webbrowser
    using WebServerClass = ESP8266WebServer;
#elif defined(ESP32)
    #include <esp_wifi.h>
    #include "esp_task_wdt.h"
    #include "sys/stat.h"
    #include <WiFi.h>
    #include <ESPmDNS.h>
    #include <HTTPUpdateServer.h>

    #include <WebServer.h>
    #undef HTTP_MAX_DATA_WAIT
    #define HTTP_MAX_DATA_WAIT 100      // Fix first connection very slow
    using WebServerClass = WebServer;
#endif
#include <DNSServer.h>

typedef struct {
  size_t totalBytes;
  size_t usedBytes;
  String fsName;
} fsInfo_t;

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

class FSWebServer : public WebServerClass
{
    using CallbackF = std::function<void(void)>;
    using FsInfoCallbackF = std::function<void(fsInfo_t*)>;

public:
    SetupConfigurator* setup;

    FSWebServer(fs::FS& fs, uint16_t port, const char* host = "esphost"):
        WebServerClass(port),
        m_filesystem(&fs),
        m_host((char*)host)
    {
		setup = new SetupConfigurator(m_filesystem);
        m_port = port;
    }

    // Override default begin() method to set library built-in handlers
    virtual void begin();

    void run();
    void setAPWebPage(const char *url);                 // point a custom setup webpage
    void setAP(const char *ssid, const char *psk);      // set AP SSID and password

    IPAddress startAP();

    IPAddress startWiFi(
        uint32_t timeout            // if 0 - do not wait for wifi connections
        , bool apFlag = false       // if true, start AP only if no credentials was found
        , CallbackF fn = nullptr    // execute callback function during wifi connection
        );

    void clearWifiCredentials();

    /*
    * Set current firmware version (shown in /setup webpage)
    */
    void setFirmwareVersion(char* version) {
      strncpy(m_version, version, sizeof(m_version));
    }

    inline uint32_t getTimeout() const { return m_timeout; }
    inline bool getAPMode() const { return m_apmode; }

	// List all files
	void printFileList(fs::FS& fs, Print& p, const char* dirName, uint8_t level = 1);

    // Get library version
    const char* getVersion();

    /*
      Enable the built-in ACE web file editor
      @ param
        * Set callback function to provide updated FS info to library
        * This it is necessary due to the different implementation of
        * libraries for the filesystem (LittleFS, FFat, SPIFFS etc etc)
    */
    void enableFsCodeEditor(FsInfoCallbackF fsCallback = nullptr);

    /*
      Enable authenticate for /setup webpage
    */
    void setAuthentication(const char* user, const char* pswd);

#if ESP_FS_WS_SETUP
    /*
    * Get reference to current config.json file
    */
    File getConfigFile(const char* mode) {
      File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, mode);
      return file;
    }

    /*
    * Get complete path of config.json file
    */
    const char* getConfigFilepath();

    /*
    * Clear config file
    */
    bool clearOptions();

    /////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////   SETUP PAGE CONFIGURATION ////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////
    bool optionToFile(const char* f, const char* id, bool ow) {return setup->optionToFile(f, id, ow);}
    void addHTML(const char* h, const char* id, bool ow = false) {setup->addHTML(h, id, ow);}
    void addCSS(const char* c, const char* id, bool ow = false){setup->addCSS(c, id, ow);}
    void addJavascript(const char* s, const char* id, bool ow = false) {setup->addJavascript(s, id, ow);}
    void addDropdownList(const char *l, const char** a, size_t size){setup->addDropdownList(l, a, size);}
    void addOptionBox(const char* title) { setup->addOption("param-box", title); }
    void setLogoBase64(const char* logo, const char* w = "128", const char* h = "128", bool ow = false) {
      setup->setLogoBase64(logo, w, h, ow);
    }
    template <typename T>
    void addOption(const char *lbl, T val, double min, double max, double st){
      setup->addOption(lbl, val, false, min, max, st);
    }
    template <typename T>
    void addOption(const char *lbl, T val, bool hd = false,  double min = MIN_F,
      double max = MAX_F, double st = 1.0) {
      setup->addOption(lbl, val, hd, min, max, st);
    }
    template <typename T>
    bool getOptionValue(const char *lbl, T &var) { return setup->getOptionValue(lbl, var);}
    /////////////////////////////////////////////////////////////////////////////////////////////////
#endif

private:
    char* m_pageUser = nullptr;
    char* m_pagePswd = nullptr;

    FsInfoCallbackF getFsInfo;
    DNSServer* m_dnsServer;
    fs::FS *m_filesystem;
    File m_uploadFile;
    bool m_fsOK = false;
    bool m_apmode = false;
    String m_apWebpage = "/setup";
    String m_apSsid = "";
    String m_apPsk = "";
    uint32_t m_timeout = 10000;

    char m_version[16] = {__TIME__};
    uint16_t m_port = 80;
    char* m_host;

    #if defined(ESP32)
    // Override default handleClient() method to increase connection speed
    virtual void handleClient();
    #endif

    // Default handler for all URIs not defined above, use it to read files from filesystem
    void doWifiConnection();
    void doRestart();
    void replyOK();
    void handleRequest();
    void removeWhiteSpaces(String& str);

#if ESP_FS_WS_SETUP
    void getStatus() ;
    void getConfigFile();
    void handleSetup();
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


#endif
