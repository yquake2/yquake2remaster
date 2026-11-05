# Functional Specification: Biggun (`weapon_biggun`)

## Required behavior

- A shot traces up to 8192 units. It continues past monsters and clients to
  affect later eligible targets along the same line. The weapon sets a
  damage value in the range 128-155 and 250 knockback in single-player, or
  68-85 damage and 150 knockback in deathmatch.
- Each damageable target in the path takes 1 immediate damage and knockback,
  is lifted by 80 units and pushed along the beam, and receives a red marking.
  After 0.5 seconds it takes a second hit using the selected damage value; the
  red marking is removed afterward.
- A damageable window encountered by the trace receives a separate 32-point
  radial hit with a 32-unit radius.
- The shot produces two visible beam segments and a bubble-trail event. It
  generates impact noise at the end of the trace.
- At field of view 10, firing plays `weapons/railgf1a.wav` quietly and applies
  slight backward recoil, without the normal muzzle flash, firing noise, or
  strong recoil. Otherwise it sends the railgun muzzle flash, applies stronger
  recoil, and generates weapon noise.
- The beam begins from offset (forward -1, right 7, vertical view height).
- The weapon uses Energy and consumes one unit unless infinite ammo or
  temporary invulnerability is active. Quad damage does not scale this
  weapon's damage or knockback.
- Weapon animation endpoints are 3/19/57/62, with no pause frames and firing
  frame 4.

## Current remaster comparison

- The dynamic route uses the specified frame schedule, and the remaster retains
  the trace range, mode-dependent damage/kick ranges, delayed 0.5-second hit,
  red marking, zoom setting, and ammo consumption.
- The remaster omits the source's 1-point immediate hit and target velocity
  push. Its delayed hit instead applies all selected damage and kick together;
  the specified delayed hit has no kick. It raises the target by 100 rather
  than 80 units. It also only marks living clients and monsters, while other
  damageable entities can be hit by the source trace. The remaster omits the
  separate damage to damageable windows.
- The remaster traces with a simpler mask, displays two rail-trail events
  rather than short-lived beam entities, and only emits a bubble trail through
  water. The specified bubble trail spans every shot.
- Its normal firing adds an audible railgun shot and does not apply the
  source's backward player-velocity impulse. Zoomed firing also lacks the
  specified velocity impulse and applies view recoil instead. The beam start
  position also differs from the specified offset.

## Verification checklist

- Test multiple targets on one trace, immediate and delayed damage, target
  marking/cleanup, knockback, window damage, beam and bubble effects.
- Verify damage ranges in both modes and normal versus zoomed firing behavior.
- Verify ammo and animation behavior with infinite ammo and invulnerability.
