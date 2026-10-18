# Functional Specification: Oblivion Additional Weapons & Ammo

## 1. Overview
This specification covers advanced Oblivion weapons and ammunition types:
- Weapons: `weapon_hellfury`, `weapon_plasma_pistol`, `weapon_plasma_rifle`, `weapon_deatomizer`, `weapon_remote_detonator`
- Ammunition / Explosives: `ammo_mines`, `ammo_detpack`, `ammo_rifleplasma`

---

## 2. Source Files & Implementation
- **Source Files:** `oblivion/REBLIVION/src/g_weapon.cpp`, `g_items.cpp`
- **Weapons Mechanics:**
  - Plasma Pistol & Rifle: Energy projectile firing with specialized glow/flash effects and plasma damage types (`MOD_PLASMA`).
  - Hellfury & Deatomizer: Heavy destructive energy weapons with high-damage scaling and distinct projectile traits.
  - Remote Detonator & Detpack / Mines: Placeable explosive entities with remote trigger mechanisms and proximity detonation.
- **Ammo Management:** Integrates enforcement limits (`mine_enforce_limit`, `detpack_enforce_limit`) and inventory pickups.
