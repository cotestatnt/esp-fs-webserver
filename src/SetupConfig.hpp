#ifndef CONFIGURATOR_HPP
#define CONFIGURATOR_HPP
#include <type_traits>
#include <FS.h>

#include "Json.h"
#include "SerialLog.h"
#include "ConfigUpgrader.hpp"

#define MIN_F -3.4028235E+38
#define MAX_F 3.4028235E+38

// Public dropdown definition type, available only when /setup is enabled
namespace SetupConfig {
    struct DropdownList {
        const char* label;                 // JSON key / UI label id
        const char* const* values;         // Static array of values (null-terminated strings)
        size_t size;                       // Number of items in values
        size_t selectedIndex;              // Currently selected item index
    };

    struct Slider {
        const char* label;                 // JSON key / UI label id
        double min;                        // Minimum value
        double max;                        // Maximum value
        double step;                       // Step increment
        double value;                      // Current value
    };
}

class SetupConfigurator
{
    protected:
        uint8_t numOptions = 0;
        fs::FS* m_filesystem = nullptr;
        CJSON::Json* m_doc = nullptr;
        CJSON::Json* m_savedDoc = nullptr;  // Temporary storage for saved file values

        // Builders for v2 hierarchical schema (sections / elements)
        CJSON::Json m_sectionsArray;        // Root array of sections for current session
        CJSON::Json m_currentSection;       // Currently open section
        CJSON::Json m_currentElements;      // Elements array for current section
        bool m_hasCurrentSection = false;   // True if a section is open

        // (previously grouping settings were global; now per-option)

        uint16_t& m_port;         
        String& m_host;
        bool m_opened = false;

        // --------- Helpers for v2 hierarchical schema ---------

        // Ensure there is an open section to which options can be added
        void ensureActiveSection() {
            if (m_hasCurrentSection) return;
            // Default section when user doesn't call addOptionBox explicitly
            m_currentSection.createObject();
            m_currentSection.setString("title", "General Options");
            m_currentElements.createArray();
            m_hasCurrentSection = true;
        }

        // Start a new named section (used by addOptionBox)
        void startNewSection(const char* title) {
            // Finalize previous section first
            if (m_hasCurrentSection) {
                m_currentSection.set("elements", m_currentElements);
                m_sectionsArray.add(m_currentSection);
                m_currentSection.createObject();
                m_currentElements.createArray();
            }
            String t = String(title);
            m_currentSection.setString("title", t);
            m_hasCurrentSection = true;
        }

        // Finalize any open section and attach sections array to root document
        void finalizeSectionsToRoot() {
            if (m_doc == nullptr) return;
            if (m_hasCurrentSection) {
                m_currentSection.set("elements", m_currentElements);
                m_sectionsArray.add(m_currentSection);
                m_hasCurrentSection = false;
            }
            m_doc->set("sections", m_sectionsArray);
        }

        bool isOpened() {
            return m_opened;
        }

        bool openConfiguration() {
            if (checkConfigFile()) {
                // Check if config needs upgrade from v1 to v2
                upgradeConfigIfNeeded();
                
                // Read existing file into m_savedDoc (background copy for value lookup)
                if (m_filesystem->exists(ESP_FS_WS_CONFIG_FILE)) {
                    File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "r");
                    if (file) {
                        String content = file.readString();
                        file.close();
                        
                        m_savedDoc = new CJSON::Json();
                        if (!m_savedDoc->parse(content)) {
                            log_error("Failed to parse existing configuration");
                            delete m_savedDoc;
                            m_savedDoc = nullptr;
                            // Don't continue if parsing fails
                            return false;
                        }
                    }
                }
                
                // Create fresh v2 root document for this session
                m_doc = new CJSON::Json();
                m_doc->createObject();

                // Version tag
                m_doc->setString("_version", "2.0");

                // Metadata section
                m_doc->ensureObject("_meta");
                String appTitle = "Custom HTML Web Server";
                String logoPath = String(ESP_FS_WS_CONFIG_FOLDER) + "/logo.svg";
                if (m_savedDoc) {
                    String tmp;
                    if (m_savedDoc->getString("_meta", "app_title", tmp)) appTitle = tmp;
                    if (m_savedDoc->getString("_meta", "logo", tmp)) logoPath = tmp;
                }
                m_doc->setString("_meta", "app_title", appTitle);
                m_doc->setString("_meta", "logo", logoPath);
                m_doc->setNumber("_meta", "port", static_cast<double>(m_port));
                m_doc->setString("_meta", "host", m_host);

                // State section (object; can be extended externally if needed)
                m_doc->ensureObject("_state");

                // Assets section: carry over existing lists when present
                m_doc->ensureObject("_assets");
                std::vector<String> cssList;
                std::vector<String> jsList;
                std::vector<String> htmlList;
                if (m_savedDoc) {
                    auto copyArray = [](CJSON::Json* src, const char* obj, const char* key, std::vector<String>& out) {
                        if (!src) return;
                        const cJSON* root = src->getRoot();
                        if (!root) return;
                        const cJSON* scope = cJSON_GetObjectItemCaseSensitive(root, obj);
                        if (!scope || !cJSON_IsObject(scope)) return;
                        const cJSON* arr = cJSON_GetObjectItemCaseSensitive(scope, key);
                        if (!arr || !cJSON_IsArray(arr)) return;
                        for (const cJSON* it = arr->child; it; it = it->next) {
                            if (cJSON_IsString(it) && it->valuestring) {
                                out.emplace_back(String(it->valuestring));
                            }
                        }
                    };
                    copyArray(m_savedDoc, "_assets", "css", cssList);
                    // Prefer new key "js" but also accept legacy "javascript" for backward compatibility
                    copyArray(m_savedDoc, "_assets", "js", jsList);
                    if (jsList.empty()) {
                        copyArray(m_savedDoc, "_assets", "javascript", jsList);
                    }
                    copyArray(m_savedDoc, "_assets", "html", htmlList);
                }
                std::vector<String> empty;
                m_doc->setArray("_assets", "css", cssList.empty() ? empty : cssList);
                m_doc->setArray("_assets", "js", jsList.empty() ? empty : jsList);
                m_doc->setArray("_assets", "html", htmlList.empty() ? empty : htmlList);

                // Initialize sections builder (will be attached to m_doc on close)
                m_sectionsArray.createArray();
                m_currentSection.createObject();
                m_currentElements.createArray();
                m_hasCurrentSection = false;
                
                m_opened = true;
                return true;
            }
            return false;
        }

        /**
         * @brief Check if config needs upgrade and perform it if necessary
         * Uses ConfigUpgrader to migrate from v1 to v2 format
         */
        void upgradeConfigIfNeeded() {
            if (m_filesystem == nullptr) return;
            
            ConfigUpgrader upgrader(m_filesystem, ESP_FS_WS_CONFIG_FILE);
            if (!upgrader.upgrade()) {
                log_debug("Config upgrade check completed");
            }
        }

        // If config file or folder doesn't exist, create them. If config file exists, do nothing.
        // Returns true if config file is ready for use (exists or created successfully), false on failure.
        // Some keys might be necessary for the setup page to work properly, so this function ensures that 
        // the config file exists and is initialized with a valid JSON object if it was missing.
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
                // Create pure v2 config (no legacy flat keys)
                CJSON::Json initDoc;
                initDoc.createObject();
                initDoc.setString("_version", "2.0");

                // Metadata
                initDoc.ensureObject("_meta");
                initDoc.setString("_meta", "app_title", String("Custom HTML Web Server"));
                initDoc.setString("_meta", "logo", String(ESP_FS_WS_CONFIG_FOLDER) + "/logo.svg");
                initDoc.setNumber("_meta", "port", static_cast<double>(m_port));
                initDoc.setString("_meta", "host", m_host);

                // Empty _state object
                initDoc.ensureObject("_state");

                // _assets with empty lists
                initDoc.ensureObject("_assets");
                {
                    std::vector<String> empty;
                    initDoc.setArray("_assets", "css", empty);
                    initDoc.setArray("_assets", "js", empty);
                    initDoc.setArray("_assets", "html", empty);
                }

                // Empty sections array (as real array node)
                {
                    CJSON::Json sections;
                    sections.createArray();
                    initDoc.set("sections", sections);
                }

                String json = initDoc.serialize(true);
                file.print(json);
                file.close();
            }
            log_debug("Config file %s OK", ESP_FS_WS_CONFIG_FILE);
            return true;
        }

    public:
        friend class FSWebServer;
        SetupConfigurator(fs::FS *fs, uint16_t& port, String& host) 
            : m_filesystem(fs), m_port(port), m_host(host) { ; }

        bool closeConfiguration() {            

            // If no options were added in this session, skip writing to avoid overwriting
            if (numOptions == 0) {
                log_debug("No options added; skipping config write");
                if (m_doc) { delete m_doc; m_doc = nullptr; }
                if (m_savedDoc) { delete m_savedDoc; m_savedDoc = nullptr; }
                return true;
            }
         
            // Finalize sections into root _v2 schema
            finalizeSectionsToRoot();

            // Write configuration to file only if content has changed
            // Serialize the new content
            String newContent = m_doc->serialize(true);
            
            // Read existing file content
            String oldContent;
            if (m_filesystem->exists(ESP_FS_WS_CONFIG_FILE)) {
                File readFile = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "r");
                if (readFile) {
                    oldContent = readFile.readString();
                    readFile.close();
                }
            }
            
            // Write only if content is different
            if (oldContent != newContent) {
                File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "w");
                if (file) {
                    file.print(newContent);
                    file.close();
                    log_debug("Config file written (content changed)");
                } 
                else {
                    log_error("Error opening config file for write");
                    delete (m_doc);
                    m_doc = nullptr;
                    if (m_savedDoc) { 
                        delete (m_savedDoc); 
                        m_savedDoc = nullptr; 
                    }
                    m_opened = false;
                    return false;
                }
            } 
            else {
                log_debug("Config file unchanged, skipping write");
            }
            
            delete (m_doc);
            m_doc = nullptr;
            if (m_savedDoc) { 
                delete (m_savedDoc); 
                m_savedDoc = nullptr; 
            }

            m_opened = false;
            numOptions = 0;
            return true;
        }

        // Save logo image from binary data (uint8_t array)
        // Supports: PNG, JPEG, GIF, SVG (plain or gzipped)
        // Automatically detects gzip compression (magic bytes 0x1f 0x8b)
        void setSetupPageLogo(const uint8_t* imageData, size_t imageSize, const char* mimeType = "image/png", bool overwrite = false) {
            // Ensure configuration document is open so we can update _meta
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                    return;
                }
            }

            // Determine file extension from MIME type
            String extension = ".png";
            if (strcmp(mimeType, "image/jpeg") == 0 || strcmp(mimeType, "image/jpg") == 0) {
                extension = ".jpg";
            } else if (strcmp(mimeType, "image/gif") == 0) {
                extension = ".gif";
            } else if (strcmp(mimeType, "image/svg+xml") == 0) {
                extension = ".svg";
            }
            
            String filename = ESP_FS_WS_CONFIG_FOLDER;
            filename += "/logo";
            filename += extension;
            
            // Auto-detect gzip compression by checking magic bytes
            if (imageSize >= 2 && imageData[0] == 0x1f && imageData[1] == 0x8b) {
                filename += ".gz";
            }
            
            // Save binary logo file
            if (optionToFileBinary(filename.c_str(), imageData, imageSize, overwrite)) {
                // Store path in _meta.logo instead of creating an "img-logo" element
                m_doc->ensureObject("_meta");
                m_doc->setString("_meta", "logo", filename);
                // Mark configuration as changed so closeConfiguration() will persist it
                numOptions++;
            }
        }

        // Overload for string literals (e.g., SVG text)
        void setSetupPageLogo(const char* svgText, bool overwrite = false) {
            setSetupPageLogo((const uint8_t*)svgText, strlen(svgText), "image/svg+xml", overwrite);
        }

        // Set page title as metadata (_meta.app_title) instead of a normal option element
        void setSetupPageTitle(const char* title) {
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                    return;
                }
            }

            m_doc->ensureObject("_meta");
            m_doc->setString("_meta", "app_title", String(title));
            // Mark configuration as changed so closeConfiguration() will persist it
            numOptions++;
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

        // Save binary data to file (e.g., pre-compressed gzip data)
        bool optionToFileBinary(const char* filename, const uint8_t* data, size_t len, bool overWrite) {
            if (m_filesystem->exists(filename) && !overWrite) {
                return true;
            }
            File file = m_filesystem->open(filename, "w");
            if (file) {
                size_t written = file.write(data, len);
                file.close();
                log_debug("Binary file %s saved (%d bytes)", filename, written);
                return written == len;
            }
            log_debug("Error writing binary file %s", filename);
            return false;
        }

        // Save binary data to file (for pre-compressed gzip data)
        bool optionToFileGzip(const char* filename, const uint8_t* data, size_t len, bool overWrite) {
            if (m_filesystem->exists(filename) && !overWrite) {
                return true;
            }
            File file = m_filesystem->open(filename, "w");
            if (file) {
                size_t written = file.write(data, len);
                file.close();
                log_debug("Binary file %s saved (%d bytes)", filename, written);
                return written == len;
            }
            log_debug("Error writing binary file %s", filename);
            return false;
        }

        void addSource(const String& source, const String& id, const String& extension, bool overWrite) {
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                }
            }

            String path = ESP_FS_WS_CONFIG_FOLDER;
            path += "/";
            path += id;
            path += extension;

            bool isCss = extension.equals(".css");
            bool isJs = extension.equals(".js");

            if (optionToFile(path.c_str(), source.c_str(), overWrite)) {
                // Register asset path inside _assets.{css,js,html} instead of flat raw-* keys
                m_doc->ensureObject("_assets");
                cJSON* root = m_doc->getRoot();
                if (!root) return;
                cJSON* assets = cJSON_GetObjectItemCaseSensitive(root, "_assets");
                if (!assets || !cJSON_IsObject(assets)) return;

                const char* arrayKey = nullptr;
                if (isCss) arrayKey = "css";
                else if (isJs) arrayKey = "js";

                if (arrayKey) {
                    cJSON* arr = cJSON_GetObjectItemCaseSensitive(assets, arrayKey);
                    if (!arr || !cJSON_IsArray(arr)) {
                        arr = cJSON_CreateArray();
                        cJSON_AddItemToObject(assets, arrayKey, arr);
                    }

                    // Avoid duplicates
                    bool found = false;
                    for (cJSON* it = arr->child; it; it = it->next) {
                        if (cJSON_IsString(it) && it->valuestring && path.equals(String(it->valuestring))) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        cJSON_AddItemToArray(arr, cJSON_CreateString(path.c_str()));
                    }
                }
            }
            else {
                log_error("Source option not saved");
            }

        }

        void addHTML(const char* html, const char* id, bool overWrite) {
            String path = String(ESP_FS_WS_CONFIG_FOLDER) + "/" + id + ".htm";
            optionToFile(path.c_str(), html, overWrite);

            // Add HTML as an element in the current section
            CJSON::Json elem;
            elem.createObject();
            elem.setString("type", "html");
            elem.setString("label", "");
            elem.setString("value", path);
            
            m_currentElements.add(elem);
            numOptions++;
        }

        void addCSS(const char* css,  const char* id, bool overWrite) {
            String source = css;
            addSource(source, id, ".css", overWrite);
        }

        void addJavascript(const char* script,  const char* id, bool overWrite) {
            String source = script;
            addSource(source, id, ".js", overWrite);
        }

        /*
            Add a new dropdown input element
        */
        void addDropdownList(const char *label, const char** array, size_t size) {
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                }
            }

            ensureActiveSection();

            // Determine selected value: prefer saved, otherwise first item
            String selectedValue = (size > 0) ? String(array[0]) : String("");
            if (m_savedDoc) {
                const cJSON* root = m_savedDoc->getRoot();
                if (root) {
                    const cJSON* sections = cJSON_GetObjectItemCaseSensitive(root, "sections");
                    if (sections && cJSON_IsArray(sections)) {
                        const cJSON* sec = sections->child;
                        while (sec) {
                            const cJSON* elems = cJSON_GetObjectItemCaseSensitive(sec, "elements");
                            if (elems && cJSON_IsArray(elems)) {
                                const cJSON* el = elems->child;
                                while (el) {
                                    const cJSON* lbl = cJSON_GetObjectItemCaseSensitive(el, "label");
                                    if (lbl && cJSON_IsString(lbl) && lbl->valuestring && String(lbl->valuestring).equals(label)) {
                                        const cJSON* val = cJSON_GetObjectItemCaseSensitive(el, "value");
                                        if (val && cJSON_IsString(val) && val->valuestring) {
                                            selectedValue = String(val->valuestring);
                                        }
                                        break;
                                    }
                                    el = el->next;
                                }
                            }
                            sec = sec->next;
                        }
                    }
                }
            }

            CJSON::Json elem;
            elem.createObject();
            elem.setString("label", label);
            elem.setString("type", "select");
            elem.setString("value", selectedValue);
            std::vector<String> vals; vals.reserve(size);
            for (size_t i = 0; i < size; i++) vals.emplace_back(String(array[i]));
            elem.setArray("options", vals);

            m_currentElements.add(elem);
            numOptions++;
        }

        /*
            Add a new dropdown using a static definition that tracks current index
        */
        void addDropdownList(SetupConfig::DropdownList &def) {
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                }
            }

            const char* label = def.label;
            ensureActiveSection();

            // Determine selected value: prefer saved, otherwise provided index, otherwise first
            String selectedValue = (def.size > 0) ? String(def.values[(def.selectedIndex < def.size) ? def.selectedIndex : 0]) : String("");
            if (m_savedDoc) {
                const cJSON* root = m_savedDoc->getRoot();
                if (root) {
                    const cJSON* sections = cJSON_GetObjectItemCaseSensitive(root, "sections");
                    if (sections && cJSON_IsArray(sections)) {
                        const cJSON* sec = sections->child;
                        while (sec) {
                            const cJSON* elems = cJSON_GetObjectItemCaseSensitive(sec, "elements");
                            if (elems && cJSON_IsArray(elems)) {
                                const cJSON* el = elems->child;
                                while (el) {
                                    const cJSON* lbl = cJSON_GetObjectItemCaseSensitive(el, "label");
                                    if (lbl && cJSON_IsString(lbl) && lbl->valuestring && String(lbl->valuestring).equals(label)) {
                                        const cJSON* val = cJSON_GetObjectItemCaseSensitive(el, "value");
                                        if (val && cJSON_IsString(val) && val->valuestring) {
                                            selectedValue = String(val->valuestring);
                                        }
                                        break;
                                    }
                                    el = el->next;
                                }
                            }
                            sec = sec->next;
                        }
                    }
                }
            }

            CJSON::Json elem;
            elem.createObject();
            elem.setString("label", label);
            elem.setString("type", "select");
            elem.setString("value", selectedValue);
            std::vector<String> vals; vals.reserve(def.size);
            for (size_t i = 0; i < def.size; i++) { vals.emplace_back(String(def.values[i])); }
            elem.setArray("options", vals);

            // Update def.selectedIndex from selectedValue
            for (size_t i = 0; i < def.size; i++) {
                if (selectedValue.equals(String(def.values[i]))) {
                    def.selectedIndex = i;
                    break;
                }
            }

            m_currentElements.add(elem);
            numOptions++;
        }

        /*
            Update a dropdown definition's selectedIndex from persisted config
            Returns true if a matching value was found
        */
        bool getDropdownSelection(SetupConfig::DropdownList &def) {
            // Ensure we have a doc to read from
            if (m_doc == nullptr && !openConfiguration()) {
                log_error("Error! /setup configuration not possible");
                return false;
            }

            CJSON::Json* sourceDoc = (m_savedDoc != nullptr) ? m_savedDoc : m_doc;
            if (sourceDoc == nullptr) {
                log_error("No configuration document available for reading");
                return false;
            }

            const cJSON* root = sourceDoc->getRoot();
            if (!root) return false;
            const cJSON* sections = cJSON_GetObjectItemCaseSensitive(root, "sections");
            if (!sections || !cJSON_IsArray(sections)) return false;

            String sel;
            const cJSON* sec = sections->child;
            while (sec) {
                const cJSON* elems = cJSON_GetObjectItemCaseSensitive(sec, "elements");
                if (elems && cJSON_IsArray(elems)) {
                    const cJSON* el = elems->child;
                    while (el) {
                        const cJSON* lbl = cJSON_GetObjectItemCaseSensitive(el, "label");
                        if (lbl && cJSON_IsString(lbl) && lbl->valuestring && String(lbl->valuestring).equals(def.label)) {
                            const cJSON* val = cJSON_GetObjectItemCaseSensitive(el, "value");
                            if (val && cJSON_IsString(val) && val->valuestring) {
                                sel = String(val->valuestring);
                            }
                            break;
                        }
                        el = el->next;
                    }
                }
                sec = sec->next;
            }

            if (!sel.length()) return false;

            for (size_t i = 0; i < def.size; i++) {
                if (sel.equals(String(def.values[i]))) {
                    def.selectedIndex = i;
                    return true;
                }
            }
            return false;
        }

        /*
            Add a new slider using a static definition that tracks current value
        */
        void addSlider(SetupConfig::Slider &def) {
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                }
            }

            const char* label = def.label;
            ensureActiveSection();

            // Prefer saved value when available; else use def.value
            double current = def.value;
            if (m_savedDoc) {
                const cJSON* root = m_savedDoc->getRoot();
                if (root) {
                    const cJSON* sections = cJSON_GetObjectItemCaseSensitive(root, "sections");
                    if (sections && cJSON_IsArray(sections)) {
                        const cJSON* sec = sections->child;
                        while (sec) {
                            const cJSON* elems = cJSON_GetObjectItemCaseSensitive(sec, "elements");
                            if (elems && cJSON_IsArray(elems)) {
                                const cJSON* el = elems->child;
                                while (el) {
                                    const cJSON* lbl = cJSON_GetObjectItemCaseSensitive(el, "label");
                                    if (lbl && cJSON_IsString(lbl) && lbl->valuestring && String(lbl->valuestring).equals(label)) {
                                        const cJSON* val = cJSON_GetObjectItemCaseSensitive(el, "value");
                                        if (val && cJSON_IsNumber(val)) {
                                            current = val->valuedouble;
                                        }
                                        break;
                                    }
                                    el = el->next;
                                }
                            }
                            sec = sec->next;
                        }
                    }
                }
            }

            def.value = current;

            CJSON::Json elem;
            elem.createObject();
            elem.setString("label", label);
            elem.setString("type", "slider");
            elem.setNumber("value", current);
            elem.setNumber("min", def.min);
            elem.setNumber("max", def.max);
            elem.setNumber("step", def.step);

            m_currentElements.add(elem);
            numOptions++;
        }

        /*
            Read slider value into the provided struct from persisted config
            Returns true if a value was found
        */
        bool getSliderValue(SetupConfig::Slider &def) {
            if (m_doc == nullptr && !openConfiguration()) {
                log_error("Error! /setup configuration not possible");
                return false;
            }

            CJSON::Json* sourceDoc = (m_savedDoc != nullptr) ? m_savedDoc : m_doc;
            if (sourceDoc == nullptr) return false;

            const cJSON* root = sourceDoc->getRoot();
            if (!root) return false;
            const cJSON* sections = cJSON_GetObjectItemCaseSensitive(root, "sections");
            if (!sections || !cJSON_IsArray(sections)) return false;

            const cJSON* sec = sections->child;
            while (sec) {
                const cJSON* elems = cJSON_GetObjectItemCaseSensitive(sec, "elements");
                if (elems && cJSON_IsArray(elems)) {
                    const cJSON* el = elems->child;
                    while (el) {
                        const cJSON* lbl = cJSON_GetObjectItemCaseSensitive(el, "label");
                        if (lbl && cJSON_IsString(lbl) && lbl->valuestring && String(lbl->valuestring).equals(def.label)) {
                            const cJSON* val = cJSON_GetObjectItemCaseSensitive(el, "value");
                            if (val && cJSON_IsNumber(val)) {
                                def.value = val->valuedouble;
                                return true;
                            }
                            return false;
                        }
                        el = el->next;
                    }
                }
                sec = sec->next;
            }
            return false;
        }

        /*
            Add a new option box with custom label
        */
        void addOptionBox(const char* boxTitle) {
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                    return;
                }
            }
            startNewSection(boxTitle);
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
        // bool-specific overload with grouping flag (grouped last to avoid breaking existing code)
        void addOption(const char *label, bool val, bool hidden = false, bool grouped = true) {
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                }
            }

            ensureActiveSection();
            String lbl = label;
            bool valueFromSaved = false;
            // read saved as before
            auto readSavedBool = [&](bool& out) -> bool {
                if (!m_savedDoc) return false;
                const cJSON* root = m_savedDoc->getRoot();
                if (!root) return false;
                const cJSON* sections = cJSON_GetObjectItemCaseSensitive(root, "sections");
                if (!sections || !cJSON_IsArray(sections)) return false;
                const cJSON* sec = sections->child;
                while (sec) {
                    const cJSON* elems = cJSON_GetObjectItemCaseSensitive(sec, "elements");
                    if (elems && cJSON_IsArray(elems)) {
                        const cJSON* el = elems->child;
                        while (el) {
                            const cJSON* lblNode = cJSON_GetObjectItemCaseSensitive(el, "label");
                            if (lblNode && cJSON_IsString(lblNode) && lblNode->valuestring && String(lblNode->valuestring).equals(lbl)) {
                                const cJSON* v = cJSON_GetObjectItemCaseSensitive(el, "value");
                                if (v && cJSON_IsBool(v)) {
                                    out = cJSON_IsTrue(v);
                                    return true;
                                }
                                return false;
                            }
                            el = el->next;
                        }
                    }
                    sec = sec->next;
                }
                return false;
            };

            CJSON::Json elem;
            elem.createObject();
            elem.setString("label", lbl);

            bool current = val;
            if (readSavedBool(current)) valueFromSaved = true;
            elem.setString("type", "boolean");
            elem.setBool("value", current);

            if (!grouped) {
                elem.setBool("group", false);
            }
            if (hidden) {
                elem.setBool("hidden", true);
            }

            log_debug("Option \"%s\" using %s value", lbl.c_str(), valueFromSaved ? "saved" : "default");
            m_currentElements.add(elem);
            numOptions++;
        }

        // generic template for all other types
        template <typename T>
        void addOption(const char *label, T val, bool hidden = false,
                            double d_min = MIN_F, double d_max = MAX_F, double step = 1.0)
        {

            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                }
            }

            ensureActiveSection();

            String lbl = label;
            bool valueFromSaved = false;

            // Resolve current value: check saved v2 sections first
            auto readSavedNumber = [&](double& out) -> bool {
                if (!m_savedDoc) return false;
                const cJSON* root = m_savedDoc->getRoot();
                if (!root) return false;
                const cJSON* sections = cJSON_GetObjectItemCaseSensitive(root, "sections");
                if (!sections || !cJSON_IsArray(sections)) return false;
                const cJSON* sec = sections->child;
                while (sec) {
                    const cJSON* elems = cJSON_GetObjectItemCaseSensitive(sec, "elements");
                    if (elems && cJSON_IsArray(elems)) {
                        const cJSON* el = elems->child;
                        while (el) {
                            const cJSON* lblNode = cJSON_GetObjectItemCaseSensitive(el, "label");
                            if (lblNode && cJSON_IsString(lblNode) && lblNode->valuestring && String(lblNode->valuestring).equals(lbl)) {
                                const cJSON* v = cJSON_GetObjectItemCaseSensitive(el, "value");
                                if (v && cJSON_IsNumber(v)) {
                                    out = v->valuedouble;
                                    return true;
                                }
                                return false;
                            }
                            el = el->next;
                        }
                    }
                    sec = sec->next;
                }
                return false;
            };

            auto readSavedString = [&](String& out) -> bool {
                if (!m_savedDoc) return false;
                const cJSON* root = m_savedDoc->getRoot();
                if (!root) return false;
                const cJSON* sections = cJSON_GetObjectItemCaseSensitive(root, "sections");
                if (!sections || !cJSON_IsArray(sections)) return false;
                const cJSON* sec = sections->child;
                while (sec) {
                    const cJSON* elems = cJSON_GetObjectItemCaseSensitive(sec, "elements");
                    if (elems && cJSON_IsArray(elems)) {
                        const cJSON* el = elems->child;
                        while (el) {
                            const cJSON* lblNode = cJSON_GetObjectItemCaseSensitive(el, "label");
                            if (lblNode && cJSON_IsString(lblNode) && lblNode->valuestring && String(lblNode->valuestring).equals(lbl)) {
                                const cJSON* v = cJSON_GetObjectItemCaseSensitive(el, "value");
                                if (v && cJSON_IsString(v) && v->valuestring) {
                                    out = String(v->valuestring);
                                    return true;
                                }
                                return false;
                            }
                            el = el->next;
                        }
                    }
                    sec = sec->next;
                }
                return false;
            };

            CJSON::Json elem;
            elem.createObject();
            elem.setString("label", lbl);

            if constexpr (std::is_same<T, String>::value) {
                String current = val;
                if (readSavedString(current)) valueFromSaved = true;
                elem.setString("type", "text");
                elem.setString("value", current);
            } else if constexpr (std::is_same<T, const char*>::value || std::is_same<T, char*>::value) {
                String current = String(val);
                if (readSavedString(current)) valueFromSaved = true;
                elem.setString("type", "text");
                elem.setString("value", current);
            } else {
                double current = static_cast<double>(val);
                if (readSavedNumber(current)) valueFromSaved = true;
                elem.setString("type", "number");
                elem.setNumber("value", current);
                if (d_min != MIN_F) elem.setNumber("min", d_min);
                if (d_max != MAX_F) elem.setNumber("max", d_max);
                if (step != 1.0) elem.setNumber("step", step);
            }

            if (hidden) {
                elem.setBool("hidden", true);
            }

            log_debug("Option \"%s\" using %s value", lbl.c_str(), valueFromSaved ? "saved" : "default");
            m_currentElements.add(elem);
            numOptions++;
        }

        /*
            Get current value for a specific custom option (true on success)
            Reads from m_doc if open, or reloads from file if closed
        */
        template <typename T>
        bool getOptionValue(const char *label, T &var) {
            // If m_doc is nullptr, reload configuration from file
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                    return false;
                }
            }
            
            // Prefer persisted values when available; fall back to current session doc
            CJSON::Json* sourceDoc = (m_savedDoc != nullptr) ? m_savedDoc : m_doc;

            if (sourceDoc == nullptr) {
                log_error("No configuration document available for reading");
                return false;
            }

            const cJSON* root = sourceDoc->getRoot();
            if (!root) return false;

            // Special case: port/host are stored in _meta
            if constexpr (!std::is_same<T, String>::value && !std::is_same<T, const char*>::value && !std::is_same<T, char*>::value) {
                if (strcmp(label, "port") == 0) {
                    const cJSON* meta = cJSON_GetObjectItemCaseSensitive(root, "_meta");
                    const cJSON* p = meta ? cJSON_GetObjectItemCaseSensitive(meta, "port") : nullptr;
                    if (p && cJSON_IsNumber(p)) {
                        var = static_cast<T>(p->valuedouble);
                        return true;
                    }
                }
            }

            const cJSON* sections = cJSON_GetObjectItemCaseSensitive(root, "sections");
            if (!sections || !cJSON_IsArray(sections)) return false;

            const cJSON* sec = sections->child;
            while (sec) {
                const cJSON* elems = cJSON_GetObjectItemCaseSensitive(sec, "elements");
                if (elems && cJSON_IsArray(elems)) {
                    const cJSON* el = elems->child;
                    while (el) {
                        const cJSON* lblNode = cJSON_GetObjectItemCaseSensitive(el, "label");
                        if (lblNode && cJSON_IsString(lblNode) && lblNode->valuestring && String(lblNode->valuestring).equals(label)) {
                            const cJSON* valNode = cJSON_GetObjectItemCaseSensitive(el, "value");
                            if constexpr (std::is_same<T, String>::value) {
                                if (valNode && cJSON_IsString(valNode) && valNode->valuestring) {
                                    var = String(valNode->valuestring);
                                    return true;
                                }
                            } else if constexpr (std::is_same<T, const char*>::value || std::is_same<T, char*>::value) {
                                if (valNode && cJSON_IsString(valNode) && valNode->valuestring) {
                                    static String tmp; // Note: lifetime tied to process; acceptable for config reads
                                    tmp = String(valNode->valuestring);
                                    var = tmp.c_str();
                                    return true;
                                }
                            } else if constexpr (std::is_same<T, bool>::value) {
                                if (valNode && cJSON_IsBool(valNode)) {
                                    var = cJSON_IsTrue(valNode);
                                    return true;
                                }
                            } else {
                                if (valNode && cJSON_IsNumber(valNode)) {
                                    var = static_cast<T>(valNode->valuedouble);
                                    return true;
                                }
                            }
                            return false;
                        }
                        el = el->next;
                    }
                }
                sec = sec->next;
            }
            return false;
        }

        template <typename T>
        bool saveOptionValue(const char *label, T val) {
            if (m_doc == nullptr) {
                if (!openConfiguration()) {
                    log_error("Error! /setup configuration not possible");
                    return false;
                }
            }

            // Ensure sections are attached before modifying (so we always work on the same tree)
            finalizeSectionsToRoot();

            cJSON* root = m_doc->getRoot();
            if (!root) return false;
            cJSON* sections = cJSON_GetObjectItemCaseSensitive(root, "sections");
            if (!sections || !cJSON_IsArray(sections)) return false;

            cJSON* targetElement = nullptr;
            for (cJSON* sec = sections->child; sec; sec = sec->next) {
                cJSON* elems = cJSON_GetObjectItemCaseSensitive(sec, "elements");
                if (!elems || !cJSON_IsArray(elems)) continue;
                for (cJSON* el = elems->child; el; el = el->next) {
                    cJSON* lblNode = cJSON_GetObjectItemCaseSensitive(el, "label");
                    if (lblNode && cJSON_IsString(lblNode) && lblNode->valuestring && String(lblNode->valuestring).equals(label)) {
                        targetElement = el;
                        break;
                    }
                }
                if (targetElement) break;
            }

            if (!targetElement) return false;

            // Replace or create "value" field inside target element
            cJSON_DeleteItemFromObjectCaseSensitive(targetElement, "value");

            if constexpr (std::is_same<T, String>::value) {
                cJSON_AddItemToObject(targetElement, "value", cJSON_CreateString(val.c_str()));
            } else if constexpr (std::is_same<T, const char*>::value || std::is_same<T, char*>::value) {
                cJSON_AddItemToObject(targetElement, "value", cJSON_CreateString(String(val).c_str()));
            } else if constexpr (std::is_same<T, bool>::value) {
                cJSON_AddItemToObject(targetElement, "value", cJSON_CreateBool(val));
            } else {
                cJSON_AddItemToObject(targetElement, "value", cJSON_CreateNumber(static_cast<double>(val)));
            }
            return true;
        }

};

#endif