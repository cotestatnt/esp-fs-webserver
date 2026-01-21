#include "FSWebServer.h"

#if defined(ESP32)
    #include "mimetable/mimetable.h"
#endif


void FSWebServer::begin(WebSocketsServer::WebSocketServerEvent wsEventHandler) {

    // Set build date as default firmware version (YYMMDDHHmm) from Version.h constexprs
    if (m_version.length() == 0)
        m_version = String(BUILD_TIMESTAMP);

//////////////////////    BUILT-IN HANDLERS    ///////////////////////////
#if ESP_FS_WS_SETUP
    m_filesystem_ok = getSetupConfigurator()->checkConfigFile();
    if (getSetupConfigurator()->isOpened()) {
        log_debug("Config file %s closed", ESP_FS_WS_CONFIG_FILE);
        getSetupConfigurator()->closeConfiguration();
    }
    // Setup page handlers
    on("*", HTTP_HEAD, [this]() { this->handleFileName(); });
    on("/", HTTP_GET, [this]() { this->handleIndex(); });
    on("/setup", HTTP_GET, [this]() { this->handleSetup(); });
    on("/connect", HTTP_POST, [this]() { this->doWifiConnection(); });
    on("/scan", HTTP_GET, [this]() { this->handleScanNetworks(); });
    on("/getStatus", HTTP_GET, [this]() { this->getStatus(); });
    on("/clear_config", HTTP_GET, [this]() { this->clearConfig(); });
    // WiFi info handler
    on("/wifi", HTTP_GET, [this]() {
        CJSON::Json doc;
        doc.setString("ssid", WiFi.SSID());
        doc.setNumber("rssi", WiFi.RSSI());
        this->send(200, "application/json", doc.serialize());
    });
    // WiFi credentials management (no plaintext passwords exposed)
    on("/wifi/credentials", HTTP_GET, [this]() {
        CJSON::Json json_array;
        json_array.createArray();
        if (m_credentialManager) {
            std::vector<WiFiCredential>* creds = m_credentialManager->getCredentials();
            if (creds) {
                for (size_t i = 0; i < creds->size(); ++i) {
                    const WiFiCredential &c = (*creds)[i];
                    CJSON::Json item;
                    item.setNumber("index", static_cast<int>(i));
                    item.setString("ssid", c.ssid);
                    if (c.local_ip != IPAddress(0, 0, 0, 0)) {
                        item.setString("ip", c.local_ip.toString());
                        item.setString("gateway", c.gateway.toString());
                        item.setString("subnet", c.subnet.toString());
                    }
                    item.setNumber("hasPassword", c.password_len > 0 ? 1 : 0);
                    json_array.add(item);
                }
            }
        }
        this->send(200, "application/json", json_array.serialize());
    });
    // Delete a single credential (by index) or clear all if no index is provided
    on("/wifi/credentials", HTTP_DELETE, [this]() {
        if (!m_credentialManager) {
            this->send(404, "text/plain", "Credential manager not available");
            return;
        }

        bool ok = false;
        if (this->hasArg("index")) {
            int idx = this->arg("index").toInt();
            ok = m_credentialManager->removeCredential(static_cast<uint8_t>(idx));
        } else {
            m_credentialManager->clearAll();
            ok = true;
        }

#if defined(ESP32)
        m_credentialManager->saveToNVS();
#elif defined(ESP8266)
        m_credentialManager->saveToFS();
#endif

        this->send(ok ? 200 : 400, "text/plain", ok ? "OK" : "Invalid index");
    });
    // File upload handler for /setup page
    on("/upload", HTTP_POST,
        [this]() { this->sendOK();},
        [this]() { this->handleFileUpload(); }  
    );
    // OTA update via webbrowser
    this->on("/update", HTTP_POST,
        [this]() { this->update_second(); },
        [this]() { this->update_first(); }  
    );
    // Restart handler
    on("/reset", HTTP_GET, [this]() {
        // Send response first, then restart shortly after
        this->sendHeader(PSTR("Connection"), "close");
        this->send(200, "text/plain", WiFi.localIP().toString());
        delay(500);
#if defined(ESP8266)
        ESP.reset();
#else
        ESP.restart();
#endif
    });
#endif

    // Handler for all other files
    onNotFound([this]() { this->handleFileRequest(); });    
    
    // Start MDNS responder if hostname is set
    if (m_host.length()) {
        if (m_dnsServer)
            this->startMDNSResponder();

        this->setHostname(m_host.c_str());        
    }

    // WebSocket setup
#if ESP_FS_WS_WEBSOCKET
    if (wsEventHandler != nullptr) {
        if (!m_websocket) {
            m_websocket = new WebSocketsServer(m_port+1);
            m_websocket->begin();
            m_websocket->onEvent(wsEventHandler);
        }
    }
#endif

#ifdef ESP32
    this->enableCrossOrigin(true);    
#endif
    WebServerClass::begin();
}

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
    this->sendHeader(PSTR("Content-Encoding"), "gzip");
    // Changed array name to match SEGGER Bin2C output
    this->send_P(200, "text/html", (const char*)_acsetup_min_htm, sizeof(_acsetup_min_htm));
}
#endif

#if ESP_FS_WS_SETUP
void FSWebServer::getStatus() {
    CJSON::Json doc;
    doc.setString("firmware", m_version);
    String mode;
    if (WiFi.status() == WL_CONNECTED) {
        mode = "Station (";
        mode += WiFi.SSID();
        mode += ")";
    } else {
        mode = "Access Point";
    }
    doc.setString("mode", mode);
    String ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    doc.setString("ip", ip);
    doc.setString("hostname", m_host);
    doc.setString("path", String(ESP_FS_WS_CONFIG_FILE).substring(1));   // remove first '/'
    doc.setString("liburl", LIB_URL);
    this->send(200, "application/json", doc.serialize());
}

void FSWebServer::clearConfig() {
    if (m_filesystem->remove(ESP_FS_WS_CONFIG_FILE))
        this->send(200, "text/plain", "Clear config OK");
    else
        this->send(200, "text/plain", "Clear config not done");
}

void FSWebServer::handleScanNetworks() {
    log_info("Start scan WiFi networks");
    WiFiScanResult scan = WiFiService::scanNetworks();
    this->send(200, "application/json", scan.json);
}

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
        if (m_uploadFile) { 
            m_uploadFile.close();
        }
        log_debug("Upload: END, Size: %d\n", upload.totalSize);
    }
}

void FSWebServer::doWifiConnection() {
    String ssid, pass;
    IPAddress gateway, subnet, local_ip;
    bool noDHCP = false;
    bool newSSID = false;

    if (this->hasArg("ip_address") && this->hasArg("subnet") && this->hasArg("gateway")) {
        gateway.fromString(this->arg("gateway"));
        subnet.fromString(this->arg("subnet"));
        local_ip.fromString(this->arg("ip_address"));
        noDHCP = true;
    }

    if (this->hasArg("ssid"))
        ssid = this->arg("ssid");

    if (this->hasArg("password"))
        pass = this->arg("password");

    if (this->hasArg("newSSID")) {
        newSSID = true;
    }

    // If no password provided but a stored credential exists for this SSID,
    // reuse the stored password without exposing it to the client.
    if (pass.length() == 0 && ssid.length() && m_credentialManager &&
        m_credentialManager->checkSSIDExists(ssid.c_str())) {
        String stored = m_credentialManager->getPassword(ssid.c_str());
        if (stored.length()) {
            pass = stored;
        }
    }

    bool hasPersistentArg = this->hasArg("persistent");
    bool persistent = hasPersistentArg ? this->arg("persistent").equals("true") : true;

    if (hasPersistentArg) {
        WiFiService::applyPersistentConfig(persistent, ssid, pass);
    }

    if (persistent && ssid.length() && pass.length() && m_credentialManager) {
        WiFiCredential cred{};
        strncpy(cred.ssid, ssid.c_str(), sizeof(cred.ssid) - 1);
        cred.ssid[sizeof(cred.ssid) - 1] = '\0';

        if (noDHCP) {
            cred.local_ip = local_ip;
            cred.gateway = gateway;
            cred.subnet = subnet;
        } else {
            cred.local_ip = IPAddress(0, 0, 0, 0);
            cred.gateway = IPAddress(0, 0, 0, 0);
            cred.subnet = IPAddress(0, 0, 0, 0);
        }

        if (!m_credentialManager->updateCredential(cred, pass.c_str())) {
            m_credentialManager->addCredential(cred, pass.c_str());
        }
    #if defined(ESP32)
        m_credentialManager->saveToNVS();
    #elif defined(ESP8266)
        m_credentialManager->saveToFS();
    #endif
    }

    WiFiConnectParams params;
    params.ssid = ssid;
    params.password = pass;
    params.changeSSID = newSSID;
    params.noDHCP = noDHCP;
    params.local_ip = local_ip;
    params.gateway = gateway;
    params.subnet = subnet;
    params.host = m_host;
    params.timeout = m_timeout;
    params.wdtLongTimeout = AWS_LONG_WDT_TIMEOUT;
    params.wdtTimeout = AWS_WDT_TIMEOUT;

    WiFiConnectResult result = WiFiService::connectWithParams(params);
    if (result.connected) {
        m_serverIp = result.ip;
        delay(500);  // Give client time to receive response before system changes
    }

    const char* contentType = (result.status == 200) ? "text/html" : "text/plain";
    this->send(result.status, contentType, result.body);
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
        MDNS.end();
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
    params.ssid = ssid;
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
    this->send_P(200, "text/html", (const char*)_acedit_min_htm, sizeof(_acedit_min_htm));
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

