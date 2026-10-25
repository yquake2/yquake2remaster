# Heretic II and Quake II Demo Protocol Differences

## Scope

This document compares Heretic II `.hd2` demos using protocol 51 with the
Quake II `.dm2` protocol family supported by this repository. The concrete
Heretic II example is
[`heretic2-assets/baseq2/demos/loop.hd2`](heretic2-assets/baseq2/demos/loop.hd2).

The Heretic II reference implementation is under
[`heretic2`](heretic2). It also defines
protocol 55, an extended Heretic II Remaster protocol. The example file uses
protocol 51, so protocol-55-only fields are identified separately.

The Quake II client recognizes protocols 26, 31, 32, 34, 2022, 2023, and 2024.
Those variants differ among themselves. Unless stated otherwise, "Quake II"
below means the common DM2 structure used by this client's parser, with
variant-specific handling selected by the protocol number.

## Summary

An HD2 file is not a DM2 file with a different extension. The outer file
container is the same, but the messages inside it use different opcode
numbers and substantially different state encodings.

| Area | Heretic II protocol 51 | Quake II family |
| --- | --- | --- |
| File extension | `.hd2` | `.dm2` |
| Outer container | 32-bit message length, payload, final `-1` | Same |
| Protocol number | 51 | 26, 31, 32, 34, 2022, 2023, or 2024 |
| Service opcodes | Heretic-specific table, 0 through 31 | Quake II/KEX table, 0 through 33 |
| Serverdata | Adds downloadable type and client-effects version | No equivalent fields |
| Player-state flags | Sparse 17-byte bitset, up to 129 named bits | 16-bit base mask; RR22 may add another 16 bits |
| Entity header | Sparse five-byte bitset selected by one byte | Chained bytes selected by `U_MOREBITS*` |
| Models | FlexModel node, joint, color, and swap-frame state | Up to four model indices plus protocol extensions |
| Effects | Client-effects bytecode and persistent effects | Muzzle flashes, temp entities, and KEX additions |
| Configstrings | H2 ranges, including 768 sounds | Different range starts and limits |

Consequently, extension recognition is only the file-opening step. Correct
playback requires a protocol-51 message dispatcher and protocol-specific
serverdata, frame, player-state, entity, configstring, sound, and effect
decoders.

## Demo Container

Both formats are streams of length-prefixed server messages:

```text
repeat:
    int32_le message_length
    byte[message_length] message

int32_le -1
```

The length applies to the complete payload, not to one service command. A
payload can contain several `svc_*` commands. The first byte is normally a
service opcode, and parsing continues until the payload length is consumed.

Heretic II writes this structure in `CL_Record_f()` and writes `-1` in
`CL_Stop_f()` in
[`cl_demo.c`](heretic2/src/quake2/src/client/cl_demo.c).
Quake II uses the same framing in
[`src/client/cl_main.c`](src/client/cl_main.c).

## Protocol Numbers

Heretic II defines:

```text
51  original Heretic II protocol
55  Heretic II Remaster extension
```

The constants and protocol flag definitions are in
[`qcommon.h`](heretic2/src/qcommon/qcommon.h). Protocol 55
adds fields such as `U_BBOX`; these fields must not be assumed for a protocol
51 demo.

The Quake II protocol constants are in
[`src/common/header/common.h`](src/common/header/common.h). Protocol selection
must happen before interpreting any service opcode because the value 11, for
example, means `svc_serverdata` in Heretic II but `svc_stufftext` in Quake II.

## Service Opcode Table

The two enum tables are shifted and repurposed from their first nonzero
entry. Dispatching an HD2 payload through the Quake II switch will therefore
desynchronize immediately.

| Value | Heretic II | Quake II target |
| ---: | --- | --- |
| 0 | `svc_bad` | `svc_bad` |
| 1 | `svc_layout` | `svc_muzzleflash` |
| 2 | `svc_inventory` | `svc_muzzleflash2` |
| 3 | `svc_client_effect` | `svc_temp_entity` |
| 4 | `svc_nop` | `svc_layout` |
| 5 | `svc_disconnect` | `svc_inventory` |
| 6 | `svc_reconnect` | `svc_nop` |
| 7 | `svc_sound` | `svc_disconnect` |
| 8 | `svc_print` | `svc_reconnect` |
| 9 | `svc_gamemsg_print` | `svc_sound` |
| 10 | `svc_stufftext` | `svc_print` |
| 11 | `svc_serverdata` | `svc_stufftext` |
| 12 | `svc_configstring` | `svc_serverdata` |
| 13 | `svc_spawnbaseline` | `svc_configstring` |
| 14 | `svc_centerprint` | `svc_spawnbaseline` |
| 15 | `svc_gamemsg_centerprint` | `svc_centerprint` |
| 16 | `svc_gamemsgvar_centerprint` | `svc_download` |
| 17 | `svc_levelmsg_centerprint` | `svc_playerinfo` |
| 18 | `svc_captionprint` | `svc_packetentities` |
| 19 | `svc_obituary` | `svc_deltapacketentities` |
| 20 | `svc_download` | `svc_frame` |
| 21 | `svc_playerinfo` | `svc_splitclient` |
| 22 | `svc_packetentities` | `svc_configblast` |
| 23 | `svc_deltapacketentities` | `svc_spawnbaselineblast` |
| 24 | `svc_frame` | `svc_level_restart` |
| 25 | `svc_removeentities` | `svc_damage` |
| 26 | `svc_changeCDtrack` | `svc_locprint` |
| 27 | `svc_framenum` | `svc_fog` |
| 28 | `svc_demo_client_effect` | `svc_waitingforplayers` |
| 29 | `svc_special_client_effect` | `svc_bot_chat` |
| 30 | `svc_gamemsgdual_centerprint` | `svc_poi` |
| 31 | `svc_nameprint` | `svc_help_path` |
| 32 | - | `svc_muzzleflash3` |
| 33 | - | `svc_achievement` |

The authoritative H2 enum is in
[`qcommon.h`](heretic2/src/qcommon/qcommon.h). The target
enum and matching diagnostic strings are in
[`src/common/header/common.h`](src/common/header/common.h) and
[`src/client/cl_parse.c`](src/client/cl_parse.c).

Heretic II also adds client opcode 5, `clc_startdemo`. A recording client sends
it so the server transmits persistent effects needed at demo startup. Quake II
has client opcodes 0 through 4 only.

## Serverdata

Heretic II protocol 51 encodes `svc_serverdata` as:

| Order | Type | Field |
| ---: | --- | --- |
| 1 | `int32` | protocol, 51 |
| 2 | `int32` | server count |
| 3 | `byte` | attract-loop flag |
| 4 | NUL string | game directory |
| 5 | `byte` | downloadable game-data type |
| 6 | NUL string | client-effects compatibility string |
| 7 | `int16` | player number |
| 8 | NUL string | level name or cinematic name |

This order is implemented by `CL_ParseServerData()` in
[`Heretic II cl_parse.c`](heretic2/src/quake2/src/client/cl_parse.c)
and by the demo writer in
[`cl_demo.c`](heretic2/src/quake2/src/client/cl_demo.c).

Quake II has no downloadable-type or client-effects string at this location.
In this target, RR22 instead inserts a server-FPS byte immediately after the
attract-loop flag. Reusing the Quake II parser for protocol 51 therefore
causes the H2 fields to be interpreted as `playernum` and `levelname`.

## Configstrings

Both protocols encode a configstring update as:

```text
uint16 index
NUL-terminated string
```

The index spaces are different:

| Range | Heretic II | Quake II target |
| --- | ---: | ---: |
| `CS_MODELS` | 32 | 34 |
| Maximum models | 256 | 512 |
| `CS_SOUNDS` | 288 | 546 |
| Maximum sounds | 768 | 512 |
| `CS_IMAGES` | 1056 | 1058 |
| Maximum images | 256 | 256 |
| `CS_LIGHTS` | 1312 | 1314 |
| `CS_ITEMS` | 1568 | 1826 |
| `CS_PLAYERSKINS` | 1824 | 2338 |
| Final count | 1860 | 3106 |

Heretic II reserves index 5 for `CS_LEVEL_NUMBER`, starts statusbar data at 6,
has no target-style `CS_STORY`, `CS_SKIP`, or `CS_SHADOWLIGHTS` ranges, and
reserves four `CS_WELCOME` strings at the end. Its limits are defined in
[`q_ClientServer.h`](heretic2/src/qcommon/q_ClientServer.h)
and its ranges in
[`q_Shared.h`](heretic2/src/game/src/q_Shared.h).

The target ranges are in
[`src/common/header/shared.h`](src/common/header/shared.h). Protocol-51
support needs an H2-to-internal index conversion; storing the received index
directly will put models, sounds, items, and player skins in the wrong tables.

## Frame Envelope

An H2 `svc_frame` payload contains:

```text
int32 serverframe
int32 deltaframe
byte  areabits_length
byte[areabits_length] areabits
byte  svc_playerinfo       // H2 value 21
...   H2 player-state delta
byte  svc_packetentities   // H2 value 22
...   H2 packet entities
```

H2 derives server time as `serverframe * 100`, giving a fixed 10 Hz frame
interval. It does not read Quake II's legacy suppress-count byte. The target
uses `cl.frame_msec`, reads a suppress count for all but protocol 26, and has
RR22-specific frame-rate negotiation.

The H2 implementation is `CL_ParseFrame()` in
[`cl_entities.c`](heretic2/src/quake2/src/client/cl_entities.c).
The target implementation is in
[`src/client/cl_parse.c`](src/client/cl_parse.c).

## Player-State Delta

Heretic II does not use Quake II's `PS_*` integer mask. It defines 17 flag
bytes, covering named bits 0 through 128, and compresses those flag bytes with
a three-byte nonzero mask:

```text
byte nonzero_flag_bytes[3]
for flag_byte_index in 0..16:
    if bit(nonzero_flag_bytes, flag_byte_index):
        byte flags[flag_byte_index]
for each set H2 PS_* bit:
    field payload in the fixed parser/writer order
float leveltime                    // always present
```

The flags include these groups:

| Bits | Main contents |
| ---: | --- |
| 0-7 | view angles, frame information, split XY/Z origin and velocity, forward velocity |
| 8-15 | lower/upper animation sequences and moves, autotarget, ground-plane state, idle time |
| 16-28 | movement type/time/flags, weapon flags, gravity, delta angles, remote camera, view height, FOV, render flags |
| 29-47 | fog, map and mission state, bounds, inventory changes, ground/water state, grab point, side/up velocity |
| 48-72 | entity flags, weapon/defense state, armor and weapon variants, hand/plague/skin state, death/turn state, old Z velocity |
| 73-121 | 48 individual stat bits |
| 122-128 | cinematic state, PIV, meteor count, camera delta angles, timers, quick turn, advanced staff |

Important wire differences include:

- movement origin and velocity are split into XY and Z updates;
- view angles use three 16-bit angles;
- remote-camera state, bounds, inventory changes, ground plane, water state,
  animation sequences, weapons, defenses, and game state have no direct Q2
  player-state equivalents;
- 48 stats are individual player-state flag bits rather than a trailing Q2
  stats mask;
- `leveltime` is sent every player-state update, even without a flag.

The exact flag definitions are in
[`qcommon.h`](heretic2/src/qcommon/qcommon.h), and the exact
payload order is in `CL_ParsePlayerstate()` in
[`cl_entities.c`](heretic2/src/quake2/src/client/cl_entities.c).

Quake II starts with a 16-bit `PS_*` mask. RR22 may append another 16-bit mask
when `PS_MOREBITS` is set. Its stats are decoded separately after the normal
player-state fields. The target definitions are in
[`src/common/header/common.h`](src/common/header/common.h).

## Entity Delta

H2 has 34 entity delta bits stored in five bytes. The wire header begins with
one byte whose low five bits say which of those five flag bytes follow:

```text
byte nonzero_entity_flag_bytes
for flag_byte_index in 0..4:
    if bit(nonzero_entity_flag_bytes, flag_byte_index):
        byte entity_flags[flag_byte_index]
uint8 entity_number, or uint16 when U_NUMBER16 is set
... field payloads ...
```

Quake II instead starts with one flag byte and uses `U_MOREBITS1` through
`U_MOREBITS4` to chain additional bytes. Its bit positions and payload order
are different, so translating only the header is insufficient.

Notable H2 entity fields are:

- `U_ORIGIN12` combines X and Y while `U_ORIGIN3` carries Z;
- `U_MODEL` is one byte and there are no Q2-style model2/model3/model4 fields;
- `U_SCALE` is a byte multiplied by `0.01`;
- `U_SOUND` carries both an index byte and a `sound_data` byte;
- `U_COLOR_R/G/B/A` and `U_ABSLIGHT` carry per-entity color and lighting;
- `U_FM_INFO` carries per-FlexModel-node frame, color, flags, and skin;
- `U_JOINTED` carries skeletal joint updates;
- `U_SWAPFRAME` carries a skeletal swap frame;
- `U_CLIENT_EFFECTS` carries an entity effects buffer;
- `U_USAGE_COUNT`, `U_CLIENTNUM`, `U_BMODEL`, and `U_ENT_FREED` have no
  direct Q2 entity-state equivalents;
- `U_BBOX` is protocol-55-only.

Coordinates use signed shorts at one-eighth-unit precision:

```text
wire = (int16)(position * 8)
position = wire * (1 / 8)
```

Byte angles use 256 steps per revolution and 16-bit angles use 65536 steps.
These scalar conversions match legacy Quake II, but the surrounding field
selection does not.

Header writing is in `MSG_WriteEntityHeaderBits()` and entity writing is in
`MSG_WriteDeltaEntity()` in
[`netmsg_write.c`](heretic2/src/qcommon/netmsg_write.c).
The matching parser is in
[`cl_entities.c`](heretic2/src/quake2/src/client/cl_entities.c).

## Sound and Effects

H2 `svc_sound` uses this order:

```text
byte   flags
uint16 sound_index
if SND_PRED_INFO: byte event_id, float leveltime
if SND_VOLUME: byte volume
if SND_ATTENUATION: byte attenuation
if SND_OFFSET: byte millisecond_offset
if SND_ENT: uint16 packed_entity_and_channel
if SND_POS: int16 position[3]
```

The two-byte sound index is required because H2 supports 768 sounds. Quake II
protocol variants use different index widths and do not have H2's
`SND_PRED_INFO` payload.

H2 replaces Quake II muzzle-flash and temp-entity messages with a client
effects system. Relevant messages and fields include:

- `svc_client_effect` for normal client effects;
- `svc_demo_client_effect` for persistent effects captured at demo startup;
- `svc_special_client_effect`, which includes a 16-bit payload size;
- `U_CLIENT_EFFECTS` inside entity deltas;
- `U_FM_INFO`, joint data, and swap-frame data for animated FlexModels.

`svc_demo_client_effect` and `svc_special_client_effect` can be bounded by
their size fields. Ordinary `svc_client_effect` is parsed by the effects
module and cannot be skipped safely without understanding that effect's wire
format. Visual omission and wire consumption should therefore be separate:
an initial implementation may ignore rendering only after it can consume the
complete payload correctly.

### Reusable Quake II effect building blocks

Heretic II's named effects are registered in
[`ce_Effects.c`](<hereticII/src/client effects/src/ce_Effects.c>) and implemented
across its `Effects` directory. Several effect *primitives* have close Quake II
counterparts and can be rendered with existing client facilities after the H2
effect ID and its arguments have been decoded:

| Heretic II effect family | Quake II building block | Reuse boundary |
| --- | --- | --- |
| `FX_EXPLOSION1/2`, `FX_ARMOR_HIT`, impact puffs | Explosion entities and impact particles (`TE_*` cases in `cl_tempentities.c`) | Reuse transient sprites/models, particles, and light presentation; H2 effect timing, models, and spell-specific visuals are not equivalent. |
| `FX_BLOOD`, `FX_BLOOD_TRAIL`, `FX_GIB_TRAIL`, `FX_DEBRIS` | Blood/spark temp entities and particle effects | Reuse particle allocation, motion, and rendering. Material-specific H2 debris and blood decals/splats need H2 data and behavior. |
| `FX_SPARKS`, `FX_DUST`, `FX_DUST_PUFF`, `FX_MIST`, smoke/fire-like bursts | Q2 spark, steam, and generic particle effects | Reuse transient particle mechanics and optionally generic appearances; H2 colors, scale, lifetime, and persistent emitters must come from H2 state. |
| Player teleport effects | `CL_TeleportParticles()` and teleport temp-entity rendering | Reuse a generic teleport burst only; H2 portal/persistent effects and their attachment/lifetime are not represented by the Q2 event. |
| Water splash, entry splash, and bubbles | Splash temp entities and `CL_BubbleTrail()` / `CL_BubbleTrail2()` | Reuse splash/bubble particles. H2 ripples, wakes, and surface-following effects need H2-specific logic. |
| `FX_LIGHTNING` and `FX_ROPE` | Q2 beam and lightning rendering in `cl_tempentities.c` | Reuse beam geometry/rendering only. H2 endpoint IDs, update rules, width, models, and ownership must be decoded and maintained separately. |
| Effects with attached colored/decaying lights | Q2 dynamic-light allocation and rendering in `cl_effects.c` | Reuse the generic light facility where its color/intensity/lifetime model suffices; H2 light fades and effect ownership may require adapting the H2 values. |

Q2 provides useful lower-level facilities in
[`cl_particles.c`](src/client/cl_particles.c),
[`cl_tempentities.c`](src/client/cl_tempentities.c), and
[`cl_effects.c`](src/client/cl_effects.c). H2's particle representation and
effect lifecycle are richer and distinct (see
[`ce_Particles.c`](<hereticII/src/client effects/src/ce_Particles.c>) and
[`ce_DLight.c`](<hereticII/src/client effects/src/ce_DLight.c>)); this is a
mapping of rendering capabilities, not a claim that the implementations or
wire formats are interchangeable.

Do not map H2 spell, monster, pickup, player-persistent, or environmental
effects directly to similarly shaped Q2 temp entities. Their models, assets,
parameters, and entity relationships are game-specific. Also, this source
inventory does not establish which named effects occur in `loop.hd2`: the
demo's ordinary client-effect payloads must first be decoded using the H2
effect registry and argument formats. Even when a visual is omitted or
replaced, its H2 payload must still be fully consumed to preserve message
alignment.

## Observations from `loop.hd2`

The following values were read directly from the example file, without using
the H2 game runtime:

| Property | Value |
| --- | ---: |
| File size | 75,811 bytes |
| Length-prefixed payloads | 514 |
| Final `-1` marker offset | 75,807 |
| First payload size | 2,438 bytes |
| Protocol | 51 |
| Server count | 505,248,125 |
| Attract loop | 1 |
| Game directory | empty string |
| Downloadable type | 0 |
| Client-effects string | `Heretic II v1.06` |
| Player number | 0 |
| Level name | `Silverspring Docks` |

Counting only the first opcode of each outer payload gives:

| Leading opcode | Payload count |
| --- | ---: |
| `svc_serverdata` | 1 |
| `svc_configstring` | 27 |
| `svc_spawnbaseline` | 4 |
| `svc_client_effect` | 8 |
| `svc_levelmsg_centerprint` | 2 |
| `svc_frame` | 468 |
| `svc_special_client_effect` | 4 |

These are payload-leading counts, not total command counts, because one
payload can contain multiple commands. The first payload starts with
serverdata and continues immediately with `svc_configstring` commands.

## Playback Support in This Fork

Protocol-51 parsing is implemented in
[`src/client/cl_parse.c`](src/client/cl_parse.c), with coordinate and angle
decoding in [`src/common/movemsg.c`](src/common/movemsg.c), configstring
translation in [`src/common/protocol.c`](src/common/protocol.c), and the H2
sound limit in [`src/common/header/shared.h`](src/common/header/shared.h).
The implementation currently:

- detects H2 serverdata, consumes its extra fields, and translates H2
  configstring ranges;
- consumes H2 sound prediction metadata and uses H2 coordinate/angle
  encodings;
- parses the H2 frame envelope, sparse player-state flags, sparse entity
  headers, common entity fields, and baseline deltas;
- consumes client-effect argument formats for all 145 registered effect IDs,
  renders coarse Quake II analogues for explosions, blood, sparks, teleport,
  and water-like effects, and logs decoded effects without an analogue;
- consumes FlexModel, joint, and attached entity-effect payloads while
  reporting that their rendering is unavailable; and
- decodes/logs H2 message-table notifications that cannot be translated to
  Quake II text.

This is wire-format support, not a Heretic II renderer. H2 FlexModel
animations, skeletal joints, attached and persistent client effects, message
tables, inventories, and game-specific player-state fields are not presented
as native H2 visuals or UI. Protocol 55-only `U_BBOX` is consumed defensively
but has no supported rendering path.

The implementation has been compiled, and the effect name/format tables have
been checked to contain 145 entries each. A runtime test of `loop.hd2` with
the software renderer loaded `maps/ssdocks.bsp` and decoded frames through
server frame 888. The earlier `unknown fileid` error did not indicate an
unsupported BSP: renderer registration ran before the collision loader had
loaded and converted the map.

Unlike `map ssdocks`, which loads the collision map on the server first,
demo playback initially has no world map cached. H2 attract-loop precaching
now uses the local-demo path: load the collision map, register sounds, then
prepare the renderer, without waiting for downloads of H2 resources through
Quake II filename conventions. H2 configstrings 30 and 31 retain their
max-client and checksum meanings instead of being shifted with statusbar
strings.

The test used `+set paused 0 +set cl_loadpaused 0 +demomap loop.hd2`.
After the demo server shut down, the client reported
`CL_ParseStartSoundPacket: unexpected message end` while parsing subsequent
server messages. End-of-demo handling remains unvalidated, as does visual
fidelity; successful map loading does not establish complete H2 rendering.

The parser keeps protocol-specific wire decoding separate from the Quake II
field decoders where their layouts differ. This helps preserve byte alignment
while allowing common state and transient visual primitives to reuse existing
Quake II facilities.

## Primary Source Map

- H2 protocol constants, opcodes, and delta bits:
  [`heretic2/src/qcommon/qcommon.h`](heretic2/src/qcommon/qcommon.h)
- H2 limits:
  [`heretic2/src/qcommon/q_ClientServer.h`](heretic2/src/qcommon/q_ClientServer.h)
- H2 configstrings and coordinate macros:
  [`heretic2/src/game/src/q_Shared.h`](heretic2/src/game/src/q_Shared.h)
- H2 demo writer:
  [`heretic2/src/quake2/src/client/cl_demo.c`](heretic2/src/quake2/src/client/cl_demo.c)
- H2 server-message and serverdata parser:
  [`heretic2/src/quake2/src/client/cl_parse.c`](heretic2/src/quake2/src/client/cl_parse.c)
- H2 frame, player-state, and entity parser:
  [`heretic2/src/quake2/src/client/cl_entities.c`](heretic2/src/quake2/src/client/cl_entities.c)
- H2 entity writer:
  [`heretic2/src/qcommon/netmsg_write.c`](heretic2/src/qcommon/netmsg_write.c)
- Quake II protocol constants and masks:
  [`src/common/header/common.h`](src/common/header/common.h)
- Quake II limits and configstrings:
  [`src/common/header/shared.h`](src/common/header/shared.h)
- Quake II demo/message parser:
  [`src/client/cl_parse.c`](src/client/cl_parse.c)
