# Functional Specification: Zaero Visor & Security Camera

Clean-room specification for reimplementing the Zaero mission pack's
portable camera viewer ("Visor") and the stationary security camera
entity in a GPL codebase. This document describes externally observable
behavior, data values, and asset references only. It was produced by
black-box analysis of the Zaero mod (whose source is under the
GPL-incompatible Id "Limited Program Source Code License") and contains
no source code, internal function names, or internal variable names
from that mod. Implementations must be written from this document
alone.

## Overview

The Visor is an inventory powerup. Using it lets the player see through
stationary security cameras placed in the map, cycling between them.
Viewing time is a limited, per-player budget that is consumed while the
view is active. The player's body stays behind, visible and vulnerable;
taking any damage interrupts the view.

## Entity: `misc_securitycamera`

* Model: `models/objects/camera/tris.md2`, solid, immobile,
  bounding box mins `(-16 -16 -32)`, maxs `(16 16 0)`.
* Required key `message`: display name of the camera shown on the HUD
  while viewing. A camera without a message is invalid (diagnostic and
  removal).
* Key `mangle` (vector, pitch/yaw/roll): the direction the camera view
  faces. The model itself is rendered with yaw only (pitch and roll 0).
* View point: entity origin moved 8 units along the model's forward
  (yaw) direction and 32 units straight down.
* Power state:
  * With a `targetname`: spawns powered OFF; each activation by its
    targeting entity toggles power ON/OFF.
  * Without a `targetname`: always powered ON.
* While powered, the model animates by advancing one frame per server
  frame (10 Hz) through frames 0..59 in a loop; unpowered cameras do
  not animate.
* The camera cannot be destroyed by weapons. When it takes a hit it
  flashes with a green color shell for 0.2 seconds.
* Only powered cameras can be connected to by the Visor.
* While the camera is being viewed, the delivered view angles sway
  horizontally: yaw offset = sin(2 * pi * ((server frame count modulo
  64) / 64)) * 15 degrees around the `mangle` yaw.

## Item: `item_visor`

* Classname `item_visor`, world model `models/items/visor/tris.md2`
  (rotating), HUD icon `i_visor`, pickup name `Visor`, pickup sound
  `items/pkup.wav`. Flags: powerup, stays in coop. Precached sounds:
  `items/visor/act.wav`, `items/visor/deact.wav`.
* View-time budget: measured in server frames (10 per second). A full
  budget is 300 frames (30 seconds).
* Pickup rules:
  * Refused when the player already owns a Visor with a full budget.
  * A map-placed visor sets the budget to the full 300.
  * A dropped visor carries the remaining budget of the player who
    dropped it and adds that amount on pickup.
  * Inventory count is always 0 or 1.
  * In deathmatch, a map-placed visor respawns 30 seconds after pickup.
* Drop: the dropped item records the player's remaining budget; the
  player's budget becomes 0 and the item leaves the inventory.

## Using the Visor

* Use with no view active:
  * If the budget is empty: print "No time left for visor" to the
    player, do nothing.
  * If no powered camera exists: print "No cameras are available", do
    nothing.
  * Otherwise connect to the first available powered camera: the view
    switches to that camera, an activation sound
    (`items/visor/act.wav`) plays, the screen is covered by a solid
    white "static" flash for 0.2 seconds, and a HUD overlay appears.
* Use while a view is active: switch to the next powered camera in
  cyclic map order (skipping unpowered ones), playing the activation
  sound and another 0.2 second static flash. If no other powered
  camera exists, nothing changes.

## While viewing

* The player is frozen in place and their weapon model is hidden; the
  field of view is forced to 90.
* The view is rendered from the camera's view point with the camera's
  `mangle` angles plus the horizontal sway described above.
* The player's body remains behind at its position, visible to others
  and vulnerable: any damage the player takes immediately ends the
  view. (Engine adaptation: a solid stand-in body occupies the
  player's position and forwards damage it receives to the player.)
* HUD overlay: text "Tracking <camera message>" near the lower left of
  the screen (virtual x 26, 75 units above the screen bottom) and the
  remaining viewing time; refreshed about once per second.
* The time budget decreases by 1 each server frame. When it reaches 0
  the view ends and the Visor item is consumed (removed from the
  inventory).
* Restricted input: while viewing, only these console commands work:
  * `putaway` - ends the view,
  * `use Visor` - switches to the next camera,
  * `invuse` - acts only when the selected inventory item is the Visor,
  * `invnext` / `invprev` - inventory navigation.
  All other client commands are ignored (except during intermission).
* The view also ends when the player dies or disconnects, when the
  level ends, or when the viewed camera entity is removed.
* Ending the view plays `items/visor/deact.wav` and restores the
  normal first-person view, weapon model, and field of view.

## Notes for this codebase

* The remote view transport, player hiding, stand-in body, weapon
  hiding, movement freeze, and lifecycle (death/disconnect/
  intermission/savegame) are provided by the existing RemoteView layer
  in `src/game/g_camera.c`; the Visor attaches through that API like
  the RTDU does.
* The engine derives the client view origin from the player entity, so
  "the player stays behind" is realized by the RemoteView dummy body;
  for the Visor the body is made solid and damage-forwarding to
  preserve the vulnerability rule above.
* Because the player entity is relocated while viewing, monsters
  cannot acquire the player during the view (notarget); existing
  threats can still hurt the body, which interrupts the view as
  specified.
