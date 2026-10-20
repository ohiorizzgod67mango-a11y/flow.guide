#include "Gdr2Parser.hpp"

#include "parse/Framerate.hpp"
#include "parse/GdrTypes.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace cgv {

namespace {

bool positionIsUsable(PhysInput const& input) {
    if (!input.extensionParsed) return false;
    return std::isfinite(input.xPosition) && std::isfinite(input.yPosition);
}

ReplayInput convertInput(PhysInput const& source) {
    ReplayInput converted;
    converted.frame = source.frame;
    converted.button = source.button >= 1 && source.button <= 3 ? source.button : static_cast<uint8_t>(1);
    converted.player2 = source.player2;
    converted.down = source.down;
    converted.hasPosition = positionIsUsable(source);
    if (converted.hasPosition) {
        converted.xPos = source.xPosition;
        converted.yPos = source.yPosition;
    }
    return converted;
}

} // namespace

Gdr2ParseResult parseGdr2(std::span<const uint8_t> data) {
    std::vector<uint8_t> mutableCopy(data.begin(), data.end());

    GdrReplay replay;
    try {
        auto imported = GdrReplay::importData(std::span<uint8_t>(mutableCopy));
        if (imported.isErr()) return std::string("GDR2 parse failed: " + imported.unwrapErr());
        replay = std::move(imported).unwrap();
    } catch (std::exception const& error) {
        return std::string("GDR2 parse threw: ") + error.what();
    } catch (...) {
        return std::string("GDR2 parse threw an unknown error");
    }

    if (replay.inputs.empty()) return std::string("GDR2 replay contains no inputs");

    ReplayData result;
    result.inputs.reserve(replay.inputs.size());
    for (auto const& input : replay.inputs) {
        result.inputs.push_back(convertInput(input));
    }

    std::stable_sort(result.inputs.begin(), result.inputs.end(), [](ReplayInput const& a, ReplayInput const& b) {
        return a.frame < b.frame;
    });

    result.duration = replay.duration;
    result.botName = replay.botInfo.name.empty() ? "GDR2" : replay.botInfo.name;
    result.framerate = recoverTickRate(replay.framerate, result.inputs.back().frame, result.duration);
    result.recomputePositionCoverage();

    return result;
}

} // namespace cgv
