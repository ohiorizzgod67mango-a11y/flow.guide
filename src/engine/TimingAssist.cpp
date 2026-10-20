#include "TimingAssist.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace flow {

size_t TimingAssist::channel(Event const& event) {
    return static_cast<size_t>(event.button - 1) + (event.player2 ? 3 : 0);
}

bool TimingAssist::load(cgv::ReplayData const& replay) {
    m_ready = false;
    for (auto& list : m_targets) list.clear();
    reset();
    if (!std::isfinite(replay.framerate) || replay.framerate < 30.0 || replay.framerate > 2000.0) return false;
    if (replay.inputs.empty() || replay.inputs.size() > 2000000) return false;
    for (auto const& input : replay.inputs) {
        if (input.button < 1 || input.button > 3) continue;
        Event event{static_cast<double>(input.frame) / replay.framerate,
                    input.button, input.player2, input.down};
        m_targets[channel(event)].push_back({event, false});
    }
    for (auto& list : m_targets) {
        std::stable_sort(list.begin(), list.end(), [](auto const& a, auto const& b) {
            return a.event.seconds < b.event.seconds;
        });
        if (!list.empty()) m_ready = true;
    }
    return m_ready;
}

void TimingAssist::reset() {
    for (auto& list : m_targets) {
        for (auto& target : list) target.consumed = false;
    }
    for (auto& list : m_pending) list.clear();
    m_corrected = 0;
}

bool TimingAssist::submit(Event event, double windowSeconds) {
    if (!m_ready || event.button < 1 || event.button > 3) return false;
    if (!std::isfinite(event.seconds) || !std::isfinite(windowSeconds)) return false;
    if (windowSeconds < 0.0 || windowSeconds > 0.1) return false;
    auto& targets = m_targets[channel(event)];
    auto& pending = m_pending[channel(event)];
    if (pending.size() >= 64) return false;
    auto start = std::lower_bound(targets.begin(), targets.end(), event.seconds,
        [](Target const& target, double seconds) { return target.event.seconds < seconds; });
    bool adjusted = false;
    for (auto it = start; it != targets.end(); ++it) {
        if (it->event.seconds > event.seconds + windowSeconds + 1e-9) break;
        if (it->consumed) continue;
        if (it->event.down != event.down) break;
        it->consumed = true;
        adjusted = it->event.seconds > event.seconds + 1e-9;
        event.seconds = it->event.seconds;
        break;
    }
    if (!pending.empty()) event.seconds = std::max(event.seconds, pending.back().seconds);
    if (!adjusted && pending.empty()) return false;
    pending.push_back(event);
    if (adjusted) ++m_corrected;
    return true;
}

std::vector<Event> TimingAssist::drain(double now) {
    std::vector<Event> output;
    if (!std::isfinite(now)) return output;
    for (auto& list : m_pending) {
        auto end = std::upper_bound(list.begin(), list.end(), now + 1e-9,
            [](double seconds, Event const& event) { return seconds < event.seconds; });
        output.insert(output.end(), list.begin(), end);
        list.erase(list.begin(), end);
    }
    std::stable_sort(output.begin(), output.end(), [](auto const& a, auto const& b) {
        return a.seconds < b.seconds;
    });
    return output;
}

std::vector<Event> TimingAssist::flushChannel(int button, bool player2) {
    if (button < 1 || button > 3) return {};
    auto& list = m_pending[channel({0.0, button, player2, false})];
    std::vector<Event> result;
    result.swap(list);
    return result;
}

bool TimingAssist::ready() const { return m_ready; }
size_t TimingAssist::corrected() const { return m_corrected; }

}
