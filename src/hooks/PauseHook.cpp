#include "ui/PauseEntry.hpp"
#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

struct FlowPauseHook : Modify<FlowPauseHook, PauseLayer> {
    void customSetup() {
        PauseLayer::customSetup();
        flow::PauseEntry::attach(this);
    }
};
