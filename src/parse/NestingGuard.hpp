#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cgv {

constexpr size_t kMaxJsonNestingDepth = 256;

bool jsonNestingWithinLimit(std::vector<uint8_t> const& bytes, size_t limit = kMaxJsonNestingDepth);

} // namespace cgv
