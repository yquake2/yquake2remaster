# Functional Specification: Monster Grunt 1v1 (`monster_grunt1v1`)

## 1. Overview
The Grunt 1v1 (`monster_grunt1v1`) is an infantry/grunt enemy ported from the Infinity mod (`infinity/src/infinity/im_grunt.c`). It features multiple idle animations, circle AI movement (`im_grunt_AIcircle`), tactical combat firing, pain reactions, and death handling.

---

## 2. Entity Definition & Properties
- **Classname:** `monster_grunt1v1`
- **Source File:** `infinity/src/infinity/im_grunt.c`
- **Model Scale:** `1`
- **Distance Far Threshold:** `DIST_FAR` = 300 units.
- **Circle AI Angle Increment:** `circleAIangle` = 12 degrees.

---

## 3. Combat & AI Behaviors
- **Idle & Sight Variations:** Randomly selects from multiple idle sound clips (`sound_idle1`, `sound_idle2`) and sight alert lines (`sound_act1` through `sound_act4`).
- **Tactical Movement:** Utilizes `im_grunt_randAI` and circular positioning (`im_grunt_AIcircle`) to flank targets.
- **Attacks & Firing:** Executes ranged attacks (`im_grunt_attack`, `im_grunt_fire`, `im_grunt_option_fire`) and skill-based combat decisions.
- **Damage & Death:** Supports specialized pain states and death sequences (`im_grunt_pain`, `im_grunt_die`, `im_grunt_dead`).
