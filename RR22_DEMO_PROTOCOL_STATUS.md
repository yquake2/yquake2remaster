# ReRelease Demo Protocol Status

## RR22 and RR23 Differences

The q2repro protocol reference distinguishes protocol `2022` (`PROTOCOL_VERSION_KEX_DEMOS`) from protocol `2023` (`PROTOCOL_VERSION_KEX`):

- Both use 32-bit entity solid values, float entity angles, and float player movement origins, velocities, and angles.
- RR22 demo recordings use compact 12.3 fixed-point entity coordinates when the entity solid is zero; nonzero solids use float coordinates. RR23 uses float entity coordinates for all entities.
- Both protocols use the KEX player-state layout, including 16-bit movement time/flags, packed weapon-frame fields, and 64 stats.
- RR23 recordings can include compressed config-string and baseline blasts, level-restart config-string lists, and additional KEX messages such as POI and help-path updates.

The key RR23 desynchronization was reading `U_SOLID` as a 16-bit value even though q2repro reads it as 32-bit for both protocols. RR23 entity angles and several player-state fields were also being rejected or decoded with legacy formats.

## Implemented

- RR22 and RR23 entity deltas read 32-bit solid values. RR22 keeps solid-dependent coordinate precision; RR23 reads float coordinates throughout.
- RR23 entity angles, player movement origins/velocities, movement time/flags, view offsets/angles, kick angles, weapon-frame data, and stats use the KEX encodings.
- ReRelease gun index/skin is masked to its 13-bit model index.
- Compressed KEX config-string and baseline blasts are inflated with the bundled miniz library and decoded through the existing config-string and baseline parsers.
- KEX level-restart config-string lists, POI, help-path, waiting-for-players, bot-chat, and achievement payloads are consumed so playback remains synchronized.
- RR22 and RR23 player-state and entity parsing share the encodings that q2repro defines identically; RR22-only coordinate compression remains isolated to protocol 2022.

`q2repro/q2proto` was used as a protocol reference and was not modified.

## Validation

Build from the repository root:

```sh
make -j2 DEBUG=1
```

Run a requested recording with protocol tracing and bounded playback:

```sh
timeout --signal=TERM 240s ./debug/quake2 -datadir rerelease-assets \
  +set vid_renderer gl3 +set vid_pauseonfocuslost 1 +set cl_shownet 2 \
  +set timedemo 1 +set nextdemo quit +demomap tutorial.dm2
```

`demomap` prepends `demos/`. The checked-in asset is named `tutorial-nongod.dm2` (not `tutorial-nogod.dm2`).

| Recording | Result |
| --- | --- |
| `tutorial.dm2` | 14,389 frames; no parser errors |
| `tutorial-nongod.dm2` | 6,142 frames; no parser errors |
| `q64.dm2` | 28,009 `svc_frame` messages decoded; compressed config/baseline blasts and both level restarts parsed; no parser errors |

`q64.dm2` changes maps during playback, so the final timedemo summary reports zero frames after the last disconnect even though the trace contains 28,009 decoded frame messages. The game exits cleanly with `nextdemo` set to `quit`.

RR23 playback also succeeds with `cl_shownet 0`; tracing is only needed when inspecting message-by-message output.
