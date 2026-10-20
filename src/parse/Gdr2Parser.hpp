#pragma once

#include "model/ReplayData.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <variant>

namespace cgv {

using Gdr2ParseResult = std::variant<ReplayData, std::string>;

Gdr2ParseResult parseGdr2(std::span<const uint8_t> data);

} // namespace cgv
