#include "runtime/Session.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

struct FlowInputHook : Modify<FlowInputHook, GJBaseGameLayer> {
    void handleButton(bool down, int button, bool player1) {
        auto* play = typeinfo_cast<PlayLayer*>(static_cast<GJBaseGameLayer*>(this));
        if (play && flow::Session::get().input(play, down, button, player1)) return;
        GJBaseGameLayer::handleButton(down, button, player1);
    }

    void processCommands(float dt, bool halfTick, bool lastTick) {
        auto* play = typeinfo_cast<PlayLayer*>(static_cast<GJBaseGameLayer*>(this));
        if (play) flow::Session::get().tick(play);
        GJBaseGameLayer::processCommands(dt, halfTick, lastTick);
    }
};
