# Functional Specification: Weapon Goop Launcher (`weapon_goop`)

## 1. Overview
The Goop Launcher (`weapon_goop`) is an explosive projectile weapon ported from the Infinity mod (`infinity/src/infinity/iw_goop.c`). It fires sticky goop projectiles that animate upon landing/touch, detect nearby targets within a radius, and detonate with radius damage.

---

## 2. Weapon Definition & Properties
- **Classname:** `weapon_goop`
- **Source File:** `infinity/src/infinity/iw_goop.c`
- **Damage & Radius:** Uses radius damage (`T_RadiusDamage`) with damage mod `MOD_GOOPLAUNCHER`.
- **Explosion Effects:** Supports underwater vs surface explosion temp entities (`TE_GRENADE_EXPLOSION`, `TE_ROCKET_EXPLOSION`, water variants).

---

## 3. Mechanics & Firing Behavior
- **Proximity & Touch Sensing:** `GoopThink` scans for nearby clients and monsters within 64 units (`findradius`) and triggers premature detonation if visible targets are present.
- **Animation States:** Advances frame animation sequence up to frame 9 upon landing or hitting obstacles.
