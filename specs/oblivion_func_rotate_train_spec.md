# Functional Specification: Oblivion Rotating Train (`func_rotate_train` & `path_corner`)

## 1. Overview
The Rotating Train (`func_rotate_train`) and enhanced path corners (`path_corner` with custom `speeds`) are ported from the Oblivion mod (`oblivion/REBLIVION/src/g_rtrain.cpp`). They provide rotating platform movement along path nodes with variable speed control.

---

## 2. Entity Definition & Properties
- **Classnames:** `func_rotate_train`, `path_corner`
- **Source File:** `oblivion/REBLIVION/src/g_rtrain.cpp`
- **Spawnflags & Fields:**
  - `speeds`: Custom speed parameter for `path_corner` nodes to adjust train velocity dynamically between waypoints.
  - Rotation vectors and angular velocity tracking.

---

## 3. Mechanics & Movement Behavior
- **Waypoint Navigation:** The train calculates trajectory and speed modifications based on target / targetname links to `path_corner` entities.
- **Rotation Handling:** Combines linear translation with continuous rotational updates (`train_rotate`).
