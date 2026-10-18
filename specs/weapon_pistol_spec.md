# Functional Specification: Weapon Pistol (`weapon_pistol`)

## 1. Overview
The Pistol weapon (`weapon_pistol`) is ported from the Infinity mod (`infinity/src/infinity/iw_pistol.c`). It features sidearm mechanics, ammo consumption, and magnetic pulse projectile behaviors with laser sparks and blast effects.

---

## 2. Weapon Definition & Properties
- **Classname:** `weapon_pistol`
- **Source File:** `infinity/src/infinity/iw_pistol.c`
- **Base Damage:** 30 units
- **Projectile Speed:** 1000 units/second
- **Splash Radius:** 90 units (`dmg * 3`)
- **Maximum Lifespan:** 8.0 seconds
- **Projectile / Effect:** Magnetic pulse projectile (`magnetic_pulse_think`, `magnetic_pulse_blast`, `magnetic_pulse_touch`), emitting blue light (`EF_BLUELIGHT`).
- **Visual Feedback:** Generates temporary laser sparks (`TE_LASER_SPARKS`) along the pulse trajectory.
- **Models:** Flight model (`models/objects/bball/tris.md2`), detonation/explosion model (`models/objects/disch2/tris.md2`).
- **Audio:** Flight sound (`misc/lasfly.wav`), impact sound (`weapons/lashit.wav`).

---

## 3. Mechanics & Firing Behavior
- **Ammunition Check:** Verifies inventory ammo levels; triggers empty click sound and weapon change if depleted.
- **Player Noise:** Registers weapon firing and impact noise via `PlayerNoise`.
- **Damage & Mod:** Deals energy-based area damage with modifier `MOD_MAGNETICPULSE`.
