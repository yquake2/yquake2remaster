# Functional Specification: Laser Trip Bomb (`ammo_ired`)

## 1. Description of Actions & Behavior
- When the item is equipped and the primary attack button is pressed, the player performs a deployment action.
- A trace is performed forward from the player view position (up to 64 units). If it hits a solid wall (`worldspawn`), a stationary device is mounted against the wall surface aligned with the surface normal.
- The deployed device projects a linear beam (up to 2048 units).
- If any entity interrupts the projected beam, or if the device receives damage or reaches its timeout, it triggers an explosion resulting in radius damage and the dispersion of shrapnel projectiles.
- Each successful deployment consumes 1 unit from inventory.

## 2. Weapon Animation & Firing Behavior
- **Animation Frames:** Utilizes activation, firing, and idle frame sequences (e.g., active firing frames include 6, 10, and 15).
- **Firing Action (Frame 10):** Evaluates view angles and origin, performs forward trace, and spawns the wall-mounted device if valid. Decrements 1 unit of ammo from inventory, switches the view model index to the hand model variant, and plays the placement/quad sound effect.
- **Model Reset (Frame 15):** Reverts the view model index back to the standard weapon model and resets gun frame state if necessary.

## 3. Numerical Constants & Common Constants
- Max existing instances: `8`
- Timeout duration: `120.0` seconds
- Deployment delay: `1.0` second
- Shrapnel count: `6`
- Shrapnel damage: `15`
- Explosion damage: `200` (scaled by 4 if quad damage active)
- Explosion damage radius: `200.0`
- Common flags/types from headers: `IT_AMMO`, `IT_WEAPON`, `SOLID_BBOX`, `MOVETYPE_NONE`, `DAMAGE_IMMORTAL`, `MASK_SHOT`

## 4. Text Fields, Classnames, Models & Sounds
- **Classname:** `ammo_ired`
- **Pickup Name:** `IRED`
- **Icon:** `w_ired`
- **World Model:** `models/items/ammo/ireds/tris.md2`
- **View Model:** `models/weapons/v_ired/tris.md2`
- **Hand Model:** `models/weapons/v_ired/hand.md2`
- **Object Model:** `models/objects/ired/tris.md2`
- **Shrapnel Model:** `models/objects/shrapnel/tris.md2`
- **Setting Sound:** `weapons/ired/las_set.wav`
- **Arming Sound:** `weapons/ired/las_arm.wav`
