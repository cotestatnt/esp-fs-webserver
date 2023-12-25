#include "esp-fs-webserver.h"

FSWebServer::FSWebServer(fs::FS& fs, WebServerClass& server)
    : m_apWebpage("/setup")
    , m_apSsid("ESP_AP")
    , m_apPsk("123456789")
{
    m_filesystem = &fs;
    webserver = &server;
}

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

bool FSWebServer::checkDir(const char* dirname)
{
    { // scope for the root file
        File root = m_filesystem->open(dirname, "r");
        if (!root) {
            DebugPrintln("- failed to open directory");
            return false;
        }
        if (!root.isDirectory()) {
            DebugPrintln(" - not a directory");
            return false;
        }
    }
#if ESP_FS_WS_INFO_MODE
    // List all files saved in the selected filesystem
    PrintDir(*m_filesystem, ESP_FS_WS_INFO_OUTPUT, dirname);
#endif
    return true;
}

bool FSWebServer::begin()
{
    DebugPrintln("List the files of webserver: ");
    File root = m_filesystem->open(ESP_FS_WS_CONFIG_FOLDER, "r");
    if (!root) {
        DebugPrintln("Failed to open " ESP_FS_WS_CONFIG_FOLDER " directory. Create new folder");
        m_filesystem->mkdir(ESP_FS_WS_CONFIG_FOLDER);
        // Create empty config file
        File config = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "w");
        config.println("{\"param-box0\":\"\"}");
        config.close();
        ESP.restart();
    }
    m_fsOK = checkDir("/");

    // Backward compatibility, move config.json to new path
    root = m_filesystem->open("/config.json", "r");
    if (root) {
        root.close();
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
    if (!m_apSsid.length() || !m_apPsk.length())
        setAP("ESP_AP", "123456789");

    m_apmode = true;
    WiFi.mode(WIFI_AP_STA);
    WiFi.persistent(false);
    WiFi.softAP(m_apSsid.c_str(), m_apPsk.c_str());
    /* Setup the DNS server redirecting all the domains to the apIP */
    m_dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    IPAddress ip = WiFi.softAPIP();
    m_dnsServer.start(53, "*", ip);
    WiFi.begin(); //???
    ip = WiFi.softAPIP();
    InfoPrintln(F("AP mode."));
    InfoPrint(F("Server IP address: "));
    InfoPrintln(ip);
    return ip;
}

IPAddress FSWebServer::startWiFi(uint32_t timeout, bool apFlag, CallbackF fn)
{
    if (timeout > 0)
        m_timeout = timeout;
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
        m_apmode = false;
        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid, _pass);
        uint32_t startTime = millis();
        InfoPrint(F("Connecting to "));
        InfoPrintln(_ssid);
        while ((WiFi.status() != WL_CONNECTED) && (timeout > 0)) {
            // execute callback function during wifi connection
            if (fn != nullptr)
                fn();
            delay(250);
            InfoPrint(".");
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
    String serverLoc("https:://");
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
    String ssid, pass;
    bool persistent = true;
    WiFi.mode(WIFI_AP_STA);

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
        InfoPrintln("Disconnect from current WiFi network");
        WiFi.disconnect();
    }

    if (ssid.length()) { // pass could be empty
        // Try to connect to new ssid
        InfoPrint("Connecting to ");
        InfoPrintln(ssid);
        WiFi.begin(ssid.c_str(), pass.c_str());

        uint32_t beginTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(250);
            InfoPrint(".");
            if (millis() - beginTime > m_timeout)
                break;
        }
        // reply to client
        if (WiFi.status() == WL_CONNECTED) {
            // WiFi.softAPdisconnect();
            IPAddress ip = WiFi.localIP();
            InfoPrint("Connected to Wifi! IP address: ");
            InfoPrintln(ip);
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
};

void FSWebServer::handleScanNetworks()
{
    String jsonList = "[";
    DebugPrint("Scanning WiFi networks...");
    int n = WiFi.scanNetworks();
    DebugPrintln(" complete.");
    if (n == 0) {
        DebugPrintln("no networks found");
        webserver->send(200, "text/json", "[]");
        return;
    } else {
        DebugPrint(n);
        DebugPrintln(" networks found:");

        for (int i = 0; i < n; ++i) {
            String ssid = WiFi.SSID(i);
            int rssi = WiFi.RSSI(i);
#if defined(ESP8266)
            String security = WiFi.encryptionType(i) == AUTH_OPEN ? "none" : "enabled";
#elif defined(ESP32)
            String security = WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "none" : "enabled";
#endif
            jsonList += "{\"ssid\":\"";
            jsonList += ssid;
            jsonList += "\",\"strength\":\"";
            jsonList += rssi;
            jsonList += "\",\"security\":";
            jsonList += security == "none" ? "false" : "true";
            jsonList += ssid.equals(WiFi.SSID()) ? ",\"selected\": true" : "";
            jsonList += i < n - 1 ? "}," : "}";
        }
        jsonList += "]";
    }
    webserver->send(200, "text/json", jsonList);
    DebugPrintln(jsonList);
}

#if ESP_FS_WS_SETUP

void FSWebServer::getStatus()
{
    uint32_t ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP() : WiFi.softAPIP();
    String reply = "{\"firmware\": \"";
    reply += m_version;
    reply += "\", \"mode\":\"";
    reply += WiFi.status() == WL_CONNECTED ? "Station" : "Access Point";
    reply += "\", \"ip\":";
    reply += ip;
    reply += "}";
    webserver->send(200, "application/json", reply);
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

/*
 * In order to keep config.json file small and clean, custom HTML, CSS and Javascript
 * will be saved as file. The related option will contain the path to this file
 */
bool FSWebServer::optionToFile(const char* filename, const char* str, bool overWrite)
{
    // Check if file is already saved
    File file = m_filesystem->open(filename, "r");
    if (file && !overWrite) {
        file.close();
        return true;
    }
    // Create or overwrite option file
    else {
        file = m_filesystem->open(filename, "w");
        if (file) {
            file.print(str);
            file.close();
            return true;
        } else {
            DebugPrintf("Error writing file %s\n", filename);
        }
    }
    return false;
}

/*
 * Add an option which contain "raw" HTML code to be injected in /setup page
 * Th HTML code will be written in a file with named as option id
 */
void FSWebServer::addHTML(const char* html, const char* id, bool overWrite)
{
    String elementId = "raw-html-";
    elementId += id;
    String path = "/setup/";
    path += elementId;
    path += ".htm";
#if defined(ESP8266)
    String content = html;
    optionToFile(path.c_str(), content.c_str(), overWrite);
#else
    optionToFile(path.c_str(), html, overWrite);
#endif
    addOption(elementId.c_str(), path.c_str(), false);
}

/*
 * Add an option which contain "raw" CSS style to be injected in /setup page
 * Th CSS code will be written in a file with named as option raw-css.css
 */
void FSWebServer::addCSS(const char* css, bool overWrite)
{
#if defined(ESP8266)
    String content = css;
    optionToFile("/setup/raw-css.css", content.c_str(), overWrite);
#else
    optionToFile("/setup/raw-css.css", css, overWrite);
#endif
    addOption("raw-css", "/setup/raw-css.css", true);
}

/*
 * Add an option which contain "raw" JS script to be injected in /setup page
 * Th JS code will be written in a file with named as option raw-javascript.js
 */
void FSWebServer::addJavascript(const char* script, bool overWrite)
{
#if defined(ESP8266)
    String content = script;
    optionToFile("/setup/raw-javascript.js", content.c_str(), overWrite);
#else
    optionToFile("/setup/raw-javascript.js", script, overWrite);
#endif
    addOption("raw-javascript", "/setup/raw-javascript.js", true);
}

void FSWebServer::addDropdownList(const char* label, const char** array, size_t size)
{
    File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "r");
    int sz = file.size() * 1.33;
    int docSize = max(sz, 2048);
    DynamicJsonDocument doc((size_t)docSize);
    if (file) {
        // If file is present, load actual configuration
        DeserializationError error = deserializeJson(doc, file);
        if (error) {
            DebugPrintln(F("Failed to deserialize file, may be corrupted"));
            DebugPrintln(error.c_str());
            file.close();
            return;
        }
        file.close();
    } else {
        DebugPrintln(F("File not found, will be created new configuration file"));
    }

    numOptions++;

    // If key is present in json, we don't need to create it.
    if (doc.containsKey(label))
        return;

    JsonObject obj = doc.createNestedObject(label);
    obj["selected"] = array[0]; // first element selected as default
    JsonArray arr = obj.createNestedArray("values");
    for (unsigned int i = 0; i < size; i++) {
        arr.add(array[i]);
    }

    file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "w");
    if (serializeJsonPretty(doc, file) == 0) {
        DebugPrintln(F("Failed to write to file"));
    }
    file.close();
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
    DebugPrintln("handleSetup");
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
    DebugPrintln("handleIndex");
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
    DebugPrintln("handleFileRead: " + path);
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
            DebugPrintln(PSTR("Sent less data than expected!"));
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
                    DebugPrintf("Folder %s created\n", dir.c_str());
                } else {
                    DebugPrintf("Error. Folder %s not created\n", dir.c_str());
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

        DebugPrintf_P(PSTR("handleFileUpload Name: %s\n"), filename.c_str());
        m_uploadFile = m_filesystem->open(filename, "w");
        if (!m_uploadFile) {
            replyToCLient(ERROR, PSTR("CREATE FAILED"));
            return;
        }
        DebugPrintf_P(PSTR("Upload: START, filename: %s\n"), filename.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (m_uploadFile) {
            size_t bytesWritten = m_uploadFile.write(upload.buf, upload.currentSize);
            if (bytesWritten != upload.currentSize) {
                replyToCLient(ERROR, PSTR("WRITE FAILED"));
                return;
            }
        }
        DebugPrintf_P(PSTR("Upload: WRITE, Bytes: %d\n"), upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (m_uploadFile) {
            m_uploadFile.close();
        }
        DebugPrintf_P(PSTR("Upload: END, Size: %d\n"), upload.totalSize);
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
    DebugPrintln(filename);
    DebugPrintln(error);
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
    DebugPrintln("handleFileList: " + path);
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
        DebugPrintf_P(PSTR("handleFileCreate: %s\n"), path.c_str());
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

        DebugPrintf_P(PSTR("handleFileCreate: %s from %s\n"), path.c_str(), src.c_str());
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
void FSWebServer::handleFileDelete()
{

    String path = webserver->arg(0);
    if (path.isEmpty() || path == "/") {
        replyToCLient(BAD_REQUEST, PSTR("BAD PATH"));
        return;
    }

    DebugPrintf_P(PSTR("handleFileDelete: %s\n"), path.c_str());
    if (!m_filesystem->exists(path)) {
        replyToCLient(NOT_FOUND, PSTR(FILE_NOT_FOUND));
        return;
    }
    // deleteRecursive(path);
    File root = m_filesystem->open(path, "r");
    // If it's a plain file, delete it
    if (!root.isDirectory()) {
        root.close();
        m_filesystem->remove(path);
        replyOK();
    } else {
        m_filesystem->rmdir(path);
        replyOK();
    }
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
    DebugPrintln(PSTR("handleStatus"));

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