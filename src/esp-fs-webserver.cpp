#include "esp-fs-webserver.h"
#include "detail/RequestHandlersImpl.h"

// Override default handleClient() method to increase connection speed
#if defined(ESP32)
void FSWebServer::handleClient()
{
    if (_currentStatus == HC_NONE) {
        _currentClient = _server.available();
        if (!_currentClient) {
            if (_nullDelay) {
                delay(1);
            }
            return;
        }
        log_v("New client: client.localIP()=%s", _currentClient.localIP().toString().c_str());
        _currentStatus = HC_WAIT_READ;
        _statusChange = millis();
    }

    bool keepCurrentClient = false;

    if (_currentClient.connected()) {
        switch (_currentStatus) {
            // No-op to avoid C++ compiler warning
            case HC_NONE:
                break;

            // Wait for data from client to become available
            case HC_WAIT_READ:
                if (_currentClient.available()) {
                    if (_parseRequest(_currentClient)) {
                        _currentClient.setTimeout(HTTP_MAX_SEND_WAIT / 1000);
                        _contentLength = CONTENT_LENGTH_NOT_SET;
                        _handleRequest();
                    }
                }
                else {
                    if (millis() - _statusChange <= 100) {
                        keepCurrentClient = true;
                    }
                    yield();
                }
                break;

            // Wait for client to close the connection
            case HC_WAIT_CLOSE:
                if (millis() - _statusChange <= HTTP_MAX_CLOSE_WAIT) {
                    keepCurrentClient = true;
                    yield();
                }
                break;
        }
    }

    if (!keepCurrentClient) {
        _currentClient = WiFiClient();
        _currentStatus = HC_NONE;
        _currentUpload.reset();
    }
}
#endif


// Override default begin() method to set library built-in handlers
void FSWebServer::begin()
{
    File file = m_filesystem->open(ESP_FS_WS_CONFIG_FOLDER, "r");
    if (!file) {
        log_error("Failed to open /setup directory. Create new folder\n");
        m_filesystem->mkdir(ESP_FS_WS_CONFIG_FOLDER);
        ESP.restart();
    }
    m_fsOK = true;

    // Check if config file exist, and create if necessary
    if (!m_filesystem->exists(ESP_FS_WS_CONFIG_FILE)) {
        file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "w");
        file.print("{\"wifi-box\": \"\"}");
        file.close();
    }

    // Captive Portal redirect
    // Windows
    this->on("/connecttest.txt", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    this->on("/www.msftconnecttest.com/redirect", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    // Apple
    this->on("/hotspot-detect.html", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    // Android
    this->on("/generate_204", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    this->on("/gen_204", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));

#if ESP_FS_WS_SETUP
    this->on("/setup", HTTP_GET, std::bind(&FSWebServer::handleSetup, this));
    this->on("/scan", HTTP_GET, std::bind(&FSWebServer::handleScanNetworks, this));
    this->on("/getStatus", HTTP_GET, std::bind(&FSWebServer::getStatus, this));
#endif

    this->onNotFound(std::bind(&FSWebServer::handleRequest, this));
    this->on("/favicon.ico", HTTP_GET, std::bind(&FSWebServer::replyOK, this));
    this->on("/", HTTP_GET, std::bind(&FSWebServer::handleIndex, this));
    this->on("/connect", HTTP_POST, std::bind(&FSWebServer::doWifiConnection, this));
    this->on("/reset", HTTP_GET, std::bind(&FSWebServer::doRestart, this));
    this->on("/restart", HTTP_GET, std::bind(&FSWebServer::doRestart, this));

    // Upload file
    // - first callback is called after the request has ended with all parsed arguments
    // - second callback handles file upload at that location
    this->on("/edit", HTTP_POST,
        std::bind(&FSWebServer::replyOK, this),
        std::bind(&FSWebServer::handleFileUpload, this));

    // OTA update via webbrowser
    this->on("/update", HTTP_POST,
        std::bind(&FSWebServer::update_second, this),
        std::bind(&FSWebServer::update_first, this)
    );

#ifdef ESP32
    this->enableCrossOrigin(true);
    _server.setNoDelay(true);
#endif

    close();
    _server.begin();
}


void FSWebServer::run()
{
    this->handleClient();
    if (m_apmode)
        m_dnsServer->processNextRequest();
    yield();
}

void FSWebServer::enableFsCodeEditor(FsInfoCallbackF fsCallback) {
#if ESP_FS_WS_EDIT
    getFsInfo = fsCallback;
    this->on("/status", HTTP_GET, std::bind(&FSWebServer::handleStatus, this));
    this->on("/list", HTTP_GET, std::bind(&FSWebServer::handleFileList, this));
    this->on("/edit", HTTP_GET, std::bind(&FSWebServer::handleGetEdit, this));
    this->on("/edit", HTTP_PUT, std::bind(&FSWebServer::handleFileCreate, this));
    this->on("/edit", HTTP_DELETE, std::bind(&FSWebServer::handleFileDelete, this));
#endif
}

void FSWebServer::setAuthentication(const char* user, const char* pswd) {
    m_pageUser = (char*) malloc(strlen(user)*sizeof(char));
    m_pagePswd = (char*) malloc(strlen(pswd)*sizeof(char));
    strcpy(m_pageUser, user);
    strcpy(m_pagePswd, pswd);
}


void FSWebServer::setAPWebPage(const char* url)
{
    m_apWebpage = url;
}

void FSWebServer::setAP(const char* ssid, const char* psk)
{
    m_apSsid = ssid;
    m_apPsk = psk;
}

IPAddress FSWebServer::startAP()
{
    if (!m_apSsid.length()) {
        char ssid[21];
        #ifdef ESP8266
        snprintf(ssid, sizeof(ssid), "ESP-%dX", ESP.getChipId());
        #elif defined(ESP32)
        snprintf(ssid, sizeof(ssid), "ESP-%llX", ESP.getEfuseMac());
        #endif
        m_apSsid = ssid;
    }

    WiFi.mode(WIFI_AP);
    // Set AP IP 8.8.8.8 and subnet 255.255.255.0
    if (! WiFi.softAPConfig(0x08080808, 0x08080808, 0x00FFFFFF)) {
        log_error("Captive portal failed to start: WiFi.softAPConfig failed!");
        WiFi.enableAP(false);
        return IPAddress((uint32_t) 0);
    }

    WiFi.persistent(false);
	if (m_apPsk.length())
		WiFi.softAP(m_apSsid.c_str(), m_apPsk.c_str());
	else
		WiFi.softAP(m_apSsid.c_str());

    /* Setup the DNS server redirecting all the domains to the apIP */
    m_dnsServer = new DNSServer();
    m_dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    IPAddress ip = WiFi.softAPIP();
    if (! m_dnsServer->start(53, "*", WiFi.softAPIP())) {
        log_error("Captive portal failed to start: no sockets for DNS server available!");
        WiFi.enableAP(false);
        return IPAddress((uint32_t) 0);
    }

    ip = WiFi.softAPIP();
    log_info("AP mode.\nServer IP address: %s", ip.toString().c_str());
    m_apmode = true;
    return ip;
}

IPAddress FSWebServer::startWiFi(uint32_t timeout, bool apFlag, CallbackF fn)
{
    IPAddress local_ip, subnet, gateway;
    File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "r");
#if ARDUINOJSON_VERSION_MAJOR > 6
    JsonDocument doc;
#else
    int sz = file.size() * 1.33;
    int docSize = max(sz, 2048);
    DynamicJsonDocument doc((size_t)docSize);
#endif
    if (file) {
        // If file is present, load actual configuration
        DeserializationError error = deserializeJson(doc, file);
        if (error) {
            log_error("Failed to deserialize file, may be corrupted\n %s\n", error.c_str());
            file.close();
        }
        file.close();
        if (doc["dhcp"].as<bool>() == true) {
            gateway.fromString(doc["gateway"].as<String>());
            subnet.fromString(doc["subnet"].as<String>());
            local_ip.fromString(doc["ip_address"].as<String>());
            log_info("Manual config WiFi connection with IP: %s\n", local_ip.toString().c_str());
            if (!WiFi.config(local_ip, gateway, subnet))
                log_error("STA Failed to configure");
            delay(100);
        }
    }
    else {
        log_error("File not found, will be created new configuration file");
    }

    if (timeout > 0)
        m_timeout = timeout;

    m_apmode = false;
    WiFi.mode(WIFI_STA);
#if defined(ESP8266)
    struct station_config conf;
    wifi_station_get_config_default(&conf);
    const char* _ssid = reinterpret_cast<const char*>(conf.ssid);
    const char* _pass = reinterpret_cast<const char*>(conf.password);
#elif defined(ESP32)
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    const char* _ssid = reinterpret_cast<const char*>(conf.sta.ssid);
    const char* _pass = reinterpret_cast<const char*>(conf.sta.password);
#endif
    if (strlen(_ssid) && strlen(_pass)) {

        WiFi.begin(_ssid, _pass);
        uint32_t startTime = millis();
        log_info("Connecting to '%s'", _ssid);
        while ((WiFi.status() != WL_CONNECTED) && (timeout > 0)) {
            // execute callback function during wifi connection
            if (fn != nullptr)
                fn();
            delay(1000);
            Serial.print(".");
            // If no connection after a while break
            if (millis() - startTime > timeout)
                break;
        }
        if ((WiFi.status() == WL_CONNECTED) || apFlag)
            return WiFi.localIP();
    }
    // start Access Point
    return startAP();
}

////////////////////////////////  WiFi  /////////////////////////////////////////

/**
 * Redirect to captive portal if we got a request for another domain.
 */
bool FSWebServer::captivePortal()
{
    String serverLoc("http://");
    serverLoc += this->client().localIP().toString();
    serverLoc += m_apWebpage;
    // redirect if hostheader not server ip, prevent redirect loops
    if (serverLoc != this->hostHeader()) {
        this->sendHeader("Location", serverLoc, true);
        this->send(301, "text/html", "");
        return true;
    }
    return false;
}

void FSWebServer::handleRequest()
{
    if (!m_fsOK) {
        replyToCLient(ERROR, PSTR(FS_INIT_ERROR));
        return;
    }
    String _url = WebServerClass::urlDecode(this->uri());
    // First try to find and return the requested file from the filesystem,
    // and if it fails, return a 404 page with debug information
    if (handleFileRead(_url.c_str()))
        return;
    else
        replyToCLient(NOT_FOUND, PSTR(FILE_NOT_FOUND));
}

void FSWebServer::getConfigFile()
{
    this->send(200, "text/json", ESP_FS_WS_CONFIG_FILE);
}

void FSWebServer::doRestart()
{
    this->send(200, "text/json", "Going to restart ESP");
    delay(500);
    ESP.restart();
}

void FSWebServer::doWifiConnection()
{
    String ssid, pass;
    bool persistent = true;
    WiFi.mode(WIFI_AP_STA);

    if (this->hasArg("ssid")) {
        ssid = this->arg("ssid");
    }

    if (this->hasArg("password")) {
        pass = this->arg("password");
    }

    if (this->hasArg("persistent")) {
        String pers = this->arg("persistent");
        if (pers.equals("false")) {
            persistent = false;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        // IPAddress ip = WiFi.localIP();
        String resp = "ESP is currently connected to a WiFi network.<br><br>"
                      "Actual connection will be closed and a new attempt will be done with <b>";
        resp += ssid;
        resp += "</b> WiFi network.";
        this->send(200, "text/plain", resp);

        delay(500);
        log_info("Disconnect from current WiFi network");
        WiFi.disconnect();
    }

    if (ssid.length()) { // pass could be empty
        // Try to connect to new ssid
        log_info("Connecting to %s", ssid.c_str());
        WiFi.begin(ssid.c_str(), pass.c_str());

        uint32_t beginTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(250);
            Serial.print("*");
            if (millis() - beginTime > m_timeout)
                break;
        }
        Serial.println();
        // reply to client
        if (WiFi.status() == WL_CONNECTED) {
            // WiFi.softAPdisconnect();
            IPAddress ip = WiFi.localIP();
            log_info("Connected to Wifi! IP address: %s", ip.toString().c_str());
            String serverLoc = F("http://");
            for (int i = 0; i < 4; i++)
                serverLoc += i ? "." + String(ip[i]) : String(ip[i]);

            String resp = "Restart ESP and then reload this page from <a href='";
            resp += serverLoc;
            resp += "/setup'>the new LAN address</a>";

            this->send(200, "text/plain", resp);
            m_apmode = false;

            // Store current WiFi configuration in flash
            if (persistent) {
#if defined(ESP8266)
                struct station_config stationConf;
                wifi_station_get_config_default(&stationConf);
                // Clear previuos configuration
                memset(&stationConf, 0, sizeof(stationConf));
                os_memcpy(&stationConf.ssid, ssid.c_str(), ssid.length());
                os_memcpy(&stationConf.password, pass.c_str(), pass.length());
                wifi_set_opmode(STATION_MODE);
                wifi_station_set_config(&stationConf);
#elif defined(ESP32)
                wifi_config_t stationConf;
                esp_wifi_get_config(WIFI_IF_STA, &stationConf);
                // Clear previuos configuration
                memset(&stationConf, 0, sizeof(stationConf));
                memcpy(&stationConf.sta.ssid, ssid.c_str(), ssid.length());
                memcpy(&stationConf.sta.password, pass.c_str(), pass.length());
                esp_wifi_set_config(WIFI_IF_STA, &stationConf);
#endif
            } else {
                this->clearWifiCredentials();
            }
        } else
            this->send(500, "text/plain", "Connection error, maybe the password is wrong?");
    }
    this->send(500, "text/plain", "Wrong credentials provided");
}

void FSWebServer::clearWifiCredentials()
{
#if defined(ESP8266)
    struct station_config stationConf;
    wifi_station_get_config_default(&stationConf);
    // Clear previuos configuration
    memset(&stationConf, 0, sizeof(stationConf));
    wifi_station_set_config(&stationConf);
#elif defined(ESP32)
    wifi_config_t stationConf;
    esp_wifi_get_config(WIFI_IF_STA, &stationConf);
    // Clear previuos configuration
    memset(&stationConf, 0, sizeof(stationConf));
    esp_wifi_set_config(WIFI_IF_STA, &stationConf);
#endif
}

void FSWebServer::setCrossOrigin()
{
    this->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    this->sendHeader(F("Access-Control-Max-Age"), F("600"));
    this->sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
    this->sendHeader(F("Access-Control-Allow-Headers"), F("*"));
};


#if ESP_FS_WS_SETUP
void FSWebServer::handleScanNetworks() {
    log_info("Scanning WiFi networks...");
    int res = WiFi.scanNetworks(false, false, false);
    log_info(" done!\nNumber of networks: %d", res);

    JSON_DOC(res*96);
    JsonArray array = doc.to<JsonArray>();

    if (res > 0) {
        for (int i = 0; i < res; ++i) {
            #if ARDUINOJSON_VERSION_MAJOR > 6
                JsonObject obj = doc.add<JsonObject>();
            #else
                JsonObject obj = array.createNestedObject();
            #endif
            obj["strength"] = WiFi.RSSI(i);
            obj["ssid"] = WiFi.SSID(i);
            #if defined(ESP8266)
            obj["security"] = AUTH_OPEN ? "none" : "enabled";
            #elif defined(ESP32)
            obj["security"] = WIFI_AUTH_OPEN ? "none" : "enabled";
            #endif
        }
        WiFi.scanDelete();
    }
    String json;
    serializeJson(doc, json);
    this->send(200, "application/json", json);
    log_info("%s", json.c_str());
}


const char* FSWebServer::getConfigFilepath() {
    return ESP_FS_WS_CONFIG_FILE;
}

void FSWebServer::getStatus()
{
    JSON_DOC(256);
    doc["firmware"] = m_version;
    doc["mode"] =  WiFi.status() == WL_CONNECTED ? ("Station (" + WiFi.SSID()) +')' : "Access Point";
    doc["ip"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    doc["path"] = ESP_FS_WS_CONFIG_FILE;
    doc["liburl"] = LIB_URL;
    String reply;
    serializeJson(doc, reply);
    this->send(200, "application/json", reply);
}

bool FSWebServer::clearOptions()
{
    File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "r");
    if (file) {
        file.close();
        m_filesystem->remove(ESP_FS_WS_CONFIG_FILE);
        return true;
    }
    return false;
}

void FSWebServer::removeWhiteSpaces(String& str)
{
    const char noChars[] = { '\n', '\r', '\t' };
    int pos = -1;
    // Remove non printable characters
    for (unsigned int i = 0; i < sizeof(noChars); i++) {
        pos = str.indexOf(noChars[i]);
        while (pos > -1) {
            str.replace(String(noChars[i]), "");
            pos = str.indexOf(noChars[i]);
        }
    }
    // Remove doubles spaces
    pos = str.indexOf("  ");
    while (pos > -1) {
        str.replace("  ", " ");
        pos = str.indexOf("  ");
    }
}

void FSWebServer::handleSetup()
{

    log_debug("[%d] - handleSetup start", millis());
#if ESP_FS_WS_SETUP_HTM
    if (m_pageUser != nullptr) {
        if(!this->authenticate(m_pageUser, m_pagePswd))
            return this->requestAuthentication();
    }
    this->sendHeader(PSTR("Content-Encoding"), "gzip");
    // Changed array name to match SEGGER Bin2C output
    this->send_P(200, "text/html", (const char*)_acsetup_min_htm, sizeof(_acsetup_min_htm));
#else
    replyToCLient(NOT_FOUND, PSTR("FILE_NOT_FOUND"));
#endif
    log_debug("[%d] - handleSetup end", millis());
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
        txt = "Update completed successfully. The ESP32 will restart";
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
    static uint8_t otaDone = 0;
    size_t fsize;
    if (this->hasArg("size"))
        fsize = this->arg("size").toInt();
    else {
        this->send(500, "text/plain", "Request malformed: missing file size");
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

#endif // ESP_FS_WS_SETUP

void FSWebServer::handleIndex()
{
    log_debug("handleIndex");
    if (m_filesystem->exists("/index.htm")) {
        handleFileRead("/index.htm");
    } else if (m_filesystem->exists("/index.html")) {
        handleFileRead("/index.html");
    } else {
        handleSetup();
    }
}

/*
    Read the given file from the filesystem and stream it back to the client
*/
bool FSWebServer::handleFileRead(const char* uri)
{
    log_debug("handleFileRead: %", uri);

    // Check if requested file (or gzipped version) exists
    String path = uri;
    if (!m_filesystem->exists(path)) {
        path += ".gz";
        if (!m_filesystem->exists(path))
            return false;
    }

    const char* contentType = getContentType(path.c_str());
    File file = m_filesystem->open(path, "r");
    if (this->streamFile(file, contentType) != file.size()) {
        log_debug("Sent less data than expected!");
    }
    file.close();
    return true;
}

bool FSWebServer::createDirFromPath(const String& path)
{
    String dir;
    int p1 = 0;
    int p2 = 0;
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
    Handle a file upload request
*/
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
            replyToCLient(ERROR, PSTR("INVALID FILENAME"));
            return;
        }

        log_debug("handleFileUpload Name: %s\n", filename.c_str());
        m_uploadFile = m_filesystem->open(filename, "w");
        if (!m_uploadFile) {
            replyToCLient(ERROR, PSTR("CREATE FAILED"));
            return;
        }
        log_debug("Upload: START, filename: %s\n", filename.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (m_uploadFile) {
            size_t bytesWritten = m_uploadFile.write(upload.buf, upload.currentSize);
            if (bytesWritten != upload.currentSize) {
                replyToCLient(ERROR, PSTR("WRITE FAILED"));
                return;
            }
        }
        log_debug("Upload: WRITE, Bytes: %d\n", upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (m_uploadFile) {
            m_uploadFile.close();
        }
        log_debug("Upload: END, Size: %d\n", upload.totalSize);
    }
}

void FSWebServer::replyOK()
{
    replyToCLient(MSG_OK, "");
}

void FSWebServer::replyToCLient(int msg_type = 0, const char* msg = "")
{
    this->sendHeader("Access-Control-Allow-Origin", "*");
    switch (msg_type) {
    case MSG_OK:
        this->send(200, FPSTR(TEXT_PLAIN), "");
        break;
    case CUSTOM:
        this->send(200, FPSTR(TEXT_PLAIN), msg);
        break;
    case NOT_FOUND:
        this->send(404, FPSTR(TEXT_PLAIN), msg);
        break;
    case BAD_REQUEST:
        this->send(400, FPSTR(TEXT_PLAIN), msg);
        break;
    case ERROR:
        this->send(500, FPSTR(TEXT_PLAIN), msg);
        break;
    }
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

const char* FSWebServer::getContentType(const char* filename)
{
    if (this->hasArg("download"))
        return PSTR("application/octet-stream");
    else if (strstr(filename, ".htm"))
        return PSTR("text/html");
    else if (strstr(filename, ".html"))
        return PSTR("text/html");
    else if (strstr(filename, ".css"))
        return PSTR("text/css");
    else if (strstr(filename, ".sass"))
        return PSTR("text/css");
    else if (strstr(filename, ".js"))
        return PSTR("application/javascript");
    else if (strstr(filename, ".png"))
        return PSTR("image/png");
    else if (strstr(filename, ".svg"))
        return PSTR("image/svg+xml");
    else if (strstr(filename, ".gif"))
        return PSTR("image/gif");
    else if (strstr(filename, ".jpg"))
        return PSTR("image/jpeg");
    else if (strstr(filename, ".ico"))
        return PSTR("image/x-icon");
    else if (strstr(filename, ".xml"))
        return PSTR("text/xml");
    else if (strstr(filename, ".pdf"))
        return PSTR("application/x-pdf");
    else if (strstr(filename, ".zip"))
        return PSTR("application/x-zip");
    else if (strstr(filename, ".gz"))
        return PSTR("application/x-gzip");
    return PSTR("text/plain");
}

// edit page, in usefull in some situation, but if you need to provide only a web interface, you can disable
#if ESP_FS_WS_EDIT

/*
    Return the list of files in the directory specified by the "dir" query string parameter.
    Also demonstrates the use of chuncked responses.
*/
void FSWebServer::handleFileList()
{
    if (!this->hasArg("dir")) {
        return replyToCLient(BAD_REQUEST, "DIR ARG MISSING");
    }

    String path = this->arg("dir");
    log_debug("handleFileList: %s", path.c_str());
    if (path != "/" && !m_filesystem->exists(path)) {
        return replyToCLient(BAD_REQUEST, "BAD PATH");
    }

    File root = m_filesystem->open(path, "r");
    path = String();
    String output;
    output.reserve(256);
    output = "[";
    if (root.isDirectory()) {
        File file = root.openNextFile();
        while (file) {
            String filename = file.name();
            if (filename.lastIndexOf("/") > -1) {
                filename.remove(0, filename.lastIndexOf("/") + 1);
            }
            if (output != "[") {
                output += ',';
            }
            output += "{\"type\":\"";
            output += (file.isDirectory()) ? "dir" : "file";
            output += "\",\"size\":\"";
            output += file.size();
            output += "\",\"name\":\"";
            output += filename;
            output += "\"}";
            file = root.openNextFile();
        }
    }
    output += "]";
    this->send(200, "text/json", output);
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
        replyToCLient(BAD_REQUEST, PSTR("PATH ARG MISSING"));
        return;
    }
    if (path == "/") {
        replyToCLient(BAD_REQUEST, PSTR("BAD PATH"));
        return;
    }

    String src = this->arg("src");
    if (src.isEmpty()) {
        // No source specified: creation
        log_debug("handleFileCreate: %s\n", path.c_str());
        if (path.endsWith("/")) {
            // Create a folder
            path.remove(path.length() - 1);
            if (!m_filesystem->mkdir(path)) {
                replyToCLient(ERROR, PSTR("MKDIR FAILED"));
                return;
            }
        } else {
            // Create a file
            File file = m_filesystem->open(path, "w");
            if (file) {
                file.write(0);
                file.close();
            } else {
                replyToCLient(ERROR, PSTR("CREATE FAILED"));
                return;
            }
        }
        replyToCLient(CUSTOM, path.c_str());
    } else {
        // Source specified: rename
        if (src == "/") {
            replyToCLient(BAD_REQUEST, PSTR("BAD SRC"));
            return;
        }
        if (!m_filesystem->exists(src)) {
            replyToCLient(BAD_REQUEST, PSTR("BSRC FILE NOT FOUND"));
            return;
        }

        log_debug("handleFileCreate: %s from %s\n", path.c_str(), src.c_str());
        if (path.endsWith("/")) {
            path.remove(path.length() - 1);
        }
        if (src.endsWith("/")) {
            src.remove(src.length() - 1);
        }
        if (!m_filesystem->rename(src, path)) {
            replyToCLient(ERROR, PSTR("RENAME FAILED"));
            return;
        }
        replyOK();
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
    log_debug("File %s deleted", path.c_str());
    return;
  }

  file.rewindDirectory();
  while (true) {
    File entry = file.openNextFile();
    if (!entry) {
      break;
    }
    String entryPath = path + "/" + entry.name();
    if (entry.isDirectory()) {
      entry.close();
      deleteContent(entryPath);
    }
    else {
      entry.close();
      m_filesystem->remove(entryPath.c_str());
      log_debug("File %s deleted", path.c_str());
    }
    yield();
  }
  m_filesystem->rmdir(path.c_str());
  log_debug("Folder %s removed", path.c_str());
  file.close();
}



void FSWebServer::handleFileDelete() {
    String path = this->arg(0);
    if (path.isEmpty() || path == "/")  {
        replyToCLient(BAD_REQUEST, PSTR("BAD PATH"));
    }
    if (!m_filesystem->exists(path))  {
        replyToCLient(NOT_FOUND, PSTR(FILE_NOT_FOUND));
    }
    deleteContent(path);
    replyOK();
}


/*
    This specific handler returns the edit.htm (or a gzipped version) from the /edit folder.
    If the file is not present but the flag INCLUDE_FALLBACK_INDEX_HTM has been set, falls back to the version
    embedded in the program code.
    Otherwise, fails with a 404 page with debug information
*/
void FSWebServer::handleGetEdit()
{
#if ESP_FS_WS_EDIT_HTM
    if (m_pageUser != nullptr) {
        if(!this->authenticate(m_pageUser, m_pagePswd))
            return this->requestAuthentication();
    }
    this->sendHeader(PSTR("Content-Encoding"), "gzip");
    this->send_P(200, "text/html", edit_htm_gz, EDIT_HTML_SIZE);
#else
    replyToCLient(NOT_FOUND, PSTR("FILE_NOT_FOUND"));
#endif
}

/*
    Return the FS type, status and size info
*/
void FSWebServer::handleStatus()
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

    JSON_DOC(256);
    doc["isOk"] =  m_fsOK;
    doc["type"] = info.fsName;
    doc["totalBytes"] =  info.totalBytes;
    doc["usedBytes"] =  info.usedBytes;
    doc["mode"] =  WiFi.status() == WL_CONNECTED ? ("Station: " + WiFi.SSID()) : "Access Point";
    doc["ssid"] =  WiFi.SSID();
    doc["ip"] =  (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    doc["unsupportedFiles"] =  "";
    String json;
    serializeJson(doc, json);
    this->send(200, "application/json", json);
}

#endif // ESP_FS_WS_EDIT


void FSWebServer::printFileList(fs::FS& fs, Print& p, const char* dirName, uint8_t level)
{
    p.printf("\nListing directory: %s\n", dirName);
    File root = fs.open(dirName, "r");
    if(!root){
        p.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        p.println(" - not a directory");
        return;
    }
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            if(level){
            #ifdef ESP32
			  printFileList(fs, p, file.path(), level - 1);
			#elif defined(ESP8266)
			  printFileList(fs, p, file.fullName(), level - 1);
			#endif
            }
        } else {
            p.printf("|__ %s \t(%d bytes)\n",file.name(), file.size());
        }
        file = root.openNextFile();
    }
}