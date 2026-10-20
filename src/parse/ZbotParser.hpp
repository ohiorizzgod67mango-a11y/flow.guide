#pragma once

#include "model/ReplayData.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <variant>

namespace cgv {

constexpr size_t kZbotHeaderBytes = 8;
constexpr size_t kZbotRecordBytes = 6;
constexpr double kZbotMinimumTickRate = 1.0;
constexpr double kZbotMaximumTickRate = 100000.0;

bool looksLikeZbot(std::span<const uint8_t> bytes);

std::variant<ReplayData, std::string> parseZbot(std::span<const uint8_t> bytes);

} // namespace cgv
