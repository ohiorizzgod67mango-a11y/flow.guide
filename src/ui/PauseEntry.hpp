#pragma once

#include <Geode/Geode.hpp>

namespace flow {

class PauseEntry : public cocos2d::CCNode {
public:
    static void attach(PauseLayer* layer);

private:
    void onOpen(cocos2d::CCObject*);
};

}
