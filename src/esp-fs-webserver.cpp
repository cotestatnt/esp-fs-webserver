#include "esp-fs-webserver.h"

void FSWebServer::run()
{
    yield();
    webserver->handleClient();
    if (m_apmode)
        m_dnsServer.processNextRequest();
}

void FSWebServer::addHandler(const Uri& uri, HTTPMethod method, WebServerClass::THandlerFunction fn)
{
    webserver->on(uri, method, fn);
}

void FSWebServer::addHandler(const Uri& uri, WebServerClass::THandlerFunction handler)
{
    webserver->on(uri, HTTP_ANY, handler);
}

bool FSWebServer::begin()
{
    if (!m_filesystem->exists(ESP_FS_WS_CONFIG_FOLDER)) {
        log_error("Failed to open " ESP_FS_WS_CONFIG_FOLDER " directory. Create new folder");
        m_filesystem->mkdir(ESP_FS_WS_CONFIG_FOLDER);
        // Create empty config file
        File config = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "w");
        config.println("{\"wifi-box\":\"\"}");
        config.close();
        ESP.restart();
    }
    m_fsOK = true;

    // Backward compatibility, move config.json to new path
    File file = m_filesystem->open("/config.json", "r");
    if (file) {
        file.close();
        m_filesystem->rename("/config.json", ESP_FS_WS_CONFIG_FILE);
    }

#if ESP_FS_WS_EDIT
    webserver->on("/status", HTTP_GET, std::bind(&FSWebServer::handleStatus, this));
    webserver->on("/list", HTTP_GET, std::bind(&FSWebServer::handleFileList, this));
    webserver->on("/edit", HTTP_GET, std::bind(&FSWebServer::handleGetEdit, this));
    webserver->on("/edit", HTTP_PUT, std::bind(&FSWebServer::handleFileCreate, this));
    webserver->on("/edit", HTTP_DELETE, std::bind(&FSWebServer::handleFileDelete, this));
#endif
    webserver->onNotFound(std::bind(&FSWebServer::handleRequest, this));
    webserver->on("/favicon.ico", HTTP_GET, std::bind(&FSWebServer::replyOK, this));
    webserver->on("/", HTTP_GET, std::bind(&FSWebServer::handleIndex, this));
#if ESP_FS_WS_SETUP
    webserver->on("/setup", HTTP_GET, std::bind(&FSWebServer::handleSetup, this));
    webserver->on("/get_config", HTTP_GET, std::bind(&FSWebServer::getConfigFile, this));
    webserver->on("/wifistatus", HTTP_GET, std::bind(&FSWebServer::getStatus, this));
#endif
    webserver->on("/scan", HTTP_GET, std::bind(&FSWebServer::handleScanNetworks, this));
    webserver->on("/connect", HTTP_POST, std::bind(&FSWebServer::doWifiConnection, this));
    webserver->on("/restart", HTTP_GET, std::bind(&FSWebServer::doRestart, this));
    webserver->on("/reset", HTTP_GET, std::bind(&FSWebServer::doRestart, this));
    webserver->on("/ipaddress", HTTP_GET, std::bind(&FSWebServer::getIpAddress, this));

    // Captive Portal redirect
    webserver->on("/redirect", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    // Windows
    webserver->on("/connecttest.txt", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    webserver->on("/www.msftconnecttest.com/redirect", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    // Apple
    webserver->on("/hotspot-detect.html", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    // Android
    webserver->on("/generate_204", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    webserver->on("/gen_204", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));

    // Upload file
    // - first callback is called after the request has ended with all parsed arguments
    // - second callback handles file upload at that location
    webserver->on("/edit", HTTP_POST,
        std::bind(&FSWebServer::replyOK, this),
        std::bind(&FSWebServer::handleFileUpload, this));

    // OTA update via webbrowser
    webserver->on("/update", HTTP_POST,
        std::bind(&FSWebServer::update_second, this),
        std::bind(&FSWebServer::update_first, this));

#ifdef ESP32
    webserver->enableCrossOrigin(true);
    webserver->enableDelay(false);
#endif
    // webserver->setContentLength(50);
    webserver->begin();
    return true;
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
        snprintf(ssid, sizeof(ssid), "ESP-%llX", ESP.getEfuseMac());
        m_apSsid = ssid;
    }

    m_apmode = true;
    WiFi.mode(WIFI_AP);
    WiFi.persistent(false);
	if (m_apPsk.length())
		WiFi.softAP(m_apSsid.c_str(), m_apPsk.c_str());	
	else
		WiFi.softAP(m_apSsid.c_str());
    /* Setup the DNS server redirecting all the domains to the apIP */
    m_dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    IPAddress ip = WiFi.softAPIP();
    m_dnsServer.start(53, "*", ip);

    ip = WiFi.softAPIP();
    log_info("AP mode.\n Server IP address: %s", ip.toString().c_str());
    return ip;
}

IPAddress FSWebServer::startWiFi(uint32_t timeout, bool apFlag, CallbackF fn)
{
    IPAddress local_ip, subnet, gateway;
    File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "r");
    int sz = file.size() * 1.33;
    int docSize = max(sz, 2048);
    DynamicJsonDocument doc((size_t)docSize);
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
    serverLoc += webserver->client().localIP().toString();
    serverLoc += m_apWebpage;
    // redirect if hostheader not server ip, prevent redirect loops
    if (serverLoc != webserver->hostHeader()) {
        webserver->sendHeader(F("Location"), serverLoc, true);
        webserver->send(302, F("text/html"), ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
        webserver->client().stop(); // Stop is needed because we sent no content length
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
    String _url = WebServerClass::urlDecode(webserver->uri());
    // First try to find and return the requested file from the filesystem,
    // and if it fails, return a 404 page with debug information
    if (handleFileRead(_url.c_str()))
        return;
    else
        replyToCLient(NOT_FOUND, PSTR(FILE_NOT_FOUND));
}

void FSWebServer::getIpAddress()
{
    webserver->send(200, "text/json", WiFi.localIP().toString());
}

void FSWebServer::getConfigFile()
{
    webserver->send(200, "text/json", ESP_FS_WS_CONFIG_FILE);
}

void FSWebServer::doRestart()
{
    webserver->send(200, "text/json", "Going to restart ESP");
    delay(500);
    ESP.restart();
}

void FSWebServer::doWifiConnection()
{
    IPAddress local_ip, subnet, gateway;
    String ssid, pass;
    bool persistent = true;
    bool config = false;
    WiFi.mode(WIFI_AP_STA);

    if (webserver->hasArg("ip_address") && webserver->hasArg("subnet") && webserver->hasArg("gateway")) {
        gateway.fromString(webserver->arg("gateway"));
        subnet.fromString(webserver->arg("subnet"));
        local_ip.fromString(webserver->arg("ip_address"));
        config = true;
    }

    if (webserver->hasArg("ssid")) {
        ssid = webserver->arg("ssid");
    }

    if (webserver->hasArg("password")) {
        pass = webserver->arg("password");
    }

    if (webserver->hasArg("persistent")) {
        String pers = webserver->arg("persistent");
        if (pers.equals("false")) {
            persistent = false;
            clearWifiCredentials();
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        // IPAddress ip = WiFi.localIP();
        String resp = "ESP is currently connected to a WiFi network.<br><br>"
                      "Actual connection will be closed and a new attempt will be done with <b>";
        resp += ssid;
        resp += "</b> WiFi network.";
        webserver->send(200, "text/plain", resp);

        delay(500);
        log_info("Disconnect from current WiFi network");
        WiFi.disconnect();
    }

    if (ssid.length()) { // pass could be empty

        // Try to connect to new ssid
        log_info("Connecting to \"%s\"", ssid.c_str());
        WiFi.begin(ssid.c_str(), pass.c_str());

        // Manual connection setup
        if (config) {
            log_info("Manual config WiFi connection with IP: %s", local_ip.toString().c_str());
            if (!WiFi.config(local_ip, gateway, subnet))
                log_error("STA Failed to configure");
        }

        uint32_t beginTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.print(".");
            if (millis() - beginTime > m_timeout)
                break;
        }

        // reply to client
        if (WiFi.status() == WL_CONNECTED) {
            IPAddress ip = WiFi.localIP();
            log_info("Connected to Wifi! IP address: %s", ip.toString().c_str());
            String serverLoc = F("http://");
            for (int i = 0; i < 4; i++)
                serverLoc += i ? "." + String(ip[i]) : String(ip[i]);
            String resp = "Restart ESP and then reload this page from <a href='";
            resp += serverLoc;
            resp += "/setup'>the new LAN address</a>";
            webserver->send(200, "text/plain", resp);
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
            webserver->send(500, "text/plain", "Connection error, maybe the password is wrong?");
    }
    webserver->send(500, "text/plain", "Wrong credentials provided");
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
    webserver->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    webserver->sendHeader(F("Access-Control-Max-Age"), F("600"));
    webserver->sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
    webserver->sendHeader(F("Access-Control-Allow-Headers"), F("*"));
}

// void FSWebServer::handleScanNetworks()
// {
//     String jsonList = "[";
//     log_info("Scanning WiFi networks...");
//     int n = WiFi.scanNetworks();
//     log_info(" complete.");
//     if (n == 0) {
//         log_info("no networks found");
//         webserver->send(200, "text/json", "[]");
//         return;
//     } else {
//         log_info("%d networks found", n);

//         for (int i = 0; i < n; ++i) {
//             String ssid = WiFi.SSID(i);
//             int rssi = WiFi.RSSI(i);
// #if defined(ESP8266)
//             String security = WiFi.encryptionType(i) == AUTH_OPEN ? "none" : "enabled";
// #elif defined(ESP32)
//             String security = WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "none" : "enabled";
// #endif
//             jsonList += "{\"ssid\":\"";
//             jsonList += ssid;
//             jsonList += "\",\"strength\":\"";
//             jsonList += rssi;
//             jsonList += "\",\"security\":";
//             jsonList += security == "none" ? "false" : "true";
//             jsonList += ssid.equals(WiFi.SSID()) ? ",\"selected\": true" : "";
//             jsonList += i < n - 1 ? "}," : "}";
//         }
//         jsonList += "]";
//     }
//     webserver->send(200, "text/json", jsonList);
// }

void FSWebServer::handleScanNetworks() {
    log_info("Start scan WiFi networks");
    int res = WiFi.scanNetworks();
    log_info(" done!\nNumber of networks: %d", res);
    DynamicJsonDocument doc(res*96);
    JsonArray array = doc.to<JsonArray>();

    if (res > 0) {
        for (int i = 0; i < res; ++i) {
            String ssid = WiFi.SSID(i);
            JsonObject obj = array.createNestedObject();
            obj["strength"] = WiFi.RSSI(i);
            obj["ssid"] = WiFi.SSID(i);
            #if defined(ESP8266)
            obj["security"] = AUTH_OPEN ? "none" : "enabled";
            #elif defined(ESP32)
            obj["security"] = WIFI_AUTH_OPEN ? "none" : "enabled";
            #endif
            if (ssid.equals(WiFi.SSID()))
                obj["selected"] =  true;
        }
        WiFi.scanDelete();
    }
    String json;
    serializeJson(doc, json);
    webserver->send(200, "application/json", json);
    log_info("%s", json.c_str());
}

#if ESP_FS_WS_SETUP

void FSWebServer::getStatus()
{
    IPAddress ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP() : WiFi.softAPIP();
    String reply = "{\"firmware\": \"";
    reply += m_version;
    reply += "\", \"mode\":\"";
    if (WiFi.status() == WL_CONNECTED) {
        reply += " Station (";
        reply += WiFi.SSID();
        reply += ")";
    }
    else
        reply += "Access Point";
    reply += "\", \"ip\":\"";
    reply += ip.toString();
    reply += "\"}";
    webserver->send(200, "application/json", reply);
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
    log_debug("handleSetup");
#if ESP_FS_WS_SETUP_HTM
    webserver->sendHeader(PSTR("Content-Encoding"), "gzip");
    webserver->send_P(200, "text/html", SETUP_HTML, SETUP_HTML_SIZE);
#else
    replyToCLient(NOT_FOUND, PSTR("FILE_NOT_FOUND"));
#endif
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
    webserver->send((Update.hasError()) ? 500 : 200, "text/plain", txt);
    if (!Update.hasError()) {
        delay(500);
        ESP.restart();
    }
}

void FSWebServer::update_first()
{
    static uint8_t otaDone = 0;

    size_t fsize;
    if (webserver->hasArg("size"))
        fsize = webserver->arg("size").toInt();
    else {
        webserver->send(500, "text/plain", "Request malformed: missing file size");
        return;
    }

    HTTPUpload& upload = webserver->upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Receiving Update: %s, Size: %d\n", upload.filename.c_str(), fsize);
        if (!Update.begin(fsize)) {
            otaDone = 0;
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        } else {
            static uint32_t pTime = millis();
            if (millis() - pTime > 500) {
                pTime = millis();
                otaDone = 100 * Update.progress() / Update.size();
                Serial.printf("OTA progress: %d%%\n", otaDone);
            }
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("Update Success: %u bytes\nRebooting...\n", upload.totalSize);
        } else {
#if defined(ESP8266)
            Serial.printf("%s\n", Update.getErrorString().c_str());
#elif defined(ESP32)
            Serial.printf("%s\n", Update.errorString());
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
    String path = uri;
    log_debug("handleFileRead: %s", path);
    if (path.endsWith("/")) {
        path += "index.htm";
    }
    String pathWithGz = path + ".gz";

    if (m_filesystem->exists(pathWithGz) || m_filesystem->exists(path)) {
        if (m_filesystem->exists(pathWithGz)) {
            path += ".gz";
        }
        const char* contentType = getContentType(path.c_str());
        File file = m_filesystem->open(path, "r");
        if (webserver->streamFile(file, contentType) != file.size()) {
            log_debug(PSTR("Sent less data than expected!"));
            // webserver->stop();
        }
        file.close();
        return true;
    }
    return false;
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
                    log_error("Error. Folder %s not created\n", dir.c_str());
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
    HTTPUpload& upload = webserver->upload();
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

        log_debug("handleFileUpload Name: %s", filename.c_str());
        m_uploadFile = m_filesystem->open(filename, "w");
        if (!m_uploadFile) {
            replyToCLient(ERROR, PSTR("CREATE FAILED"));
            return;
        }
        log_debug("Upload: START, filename: %s", filename.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (m_uploadFile) {
            size_t bytesWritten = m_uploadFile.write(upload.buf, upload.currentSize);
            if (bytesWritten != upload.currentSize) {
                replyToCLient(ERROR, PSTR("WRITE FAILED"));
                return;
            }
        }
        log_debug("Upload: WRITE, Bytes: %d", upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (m_uploadFile) {
            m_uploadFile.close();
        }
        log_debug("Upload: END, Size: %d", upload.totalSize);
    }
}

void FSWebServer::replyOK()
{
    replyToCLient(MSG_OK, "");
}

void FSWebServer::replyToCLient(int msg_type = 0, const char* msg = "")
{
    webserver->sendHeader("Access-Control-Allow-Origin", "*");
    switch (msg_type) {
    case MSG_OK:
        webserver->send(200, FPSTR(TEXT_PLAIN), "");
        break;
    case CUSTOM:
        webserver->send(200, FPSTR(TEXT_PLAIN), msg);
        break;
    case NOT_FOUND:
        webserver->send(404, FPSTR(TEXT_PLAIN), msg);
        break;
    case BAD_REQUEST:
        webserver->send(400, FPSTR(TEXT_PLAIN), msg);
        break;
    case ERROR:
        webserver->send(500, FPSTR(TEXT_PLAIN), msg);
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
    log_debug("Error on %s: %s", filename.c_str(), error.c_str());
}

const char* FSWebServer::getContentType(const char* filename)
{
    if (webserver->hasArg("download"))
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
    if (!webserver->hasArg("dir")) {
        return replyToCLient(BAD_REQUEST, "DIR ARG MISSING");
    }

    String path = webserver->arg("dir");
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
    webserver->send(200, "text/json", output);
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
    String path = webserver->arg("path");
    if (path.isEmpty()) {
        replyToCLient(BAD_REQUEST, PSTR("PATH ARG MISSING"));
        return;
    }
    if (path == "/") {
        replyToCLient(BAD_REQUEST, PSTR("BAD PATH"));
        return;
    }

    String src = webserver->arg("src");
    if (src.isEmpty()) {
        // No source specified: creation
        log_debug("handleFileCreate: %s", path.c_str());
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

        log_debug("handleFileCreate: %s from %s", path.c_str(), src.c_str());
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
    String path = webserver->arg(0);
    if (path.isEmpty() || path == "/")  {
        replyToCLient(BAD_REQUEST, PSTR("BAD PATH"));
    }
    if (!m_filesystem->exists(path))  {
        replyToCLient(NOT_FOUND, PSTR(FILE_NOT_FOUND));
    }
    deleteContent(path);
    replyOK();
}

// void FSWebServer::handleFileDelete()
// {

//     String path = webserver->arg(0);
//     if (path.isEmpty() || path == "/") {
//         replyToCLient(BAD_REQUEST, PSTR("BAD PATH"));
//         return;
//     }

//     DebugPrintf_P(PSTR("handleFileDelete: %s\n"), path.c_str());
//     if (!m_filesystem->exists(path)) {
//         replyToCLient(NOT_FOUND, PSTR(FILE_NOT_FOUND));
//         return;
//     }
//     // deleteRecursive(path);
//     File root = m_filesystem->open(path, "r");
//     // If it's a plain file, delete it
//     if (!root.isDirectory()) {
//         root.close();
//         m_filesystem->remove(path);
//         replyOK();
//     } else {
//         m_filesystem->rmdir(path);
//         replyOK();
//     }
// }

/*
    This specific handler returns the edit.htm (or a gzipped version) from the /edit folder.
    If the file is not present but the flag INCLUDE_FALLBACK_INDEX_HTM has been set, falls back to the version
    embedded in the program code.
    Otherwise, fails with a 404 page with debug information
*/
void FSWebServer::handleGetEdit()
{
#if ESP_FS_WS_EDIT_HTM
    webserver->sendHeader(PSTR("Content-Encoding"), "gzip");
    webserver->send_P(200, "text/html", edit_htm_gz, EDIT_HTML_SIZE);
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

    size_t totalBytes = 1024;
    size_t usedBytes = 0;

#ifdef ESP8266
    FSInfo fs_info;
    m_filesystem->info(fs_info);
    totalBytes = fs_info.totalBytes;
    usedBytes = fs_info.usedBytes;
#elif defined(ESP32)
    // totalBytes = m_filesystem->totalBytes();
    // usedBytes = m_filesystem->usedBytes();
#endif

    String json;
    json.reserve(128);
    json = "{\"type\":\"Filesystem\", \"isOk\":";
    if (m_fsOK) {
        uint32_t ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP() : WiFi.softAPIP();
        json += PSTR("\"true\", \"totalBytes\":\"");
        json += totalBytes;
        json += PSTR("\", \"usedBytes\":\"");
        json += usedBytes;
        json += PSTR("\", \"mode\":\"");
        json += WiFi.status() == WL_CONNECTED ? "Station" : "Access Point";
        json += PSTR("\", \"ssid\":\"");
        json += WiFi.SSID();
        json += PSTR("\", \"ip\":\"");
        json += ip;
        json += "\"";
    } else
        json += "\"false\"";
    json += PSTR(",\"unsupportedFiles\":\"\"}");
    webserver->send(200, "application/json", json);
}

#endif // ESP_FS_WS_EDIT

////////////////////////////////  Filesystem  /////////////////////////////////////////
// List all files
// void PrintDir(fs::FS& fs, Print& p, const char* dirName, uint8_t level)
// {
//     File root = fs.open(dirName, "r");
//     if (!root)
//         return;
//     if (!root.isDirectory())
//         return;
//     File file = root.openNextFile();
//     while (file) {
//         for (uint8_t lev = 0; lev < level; lev++)
//             p.print("  ");
//         if (file.isDirectory()) {
//             String dirStr;
//             if (strcmp(dirName, "/") != 0)
//                 dirStr += dirName;
//             dirStr += "/";
//             dirStr += file.name();
//             p.printf("%s\n", dirStr.c_str());
//             PrintDir(fs, p, dirStr.c_str(), level + 1);
//         } else {
//             p.printf("|__ FILE: %s (%d bytes)\n",file.name(), file.size());
//         }
//         file = root.openNextFile();
//     }
// }


void PrintDir(fs::FS& fs, Print& p, const char* dirName, uint8_t level)
{
    p.printf("\nListing directory: %s\n", dirName);
    File root = fs.open(dirName, "r");
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            if(level){
            #ifdef ESP32
			  PrintDir(fs, Serial, file.path(), level - 1);
			#elif defined(ESP8266)
			  PrintDir(fs, Serial, file.fullName(), level - 1);
			#endif
            }
        } else {
            p.printf("|__ %s \t(%d bytes)\n",file.name(), file.size());
        }
        file = root.openNextFile();
    }
}