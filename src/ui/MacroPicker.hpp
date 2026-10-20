#pragma once

#include <filesystem>
#include <functional>
#include <optional>

namespace cgv {

using PickCallback = std::function<void(std::optional<std::filesystem::path>)>;

void pickMacroFile(PickCallback callback);

} // namespace cgv
