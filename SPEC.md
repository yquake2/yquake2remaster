# Functional Specification: Item Radar (Clean-Room Porting Spec)

## 1. Overview
The Radar is a collectible inventory item and active powerup that allows players to scan the surrounding area for nearby entities (players and monsters) and display their relative positions, distances, and elevation changes on a radar display overlaid on the user interface.

---

## 2. Item Definition & Properties
- **Classname:** `item_radar`
- **Editor Bounding Box:** `(-16 -16 -16)` to `(16 16 16)`
- **Spawn Flags:** Standard item flags.
- **Model:** `models/items/radar/tris.md2` with rotation effect (`EF_ROTATE`).
- **Pickup Sound:** `items/pkup.wav`
- **HUD Icon:** `i_radar`
- **Pickup String:** `Radar`
- **Item Flags:** Powerup category (`IT_POWERUP`), inventory quantity tracking (`2` inventory width).
- **Activation / Usage Type:** Toggle-based powerup (`Use_Radar`, `Drop_Radar`).

---

## 3. Mechanics & State Behavior

### 3.1. Activation & Deactivation (`Use`)
- **Toggle State (`toggleradar`):**
  - **Turning ON:**
    - Checks if the player has "energy" in their inventory (`FindItem("energy")`).
    - If energy inventory count is `0`, plays failure message (`"No energy for radar\n"` or similar) and aborts activation.
    - If energy is available, sets radar active state to true and plays activation power sound (`misc/power1.wav`).
  - **Turning OFF:**
    - Sets radar active state to false and plays deactivation power sound (`misc/power2.wav`).

### 3.2. Dropping (`Drop`)
- If the radar is currently active and the player drops their last radar item (inventory count equals `1`), the radar is automatically turned off before being dropped.

### 3.3. Energy Consumption & Update Loop
- **Update Frequency / Throttling:**
  - Radar rendering and state updates are processed during screen/HUD updates.
  - An idle timer restricts excessive updates/processing intervals (e.g., 1.5 seconds minimum cadence when scoreboard/help toggles are involved).
- **Energy Drain Rate:**
  - When the radar is active, energy is consumed over time. Every 5 radar update cycles (i.e. `radarnum` counter reaching 5), `1` unit of energy (`energy` inventory item) is decremented.
  - If energy is fully depleted during an active radar session, the radar automatically deactivates, prints an exhaustion message (`"Energy exhausted\n"`), and plays the powerdown sound (`misc/power2.wav`).

---

## 4. Visual Rendering & HUD Layout

### 4.1. Stat Constants
- **Icon Stat Index:** `STAT_RADAR_ICON` (value `18`) stores the image index for the radar icon when active.
- **Value Stat Index:** `STAT_RADAR` (value `19`) stores the current energy count displayed on the HUD.

### 4.2. Layout Screen Coordinates & Elements
- **Radar Background Graphic:** `radar/radar` drawn at screen position `xv 0 yv 136`.
- **Scan Radius:** 500 units (`RADAR_RADIUS`).
- **Max Layout String Length:** 1400 characters (`RADAR_MAX`).

### 4.3. Target Entity Filtering
- Scans all entities within `RADAR_RADIUS` of the player's origin (`findradius`).
- **Exclusions:**
  - Ignores the player themselves (`head == ent`).
  - Ignores entities that are neither clients (players) nor monsters.
  - Ignores dead or non-living entities (`head->deadflag` or `head->health <= 0`).

### 4.4. Relative Position & Elevation Indicators
- For each valid target found:
  - Computes the 2D vector offset from player to target, normalizing and rotating it by the player's yaw view angle (`ent->s.angles[1]`).
  - Scales the coordinate relative to the radar display radius (mapping 500 units to a 32-pixel radar radius).
  - Determines relative elevation (`hd = target_z - player_z`):
    - **Equal elevation (`hd == 0`):** Uses standard dot graphic (`radar/dot`).
    - **Target higher (`hd < 0`):** Uses upward indicator graphic (`radar/up`).
    - **Target lower (`hd > 0`):** Uses downward indicator graphic (`radar/down`).
  - Appends drawing commands (`xv [x] yv [y] picn [tag]`) to the layout string sent via `unicast`.
