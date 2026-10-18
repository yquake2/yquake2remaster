# Functional Specification: Monster Grunt 2 (`monster_grunt2`)

## 1. Overview
The Grunt 2 (`monster_grunt2`) is a heavy tactical grunt enemy from the Infinity mod (`infinity/src/infinity/im_grnt2.c`). It builds upon the core grunt framework with advanced combat scripts, weapon attachments, and specialized movement states.

---

## 2. Entity Definition & Properties
- **Classname:** `monster_grunt2`
- **Source File:** `infinity/src/infinity/im_grnt2.c`
- **Model Scale:** `1`
- **Muzzle Attachments:** Weapon source vectors `g2s1` and `g2s2` for projectile / hitscan origin calculations.

---

## 3. Combat & AI Behaviors
- **State Transitions:** Integrates seamlessly with `im_grunt2_walk`, `im_grunt2_stand`, and `im_grunt2_run`.
- **Attack Routing:** Manages `im_grunt2_skill_attack`, `im_grunt2_attack`, and `im_grunt2_fire`.
- **Robust Defense:** Implements distinct pain thresholds (`pain_a`, `pain_b`) and multi-tier death states (`death_a`, `death_b`, `raildeath`).
