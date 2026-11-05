# Functional Specification: Infinity Alienship 1

## Entity and map interface
- **Classname:** `monster_alienship1`
- **Map bounds:** Minimum `(-34, -25, -13)`; maximum `(45, 43, 41)`.
- **Map flags:** `Ambush`, `Trigger_Spawn`, `Sight`, `Style1`, and `Style2`.
- **Model:** `models/monsters/aliens1/tris.md2`; `Style1` selects `models/monsters/aliens1/tris2.md2`.
- **Scale, health, mass:** `1`, `35`, and `50`.
- **Movement:** Flying; no melee attack.
- **Animation frames:** birth `0–7`; standing, walking, and flight `8–12`; attack `13–17`.
- The monster does not spawn in deathmatch.

## Behavior
- It pursues its current enemy. At distances below `180` units it backs away, moving `18` units backward per frame; otherwise it advances at `10` units per frame.
- An attack opportunity occurs during movement when `difficulty × U > 1.8`, where `U` is a random value in `[0, 1)`. The five-frame attack sequence fires on its first three frames.
- Each projectile deals `3 + 8 × difficulty × U` damage and travels at speed `880`. The attack sound plays for each shot.
- Sight, idle, pain, death, and attack sounds are:
  - `monsters/aliens1/asact1.wav`
  - `monsters/aliens1/asidle1.wav`, `monsters/aliens1/asidle2.wav`
  - `monsters/aliens1/aspain1.wav`, `monsters/aliens1/aspain2.wav`
  - `monsters/aliens1/asdeath.wav`
  - `weapons/aswhip1.wav`
- Death releases five ship-specific gib models: `models/monsters/shipgib/shipgib1.md2` through `models/monsters/shipgib/shipgib5.md2`.
