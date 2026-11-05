# Functional Specification: Infinity Dynamic Weapons

## Scope

The README identifies six Infinity weapons: `weapon_pistol`,
`weapon_6bshot`, `weapon_biggun`, `weapon_blaze`, `weapon_goop`, and
`weapon_rifle`. Each weapon must route to its own firing behavior and animation
schedule when selected.

## Weapon animation schedules

| Weapon | Activation / fire / idle / deactivation end | Pause frames | Fire frames |
| --- | --- | --- | --- |
| `weapon_pistol` | 4 / 8 / 52 / 55 | 13, 40 | 5 |
| `weapon_6bshot` | 4 / 46 / 84 / 88 | none | 6, 9, 12, 15, 18, 21 |
| `weapon_biggun` | 3 / 19 / 57 / 62 | none | 4 |
| `weapon_blaze` | 7 / 48 / 79 / 84 | none | 8-43 |
| `weapon_goop` | 4 / 11 / 51 / 55 | 14 | 6 |
| `weapon_rifle` | 6 / 20 / 60 / 67 | none | 7-13 |

The per-weapon behavior, resource identifiers, ammunition rules, and
verification criteria are recorded in the six `weapon_*_spec.md` documents.

## Current remaster comparison

- The remaster validates the entity, client, selected weapon, and classname
  before dispatch. It recognizes all six classnames and uses the required
  animation endpoints, pause frames, and fire-frame lists.
- Each route calls the corresponding remaster firing behavior; detailed
  behavioral differences are listed in the individual specifications.
- The source's shared weapon controller checks ammunition before starting a
  firing sequence. The remaster preserves this for the pistol, shotgun,
  Biggun, and Goop routes, while it deliberately allows the Rifle and Blaze
  callbacks to run without ammo so those callbacks can handle mode switching
  and charging. The Rifle path handles empty ammo; the Blaze path currently
  adds charge even with no ammo.
