#include "Json.h"
#include <cstring>
#include <stdlib.h>

using namespace CJSON;

Json::Json() : root(nullptr) {}
Json::~Json()
{
    if (root)
        cJSON_Delete(root);
}

bool Json::parse(const String &text)
{
    if (root)
        cJSON_Delete(root);
    root = cJSON_Parse(text.c_str());
    return root != nullptr;
}

static void jsonEscapeString(const char* in, String& out) {
    if (!in) { out += ""; return; }
    out.reserve(out.length() + strlen(in) + 4);
    for (const char* p = in; *p; ++p) {
        char c = *p;
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if ((unsigned char)c < 0x20) {
                    // Control chars -> skip or encode minimally
                    // Minimal approach: skip
                } else {
                    out += c;
                }
        }
    }
}

static void serializeNode(const cJSON* item, String& out, bool pretty, int indent);

static void addIndent(String& out, int indent) {
    for (int i = 0; i < indent; i++) out += "  ";
}

static void serializeArray(const cJSON* array, String& out, bool pretty, int indent) {
    out.reserve(out.length() + 64);
    out += '[';
    const cJSON* child = array->child;
    bool first = true;
    if (pretty && child) out += '\n';
    
    while (child) {
        if (!first) {
            out += ',';
            if (pretty) out += '\n';
        }
        first = false;
        if (pretty) addIndent(out, indent + 1);
        serializeNode(child, out, pretty, indent + 1);
        child = child->next;
    }
    if (pretty && array->child) {
        out += '\n';
        addIndent(out, indent);
    }
    out += ']';
}

static void serializeObject(const cJSON* obj, String& out, bool pretty, int indent) {
    out.reserve(out.length() + 64);
    out += '{';
    const cJSON* child = obj->child;
    bool first = true;
    if (pretty && child) out += '\n';

    while (child) {
        if (!first) {
            out += ',';
            if (pretty) out += '\n';
        }
        first = false;
        if (pretty) addIndent(out, indent + 1);
        out += '"';
        jsonEscapeString(child->string, out);
        out += '"';
        out += ':';
        if (pretty) out += ' ';
        serializeNode(child, out, pretty, indent + 1);
        child = child->next;
    }
    if (pretty && obj->child) {
        out += '\n';
        addIndent(out, indent);
    }
    out += '}';
}

static void serializeNumber(const cJSON* item, String& out) {
    // Prefer integer when representable
    double d = item->valuedouble;
    // Use valueint if it matches
    if ((double)item->valueint == d) {
        out += String(item->valueint);
        return;
    }
    // Fallback: limited precision to reduce code size
    // Using String(double, digits) avoids heavy printf linkage
    out += String(d, 6);
}

static void serializeNode(const cJSON* item, String& out, bool pretty, int indent) {
    if (!item) { out += "null"; return; }
    switch (item->type & 0xFF) {
        case cJSON_False: out += "false"; break;
        case cJSON_True: out += "true"; break;
        case cJSON_NULL: out += "null"; break;
        case cJSON_Number: serializeNumber(item, out); break;
        case cJSON_String:
            out += '"';
            jsonEscapeString(item->valuestring, out);
            out += '"';
            break;
        case cJSON_Array: serializeArray(item, out, pretty, indent); break;
        case cJSON_Object: serializeObject(item, out, pretty, indent); break;
        default: out += "null"; break;
    }
}

String Json::serialize(bool pretty) const
{
    if (!root)
        return String();
    String s;
    s.reserve(256);
    serializeNode(root, s, pretty, 0);
    return s;
}

// --------- Construction helpers for nested structures ---------
bool Json::createObject()
{
    if (root) cJSON_Delete(root);
    root = cJSON_CreateObject();
    return root != nullptr;
}

bool Json::createArray()
{
    if (root) cJSON_Delete(root);
    root = cJSON_CreateArray();
    return root != nullptr;
}

bool Json::add(const Json& child)
{
    if (!root || !cJSON_IsArray(root)) return false;
    // Deep copy child root into this array
    cJSON* copy = nullptr;
    if (child.root) {
        copy = cJSON_Duplicate(child.root, /*recurse*/1);
    } else {
        copy = cJSON_CreateNull();
    }
    if (!copy) return false;
    cJSON_AddItemToArray(root, copy);
    return true;
}

bool Json::set(const String& key, const Json& child)
{
    if (!root || !cJSON_IsObject(root)) return false;
    cJSON_DeleteItemFromObjectCaseSensitive(root, key.c_str());
    cJSON* copy = nullptr;
    if (child.root) {
        copy = cJSON_Duplicate(child.root, /*recurse*/1);
    } else {
        copy = cJSON_CreateNull();
    }
    if (!copy) return false;
    cJSON_AddItemToObject(root, key.c_str(), copy);
    return true;
}

bool Json::hasObject(const String &key) const
{
    if (!root)
        return false;
    cJSON *obj = cJSON_GetObjectItemCaseSensitive(root, key.c_str());
    return obj && cJSON_IsObject(obj);
}

void Json::ensureObject(const String &key)
{
    if (!root)
        root = cJSON_CreateObject();
    cJSON *obj = cJSON_GetObjectItemCaseSensitive(root, key.c_str());
    if (!(obj && cJSON_IsObject(obj)))
    {
        cJSON *n = cJSON_CreateObject();
        cJSON_AddItemToObject(root, key.c_str(), n);
    }
}

// --------- Top-level helpers ---------

bool Json::hasKey(const String &key) const
{
    if (!root) return false;
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key.c_str());
    return item != nullptr;
}


bool Json::hasKey(const String &objName, const String &key) const
{
    if (!root)
        return false;
    cJSON *scope = cJSON_GetObjectItemCaseSensitive(root, objName.c_str());
    if (!scope)
        return false;
    cJSON *item = cJSON_GetObjectItemCaseSensitive(scope, key.c_str());
    return item != nullptr;
}

bool Json::setString(const String &key, const String &value)
{
    if (!root) root = cJSON_CreateObject();
    cJSON_DeleteItemFromObjectCaseSensitive(root, key.c_str());
    cJSON_AddItemToObject(root, key.c_str(), cJSON_CreateString(value.c_str()));
    return true;
}

bool Json::setNumber(const String &key, double value)
{
    if (!root) root = cJSON_CreateObject();
    cJSON_DeleteItemFromObjectCaseSensitive(root, key.c_str());
    cJSON_AddItemToObject(root, key.c_str(), cJSON_CreateNumber(value));
    return true;
}

bool Json::setBool(const String &key, bool value)
{
    if (!root) root = cJSON_CreateObject();
    cJSON_DeleteItemFromObjectCaseSensitive(root, key.c_str());
    cJSON_AddItemToObject(root, key.c_str(), cJSON_CreateBool(value));
    return true;
}

bool Json::setArray(const String &key, const std::vector<String> &values)
{
    if (!root) root = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    for (auto &v : values) cJSON_AddItemToArray(arr, cJSON_CreateString(v.c_str()));
    cJSON_DeleteItemFromObjectCaseSensitive(root, key.c_str());
    cJSON_AddItemToObject(root, key.c_str(), arr);
    return true;
}


bool Json::setString(const String &objName, const String &key, const String &value)
{
    ensureObject(objName);
    cJSON *target = cJSON_GetObjectItemCaseSensitive(root, objName.c_str());
    if (!target)
        return false;
    cJSON_DeleteItemFromObjectCaseSensitive(target, key.c_str());
    cJSON_AddItemToObject(target, key.c_str(), cJSON_CreateString(value.c_str()));
    return true;
}

bool Json::setNumber(const String &objName, const String &key, double value)
{
    ensureObject(objName);
    cJSON *target = cJSON_GetObjectItemCaseSensitive(root, objName.c_str());
    if (!target)
        return false;
    cJSON_DeleteItemFromObjectCaseSensitive(target, key.c_str());
    cJSON_AddItemToObject(target, key.c_str(), cJSON_CreateNumber(value));
    return true;
}

bool Json::setBool(const String &objName, const String &key, bool value)
{
    ensureObject(objName);
    cJSON *target = cJSON_GetObjectItemCaseSensitive(root, objName.c_str());
    if (!target)
        return false;
    cJSON_DeleteItemFromObjectCaseSensitive(target, key.c_str());
    cJSON_AddItemToObject(target, key.c_str(), cJSON_CreateBool(value));
    return true;
}

bool Json::setArray(const String &objName, const String &key, const std::vector<String> &values)
{
    ensureObject(objName);
    cJSON *target = cJSON_GetObjectItemCaseSensitive(root, objName.c_str());
    if (!target)
        return false;
    cJSON *arr = cJSON_CreateArray();
    for (auto &v : values)
        cJSON_AddItemToArray(arr, cJSON_CreateString(v.c_str()));
    cJSON_DeleteItemFromObjectCaseSensitive(target, key.c_str());
    cJSON_AddItemToObject(target, key.c_str(), arr);
    return true;
}

bool Json::getString(const String &key, String &out) const
{
    if (!root) return false;
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key.c_str());
    if (!item || !cJSON_IsString(item)) return false;
    out = item->valuestring ? String(item->valuestring) : String();
    return true;
}

bool Json::getBool(const String& key, bool& out) const {
    if (!root) return false;
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key.c_str());
    if (!item) return false;
    if (cJSON_IsBool(item)) {
        out = cJSON_IsTrue(item);
        return true;
    }
    if (cJSON_IsNumber(item)) {
        out = (item->valuedouble != 0.0);
        return true;
    }
    return false;
}

bool Json::getNumber(const String &key, double &out) const
{
    if (!root) return false;
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key.c_str());
    if (!item) return false;
    if (cJSON_IsNumber(item)) {
        out = item->valuedouble;
        return true;
    }
    if (cJSON_IsString(item) && item->valuestring) {
        out = atof(item->valuestring);
        return true;
    }
    return false;
}


// Object-scoped key helpers
bool Json::getString(const String &objName, const String &key, String &out) const
{
    if (!root)
        return false;
    cJSON *scope = cJSON_GetObjectItemCaseSensitive(root, objName.c_str());
    if (!scope)
        return false;
    cJSON *item = cJSON_GetObjectItemCaseSensitive(scope, key.c_str());
    if (!item || !cJSON_IsString(item))
        return false;
    out = item->valuestring ? String(item->valuestring) : String();
    return true;
}

bool Json::getNumber(const String &objName, const String &key, double &out) const
{
    if (!root)
        return false;
    cJSON *scope = cJSON_GetObjectItemCaseSensitive(root, objName.c_str());
    if (!scope)
        return false;
    cJSON *item = cJSON_GetObjectItemCaseSensitive(scope, key.c_str());
    if (!item)
        return false;
    out = item->valuedouble;
    return true;
}

bool Json::getBool(const String &objName, const String &key, bool &out) const
{
    if (!root)
        return false;
    cJSON *scope = cJSON_GetObjectItemCaseSensitive(root, objName.c_str());
    if (!scope)
        return false;
    cJSON *item = cJSON_GetObjectItemCaseSensitive(scope, key.c_str());
    if (!item)
        return false;
    if (cJSON_IsBool(item)) {
        out = cJSON_IsTrue(item);
        return true;
    }
    if (cJSON_IsNumber(item)) {
        out = (item->valuedouble != 0.0);
        return true;
    }
    return false;
}


