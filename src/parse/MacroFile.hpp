#pragma once

#include "model/ReplayData.hpp"

#include <filesystem>
#include <string>
#include <variant>

namespace cgv {

using MacroLoadResult = std::variant<ReplayData, std::string>;

constexpr size_t kMaxMacroFileBytes = 256ull * 1024ull * 1024ull;

MacroLoadResult loadMacroFile(std::filesystem::path const& path);

} // namespace cgv
