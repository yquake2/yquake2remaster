# Functional Specification: Monster Alienship 1 (`monster_alienship1`)

## 1. Overview
The Alienship 1 (`monster_alienship1`) is a flying enemy ported from the Infinity mod (`infinity/src/infinity/im_ship1.c`). It features birth, walk/fly, attack, pain, and death sequences, with distance-based movement adjustments (running away if too close).

---

## 2. Entity Definition & Properties
- **Classname:** `monster_alienship1`
- **Source File:** `infinity/src/infinity/im_ship1.c`
- **Frames & Animation States:**
  - **Birth:** Frames `0` to `7` (`BIRTHSTART` to `BIRTHEND`)
  - **Walk / Fly:** Frames `8` to `12` (`WALKSTART` to `WALKEND`)
  - **Attack:** Frames `13` to `17` (`ATTACKSTART` to `ATTACKEND`)
- **Proximity Threshold:** `TOCLOSE` = 180 units. If the enemy is closer than 180 units, the ship executes avoidance movement (`im_ship1_move_runaway`).

---

## 3. Combat & AI Behaviors
- **Sight / Idle / Pain / Death Sounds:** Plays designated audio hooks (`sound_sight`, `sound_idle1`, `sound_idle2`, `sound_pain1`, `sound_pain2`, `sound_die`, `sound_attack`).
- **Skill Attacks:** Periodically triggers `im_ship1_skill_attack` during movement/running cycles.
- **Flight Dynamics:** Operates with flying movement logic, maintaining distance and executing strafing/firing maneuvers.
