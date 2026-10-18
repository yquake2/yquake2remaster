# Functional Specification: Monster Grunt 1v2 (`monster_grunt1v2`)

## 1. Overview
The Grunt 1v2 (`monster_grunt1v2`) is an alternate variant grunt enemy from the Infinity mod (`infinity/src/infinity/im_grnt2.c` / `im_grnt3.c`). It features expanded animation ranges (standing, idle, earpiece interaction, walking, running, ducking, jumping, shooting, pain, and death sequences).

---

## 2. Entity Definition & Properties
- **Classname:** `monster_grunt1v2`
- **Source Files:** `infinity/src/infinity/im_grnt2.c`, `infinity/src/infinity/im_grnt3.c`
- **Offsets:** Muzzle / tag offsets (`g2s1` = `{33,-12.5,20}`, `g2s2` = `{30,-8,23}`).
- **Animation Ranges (Key Frames):**
  - Stand / Idle A / Idle B / Earpiece
  - Walk / WalkShoot / Run / RunShoot
  - Duck / JumpJ / ShootA / ShootB
  - Pain A / Pain A1 / Pain A2 / Pain B
  - Death A / Death B / RailDeath

---

## 3. Combat & AI Behaviors
- **Action Triggers:** Earpiece animations and multi-stage idle reactions (`sound_idle1`, `sound_idle2`, `sound_idle3`).
- **Combat Proficiency:** Supports aiming offsets, ducking, jumping, and dual-mode shooting (`SHOOTA` / `SHOOTB`).
- **Damage & Death:** Comprehensive pain and death reactions with railgun-specific death animations (`raildeath`).
