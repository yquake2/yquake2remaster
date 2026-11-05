# Functional Specification: Infinity Screamer

## Entity and map interface
- **Classname:** `monster_screamer`
- **Map bounds:** Minimum `(-20, -20, -25)`; maximum `(20, 20, 38)`.
- **Map flags:** `Ambush`, `Trigger_Spawn`, and `Sight`.
- **Model and scale:** `models/monsters/screamer/tris.md2`, scale `1`.
- **Initial health, gib threshold, mass:** `180`, `-50`, and `200`.
- **Melee strike damage:** `12` to `16`.
- **Animation frames:** standing `0–37`; idle `38–59`; ground movement `60–65`; running strike `66–71`; jump `72–86`; jump attack `87–101`; ranged scream `102–115`; wall movement and pause `116–131`; ground strikes `132–162`; swimming `163–174`; swimming strike `175–180`; submerged idle, scream, melee, and pain `181–241`; ground pain `242–273`; death `274–300`.
- The monster does not spawn in deathmatch.

## Behavior
- On land, an attack attempt uses a ranged scream with `68%` probability when the target is farther than `180` units. At distances under `100` units it uses a running strike; at intermediate distance it jumps toward the target.
- Each ranged scream deals `3 + 8 × difficulty × U` damage and travels at speed `450`, where `U` is a random value in `[0, 1)`. A scream sound plays for each shot.
- A melee strike deals `12 + N` damage, where `N` is an integer from `0` through `4`. One strike animation can hit on both sides.
- When more than one water level deep, it chooses an underwater ranged attack with `68%` probability if the target is more than `120` units away. If the target is not submerged it tries to jump out; otherwise it uses an underwater melee attack. It selects between two swimming cycles while moving underwater.
- A jump launches forward at speed `600` with an upward speed of `250`. On impact with a damageable target, if its speed exceeds `400`, it deals `40 + 10 × U` collision damage.
- A jump into a vertical solid wall can begin wall movement. It alternates randomly between crawling, pausing, and jumping away; after more than three wall jumps it resumes ordinary movement.
- Pain reactions are suppressed during attacks, wall movement, other pain sequences, and for three seconds after the previous pain event. The highest difficulty suppresses pain animations. Underwater pain uses submerged sequences; a ground hit greater than `50` has a `50%` chance to use the stronger pain sequence.
- Idle sounds: `Monsters/Screamer/sidling1.wav`, `Monsters/Screamer/sidling2.wav`, and `Monsters/Screamer/sidling3.wav`.
- Pain sounds: `Monsters/Screamer/spain1.wav` and `Monsters/Screamer/spain2.wav`. Death sound: `Monsters/Screamer/sdeath1.wav`. Sight responses: `Monsters/Screamer/sact1.wav` and `Monsters/Screamer/sact2.wav`. Melee and ranged-attack sounds: `Monsters/Screamer/smelee.wav` and `Monsters/Screamer/ssscream.wav`.
- Normal death selects one of three animations. At or below `-50` health it is gibbed.
