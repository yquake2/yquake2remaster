# Functional Specification: Magnetic Pistol (`weapon_pistol`)

## Required behavior

- The weapon can fire without consuming ammunition. It does not check ammo.
  The weaponless player state prevents firing.
- A shot launches a magnetic pulse in the player's aim direction at 1000
  units/second and sets its damage value to 30. The pulse has a blue light,
  spins while flying, and every 0.1 seconds emits three laser-spark trail
  events at points 10, 35, and 60 units behind it. It is removed after
  8 seconds if it has not landed.
- The projectile ignores its owner and is removed when it hits the sky. Other
  contact converts it into a stationary impact effect: it displays
  `models/objects/disch2/tris.md2`, emits 28 laser sparks, plays
  `weapons/lashit.wav`, and applies radial damage of 30 within a 90-unit radius,
  excluding the owner. The impact display advances through three frames and
  is then removed. It does not apply direct collision damage.
- Firing applies modest recoil, sends the blaster muzzle flash, and generates
  weapon noise. The projectile launches from offset (forward 16, right 5,
  vertical view height minus 5). Weapon animation endpoints are 4/8/52/55,
  pause frames are 13 and 40, and the firing frame is 5.

## Current remaster comparison

- The dynamic weapon route uses the expected animation schedule and the
  projectile implementation retains the 1000-unit speed, 30 damage, 90-unit
  radius, owner exclusion, sky removal, impact sound/model, and 0.4-second
  cleanup.
- The remaster does not check or consume ammo, as specified, but it does not
  honor the weaponless state.
- The projectile trail emits two sparks at each of three points rather than
  eight; its spark direction/color differ. The impact does not advance the
  three-frame animation. Its visual effect flag differs from the specified
  blue light. The shot origin and muzzle-flash multicast origin also differ
  from the specified firing behavior. The remaster omits the source AI dodge
  notification and initial obstruction trace.

## Verification checklist

- Test firing without ammo, in weaponless state, and with infinite-ammo and
  invulnerability settings.
- Verify projectile speed, lifetime, trail, sky and owner collisions, impact
  model/sparks/sound, blast damage/radius, owner exclusion, and impact cleanup.
- Verify firing animation, recoil, muzzle flash, and weapon noise.
