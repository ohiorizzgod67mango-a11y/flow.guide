#include "engine/TimingAssist.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>

using flow::Event;
using flow::TimingAssist;

cgv::ReplayData macro() {
    cgv::ReplayData data;
    data.framerate = 240.0;
    data.inputs = {{100, 0, 0, 1, false, true}, {120, 0, 0, 1, false, false},
                   {100, 0, 0, 1, true, true}, {120, 0, 0, 1, true, false}};
    return data;
}

int main() {
    TimingAssist engine;
    assert(engine.load(macro()));
    assert(engine.drain(100).empty());
    assert(engine.submit({97.0 / 240, 1, false, true}, 3.0 / 240));
    assert(engine.drain(99.0 / 240).empty());
    auto due = engine.drain(100.0 / 240);
    assert(due.size() == 1 && due[0].down && !due[0].player2);
    assert(engine.corrected() == 1);
    assert(engine.drain(101.0 / 240).empty());
    assert(engine.submit({118.0 / 240, 1, false, false}, 3.0 / 240));
    due = engine.drain(120.0 / 240);
    assert(due.size() == 1 && !due[0].down);
    engine.reset();
    assert(engine.corrected() == 0);
    assert(!engine.submit({101.0 / 240, 1, false, true}, 3.0 / 240));
    assert(!engine.submit({90.0 / 240, 1, false, true}, 3.0 / 240));
    assert(!engine.submit({100.0 / 240, 1, false, true}, 3.0 / 240));
    assert(!engine.submit({97.0 / 240, 1, false, true}, 0.0));
    engine.reset();
    assert(engine.submit({98.0 / 240, 1, true, true}, 3.0 / 240));
    assert(engine.submit({98.0 / 240, 1, false, true}, 3.0 / 240));
    assert(engine.drain(100.0 / 240).size() == 2);
    engine.reset();
    assert(engine.submit({98.0 / 240, 1, false, true}, 3.0 / 240));
    assert(engine.submit({99.0 / 240, 1, false, false}, 3.0 / 240));
    due = engine.drain(100.0 / 240);
    assert(due.size() == 2 && due[0].down && !due[1].down);
    engine.reset();
    assert(engine.drain(100).empty());
    assert(!engine.submit({0, 0, false, true}, 0.01));
    assert(!engine.submit({0, 4, false, true}, 0.01));
    assert(!engine.submit({NAN, 1, false, true}, 0.01));
    assert(!engine.submit({0, 1, false, true}, NAN));
    assert(!engine.submit({0, 1, false, true}, -1));
    assert(!engine.submit({0, 1, false, true}, 1));
    assert(engine.drain(NAN).empty());
    auto bad = macro();
    bad.framerate = NAN;
    assert(!engine.load(bad));
    assert(!engine.ready());
    assert(engine.drain(100).empty());
    bad.framerate = 0;
    assert(!engine.load(bad));
    bad.framerate = 1000000;
    assert(!engine.load(bad));
    bad.inputs.clear();
    bad.framerate = 240;
    assert(!engine.load(bad));
    assert(engine.load(macro()));
    assert(engine.submit({98.0 / 240, 1, false, true}, 3.0 / 240));
    for (int i = 1; i < 64; ++i) {
        assert(engine.submit({99.0 / 240, 1, false, true}, 3.0 / 240));
    }
    assert(!engine.submit({99.0 / 240, 1, false, false}, 3.0 / 240));
    assert(engine.flushChannel(1, false).size() == 64);
    assert(engine.drain(100).empty());
    assert(engine.flushChannel(0, false).empty());
    assert(engine.load(macro()));
    assert(!engine.submit({98.0 / 240, 2, false, true}, 3.0 / 240));
    assert(engine.submit({98.0 / 240, 1, false, true}, 3.0 / 240));
    engine.reset();
    assert(engine.drain(100).empty());
    assert(engine.submit({98.0 / 240, 1, false, true}, 3.0 / 240));
    assert(engine.drain(100.0 / 240).size() == 1);
    std::mt19937 random(42);
    for (int iteration = 0; iteration < 5000; ++iteration) {
        cgv::ReplayData data;
        data.framerate = 240;
        for (uint64_t i = 0; i < 40; ++i) data.inputs.push_back({100 + 20 * i, 0, 0, 1, false, i % 2 == 0});
        assert(engine.load(data));
        std::vector<Event> delivered;
        for (auto const& input : data.inputs) {
            double now = (static_cast<double>(input.frame) + static_cast<int>(random() % 11) - 5) / 240.0;
            auto old = engine.drain(now);
            delivered.insert(delivered.end(), old.begin(), old.end());
            Event event{now, 1, false, input.down};
            if (!engine.submit(event, 3.0 / 240)) delivered.push_back(event);
        }
        auto tail = engine.drain(1000);
        delivered.insert(delivered.end(), tail.begin(), tail.end());
        assert(delivered.size() == data.inputs.size());
        for (size_t i = 0; i < delivered.size(); ++i) {
            assert(delivered[i].down == data.inputs[i].down);
            if (i) assert(delivered[i].seconds >= delivered[i-1].seconds);
        }
    }
    std::cout << "deterministic checks and 5000 randomized sequences passed\n";
}
