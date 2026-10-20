#include "ZbotParser.hpp"

#include <cmath>
#include <cstring>

namespace cgv {

namespace {

constexpr uint8_t kAsciiZero = '0';
constexpr uint8_t kAsciiOne = '1';

constexpr size_t kHoldOffset = 4;
constexpr size_t kPlayerOffset = 5;
constexpr size_t kProbeRecords = 8;

float readFloat(std::span<const uint8_t> bytes, size_t offset) {
    float value = 0.f;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

uint32_t readUint32(std::span<const uint8_t> bytes, size_t offset) {
    uint32_t value = 0;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

bool isFlagByte(uint8_t value) {
    return value == kAsciiZero || value == kAsciiOne;
}

bool isPlayerByte(uint8_t value) {
    return value == kAsciiZero || value == kAsciiOne;
}

bool payloadDividesEvenly(std::span<const uint8_t> bytes) {
    if (bytes.size() <= kZbotHeaderBytes) return false;
    return (bytes.size() - kZbotHeaderBytes) % kZbotRecordBytes == 0;
}

bool deltaLooksSane(float delta) {
    if (!std::isfinite(delta) || delta <= 0.f) return false;
    double rate = 1.0 / static_cast<double>(delta);
    return rate >= kZbotMinimumTickRate && rate <= kZbotMaximumTickRate;
}

} // namespace

bool looksLikeZbot(std::span<const uint8_t> bytes) {
    if (!payloadDividesEvenly(bytes)) return false;
    if (!deltaLooksSane(readFloat(bytes, 0))) return false;

    size_t records = (bytes.size() - kZbotHeaderBytes) / kZbotRecordBytes;
    size_t probe = records < kProbeRecords ? records : kProbeRecords;

    for (size_t index = 0; index < probe; ++index) {
        size_t offset = kZbotHeaderBytes + index * kZbotRecordBytes;
        if (!isFlagByte(bytes[offset + kHoldOffset])) return false;
        if (!isPlayerByte(bytes[offset + kPlayerOffset])) return false;
    }

    return probe > 0;
}

std::variant<ReplayData, std::string> parseZbot(std::span<const uint8_t> bytes) {
    if (!payloadDividesEvenly(bytes)) {
        return std::string("Zbot macro has a truncated record");
    }

    float delta = readFloat(bytes, 0);
    if (!deltaLooksSane(delta)) {
        return std::string("Zbot macro has an unreadable frame rate");
    }

    ReplayData replay;
    replay.framerate = 1.0 / static_cast<double>(delta);
    replay.botName = "zBot";

    size_t records = (bytes.size() - kZbotHeaderBytes) / kZbotRecordBytes;
    replay.inputs.reserve(records);

    uint64_t lastFrame = 0;

    for (size_t index = 0; index < records; ++index) {
        size_t offset = kZbotHeaderBytes + index * kZbotRecordBytes;

        uint8_t hold = bytes[offset + kHoldOffset];
        uint8_t player = bytes[offset + kPlayerOffset];
        if (!isFlagByte(hold) || !isPlayerByte(player)) {
            return std::string("Zbot macro has a corrupted input record");
        }

        ReplayInput input;
        input.frame = static_cast<uint64_t>(readUint32(bytes, offset));
        input.down = hold == kAsciiOne;
        input.player2 = player == kAsciiZero;
        input.button = 1;

        if (input.frame < lastFrame) {
            return std::string("Zbot macro inputs are out of order");
        }
        lastFrame = input.frame;

        replay.inputs.push_back(input);
    }

    if (replay.inputs.empty()) {
        return std::string("Zbot macro has no inputs");
    }

    replay.duration = static_cast<float>(static_cast<double>(lastFrame) / replay.framerate);
    replay.recomputePositionCoverage();

    return replay;
}

} // namespace cgv
