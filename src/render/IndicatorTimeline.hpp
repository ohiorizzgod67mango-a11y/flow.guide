#pragma once

#include "model/ReplayData.hpp"
#include <vector>
#include <cstddef>

namespace flow {

struct ClickMarker {
    double seconds = 0.0;
    bool player2 = false;
};

class IndicatorTimeline {
public:
    bool load(cgv::ReplayData const& replay);
    void clear();
    std::vector<ClickMarker> upcoming(double now, double ahead, size_t limit = 512) const;

private:
    std::vector<ClickMarker> m_markers;
};

}
