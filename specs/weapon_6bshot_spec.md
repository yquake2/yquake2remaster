# Functional Specification: Weapon 6B Shotgun (`weapon_6bshot`)

## 1. Overview
The 6B Shotgun (`weapon_6bshot`) is ported from the Infinity mod (`infinity/src/infinity/iw_6bshot.c`). It is a high-impact riot/shotgun weapon firing multiple pellet traces per shot with customized muzzle flashes and ammo handling.

---

## 2. Weapon Definition & Properties
- **Classname:** `weapon_6bshot`
- **Source File:** `infinity/src/infinity/iw_6bshot.c`
- **Base Damage / Kick:** `damage` = 4 per pellet, `kick` = 8.
- **Muzzle Flash:** `MZ_SSHOTGUN` (supershotgun style flash).

---

## 3. Mechanics & Firing Behavior
- **Pellet Dispersion:** Fires shotgun pellet spread via `fire_shotgun` (respecting deathmatch default counts or SP counts, using `MOD_RIOTGUN`).
- **Recoil & Kick:** Applies client kickback (`ent->client->kick_origin` and `kick_angles`).
- **Ammo Management:** Consumes 1 ammo unit per shot unless infinite ammo or invincible flags are active.
