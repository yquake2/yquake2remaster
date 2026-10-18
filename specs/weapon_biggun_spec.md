# Functional Specification: Weapon Biggun (`weapon_biggun`)

## 1. Overview
The Biggun (`weapon_biggun`) is a heavy energy/beam weapon ported from the Infinity mod (`infinity/src/infinity/iw_biggun.c`). It features directed energy beams (`fire_beam`), red shell effects (`EF_FLAG1`, `RF_SHELL_RED`), and delayed energy damage mechanics (`energy_think`).

---

## 2. Weapon Definition & Properties
- **Classname:** `weapon_biggun`
- **Source File:** `infinity/src/infinity/iw_biggun.c`
- **Damage Modifiers:** `MOD_BIGGUN`
- **Special Effects:** Applies red shell rendering effects (`EF_FLAG1 | EF_COLOR_SHELL | RF_SHELL_RED`) on targets during energy transfer (`spawn_energy`).

---

## 3. Mechanics & Firing Behavior
- **Beam Spawning:** Instantiates persistent beam entities (`beam_think`, `RF_BEAM | RF_TRANSLUCENT`) that track target origins over time.
- **Delayed Damage:** Inflicts damage via `T_Damage` after thinking delay intervals using `MOD_BIGGUN`, handling monsters and client targets.
