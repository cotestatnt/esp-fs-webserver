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
    WiFiCredential config;                       // WiFi credentials (ssid, encrypted password, IP config, DNS)
    bool fromApClient = false;                  // True if the /connect request was initiated by a client
    bool dhcp = true;                           // True to use DHCP, false for static IP
    String password;                            // Plaintext password (temporary, not stored in credential)
    String host;                                // Hostname for mDNS
    uint32_t timeout = 0;                       // Connection timeout in milliseconds
    uint32_t wdtLongTimeout = 0;                // Long WDT timeout in milliseconds 
    uint32_t wdtTimeout = 0;                    // Regular WDT timeout in milliseconds
};

struct WiFiConnectResult {
    bool connected = false;
    int status = 500;    
    IPAddress ip = IPAddress(0, 0, 0, 0);
    String body;
};

class WiFiService {
public:
    static void setTaskWdt(uint32_t timeout);
    static WiFiScanResult scanNetworks();
    static WiFiConnectResult connectWithParams(const WiFiConnectParams& params);
    static WiFiStartResult startWiFi(CredentialManager* credentialManager, fs::FS* filesystem, const char* configFile, uint32_t timeout);    
    static bool startAccessPoint(WiFiConnectParams& params, IPAddress& outIp);
    static bool startMDNSResponder(DNSServer*& dnsServer, const String& host, uint16_t port, const IPAddress& serverIp);
    static bool startMDNSOnly(const String& host, uint16_t port);
};
