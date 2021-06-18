#include "esp_webserver.h"

WebServer::WebServer(fs::FS &fs, WebServerClass& server){
    m_filesystem = &fs;
    webserver = &server;
}


const char* WebServer::getContentType(const char* filename) {
    if (webserver->hasArg("download"))
        return PSTR("application/octet-stream");
    else if (strstr(filename, ".htm"))
        return PSTR("text/html");
    else if (strstr(filename, ".html"))
        return PSTR("text/html");
    else if (strstr(filename, ".css"))
        return PSTR("text/css");
    else if (strstr(filename, ".js"))
        return PSTR("application/javascript");
    else if (strstr(filename, ".png"))
        return PSTR("image/png");
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

bool WebServer::begin(){

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

// #ifdef ESP8266
//     m_server.reset(new ESP8266WebServer(80));
// #else
//     m_server.reset(new WebServer(80));
// #endif

#ifdef INCLUDE_EDIT_HTM
    webserver->on("/status", HTTP_GET, std::bind(&WebServer::handleStatus, this));
    webserver->on("/list", HTTP_GET, std::bind(&WebServer::handleFileList, this));
    webserver->on("/edit", HTTP_GET, std::bind(&WebServer::handleGetEdit, this));
    webserver->on("/edit", HTTP_PUT, std::bind(&WebServer::handleFileCreate, this));
    webserver->on("/edit", HTTP_DELETE, std::bind(&WebServer::handleFileDelete, this));
#endif
    // Handle request for token generation page
    webserver->onNotFound(std::bind(&WebServer::handleRequest, this));
    // m_server->on("/", HTTP_GET, std::bind(&WebServer::handleSetup, this));
    webserver->on("/favicon.ico", HTTP_GET, std::bind(&WebServer::replyOK, this));
    webserver->on("/", HTTP_GET, std::bind(&WebServer::handleSetup, this));
    webserver->on("/setup", HTTP_GET, std::bind(&WebServer::handleSetup, this));
    webserver->on("/scan", HTTP_GET, std::bind(&WebServer::handleScanNetworks, this));
    webserver->on("/connect", HTTP_GET, std::bind(&WebServer::doWifiConnection, this));
    webserver->on("/restart", HTTP_GET, std::bind(&WebServer::doRestart, this));
    webserver->on("/ipaddress", HTTP_GET, std::bind(&WebServer::getIpAddress, this));

    // Upload file
    // - first callback is called after the request has ended with all parsed arguments
    // - second callback handles file upload at that location
    webserver->on("/edit",  HTTP_POST, std::bind(&WebServer::replyOK, this), std::bind(&WebServer::handleFileUpload, this));
#ifdef ESP32
    m_server->enableCrossOrigin(true);
#endif
    webserver->setContentLength(50);
    webserver->begin();
    return true;
}


void WebServer::getIpAddress() {
    webserver->send(200, "text/json", WiFi.localIP().toString());
}

void WebServer::doRestart(){
    webserver->send(200, "text/json", "Going to restart ESP");
    delay(500);
    ESP.restart();
}

void WebServer::doWifiConnection(){
    String ssid, pass;
    if(webserver->hasArg("ssid"))
        ssid = webserver->arg("ssid");
    if(webserver->hasArg("password"))
        pass = webserver->arg("password");

    if(ssid.length() && pass.length()) {
        // Try to connect to new ssid
        if(WiFi.status() != WL_CONNECTED)
            WiFi.begin(ssid.c_str(), pass.c_str());

        uint32_t beginTime = millis();
        while (WiFi.status() != WL_CONNECTED ){
            delay(1000);
            Serial.print("*.*");
            if( millis() - beginTime > 10000 )
                break;
        }
        // reply to client
        if (WiFi.status() == WL_CONNECTED ) {
            Serial.print("\nConnected! IP address: ");
            Serial.println(WiFi.localIP());
            webserver->send(200, "text/plain", "Connection succesfull");
        }
        else
            webserver->send(200, "text/plain", "Connection error");
    }
    webserver->send(200, "text/plain", "Wrong credentials provided");
}

void WebServer::setCrossOrigin(){
    webserver->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    webserver->sendHeader(F("Access-Control-Max-Age"), F("600"));
    webserver->sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
    webserver->sendHeader(F("Access-Control-Allow-Headers"), F("*"));
};

void WebServer::handleScanNetworks() {

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

void WebServer::handleSetup(){
    webserver->sendHeader(PSTR("Content-Encoding"), "gzip");
    webserver->send_P(200, "text/html", WEBPAGE_HTML, WEBPAGE_HTML_SIZE);
}

void WebServer::handleRequest(){
    if (!m_fsOK){
        replyToCLient(ERROR, PSTR(FS_INIT_ERROR));
        return;
    }
#ifdef ESP32
    String _url = WebServer::urlDecode(m_server->uri());
#elif defined(ESP8266)
    String _url = ESP8266WebServer::urlDecode(webserver->uri());
#endif
    // First try to find and return the requested file from the filesystem,
    // and if it fails, return a 404 page with debug information
    if (handleFileRead(_url))
        return;
    else
        replyToCLient(NOT_FOUND, PSTR(FILE_NOT_FOUND));
}


/*
    Read the given file from the filesystem and stream it back to the client
*/
bool WebServer::handleFileRead( const String &uri) {
    DBG_OUTPUT_PORT.printf_P(PSTR("handleFileRead: %s\n"), uri.c_str());

    if (m_filesystem->exists(uri)) {
        const char* contentType = getContentType(uri.c_str());
        File file = m_filesystem->open(uri, "r");

        if(uri.indexOf(".gz") > -1) {
            webserver->sendHeader(PSTR("Content-Encoding"), "gzip");
        }

        if (webserver->streamFile(file, contentType) != file.size()) {
            DBG_OUTPUT_PORT.println(PSTR("Sent less data than expected!"));
        }
        file.close();
        return true;
    }
    return false;
}

/*
    Handle a file upload request
*/
void WebServer::handleFileUpload() {
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

void WebServer::replyToCLient(int msg_type=0, const char* msg="") {
    webserver->sendHeader("Access-Control-Allow-Origin", "*");
    switch (msg_type){
        case OK:
            webserver->send(200, FPSTR(TEXT_PLAIN), "");
            break;
        case CUSTOM:
            webserver->send(404, FPSTR(TEXT_PLAIN), msg);
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

void WebServer::replyOK(){
    replyToCLient(OK, "");
}

/*
    Checks filename for character combinations that are not supported by FSBrowser (alhtough valid on SPIFFS).
    Returns an empty String if supported, or detail of error(s) if unsupported
*/
void WebServer::checkForUnsupportedPath(String &filename, String &error) {
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
void WebServer::handleFileList() {
    if (!webserver->hasArg("dir")) {
        webserver->send(500, "text/plain", "BAD ARGS");
        return;
    }

    String path = webserver->arg("dir");
    DBG_OUTPUT_PORT.println("handleFileList: " + path);

    File root = m_filesystem->open(path, "r");
    path = String();
    String output = "[";
    if(root.isDirectory()){
        File file = root.openNextFile();

        while(file){
            String filename = file.name();
            // Remove the first "/" if present (depends from filesystem used)
            if(filename[0] == '/')
                filename = filename.substring(1);

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
void WebServer::handleFileCreate(){
    String path = webserver->arg("path");
    if (path.isEmpty()) {
        replyToCLient(BAD_REQUEST, PSTR("PATH ARG MISSING"));
        return;
    }
    if (path == "/") {
        replyToCLient(BAD_REQUEST, PSTR("BAD PATH"));
        return;
    }
    if (m_filesystem->exists(path)) {
        replyToCLient(BAD_REQUEST, PSTR("PATH FILE EXISTS"));
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
        if (path.lastIndexOf('/') > -1) {
            path = path.substring(0, path.lastIndexOf('/'));
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
void WebServer::handleFileDelete() {

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
        return;
    }
}

/*
    This specific handler returns the edit.htm (or a gzipped version) from the /edit folder.
    If the file is not present but the flag INCLUDE_FALLBACK_INDEX_HTM has been set, falls back to the version
    embedded in the program code.
    Otherwise, fails with a 404 page with debug information
*/
void WebServer::handleGetEdit() {
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
void WebServer::handleStatus() {
    DBG_OUTPUT_PORT.println(PSTR("handleStatus"));

    size_t totalBytes = 100;// = m_filesystem->totalBytes();
    size_t usedBytes = 1;// = m_filesystem->usedBytes();

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
