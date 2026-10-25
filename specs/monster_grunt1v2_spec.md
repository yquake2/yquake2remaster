# Functional Specification: Infinity Grunt 1v2

## Entity and map interface
- **Classname:** `monster_grunt1v2`
- **Map bounds:** Minimum `(-16, -16, -25)`; maximum `(16, 16, 30)`.
- **Map flags:** `Ambush`, `Trigger_Spawn`, and `Sight`.
- **Model and skin:** `models/monsters/grunt/tris.md2`, skin `1`; scale `1`.
- **Initial health, gib threshold, mass:** `80`, `-40`, and `100`.
- **Animation frames:** idle variations `0–15`, `16–44`, `45–64`, and `65–85`; walking/jogging `86–99`; running `100–109`; attacks `110–119`, `120–135`, `136–152`, and `153–165`; pain `166–206`; death `207–236`; headshot death `237–249`.
- The monster does not spawn in deathmatch.

## Behavior
- Movement, target selection, sight and idle responses, and death behavior match Grunt 1v1. This skin selects shotgun fire.
- When pursuing a target, it jogs at distances of `300` units or less and runs farther away. Above `300` units, it has a `40%` chance per movement decision to circle, turning `12` degrees per step.
- Attack attempts are made when `difficulty × U > 1.8`, where `U` is a random value in `[0, 1)`. At longer range it chooses a running or hand-held attack; at close range it chooses between mounted and hand-held firing.
- Each shotgun firing event deals `2 + 2 × difficulty × U` damage per pellet.
- Its close-range strike deals `15` to `19` damage.
- Idle sounds are selected from `Monsters/grunt1/g1idle1.wav` and `Monsters/grunt1/g1idle2.wav`. Sight responses are selected from `Monsters/grunt1/g1act1.wav` through `Monsters/grunt1/g1act4.wav`.
- Pain sounds: `Monsters/grunt1/g1pain1.wav`, `Monsters/grunt1/g1pain2.wav`, and `Monsters/grunt1/g1pain3.wav`. Death sounds: `Monsters/grunt1/g1death1.wav` and `Monsters/grunt1/g1death2.wav`. Close-range attack sound: `Monsters/grunt1/g1melee.wav`.
- Pain animation is suppressed on the highest difficulty and cannot retrigger during its three-second debounce period. A hit to within `15` units of the top causes a headshot death sequence; otherwise it selects one of two normal death sequences.
- At or below the gib threshold, it is dismembered. The headshot and gib effects use `misc/udeath3.wav`, `misc/udeath.wav`, and grunt-specific gib models.
