# Functional Specification: Goop Launcher (`weapon_goop`)

## Required behavior

- A shot launches a bouncing projectile at 700 units/second, with an added
  aim-relative upward velocity of 50 plus or minus 10 units/second and
  lateral velocity of plus or minus 10 units/second. Its normal fuse is
  2.5 seconds.
- The weapon uses XP1/C9, with one unit consumed per shot.
- The projectile deals 120 radial damage within a radius of 160 and can be
  destroyed after taking 10 damage. A sky collision removes it without
  exploding. Contact with the owner is ignored.
- Contact with a solid, non-damageable surface stops and lands the projectile.
  It animates to frame 5, then selects a final frame from 5-8 and waits 8-16
  seconds before detonating. While active, a visible living player or monster
  within 64 units triggers detonation after 0.1 seconds; this includes the
  owner. Contact with a damageable entity detonates immediately. Destruction
  schedules detonation after 0.1 seconds. Landing plays
  `weapons/goop/gghit.wav`.
- Detonation generates impact noise and radial damage. Its visual effect
  distinguishes water from air and a grounded detonation from an airborne
  detonation.
- Firing plays `weapons/goop/ggsht.wav`, applies modest recoil, and generates
  weapon noise. The shared weapon controller requires ammo before a new firing
  sequence; the shot decrements one ammo unit unless infinite ammo or temporary
  invulnerability is active. The projectile launches from offset
  (forward 12, right 6, vertical view height minus 6).
- Weapon animation endpoints are 4/11/51/55, pause frame is 14, and firing
  frame is 6.

## Current remaster comparison

- The dynamic route has the required frame schedule. The projectile retains
  120 damage, 160-unit radius, 700-unit launch speed, 2.5-second flight fuse,
  10 health, owner-ignore, sky removal, sticky landing, and delayed
  detonation. The weapon checks and consumes ammo.
- Its velocity adds the fixed lift along world vertical, plus random
  aim-relative up and lateral components; the specified lift is aim-relative
  up with lateral variation only. Its launch offset differs from the specified
  (12, 6, view-height minus 6) offset.
- It sticks on any non-damageable collision rather than requiring a solid
  surface. The landed animation loops frames 0-5 instead of advancing once to
  a randomized final frame in the 6-9 range.
- Its proximity scan excludes the owner, while the source behavior can include
  a visible living owner within the trigger radius. Its detonation type is
  selected using point contents rather than the projectile's water state.
- Its firing origin and recoil differ, the landed animation loops frames 0-5
  rather than choosing a final frame from 5-8, and it omits the landing sound.
- The firing sound is the same, but uses a different sound channel.

## Verification checklist

- Test projectile velocity variation, damage, radius, fuse, destructibility,
  owner and sky contact.
- Verify sticking, landed animation and fuse, proximity visibility/radius,
  damageable-target contact, and destruction.
- Verify detonation effects in water/air and grounded/airborne states, ammo
  behavior, recoil, sounds, and animation.
