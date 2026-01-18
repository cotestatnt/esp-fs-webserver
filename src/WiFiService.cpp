#include "WiFiService.h"
#include "Json.h"

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
    // Ensure current task is subscribed to WDT
    esp_task_wdt_add(NULL);
#elif defined(ESP8266)
    ESP.wdtDisable();
    ESP.wdtEnable(timeout);
#endif
}

void WiFiService::applyPersistentConfig(bool persistentEnabled, const String& ssid, const String& pass) {
    if (!persistentEnabled) {
        WiFi.persistent(false);
#if defined(ESP8266)
        struct station_config stationConf;
        wifi_station_get_config_default(&stationConf);
        memset(&stationConf, 0, sizeof(stationConf));
        wifi_station_set_config(&stationConf);
#elif defined(ESP32)
        wifi_config_t stationConf;
        esp_err_t err = esp_wifi_get_config(WIFI_IF_STA, &stationConf);
        if (err == ESP_OK) {
            memset(&stationConf, 0, sizeof(stationConf));
            memcpy(&stationConf.sta.ssid, ssid.c_str(), ssid.length());
            memcpy(&stationConf.sta.password, pass.c_str(), pass.length());
            err = esp_wifi_set_config(WIFI_IF_STA, &stationConf);
            if (err) {
                log_error("Set WiFi config: %s", esp_err_to_name(err));
            }
        }
#endif
        return;
    }

    WiFi.persistent(true);
#if defined(ESP8266)
    struct station_config stationConf;
    wifi_station_get_config_default(&stationConf);
    memset(&stationConf, 0, sizeof(stationConf));
    os_memcpy(&stationConf.ssid, ssid.c_str(), ssid.length());
    os_memcpy(&stationConf.password, pass.c_str(), pass.length());
    wifi_set_opmode(STATION_MODE);
    wifi_station_set_config(&stationConf);
#elif defined(ESP32)
    wifi_config_t stationConf;
    esp_err_t err = esp_wifi_get_config(WIFI_IF_STA, &stationConf);
    if (err == ESP_OK) {
        memset(&stationConf, 0, sizeof(stationConf));
        memcpy(&stationConf.sta.ssid, ssid.c_str(), ssid.length());
        memcpy(&stationConf.sta.password, pass.c_str(), pass.length());
        err = esp_wifi_set_config(WIFI_IF_STA, &stationConf);
        if (err) {
            log_error("Set WiFi config: %s", esp_err_to_name(err));
        }
    }
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
    if (WiFi.status() == WL_CONNECTED && !params.changeSSID && WiFi.getMode() != WIFI_AP) {
        String resp;
        resp.reserve(512);
        resp  = "ESP is already connected to <b>";
        resp += WiFi.SSID();
        resp += "</b> WiFi!<br>Do you want close this connection and attempt to connect to <b>";
        resp += params.ssid;
        resp += "</b>?<br><br><i>Note:<br>Flash stored WiFi credentials will be updated.<br>The ESP will no longer be reachable from this web page due to the change of WiFi network.<br>To find out the new IP address, check the serial monitor or your router.<br></i>";
        result.status = 200;
        result.body = resp;
        return result;
    }

    if (params.ssid.length() && params.password.length()) {
        setTaskWdt(params.wdtLongTimeout);
        WiFi.mode(WIFI_AP_STA);

        if (params.noDHCP) {
            log_info("Manual config WiFi connection with IP: %s", params.local_ip.toString().c_str());
            if (!WiFi.config(params.local_ip, params.gateway, params.subnet)) {
                log_error("STA Failed to configure");
            }
        }

        Serial.print("\n\n\nConnecting to ");
        Serial.println(params.ssid);
        WiFi.begin(params.ssid.c_str(), params.password.c_str());

        if (WiFi.status() == WL_CONNECTED && params.changeSSID) {
            log_info("Disconnect from current WiFi network");
            WiFi.disconnect();
            delay(10);
        }

        uint32_t beginTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(250);
            Serial.print("*");
#if defined(ESP8266)
            ESP.wdtFeed();
#else
            esp_task_wdt_reset();
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
            String serverLoc = F("http://");
            for (int i = 0; i < 4; i++) {
                if (i) serverLoc += ".";
                serverLoc += result.ip[i];
            }
            serverLoc += "/setup";
            String resp;
            resp  = "ESP successfully connected to ";
            resp += params.ssid;
            resp += " WiFi network. <br><b>Restart ESP now?</b><br><br><i>Note: disconnect your browser from ESP AP and then reload <a href='";
            resp += serverLoc;
            resp += "'>";
            resp += serverLoc;
            resp += "</a> or <a href='http://";
            resp += params.host;
            resp += ".local'>http://";
            resp += params.host;
            resp += ".local</a></i>";
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

#if ESP_FS_WS_SETUP
    if (filesystem && configFile) {
        File file = filesystem->open(configFile, "r");
        if (file) {
            CJSON::Json doc;
            String content = "";
            while (file.available()) {
                content += (char)file.read();
            }
            file.close();
            if (doc.parse(content)) {
                bool dhcp = false;
                IPAddress local_ip, subnet, gateway;
                if (doc.getBool("dhcp", dhcp) && dhcp == true) {
                    String gw, sn, ip;
                    if (doc.getString("gateway", gw)) gateway.fromString(gw);
                    if (doc.getString("subnet", sn)) subnet.fromString(sn);
                    if (doc.getString("ip_address", ip)) local_ip.fromString(ip);
                    log_info("Manual config WiFi connection with IP: %s\n", local_ip.toString().c_str());
                    if (!WiFi.config(local_ip, gateway, subnet)) {
                        log_error("STA Failed to configure");
                    }
                    delay(100);
                }
            } else {
                log_error("Failed to parse WiFi config file");
            }
        }
    }
#endif

    WiFi.mode(WIFI_STA);

    if (credentialManager) {
#ifdef ESP32
        credentialManager->loadFromNVS();
#else
        credentialManager->loadFromFS();
#endif

        std::vector<WiFiCredential>* creds = credentialManager->getCredentials();
        if (creds && creds->size() > 0) {
            int networksFound = WiFi.scanNetworks();
            if (networksFound > 0) {
                int32_t bestRSSI = -200;
                WiFiCredential* bestCred = nullptr;

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
                        if (!WiFi.config(bestCred->local_ip, bestCred->gateway, bestCred->subnet)) {
                            log_error("Failed to configure static IP");
                        }
                    }

                    String plainPassword = credentialManager->getPassword(bestCred->ssid);
                    if (plainPassword.length() > 0) {
                        log_info("Connecting to %s (RSSI: %d dBm)...", bestCred->ssid, bestRSSI);
                        WiFi.begin(bestCred->ssid, plainPassword.c_str());

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
                                result.ip = WiFi.localIP();
                                result.action = WiFiStartAction::Connected;
                                return result;
                            default:
                                log_debug("[WiFi] WiFi Status: %d", WiFi.status());
                                break;
                            }
                            delay(tryDelay);
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

#if defined(ESP8266)
    struct station_config conf;
    wifi_station_get_config_default(&conf);
    const char* _ssid = reinterpret_cast<const char*>(conf.ssid);
    const char* _pass = reinterpret_cast<const char*>(conf.password);
#elif defined(ESP32)
    wifi_config_t conf;
    esp_err_t err = esp_wifi_get_config(WIFI_IF_STA, &conf);
    if (err) {
        log_error("Get WiFi config: %s", esp_err_to_name(err));
        result.action = WiFiStartAction::Failed;
        return result;
    }
    const char* _ssid = reinterpret_cast<const char*>(conf.sta.ssid);
    const char* _pass = reinterpret_cast<const char*>(conf.sta.password);
#endif

    if (strlen(_ssid) && strlen(_pass)) {
        WiFi.begin(_ssid, _pass);
        log_debug("Connecting to %s, %s", _ssid, _pass);

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
                result.ip = WiFi.localIP();
                result.action = WiFiStartAction::Connected;
                return result;
            default:
                log_debug("[WiFi] WiFi Status: %d", WiFi.status());
                break;
            }
            delay(tryDelay);
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

bool WiFiService::startAccessPoint(const char* ssid, const char* pass, IPAddress& outIp) {
    delay(100);
    WiFi.mode(WIFI_AP);

    IPAddress apIP(192, 168, 4, 1);
    IPAddress netmask(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, apIP, netmask);

    if (!WiFi.softAP(ssid, pass)) {
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
        MDNS.setInstanceName("async-fs-webserver");
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
    MDNS.setInstanceName("async-fs-webserver");
    return true;
}
