# flow assist v0.1.0 source alpha

macro guided input assistance for geometry dash practice

## what it does

- you supply a reference macro and still play the level yourself
- early presses and releases within the selected window wait for their macro targets
- default window is 3 macro frames, 12.5 ms for a 240 fps macro
- player 1 and player 2 have separate targets, as do the three button channels
- no matching physical input means no assisted action
- late and out of window inputs pass through, they are not retroactively fixed
- assistance only runs in practice mode, normal mode inputs pass through
- a visible label shows whether assistance is active and counts early corrections

## first use after building

1 choose a macro in the mod settings before entering a level
2 leave the early window at 3 frames
3 enter the matching level and switch to practice mode
4 play normally, start with clicks slightly early rather than late
5 try 1 frame for lighter assistance or 6 for a wider window

reenter the level after changing the macro file

frames mean macro frames, not monitor refresh frames

the buffer has a hard limit of 100 ms regardless of the frame setting

## status

this is source code, not an installable geode package

the standalone timing engine was compiled locally with address and undefined behaviour sanitizers and tested against deterministic checks and 5000 randomized sequences

the geode adapter has not been compiled or tested in gd, the included windows workflow is the next build gate, a successful build still needs in game testing

windows gd 2.2081 and geode 5.8.2 are the only declared target for this alpha

## limitations

this is an early input buffer, not full timing window expansion or late input correction

it does not rewind, prevent collisions, predict safe paths, or guarantee that a macro completes the level

short taps can become a press and release on the same simulation tick when the release arrives before a buffered press, gd handling needs testing

targets use the game state level clock, checkpoint restore alignment and start positions need verification, a manual offset is available but is not a substitute for verified timing

positive offset moves macro targets later in level time

practice mode only prevents this mod assisting a normal mode completion, this alpha does not implement a separate progress saving or networking hook, practice progress can still be saved by gd

macro files are loaded synchronously on level entry, very large files can stall loading

other input and tickrate mods can conflict, start testing without them

there is no level id compatibility check, use a macro for the exact level revision and physics setup

pending events are capped at 64 per channel, overflow flushes that channel before passing the new input through, preserving input order rather than correction timing

## build

set GEODE_SDK to a geode 5.8.2 sdk checkout and use its normal windows toolchain

or push this folder as the root of a new repository and run the included windows alpha workflow

do not upload this over the existing click guide visualizer repository

## local engine tests

```sh
g++ -std=c++23 -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc tests/TimingAssistTest.cpp src/engine/TimingAssist.cpp -o timing-tests
./timing-tests
python3 tools/check-comments.py
```

## required in game checks before sharing a binary

- normal mode never buffers inputs
- assistance indicator matches actual practice state
- early press and early release each land on the intended simulation tick
- late inputs pass through and are never claimed as corrected
- short taps do not leave jump held
- dual targets do not consume one another
- death, reset, checkpoint restore, pause, mode changes and quit do not leak pending inputs
- 120, 240 and 360 fps macros retain their expected timing under different render rates
- missing, invalid and incompatible files fail without affecting normal controls

## parser provenance

macro readers were reused from your local click guide visualizer project

gdr parsing depends on maxnuts GDReplayFormat at the pinned revision in CMakeLists.txt

supported readers include gdr, gdr2, compatible json and msgpack, and the supported zbot family binary layout, extensions alone do not guarantee compatibility
