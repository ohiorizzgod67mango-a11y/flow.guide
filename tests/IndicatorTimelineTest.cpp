#include "render/IndicatorTimeline.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    cgv::ReplayData replay;
    replay.framerate = 240;
    replay.inputs = {{480,0,0,1,true,true},{240,0,0,1,false,true},
                     {260,0,0,1,false,false},{250,0,0,2,false,true}};
    flow::IndicatorTimeline timeline;
    assert(timeline.load(replay));
    auto markers = timeline.upcoming(0,3);
    assert(markers.size() == 2);
    assert(markers[0].seconds == 1 && !markers[0].player2);
    assert(markers[1].seconds == 2 && markers[1].player2);
    assert(timeline.upcoming(1,0).size() == 1);
    assert(timeline.upcoming(1.1,0.8).empty());
    assert(timeline.upcoming(0,3,1).size() == 1);
    assert(timeline.upcoming(0,3,0).empty());
    assert(timeline.upcoming(NAN,3).empty());
    assert(timeline.upcoming(0,-1).empty());
    assert(timeline.upcoming(0,NAN).empty());
    timeline.clear();
    assert(timeline.upcoming(0,10).empty());
    replay.framerate = 0;
    assert(!timeline.load(replay));
    replay.framerate = NAN;
    assert(!timeline.load(replay));
    std::cout << "indicator timeline tests passed\n";
}
