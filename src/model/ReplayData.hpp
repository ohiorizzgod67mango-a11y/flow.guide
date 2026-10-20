#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cgv {

struct ReplayInput {
    uint64_t frame = 0;
    float xPos = 0.f;
    float yPos = 0.f;
    uint8_t button = 1;
    bool player2 = false;
    bool down = false;
    bool hasPosition = false;
};

struct ReplayData {
    std::vector<ReplayInput> inputs;
    double framerate = 240.0;
    float duration = 0.f;
    std::string botName;
    std::string sourceName;
    bool allPositions = false;

    void recomputePositionCoverage() {
        if (inputs.empty()) {
            allPositions = false;
            return;
        }
        for (auto const& input : inputs) {
            if (!input.hasPosition) {
                allPositions = false;
                return;
            }
        }
        allPositions = true;
    }
};

} // namespace cgv
