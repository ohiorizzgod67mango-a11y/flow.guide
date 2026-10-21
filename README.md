# flow assist v0.1.3 source alpha

macro guided early input assistance, with an input reliability rework and a playable test workshop

## thin click lines

upcoming macro jump presses now draw as thin vertical lines in the actual mod, green for player 1 and orange for player 2

pause and click flow, choose a macro and restart the attempt, the toggle click lines button controls the overlay independently of assistance

lines work with assistance off and in normal mode, they do not press inputs

default width is one level unit, adjust click line width and click line lookahead in geode settings

markers use the same macro offset as assistance, horizontal positions come from posForTime and vertical lines are centred on the players current height

they are timing references, not predicted landing positions, wave trajectories or safe collision windows

at most 512 upcoming markers are drawn per update

the timeline selection tests pass locally, actual rendering still needs a geode build and gd test

## current status

the previous build loaded in gd but made normal gameplay worse in your testing

this patch fixes reproduced scheduler regressions, the new gd adapter has not been compiled or tested in game

no late correction, rollback or automatic macro playback is included

## corrected input policy

an early jump press inside the selected window waits for the matching macro press

its release follows the same delay, preserving your physical hold duration instead of independently snapping to a macro release

queued press and release transitions cannot both drain on one simulation timestamp

when a queued press is delivered late, its paired release moves by that extra delay too

holds may gain up to one physics tick from rounding in the tested nonoverlapping cases, congested input bursts can gain more because order is preserved

this does not reproduce the macros release timing, it preserves your own holds, that tradeoff is deliberate

buttons 2 and 3 pass through without macro snapping

late and out of window presses pass through

## pause menu

pause a level and click flow

choose a local macro, restart the attempt to apply it, then use practice mode

use toggle to enable assistance, the frame control adjusts the early press window from 0 to 6 macro frames

new installs default to assistance off, existing settings can retain their previous value

0 frames is a bypass, the offset control moves the reference timeline in 5 ms steps and requires restarting the attempt when changed

keep offset at zero unless a clock mismatch is verified

changing settings cancels queued assistance and reconciles only channels whose output state differs from physical state, it no longer sends six unconditional releases

switching off while a key is held can emit a state synchronization edge, after that transition bypass preserves physical events unchanged

## diagnostics

the hud shows corrected press count, pending input count and the last measured queued delivery delay

the pause popup shows the last routing decision and queue information

timing values use the gd game state level clock, whether that hook phase matches macro frames still needs real gd verification

## workshop

see workshop/README.md for the playable cube and wave test sandbox

it calls this exact c++ engine through a small native bridge, not a second javascript assistance implementation

its physics are simplified and original, not geometry dash physics, a successful mock course does not verify gd compatibility

## building

push this folder as the root of your flow.guide repository, the included windows workflow builds and uploads the .geode artifact

target is gd 2.2081 and geode 5.8.2 on windows

this zip is source, not an installable mod

## tests

```sh
g++ -std=c++23 -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc tests/TimingAssistTest.cpp src/engine/TimingAssist.cpp -o timing-tests
./timing-tests
python3 tools/check-comments.py
```

see TEST-REPORT.md for local results and outstanding gd checks

## macro readers

reused from your click guide visualizer project, includes gdr, gdr2, compatible json and msgpack and the supported zbot binary layout

gdr parsing uses maxnuts GDReplayFormat at the pinned CMake revision

macro files load synchronously, large files can stall loading, there is no level id compatibility guarantee

practice mode is the runtime gate, this is not a separate save prevention or networking system
