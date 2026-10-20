#pragma once

#include "model/ReplayData.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace flow {

struct Event {
    double seconds = 0.0;
    int button = 1;
    bool player2 = false;
    bool down = false;
};

class TimingAssist {
public:
    bool load(cgv::ReplayData const& replay);
    void reset();
    bool submit(Event event, double windowSeconds);
    std::vector<Event> drain(double now);
    std::vector<Event> flushChannel(int button, bool player2);
    bool ready() const;
    size_t corrected() const;

private:
    struct Target {
        Event event;
        bool consumed = false;
    };
    static size_t channel(Event const& event);
    std::array<std::vector<Target>, 6> m_targets;
    std::array<std::vector<Event>, 6> m_pending;
    size_t m_corrected = 0;
    bool m_ready = false;
};

}
