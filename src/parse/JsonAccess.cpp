#include "JsonAccess.hpp"

#include <charconv>
#include <cmath>

namespace cgv::json {

namespace {

bool textToNumber(std::string const& text, double& out) {
    if (text.empty()) return false;
    try {
        size_t consumed = 0;
        double parsed = std::stod(text, &consumed);
        if (consumed == 0 || !std::isfinite(parsed)) return false;
        out = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool looksTrue(std::string const& text) {
    return text == "true" || text == "True" || text == "TRUE" || text == "1" || text == "yes";
}

} // namespace

const matjson::Value* member(matjson::Value const& value, std::string const& key) {
    if (!value.isObject()) return nullptr;
    auto found = value.get(key);
    if (!found) return nullptr;
    return &found.unwrap();
}

const matjson::Value* firstMember(matjson::Value const& value, std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        if (auto* found = member(value, key)) return found;
    }
    return nullptr;
}

double toNumber(matjson::Value const& value, double fallback) {
    if (value.isNumber()) {
        double raw = value.asDouble().unwrapOr(fallback);
        return std::isfinite(raw) ? raw : fallback;
    }
    if (value.isBool()) return value.asBool().unwrapOr(false) ? 1.0 : 0.0;
    if (value.isString()) {
        double parsed = fallback;
        if (textToNumber(value.asString().unwrapOr(""), parsed)) return parsed;
    }
    return fallback;
}

bool toBool(matjson::Value const& value, bool fallback) {
    if (value.isBool()) return value.asBool().unwrapOr(fallback);
    if (value.isNumber()) return toNumber(value, 0.0) != 0.0;
    if (value.isString()) return looksTrue(value.asString().unwrapOr(""));
    return fallback;
}

std::string toText(matjson::Value const& value, std::string const& fallback) {
    if (value.isString()) return value.asString().unwrapOr(fallback);
    if (value.isNumber()) return std::to_string(toNumber(value, 0.0));
    if (value.isBool()) return toBool(value, false) ? "true" : "false";
    return fallback;
}

double numberAt(matjson::Value const& value, std::initializer_list<const char*> keys, double fallback) {
    if (auto* found = firstMember(value, keys)) return toNumber(*found, fallback);
    return fallback;
}

bool boolAt(matjson::Value const& value, std::initializer_list<const char*> keys, bool fallback) {
    if (auto* found = firstMember(value, keys)) return toBool(*found, fallback);
    return fallback;
}

std::string textAt(matjson::Value const& value, std::initializer_list<const char*> keys, std::string const& fallback) {
    if (auto* found = firstMember(value, keys)) return toText(*found, fallback);
    return fallback;
}

bool hasAny(matjson::Value const& value, std::initializer_list<const char*> keys) {
    return firstMember(value, keys) != nullptr;
}

} // namespace cgv::json
