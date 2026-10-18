# Functional Specification: Weapon Rifle (`weapon_rifle`)

## 1. Overview
The Rifle (`weapon_rifle`) is a high-velocity HE (High Explosive) and AP projectile weapon ported from the Infinity mod (`infinity/src/infinity/iw_rifle.c`). It fires HE bullets (`fire_HE_bullet`, `fire_HE_shot`) with missile physics, shell casting, and mode switching.

---

## 2. Weapon Definition & Properties
- **Classname:** `weapon_rifle`
- **Source File:** `infinity/src/infinity/iw_rifle.c`
- **Projectile Speed:** 1680 units/sec.
- **Model:** `models/objects/beam/tris.md2` with `EF_GRENADE` and full bright rendering (`RF_FULLBRIGHT`).
- **Sound:** Projectile sound `weapons/bohsht.wav`, mode switch sound `weapons/m26/m26co.wav`.
- **Damage Modifiers:** `MOD_RIFLE_HE`, `MOD_RIFLE_AP`.

---

## 3. Mechanics & Firing Behavior
- **Explosive Impact:** `he_touch` triggers radius damage (`T_RadiusDamage`) with `MOD_RIFLE_HE` and rocket/water explosion PHS events.
- **Dodge Checks:** Invokes `check_dodge` for nearby AI actors.
- **Shell Casting:** Ejects casings via `Shellcasting`.
- **Mode Switching:** Toggles between Armor Piercing (AP) and High Explosive (HE) ammo types.
