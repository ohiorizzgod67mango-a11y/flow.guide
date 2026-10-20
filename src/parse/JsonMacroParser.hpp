#pragma once

#include "model/ReplayData.hpp"

#include <matjson.hpp>
#include <string>
#include <variant>

namespace cgv {

using JsonParseResult = std::variant<ReplayData, std::string>;

JsonParseResult parseJsonMacro(matjson::Value const& root);
JsonParseResult parseJsonMacroText(std::string const& text);

} // namespace cgv
