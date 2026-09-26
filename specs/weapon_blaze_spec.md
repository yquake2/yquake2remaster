# Functional Specification: Weapon Blazer (`weapon_blaze`)

## 1. Overview
The Blazer (`weapon_blaze`) is an energy charge and shockwave weapon ported from the Infinity mod (`infinity/src/infinity/iw_blaze.c`). It spawns charge bolts (`SpawnCharge`) and transient blaze effects (`Spawn_BlazeEffect`, `blaze_think`) using boom flash and shock models.

---

## 2. Weapon Definition & Properties
- **Classname:** `weapon_blaze`
- **Source File:** `infinity/src/infinity/iw_blaze.c`
- **Effects & Models:**
  - Charge sprite: `sprites/null.sp2` with `EF_BFG | EF_ANIM_ALLFAST`.
  - Flash model: `models/objects/boom/flash.md2`.
  - Shock model: `models/objects/boom/shock.md2` with `EF_TRANSLUCENT33`.

---

## 3. Mechanics & Firing Behavior
- **Effect Spawning:** Dynamically instantiates visual shockwave and flash entities along the trajectory.
- **Think Loop:** Manages expansion, translucency, and lifecycle cleanup through `blaze_think`.
