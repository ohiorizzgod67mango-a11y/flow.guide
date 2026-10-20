#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

namespace flow {

class AssistPopup : public geode::Popup {
public:
    static AssistPopup* create();

protected:
    bool init();

private:
    void refresh();
    void onChoose(cocos2d::CCObject*);
    void onToggle(cocos2d::CCObject*);
    void onFrames(cocos2d::CCObject* sender);
    void onOffset(cocos2d::CCObject* sender);
    void addButton(char const* text, float x, float y, cocos2d::SEL_MenuHandler callback, int tag = 0);
    cocos2d::CCLabelBMFont* m_macro = nullptr;
    cocos2d::CCLabelBMFont* m_state = nullptr;
    cocos2d::CCLabelBMFont* m_frames = nullptr;
    cocos2d::CCLabelBMFont* m_offset = nullptr;
    cocos2d::CCMenu* m_controls = nullptr;
    bool m_picking = false;
};

}
