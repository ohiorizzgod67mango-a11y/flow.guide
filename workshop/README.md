# flow input workshop

a playable original cube and wave sandbox for input reliability testing

## run locally

requires linux or wsl with python 3 and g++ supporting c++23

from the project root

```sh
python3 workshop/server.py --port 8080
```

open http://localhost:8080 in your browser

the server compiles the same src/engine/TimingAssist.cpp used by the mod and runs the native regression suite before listening

standalone windows python is not supported by this linux shared library launcher, use wsl or the supplied live preview

## controls

play live starts an 8 second course, use space or hold the canvas

mint shows assisted control, pink shows raw control, both receive the same physical events

short tap demo replays a prerecorded early one-frame tap, early hold demo replays a longer reference trace

the demos are explicit test recordings, the mod itself does not autoplay

switch between cube and wave, macro fps and simulation fps to inspect rounding

changing a setting resets the experiment rather than altering an existing replay

export trace downloads the physical events, emitted events, targets and diagnostics

## json reference format

```json
{"framerate":240,"inputs":[{"frame":240,"button":1,"player2":false,"down":true}]}
```

this lab accepts a small player 1 jump-press json schema, not the full mods gdr or other binary readers

loading a real levels reference does not turn this mock course into that level

## limitations

no gd assets, exact physics, portals, real collision engine, level parsing or gd hook phase simulation

network response latency affects visual feedback, it does not change the timestamped native scheduling replay

one physics tick can round a hold upward, very dense bursts may be delayed further to maintain event order

a pass here is evidence about the scheduler only

## browser smoke tests

optional, requires playwright with chromium and its system libraries

```sh
python3 workshop/browser-test.py
```

run against the server on port 8080
