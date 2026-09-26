# Functional Specification: Weapon RTDU (`weapon_rtdu`)

## 1. Overview
The RTDU weapon (`weapon_rtdu`) is ported from the Oblivion mod (`oblivion/REBLIVION/src/g_rtdu.cpp`). It provides specialized energy/projectile mechanics, firing sequences, and damage properties unique to Oblivion's arsenal.

---

## 2. Weapon Definition & Properties
- **Classname:** `weapon_rtdu`
- **Source File:** `oblivion/REBLIVION/src/g_rtdu.cpp`
- **Damage Type:** Custom energy / explosive damage with dedicated mod identifier (`MOD_RTDU`).

---

## 3. Mechanics & Firing Behavior
- **Ammunition & Attack:** Integrates with player inventory, handles ammunition consumption, muzzle flashes, and sound cues.
- **Projectile / Effect:** Spawns specialized projectiles with custom think and touch callbacks.
