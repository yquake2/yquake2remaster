# Camera / Remote View Comparison & Functional Specification

Comparison of every camera-like ("remote view") entity found in the
reference trees checked into this workspace, and how
`yquake2remaster` (`src/game`) currently implements each of them.

Compared trees (nothing else was used):

| Short name  | Path in workspace                               | License                                   | Handling                                   |
|-------------|-------------------------------------------------|-------------------------------------------|--------------------------------------------|
| Heretic II  | `hereticII/src`                                 | GPLv3 (Heretic2R)                         | clean-room: GPLv3-only code must not be copied into the GPLv2+ tree |
| Oblivion    | `oblivion`                                      | no license file (mod code on rerelease base) | clean-room                               |
| Zaero       | `zaero/src`                                     | Id "Limited Program Source Code License"  | clean-room                                 |
| Rerelease   | `q2repro/subprojects/rerelease-game/rerelease`  | GPLv2                                     | compatible, may be used as reference       |
| Remaster    | `src/game`                                      | GPLv2+ (this project)                     | target                                     |

This document follows the clean-room rules from
`.agents/skills/clean-room-porting/SKILL.md`: for the GPL-incompatible
trees it describes only externally observable behavior, classnames, map
keys, spawnflags, constants, asset names and text strings. It contains
no source code and no internal function or variable names from those
trees. Names that appear in the "Remaster" column refer to this
project's own GPL code.

The Zaero Visor and security camera are specified in detail in
[zaero_visor_SPEC.md](zaero_visor_SPEC.md); this document only adds the
comparison and the behavior differences that were observed.

---

## 1. Entity / classname inventory

`+` present, `-` absent, `~` alias or partial.

| Classname                   | Kind                         | Heretic II | Oblivion | Zaero | Rerelease | Remaster |
|-----------------------------|------------------------------|:----------:|:--------:|:-----:|:---------:|:--------:|
| `misc_remote_camera`        | point camera, look-at target | +          | -        | -     | -         | -        |
| `misc_camera`               | point camera, path + tracking| -          | +        | -     | -         | +        |
| `misc_camera_target`        | moving look-at marker        | -          | +        | -     | -         | +        |
| `trigger_misc_camera`       | brush trigger for misc_camera| -          | +        | -     | -         | +        |
| `misc_deatomizer_control`   | alias of `misc_camera`       | -          | ~        | -     | -         | -        |
| `misc_deatomizer_target`    | alias of `misc_camera_target`| -          | ~        | -     | -         | -        |
| `target_camera`             | N64 style cutscene camera    | -          | +        | -     | +         | ~ (no hackflags) |
| `misc_securitycamera`       | Visor camera                 | -          | -        | +     | -         | +        |
| `item_visor`                | inventory item, views cameras| -          | -        | +     | -         | +        |
| `weapon_rtdu`               | deployable turret, remote view | -        | +        | -     | -         | +        |
| `info_player_intermission`  | static intermission view     | +          | +        | +     | +         | +        |
| `item_sphere_hunter`        | sphere with "huntercam" view | -          | + (Rogue)| -     | + (Rogue) | +        |
| `script_runner`             | drives scripted cameras      | + (indirect) | -      | -     | -         | -        |
| `viewthing`                 | debug model viewer, **not** a camera | - | -     | +     | -         | +        |

Notes:
* `target_camera` exists in Oblivion only because Oblivion is built on the
  rerelease game; the behavior is identical to the rerelease one.
* `viewthing` was checked and is excluded: it only spawns an animated
  test model and never changes any player view.
* Spectator chase cam (`chasecam` command, `src/game/g_chase.c`) is a
  player mode, not a map entity, and is listed only for completeness in
  the feature table.

---

## 2. Feature comparison

| Aspect | Heretic II `misc_remote_camera` | Oblivion `misc_camera` | Zaero `misc_securitycamera` + `item_visor` | Rerelease `target_camera` | Remaster (`src/game`) |
|---|---|---|---|---|---|
| Started by | `use` (trigger, script) | `use` or `trigger_misc_camera` | player uses Visor item | `use` | same as each source |
| Who sees it | activator only (flag 1) or every connected client | every connected client | only the Visor user | every client (intermission) | `misc_camera`: every living non-spectator client; Visor/RTDU: owner; `target_camera`: everyone |
| Ends by | `delay` timeout, or second `use` if SCRIPTED | `wait` timeout (default 3 s, -1 = never), path corner `wait -1`, second `use` | `putaway`, budget empty, damage, death, disconnect | path end; optional skip by any button | as source + intermission, death, disconnect, view entity removed |
| View position | own origin, or origin of `pathtarget` entity (followed each frame) | own origin, moves along `pathtarget` path_corner chain | fixed eye point (8 forward, 32 down of model) | own origin, then moves along `target` path_corner chain | same as each source |
| View angles | always look at `target` entity | turn 10 % per frame toward tracked entity; otherwise spawn angles | `mangle` + yaw sway ±15° (64 frame period) | look at `pathtarget` entity, else unchanged | same as each source |
| Player body | stays in place, controls frozen | stays in place, hidden, visible copy left behind | stays in place, hidden, visible solid copy | dummy player model left behind, real player moved to intermission | player entity moved to view, visible stand-in body left behind |
| Player damage | **immune** while any remote camera is active | not affected, view continues | damage ends the view | n/a (intermission) | `misc_camera`/RTDU: body intangible, player notarget; Visor: body solid, forwards damage, damage ends view |
| Movement / input | movement zeroed, frozen, all client input skipped, crosshair hidden | frozen; with NO_CONTROL also look/fire/use/holster/jump/crouch blocked | frozen; only Visor-related commands accepted | intermission | frozen; NO_CONTROL blocks fire/use; Visor filters commands |
| Weapon model | n/a (third-person game) | hidden, restored | hidden, restored | intermission | hidden, restored |
| FOV | unchanged | unchanged | forced 90 | unchanged | Visor forces 90, restored from userinfo |
| HUD | none | none (RTDU: powerup wheel state) | "Tracking <message>" + timer | none | Visor: "Tracking <message>" + "Time: N sec" |
| Networking | dedicated player-state fields: camera entity number, origin (1/8 unit), angles; client interpolates; PVS from camera origin | view origin set in player state (rerelease protocol separates view origin from entity) | view origin in player state | intermission origin/angles | player entity relocated (engine derives view origin and PVS from the entity) |
| Overlapping cameras | lock counter; last started camera is shown until all are released | a new camera replaces the previous view | switching cycles cameras | n/a | new controller replaces previous view |
| Deathmatch | kept | removed | allowed (Visor respawns after 30 s) | removed | as source |
| Savegame | regular entity fields | view state saved | client state saved | regular fields | remote view dropped on load; orphan body restores the player |

---

## 3. Heretic II - `misc_remote_camera`

Editor box `(-4 -4 -4) (4 4 4)`, color `(0 0.5 0.8)`.

Map keys:
* `target` (required) - targetname of the entity to look at. Without it:
  diagnostic `Object 'misc_remote_camera' without a target.` and removal.
* `pathtarget` - targetname of an "owner" entity the camera is attached to.
* `delay` - seconds the view stays active (non-scripted cameras).

Spawnflags:
| Bit | Name        | Meaning |
|-----|-------------|---------|
| 1   | ACTIVATING  | only the activator gets the view, otherwise every connected client |
| 2   | SCRIPTED    | stays active until used a second time (used by the script system) |
| 4   | NO_DELETE   | entity is not removed when the view ends |

Behavior:
* Not solid, no model, not moving.
* On `use` while inactive: mark active; for each affected client
  increment a per-client "remote view lock" counter and record this
  camera as the viewed camera.
* On `use` while active: SCRIPTED cameras end the view; others ignore it.
* Position: without `pathtarget` the camera stays at its own origin. With
  `pathtarget` the owner entity is searched by targetname every 0.1 s and
  the camera copies its origin (follows moving entities).
* Angles: every 0.1 s the direction to the `target` entity origin is
  converted to angles (pitch sign inverted for the view). If the target
  disappears the last angles are kept.
* Duration (non-SCRIPTED): `delay` is decreased by 0.1 every 0.1 s; when
  it drops below 0 the view ends. A camera with `delay 0` is visible for
  a single 0.1 s frame.
* Ending: affected clients' lock counter is decremented, entity removed
  unless NO_DELETE. A NO_DELETE camera stays marked active, so later
  `use` does nothing unless it is SCRIPTED.
* While a client's lock counter is above 0:
  * movement is zeroed and the player is frozen, player input commands
    are skipped,
  * the player cannot be damaged,
  * the crosshair is hidden,
  * the client view origin/angles come from the camera (sent in the
    player state with 1/8 unit precision, interpolated by the client),
  * entity visibility (PVS) is computed from the camera origin.

---

## 4. Oblivion - `misc_camera`, `misc_camera_target`, `trigger_misc_camera`, RTDU

### 4.1 `misc_camera`

Editor box `(-8 -8 -8) (8 8 8)`, spawnflag 1 `NO_CONTROL`.

* Removed in deathmatch. Invisible to clients, not solid, zero size.
  Model index `sprites/s_deatom1.sp2` is assigned but never sent.
* Missing `targetname`: diagnostic `<classname> with no targetname`,
  targetname becomes `unused`.
* Keys: `wait` (default 3, `-1` = never times out), `target` (entity to
  track), `pathtarget` (first `path_corner` of the fly path), `speed`
  (path speed, default 100).
* `use` toggles active/inactive.
* Activation:
  * resolve `target` (random pick among matches) as tracked entity;
    diagnostic `<classname> target <name> not found` if missing,
  * if `pathtarget` names a `path_corner`, jump to it and start the path,
  * give the view to every connected client,
  * start the timeout.
* Every frame while active:
  * deactivate when `wait` seconds have passed (unless `wait -1`),
  * advance along the path,
  * tracking: aim point = center of the tracked entity bbox raised by its
    view height; new forward = normalize(0.9 * current forward +
    0.1 * direction to aim point).
* Path movement:
  * speed for each leg: `speed` of the path_corner, else the camera
    `speed`, else 100;
  * at a corner with `wait > 0` the camera pauses that long; `wait -1`
    stops the path and deactivates the camera; end of chain stops the
    movement (view stays until timeout);
  * a path that keeps reaching corners instantly is cut after 64 steps
    per frame with diagnostic `camera path loop detected, stopping path`;
  * missing path: diagnostic `<self> path <name> not found`;
  * the generic `path_corner` touch logic ignores these entities.
* Viewing client: frozen, weapon model hidden (restored afterwards),
  view origin and angles from the camera, view height 0, player entity
  hidden from others while a visible, non-solid, non-damageable copy is
  left at (and kept in sync with) the player position. The player itself
  stays where it is and stays vulnerable; damage does not end the view.
* NO_CONTROL: view angle input, forward/side movement and the attack,
  use, holster, jump and crouch buttons are discarded.

### 4.2 `trigger_misc_camera`

Brush trigger, spawnflags `1 MONSTER`, `2 NOT_PLAYER`, `4 TRIGGERED`.

* Removed in deathmatch.
* Activators: players (unless NOT_PLAYER) and monsters (only with
  MONSTER). TRIGGERED spawns disabled, the first `use` enables it.
* `angle`/`angles`: only fires when the activator faces the trigger
  direction (dot product of facing and direction >= 0).
* `message`: center-printed to a non-monster activator with a sound:
  `sounds 1` `misc/secret.wav`, `2` `misc/talk.wav`, `3`
  `misc/trigger1.wav`, otherwise `misc/talk1.wav`.
* `target`: first `misc_camera` with that targetname; otherwise
  diagnostic `Illegal target for trigger_misc_camera`.
  * `wait` (non-zero) overrides the camera `wait`,
  * `pathtarget` replaces the camera's tracked target name, otherwise the
    activator is set as tracked entity (observed: if the camera was
    inactive, activation re-resolves the camera's own target and the
    activator is not tracked),
  * inactive camera: activated; active camera: timeout restarted.
* `delay > 0`: re-arms after `delay` seconds; otherwise single use and
  removed.

### 4.3 `misc_camera_target`

* Removed in deathmatch, invisible marker, missing targetname as above.
* `use` toggles: start moving from the `path_corner` named by `target`
  (speed rules as 4.1, default 100) / stop.
* Stops automatically at the end of the chain or at a corner with
  `wait -1`.

### 4.4 Aliases

* `misc_deatomizer_control` behaves exactly like `misc_camera`.
* `misc_deatomizer_target` behaves exactly like `misc_camera_target`.

### 4.5 RTDU remote view (`weapon_rtdu`)

* Using the RTDU item with a deployed turret toggles a remote view into
  the turret; first use deploys the turret.
* View raised 15 units (`RTDU_REMOTE_VIEW_Z`).
* While viewing: forward/side movement, jump and crouch discarded; turret
  yaw = player yaw, turret pitch = player pitch / 3, roll 0; attack fires
  the turret.
* Turret destruction ends the view. Powerup wheel shows state 2 while
  viewing.

---

## 5. Rerelease - `target_camera` (GPLv2 reference)

Editor box `(-8 -8 -8) (8 8 8)`, "camera path as seen in the N64 version".

* Removed in deathmatch, invisible.
* `use`:
  * `sounds` non-zero: switch CD track to that number,
  * `target` (required) is the first `path_corner`; without it nothing
    happens,
  * intermission starts; a player-looking dummy (custom player model,
    activator skin/frame/velocity, solid, step movement, minimal light,
    animated like a running player) is left at the activator position,
  * dead clients are respawned, every client is moved to the
    intermission point = camera origin, looking at the `pathtarget`
    entity (pitch +/-90 when straight above/below),
  * movement starts after `wait` seconds with `speed`.
* Movement: per 0.1 s the remaining distance shrinks by
  `speed * 0.1 * 0.8`; the view is interpolated between corners; at a
  corner it pauses for the corner `wait`, then continues with the corner
  `speed` (default 55).
* Hackflags (`hackflags` key):
  | Bit | On             | Meaning |
  |-----|----------------|---------|
  | 2   | path_corner    | teleport-out: dummy plays the teleport event and fades out over the corner `wait` |
  | 64  | target_camera  | skippable by any button once level time > 2 s |
  | 128 | target_camera  | show end-of-unit message on use |
* End of path: with `killtarget`, the dummy is removed, the `killtarget`
  entities are used (with intermission cleared during the call), the
  intermission point is marked as already set, and for an end-of-unit
  map change (no `*` in the next map) the intermission is left at once.
* Note: the first leg uses the camera `speed` without default; `speed 0`
  never moves.

---

## 6. Zaero - differences to `zaero_visor_SPEC.md`

Observed behavior that differs from the current Remaster implementation:

| Topic | Zaero | Remaster |
|---|---|---|
| Camera without `message` | fatal error `misc_securitycamera w/o message` | diagnostic and removal |
| Camera damage | immortal (health never reduced), green shell for 0.2 s | high health reset in pain/die, same shell |
| Cameras exist but none powered | camera search never terminates (hang) | prints `No cameras are available` |
| Player position while viewing | player entity stays, only the view moves; real player remains solid and damageable; pain or death ends the view | player relocated, solid stand-in body forwards damage |
| Visible copy | solid, not damageable, follows player frame/model; on userinfo change its model becomes `models/objects/gibs/head2/tris.md2` | stand-in body |
| HUD timer | `xv 26 yb -75 string "Tracking <message>"`, then stat-driven `xv 246 num 3 <timer stat>` and `xv 296 pic <icon stat>` (icon `i_visor`, value = seconds left) refreshed every 32 frames | `xv 26 yb -75 string2 "Tracking <message>" xv 200 yb -75 string "Time: N sec"` refreshed once a second |
| Generic tracking | can also follow a client (eye position + 10 forward) or any entity's angles; only security cameras use it | only security cameras |
| Item quantity / ammo | width 1, quantity 30, ammo "Cells" (unused) | not used |

---

## 7. Related view mechanisms (not dedicated camera entities)

| Mechanism | Sources | Behavior |
|---|---|---|
| `info_player_intermission` | all | static view used during intermission; Heretic II uses the intermission pmove type, which its client also treats as "remote view underway" |
| Hunter sphere ("Sam Raimi cam") | Rerelease (Rogue), Oblivion (Rogue), Remaster | when an owned hunter sphere is launched at an enemy at least 192 units away, the owner is gibbed and flies with the sphere; disabled by `huntercam 0` or forced respawn |
| Spectator chase cam | Rerelease, Remaster | spectator follows another player; command/menu driven |
| Heretic II scripts | Heretic II | `script_runner` scripts use SCRIPTED `misc_remote_camera` entities for cutscenes |

---

## 8. Gaps in `src/game`

| Missing behavior | Source | Notes |
|---|---|---|
| `misc_remote_camera` | Heretic II | needs look-at + owner attachment, ACTIVATING/SCRIPTED/NO_DELETE, per-client lock count; maps to the existing RemoteView layer; damage immunity and per-client visibility differ from `misc_camera` |
| `misc_deatomizer_control`, `misc_deatomizer_target` | Oblivion | plain spawn aliases of `misc_camera` / `misc_camera_target` |
| `target_camera` hackflags 2/64/128 and "intermission already set" | Rerelease | current port has none of them |
| `misc_camera` NO_CONTROL blocks look/holster/jump/crouch | Oblivion | Remaster only blocks movement, attack and use (look angles are overridden anyway) |
| Visor HUD stats (icon + numeric timer) | Zaero | Remaster uses a text timer |
