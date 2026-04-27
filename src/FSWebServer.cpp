#include "FSWebServer.h"

#if defined(ESP32)
    #include "mimetable/mimetable.h"
#endif

namespace {
String serializeJsonDocument(cJSON *root) {
    if (!root) {
        return "{}";
    }

    char *raw = cJSON_PrintUnformatted(root);
    String out = raw ? String(raw) : String("{}");
    if (raw) {
        free(raw);
    }
    cJSON_Delete(root);
    return out;
}
}


void FSWebServer::begin(WebSocketsServer::WebSocketServerEvent wsEventHandler) {

    // Set build date as default firmware version (YYMMDDHHmm) from Version.h constexprs
    if (m_version.length() == 0)
        m_version = String(BUILD_TIMESTAMP);

//////////////////////    BUILT-IN HANDLERS    ///////////////////////////
#if ESP_FS_WS_SETUP
    ConfigUpgrader upgrader(m_filesystem, ESP_FS_WS_CONFIG_FILE);
    bool migratedLegacySetupStorage = false;
    upgrader.migrateLegacySetupStorage("/config/config.json", "/config", ESP_FS_WS_CONFIG_FOLDER, &migratedLegacySetupStorage);
    if (migratedLegacySetupStorage) {
        ESP.restart();
        return;
    }

    m_filesystem_ok = getSetupConfigurator()->checkConfigFile();
    if (!m_filesystem_ok) {
        log_error("Filesystem not available. Setup page will not work.");
    }

    // Close config file if it was opened during setup (will be reopened on demand when accessing config options)
    if (getSetupConfigurator()->isOpened()) {
        log_debug("Config file %s closed", ESP_FS_WS_CONFIG_FILE);
        getSetupConfigurator()->closeConfiguration();
    }   
         
    // Apply port override from config.json if present   
    uint16_t port = 0;
    if (getSetupConfigurator()->getOptionValue("port", port)) {
        log_debug("Port value %u read from config file", port);
        if (port != m_port && port != 0) {
            log_debug("Overriding server port to %u from config file", port);
            m_port = port;
            getSetupConfigurator()->closeConfiguration();
        }
    }     

    // Register the setup websocket before serving /setup to avoid a first-load race.
    initSetupWebSocket();

    // Setup page handlers
    on("*", HTTP_HEAD, [this]() { this->handleFileName(); });
    on("/", HTTP_GET, [this]() { this->handleIndex(); });
    on("/setup", HTTP_GET, [this]() { this->handleSetup(); });
    onNotFound([this]() { this->handleFileRequest(); });

    // Serve default logo from PROGMEM when no custom logo exists on filesystem
    on(ESP_FS_WS_CONFIG_FOLDER "/logo.svg", HTTP_GET, [this]() {
        const String logoBase = String(ESP_FS_WS_CONFIG_FOLDER) + "/logo";
        if (!m_filesystem->exists(logoBase + ".svg") && 
            !m_filesystem->exists(logoBase + ".svg.gz") &&
            !m_filesystem->exists(logoBase + ".png") &&
            !m_filesystem->exists(logoBase + ".jpg") &&
            !m_filesystem->exists(logoBase + ".gif")) {
                this->sendHeader(PSTR("Content-Encoding"), "gzip");
                this->sendHeader(PSTR("Cache-Control"), "public, max-age=86400");
                this->send_P(200, "image/svg+xml", (const char*)_aclogo_svg, sizeof(_aclogo_svg));
        } else {
           this->handleFileRequest();
        }
    });

    // WiFi info handler
    on("/wifi", HTTP_GET, [this]() {
        CJSON::Json doc;
        doc.setString("ssid", WiFi.SSID());
        doc.setNumber("rssi", WiFi.RSSI());
        this->send(200, "application/json", doc.serialize());
    });
#endif

#if ESP_FS_WS_WEBSOCKET
    if (wsEventHandler) {
        m_websocket = new WebSocketsServer(m_port + 1);
        m_websocket->begin();
        m_websocket->onEvent(wsEventHandler);
        log_debug("WebSocket server started on port %u", m_port + 1);
    }
#endif

#ifdef ESP32
    this->enableCrossOrigin(true);    
#endif
    WebServerClass::begin(m_port);
    log_debug("HTTP server started on port %u", m_port);
    
#if ESP_FS_WS_SETUP
    // Free SetupConfigurator memory after initialization (will be recreated lazily if needed)
    freeSetupConfigurator();
#endif
}

#if ESP_FS_WS_SETUP
void FSWebServer::initSetupWebSocket() {
    if (m_setupWebSocket) {
        return;
    }

    m_setupWebSocket = new WebSocketsServer(m_port + 2);
    m_setupWebSocket->begin();
    m_setupWebSocket->onEvent([this](uint8_t clientId, WStype_t type, uint8_t *payload, size_t length) {
        this->handleSetupWebSocket(clientId, type, payload, length);
    });
    log_debug("Setup WebSocket server started on port %u", m_port + 2);
}

void FSWebServer::releaseSetupWebSocketIfIdle() {
    m_releaseSetupWebSocketPending = false;
    if (!m_setupWebSocket || m_setupWebSocket->connectedClients() > 0) {
        return;
    }

    // Keep the setup websocket listener alive across page reloads.
    // Tearing it down on every disconnect creates a race where the browser's
    // next reload connects while the server is being closed and recreated.
    log_debug("Setup WebSocket idle; listener kept active");
}

void FSWebServer::handleSetupWebSocket(uint8_t clientId, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            sendSetupWsEvent(clientId, "setup.socket.ready");
            break;
        case WStype_TEXT:
            handleSetupWebSocketMessage(clientId, payload, length);
            break;
        case WStype_DISCONNECTED:
            if (m_setupWebSocket && m_setupWebSocket->connectedClients() == 0) {
                m_releaseSetupWebSocketPending = true;
            }
            break;
        default:
            break;
    }
}

void FSWebServer::handleSetupWebSocketMessage(uint8_t clientId, const uint8_t *data, size_t len) {
    if (!data || len == 0) {
        return;
    }

    cJSON *root = cJSON_ParseWithLengthOpts(reinterpret_cast<const char *>(data), len, nullptr, 0);
    if (!root) {
        sendSetupWsResponse(clientId, String(), false, "invalid", String(), "Invalid JSON");
        return;
    }

    cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON *payload = cJSON_GetObjectItemCaseSensitive(root, "payload");
    cJSON *reqId = cJSON_GetObjectItemCaseSensitive(root, "reqId");

    String reqIdStr = cJSON_IsString(reqId) && reqId->valuestring ? String(reqId->valuestring) : String();
    String nameStr = cJSON_IsString(name) && name->valuestring ? String(name->valuestring) : String();

    if (!cJSON_IsString(type) || strcmp(type->valuestring, "cmd") != 0 || nameStr.length() == 0) {
        cJSON_Delete(root);
        sendSetupWsResponse(clientId, reqIdStr, false, "invalid", String(), "Invalid command envelope");
        return;
    }

    if (nameStr == "status.get") {
        cJSON_Delete(root);
        sendSetupWsResponse(clientId, reqIdStr, true, nameStr.c_str(), buildSetupStatusPayload());
        return;
    }

    if (nameStr == "config.get") {
        cJSON_Delete(root);
        sendSetupWsResponse(clientId, reqIdStr, true, nameStr.c_str(), buildSetupConfigPayload());
        return;
    }

    if (nameStr == "credentials.get") {
        cJSON_Delete(root);
        sendSetupWsResponse(clientId, reqIdStr, true, nameStr.c_str(), buildSetupCredentialsPayload());
        return;
    }

    if (nameStr == "credentials.delete") {
        bool ok = false;
        if (m_credentialManager && cJSON_IsObject(payload)) {
            cJSON *index = cJSON_GetObjectItemCaseSensitive(payload, "index");
            if (cJSON_IsNumber(index)) {
                ok = m_credentialManager->removeCredential(static_cast<uint8_t>(index->valuedouble));
            }
        }
        if (ok) {
#if defined(ESP32)
            m_credentialManager->saveToNVS();
#elif defined(ESP8266)
            m_credentialManager->saveToFS();
#endif
        }
        cJSON_Delete(root);
        sendSetupWsResponse(clientId, reqIdStr, ok, nameStr.c_str(), buildSetupCredentialsPayload(), ok ? String() : String("Invalid credential index"));
        return;
    }

    if (nameStr == "credentials.clear") {
        bool ok = m_credentialManager != nullptr;
        if (m_credentialManager) {
            m_credentialManager->clearAll();
#if defined(ESP32)
            m_credentialManager->saveToNVS();
#elif defined(ESP8266)
            m_credentialManager->saveToFS();
#endif
        }
        cJSON_Delete(root);
        sendSetupWsResponse(clientId, reqIdStr, ok, nameStr.c_str(), buildSetupCredentialsPayload(), ok ? String() : String("Credential manager not available"));
        return;
    }

    if (nameStr == "wifi.scan") {
        WiFiScanResult scan = WiFiService::scanNetworks();
        cJSON *scanPayload = cJSON_CreateObject();
        if (scan.reload) {
            cJSON_AddBoolToObject(scanPayload, "reload", true);
        } else {
            cJSON *networks = cJSON_Parse(scan.json.c_str());
            if (!networks) {
                networks = cJSON_CreateArray();
            }
            cJSON_AddItemToObject(scanPayload, "networks", networks);
        }
        cJSON_Delete(root);
        sendSetupWsResponse(clientId, reqIdStr, true, nameStr.c_str(), serializeJsonDocument(scanPayload));
        return;
    }

    if (nameStr == "config.save") {
        bool ok = false;
        if (cJSON_IsObject(payload)) {
            cJSON *configNode = cJSON_GetObjectItemCaseSensitive(payload, "config");
            if (configNode) {
                char *rawConfig = cJSON_Print(configNode);
                if (rawConfig) {
                    ok = saveSetupConfigJson(String(rawConfig));
                    free(rawConfig);
                }
            }
        }
        cJSON_Delete(root);
        sendSetupWsResponse(clientId, reqIdStr, ok, nameStr.c_str(), buildSetupConfigPayload(), ok ? String() : String("Failed to save config"));
        return;
    }

    if (nameStr == "device.restart") {
        cJSON_Delete(root);
        sendSetupWsResponse(clientId, reqIdStr, true, nameStr.c_str());
        m_pendingSetupRestartAt = millis() + 200;
        return;
    }

    if (nameStr == "wifi.connect") {
        WiFiConnectParams params;
        bool persistent = true;
        bool confirmed = false;

        if (cJSON_IsObject(payload)) {
            cJSON *ssid = cJSON_GetObjectItemCaseSensitive(payload, "ssid");
            cJSON *password = cJSON_GetObjectItemCaseSensitive(payload, "password");
            cJSON *host = cJSON_GetObjectItemCaseSensitive(payload, "hostname");
            cJSON *dhcp = cJSON_GetObjectItemCaseSensitive(payload, "dhcp");
            cJSON *persistentNode = cJSON_GetObjectItemCaseSensitive(payload, "persistent");
            cJSON *confirmedNode = cJSON_GetObjectItemCaseSensitive(payload, "confirmed");
            cJSON *ip = cJSON_GetObjectItemCaseSensitive(payload, "ip_address");
            cJSON *gateway = cJSON_GetObjectItemCaseSensitive(payload, "gateway");
            cJSON *subnet = cJSON_GetObjectItemCaseSensitive(payload, "subnet");
            cJSON *dns1 = cJSON_GetObjectItemCaseSensitive(payload, "dns1");
            cJSON *dns2 = cJSON_GetObjectItemCaseSensitive(payload, "dns2");

            if (cJSON_IsString(ssid) && ssid->valuestring) {
                strlcpy(params.config.ssid, ssid->valuestring, sizeof(params.config.ssid));
            }
            if (cJSON_IsString(password) && password->valuestring) {
                params.password = password->valuestring;
            }
            if (cJSON_IsString(host) && host->valuestring) {
                String requestedHost = String(host->valuestring);
                requestedHost.trim();
                if (requestedHost.length() > 32) {
                    requestedHost.remove(32);
                }
                if (requestedHost.length()) {
                    m_host = requestedHost;
                    if (m_credentialManager) {
                        m_credentialManager->setHostname(m_host.c_str());
                    }
                }
            }

            params.dhcp = !cJSON_IsBool(dhcp) || cJSON_IsTrue(dhcp);
            persistent = !cJSON_IsBool(persistentNode) || cJSON_IsTrue(persistentNode);
            confirmed = cJSON_IsBool(confirmedNode) && cJSON_IsTrue(confirmedNode);

            if (!params.dhcp) {
                if (cJSON_IsString(ip) && ip->valuestring) params.config.local_ip.fromString(ip->valuestring);
                if (cJSON_IsString(gateway) && gateway->valuestring) params.config.gateway.fromString(gateway->valuestring);
                if (cJSON_IsString(subnet) && subnet->valuestring) params.config.subnet.fromString(subnet->valuestring);
                if (cJSON_IsString(dns1) && dns1->valuestring) params.config.dns1.fromString(dns1->valuestring);
                if (cJSON_IsString(dns2) && dns2->valuestring) params.config.dns2.fromString(dns2->valuestring);
            }
        }

        if (params.password.length() == 0 && strlen(params.config.ssid) && m_credentialManager &&
            m_credentialManager->checkSSIDExists(params.config.ssid)) {
            String stored = m_credentialManager->getPassword(params.config.ssid);
            if (stored.length()) {
                params.password = stored;
            }
        }

        bool wasStaConnected = (WiFi.status() == WL_CONNECTED && WiFi.getMode() != WIFI_AP);
        params.fromApClient = m_isApMode;
        params.host = m_host;
        params.timeout = m_timeout;
        params.wdtLongTimeout = AWS_LONG_WDT_TIMEOUT;
        params.wdtTimeout = AWS_WDT_TIMEOUT;

        if (!params.fromApClient && !confirmed && WiFi.status() == WL_CONNECTED) {
            cJSON *confirmPayload = cJSON_CreateObject();
            cJSON_AddBoolToObject(confirmPayload, "confirmRequired", true);
            cJSON_AddStringToObject(confirmPayload, "ssid", params.config.ssid);
            cJSON_Delete(root);
            sendSetupWsResponse(clientId, reqIdStr, true, nameStr.c_str(), serializeJsonDocument(confirmPayload));
            return;
        }

        cJSON_Delete(root);
        sendSetupWsResponse(clientId, reqIdStr, true, nameStr.c_str(), String("{\"accepted\":true}"));
        queueSetupWifiConnect(clientId, params, persistent, wasStaConnected && !params.fromApClient, params.fromApClient);
        return;
    }

    cJSON_Delete(root);
    sendSetupWsResponse(clientId, reqIdStr, false, nameStr.c_str(), String(), "Unknown command");
}

void FSWebServer::sendSetupWsResponse(uint8_t clientId, const String &reqId, bool ok, const char *name, const String &payload, const String &error) {
    if (!m_setupWebSocket || !m_setupWebSocket->clientIsConnected(clientId)) {
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "res");
    cJSON_AddStringToObject(root, "name", name ? name : "unknown");
    if (reqId.length()) {
        cJSON_AddStringToObject(root, "reqId", reqId.c_str());
    }
    cJSON_AddBoolToObject(root, "ok", ok);
    if (payload.length()) {
        cJSON *payloadNode = cJSON_Parse(payload.c_str());
        if (payloadNode) {
            cJSON_AddItemToObject(root, "payload", payloadNode);
        }
    }
    if (error.length()) {
        cJSON_AddStringToObject(root, "error", error.c_str());
    }

    String message = serializeJsonDocument(root);
    m_setupWebSocket->sendTXT(clientId, message.c_str(), message.length());
}

void FSWebServer::sendSetupWsEvent(uint8_t clientId, const char *name, const String &payload) {
    if (!m_setupWebSocket || !m_setupWebSocket->clientIsConnected(clientId)) {
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "evt");
    cJSON_AddStringToObject(root, "name", name ? name : "unknown");
    if (payload.length()) {
        cJSON *payloadNode = cJSON_Parse(payload.c_str());
        if (payloadNode) {
            cJSON_AddItemToObject(root, "payload", payloadNode);
        }
    }

    String message = serializeJsonDocument(root);
    m_setupWebSocket->sendTXT(clientId, message.c_str(), message.length());
}

String FSWebServer::buildSetupStatusPayload() const {
    cJSON *payload = cJSON_CreateObject();
    cJSON *status = cJSON_CreateObject();
    cJSON_AddStringToObject(status, "firmware", m_version.c_str());

    String mode;
    if (WiFi.status() == WL_CONNECTED) {
        mode = "Station (";
        mode += WiFi.SSID();
        mode += ")";
    } else {
        mode = "Access Point";
    }
    cJSON_AddStringToObject(status, "mode", mode.c_str());
    String ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    cJSON_AddStringToObject(status, "ip", ip.c_str());
    cJSON_AddStringToObject(status, "hostname", m_host.c_str());
    cJSON_AddStringToObject(status, "path", String(ESP_FS_WS_CONFIG_FILE).substring(1).c_str());
    cJSON_AddStringToObject(status, "liburl", LIB_URL);

    String logoPath;
    if (const_cast<FSWebServer *>(this)->getSetupConfigurator()->getOptionValue("img-logo", logoPath) && logoPath.length() > 0) {
        cJSON_AddStringToObject(status, "img-logo", logoPath.c_str());
    }

    String pageTitle;
    if (const_cast<FSWebServer *>(this)->getSetupConfigurator()->getOptionValue("page-title", pageTitle) && pageTitle.length() > 0) {
        cJSON_AddStringToObject(status, "page-title", pageTitle.c_str());
    }

    cJSON_AddItemToObject(payload, "status", status);
    return serializeJsonDocument(payload);
}

String FSWebServer::buildSetupConfigPayload() const {
    cJSON *payload = cJSON_CreateObject();
    cJSON *config = nullptr;
    if (m_filesystem && m_filesystem->exists(ESP_FS_WS_CONFIG_FILE)) {
        File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "r");
        if (file) {
            String content = file.readString();
            file.close();
            config = cJSON_Parse(content.c_str());
        }
    }
    if (!config) {
        config = cJSON_CreateObject();
    }
    cJSON_AddItemToObject(payload, "config", config);
    return serializeJsonDocument(payload);
}

String FSWebServer::buildSetupCredentialsPayload() const {
    cJSON *payload = cJSON_CreateObject();
    cJSON *credentials = cJSON_CreateArray();

    if (m_credentialManager) {
        std::vector<WiFiCredential> *creds = m_credentialManager->getCredentials();
        if (creds) {
            for (size_t i = 0; i < creds->size(); ++i) {
                const WiFiCredential &cred = (*creds)[i];
                cJSON *item = cJSON_CreateObject();
                cJSON_AddNumberToObject(item, "index", static_cast<double>(i));
                cJSON_AddStringToObject(item, "ssid", cred.ssid);
                if (cred.local_ip != IPAddress(0, 0, 0, 0)) {
                    cJSON_AddStringToObject(item, "ip", cred.local_ip.toString().c_str());
                    cJSON_AddStringToObject(item, "gateway", cred.gateway.toString().c_str());
                    cJSON_AddStringToObject(item, "subnet", cred.subnet.toString().c_str());
                    if (cred.dns1 != IPAddress(0, 0, 0, 0)) {
                        cJSON_AddStringToObject(item, "dns1", cred.dns1.toString().c_str());
                    }
                    if (cred.dns2 != IPAddress(0, 0, 0, 0)) {
                        cJSON_AddStringToObject(item, "dns2", cred.dns2.toString().c_str());
                    }
                }
                cJSON_AddBoolToObject(item, "hasPassword", cred.pwd_len > 0);
                cJSON_AddItemToArray(credentials, item);
            }
        }
    }

    cJSON_AddItemToObject(payload, "credentials", credentials);
    return serializeJsonDocument(payload);
}

bool FSWebServer::saveSetupConfigJson(const String &jsonText) {
    if (!m_filesystem) {
        return false;
    }

    File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "w");
    if (!file) {
        return false;
    }

    size_t written = file.print(jsonText);
    file.close();
    if (written == 0) {
        return false;
    }

    if (m_configSavedCallback) {
        m_configSavedCallback(ESP_FS_WS_CONFIG_FILE);
    }
    return true;
}

void FSWebServer::queueSetupWifiConnect(uint8_t clientId, const WiFiConnectParams &params, bool persistent, bool allowApFallback, bool fromApClient) {
    m_pendingSetupClientId = clientId;
    m_pendingSetupParams = params;
    m_pendingSetupPersistent = persistent;
    m_pendingSetupAllowApFallback = allowApFallback;
    m_pendingSetupFromApClient = fromApClient;
    m_pendingSetupWifiConnect = true;

    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "ssid", params.config.ssid);
    sendSetupWsEvent(clientId, "wifi.connect.started", serializeJsonDocument(payload));
}

void FSWebServer::processPendingSetupWifiConnection() {
    m_pendingSetupWifiConnect = false;

    WiFiConnectParams params = m_pendingSetupParams;
    bool persistent = m_pendingSetupPersistent;
    bool allowApFallback = m_pendingSetupAllowApFallback;
    bool fromApClient = m_pendingSetupFromApClient;
    uint8_t clientId = m_pendingSetupClientId;

    WiFiConnectResult result = WiFiService::connectWithParams(params);

    if (result.connected && persistent && strlen(params.config.ssid) && params.password.length() && m_credentialManager) {
        if (params.dhcp) {
            params.config.local_ip = IPAddress(0, 0, 0, 0);
            params.config.gateway = IPAddress(0, 0, 0, 0);
            params.config.subnet = IPAddress(0, 0, 0, 0);
            params.config.dns1 = IPAddress(0, 0, 0, 0);
            params.config.dns2 = IPAddress(0, 0, 0, 0);
        }
        if (!m_credentialManager->updateCredential(params.config, params.password.c_str())) {
            m_credentialManager->addCredential(params.config, params.password.c_str());
        }
#if defined(ESP32)
        m_credentialManager->saveToNVS();
#elif defined(ESP8266)
        m_credentialManager->saveToFS();
#endif
    }

    if (result.connected) {
        m_serverIp = result.ip;
    } else if (allowApFallback && strlen(m_apSSID) > 0) {
        startCaptivePortal(m_apSSID, m_apPassword, "/setup");
    }

    cJSON *payload = cJSON_CreateObject();
    cJSON_AddBoolToObject(payload, "connected", result.connected);
    cJSON_AddNumberToObject(payload, "status", result.status);
    cJSON_AddStringToObject(payload, "body", result.body.c_str());
    cJSON_AddStringToObject(payload, "ip", result.ip.toString().c_str());
    cJSON_AddStringToObject(payload, "hostname", m_host.c_str());
    cJSON_AddBoolToObject(payload, "fromApClient", fromApClient);
    sendSetupWsEvent(clientId, result.connected ? "wifi.connect.success" : "wifi.connect.error", serializeJsonDocument(payload));
}
#endif


void FSWebServer::handleIndex(){
    log_debug("handleIndex");
    if (m_filesystem->exists("/index.htm")) {
        File dataFile = m_filesystem->open("/index.htm", "r");
        this->streamFile(dataFile, "text/html");
        dataFile.close();
        log_debug("Serving /index.htm");
    } 
    else if (m_filesystem->exists("/index.html")) {
        File dataFile = m_filesystem->open("/index.html", "r");
        this->streamFile(dataFile, "text/html");
        dataFile.close();
        log_debug("Serving /index.html");
    }

    #if ESP_FS_WS_SETUP 
    else {
        handleSetup();
        log_debug("No index file found, redirecting to /setup");
    }
    #endif
}

void FSWebServer::printFileList(fs::FS &fs, const char * dirname, uint8_t levels) {
    printFileList(fs, dirname, levels, Serial);
}

void FSWebServer::printFileList(fs::FS &fs, const char * dirname, uint8_t levels, Print& out) {
    out.print("\nListing directory: ");
    out.println(dirname);
    File root = fs.open(dirname, "r");
    if (!root) {
        out.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        out.println(" - not a directory");
        return;
    }
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            if (levels) {
                #ifdef ESP32
                printFileList(fs, file.path(), levels - 1, out);
                #elif defined(ESP8266)
                printFileList(fs, file.fullName(), levels - 1, out);
                #endif
            }
        } else {
            String line = "|__ FILE: ";
            if (typeName == "SPIFFS") {
                #ifdef ESP32
                line += file.path();
                #elif defined(ESP8266)
                line += file.fullName();
                #endif
            } else {
                line += file.name();
            }            
            line += " (";
            line += (unsigned long)file.size();
            line += " bytes)";
            out.println(line);
        }
        file = root.openNextFile();
    }
}

#if ESP_FS_WS_EDIT
void FSWebServer::enableFsCodeEditor() {
    on("/status", HTTP_GET, [this]() { this->handleFsStatus(); });
    on("/list", HTTP_GET, [this]() { this->handleFileList(); });
    on("/edit", HTTP_PUT, [this]() { this->handleFileCreate(); });
    on("/edit", HTTP_GET, [this]() { this->handleFileEdit(); });
    on("/edit", HTTP_DELETE, [this]() { this->handleFileDelete(); });
    on("/edit", HTTP_POST,
        [this]() { this->sendOK(); },
        [this]() { this->handleFileUpload(); }
    );
}
#endif

void FSWebServer::setAuthentication(const char* user, const char* pswd) {
    // Free previous allocations if they exist
    if (m_pageUser) {
        free(m_pageUser);
        m_pageUser = nullptr;
    }
    if (m_pagePswd) {
        free(m_pagePswd);
        m_pagePswd = nullptr;
    }
    
    // Validate input parameters
    if (!user || !pswd || strlen(user) == 0 || strlen(pswd) == 0) {
        log_error("Invalid authentication credentials");
        return;
    }
    
    // Allocate with proper size (+1 for null terminator)
    size_t userLen = strlen(user) + 1;
    size_t pswnLen = strlen(pswd) + 1;
    
    m_pageUser = (char*) malloc(userLen);
    m_pagePswd = (char*) malloc(pswnLen);
    
    if (m_pageUser && m_pagePswd) {
        strncpy(m_pageUser, user, userLen - 1);
        strncpy(m_pagePswd, pswd, pswnLen - 1);
        m_pageUser[userLen - 1] = '\0';
        m_pagePswd[pswnLen - 1] = '\0';
        log_debug("Authentication credentials set successfully");
    } 
    else {
        log_error("Failed to allocate memory for authentication credentials");
        if (m_pageUser) {
            free(m_pageUser);
            m_pageUser = nullptr;
        }
        if (m_pagePswd) {
            free(m_pagePswd);
            m_pagePswd = nullptr;
        }
    }
}

void FSWebServer::handleFileName() {
    if (m_filesystem->exists(this->uri()))
        this->send(301, "text/plain", "OK");
    this->send(404, "text/plain", "File not found");
}

void FSWebServer::sendOK() {
  this->send(200, "text/plain", "OK");
}


// First try to find and return the requested file from the filesystem,
// and if it fails, return a 404 page with debug information        
void FSWebServer::handleFileRequest() {
    String _url = WebServerClass::urlDecode(this->uri());
    log_debug("handleFileRequest for %s", _url.c_str());

    // Check if authentication for all routes is turned on, and credentials are present:
    if (m_authAll && m_pageUser != nullptr) {
        if(!this->authenticate(m_pageUser, m_pagePswd))
            return this->requestAuthentication();
    }

#if defined(ESP8266)
    // ESP8266WebServer has its own mime namespace
    using namespace mime;
    String contentType = getContentType(_url);
#elif defined(ESP32)
    // Use local mimetable from library
    String contentType = mimetype::getContentType(_url);
#endif

    if (!m_filesystem->exists(_url)) {
        // Requested file not found, check if gzipped version exists
        log_debug("File %s not found, checking for gzipped version", this->uri().c_str());
        _url += ".gz";  
    }
    
    if (m_filesystem->exists(_url)) {
        File file = m_filesystem->open(_url , "r");
        if (file) {               
            this->streamFile(file, contentType);
            file.close();
            return; // If file was served, skip the rest
        } 
        else {
            log_debug("Failed to open file %s", _url.c_str());
            this->send(500, "text/plain", "FsWebServer: Failed to open file resource");
        } 
    }
    else {
        log_debug("File %s not found, checking for index redirection", this->uri().c_str());
        
        // File not found
        if (this->uri() == "/" && !m_filesystem->exists("/index.htm") && !m_filesystem->exists("/index.html")) {
            this->sendHeader(PSTR("Location"), "/");
            this->send(302, "text/plain", "");
            log_debug("Redirecting \"/\" to \"/setup\" (no index file found)");
            return;
        }
    }

    this->send(404, "text/plain", "FsWebServer: resource not found");
    log_debug("Resource %s not found", this->uri().c_str());
}


#if ESP_FS_WS_SETUP
void FSWebServer::handleSetup() {
    if (m_pageUser != nullptr) {
        if(!this->authenticate(m_pageUser, m_pagePswd))
            return this->requestAuthentication();
    }
    initSetupWebSocket();
    this->sendHeader(PSTR("Content-Encoding"), "gzip");
    this->sendHeader(PSTR("X-Config-File"), ESP_FS_WS_CONFIG_FILE);
    this->sendHeader(PSTR("Set-Cookie"), "esp_fs_ws_mode=dedicated; Path=/; SameSite=Lax");
    // Changed array name to match SEGGER Bin2C output
    this->send_P(200, "text/html", (const char*)_acsetup_min_htm, sizeof(_acsetup_min_htm));
}
#endif

#if ESP_FS_WS_SETUP
bool FSWebServer::createDirFromPath(const String& path) {
    String dir;
    dir.reserve(path.length());
    int p1 = 0;  int p2 = 0;
    while (p2 != -1) {
        p2 = path.indexOf("/", p1 + 1);
        dir += path.substring(p1, p2);
        // Check if its a valid dir
        if (dir.indexOf(".") == -1) {
            if (!m_filesystem->exists(dir)) {
                if (m_filesystem->mkdir(dir)) {
                    log_info("Folder %s created\n", dir.c_str());
                } else {
                    log_info("Error. Folder %s not created\n", dir.c_str());
                    return false;
                }
            }
        }
        p1 = p2;
    }
    return true;
}


/*
    Checks filename for character combinations that are not supported by FSBrowser (alhtough valid on SPIFFS).
    Returns an empty String if supported, or detail of error(s) if unsupported
*/
void FSWebServer::checkForUnsupportedPath(String& filename, String& error)
{
    if (!filename.startsWith("/")) {
        error += PSTR(" !! NO_LEADING_SLASH !! ");
    }
    if (filename.indexOf("//") != -1) {
        error += PSTR(" !! DOUBLE_SLASH !! ");
    }
    if (filename.endsWith("/")) {
        error += PSTR(" ! TRAILING_SLASH ! ");
    }
    log_debug("%s: %s", filename.c_str(), error.c_str());
}

void FSWebServer::handleFileUpload()
{
    HTTPUpload& upload = this->upload();
    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        String result;
        // Make sure paths always start with "/"
        if (!filename.startsWith("/")) {
            filename = "/" + filename;
        }
        checkForUnsupportedPath(filename, result);
        createDirFromPath(filename);

        if (result.length() > 0) {
            this->send(500, "text/plain", "INVALID FILENAME");
            return;
        }

        log_debug("handleFileUpload Name: %s\n", filename.c_str());
        m_uploadFile = m_filesystem->open(filename, "w");
        if (!m_uploadFile) {
            this->send(500, "text/plain", "CREATE FAILED");
            return;
        }
        log_debug("Upload: START, filename: %s\n", filename.c_str());
    } 
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (m_uploadFile) {
            size_t bytesWritten = m_uploadFile.write(upload.buf, upload.currentSize);
            if (bytesWritten != upload.currentSize) {
                this->send(500, "text/plain", "WRITE FAILED");
                return;
            }
        }
        log_debug("Upload: WRITE, Bytes: %d\n", upload.currentSize);
    } 
    else if (upload.status == UPLOAD_FILE_END) {
        #if defined(ESP32)
            const char* filepath = m_uploadFile.path();            
        #elif defined(ESP8266)
            const char* filepath = m_uploadFile.fullName();   
        #endif

        // Call config saved callback if this is the config file
        if (strcmp(filepath, ESP_FS_WS_CONFIG_FILE) == 0 && m_configSavedCallback) {
            log_debug("Config file saved, calling callback");
            m_configSavedCallback(filepath);
        }

        if (m_uploadFile) { 
            m_uploadFile.close();
        }
        log_debug("Upload: END, Size: %d\n", upload.totalSize);
    }
}

void FSWebServer::update_first()
{
    size_t fsize;
    if (this->hasArg("size"))
        fsize = this->arg("size").toInt();
    else {
        this->send(500, "text/plain", F("Request malformed: missing file size"));
        return;
    }

    HTTPUpload& upload = this->upload();
    if (upload.status == UPLOAD_FILE_START) {
        log_info("Receiving Update: %s, Size: %d", upload.filename.c_str(), fsize);
        if (!Update.begin(fsize)) {
            otaDone = 0;
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
        else {
            static uint32_t pTime = millis();
            if (millis() - pTime > 500) {
                pTime = millis();
                otaDone = 100 * Update.progress() / Update.size();
                log_info("OTA progress: %d%%", otaDone);
            }
        }
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            log_info("Update Success: %u bytes\nRebooting...", upload.totalSize);
        }
        else {
#if defined(ESP8266)
            log_error("%s\n", Update.getErrorString().c_str());
#elif defined(ESP32)
            log_error("%s\n", Update.errorString());
#endif
            otaDone = 0;
        }
    }
}


// the request handler is triggered after the upload has finished...
// create the response, add header, and send response
void FSWebServer::update_second()
{
    String txt;
    if (Update.hasError()) {
        txt = "Error! ";
#if defined(ESP8266)
        txt += Update.getErrorString().c_str();
#elif defined(ESP32)
        txt += Update.errorString();
#endif
    } else {
        txt = F("Update completed successfully. The ESP32 will restart");
    }
    log_info("%s", txt.c_str());
    this->send((Update.hasError()) ? 500 : 200, "text/plain", txt);
    if (!Update.hasError()) {
        delay(500);
        ESP.restart();
    }
}


#endif //ESP_FS_WS_SETUP

bool FSWebServer::startWiFi(uint32_t timeout, CallbackF fn) {
#ifdef ESP32
    if (m_credentialManager && m_credentialManager->loadFromNVS()) {
        log_debug("Credentials loaded from NVS");
        log_debug("%s", m_credentialManager->getDebugInfo().c_str());
    }
#else
    if (m_credentialManager && m_credentialManager->loadFromFS()) {
        log_debug("Credentials loaded from filesystem");
        log_debug("%s", m_credentialManager->getDebugInfo().c_str());
    }
#endif

    WiFiStartResult result = WiFiService::startWiFi(m_credentialManager, m_filesystem, ESP_FS_WS_CONFIG_FILE, timeout);
    if (result.action == WiFiStartAction::Connected) {
        m_serverIp = result.ip;
        m_isApMode = false;
    #if ESP_FS_WS_MDNS
        WiFiService::startMDNSOnly(m_host, m_port);
        log_info("mDNS started on http://%s.local", m_host.c_str());
    #endif
        return true;
    }

    if (result.action == WiFiStartAction::StartAp && strlen(m_apSSID) > 0) {
        log_info("Starting AP mode: SSID=%s", m_apSSID);
        startCaptivePortal(m_apSSID, m_apPassword, "/setup");
        return true;
    }
    return false;
}


bool FSWebServer::startMDNSResponder() {
#if ESP_FS_WS_MDNS
    return WiFiService::startMDNSResponder(m_dnsServer, m_host, m_port, m_serverIp);
#else
    return true;
#endif
}

void FSWebServer::redirect(const char* url) {
    if (strcmp(url, "/setup") == 0) {
        return this->handleSetup();
    }

    this->sendHeader(PSTR("Location"), url);
    this->send(302, "text/plain", "");
    log_debug("Redirecting %s", url);
}

bool FSWebServer::startCaptivePortal(const char* ssid, const char* pass, const char* redirectTargetURL) {
    WiFiConnectParams params;
    strncpy(params.config.ssid, ssid, sizeof(params.config.ssid) - 1);
    params.config.ssid[sizeof(params.config.ssid) - 1] = '\0';
    params.password = pass;
    return startCaptivePortal(params, redirectTargetURL);
}


bool FSWebServer::startCaptivePortal(WiFiConnectParams& params, const char *redirectTargetURL) {
    // Captive portal OS connectivity probes
    this->on("/generate_204", HTTP_GET, [this, redirectTargetURL]() { this->redirect(redirectTargetURL); }); // Android probe
    this->on("/hotspot-detect.html", HTTP_GET, [this, redirectTargetURL]() { this->redirect(redirectTargetURL); }); // iOS/macOS probe
    this->on("/ncsi.txt", HTTP_GET,  [this, redirectTargetURL]() { this->redirect(redirectTargetURL); });  // Windows NCSI probe
    this->on("/connecttest.txt", HTTP_GET,  [this, redirectTargetURL]() { this->redirect(redirectTargetURL); }); // Windows 8/10 probe
    this->on("/success.txt", HTTP_GET,  [this, redirectTargetURL]() { this->redirect(redirectTargetURL); }); 
    this->on("/library/test/success.html", HTTP_GET,  [this, redirectTargetURL]() { this->redirect(redirectTargetURL); }); // ChromeOS probe
    this->on("/redirect", HTTP_GET,  [this, redirectTargetURL]() { this->redirect(redirectTargetURL); });
    if (!WiFiService::startAccessPoint(params, m_serverIp)) {
        return false;
    }
    m_isApMode = true;
    this->startMDNSResponder();
    log_info("Captive portal started. Redirecting all requests to %s", redirectTargetURL);
    return true;
}


// edit page, in usefull in some situation, but if you need to provide only a web interface, you can disable
#if ESP_FS_WS_EDIT

void FSWebServer::handleFileEdit() {
  
    if (m_pageUser != nullptr) {
        if(!this->authenticate(m_pageUser, m_pagePswd))
            return this->requestAuthentication();
    }
    this->sendHeader(PSTR("Content-Encoding"), "gzip");
    this->send_P(200, "text/html", (const char*)_acedit_htm, sizeof(_acedit_htm));
}

/*
    Return the list of files in the directory specified by the "dir" query string parameter.
    Also demonstrates the use of chunked responses.
*/
void FSWebServer::handleFileList()
{
    if (!this->hasArg("dir")) {
        return this->send(400, "DIR ARG MISSING");
    }

    String path = this->arg("dir");
    log_debug("handleFileList: %s", path.c_str());
    if (path != "/" && !m_filesystem->exists(path)) {
        return this->send(400, "BAD PATH");
    }

    File root = m_filesystem->open(path, "r");
    CJSON::Json json_array;
    json_array.createArray();
    if (root.isDirectory()) {
        File file = root.openNextFile();
        while (file) {
            String filename;
            if (typeName.equals("SPIFFS")) {
                // SPIFFS returns full path and subfolders are unsupported, remove leading '/'                
                #ifdef ESP32
                filename += file.path();
                #elif defined(ESP8266)
                filename += file.fullName();
                #endif
                filename.remove(0, 1);
            } 
            else {
                filename = file.name();
                if (filename.lastIndexOf("/") > -1) {
                    filename.remove(0, filename.lastIndexOf("/") + 1);
                }
            }                    
            CJSON::Json item;
            item.setString("type", (file.isDirectory()) ? "dir" : "file");
            item.setNumber("size", file.size());
            item.setString("name", filename);
            json_array.add(item);
            file = root.openNextFile();
        }
    }
    this->send(200, "text/json", json_array.serialize());
}

/*
    Handle the creation/rename of a new file
    Operation      | req.responseText
    ---------------+--------------------------------------------------------------
    Create file    | parent of created file
    Create folder  | parent of created folder
    Rename file    | parent of source file
    Move file      | parent of source file, or remaining ancestor
    Rename folder  | parent of source folder
    Move folder    | parent of source folder, or remaining ancestor
*/
void FSWebServer::handleFileCreate()
{
    String path = this->arg("path");
    if (path.isEmpty()) {
        return this->send(400, "PATH ARG MISSING");
    }
    if (path == "/") {
        return this->send(400, "BAD PATH");
    }

    String src = this->arg("src");
    if (src.isEmpty())  {
        // No source specified: creation
        log_debug("handleFileCreate: %s\n", path.c_str());
        if (path.endsWith("/")) {
            // Create a folder
            path.remove(path.length() - 1);
            if (!m_filesystem->mkdir(path)) {
                return this->send(500, "MKDIR FAILED");
            }
        }
        else  {
            // Create a file
            File file = m_filesystem->open(path, "w");
            if (file) {
                file.write(0);
                file.close();
            }
            else {
                return this->send(500, "CREATE FAILED");
            }
        }
        this->send(200,  path.c_str());
    }
    else  {
        // Source specified: rename
        if (src == "/") {
            return this->send(400, "BAD SRC");
        }
        if (!m_filesystem->exists(src)) {
            return this->send(400,  "FILE NOT FOUND");
        }

        log_debug("handleFileCreate: %s from %s\n", path.c_str(), src.c_str());
        if (path.endsWith("/")) {
            path.remove(path.length() - 1);
        }
        if (src.endsWith("/")) {
            src.remove(src.length() - 1);
        }
        if (!m_filesystem->rename(src, path))  {
            return this->send(500, "RENAME FAILED");
        }
        this->sendOK();
    }
}

/*
    Handle a file deletion request
    Operation      | req.responseText
    ---------------+--------------------------------------------------------------
    Delete file    | parent of deleted file, or remaining ancestor
    Delete folder  | parent of deleted folder, or remaining ancestor
*/

void FSWebServer::deleteContent(String& path) {
  File file = m_filesystem->open(path.c_str(), "r");
  if (!file.isDirectory()) {
    file.close();
    m_filesystem->remove(path.c_str());
    log_info("File %s deleted", path.c_str());
    return;
  }

  file.rewindDirectory();
  while (true) {
    File entry = file.openNextFile();
    if (!entry) {
      break;
    }
    String entryPath = path;
    entryPath += "/";
    entryPath += entry.name();
    if (entry.isDirectory()) {
      entry.close();
      deleteContent(entryPath);
    }
    else {
      entry.close();
      m_filesystem->remove(entryPath.c_str());
      log_info("File %s deleted", path.c_str());
    }
    yield();
  }
  m_filesystem->rmdir(path.c_str());
  log_info("Folder %s removed", path.c_str());
  file.close();
}



void FSWebServer::handleFileDelete() {
    String path = this->arg((size_t)0);
    if (path.isEmpty() || path == "/")  {
        return this->send(400, "BAD PATH");
    }
    if (!m_filesystem->exists(path))  {
        return this->send(400, "File Not Found");
    }
    deleteContent(path);
    this->sendOK();
}

/*
    Return the FS type, status and size info
*/
void FSWebServer::handleFsStatus()
{
    log_debug("handleStatus");
    fsInfo_t info = {1024, 1024, "ESP Filesystem"};
#ifdef ESP8266
    FSInfo fs_info;
    m_filesystem->info(fs_info);
    info.totalBytes = fs_info.totalBytes;
    info.usedBytes = fs_info.usedBytes;
#endif
    if (getFsInfo != nullptr) {
        getFsInfo(&info);
    }
    CJSON::Json doc;
    doc.setString("type", info.fsName);
    doc.setString("isOk", m_filesystem_ok ? "true" : "false");

    if (m_filesystem_ok)  {
        IPAddress ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP() : WiFi.softAPIP();
        doc.setString("totalBytes", String(info.totalBytes));
        doc.setString("usedBytes", String(info.usedBytes));
        doc.setString("mode", WiFi.status() == WL_CONNECTED ? "Station" : "Access Point");
        doc.setString("ssid", WiFi.SSID());
        doc.setString("ip", ip.toString());
    }
    doc.setString("unsupportedFiles", "");
    this->send(200, "application/json", doc.serialize());
}
#endif // ESP_FS_WS_EDIT

