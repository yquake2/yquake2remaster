# Q2Test Demo Protocol 25

This document records the wire format needed to play the bundled
`q2test-assets/baseq2/demos/demo1.dm2` recording. It is based on the protocol
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
| `0x800000` | extended skin value; paired with the 8-bit skin flag, it forms a 32-bit value |
| `0x1000000` | sound |

Coordinates and old-origin components are signed 16-bit values in eighth-unit
increments. Angles are signed bytes scaled by `360 / 256`. Model indexes and
sound indexes are bytes. Frames and render/effect/skin values use the widths
selected by their flags; a paired 8-/16-bit flag denotes a 32-bit value.

## Temporary Entities

Service opcode 10 is followed by a one-byte temporary-entity subtype. In the
bundled recording, subtype 16 carries three signed 16-bit position components
in eighth-unit increments, followed by one direction-table byte. This differs
from the later Quake II parasite-beam payload, which must not be used to parse
this Q2Test subtype. Subtype 19 carries only the three position components;
it must not be decoded as the later medic-cable beam payload.

## Frame Messages

A frame message has a Q2Test-specific envelope:

1. Advance the local server-frame number by one.
2. Read a 32-bit time value. The reference playback interprets it in tenths of
   a second; the Remaster client's millisecond clock should use `value * 100`.
3. Read one byte giving the area-bit count, followed by that many area bytes.
4. Read player-info opcode 14 and its player-state payload.
5. Read packet-entities opcode 16 and its entity deltas.

There is no transmitted server-frame number, delta-frame number, or suppression
count in this envelope. Entity deltas use the prior decoded frame when one is
available; the first frame uses baselines.

## Player State

The player-info payload begins with a 16-bit flags value, followed by fixed
data:

- Three signed 16-bit origin components in eighth-unit increments.
- Three signed 16-bit player-movement velocity components, followed by three
  signed 16-bit delta angles. Delta angles use the standard 16-bit angle scale.

The flags select optional data:

| Flag | Payload |
| ---: | --- |
| `0x01` | four blend bytes |
| `0x02` | one FOV byte; default FOV is 90 when absent |
| `0x04` | weapon index and frame, one byte each |
| `0x08` | two weapon bytes followed by six signed angle-offset bytes in quarter-unit increments |
| `0x80` | one render-definition flags byte |

After those fields, a 32-bit stats mask is read. Each set bit, from bit 0 to
bit 31, is followed by one signed 16-bit stat value.

The bundled `demo1.dm2` trace is the primary playback check for this
implementation.

## Validation

Build and run the demo with a bounded protocol trace from the repository root:

```sh
make -j2 DEBUG=1
timeout --signal=TERM 40s ./debug/quake2 -datadir q2test-assets \
  +set vid_renderer gl3 +set vid_pauseonfocuslost 1 \
  +set cl_shownet 2 +demomap demo1.dm2
```

`demomap` supplies the `demos/` directory. A timeout is expected for an
interactive trace; inspect the final output for the first parser error. A
successful check reaches the end of `demo1.dm2` without a protocol or message
parse error.