# Functional Specification: Blaze (`weapon_blaze`)

## Required behavior

- While attack is held, the weapon charges up to 30 points. It adds one point
  when ammunition is available and charge is below the cap; each added point
  consumes one ammo unit unless infinite ammo or temporary invulnerability is
  active. Reaching full charge loops the charge animation over frames 38-43.
- The weapon uses Power Crystals, with one unit spent per charge point.
- When attack is released with a nonzero charge, the weapon launches one
  projectile at 800 units/second, plays `weapons/blazer/blzsht.wav`, resets
  charge, and sets the weapon frame to 44. Release itself does not consume
  ammunition. If charge is zero, it returns to idle without firing. If charge
  is zero and ammo is empty while attack is held, it plays the throttled
  no-ammo sound and requests a weapon switch.
- Charge generates an initial sound and a repeating charge-loop sound. A
  charge above 5 also emits short-lived charge visuals. Projectile charge has
  a minimum value of 5 and controls its displayed frame and impact effects.
- The projectile launches from offset (forward 12, right 4, vertical view
  height minus 3). Impacts with charge above 20 emit a large-explosion event.
  Impact sound is `weapons/blazer/blzexp1.wav` below charge 10,
  `weapons/blazer/blzexp2.wav` from 10 through 19, and
  `weapons/blazer/blzexp3.wav` at 20 or above.
- The projectile ignores its owner and disappears on sky contact. Other
  contact stops it and starts an expanding impact. A surface with an exactly
  vertical upward normal produces a flash effect; other impacts produce a
  shock effect. Each damaging impact
  tick applies radial damage equal to the greater of 32 and three times the
  charge. Its radius is the greater of 64 and the charge multiplied by the
  current effect frame. The projectile is removed after 5 seconds if it does not
  hit anything.
- Weapon animation endpoints are 7/48/79/84, with no pause frames and firing
  frames 8-43.

## Current remaster comparison

- The dynamic route uses the expected animation schedule. The remaster
  implements charging, a 30-point cap, charge-scaled projectile frame, and
  release-to-fire, but its shared weapon controller allows charging at zero
  ammo and the callback increments charge even when no ammo is available.
- The remaster does not play the charge-up/loop sounds or provide the
  charge visuals and recoil. It launches at the correct speed and minimum
  charge, but uses a different launch offset and release sound and an
  8-second projectile timeout rather than 5 seconds.
- Its impact uses a generic air/water explosion, omits the source's distinct
  flash/shock models and impact sounds, and applies damage under a different
  damage type. Radius grows from the initial charge-based radius through ten
  ticks, rather than growing from the projectile's initial display frame
  through its charge-dependent animation.
- The charge animation remains on frame 43 at maximum charge instead of
  looping across frames 38-43. Empty uncharged fire does not reproduce the
  specified no-ammo feedback and weapon switch. On release, the remaster sets
  frame 48 instead of frame 44.

## Verification checklist

- Test held-charge progression, cap and loop, ammunition per charge step,
  release with and without remaining ammo, and release without charge.
- Verify minimum charge, projectile speed and timeout, owner/sky contact,
  floor-versus-wall effect, damage/radius progression, and cleanup.
- Verify charging, firing, no-ammo feedback, and animation transitions.
