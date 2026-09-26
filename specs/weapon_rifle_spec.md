# Functional Specification: Weapon Rifle (`weapon_rifle`)

## 1. Overview
The Rifle (`weapon_rifle`) is a high-velocity HE (High Explosive) projectile weapon ported from the Infinity mod (`infinity/src/infinity/iw_rifle.c`). It fires HE bullets (`fire_HE_bullet`, `fire_HE_shot`) with missile physics and shell casting.

---

## 2. Weapon Definition & Properties
- **Classname:** `weapon_rifle`
- **Source File:** `infinity/src/infinity/iw_rifle.c`
- **Projectile Speed:** 1680 units/sec.
- **Model:** `models/objects/beam/tris.md2` with `EF_GRENADE` and full bright rendering.
- **Damage Modifiers:** `MOD_RIFLE_HE`

---

## 3. Mechanics & Firing Behavior
- **Explosive Impact:** `he_touch` triggers radius damage (`T_RadiusDamage`) and rocket explosion PHS events.
- **Dodge Checks:** Invokes `check_dodge` for nearby AI actors.
- **Shell Casting:** Ejects casings via `Shellcasting`.
