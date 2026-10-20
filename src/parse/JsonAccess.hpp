#pragma once

#include <matjson.hpp>

#include <cstdint>
#include <initializer_list>
#include <string>

namespace cgv::json {

const matjson::Value* member(matjson::Value const& value, std::string const& key);
const matjson::Value* firstMember(matjson::Value const& value, std::initializer_list<const char*> keys);

double toNumber(matjson::Value const& value, double fallback);
bool toBool(matjson::Value const& value, bool fallback);
std::string toText(matjson::Value const& value, std::string const& fallback);

double numberAt(matjson::Value const& value, std::initializer_list<const char*> keys, double fallback);
bool boolAt(matjson::Value const& value, std::initializer_list<const char*> keys, bool fallback);
std::string textAt(matjson::Value const& value, std::initializer_list<const char*> keys, std::string const& fallback);

bool hasAny(matjson::Value const& value, std::initializer_list<const char*> keys);

} // namespace cgv::json
