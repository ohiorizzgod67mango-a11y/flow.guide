#include "runtime/Session.hpp"
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

struct FlowPlayHook : Modify<FlowPlayHook, PlayLayer> {
    bool init(GJGameLevel* level, bool replay, bool skipObjects) {
        if (!PlayLayer::init(level, replay, skipObjects)) return false;
        flow::Session::get().attach(this);
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        flow::Session::get().reset(this);
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        flow::Session::get().drawHud(this);
    }

    void onQuit() {
        flow::Session::get().detach();
        PlayLayer::onQuit();
    }
};
