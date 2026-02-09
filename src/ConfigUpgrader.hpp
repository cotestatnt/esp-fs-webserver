#ifndef CONFIG_UPGRADER_HPP
#define CONFIG_UPGRADER_HPP

#include <FS.h>
#include "SerialLog.h"

extern "C" {
#include "json/cJSON.h"
}

/**
 * @brief ConfigUpgrader handles migration from v1 (flat JSON) to v2 (hierarchical JSON)
 * Uses cJSON directly for reliable key iteration
 */
class ConfigUpgrader
{
public:
    ConfigUpgrader(fs::FS* filesystem, const char* configFile)
        : m_filesystem(filesystem), m_configFile(configFile) {}

    ~ConfigUpgrader() {}

    /**
     * @brief Check if upgrade is needed and perform it
     * @param outputFile Optional: save upgraded config to different file
     * @return true if upgrade was performed or file is already v2, false on error
     */
    bool upgrade(const char* outputFile = nullptr) {
        if (m_filesystem == nullptr || m_configFile == nullptr) {
            log_error("ConfigUpgrader: Invalid filesystem or config file");
            return false;
        }

        if (!m_filesystem->exists(m_configFile)) {
            log_debug("ConfigUpgrader: Config file does not exist, no upgrade needed");
            return true;
        }

        // Read config file
        File file = m_filesystem->open(m_configFile, "r");
        if (!file) {
            log_error("ConfigUpgrader: Failed to open config file");
            return false;
        }

        String content = file.readString();
        file.close();

        // Parse JSON with cJSON
        cJSON* oldJsonRoot = cJSON_Parse(content.c_str());
        if (!oldJsonRoot) {
            log_error("ConfigUpgrader: Failed to parse config JSON");
            return false;
        }

        // Check version
        cJSON* versionItem = cJSON_GetObjectItem(oldJsonRoot, "_version");
        if (versionItem && versionItem->valuestring && String(versionItem->valuestring).equals("2.0")) {
            log_debug("ConfigUpgrader: Config is already v2.0, no upgrade needed");
            cJSON_Delete(oldJsonRoot);
            return true;
        }

        log_info("ConfigUpgrader: Upgrading config from v1 to v2.0");
        
        // Perform upgrade
        String upgraded = upgradeFromV1(oldJsonRoot);
        cJSON_Delete(oldJsonRoot);

        if (upgraded.isEmpty()) {
            log_error("ConfigUpgrader: Upgrade failed");
            return false;
        }

        // Determine output file
        const char* targetFile = (outputFile != nullptr) ? outputFile : m_configFile;

        // Write upgraded config
        file = m_filesystem->open(targetFile, "w");
        if (!file) {
            log_error("ConfigUpgrader: Failed to open config file for writing");
            return false;
        }

        file.print(upgraded);
        file.close();
        
        log_info("ConfigUpgrader: Config upgraded and saved to %s", targetFile);
        return true;
    }

private:
    fs::FS* m_filesystem = nullptr;
    const char* m_configFile = nullptr;

    /**
     * @brief Generate valid ID from label
     */
    String generateId(const String& label) {
        String id = label;
        id.toLowerCase();
        id.replace(" ", "-");
        
        String cleaned;
        for (int i = 0; i < id.length(); i++) {
            char c = id[i];
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_') {
                cleaned += c;
            }
        }
        
        return cleaned.isEmpty() ? String("option-") : cleaned;
    }

    /**
     * @brief Escape special characters for JSON
     */
    String escapeJson(const String& input) {
        String result;
        for (int i = 0; i < input.length(); i++) {
            char c = input[i];
            switch (c) {
                case '\"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (c < 32) {
                        // Skip control characters
                    } else {
                        result += c;
                    }
            }
        }
        return result;
    }

    /**
     * @brief Upgrade JSON from v1 flat format to v2 hierarchical format
     */
    String upgradeFromV1(cJSON* oldRoot) {
        String result = "{\n";
        
        // Add version
        result += "  \"_version\": \"2.0\",\n";

        // Extract metadata
        result += "  \"_meta\": {\n";
        
        String pageTitle = "Configuration";
        String logoPath = "";
        double port = 80;
        String host = "myserver";

        cJSON* item = nullptr;
        if ((item = cJSON_GetObjectItem(oldRoot, "page-title")) && item->valuestring) {
            pageTitle = item->valuestring;
        }
        if ((item = cJSON_GetObjectItem(oldRoot, "img-logo")) && item->valuestring) {
            logoPath = item->valuestring;
        }
        if ((item = cJSON_GetObjectItem(oldRoot, "port")) && item->type == cJSON_Number) {
            port = item->valuedouble;
        }
        if ((item = cJSON_GetObjectItem(oldRoot, "host")) && item->valuestring) {
            host = item->valuestring;
        }

        result += "    \"app_title\": \"" + escapeJson(pageTitle) + "\",\n";
        if (!logoPath.isEmpty()) {
            result += "    \"logo\": \"" + escapeJson(logoPath) + "\",\n";
        }
        result += "    \"port\": " + String((long)port) + ",\n";
        result += "    \"host\": \"" + escapeJson(host) + "\"\n";
        result += "  },\n";

        // Add state (empty)
        result += "  \"_state\": {},\n";

        // Extract assets (CSS, JS, and HTML)
        std::vector<String> cssList;
        std::vector<String> jsList;
        std::vector<String> htmlList;
        
        for (cJSON* assetItem = oldRoot->child; assetItem; assetItem = assetItem->next) {
            String key = assetItem->string ? String(assetItem->string) : String("");
            
            if (key.indexOf("raw-css-") == 0 && assetItem->valuestring) {
                cssList.push_back(assetItem->valuestring);
            } else if ((key.indexOf("raw-javascript-") == 0 || key.indexOf("raw-js-") == 0) && assetItem->valuestring) {
                jsList.push_back(assetItem->valuestring);
            } else if (key.indexOf("raw-html-") == 0 && assetItem->valuestring) {
                htmlList.push_back(assetItem->valuestring);
            }
        }

        // Add assets section
        result += "  \"_assets\": {\n";
        result += "    \"css\": [";
        for (size_t i = 0; i < cssList.size(); i++) {
            result += "\"" + escapeJson(cssList[i]) + "\"";
            if (i < cssList.size() - 1) result += ", ";
        }
        result += "],\n";
        
        result += "    \"js\": [";
        for (size_t i = 0; i < jsList.size(); i++) {
            result += "\"" + escapeJson(jsList[i]) + "\"";
            if (i < jsList.size() - 1) result += ", ";
        }
        result += "],\n";
        
        result += "    \"html\": [";
        for (size_t i = 0; i < htmlList.size(); i++) {
            result += "\"" + escapeJson(htmlList[i]) + "\"";
            if (i < htmlList.size() - 1) result += ", ";
        }
        result += "]\n";
        result += "  },\n";

        // Add sections
        result += "  \"sections\": [\n";
        result += upgradeToSections(oldRoot);
        result += "  ]\n";
        result += "}\n";

        return result;
    }

    /**
     * @brief Convert v1 elements to v2 sections
     */
    String upgradeToSections(cJSON* oldRoot) {
        String result;
        String currentSectionId = "general-options";
        String currentSectionTitle = "Options";
        std::vector<String> currentElements;
        bool firstSection = true;

        // Iterate through all top-level keys
        for (cJSON* item = oldRoot->child; item; item = item->next) {
            String key = item->string ? String(item->string) : String("");
            if (key.isEmpty()) continue;

            // Skip system keys
            if (key.equals("_version") || key.equals("_meta") || 
                key.equals("_state") || key.equals("_assets") ||
                key.equals("page-title") || key.equals("img-logo") ||
                key.equals("port") || key.equals("host")) {
                continue;
            }

            // Skip raw-css, raw-javascript, and raw-html (they all go to _assets)
            if ((key.indexOf("raw-css-") == 0 || key.indexOf("raw-javascript-") == 0 || 
                 key.indexOf("raw-js-") == 0 || key.indexOf("raw-html-") == 0) && 
                key.indexOf("raw-html-") != 0) {
                continue;
            }
            
            // Skip raw-html (goes to assets)
            if (key.indexOf("raw-html-") == 0) {
                continue;
            }

            // Handle section titles
            if (key.indexOf("param-box") == 0) {
                // Save current section if has elements
                if (currentElements.size() > 0) {
                    if (!firstSection) result += ",\n";
                    result += buildSection(currentSectionId, currentSectionTitle, currentElements);
                    firstSection = false;
                }

                // Start new section
                String sectionTitle = item->valuestring ? String(item->valuestring) : String("Section");
                currentSectionId = generateId(sectionTitle);
                currentSectionTitle = sectionTitle;
                currentElements.clear();
                continue;
            }

            // Skip image and name keys
            if (key.indexOf("img-") == 0 || key.indexOf("name-") == 0) {
                continue;
            }

            // Convert option
            String elemJson = convertV1Option(key, item);
            if (!elemJson.isEmpty()) {
                currentElements.push_back(elemJson);
            }
        }

        // Save last section
        if (currentElements.size() > 0) {
            if (!firstSection) result += ",\n";
            result += buildSection(currentSectionId, currentSectionTitle, currentElements);
            result += "\n";
        }

        return result;
    }

    /**
     * @brief Build a section JSON block
     */
    String buildSection(const String& id, const String& title, const std::vector<String>& elements) {
        String result = "    {\n";
        result += "      \"title\": \"" + escapeJson(title) + "\",\n";
        result += "      \"elements\": [\n";
        for (size_t i = 0; i < elements.size(); i++) {
            result += elements[i];
            if (i < elements.size() - 1) result += ",";
            result += "\n";
        }
        result += "      ]\n";
        result += "    }";
        return result;
    }

    /**
     * @brief Convert a single v1 option to v2 element
     */
    String convertV1Option(const String& key, cJSON* item) {
        String result = "        {\n";
        result += "          \"label\": \"" + escapeJson(key) + "\",\n";

        // Check if it's an object with metadata
        if (item->type == cJSON_Object) {
            cJSON* typeItem = cJSON_GetObjectItem(item, "type");
            String typeStr = (typeItem && typeItem->valuestring) ? String(typeItem->valuestring) : String("");

            if (typeStr.equals("slider")) {
                result += "          \"type\": \"slider\",\n";
                
                double value = 0, min = 0, max = 100, step = 1;
                cJSON* v = cJSON_GetObjectItem(item, "value");
                if (v && v->type == cJSON_Number) value = v->valuedouble;
                v = cJSON_GetObjectItem(item, "min");
                if (v && v->type == cJSON_Number) min = v->valuedouble;
                v = cJSON_GetObjectItem(item, "max");
                if (v && v->type == cJSON_Number) max = v->valuedouble;
                v = cJSON_GetObjectItem(item, "step");
                if (v && v->type == cJSON_Number) step = v->valuedouble;
                
                result += "          \"value\": " + String(value) + ",\n";
                result += "          \"min\": " + String(min) + ",\n";
                result += "          \"max\": " + String(max) + ",\n";
                result += "          \"step\": " + String(step) + "\n";
            } 
            else if (typeStr.equals("number")) {
                result += "          \"type\": \"number\",\n";
                
                double value = 0, min = -3.4e38, max = 3.4e38, step = 1;
                cJSON* v = cJSON_GetObjectItem(item, "value");
                if (v && v->type == cJSON_Number) value = v->valuedouble;
                v = cJSON_GetObjectItem(item, "min");
                if (v && v->type == cJSON_Number) min = v->valuedouble;
                v = cJSON_GetObjectItem(item, "max");
                if (v && v->type == cJSON_Number) max = v->valuedouble;
                v = cJSON_GetObjectItem(item, "step");
                if (v && v->type == cJSON_Number) step = v->valuedouble;
                
                result += "          \"value\": " + String(value) + ",\n";
                result += "          \"min\": " + String(min) + ",\n";
                result += "          \"max\": " + String(max) + ",\n";
                result += "          \"step\": " + String(step) + "\n";
            }
            else if (cJSON_GetObjectItem(item, "selected") != nullptr) {
                // Dropdown
                result += "          \"type\": \"select\",\n";
                
                cJSON* selected = cJSON_GetObjectItem(item, "selected");
                if (selected && selected->valuestring) {
                    result += "          \"value\": \"" + escapeJson(selected->valuestring) + "\",\n";
                }
                
                // Extract options array
                result += "          \"options\": [";
                cJSON* valuesArray = cJSON_GetObjectItem(item, "values");
                if (valuesArray && valuesArray->type == cJSON_Array) {
                    bool firstOption = true;
                    for (cJSON* optItem = valuesArray->child; optItem; optItem = optItem->next) {
                        if (optItem->valuestring) {
                            if (!firstOption) result += ", ";
                            result += "\"" + escapeJson(optItem->valuestring) + "\"";
                            firstOption = false;
                        }
                    }
                }
                result += "]\n";
            }
            else {
                result += "          \"type\": \"text\",\n";
                cJSON* val = cJSON_GetObjectItem(item, "value");
                if (val && val->valuestring) {
                    result += "          \"value\": \"" + escapeJson(val->valuestring) + "\"\n";
                }
            }
        } 
        else {
            // Primitive value - infer type
            if (item->type == cJSON_True || item->type == cJSON_False) {
                result += "          \"type\": \"boolean\",\n";
                result += "          \"value\": " + String(item->type == cJSON_True ? "true" : "false") + "\n";
            } 
            else if (item->type == cJSON_Number) {
                result += "          \"type\": \"number\",\n";
                result += "          \"value\": " + String(item->valuedouble) + "\n";
            } 
            else if (item->type == cJSON_String && item->valuestring) {
                result += "          \"type\": \"text\",\n";
                result += "          \"value\": \"" + escapeJson(item->valuestring) + "\"\n";
            }
            else {
                result += "          \"type\": \"text\",\n";
                result += "          \"value\": \"\"\n";
            }
        }

        result += "        }";
        return result;
    }
};

#endif // CONFIG_UPGRADER_HPP
