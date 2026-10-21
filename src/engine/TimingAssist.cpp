#include "TimingAssist.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace flow {

size_t TimingAssist::channel(Event const& event) {
    return static_cast<size_t>(event.button - 1) + (event.player2 ? 3 : 0);
}

bool TimingAssist::load(cgv::ReplayData const& replay) {
    m_channels = {};
    reset();
    m_ready = false;
    if (!std::isfinite(replay.framerate) || replay.framerate < 30.0 || replay.framerate > 2000.0) return false;
    if (replay.inputs.empty() || replay.inputs.size() > 2000000) return false;
    for (auto const& input : replay.inputs) {
        if (input.button != 1 || !input.down) continue;
        Event event{static_cast<double>(input.frame) / replay.framerate, input.button, input.player2, true};
        m_channels[channel(event)].targets.push_back({event.seconds, false});
        m_ready = true;
    }
    for (auto& state : m_channels) {
        std::stable_sort(state.targets.begin(), state.targets.end(), [](auto const& a, auto const& b) {
            return a.seconds < b.seconds;
        });
    }
    return m_ready;
}

void TimingAssist::reset() {
    for (auto& state : m_channels) {
        for (auto& target : state.targets) target.consumed = false;
        state.pending.clear();
        state.physical = false;
        state.output = false;
        state.lastWasQueued = false;
        state.delay = 0.0;
        state.lastDelivered = -1.0;
        state.pair = 0;
    }
    m_diagnostics = {};
    m_active = false;
    m_nextPair = 0;
}

void TimingAssist::append(std::vector<Event>& output, Event event) {
    auto& state = m_channels[channel(event)];
    state.output = event.down;
    state.lastWasQueued = false;
    state.lastDelivered = event.seconds;
    output.push_back(event);
}

void TimingAssist::reconcile(std::vector<Event>& output, double now) {
    for (size_t index = 0; index < m_channels.size(); ++index) {
        auto& state = m_channels[index];
        state.pending.clear();
        state.delay = 0.0;
        if (state.physical != state.output) {
            append(output, {now, static_cast<int>(index % 3) + 1, index >= 3, state.physical});
        }
    }
    m_active = false;
}

std::vector<Event> TimingAssist::bypass(double now) {
    std::vector<Event> output;
    if (std::isfinite(now)) reconcile(output, now);
    return output;
}

std::vector<Event> TimingAssist::route(Event event, double windowSeconds, bool active) {
    std::vector<Event> output;
    if (!std::isfinite(event.seconds) || event.button < 1 || event.button > 3) return output;
    bool usable = active && m_ready && std::isfinite(windowSeconds) && windowSeconds > 0.0 && windowSeconds <= 0.1;
    auto& state = m_channels[channel(event)];
    bool wasHeld = state.physical;
    state.physical = event.down;
    if (!usable && m_active) reconcile(output, event.seconds);
    m_diagnostics.lastPhysical = event.seconds;
    if (!usable || event.button != 1) {
        append(output, event);
        ++m_diagnostics.passed;
        m_diagnostics.decision = "bypass";
        return output;
    }
    m_active = true;
    if (event.down == wasHeld) {
        ++m_diagnostics.duplicates;
        if (state.pending.empty()) {
            append(output, event);
            m_diagnostics.decision = "repeat unchanged";
        } else {
            m_diagnostics.decision = "buffered repeat ignored";
        }
        return output;
    }
    if (state.pending.size() >= 64) {
        reconcile(output, event.seconds);
        m_diagnostics.decision = "overflow, reconciled to physical state";
        return output;
    }
    double physicalTime = event.seconds;
    if (event.down) {
        state.pair = ++m_nextPair;
        state.delay = 0.0;
        auto start = std::lower_bound(state.targets.begin(), state.targets.end(), physicalTime,
            [](Target const& target, double seconds) { return target.seconds < seconds; });
        for (auto it = start; it != state.targets.end(); ++it) {
            if (it->seconds > physicalTime + windowSeconds + 1e-9) break;
            if (it->consumed) continue;
            it->consumed = true;
            state.delay = std::max(0.0, it->seconds - physicalTime);
            m_diagnostics.lastTarget = it->seconds;
            if (state.delay > 1e-9) ++m_diagnostics.corrected;
            break;
        }
    }
    event.seconds += state.delay;
    if (!state.pending.empty()) event.seconds = std::max(event.seconds, state.pending.back().event.seconds);
    m_diagnostics.lastScheduled = event.seconds;
    bool sameQueuedTick = state.lastWasQueued && physicalTime <= state.lastDelivered + 1e-9;
    if (event.seconds > physicalTime + 1e-9 || !state.pending.empty() || sameQueuedTick) {
        state.pending.push_back({event, state.pair});
        m_diagnostics.decision = event.down ? "press buffered" : "release follows press delay";
    } else {
        append(output, event);
        ++m_diagnostics.passed;
        m_diagnostics.decision = "unchanged";
    }
    return output;
}

std::vector<Event> TimingAssist::pump(double now, bool active) {
    std::vector<Event> output;
    if (!std::isfinite(now)) return output;
    if (!active) {
        if (m_active) reconcile(output, now);
        return output;
    }
    for (auto& state : m_channels) {
        if (state.pending.empty() || now <= state.lastDelivered + 1e-9) continue;
        auto item = state.pending.front();
        if (item.event.seconds > now + 1e-9) continue;
        state.pending.pop_front();
        double lateness = std::max(0.0, now - item.event.seconds);
        if (item.event.down && lateness > 1e-9) {
            for (auto& future : state.pending) {
                if (future.pair == item.pair) future.event.seconds += lateness;
            }
            if (state.pair == item.pair && state.physical) state.delay += lateness;
        }
        item.event.seconds = now;
        append(output, item.event);
        state.lastWasQueued = true;
        ++m_diagnostics.delivered;
        m_diagnostics.lastDelivered = now;
        m_diagnostics.lastLateness = lateness;
    }
    std::stable_sort(output.begin(), output.end(), [](auto const& a, auto const& b) {
        return a.seconds < b.seconds;
    });
    return output;
}

bool TimingAssist::ready() const { return m_ready; }
size_t TimingAssist::corrected() const { return m_diagnostics.corrected; }
size_t TimingAssist::pendingCount() const {
    size_t count = 0;
    for (auto const& state : m_channels) count += state.pending.size();
    return count;
}
Diagnostics const& TimingAssist::diagnostics() const { return m_diagnostics; }

}
