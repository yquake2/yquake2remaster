# Functional Specification: Rifle (`weapon_rifle`)

## Required behavior

- The default armor-piercing (AP) mode fires bullets dealing 8 damage and 2
  kick, with horizontal/vertical spread of 150/250. AP shots do not use quad
  scaling.
- High-explosive (HE) mode fires a projectile at 1680 units/second with
  68-85 damage. On impact it deals that amount of radial damage within a
  radius 40 units greater than the damage amount. Sky contact removes it
  without an explosion; owner contact is ignored. If it does not hit
  anything, it disappears after two seconds without exploding. The projectile
  displays `models/objects/beam/tris.md2`, is fullbright, and plays
  `weapons/bohsht.wav`.
- Attack must be held for repeated fire. Releasing attack resets the firing
  sequence to frame 21 and clears the repeated-shot recoil counter.
- If the selected ammo type is empty, the weapon switches to the other mode,
  changes its ammo type between `Bullets` and `HE Rounds`, notifies the player,
  plays `weapons/m26/m26co.wav`, and enters the switching animation at frame
  68 while keeping the weapon in its firing state. It does not check that the
  other ammo type is available before switching.
- AP uses Bullets and HE uses HE Rounds; each successful shot spends one
  round of the selected type unless infinite ammo or temporary invulnerability
  is active.
- AP recoil rises by 1.5 degrees per shot up to three steps outside
  deathmatch and resets on release; small random view and origin offsets are
  applied while firing, and the recoil angle affects AP aim. HE shots play
  `weapons/m26/m26exr.wav`. Both modes send a machinegun muzzle flash and
  weapon noise; HE also ejects a casing.
- AP launches from offset (forward 0, right 0, vertical view height minus 8);
  HE launches from (forward 13, right 6, vertical view height minus 6).
- Weapon animation endpoints are 6/20/60/67, with no pause frames and fire
  frames 7-13.

## Current remaster comparison

- The dynamic route uses the expected endpoints and firing-frame list. The
  remaster implements AP bullets, HE projectiles, held-fire handling, mode
  switching when the active ammo is empty and the other type is available,
  shot noise, and one-round ammo consumption.
- The remaster uses `Grenades` rather than `HE Rounds` for HE ammo and only
  switches if the other type has ammo. The source changes mode even when the
  alternate ammo is empty. The mode-switch sound and notification text differ.
  It enters the ready state during the frame-68 transition rather than
  retaining the firing state.
- HE firing uses the generic grenade behavior: it adds a large aim-relative
  upward arc, explodes when its two-second timer expires, and uses a generic
  grenade model, bounce sound, and damage attribution. The specified HE round
  travels along the aim direction, displays the rifle projectile model,
  disappears silently at two seconds, and uses its rifle-specific damage type.
- AP and HE use one shared muzzle offset in the remaster. AP also uses a
  different recoil progression (it rises by 0.5 degrees per shot to the same
  4.5-degree cap over nine shots rather than three, lacks the random offsets,
  and does not apply the recoil angle to aim). AP adds a firing sound unlike
  the specified behavior, and both modes use a different muzzle-flash type.
  The HE firing sound differs from the specified sound.
- The remaster cycles both modes through frames 7-13; the source callback
  changes the next frame according to mode: AP returns to frame 7 when firing
  from frame 12, while HE advances from its first fire frame to frame 14.
- On release, the remaster temporarily sets frame 60 before its normal idle
  handling returns the weapon to the idle start. The specified release target
  is ready frame 21.

## Verification checklist

- Test AP and HE firing separately, including damage, spread, projectile speed,
  timeout, sky/owner contact, blast damage, and radius.
- Test held/released attack, ammo-mode switching with each and neither type
  available, notification, sound, and switching frame.
- Verify AP recoil progression, muzzle flash, weapon noise, HE sound/casing,
  ammo handling, and frame progression.
