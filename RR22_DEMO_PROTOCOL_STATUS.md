# ReRelease Demo Protocol Status

## Current State

RR22 demo protocol support is in progress. The current baseline is commit `2efb84dd` (`continue parse`), following the initial work in `0e4d54a3` (`RR22`). The parser changes are in [src/client/cl_parse.c](src/client/cl_parse.c); `q2proto/` is being used as a protocol reference and has not been modified for this work.

## Implemented So Far

- RR22 entity parsing handles `U_MODEL16`, the extended effects field, 32-bit solid values, compact coordinates for nonsolid entities, float angles, and packed sound fields.
- RR22 player-state parsing handles float movement origins/velocities and angles, short view offsets, packed weapon-frame flags, and the 64-stat layout.
- RR22 stats are read in wire order: each 32-bit stat mask is followed by that group's values.
- KEX extended player-state flags, damage-blend and team-ID payloads are consumed; gun index/skin is masked to the 13-bit index.
- Entity deltas support the fifth flag byte, KEX scale/instance/owner/oldframe fields, and RR22 solid-dependent coordinate precision.
- `svc_splitclient`, `svc_locprint`, `svc_damage`, and `svc_muzzleflash3` are dispatched; sound entity/channel fields and positional coordinates use KEX demo encodings.
- KEX temporary-entity additions are decoded without colliding with the RTX `TE_FLARE` value.
- RR22 gun-frame flags are kept unsigned, and legacy gun offset/angle bytes are not consumed for RR22.

All seven bundled demos have been replayed with `timedemo` enabled and no parser errors. They produced frame summaries from 962 frames (`demo2.dm2`) to 2,040 frames (`rdemo1.dm2`).

## Current Validation Frontier

No parser failure is currently known in the bundled RR22 demos. Continue by replaying all seven after changes to shared frame, player-state, sound, or temporary-entity decoding.

## Reproduction

From the repository root, build with the native Makefile (CMake Tools has no configured CMake cache here):

```sh
make -j2 DEBUG=1
```

Run one demo with protocol tracing:

```sh
./debug/quake2 -datadir rerelease-assets +set cl_shownet 2 +demomap demo1.dm2
```

`demomap` prepends `demos/` itself, so pass the filename only. The bundled files are `demo1.dm2`, `demo2.dm2`, `rdemo1.dm2`, `rdemo2.dm2`, `xdemo1.dm2`, `xdemo2.dm2`, and `xdemo3.dm2` in `rerelease-assets/baseq2/demos/`.

To bound a headless/automated replay and keep the trace for inspection:

```sh
timeout --signal=TERM 40s ./debug/quake2 -datadir rerelease-assets +set vid_renderer gl3 +set vid_pauseonfocuslost 1 +set cl_shownet 2 +demomap demo1.dm2 > /tmp/rr22-demo1.log 2>&1
```

Set `vid_pauseonfocuslost` to `1` for headless or background playback; otherwise losing window focus pauses the local demo server. The bounded trace may exit 124 by design. The timedemo must run to its `nextdemo quit` completion to produce a valid benchmark.

## Build And Worktree Notes

- `make -j2 DEBUG=1` successfully compiled and linked the current client and renderer modules.
- `Build_CMakeTools` was attempted but could not load a CMake cache; use the native Makefile unless a CMake build is configured.
- Source diagnostics for the touched parser, effects, temp-entity, and protocol files reported no errors.
- The worktree also contains untracked `DoomViewer/`, `bsp_viewer/`, `q2proto/`, `rerelease-assets/`, and `rerelease/` directories. These were present during this work and should be preserved.