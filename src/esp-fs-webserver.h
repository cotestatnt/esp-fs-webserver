#ifndef esp_fs_webserver_H
#define esp_fs_webserver_H

#include <Arduino.h>
#include <FS.h>

#define DBG_OUTPUT_PORT     Serial
#define LOG_LEVEL           1         // (0 disable, 1 error, 2 info, 3 debug)
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
    #include <mdns.h>
    #include <Update.h>
    #include <WebServer.h>
    using WebServerClass = WebServer;
#endif

#include <DNSServer.h>
#include "esp-websocket.h"

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
    using CallbackF         = std::function<void(void)>;
    using FsInfoCallbackF   = std::function<void(fsInfo_t*)>;

public:

    FSWebServer(fs::FS& fs, uint16_t port = 80, const char* host = "esphost"):
        WebServerClass(port),
        m_filesystem(&fs),
        m_host(host)
    {
		setup = new SetupConfigurator(m_filesystem);
        m_port = port;
    }

    /*
    *   Set web server hostname
    */
    void setHostname(const char* host);

    /*
    * setup web page "configurator" class reference
    */
    SetupConfigurator* setup;

    /*
    * Override default begin() method to set library built-in handlers
    */
    virtual void begin(uint16_t port = 80);

    /*
    * Call this method in your loop
    */
    void run();

    /*
    * Set Access Point SSID and password
    */
    void setAP(const char *ssid, const char *psk);      // set AP SSID and password

    /*
    * Start ESP in Access Point mode
    */
    IPAddress startAP();

    /*
    * Start WiFi connection
    */
    IPAddress startWiFi(
        uint32_t timeout            // if 0 - do not wait for wifi connections
        , bool apFlag = false       // if true, start AP only if no credentials was found
        , CallbackF fn = nullptr    // execute callback function during wifi connection
    );

    /*
    * Clear store wifi credentials
    */
    void clearWifiCredentials();

    /*
    * Set current firmware version (shown in /setup webpage)
    */
    void setFirmwareVersion(const char* version) {
      strncpy(m_version, version, sizeof(m_version));
    }

    /*
    * Return true if in AP mode
    */
    inline bool getAPMode() const { return m_apmode; }

    /*
    * List all files in folder
    */
	void printFileList(fs::FS& fs, Print& p, const char* dirName, uint8_t level = 1);

    /*
    * Get library version
    */
    const char* getVersion();
    
    /**
     *  authenticate user
     *  use internal function to check if user is authenticated
     * ( intended to be used outside the library's scope )
     *  @return true if authenticated, false otherwise
     */
    bool authenticate_internal();

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

    /*
      Enable authentication for other pages, too.
      Call setAuthentication first.
    */
    void requireAuthentication(bool require);

    ///////////////////////////   WEBSOCKET  ///////////////////////////////////
    /*
      Enable built-in websocket server. Events like connect/disconnect or
      messages can be handled using callback function
    */
    void enableWebsocket(uint16_t port, ServerWebSocket::WsReceive_cb fn_receive,
            ServerWebSocket::WsConnect_cb fn_connect = nullptr, ServerWebSocket::WsConnect_cb fn_disconnect = nullptr);
    void onWebsocketConnect(ServerWebSocket::WsConnect_cb fn_connect);
    void onWebsocketDisconnect(ServerWebSocket::WsConnect_cb fn_disconnect);
    bool broadcastWebSocket(const char* payload);
    bool sendWebSocket(uint8_t num, const char* payload);

    inline bool broadcastWebSocket(const String& payload) { return broadcastWebSocket(payload.c_str()); }
    inline bool sendWebSocket(uint8_t num, const String& payload) { return sendWebSocket(num, payload.c_str()); }
    inline ServerWebSocket * getWebSocketServer() { return m_websocket; }
    //////////////////////////////////////////////////////////////////////////////

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

    /*
    * Set IP address in AP mode
    */
    void setIPaddressAP(IPAddress ip) {
        m_captiveIp = ip;
    }

    /*
    * Get current web server port
    */
    uint16_t getPort() { return m_port;}

    /////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////   SETUP PAGE CONFIGURATION ////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////
    bool closeConfiguration( bool write = true) {return setup->closeConfiguration(write);}
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
    template <typename T>
    bool saveOptionValue(const char *lbl, T &var) { return setup->saveOptionValue(lbl, var);}
    /////////////////////////////////////////////////////////////////////////////////////////////////
#endif

private:
    FsInfoCallbackF getFsInfo;
    char*           m_pageUser = nullptr;
    char*           m_pagePswd = nullptr;
    bool            m_authAll = false;
    DNSServer*      m_dnsServer  = nullptr;
    fs::FS*         m_filesystem = nullptr;
    File            m_uploadFile;
    String          m_apWebpage = "/setup";
    String          m_apSsid = "";
    String          m_apPsk = "";
    String          m_host;
    uint32_t        m_timeout = 10000;
    bool            m_fsOK = false;
    bool            m_apmode = false;
    uint16_t        m_port = 80;
    char            m_version[16] = {__TIME__};
    IPAddress       m_captiveIp = IPAddress(192, 168, 4, 1);
    ServerWebSocket*  m_websocket;
    uint8_t otaDone = 0;
    // Default handler for all URIs not defined above, use it to read files from filesystem
    bool captivePortal();
    void doWifiConnection();
    void doRestart();
    void replyOK();
    void handleRequest();
    void handleIndex();
    bool handleFileRead(const char* path);
    void handleFileUpload();
    void replyToCLient(int msg_type, const char *msg);
    void checkForUnsupportedPath(String &filename, String &error);
    void setCrossOrigin();
    void handleScanNetworks();
    const char *getContentType(const char *filename);

    // if you don't need /setup page, disable
#if ESP_FS_WS_SETUP
    void getStatus() ;
    void getConfigFile();
    void handleSetup();
    void update_second();
    void update_first() ;
    bool createDirFromPath(const String& path);
#endif

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
