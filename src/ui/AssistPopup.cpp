#include "AssistPopup.hpp"

#include "runtime/Session.hpp"
#include "ui/MacroPicker.hpp"
#include <Geode/binding/ButtonSprite.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace geode::prelude;

namespace flow {

AssistPopup* AssistPopup::create() {
    auto* popup = new AssistPopup();
    if (popup->init()) {
        popup->autorelease();
        return popup;
    }
    delete popup;
    return nullptr;
}

bool AssistPopup::init() {
    if (!Popup::init(410.f, 330.f, "GJ_square01.png")) return false;
    setTitle("flow assist", "goldFont.fnt", 0.9f);
    auto label = [this](float y, float scale) {
        auto* node = CCLabelBMFont::create("", "bigFont.fnt");
        node->setScale(scale);
        node->setPosition({205.f, y});
        m_mainLayer->addChild(node);
        return node;
    };
    m_macro = label(246.f, 0.32f);
    m_state = label(185.f, 0.38f);
    m_frames = label(145.f, 0.35f);
    m_offset = label(101.f, 0.32f);
    auto* hint = label(33.f, 0.25f);
    hint->setString("practice only | early inputs only | no autoplay");
    hint->setColor({170, 220, 190});
    m_controls = CCMenu::create();
    m_controls->setPosition({0.f, 0.f});
    m_mainLayer->addChild(m_controls);
    addButton("toggle click lines", 205.f, 279.f, menu_selector(AssistPopup::onIndicators));
    addButton("choose macro", 205.f, 216.f, menu_selector(AssistPopup::onChoose));
    addButton("toggle", 335.f, 185.f, menu_selector(AssistPopup::onToggle));
    addButton("-", 62.f, 145.f, menu_selector(AssistPopup::onFrames), -1);
    addButton("+", 348.f, 145.f, menu_selector(AssistPopup::onFrames), 1);
    addButton("-5ms", 80.f, 64.f, menu_selector(AssistPopup::onOffset), -1);
    addButton("zero offset", 205.f, 64.f, menu_selector(AssistPopup::onOffset), 0);
    addButton("+5ms", 330.f, 64.f, menu_selector(AssistPopup::onOffset), 1);
    m_diagnostic = label(13.f, 0.18f);
    refresh();
    return true;
}

void AssistPopup::addButton(char const* text, float x, float y, SEL_MenuHandler callback, int tag) {
    auto* sprite = ButtonSprite::create(text);
    sprite->setScale(0.55f);
    auto* button = CCMenuItemSpriteExtra::create(sprite, this, callback);
    button->setPosition({x, y});
    button->setTag(tag);
    m_controls->addChild(button);
}

void AssistPopup::refresh() {
    auto* mod = Mod::get();
    std::string lineState = mod->getSettingValue<bool>("show-click-lines") ? "lines on | " : "lines off | ";
    m_diagnostic->setString((lineState + Session::get().diagnosticSummary()).c_str());
    m_diagnostic->limitLabelWidth(380.f, 0.18f, 0.12f);
    m_macro->setString(Session::get().description().c_str());
    m_macro->limitLabelWidth(370.f, 0.32f, 0.15f);
    m_state->setString(mod->getSettingValue<bool>("enabled") ? "assistance on" : "assistance off");
    m_state->setPositionX(160.f);
    auto frames = mod->getSettingValue<int64_t>("assist-frames");
    auto ms = std::min(100.0, frames * 1000.0 / Session::get().tickRate());
    std::ostringstream text;
    text << frames << " frames | " << std::fixed << std::setprecision(1) << ms << " ms";
    m_frames->setString(text.str().c_str());
    std::ostringstream offset;
    offset << "macro offset " << std::showpos << std::fixed << std::setprecision(0)
           << mod->getSettingValue<double>("offset-seconds") * 1000.0 << " ms";
    m_offset->setString(offset.str().c_str());
}

void AssistPopup::onChoose(CCObject*) {
    if (m_picking) return;
    m_picking = true;
    WeakRef<AssistPopup> weak = this;
    WeakRef<PlayLayer> owner = PlayLayer::get();
    cgv::pickMacroFile([weak, owner](std::optional<std::filesystem::path> path) mutable {
        auto popup = weak.lock();
        if (!popup) return;
        popup->m_picking = false;
        auto layer = owner.lock();
        if (!layer || layer.data() != PlayLayer::get() || !path) return;
        auto error = Session::get().loadMacro(*path);
        if (!error.empty()) {
            FLAlertLayer::create("macro not loaded", error, "ok")->show();
            return;
        }
        Mod::get()->setSettingValue<std::filesystem::path>("macro-file", *path);
        popup->refresh();
    });
}

void AssistPopup::onIndicators(CCObject*) {
    auto* mod = Mod::get();
    mod->setSettingValue<bool>("show-click-lines", !mod->getSettingValue<bool>("show-click-lines"));
    refresh();
}

void AssistPopup::onToggle(CCObject*) {
    auto* mod = Mod::get();
    Session::get().cancelPending();
    mod->setSettingValue<bool>("enabled", !mod->getSettingValue<bool>("enabled"));
    refresh();
}

void AssistPopup::onFrames(CCObject* sender) {
    auto* mod = Mod::get();
    auto value = mod->getSettingValue<int64_t>("assist-frames");
    value = std::clamp<int64_t>(value + static_cast<CCNode*>(sender)->getTag(), 0, 6);
    Session::get().cancelPending();
    mod->setSettingValue<int64_t>("assist-frames", value);
    refresh();
}

void AssistPopup::onOffset(CCObject* sender) {
    auto* mod = Mod::get();
    auto tag = static_cast<CCNode*>(sender)->getTag();
    auto value = mod->getSettingValue<double>("offset-seconds");
    value = tag == 0 ? 0.0 : std::clamp(value + tag * 0.005, -3600.0, 3600.0);
    if (value != mod->getSettingValue<double>("offset-seconds")) Session::get().requireRestart();
    mod->setSettingValue<double>("offset-seconds", value);
    refresh();
}

}
