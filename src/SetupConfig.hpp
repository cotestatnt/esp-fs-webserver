#ifndef CONFIGURATOR_HPP
#define CONFIGURATOR_HPP
#include <ArduinoJson.h>
#include <FS.h>
#include "SerialLog.h"

#define MIN_F -3.4028235E+38
#define MAX_F 3.4028235E+38

class SetupConfigurator
{
    protected:
        uint8_t numOptions = 0;
        fs::FS* m_filesystem = nullptr;
        #if ARDUINOJSON_VERSION_MAJOR > 6
        JsonDocument* m_doc = nullptr;
        #else
        DynamicJsonDocument* m_doc = nullptr;
        #endif

        bool m_opened = false;

        bool isOpened() {
            return m_opened;
        }

        bool openConfiguration() {
            if (checkConfigFile()) {
                File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "r");
                #if ARDUINOJSON_VERSION_MAJOR > 6
                    m_doc = new JsonDocument();
                #else
                    int sz = file.size() * 1.33;
                    int docSize = max(sz, 2048);
                    m_doc = new DynamicJsonDocument((size_t)docSize);
                #endif
                DeserializationError error = deserializeJson(*m_doc, file);
                if (error) {
                    log_error("Failed to deserialize file, may be corrupted\n %s\n", error.c_str());
                    file.close();
                    return false;
                }
                file.close();
                m_opened = true;
                return true;
            }
            return false;
        }

        bool checkConfigFile() {
            File file = m_filesystem->open(ESP_FS_WS_CONFIG_FOLDER, "r");
            if (!file) {
                log_error("Failed to open /setup directory. Create new folder\n");
                if (!m_filesystem->mkdir(ESP_FS_WS_CONFIG_FOLDER)) {
                    log_error("Error. Folder %s not created", ESP_FS_WS_CONFIG_FOLDER);
                    return false;
                }
            }

            // Check if config file exist, and create if necessary
            if (!m_filesystem->exists(ESP_FS_WS_CONFIG_FILE)) {
                file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "w");
                if (!file) {
                    log_error("Error. File %s not created", ESP_FS_WS_CONFIG_FILE);
                    return false;
                }
                file.println("{\"wifi-box\": \"\", \"dhcp\": false}");
                file.close();
            }
            return true;
        }

    public:
        friend class FSWebServer;
        SetupConfigurator(fs::FS *fs) : m_filesystem(fs) { ; }

        bool closeConfiguration( bool write = true) {
            m_opened = false;
            if (!write) {
                m_doc->clear();
                delete (m_doc);
                m_doc = nullptr;
                return true;
            }

            File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "w");
            if (file) {
                if (serializeJsonPretty(*m_doc, file) == 0) {
                    log_error("Failed to write to file");
                }
                file.close();
                m_doc->clear();
                delete (m_doc);
                m_doc = nullptr;
                return true;
            }
            return false;
        }

        void setLogoBase64(const char* logo, const char* width, const char* height, bool overwrite) {
            char filename[32] = {ESP_FS_WS_CONFIG_FOLDER};
            strcat(filename, "/img-logo-");
            strcat(filename, width);
            strcat(filename, "_");
            strcat(filename, height);
            strcat(filename, ".txt");

            optionToFile(filename, logo, overwrite);
            addOption("img-logo", filename);
        }

        bool optionToFile(const char* filename, const char* str, bool overWrite) {
            // Check if file is already saved
            if (m_filesystem->exists(filename) && !overWrite) {
                return true;
            }
            // Create or overwrite option file
            else {
                File file = m_filesystem->open(filename, "w");
                if (file) {
                    #if defined(ESP8266)
                    String _str = str;
                    file.print(_str);
                    #else
                    file.print(str);
                    #endif
                    file.close();
                    log_debug("File %s saved", filename);
                    return true;
                }
                else {
                    log_debug("Error writing file %s", filename);
                }
            }
            return false;
        }

        void addSource(const String& source, const String& tag, bool overWrite) {
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                }
            }

            String path = ESP_FS_WS_CONFIG_FOLDER;
            path += "/";
            path += tag;

            if (tag.indexOf("html") > -1)
                path += ".htm";
            else if (tag.indexOf("css") > -1)
                path += ".css";
            else if (tag.indexOf("javascript") > -1)
                path += ".js";

            if (optionToFile(path.c_str(), source.c_str(), overWrite)){
                (*m_doc)[tag] = path;
            }
            else {
                log_error("Source option not saved");
            }

        }

        void addHTML(const char* html, const char* id, bool overWrite) {
            String _id = "raw-html-";
            _id  += id;
            String source = html;
            addSource(source, _id, overWrite);
        }

        void addCSS(const char* css,  const char* id, bool overWrite) {
            String _id = "raw-css-" ;
            _id  += id;
            String source = css;
            addSource(source, _id, overWrite);
        }

        void addJavascript(const char* script,  const char* id, bool overWrite) {
            String _id = "raw-javascript-" ;
            _id  += id;
            String source = script;
            addSource(source, _id, overWrite);
        }


        /*
            Add a new dropdown input element
        */
        void addDropdownList(const char *label, const char** array, size_t size) {

            // If key is present in json, we don't need to create it.
        #if ARDUINOJSON_VERSION_MAJOR > 6
            JsonObject obj = (*m_doc)[label].to<JsonObject>();
        #else
            JsonObject obj = (*m_doc).createNestedObject(label);
        #endif

            if (obj.isNull())
                return;

            obj["selected"] = array[0];     // first element selected as default
            #if ARDUINOJSON_VERSION_MAJOR > 6
                JsonArray arr = obj["values"].to<JsonArray>();
            #else
                JsonArray arr = obj.createNestedArray("values");
            #endif

            for (unsigned int i=0; i<size; i++) {
                arr.add(array[i]);
            }

            numOptions++ ;
        }

        /*
            Add a new option box with custom label
        */
        void addOptionBox(const char* boxTitle) {
            addOption("param-box", boxTitle, false);
        }

        /*
            Add custom option to config webpage (float values)
        */
        template <typename T>
        void addOption(const char *label, T val, double d_min, double d_max, double step) {
            addOption(label, val, false, d_min, d_max, step);
        }

        /*
        Add custom option to config webpage (type of parameter will be deduced from variable itself)
        */
        template <typename T>
        void addOption(const char *label, T val, bool hidden = false,
                            double d_min = MIN_F, double d_max = MAX_F, double step = 1.0)
        {

            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                }
            }

            String key = label;
            if (hidden)
                key += "-hidden";
            // Univoque key name
            if (key.equals("param-box"))
                key += numOptions ;
            if (key.equals("raw-javascript"))
                key += numOptions ;

            // If key is present and value is the same, we don't need to create/update it.
            JsonVariant foundObject = (*m_doc)[key];
            if (foundObject.isNull())
                return;
            
            // if (m_doc->containsKey(key.c_str()))
            //     return;

            // if min, max, step != from default, treat this as object in order to set other properties
            if (d_min != MIN_F || d_max != MAX_F || step != 1.0) {
                #if ARDUINOJSON_VERSION_MAJOR > 6
                    JsonObject obj = (*m_doc)[key].to<JsonObject>();
                #else
                    JsonObject obj = (*m_doc).createNestedObject(key);
                #endif
                obj["value"] = static_cast<T>(val);
                obj["min"] = d_min;
                obj["max"] = d_max;
                obj["step"] = step;
            }
            else {
                (*m_doc)[key] = static_cast<T>(val);
            }

            numOptions++;
        }

        /*
            Get current value for a specific custom option (true on success)
        */
        template <typename T>
        bool getOptionValue(const char *label, T &var) {
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                    return false;
                }
            }

            if ((*m_doc)[label]["value"])
                var = (*m_doc)[label]["value"].as<T>();
            else if ((*m_doc)[label]["selected"])
                var = (*m_doc)[label]["selected"].as<T>();
            else
                var = (*m_doc)[label].as<T>();
            return true;
        }

        template <typename T>
        bool saveOptionValue(const char *label, T val) {
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                    return false;
                }
            }

            if ((*m_doc)[label]["value"])
                (*m_doc)[label]["value"] = val;
            else if ((*m_doc)[label]["selected"])
                (*m_doc)[label]["selected"] = val;
            else
                (*m_doc)[label] = val;
            return true;
        }

};

#endif