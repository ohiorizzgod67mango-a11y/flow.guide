#pragma once

#include <cstdint>
#include <matjson.hpp>
#include <span>
#include <string>
#include <variant>

namespace cgv {

using MsgpackResult = std::variant<matjson::Value, std::string>;

MsgpackResult decodeMsgpack(std::span<const uint8_t> data);

} // namespace cgv
