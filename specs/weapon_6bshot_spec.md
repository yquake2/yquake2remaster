# Functional Specification: 6B Shotgun (`weapon_6bshot`)

## Required behavior

- A shot fires 12 pellets in both single-player and deathmatch. Each pellet
  deals 4 damage and 8 kick, with horizontal and vertical spread both set to
  500. Damage is attributed to the riot-shotgun damage type.
- The weapon uses Shells, with one unit consumed per shot.
- The weapon requires at least one ammo unit to fire. If empty, it sets the
  weapon frame to 45, plays `weapons/noammo.wav` at most once per second, and
  requests an ammo-available weapon switch.
- A successful shot applies modest recoil, sends the supershotgun muzzle flash,
  and generates weapon noise. One unit of ammunition is consumed unless
  infinite ammo or temporary invulnerability is active.
- Weapon animation endpoints are 4/46/84/88, with no pause frames and firing
  frames 6, 9, 12, 15, 18, and 21.

## Current remaster comparison

- The dynamic route uses the specified animation schedule. The callback
  matches pellet count, per-pellet damage and kick, spread, damage type,
  muzzle flash, recoil, weapon noise, and one-round ammo behavior.
- The remaster additionally plays a firing sound and ejects a casing.
- Its empty-ammo path is handled by the shared weapon-frame controller rather
  than by the firing callback's specified frame-45 handling.

## Verification checklist

- Verify 12 pellets in both game modes, their damage, kick and spread, and
  damage attribution.
- Test empty-ammo handling and the one-second sound throttle.
- Verify recoil, muzzle flash, weapon noise, firing frames, and ammo handling
  with infinite ammo and invulnerability on and off.
