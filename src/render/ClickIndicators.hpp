#pragma once

#include "render/IndicatorTimeline.hpp"
#include <Geode/Geode.hpp>

namespace flow {

class ClickIndicators {
public:
    void attach(PlayLayer* layer);
    void detach();
    void load(cgv::ReplayData const& replay);
    void update(PlayLayer* layer, bool waitingForRestart);

private:
    IndicatorTimeline m_timeline;
    cocos2d::CCDrawNode* m_node = nullptr;
};

}
