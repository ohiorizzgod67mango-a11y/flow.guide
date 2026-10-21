#include "engine/TimingAssist.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>

using flow::Event;
using flow::TimingAssist;

cgv::ReplayData macro() {
    cgv::ReplayData data;
    data.framerate = 240.0;
    data.inputs = {{100, 0, 0, 1, false, true}, {110, 0, 0, 1, false, false},
                   {100, 0, 0, 1, true, true}, {120, 0, 0, 1, true, false}};
    return data;
}

bool near(double a, double b) { return std::abs(a - b) < 1e-8; }

int main() {
    TimingAssist engine;
    assert(engine.load(macro()));
    assert(engine.pump(100, true).empty());
    assert(engine.route({97.0 / 240, 1, false, true}, 3.0 / 240, true).empty());
    assert(engine.route({98.0 / 240, 1, false, false}, 3.0 / 240, true).empty());
    assert(engine.pump(99.0 / 240, true).empty());
    auto press = engine.pump(100.0 / 240, true);
    assert(press.size() == 1 && press[0].down);
    assert(engine.pump(100.0 / 240, true).empty());
    auto release = engine.pump(101.0 / 240, true);
    assert(release.size() == 1 && !release[0].down);
    assert(near(release[0].seconds - press[0].seconds, 1.0 / 240));
    engine.reset();
    engine.route({97.0 / 240, 1, false, true}, 3.0 / 240, true);
    press = engine.pump(100.0 / 240, true);
    assert(engine.route({117.0 / 240, 1, false, false}, 3.0 / 240, true).empty());
    release = engine.pump(120.0 / 240, true);
    assert(press.size() == 1 && release.size() == 1);
    assert(near(release[0].seconds - press[0].seconds, 20.0 / 240));
    engine.reset();
    engine.route({97.0 / 240, 1, false, true}, 3.0 / 240, true);
    engine.route({98.0 / 240, 1, false, false}, 3.0 / 240, true);
    press = engine.pump(102.0 / 240, true);
    assert(press.size() == 1 && press[0].down);
    assert(engine.pump(102.0 / 240, true).empty());
    release = engine.pump(103.0 / 240, true);
    assert(release.size() == 1 && !release[0].down);
    assert(near(release[0].seconds - press[0].seconds, 1.0 / 240));
    engine.reset();
    auto events = engine.route({101.0 / 240, 1, false, true}, 3.0 / 240, true);
    assert(events.size() == 1 && near(events[0].seconds, 101.0 / 240));
    assert(engine.corrected() == 0);
    engine.reset();
    events = engine.route({90.0 / 240, 1, false, true}, 3.0 / 240, true);
    assert(events.size() == 1 && events[0].down);
    engine.reset();
    engine.route({97.0 / 240, 1, false, true}, 3.0 / 240, true);
    assert(engine.route({97.5 / 240, 1, false, true}, 3.0 / 240, true).empty());
    assert(engine.pendingCount() == 1 && engine.diagnostics().duplicates == 1);
    engine.route({97.0 / 240, 1, true, true}, 3.0 / 240, true);
    assert(engine.pump(100.0 / 240, true).size() == 2);
    assert(engine.bypass(101.0 / 240).empty());
    events = engine.route({102.0 / 240, 1, true, false}, 0, false);
    assert(events.size() == 1 && !events[0].down && events[0].player2);
    engine.reset();
    engine.route({97.0 / 240, 1, false, true}, 3.0 / 240, true);
    events = engine.route({98.0 / 240, 1, false, false}, 0, false);
    assert(events.size() == 1 && !events[0].down);
    assert(engine.pump(200.0 / 240, false).empty());
    engine.reset();
    engine.route({97.0 / 240, 1, false, true}, 3.0 / 240, true);
    events = engine.bypass(98.0 / 240);
    assert(events.size() == 1 && events[0].down);
    assert(engine.pendingCount() == 0);
    events = engine.route({99.0 / 240, 1, false, false}, 0, false);
    assert(events.size() == 1 && !events[0].down);
    engine.reset();
    assert(engine.pump(100, true).empty());
    assert(engine.route({NAN, 1, false, true}, 0.01, true).empty());
    assert(engine.route({0, 0, false, true}, 0.01, true).empty());
    assert(engine.pump(NAN, true).empty());
    for (double invalid : {0.0, -1.0, 1e9, static_cast<double>(NAN)}) {
        auto data = macro();
        data.framerate = invalid;
        assert(!engine.load(data));
        assert(!engine.ready());
    }
    assert(engine.load(macro()));
    engine.reset();
    auto repeated = engine.route({90.0 / 240, 1, false, true}, 3.0 / 240, true);
    assert(repeated.size() == 1);
    repeated = engine.route({91.0 / 240, 1, false, true}, 3.0 / 240, true);
    assert(repeated.size() == 1 && repeated[0].down);
    assert(engine.corrected() == 0);
    engine.reset();
    auto direction = engine.route({97.0 / 240, 2, false, true}, 3.0 / 240, true);
    assert(direction.size() == 1 && direction[0].button == 2);
    assert(engine.bypass(98.0 / 240).empty());
    for (double invalidWindow : {-1.0, 1.0, static_cast<double>(NAN)}) {
        engine.reset();
        auto result = engine.route({97.0 / 240, 1, false, true}, invalidWindow, true);
        assert(result.size() == 1 && near(result[0].seconds, 97.0 / 240));
        assert(engine.pendingCount() == 0);
    }
    std::mt19937 random(42);
    for (int i = 0; i < 100000; ++i) {
        Event event{i * 0.001, static_cast<int>(random() % 3) + 1,
                    random() % 2 == 0, random() % 2 == 0};
        auto result = engine.route(event, 0, false);
        assert(result.size() == 1);
        assert(result[0].seconds == event.seconds && result[0].down == event.down);
        assert(result[0].button == event.button && result[0].player2 == event.player2);
        assert(engine.pump(event.seconds, false).empty());
    }
    for (int rate : {120, 240, 360, 1000}) {
        for (int trial = 0; trial < 1250; ++trial) {
            cgv::ReplayData data;
            data.framerate = rate;
            for (uint64_t frame = 100; frame < 1200; frame += 30) {
                data.inputs.push_back({frame, 0, 0, 1, false, true});
            }
            assert(engine.load(data));
            std::vector<Event> physical;
            for (auto const& input : data.inputs) {
                double time = (static_cast<double>(input.frame) - random() % 8) / rate;
                physical.push_back({time, 1, false, true});
                physical.push_back({time + (1.0 + random() % 12) / rate, 1, false, false});
            }
            std::vector<Event> emitted;
            size_t next = 0;
            double end = physical.back().seconds + 1.0;
            for (int step = 0; step / 240.0 <= end; ++step) {
                double now = step / 240.0;
                while (next < physical.size() && physical[next].seconds <= now + 1e-9) {
                    Event event = physical[next++];
                    event.seconds = now;
                    auto delivered = engine.route(event, std::min(0.1, 6.0 / rate), true);
                    emitted.insert(emitted.end(), delivered.begin(), delivered.end());
                }
                auto delivered = engine.pump(now, true);
                emitted.insert(emitted.end(), delivered.begin(), delivered.end());
            }
            assert(emitted.size() == physical.size());
            for (size_t i = 0; i < emitted.size(); ++i) {
                assert(emitted[i].down == physical[i].down);
                if (i) assert(emitted[i].seconds >= emitted[i-1].seconds);
                if (i % 2) {
                    double original = std::ceil(physical[i].seconds * 240.0 - 1e-8) / 240.0 -
                                      std::ceil(physical[i-1].seconds * 240.0 - 1e-8) / 240.0;
                    double held = emitted[i].seconds - emitted[i-1].seconds;
                    assert(held + 1e-8 >= original);
                    assert(held <= original + 1.0 / 240 + 1e-8);
                }
            }
            assert(engine.pendingCount() == 0);
        }
    }
    std::cout << "regressions, 100000 bypass events and 5000 multi-rate sequences passed\n";
}
