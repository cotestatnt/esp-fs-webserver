#pragma once

#include <Arduino.h>
#include <DNSServer.h>
#include <FS.h>
#include "SerialLog.h"
#include "CredentialManager.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#else
#error Platform not supported
#endif

struct WiFiScanResult {
    bool reload = false;
    String json;
};

enum class WiFiStartAction {
    Connected,
    StartAp,
    Failed
};

struct WiFiStartResult {
    WiFiStartAction action = WiFiStartAction::Failed;
    IPAddress ip = IPAddress(0, 0, 0, 0);
};

struct WiFiConnectParams {
    String ssid;
    String password;
    bool changeSSID = false;
    bool noDHCP = false;
    // True if the /connect request was initiated by a client
    // connected to the ESP's own AP (captive portal).
    bool fromApClient = false;
    IPAddress local_ip;
    IPAddress gateway;
    IPAddress subnet;
    String host;
    uint32_t timeout = 0;
    uint32_t wdtLongTimeout = 0;
    uint32_t wdtTimeout = 0;
};

struct WiFiConnectResult {
    int status = 500;
    String body;
    bool connected = false;
    IPAddress ip = IPAddress(0, 0, 0, 0);
};

class WiFiService {
public:
    static void setTaskWdt(uint32_t timeout);
    static void applyPersistentConfig(bool persistentEnabled, const String& ssid, const String& pass);
    static WiFiScanResult scanNetworks();
    static WiFiConnectResult connectWithParams(const WiFiConnectParams& params);
    static WiFiStartResult startWiFi(CredentialManager* credentialManager, fs::FS* filesystem, const char* configFile, uint32_t timeout);    
    static bool startAccessPoint(WiFiConnectParams& params, IPAddress& outIp);
    static bool startMDNSResponder(DNSServer*& dnsServer, const String& host, uint16_t port, const IPAddress& serverIp);
    static bool startMDNSOnly(const String& host, uint16_t port);
};
