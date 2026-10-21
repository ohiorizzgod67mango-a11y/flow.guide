#include "IndicatorTimeline.hpp"
#include <algorithm>
#include <cmath>

namespace flow {

bool IndicatorTimeline::load(cgv::ReplayData const& replay) {
    clear();
    if (!std::isfinite(replay.framerate) || replay.framerate < 30.0 || replay.framerate > 2000.0) return false;
    for (auto const& input : replay.inputs) {
        if (input.down && input.button == 1) {
            m_markers.push_back({static_cast<double>(input.frame) / replay.framerate, input.player2});
        }
    }
    std::stable_sort(m_markers.begin(), m_markers.end(), [](auto const& a, auto const& b) {
        return a.seconds < b.seconds;
    });
    return !m_markers.empty();
}

void IndicatorTimeline::clear() { m_markers.clear(); }

std::vector<ClickMarker> IndicatorTimeline::upcoming(double now, double ahead, size_t limit) const {
    std::vector<ClickMarker> output;
    if (!std::isfinite(now) || !std::isfinite(ahead) || ahead < 0.0) return output;
    auto start = std::lower_bound(m_markers.begin(), m_markers.end(), now,
        [](ClickMarker const& marker, double time) { return marker.seconds < time; });
    for (auto it = start; it != m_markers.end() && output.size() < limit; ++it) {
        if (it->seconds > now + ahead) break;
        output.push_back(*it);
    }
    return output;
}

}
