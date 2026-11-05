# Functional Specification: Infinity Grunt 2

## Entity and map interface
- **Classname:** `monster_grunt2`
- **Map bounds:** Minimum `(-10, -23, -25)`; maximum `(12, 22, 52)`.
- **Map flags:** `Ambush`, `Trigger_Spawn`, and `Sight`.
- **Model and scale:** `models/monsters/grunt2/tris.md2`, scale `1`.
- **Initial health, gib threshold, mass:** `180`, `-60`, and `100`.
- **Animation frames:** standing `0–20`; idle variations `21–43` and `44–67`; earpiece interaction `68–91`; walking/jogging `92–101`; walk-and-fire `102–111`; running `112–117`; run-and-fire `118–123`; ducking `124–134`; jumping `135–144`; stationary attacks `145–158` and `159–172`; pain `173–184` and `185–206`; death `207–211` and `212–217`; heavy-damage death `218–227`.
- The monster does not spawn in deathmatch.

## Behavior
- With no enemy or while ordered to hold ground, it idles. During pursuit it runs when farther than `300` units and jogs at closer distances.
- An attack attempt occurs during movement when `difficulty × U > 1.8`, where `U` is a random value in `[0, 1)`. It uses a rear-facing attack if its target is behind it, a running attack beyond `300` units, a stationary attack within `150` units, and otherwise randomly chooses between moving and stationary attacks.
- Attacks fire a pink-light laser travelling at speed `1200`. Each shot deals `28 + 8 × difficulty × U` damage.
- When selected to dodge, it reacts on `25%` of opportunities. It ducks at the lowest difficulty; at higher difficulties it may counterattack instead, with the counterattack more likely at the higher settings.
- Pain reactions are ignored during attacks and during the three-second pain debounce. On the highest difficulty no pain animation is played. Damage greater than `50` selects the stronger pain animation.
- Normal death selects between two sequences. A killing hit greater than `50` selects the special heavy-damage death sequence. At or below `-60` health the monster is gibbed.
- Idle sounds: `Monsters/grunt2/g2idlng1.wav`, `Monsters/grunt2/g2idlng2.wav`, and `Monsters/grunt2/g2idlng3.wav`.
- Pain sounds: `Monsters/grunt2/g2pain1.wav`, `Monsters/grunt2/g2pain2.wav`, and `Monsters/grunt2/g2pain3.wav`. Death sounds: `Monsters/grunt2/g2death1.wav` and `Monsters/grunt2/g2death2.wav`. Sight responses: `Monsters/grunt2/g2act1.wav`, `Monsters/grunt2/g2act2.wav`, and `Monsters/grunt2/g2act3.wav`.
