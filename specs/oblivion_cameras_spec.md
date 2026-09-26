# Functional Specification: Oblivion Cameras (`misc_camera`, `misc_camera_target`, `trigger_misc_camera`)

## 1. Overview
The Oblivion Camera system (`misc_camera`, `misc_camera_target`, `trigger_misc_camera`) ported from `oblivion/REBLIVION/src/g_camera.cpp` allows cinematic camera views, target tracking, and trigger activation of camera perspectives for players.

---

## 2. Entity Definitions & Properties
- **Classnames:** `misc_camera`, `misc_camera_target`, `trigger_misc_camera`
- **Source File:** `oblivion/REBLIVION/src/g_camera.cpp`
- **Fields:** Target linkage (`target`, `targetname`), FOV, angles, and activation states.

---

## 3. Mechanics & Behavior
- **Trigger Activation:** Stepping into or triggering `trigger_misc_camera` switches the player's view viewport to the designated `misc_camera`.
- **Target Tracking:** `misc_camera_target` allows cameras to dynamically pivot and track moving entities or focal points.
