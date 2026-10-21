#pragma once

#include "engine/TimingAssist.hpp"
#include "render/ClickIndicators.hpp"
#include <Geode/Geode.hpp>
#include <filesystem>
#include <string>

namespace flow {

class Session {
public:
    static Session& get();
    void attach(PlayLayer* layer);
    void reset(PlayLayer* layer);
    void detach();
    std::string loadMacro(std::filesystem::path const& path, bool requireRestart = true);
    std::string description() const;
    double tickRate() const;
    void cancelPending();
    void requireRestart();
    void tick(PlayLayer* layer);
    void drawHud(PlayLayer* layer);
    std::string diagnosticSummary() const;
    bool input(PlayLayer* layer, bool down, int button, bool player1);

private:
    void dispatch(PlayLayer* layer, std::vector<Event> const& events);
    bool enabled(PlayLayer* layer) const;
    TimingAssist m_engine;
    ClickIndicators m_indicators;
    PlayLayer* m_layer = nullptr;
    cocos2d::CCLabelBMFont* m_label = nullptr;
    bool m_dispatching = false;
    std::string m_error;
    double m_tickRate = 240.0;
    bool m_needsRestart = false;
    std::string m_macroName;
};

}
