# Functional Specification: Oblivion Ammo DoD (`ammo_dod`)

## 1. Overview
The DoD ammunition item (`ammo_dod`) is ported from the Oblivion mod (`oblivion/REBLIVION/src/g_items.cpp`). It supplies ammunition pickups for DoD-related weapon types in player inventories.

---

## 2. Entity Definition & Properties
- **Classname:** `ammo_dod`
- **Source File:** `oblivion/REBLIVION/src/g_items.cpp`
- **Pickup Behavior:** Adds ammo quantity to player inventory upon contact, playing standard pickup sound effects and respawning in cooperative/deathmatch modes.
