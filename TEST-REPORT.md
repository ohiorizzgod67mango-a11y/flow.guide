# v0.1.2 input reliability checks

## reproduced in v0.1.1

- press at frame 97 and release at 98, target at 100, both queued edges drained together at frame 100, eliminating the hold
- press at 97, release at 109, macro targets at 100 and 110, physical hold was 12 frames but output hold was 10 frames
- session cancellation unconditionally injected releases on all six channels, including channels without a held output

these were reproduced or verified in the local previous source, they are not a diagnosis of every reported death in gd

## new engine results

compiled locally with g++ c++23, warnings as errors, address sanitizer and undefined behaviour sanitizer

- deterministic short tap regression passed
- independent release snapping regression passed
- late press delivery preserves the paired release delay
- repeated pump calls at one timestamp cannot collapse queued edges
- player 1 and player 2 remain separate
- duplicate active presses do not consume additional macro targets
- no physical action produces no scheduled output
- late and out of window presses remain unchanged
- disabling before an early tap finishes does not inject a pending press on release
- disabling while held reconciles to the physical hold, then passes the physical release
- reset clears pending inputs
- direction buttons bypass jump assistance
- invalid assistance windows fall back to unchanged physical input
- invalid rate, nonfinite time and invalid button checks passed
- 100000 inactive events across all six channels preserved exact event timestamps, states and identities
- 5000 randomized sequences at macro rates 120, 240, 360 and 1000 passed order, count, final queue and hold duration assertions on a 240 hz scheduler

## native bridge checks

short tap scheduling, bypass identity and invalid request guards passed through the workshop native bridge

the active engine source fingerprint was b3bfaa4941f7

## browser checks

chromium automation passed

- native short tap demo delivered a nonzero 4.17 ms hold
- bypass demo produced zero corrections
- wave demo reached the native engine
- live keyboard down and up reached the engine
- inactive raw and assisted mock trajectories matched exactly
- pointer down and up reached the native engine
- mobile layout had no horizontal overflow at 390 px
- no browser runtime errors during those checks

the gd adapter now returns unchanged events directly to the original hook path rather than redispatching them recursively, this was reviewed in source, not tested inside gd

## not verified

- compiling v0.1.2 against the real geode sdk
- gd hook phase and timestamp alignment
- real cube, wave, ship or robot behaviour
- checkpoints, portals, pause, death and reset interactions inside gd
- conflicts with cbf, tickrate modifications or other input hooks

## required real gd checks

compare fully disabled mod, loaded mod with toggle off, and zero frame mode on the same section

then enable a small early window and inspect the delivery delay and hold behaviour, a queued tap must span distinct physics steps

check short taps, long holds, rapid successive presses, player 2, death, checkpoint restore and turning assistance off while held

keep normal mode unassisted and verify the practice gate

this remains a testing alpha, not a confirmed fix for every gameplay issue

## v0.1.3 indicator addition

indicator timeline tests passed for ordering, press filtering, dual player flags, inclusive boundaries, empty windows, output limits and invalid rates or times

rendering uses object layer children and posForTime, with green and orange thin vertical segments, rendering is uncompiled and unverified in gd

the v0.1.2 scheduler is unchanged by this addition
