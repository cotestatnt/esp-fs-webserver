#ifndef CONFIGURATOR_HPP
#define CONFIGURATOR_HPP
#include <ArduinoJson.h>
#include <FS.h>
#include "esp-fs-webserver.h"
#include "SerialLog.h"

#define MIN_F -3.4028235E+38
#define MAX_F 3.4028235E+38

class SetupConfigurator
{
    private:
        fs::FS* m_filesystem = nullptr;
        uint8_t numOptions = 0;

    public:
        SetupConfigurator(fs::FS *fs) {
            m_filesystem = fs;
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

        void addSource(const char* source, const char* tag, bool overWrite) {
            String path = ESP_FS_WS_CONFIG_FOLDER;
            path += "/";
            path += tag;

            if (strstr(tag, "html") != NULL)
                path += ".htm";
            else if (strstr(tag, "css") != NULL)
                path += ".css";
            else if (strstr(tag, "javascript") != NULL)
                path += ".js";

            optionToFile(path.c_str(), source, overWrite);
            addOption(tag, path.c_str(), false);
        }

        void addHTML(const char* html, const char* id, bool overWrite) {
            String _id = "raw-html-";
            _id  += id;
            addSource(html, _id.c_str(), overWrite);
        }

        void addCSS(const char* css,  const char* id, bool overWrite) {
            String _id = "raw-css-" ;
            _id  += id;
            addSource(css, _id.c_str(), overWrite);
        }

        void addJavascript(const char* script,  const char* id, bool overWrite) {
            String _id = "raw-javascript-" ;
            _id  += id;
            addSource(script, _id.c_str(), overWrite);
        }


        /*
            Add a new dropdown input element
        */
        void addDropdownList(const char *label, const char** array, size_t size) {
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
                    return;
                }
                file.close();
            }
            else {
                log_error("File not found, will be created new configuration file");
            }
            numOptions++ ;

            // If key is present in json, we don't need to create it.
            if (doc.containsKey(label))
                return;

            JsonObject obj = doc.createNestedObject(label);
            obj["selected"] = array[0];     // first element selected as default
            JsonArray arr = obj.createNestedArray("values");
            for (unsigned int i=0; i<size; i++) {
                arr.add(array[i]);
            }

            file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "w");
            if (serializeJsonPretty(doc, file) == 0) {
                log_error("Failed to write to file");
            }
            file.close();
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
        /*
        Add custom option to config webpage (type of parameter will be deduced from variable itself)
        */
        template <typename T>
        void addOption(const char *label, T val, bool hidden = false,
                            double d_min = MIN_F, double d_max = MAX_F, double step = 1.0)
        {
            File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "r");
            int sz = file.size() * 1.33;
            int docSize = max(sz, 2048);
            DynamicJsonDocument doc((size_t)docSize);
            if (file) {
                // If file is present, load actual configuration
                DeserializationError error = deserializeJson(doc, file);
                if (error) {
                log_error("Failed to deserialize file, may be corrupted\n %s", error.c_str());
                file.close();
                return;
                }
                file.close();
            }
            else
                log_error("File not found, will be created new configuration file");

            numOptions++ ;
            String key = label;
            if (hidden)
                key += "-hidden";
            // Univoque key name
            if (key.equals("param-box"))
                key += numOptions ;
            if (key.equals("raw-javascript"))
                key += numOptions ;

            // If key is present in json, we don't need to create it.
            if (doc.containsKey(key.c_str()))
                return;

            // if min, max, step != from default, treat this as object in order to set other properties
            if (d_min != MIN_F || d_max != MAX_F || step != 1.0) {
                JsonObject obj = doc.createNestedObject(key);
                obj["value"] = static_cast<T>(val);
                obj["min"] = d_min;
                obj["max"] = d_max;
                obj["step"] = step;
            }
            else {
                doc[key] = static_cast<T>(val);
            }

            file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "w");
            if (serializeJsonPretty(doc, file) == 0)
                log_error("Failed to write to file");
            file.close();
        }

        /*
            Get current value for a specific custom option (true on success)
        */
        template <typename T>
        bool getOptionValue(const char *label, T &var) {
            File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "r");
            DynamicJsonDocument doc(file.size() * 1.33);
            if (file) {
                DeserializationError error = deserializeJson(doc, file);
                if (error) {
                    log_error("Failed to deserialize file, may be corrupted\n %s\n", error.c_str());
                    file.close();
                    return false;
                }
                file.close();
            }
            else
            return false;

            if (doc[label]["value"])
                var = doc[label]["value"].as<T>();
            else if (doc[label]["selected"])
                var = doc[label]["selected"].as<T>();
            else
                var = doc[label].as<T>();
            return true;
        }

        template <typename T>
        bool saveOptionValue(const char *label, T val) {
            File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "w");
            DynamicJsonDocument doc(file.size() * 1.33);

            if (file) {
                DeserializationError error = deserializeJson(doc, file);
                if (error)  {
                    log_error("Failed to deserialize file, may be corrupted\n %s\n", error.c_str());
                    file.close();
                    return false;
                }
                file.close();
            }
            else
                return false;

            if (doc[label]["value"])
                doc[label]["value"] = val;
            else if (doc[label]["selected"])
                doc[label]["selected"] = val;
            else
                doc[label] = val;
            return true;
        }

};

#endif
