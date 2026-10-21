#include "engine/TimingAssist.hpp"
#include <algorithm>
#include <vector>

struct NativeEvent {
    double seconds;
    int button;
    int player2;
    int down;
};

struct NativeMetrics {
    int corrected;
    int pending;
    int emitted;
    double physical;
    double target;
    double scheduled;
    double delivered;
    double lateness;
};

extern "C" int simulate(double rate, double window, int active, int simRate,
    double duration, const double* targets, int targetCount,
    const NativeEvent* physical, int physicalCount,
    NativeEvent* output, int capacity, NativeMetrics* metrics) {
    cgv::ReplayData replay;
    replay.framerate = rate;
    for (int i = 0; i < targetCount; ++i) {
        cgv::ReplayInput input;
        input.frame = static_cast<uint64_t>(targets[i]);
        input.down = true;
        replay.inputs.push_back(input);
    }
    flow::TimingAssist engine;
    engine.load(replay);
    int count = 0;
    auto emit = [&](std::vector<flow::Event> const& events) {
        for (auto const& event : events) {
            if (count < capacity) output[count++] = {event.seconds, event.button, event.player2, event.down};
        }
    };
    int next = 0;
    for (int tick = 0; tick <= static_cast<int>(duration * simRate + 1e-8); ++tick) {
        double now = static_cast<double>(tick) / simRate;
        emit(engine.pump(now, active != 0 && window > 0));
        while (next < physicalCount && physical[next].seconds <= now + 1e-9) {
            auto const& event = physical[next++];
            emit(engine.route({now, event.button, event.player2 != 0, event.down != 0},
                std::min(0.1, window / rate), active != 0));
        }
    }
    auto const& d = engine.diagnostics();
    *metrics = {static_cast<int>(d.corrected), static_cast<int>(engine.pendingCount()), count,
                d.lastPhysical, d.lastTarget, d.lastScheduled, d.lastDelivered, d.lastLateness};
    return count;
}
