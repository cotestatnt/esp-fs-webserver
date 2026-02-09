#pragma once
#include <Arduino.h>
#include <vector>
#include <stdint.h>
extern "C" {
#include "json/cJSON.h"
}

namespace CJSON {
class Json {
public:
    Json();
    ~Json();

    bool parse(const String& text);
    String serialize(bool pretty=false) const;

    // Construction helpers for nested structures
    // Initialize the root as an empty object or array
    bool createObject();
    bool createArray();
    // Append a child to the root array
    bool add(const Json& child);
    // Set a nested child under a key in the root object
    bool set(const String& key, const Json& child);

    bool hasObject(const String& key) const;
    void ensureObject(const String& key);

    // Top-level key helpers
    bool hasKey(const String& key) const;
    bool setString(const String& key, const String& value);
    bool setNumber(const String& key, double value);
    bool setBool(const String& key, bool value);
    bool setArray(const String& key, const std::vector<String>& values);
    bool getString(const String& key, String& out) const;
    bool getBool(const String& key, bool& out) const;
    bool getNumber(const String& key, double& out) const;

    // Object-scoped key helpers
    bool hasKey(const String& obj, const String& key) const;
    bool setString(const String& obj, const String& key, const String& value);
    bool setNumber(const String& obj, const String& key, double value);
    bool setBool(const String& obj, const String& key, bool value);
    bool setArray(const String& obj, const String& key, const std::vector<String>& values);
    bool getString(const String& obj, const String& key, String& out) const;
    bool getBool(const String& obj, const String& key, bool& out) const;
    bool getNumber(const String& obj, const String& key, double& out) const;

    // Low-level accessor: expose underlying cJSON root for advanced operations
    // (e.g. iterating arrays/objects in higher-level helpers).
    // Caller must NOT free or modify the returned pointer directly.
    cJSON* getRoot() { return root; }
    const cJSON* getRoot() const { return root; }

private:
    cJSON* root;
};
}
