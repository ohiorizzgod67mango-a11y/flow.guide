#include "ClickIndicators.hpp"
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace flow {

void ClickIndicators::attach(PlayLayer* layer) {
    detach();
    if (!layer || !layer->m_objectLayer) return;
    m_node = CCDrawNode::create();
    if (!m_node) return;
    m_node->setID("flow-click-indicators"_spr);
    m_node->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
    layer->m_objectLayer->addChild(m_node, 1000);
}

void ClickIndicators::detach() {
    m_node = nullptr;
    m_timeline.clear();
}

void ClickIndicators::load(cgv::ReplayData const& replay) {
    m_timeline.load(replay);
    if (m_node) m_node->clear();
}

void ClickIndicators::update(PlayLayer* layer, bool waitingForRestart) {
    if (!m_node || !layer) return;
    m_node->clear();
    auto* mod = Mod::get();
    if (waitingForRestart || !mod->getSettingValue<bool>("show-click-lines")) return;
    double offset = mod->getSettingValue<double>("offset-seconds");
    double now = layer->m_gameState.m_levelTime - offset;
    double ahead = std::clamp(mod->getSettingValue<double>("line-lookahead"), 0.1, 5.0);
    float radius = static_cast<float>(std::clamp<int64_t>(mod->getSettingValue<int64_t>("line-width"), 1, 3)) * 0.5f;
    for (auto const& marker : m_timeline.upcoming(now, ahead)) {
        auto* player = marker.player2 ? layer->m_player2 : layer->m_player1;
        if (!player || (marker.player2 && !layer->m_gameState.m_isDualMode)) continue;
        double target = marker.seconds + offset;
        if (!std::isfinite(target) || target < 0.0) continue;
        float x = layer->posForTime(static_cast<float>(target)).x;
        float y = player->getPositionY();
        if (!std::isfinite(x) || !std::isfinite(y)) continue;
        ccColor4F color = marker.player2 ? ccColor4F{1.f, 0.65f, 0.4f, 0.85f} : ccColor4F{0.45f, 1.f, 0.7f, 0.85f};
        m_node->drawSegment({x, y - 70.f}, {x, y + 70.f}, radius, color);
    }
}

}
