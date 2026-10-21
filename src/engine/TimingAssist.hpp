#pragma once

#include "model/ReplayData.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace flow {

struct Event {
    double seconds = 0.0;
    int button = 1;
    bool player2 = false;
    bool down = false;
};

struct Diagnostics {
    size_t corrected = 0;
    size_t passed = 0;
    size_t duplicates = 0;
    size_t delivered = 0;
    double lastPhysical = 0.0;
    double lastTarget = 0.0;
    double lastScheduled = 0.0;
    double lastDelivered = 0.0;
    double lastLateness = 0.0;
    std::string decision = "idle";
};

class TimingAssist {
public:
    bool load(cgv::ReplayData const& replay);
    void reset();
    std::vector<Event> route(Event event, double windowSeconds, bool active);
    std::vector<Event> pump(double now, bool active);
    std::vector<Event> bypass(double now);
    bool ready() const;
    size_t corrected() const;
    size_t pendingCount() const;
    Diagnostics const& diagnostics() const;

private:
    struct Target {
        double seconds = 0.0;
        bool consumed = false;
    };
    struct Pending {
        Event event;
        uint64_t pair = 0;
    };
    struct Channel {
        std::vector<Target> targets;
        std::deque<Pending> pending;
        bool physical = false;
        bool output = false;
        bool lastWasQueued = false;
        double delay = 0.0;
        double lastDelivered = -1.0;
        uint64_t pair = 0;
    };
    static size_t channel(Event const& event);
    void append(std::vector<Event>& output, Event event);
    void reconcile(std::vector<Event>& output, double now);
    std::array<Channel, 6> m_channels;
    Diagnostics m_diagnostics;
    bool m_ready = false;
    bool m_active = false;
    uint64_t m_nextPair = 0;
};

}
