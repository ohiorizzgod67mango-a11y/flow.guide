#include "Session.hpp"

#include "parse/MacroFile.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

using namespace geode::prelude;

namespace flow {

Session& Session::get() {
    static Session session;
    return session;
}

void Session::attach(PlayLayer* layer) {
    detach();
    m_layer = layer;
    m_indicators.attach(layer);
    m_label = CCLabelBMFont::create("flow assist | practice only", "bigFont.fnt");
    m_label->setScale(0.28f);
    m_label->setAnchorPoint({0.0f, 1.0f});
    m_label->setPosition({8.0f, CCDirector::sharedDirector()->getWinSize().height - 8.0f});
    layer->addChild(m_label, 1000);
    auto path = Mod::get()->getSettingValue<std::filesystem::path>("macro-file");
    m_error = loadMacro(path, false);
    if (!m_error.empty()) log::warn("Flow Assist: {}", m_error);
}

std::string Session::loadMacro(std::filesystem::path const& path, bool requireRestart) {
    if (!m_layer) return "enter a level first";
    auto result = cgv::loadMacroFile(path);
    auto* data = std::get_if<cgv::ReplayData>(&result);
    if (!data) return std::get<std::string>(result);
    TimingAssist candidate;
    if (!candidate.load(*data)) return "unsupported macro timing";
    cancelPending();
    m_engine = std::move(candidate);
    m_indicators.load(*data);
    m_tickRate = data->framerate;
    m_macroName = path.filename().string();
    m_needsRestart = requireRestart;
    m_error.clear();
    return {};
}

void Session::cancelPending() {
    if (!m_layer) return;
    double offset = Mod::get()->getSettingValue<double>("offset-seconds");
    dispatch(m_layer, m_engine.bypass(m_layer->m_gameState.m_levelTime - offset));
}

void Session::requireRestart() {
    cancelPending();
    m_needsRestart = true;
}

std::string Session::diagnosticSummary() const {
    auto const& diagnostic = m_engine.diagnostics();
    return fmt::format("{} | queue {} | last tick delay {:.2f} ms",
        diagnostic.decision, m_engine.pendingCount(), diagnostic.lastLateness * 1000.0);
}

std::string Session::description() const {
    if (!m_error.empty()) return "no macro loaded, choose a file";
    if (!m_engine.ready()) return "no macro loaded";
    return m_macroName + (m_needsRestart ? " | restart attempt to apply" : " | loaded");
}

double Session::tickRate() const { return m_tickRate; }

void Session::reset(PlayLayer* layer) {
    if (layer != m_layer) return;
    m_engine.reset();
    m_needsRestart = false;
}

void Session::detach() {
    m_indicators.detach();
    m_engine = TimingAssist{};
    m_layer = nullptr;
    m_label = nullptr;
    m_dispatching = false;
    m_error.clear();
    m_macroName.clear();
    m_needsRestart = false;
}

bool Session::enabled(PlayLayer* layer) const {
    return layer && layer == m_layer && !m_needsRestart && layer->m_isPracticeMode &&
        Mod::get()->getSettingValue<bool>("enabled") &&
        Mod::get()->getSettingValue<int64_t>("assist-frames") > 0 && m_engine.ready();
}

void Session::dispatch(PlayLayer* layer, std::vector<Event> const& events) {
    if (events.empty()) return;
    m_dispatching = true;
    for (auto const& event : events) layer->handleButton(event.down, event.button, !event.player2);
    m_dispatching = false;
}

void Session::tick(PlayLayer* layer) {
    if (!layer || layer != m_layer) return;
    double offset = Mod::get()->getSettingValue<double>("offset-seconds");
    dispatch(layer, m_engine.pump(layer->m_gameState.m_levelTime - offset, enabled(layer)));
}

void Session::drawHud(PlayLayer* layer) {
    if (!layer || layer != m_layer) return;
    bool active = enabled(layer);
    m_indicators.update(layer, m_needsRestart);
    if (m_label) {
        std::string text;
        if (!m_error.empty()) text = "flow assist | macro failed, check logs";
        else if (m_needsRestart) text = "flow assist | restart attempt to apply macro";
        else if (!layer->m_isPracticeMode) text = "flow assist | practice only";
        else if (!active) text = "flow assist | off";
        else text = fmt::format("flow assist | corrected {} | pending {} | tick delay {:.2f} ms",
            m_engine.corrected(), m_engine.pendingCount(), m_engine.diagnostics().lastLateness * 1000.0);
        m_label->setString(text.c_str());
        m_label->limitLabelWidth(CCDirector::sharedDirector()->getWinSize().width - 16.f, 0.28f, 0.13f);
        m_label->setColor(active ? ccColor3B{100, 255, 170} : ccColor3B{255, 200, 100});
    }
}

bool Session::input(PlayLayer* layer, bool down, int button, bool player1) {
    if (m_dispatching || layer != m_layer || button < 1 || button > 3) return false;
    double offset = Mod::get()->getSettingValue<double>("offset-seconds");
    double now = layer->m_gameState.m_levelTime - offset;
    auto frames = Mod::get()->getSettingValue<int64_t>("assist-frames");
    double window = std::clamp(static_cast<double>(frames) / m_tickRate, 0.0, 0.1);
    if (!std::isfinite(now)) return false;
    auto events = m_engine.route({now, button, !player1, down}, window, enabled(layer));
    if (!events.empty()) {
        auto const& last = events.back();
        if (last.seconds == now && last.button == button && last.player2 == !player1 && last.down == down) {
            events.pop_back();
            std::erase_if(events, [button, player1, down](Event const& event) {
                return event.button == button && event.player2 == !player1 && event.down == down;
            });
            dispatch(layer, events);
            return false;
        }
    }
    dispatch(layer, events);
    return true;
}

}
