# Q2Test Demo Protocol 25

This document records the wire format needed to play the bundled
`q2test/baseq2/demos/demo1.dm2` recording. It is based on the protocol
behavior visible in the Q2Test reference executable and on playback traces from
the Remaster client. It describes data formats, not the reference executable's
implementation.

## Message Stream

The demo identifies itself as protocol 25 in its server-data message. Service
message opcodes use the Q2Test numbering below:

| Opcode | Message |
| ---: | --- |
| 1 | no-op |
| 2 | disconnect |
| 3 | reconnect |
| 4 | sound |
| 5 | print |
| 6 | stuff-text |
| 7 | server data |
| 8 | config string |
| 9 | entity baseline |
| 10 | temporary entity |
| 11 | center print |
| 12 | muzzle flash |
| 13 | download |
| 14 | player info |
| 15 | choke count |
| 16 | packet entities |
| 17 | delta packet entities |
| 18 | frame |
| 19 | second muzzle-flash form |
| 20 | inventory |
| 21 | layout |

The stream ends at the message boundary. The bundled demo begins with protocol
25 server data and then config-string messages, followed by entity baselines
and frames.

Opcode 15 (choke count) is followed by one byte. Playback consumes and ignores
that byte; it is not a service opcode that can be translated to a no-op.

Opcodes 14, 16 and 17 are only valid inside a frame message; outside of it
they are a fatal "out of place frame data" error.

Server data is: 32-bit protocol (25), 32-bit server count, attract-loop byte,
game-directory string, 16-bit player number and level-name string. There is no
tick-rate byte.

Config-string indexes are 16-bit, below 1312: models at 32, sounds at 288,
images at 544, light styles at 800 and items at 1056. This matches the later
protocol-26 layout up to that limit.

The sound message uses the protocol-26 layout with byte sound indexes: flags
byte, sound byte, then volume (`0x01`), attenuation (`0x02`), time offset
(`0x10`), entity/channel short (`0x08`) and position (`0x04`).

Muzzle-flash opcodes carry a 16-bit entity number and a flash byte. The
player flash numbers (0-14, silenced bit `0x80`) match the release values.

The inventory message carries 256 signed 16-bit counts.

The layout message toggles the layout: if no layout is shown, the string is
shown; otherwise the current layout is cleared and the string is ignored.
There is no layout stat; the layout is drawn whenever one is set.

## Entity Updates

Protocol 25 entity headers differ from the later Q2 demo headers. The first
little-endian 16-bit word contains the entity number in bits 6 through 15 and
six header flags in bits 0 through 5. The entity number is therefore
`word >> 6`; it is not carried in a separate byte or short.

Bits 4 and 5 of the low six flags indicate how many extension bytes follow:

| Bit 4 | Bit 5 | Extension bytes |
| --- | --- | ---: |
| clear | clear | 0 |
| set | clear | 1 |
| clear | set | 2 |
| set | set | 3 |

Each extension byte contributes the next eight flag bits, beginning at bit 8.
The wire flags map as follows:

| Wire flag | Meaning |
| ---: | --- |
| `0x000001` | origin X |
| `0x000002` | origin Y |
| `0x000004` | origin Z |
| `0x000008` | 8-bit frame |
| `0x000100`, `0x000200`, `0x000400` | angle X, Y, Z |
| `0x000800` | model 1 |
| `0x001000`, `0x002000`, `0x004000` | render effects, skin, effects (8-bit forms) |
| `0x010000`, `0x020000`, `0x040000` | 16-bit frame, render effects, effects |
| `0x080000`, `0x100000`, `0x200000` | models 2, 3, 4 |
| `0x400000` | old origin |
| `0x800000` | 32-bit skin value |
| `0x1000000` | sound |

There is no remove flag; the header word 0 ends the entity list.

Fields are read in this order: models 1-4, frame, skin, effects, render
effects, origin, angles, old origin, sound. Each width is independent: when
both the 8-bit and wide flag of a field are set, the byte is read first and
then the wide value (16-bit for frame, effects and render effects, 32-bit for
skin), and the wide value wins. Paired flags do not form a single 32-bit
value as they do in later protocols.

Coordinates and old-origin components are signed 16-bit values in eighth-unit
increments. Angles are signed bytes scaled by `360 / 256`. Model indexes and
sound indexes are bytes. When the old-origin flag is absent, the old origin is
the origin of the state being deltaed from.

Entity effect bits differ from the release values:

| Q2Test bit | Meaning | Release equivalent |
| ---: | --- | --- |
| `0x0001` | rotate | `EF_ROTATE` |
| `0x0002` | gib trail | `EF_GIB` |
| `0x0008` | blaster trail and light | `EF_BLASTER` |
| `0x0010` | rocket trail and light | `EF_ROCKET` |
| `0x0020` | grenade trail | `EF_GRENADE` |
| `0x0040` | flies particles | `EF_FLIES` |
| `0x0080` | do not interpolate this frame | none |
| `0x0100` | BFG particles and green light | `EF_BFG` |
| `0x0200`, `0x0400` | translucent colour shells | colour unknown |
| `0x0800` | animate frames 0-1 at 2 Hz | `EF_ANIM01` |
| `0x1000` | animate frames 2-3 at 2 Hz | `EF_ANIM23` |
| `0x2000` | fast 0-1 frame toggle | none (closest `EF_ANIM01`) |
| `0x4000` | animate all frames at 2 Hz | `EF_ANIM_ALL` |
| `0x8000` | animate all frames at 10 Hz | `EF_ANIM_ALLFAST` |

Render-effect bits up to `0x80` (`RF_BEAM`) match the release values.

## Temporary Entities

Service opcode 10 is followed by a one-byte temporary-entity subtype. Subtype
numbers differ from the release values. `pos` is three signed 16-bit
components in eighth-unit increments, `dir` is one direction-table byte.
Any other subtype is a fatal error.

| Subtype | Payload | Release equivalent |
| ---: | --- | --- |
| 2 | pos, dir | `TE_GUNSHOT` |
| 12 | pos, dir | `TE_BLOOD` |
| 14 | pos, dir | `TE_BLASTER` |
| 15 | pos, end pos, dir | `TE_RAILTRAIL` (extra dir) |
| 16 | pos, dir | `TE_SHOTGUN` |
| 17 | pos | `TE_ROCKET_EXPLOSION` |
| 18 | pos | `TE_EXPLOSION1` |
| 19 | pos | `TE_GRENADE_EXPLOSION` |
| 20 | pos | `TE_EXPLOSION2` |
| 21 | pos, dir | `TE_SPARKS` |
| 22 | count byte, pos, dir, colour byte | `TE_SPLASH` |
| 23 | pos, end pos | `TE_BUBBLETRAIL` |
| 24 | pos, dir | `TE_SCREEN_SPARKS` |
| 25 | pos, dir | `TE_SHIELD_SPARKS` |
| 26 | pos, dir | `TE_BULLET_SPARKS` |
| 27 | count byte, pos, dir, colour byte | `TE_LASER_SPARKS` |
| 28 | 16-bit entity, start pos, end pos | `TE_PARASITE_ATTACK` |

Subtypes 16 and 19 must not be decoded with the later parasite-beam or
medic-cable payloads.

Explosions 17-20 are drawn as sprites, not the release `r_explode` model,
which q2test does not ship. Subtypes 17 and 19 use `sprites/s_explo3.sp2`
(8 frames); 18 and 20 use `sprites/s_explo2.sp2` (13 frames) raised by 64
units. All four are fullbright, with a 350-unit (1, 0.5, 0.5) light that fades
with the animation. Only 19 and 20 also spawn explosion particles.

## Frame Messages

A frame message has a Q2Test-specific envelope:

1. Advance the local server-frame number by one.
2. Read a 32-bit time value. The reference playback interprets it in tenths of
   a second; the Remaster client's millisecond clock should use `value * 100`.
3. Read one byte giving the area-bit count, followed by that many area bytes.
4. Read an opcode. If it is player info (14), read the player-state payload
   and then the next opcode. Player info is optional; when it is absent the
   previous player state is kept.
5. The opcode must now be packet entities (16), followed by its entity deltas.
   Anything else is a fatal error.

There is no transmitted server-frame number, delta-frame number, or suppression
count in this envelope. Every entity delta is taken against that entity's
baseline, never against the previous frame. Only entities listed in the frame
exist in it; there is no carry-over of unchanged entities.

## Player State

The player-info payload begins with a 16-bit flags value, followed by fixed
data:

- Three signed 16-bit origin components in eighth-unit increments.
- Three signed 16-bit view angles, followed by three signed 16-bit delta
  angles. Both use the standard 16-bit angle scale. There is no velocity.

The flags select optional data, read in this order:

| Flag | Payload |
| ---: | --- |
| `0x04` | view-weapon model index, second view-weapon model index (bytes) |
| `0x08` | view-weapon frame, second view-weapon frame (bytes), then three gun-offset and three gun-angle signed bytes in quarter-unit increments |
| `0x01` | four blend bytes, scaled by `1 / 255` |
| `0x02` | one FOV byte; default FOV is 90 when absent |
| `0x80` | one render-definition flags byte |

Every optional field that is absent is reset to zero (FOV to 90); nothing is
carried over from the previous frame. The second view-weapon model is drawn
over the first when non-zero; the release protocol has no equivalent.

Other flag bits are not followed by data: `0x40` disables view interpolation
for this frame, and `0x20` (forced on during demo playback) selects the
transmitted view angles instead of delta angles plus local input.

After those fields, a 32-bit stats mask is read. Each set bit, from bit 0 to
bit 31, is followed by one signed 16-bit stat value; unset stats are zero.

The bundled `demo1.dm2` trace is the primary playback check for this
implementation.

## Validation

Build and run the demo with a bounded protocol trace from the repository root:

```sh
make -j2 DEBUG=1
timeout --signal=TERM 40s ./debug/quake2 -datadir q2test \
  +set vid_renderer gl3 +set vid_pauseonfocuslost 1 \
  +set cl_shownet 2 +demomap demo1.dm2
```

`demomap` supplies the `demos/` directory. A timeout is expected for an
interactive trace; inspect the final output for the first parser error. A
successful check reaches the end of `demo1.dm2` without a protocol or message
parse error.