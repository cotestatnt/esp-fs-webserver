#include "esp-fs-webserver.h"
#define DEBUG_MODE_WS true
#if DEBUG_MODE_WS
#define DebugPrint(x) Serial.print(x)
#define DebugPrintln(x) Serial.println(x)
#else
#define DebugPrint(x)
#define DebugPrintln(x)
#endif


FSWebServer::FSWebServer(fs::FS &fs, WebServerClass& server){
    m_filesystem = &fs;
    webserver = &server;
}


const char* FSWebServer::getContentType(const char* filename) {
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

bool FSWebServer::begin() {
    File root = m_filesystem->open("/", "r");
    if(root) {
        m_fsOK = true;
        File root = m_filesystem->open("/", "r");
        File file = root.openNextFile();
        while (file){
            const char* fileName = file.name();
            size_t fileSize = file.size();
            Serial.printf("FS File: %s, size: %lu\n", fileName, (long unsigned)fileSize);
            file = root.openNextFile();
        }
        Serial.println();
    }

#ifdef INCLUDE_EDIT_HTM
    webserver->on("/status", HTTP_GET, std::bind(&FSWebServer::handleStatus, this));
    webserver->on("/list", HTTP_GET, std::bind(&FSWebServer::handleFileList, this));
    webserver->on("/edit", HTTP_GET, std::bind(&FSWebServer::handleGetEdit, this));
    webserver->on("/edit", HTTP_PUT, std::bind(&FSWebServer::handleFileCreate, this));
    webserver->on("/edit", HTTP_DELETE, std::bind(&FSWebServer::handleFileDelete, this));
#endif
    webserver->onNotFound(std::bind(&FSWebServer::handleRequest, this));
    webserver->on("/favicon.ico", HTTP_GET, std::bind(&FSWebServer::replyOK, this));
    webserver->on("/", HTTP_GET, std::bind(&FSWebServer::handleIndex, this));
    webserver->on("/setup", HTTP_GET, std::bind(&FSWebServer::handleSetup, this));
    webserver->on("/scan", HTTP_GET, std::bind(&FSWebServer::handleScanNetworks, this));
    webserver->on("/connect", HTTP_POST, std::bind(&FSWebServer::doWifiConnection, this));
    webserver->on("/restart", HTTP_GET, std::bind(&FSWebServer::doRestart, this));
    webserver->on("/ipaddress", HTTP_GET, std::bind(&FSWebServer::getIpAddress, this));

    // Captive Portal redirect
    webserver->on("/redirect", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    // Windows
    webserver->on("/connecttest.txt", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    // Apple
    webserver->on("/hotspot-detect.html", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    // Android
    webserver->on("/generate_204", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));
    webserver->on("/gen_204", HTTP_GET, std::bind(&FSWebServer::captivePortal, this));

    // Upload file
    // - first callback is called after the request has ended with all parsed arguments
    // - second callback handles file upload at that location
    webserver->on("/edit",  HTTP_POST, std::bind(&FSWebServer::replyOK, this), std::bind(&FSWebServer::handleFileUpload, this));
#ifdef ESP32
    webserver->enableCrossOrigin(true);
#endif
    webserver->setContentLength(50);
    webserver->begin();
    return true;
}


IPAddress FSWebServer::setAPmode(const char* ssid, const char* psk) {
	m_apmode = true;
	WiFi.mode(WIFI_AP);
    WiFi.persistent(false);
	WiFi.softAP(ssid, psk);
	/* Setup the DNS server redirecting all the domains to the apIP */
	m_dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	m_dnsServer.start(53, "*", WiFi.softAPIP());
    return WiFi.softAPIP();
}



IPAddress FSWebServer::startWiFi(uint32_t timeout, const char* apSSID, const char* apPsw ) {
  IPAddress ip;
  m_timeout = timeout;
  WiFi.mode(WIFI_STA);

  const char* _ssid;
  const char* _pass;

 #if defined(ESP8266)
  struct station_config conf;
  wifi_station_get_config_default(&conf);
  _ssid = reinterpret_cast<const char*> (conf.ssid);
  _pass = reinterpret_cast<const char*> (conf.password);

#elif defined(ESP32)
  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_STA, &conf);

  _ssid = reinterpret_cast<const char*> (conf.sta.ssid);
  _pass = reinterpret_cast<const char*> (conf.sta.password);
#endif

  Serial.print(F("Connecting to SSID "));
  Serial.println(_ssid);

  if( _ssid != nullptr && _pass != nullptr )
	WiFi.begin(_ssid, _pass);
  else
	WiFi.begin();

  uint32_t startTime = millis();
  while (WiFi.status() != WL_CONNECTED ){
    delay(500);
    Serial.print(".");

    // If no connection go in Access Point mode
    if( millis() - startTime > m_timeout ) {
      if( apSSID != nullptr && apPsw != nullptr )
        setAPmode(apSSID, apPsw);
      else
        setAPmode("ESP_AP", "123456789");

      ip = WiFi.softAPIP();
      Serial.print(F("\nAP mode.\nServer IP address: "));
      Serial.println(ip);
      return ip;
    }
  }
  Serial.println();
  ip = WiFi.localIP();

  return ip;
}

////////////////////////////////  WiFi  /////////////////////////////////////////


/**
 * Redirect to captive portal if we got a request for another domain.
 */
bool FSWebServer::captivePortal() {
  IPAddress ip = webserver->client().localIP();
  char serverLoc [sizeof("https:://255.255.255.255/")+ sizeof(m_apWebpage) + 1];
  snprintf(serverLoc, sizeof(serverLoc), "http://%d.%d.%d.%d%s", ip[0], ip[1], ip[2], ip[3], m_apWebpage);

  // redirect if hostheader not server ip, prevent redirect loops
  if (strcmp(serverLoc, webserver->hostHeader().c_str())) {
    webserver->sendHeader(F("Location"), serverLoc, true);
    webserver->send(302, F("text/html"), "");       // Empty content inhibits Content-length header so we have to close the socket ourselves.
    webserver->client().stop();                     // Stop is needed because we sent no content length
    return true;
  }
  return false;
}


void FSWebServer::handleRequest(){
    if (!m_fsOK){
        replyToCLient(ERROR, PSTR(FS_INIT_ERROR));
        return;
    }
#if defined(ESP32)
    String _url =  WebServer::urlDecode(webserver->uri());
#elif defined(ESP8266)
    String _url = ESP8266WebServer::urlDecode(webserver->uri());
#endif
    // First try to find and return the requested file from the filesystem,
    // and if it fails, return a 404 page with debug information
    //Serial.println(_url);
    if (handleFileRead(_url))
        return;
    else
        replyToCLient(NOT_FOUND, PSTR(FILE_NOT_FOUND));
}


void FSWebServer::getIpAddress() {
    webserver->send(200, "text/json", WiFi.localIP().toString());
}

void FSWebServer::doRestart(){
    webserver->send(200, "text/json", "Going to restart ESP");
    delay(500);
    ESP.restart();
}


void FSWebServer::doWifiConnection(){
    String ssid, pass;

	bool persistent = true;

    if(webserver->hasArg("ssid")) {
        ssid = webserver->arg("ssid");
	}

	if(webserver->hasArg("password")) {
        pass = webserver->arg("password");
	}

	if(webserver->hasArg("persistent")) {
        String pers = webserver->arg("persistent");
		if (pers.equals("false")) {
			persistent = false;
		}
	}

	if(WiFi.status() == WL_CONNECTED) {
		webserver->send(500, "text/plain", "WiFi already connected");
		return;
	}

    if(ssid.length() && pass.length()) {
        // Try to connect to new ssid


        if(WiFi.status() != WL_CONNECTED)
            WiFi.begin(ssid.c_str(), pass.c_str());

        uint32_t beginTime = millis();
        while (WiFi.status() != WL_CONNECTED ){
            delay(500);
            Serial.print("*.*");
            if( millis() - beginTime > m_timeout )
                break;
        }
        // reply to client
        if (WiFi.status() == WL_CONNECTED ) {
            m_apmode = false;
			WiFi.softAPdisconnect();
			WiFi.mode(WIFI_STA);

			// Store current WiFi configuration in flash
			if(persistent) {
#if defined(ESP8266)
				struct station_config stationConf;
				wifi_station_get_config_default(&stationConf);
				os_memcpy(&stationConf.ssid, ssid.c_str(), ssid.length());
				os_memcpy(&stationConf.password, pass.c_str(), pass.length());
				wifi_set_opmode( STATION_MODE );
				wifi_station_set_config(&stationConf);

#elif defined(ESP32)
				wifi_config_t stationConf;
				esp_wifi_get_config(WIFI_IF_STA, &stationConf);
				memcpy(&stationConf.sta.ssid, ssid.c_str(), ssid.length());
				memcpy(&stationConf.sta.password, pass.c_str(), pass.length());
				esp_wifi_set_config(WIFI_IF_STA, &stationConf);
#endif
			}

            IPAddress ip = WiFi.localIP();
            Serial.print("\nConnected! IP address: ");
            Serial.println(ip);
            //webserver->send(200, "text/plain", "Connection succesfull");

            // Redirect browser to new location
            String serverLoc = F("http://");
            for (int i=0; i<4; i++)
                serverLoc += i  ? "." + String(ip[i]) : String(ip[i]);
            serverLoc += "/";
            webserver->sendHeader(F("Location"), serverLoc, true);
            webserver->send(302, F("text/html"), "");
            webserver->client().stop();
            m_dnsServer.stop();
        }
        else
            webserver->send(500, "text/plain", "Connection error, maybe the password is wrong?");
    }
    webserver->send(500, "text/plain", "Wrong credentials provided");
}

void FSWebServer::setCrossOrigin(){
    webserver->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    webserver->sendHeader(F("Access-Control-Max-Age"), F("600"));
    webserver->sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
    webserver->sendHeader(F("Access-Control-Allow-Headers"), F("*"));
};

void FSWebServer::handleScanNetworks() {

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
        jsonList += i < n-1 ? "}," : "}";
    }
    jsonList += "]";
  }
  webserver->send(200, "text/json", jsonList);
  DebugPrintln(jsonList);
}


void FSWebServer::handleSetup(){
    webserver->sendHeader(PSTR("Content-Encoding"), "gzip");
    webserver->send_P(200, "text/html", WEBPAGE_HTML, WEBPAGE_HTML_SIZE);
}

void FSWebServer::handleIndex(){
    if (m_filesystem->exists("/index.htm")) {
        handleFileRead("/index.htm");
    }
	else if (m_filesystem->exists("/index.html")) {
        handleFileRead("/index.html");
    }
    else {
        handleSetup();
    }
}

/*
    Read the given file from the filesystem and stream it back to the client
*/
bool FSWebServer::handleFileRead(const String &uri) {
  String path = uri;
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
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
        DBG_OUTPUT_PORT.println(PSTR("Sent less data than expected!"));
        //webserver->stop();
    }
    file.close();
    return true;
  }
  return false;
}


/*
    Handle a file upload request
*/
void FSWebServer::handleFileUpload() {
    if (webserver->uri() != "/edit") {
        return;
    }
    HTTPUpload& upload = webserver->upload();
    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        String result;
        // Make sure paths always start with "/"
        if (!filename.startsWith("/")) {
            filename = "/" + filename;
        }
        checkForUnsupportedPath(filename, result);
        if (result.length() > 0) {
            replyToCLient(ERROR, PSTR("INVALID FILENAME"));
            return;
        }

        DBG_OUTPUT_PORT.printf_P(PSTR("handleFileUpload Name: %s\n"), filename.c_str());
        m_uploadFile = m_filesystem->open(filename, "w");
        if (!m_uploadFile) {
            replyToCLient(ERROR, PSTR("CREATE FAILED"));
            return;
        }
        DBG_OUTPUT_PORT.printf_P(PSTR("Upload: START, filename: %s\n"), filename.c_str());
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (m_uploadFile) {
            size_t bytesWritten = m_uploadFile.write(upload.buf, upload.currentSize);
            if (bytesWritten != upload.currentSize) {
                replyToCLient(ERROR, PSTR("WRITE FAILED"));
                return;
            }
        }
        DBG_OUTPUT_PORT.printf_P(PSTR("Upload: WRITE, Bytes: %d\n"), upload.currentSize);
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (m_uploadFile) {
            m_uploadFile.close();
        }
        DBG_OUTPUT_PORT.printf_P(PSTR("Upload: END, Size: %d\n"), upload.totalSize);
    }
}

void FSWebServer::replyToCLient(int msg_type=0, const char* msg="") {
    webserver->sendHeader("Access-Control-Allow-Origin", "*");
    switch (msg_type){
        case OK:
            webserver->send(200, FPSTR(TEXT_PLAIN), "");
            break;
        case CUSTOM:
            webserver->send(200, FPSTR(TEXT_PLAIN), msg);
            break;
        case NOT_FOUND:
            webserver->send(404, FPSTR(TEXT_PLAIN), msg);
            break;
        case BAD_REQUEST:
            webserver->send(400, FPSTR(TEXT_PLAIN), msg );
            break;
        case ERROR:
            webserver->send(500, FPSTR(TEXT_PLAIN), msg );
            break;
    }
}

void FSWebServer::replyOK(){
    replyToCLient(OK, "");
}

/*
    Checks filename for character combinations that are not supported by FSBrowser (alhtough valid on SPIFFS).
    Returns an empty String if supported, or detail of error(s) if unsupported
*/
void FSWebServer::checkForUnsupportedPath(String &filename, String &error) {
    if (!filename.startsWith("/")) {
        error += PSTR(" !! NO_LEADING_SLASH !! ");
    }
    if (filename.indexOf("//") != -1) {
        error += PSTR(" !! DOUBLE_SLASH !! ");
    }
    if (filename.endsWith("/")) {
        error += PSTR(" ! TRAILING_SLASH ! ");
    }
    DBG_OUTPUT_PORT.println(filename);
    DBG_OUTPUT_PORT.println(error);
}


// edit page, in usefull in some situation, but if you need to provide only a web interface, you can disable
#ifdef INCLUDE_EDIT_HTM

/*
    Return the list of files in the directory specified by the "dir" query string parameter.
    Also demonstrates the use of chuncked responses.
*/
void FSWebServer::handleFileList() {
    if (!webserver->hasArg("dir")) {
        return replyToCLient(BAD_REQUEST, "DIR ARG MISSING");
    }

    String path = webserver->arg("dir");
    DBG_OUTPUT_PORT.println("handleFileList: " + path);
    if (path != "/" && !m_filesystem->exists(path)) {
        return replyToCLient(BAD_REQUEST, "BAD PATH");
    }

    File root = m_filesystem->open(path, "r");
    path = String();
    String output;
    output.reserve(256);
    output = "[";
    if(root.isDirectory()){
        File file = root.openNextFile();
        while(file){
            String filename = file.name();
            if (filename.lastIndexOf("/") > -1) {
                filename.remove(0, filename.lastIndexOf("/")+1);
            }
            if (output != "[") {
                output += ',';
            }
            output += "{\"type\":\"";
            output += (file.isDirectory()) ? "dir" : "file";
            output +=  "\",\"size\":\"";
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
void FSWebServer::handleFileCreate(){
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
        DBG_OUTPUT_PORT.printf_P(PSTR("handleFileCreate: %s\n"), path.c_str());
        if (path.endsWith("/")) {
            // Create a folder
            path.remove(path.length() - 1);
            if (!m_filesystem->mkdir(path)) {
                replyToCLient(ERROR, PSTR("MKDIR FAILED"));
                return;
            }
        }
        else {
            // Create a file
            File file = m_filesystem->open(path, "w");
            if (file) {
                file.write(0);
                file.close();
            }
            else {
                replyToCLient(ERROR, PSTR("CREATE FAILED"));
                return;
            }
        }
        replyToCLient(CUSTOM, path.c_str());
    }
    else {
        // Source specified: rename
        if (src == "/") {
            replyToCLient(BAD_REQUEST, PSTR("BAD SRC"));
            return;
        }
        if (!m_filesystem->exists(src)) {
            replyToCLient(BAD_REQUEST, PSTR("BSRC FILE NOT FOUND"));
            return;
        }

        DBG_OUTPUT_PORT.printf_P(PSTR("handleFileCreate: %s from %s\n"), path.c_str(), src.c_str());
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
void FSWebServer::handleFileDelete() {

    String path = webserver->arg(0);
    if (path.isEmpty() || path == "/") {
        replyToCLient(BAD_REQUEST, PSTR("BAD PATH"));
        return;
    }

    DBG_OUTPUT_PORT.printf_P(PSTR("handleFileDelete: %s\n"), path.c_str());
    if (!m_filesystem->exists(path)) {
        replyToCLient(NOT_FOUND, PSTR(FILE_NOT_FOUND));
        return;
    }
    //deleteRecursive(path);
    File root = m_filesystem->open(path, "r");
    // If it's a plain file, delete it
    if (!root.isDirectory()) {
        root.close();
        m_filesystem->remove(path);
        replyOK();
    }
    else {
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
void FSWebServer::handleGetEdit() {
    #ifdef INCLUDE_EDIT_HTM
    webserver->sendHeader(PSTR("Content-Encoding"), "gzip");
    webserver->send_P(200, "text/html", edit_htm_gz, sizeof(edit_htm_gz));
    #else
    replyToCLient(NOT_FOUND, PSTR("FILE_NOT_FOUND"));
    #endif
}

/*
    Return the FS type, status and size info
*/
void FSWebServer::handleStatus() {
    DBG_OUTPUT_PORT.println(PSTR("handleStatus"));

    size_t totalBytes = 1024;
    size_t usedBytes = 1024;

#ifdef ESP8266
    FSInfo fs_info;
    m_filesystem->info(fs_info);
    totalBytes = fs_info.totalBytes;
    usedBytes = fs_info.usedBytes;
#elif defined(ESP32)

#endif

    String json;
    json.reserve(128);
    json = "{\"type\":\"Filesystem\", \"isOk\":";
    if (m_fsOK) {
        json += PSTR("\"true\", \"totalBytes\":\"");
        json += totalBytes;
        json += PSTR("\", \"usedBytes\":\"");
        json += usedBytes;
        json += "\"";
    }
    else
        json += "\"false\"";
    json += PSTR(",\"unsupportedFiles\":\"\"}");
    webserver->send(200, "application/json", json);
}

#endif // INCLUDE_EDIT_HTM
