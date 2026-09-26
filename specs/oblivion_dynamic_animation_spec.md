# Functional Specification: Oblivion Dynamic Animations (`badass` and `floater`)

## 1. Overview
Dynamic animations based on the `activate` trigger signal for `monster_badass` and `monster_floater` are implemented in the Oblivion mod (`oblivion/REBLIVION/src/m_badass.cpp`, `oblivion/REBLIVION/src/m_float.cpp`). They allow monsters to transition from disguised or idle states into combat upon activation.

---

## 2. Monster Entity Properties & Source Files
- **Entities:** `monster_badass`, `monster_floater`
- **Source Files:** `m_badass.cpp`, `m_float.cpp`
- **Spawnflags:** `SPAWNFLAG_DISGUISE` (8).

---

## 3. Mechanics & Animation Behavior
- **Activation Trigger:** When targeted or activated, monsters executing disguise/pop frames transition from hidden or static posing (`floater_move_disguise`, `badass_move_disguise`) into active pop-out animations (`floater_move_pop`, `badass_move_pop`), subsequently entering standard walk/run AI loops.
