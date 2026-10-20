#include "NestingGuard.hpp"

namespace cgv {

namespace {

constexpr uint8_t kQuote = '"';
constexpr uint8_t kEscape = '\\';

bool opensContainer(uint8_t byte) {
    return byte == '{' || byte == '[';
}

bool closesContainer(uint8_t byte) {
    return byte == '}' || byte == ']';
}

} // namespace

bool jsonNestingWithinLimit(std::vector<uint8_t> const& bytes, size_t limit) {
    size_t depth = 0;
    bool insideString = false;
    bool escaped = false;

    for (uint8_t byte : bytes) {
        if (insideString) {
            if (escaped) {
                escaped = false;
                continue;
            }
            if (byte == kEscape) {
                escaped = true;
                continue;
            }
            if (byte == kQuote) insideString = false;
            continue;
        }

        if (byte == kQuote) {
            insideString = true;
            continue;
        }

        if (opensContainer(byte)) {
            ++depth;
            if (depth > limit) return false;
            continue;
        }

        if (closesContainer(byte) && depth > 0) --depth;
    }

    return true;
}

} // namespace cgv
