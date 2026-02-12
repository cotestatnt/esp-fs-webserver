#include "WiFiService.h"
#include "Json.h"

// Helper: log the actual station network configuration as seen by the core
// (independent from WiFiConnectParams / CredentialManager values).
static void logCurrentStaNetworkConfig() {
#if defined(ESP8266) || defined(ESP32)
    auto mode = WiFi.getMode();
    if (mode != WIFI_STA && mode != WIFI_AP_STA) {
        return;
    }
    log_info("[WiFi] Core STA config: IP=%s GW=%s SN=%s DNS1=%s DNS2=%s",
             WiFi.localIP().toString().c_str(),
             WiFi.gatewayIP().toString().c_str(),
             WiFi.subnetMask().toString().c_str(),
             WiFi.dnsIP(0).toString().c_str(),
             WiFi.dnsIP(1).toString().c_str());
#endif
}

#if defined(ESP32)
static inline void resetTaskWdtIfSubscribed() {
    if (esp_task_wdt_status(NULL) == ESP_OK) {
        esp_task_wdt_reset();
    }
}
#endif

void WiFiService::setTaskWdt(uint32_t timeout) {
#if defined(ESP32)
    #if ESP_ARDUINO_VERSION_MAJOR > 2
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = timeout,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
        .trigger_panic = false,
    };
    ESP_ERROR_CHECK(esp_task_wdt_reconfigure(&twdt_config));
    #else
    ESP_ERROR_CHECK(esp_task_wdt_init(timeout / 1000, 0));
    #endif
    // The Arduino core already subscribes loopTask to the WDT.
    // Adding it again logs "task is already subscribed". Avoid duplicate add.
#elif defined(ESP8266)
    ESP.wdtDisable();
    ESP.wdtEnable(timeout);
#endif
}

WiFiScanResult WiFiService::scanNetworks() {
    WiFiScanResult result;
    int res = WiFi.scanComplete();

#ifdef WIFI_SCAN_RUNNING
    if (res == WIFI_SCAN_RUNNING) {
        result.reload = true;
        result.json = "{\"reload\":1}";
        return result;
    }
    if (res == WIFI_SCAN_FAILED) {
        WiFi.scanNetworks(true);
        result.reload = true;
        result.json = "{\"reload\":1}";
        return result;
    }
#else
    if (res == -2) {
        WiFi.scanNetworks(true);
        result.reload = true;
        result.json = "{\"reload\":1}";
        return result;
    }
    if (res == -1) {
        result.reload = true;
        result.json = "{\"reload\":1}";
        return result;
    }
#endif

    if (res >= 0) {
        CJSON::Json json_array;
        json_array.createArray();
        for (int i = 0; i < res; ++i) {
            CJSON::Json item;
            item.setNumber("strength", WiFi.RSSI(i));
            item.setString("ssid", WiFi.SSID(i));
#if defined(ESP8266)
            item.setString("security", AUTH_OPEN ? "none" : "enabled");
#elif defined(ESP32)
            item.setString("security", WIFI_AUTH_OPEN ? "none" : "enabled");
#endif
            json_array.add(item);
        }
        result.json = json_array.serialize();
        WiFi.scanDelete();
        WiFi.scanNetworks(true);
        return result;
    }

    result.reload = true;
    result.json = "{\"reload\":1}";
    return result;
}

WiFiConnectResult WiFiService::connectWithParams(const WiFiConnectParams& params) {
    WiFiConnectResult result;

    if (strlen(params.config.ssid)) {
        setTaskWdt(params.wdtLongTimeout);
        WiFi.mode(WIFI_AP_STA);

        if (!params.dhcp) {
            log_info("Manual config WiFi connection with IP: %s", params.config.local_ip.toString().c_str());
            bool hasDns1 = params.config.dns1 != IPAddress(0, 0, 0, 0);
            bool hasDns2 = params.config.dns2 != IPAddress(0, 0, 0, 0);
            bool ok = false;
            if (hasDns1 && hasDns2) {
                ok = WiFi.config(params.config.local_ip, params.config.gateway, params.config.subnet, params.config.dns1, params.config.dns2);
            } else if (hasDns1) {
                ok = WiFi.config(params.config.local_ip, params.config.gateway, params.config.subnet, params.config.dns1);
            } else {
                ok = WiFi.config(params.config.local_ip, params.config.gateway, params.config.subnet);
            }

            if (!ok) {
                log_error("STA Failed to configure");
            }
        }

        DBG_OUTPUT_PORT.print("\n\n\nConnecting to ");
        DBG_OUTPUT_PORT.println(params.config.ssid);
        WiFi.begin(params.config.ssid, params.password.c_str());

        uint32_t beginTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(250);
            DBG_OUTPUT_PORT.print("*");
#if defined(ESP8266)
            ESP.wdtFeed();
#else
            resetTaskWdtIfSubscribed();
#endif
            if (millis() - beginTime > params.timeout) {
                result.status = 408;
                result.body = "<br><br>Connection timeout!<br>Check password or try to restart ESP.";
                setTaskWdt(params.wdtTimeout);
                return result;
            }
        }

        if (WiFi.status() == WL_CONNECTED) {
            result.ip = WiFi.localIP();
            result.connected = true;

            // Debug: print the actual STA configuration as seen by the core
            logCurrentStaNetworkConfig();

            DBG_OUTPUT_PORT.print("\nConnected to ");
            DBG_OUTPUT_PORT.print(params.config.ssid);
            DBG_OUTPUT_PORT.print(". IP address: ");
            DBG_OUTPUT_PORT.println(result.ip);
            String serverLoc = F("http://");
            for (int i = 0; i < 4; i++) {
                if (i) serverLoc += ".";
                serverLoc += result.ip[i];
            }
            serverLoc += "/setup";
            String resp;
            resp  = "ESP successfully connected to ";
            resp += params.config.ssid;
            resp += " WiFi network.";

            if (params.fromApClient) {
                // Case 1: request came from a client connected to the ESP AP.
                // We stay in WIFI_AP_STA, so we can still deliver this page over the AP and then let the user switch WiFi.
                resp += " <br><br><i>Note:<br>Disconnect your browser from ESP's access point and connect it to the new WiFi network.<br><br>IP address <a href='";
                resp += serverLoc;
                resp += "'>";
                resp += serverLoc;
                resp += "</a><br> Hostname <a href='http://";
                resp += params.host;
                resp += ".local/setup'>http://";
                resp += params.host;
                resp += ".local/setup</a></i><br><p style='text-align: center'>Do you want to proceed with a ESP restart right now?</p><div id='action-restart-required'></div>";
            } else {
                // Case 2: request came from a client already on the same WiFi as the ESP (pure SSID switch).
                // After the switch this page will no longer reach the device until the client changes WiFi as well.
                resp += " <br><br><i>Note:<br>This setup page may stop communicating with the device due to the WiFi network change.<br>After you switch your PC/phone to the new WiFi network, open <a href='http://";
                resp += params.host;
                resp += ".local'>http://";
                resp += params.host;
                resp += ".local</a> (or the new IP: ";
                resp += result.ip.toString();
                resp += ") to reach the ESP again.</i><br><p style='text-align: center'>Do you want to proceed with a ESP restart right now?</p><div id='action-restart-required'></div>";
            }
            result.status = 200;
            result.body = resp;
            setTaskWdt(params.wdtTimeout);
            return result;
        }
    }

    setTaskWdt(params.wdtTimeout);
    result.status = 401;
    result.body = "Wrong credentials provided";
    return result;
}

WiFiStartResult WiFiService::startWiFi(CredentialManager* credentialManager, fs::FS* filesystem, const char* configFile, uint32_t timeout) {
    WiFiStartResult result;
    WiFi.mode(WIFI_STA);
    WiFiCredential* bestCred = nullptr;

    if (credentialManager) {
#ifdef ESP32
        credentialManager->loadFromNVS();
#else
        credentialManager->loadFromFS();
#endif
#ifdef BOARD_HAS_SDIO_ESP_HOSTED
        delay(500);
        WiFi.mode(WIFI_STA);
        delay(500);
        WiFi.disconnect(false, true, 1000);
        delay(500);
#endif
        std::vector<WiFiCredential>* creds = credentialManager->getCredentials();
        if (creds && creds->size() > 0) {
            int networksFound = WiFi.scanNetworks();
            if (networksFound > 0) {
                int32_t bestRSSI = -200;

                for (int i = 0; i < networksFound; i++) {
                    String scannedSSID = WiFi.SSID(i);
                    int32_t scannedRSSI = WiFi.RSSI(i);
                    for (size_t j = 0; j < creds->size(); j++) {
                        if (strcmp((*creds)[j].ssid, scannedSSID.c_str()) == 0) {
                            if (scannedRSSI > bestRSSI) {
                                bestRSSI = scannedRSSI;
                                bestCred = &(*creds)[j];
                            }
                            break;
                        }
                    }
                }
                WiFi.scanDelete();

                if (bestCred != nullptr) {
                    if (bestCred->local_ip != IPAddress(0, 0, 0, 0)) {
                        log_info("Configuring static IP: %s, GW: %s, SN: %s",
                                 bestCred->local_ip.toString().c_str(),
                                 bestCred->gateway.toString().c_str(),
                                 bestCred->subnet.toString().c_str());

                        IPAddress dns1 = bestCred->dns1;
                        IPAddress dns2 = bestCred->dns2;
                        bool hasDns1 = dns1 != IPAddress(0, 0, 0, 0);
                        bool hasDns2 = dns2 != IPAddress(0, 0, 0, 0);
                        bool ok = false;
                        if (hasDns1 && hasDns2) {
                            ok = WiFi.config(bestCred->local_ip, bestCred->gateway, bestCred->subnet, dns1, dns2);
                        } else if (hasDns1) {
                            ok = WiFi.config(bestCred->local_ip, bestCred->gateway, bestCred->subnet, dns1);
                        } else {
                            ok = WiFi.config(bestCred->local_ip, bestCred->gateway, bestCred->subnet);
                        }

                        if (!ok) {
                            log_error("Failed to configure static IP");
                        }
                    }

                    if (credentialManager->getPassword(bestCred->ssid).length() > 0) {
                        log_info("Connecting to %s (RSSI: %d dBm)...", bestCred->ssid, bestRSSI);
                        WiFi.begin(bestCred->ssid, credentialManager->getPassword(bestCred->ssid).c_str());

                        int tryDelay = timeout / 10;
                        int numberOfTries = 10;
                        while (numberOfTries > 0) {
                            switch (WiFi.status()) {
                            case WL_NO_SSID_AVAIL:   log_debug("[WiFi] SSID not found"); break;
                            case WL_CONNECTION_LOST: log_debug("[WiFi] Connection was lost"); break;
                            case WL_SCAN_COMPLETED:  log_debug("[WiFi] Scan is completed"); break;
                            case WL_DISCONNECTED:    log_debug("[WiFi] WiFi is disconnected"); break;
                            case WL_CONNECT_FAILED:
                                log_debug("[WiFi] Failed - WiFi not connected!");
                                result.action = WiFiStartAction::StartAp;
                                return result;
                            case WL_CONNECTED:
                                log_debug("[WiFi] WiFi is connected!  IP address: %s", WiFi.localIP().toString().c_str());
                                // Debug: print the actual STA configuration as seen by the core
                                logCurrentStaNetworkConfig();
                                result.ip = WiFi.localIP();
                                result.action = WiFiStartAction::Connected;
                                return result;
                            default:
                                log_debug("[WiFi] WiFi Status: %d", WiFi.status());
                                break;
                            }
                            delay(tryDelay);
#if defined(ESP8266)
                            ESP.wdtFeed();
#else
                            resetTaskWdtIfSubscribed();
#endif
                            numberOfTries--;
                        }

                        log_debug("[WiFi] Failed to connect to WiFi!");
                        WiFi.disconnect();
                        result.action = WiFiStartAction::StartAp;
                        return result;
                    }
                }
            }
            result.action = WiFiStartAction::StartAp;
            return result;
        }
    }

    if (bestCred && strlen(bestCred->ssid)) {
        WiFi.setHostname(credentialManager->getHostname().c_str());
        WiFi.begin(bestCred->ssid, credentialManager->getPassword(bestCred->ssid).c_str());
        log_debug("Connecting to %s", bestCred->ssid);

        int tryDelay = timeout / 10;
        int numberOfTries = 10;

        while (true) {
            switch (WiFi.status()) {
            case WL_NO_SSID_AVAIL:   log_debug("[WiFi] SSID not found"); break;
            case WL_CONNECTION_LOST: log_debug("[WiFi] Connection was lost"); break;
            case WL_SCAN_COMPLETED:  log_debug("[WiFi] Scan is completed"); break;
            case WL_DISCONNECTED:    log_debug("[WiFi] WiFi is disconnected"); break;
            case WL_CONNECT_FAILED:
                log_debug("[WiFi] Failed - WiFi not connected!");
                result.action = WiFiStartAction::Failed;
                return result;
            case WL_CONNECTED:
                log_debug("[WiFi] WiFi is connected!  IP address: %s", WiFi.localIP().toString().c_str());
                // Debug: print the actual STA configuration as seen by the core
                logCurrentStaNetworkConfig();
                result.ip = WiFi.localIP();
                result.action = WiFiStartAction::Connected;
                return result;
            default:
                log_debug("[WiFi] WiFi Status: %d", WiFi.status());
                break;
            }
            delay(tryDelay);
#if defined(ESP8266)
            ESP.wdtFeed();
#else
            resetTaskWdtIfSubscribed();
#endif
            if (numberOfTries <= 0) {
                log_debug("[WiFi] Failed to connect to WiFi!");
                WiFi.disconnect();
                result.action = WiFiStartAction::Failed;
                return result;
            }
            else {
                numberOfTries--;
            }
        }
    }

    result.action = WiFiStartAction::StartAp;
    return result;
}


bool WiFiService::startAccessPoint(WiFiConnectParams& params, IPAddress& outIp) {
    delay(100);
    WiFi.mode(WIFI_AP);

    IPAddress apIP = params.config.local_ip != IPAddress(0,0,0,0) ? params.config.local_ip : IPAddress(192, 168, 4, 1);
    IPAddress gateway = params.config.gateway != IPAddress(0,0,0,0) ? params.config.gateway : IPAddress(192, 168, 4, 1);
    IPAddress netmask = params.config.subnet != IPAddress(0,0,0,0) ? params.config.subnet : IPAddress(255, 255, 255, 0);

    // Configure AP IP parameters
    WiFi.softAPConfig(apIP, gateway, netmask);

    if (!WiFi.softAP(params.config.ssid, params.password.c_str())) {
        log_error("Captive portal failed to start: WiFi.softAP() failed!");
        return false;
    }
    outIp = WiFi.softAPIP();
    return true;
}

bool WiFiService::startMDNSResponder(DNSServer*& dnsServer, const String& host, uint16_t port, const IPAddress& serverIp) {
    if (dnsServer) {
        delete dnsServer;
        dnsServer = nullptr;
    }
    dnsServer = new DNSServer();
    dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    if (!dnsServer->start(53, "*", serverIp)) {
        log_error("Captive portal failed to start: no sockets for DNS server available!");
        return false;
    }

    if (!MDNS.begin(host.c_str())) {
        log_error("MDNS responder not started");
        return false;
    } else {
        log_debug("MDNS responder started %s (AP mode)", host.c_str());
        MDNS.addService("http", "tcp", port);
        MDNS.setInstanceName("esp-fs-webserver");
    }
    return true;
}

bool WiFiService::startMDNSOnly(const String& host, uint16_t port) {
    if (!MDNS.begin(host.c_str())) {
        log_error("MDNS responder not started");
        return false;
    }

    log_debug("MDNS responder started %s (STA mode)", host.c_str());
    MDNS.addService("http", "tcp", port);
    MDNS.setInstanceName("esp-fs-webserver");
    return true;
}
