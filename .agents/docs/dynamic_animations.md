# Dynamic Monster Animations & M_MoveFrame Technical Documentation

## Overview
Quake II Remaster (Yquake2) supports dynamic monster animations alongside traditional static `mmove_t` / `mframe_t` frame tables. Dynamic animations allow monsters to look up frame ranges directly from model metadata (`dmdxframegroup_t`) via `gi.GetModelInfo()` rather than hardcoding static C frame arrays where movement distance (`dist`) is zero (`0`) and `thinkfunc` is `NULL`.

---

## Architecture & Mechanics

### 1. `M_MoveFrame(edict_t *self)`
The core function driving monster animation frames each tick (`monster_think` -> `M_MoveFrame`). It checks:
- **Static Move (`self->monsterinfo.currentmove`)**: If present, uses `move->firstframe` and `move->lastframe`.
- **Dynamic Action (`self->monsterinfo.action`)**: If `currentmove` is `NULL` but `self->monsterinfo.action` is set, it computes the frame range using:
  - `firstframe = self->monsterinfo.firstframe`
  - `lastframe` derived from `numframes` (`firstframe + numframes - 1` or reverse).

### 2. Frame Advancement & Completion
- Each tick, if `AI_HOLD_FRAME` is not set, `self->s.frame` increments (or decrements if reversed).
- When `self->s.frame` reaches `lastframe`:
  - **Static Moves**: Calls `move->endfunc(self)` if defined.
  - **Dynamic Actions**: Automatically handles completion transitions based on the action name:
    - `"attack"`, `"activate"`, `"pain"`, `"dodge"`, `"melee"`: Transitions back to running (`self->monsterinfo.run(self)`).
    - `"deactivate"`: Transitions to idle (`self->monsterinfo.idle(self)`).
    - `"death"`: Transitions to dead (`monster_dynamic_dead(self)`).
    - Loops back to `firstframe` otherwise.

---

## Supported Dynamic Actions & Transitions

The following action strings (`self->monsterinfo.action`) are recognized and supported by the dynamic animation subsystem in `g_monster.c`:

| Action String | Purpose / Context | Completion Behavior |
| --- | --- | --- |
| `"stand"` / `"hover"` / `"swim"` | Stationary / Idle standing loops | Loops indefinitely |
| `"walk"` / `"run"` / `"fly"` / `"swim"` | Movement loops | Loops indefinitely |
| `"idle"` / `"standidle"` | Fidget / idle loops | Loops indefinitely |
| `"attack"` / `"melee"` | Attack animations | Automatically calls `self->monsterinfo.run(self)` on completion |
| `"pain"` | Pain reactions | Automatically calls `self->monsterinfo.run(self)` on completion |
| `"dodge"` | Dodging animations | Automatically calls `self->monsterinfo.run(self)` on completion |
| `"activate"` / `"deactivate"` | Entity activation / deactivation | Deactivate calls `self->monsterinfo.idle(self)` on completion |
| `"death"` | Death sequences | Calls `monster_dynamic_dead(self)` on completion |

---

## Setting Up Dynamic Actions in Monster Code

To replace static `mmove_t` frame tables (where `dist == 0` and `thinkfunc == NULL`) with dynamic actions:
1. Remove the static `mframe_t` array and `mmove_t` structure.
2. Remove the `mmove_t` declaration from `gamemmove_decs.h` and `gamemmove_list.h`.
3. In the monster function (e.g., pain/attack/etc.), set:
   ```c
   self->monsterinfo.firstframe = FRAME_startname;
   self->monsterinfo.numframes = FRAME_endname - FRAME_startname + 1;
   monster_dynamic_action(self, "action_name", select_index);
   ```
   *(Where `select_index` is `-1` for random selection among duplicate group names, or `>= 0` for an exact group index).*

---

### AI Movement Functions (`ai_run`, `ai_walk`, `ai_stand`, `ai_move`, `ai_charge`) & Distances (`walk_dist`, `run_dist`)

In traditional static `mmove_t` definitions, each `mframe_t` specifies a think function (typically one of the core AI movement functions) and a movement distance (`dist`). In dynamic actions, since frames are looked up dynamically from model group headers rather than static `mframe_t` arrays, AI movement behavior is governed by the monster's state functions (`monsterinfo.run`, `monsterinfo.walk`, `monsterinfo.stand`, etc.):

1. **`ai_move(edict_t *self, float dist)`**: Moves the entity forward/backward by `dist` at the current facing angle (`self->s.angles[YAW]`).
2. **`ai_stand(edict_t *self, float dist)`**: Used while standing or idle; applies optional position adjustments (`dist`) and updates enemy targeting/yaw.
3. **`ai_walk(edict_t *self, float dist)`**: Handles walking movement and obstacle avoidance checks toward goals or waypoints.
4. **`ai_run(edict_t *self, float dist)`**: Handles running movement towards the current enemy with pathfinding and combat positioning checks.
5. **`ai_charge(edict_t *self, float dist)`**: Advances towards the enemy during attack states while maintaining attack range and facing.

#### Movement Distances (`walk_dist`, `run_dist`)
When running or walking via dynamic actions, monsters utilize `monsterinfo.walk_dist` and `monsterinfo.run_dist` (stored in `monsterinfo`) to determine speed/distance per tick, scaled by `monsterinfo.scale`. These replace the per-frame `dist` values found in traditional `mframe_t` tables.


When converting static moves where `dist == 0` and `thinkfunc == NULL` to dynamic actions, the movement distance is zero across all frames, making them ideal candidates for dynamic animation lookup because no per-frame distance or custom think callback is required.

### Action to AI Movement Function Mapping

| Action Category | Typical AI Function(s) | Description |
| --- | --- | --- |
| **Stand / Idle** | `ai_stand` | Stationary adjustments while scanning or idling. |
| **Walk / Movement** | `ai_walk` | Patrolling or moving toward destinations at walking speed. |
| **Run / Chase** | `ai_run` | Chasing and pathfinding towards active enemies. |
| **Attack / Melee** | `ai_charge` / `ai_move` | Advancing or holding position while executing attacks. |
| **Pain / Death** | `ai_move` (dist `0`) | Inert reactions where movement is zero (`dist == 0`). |



For moves that require static `mmove_t` structures (e.g. when movement distances or callbacks are needed) but need to resolve their frame indices dynamically from model groups, Yquake2 provides helper functions in `src/game/g_monster.c`:

### 1. `void M_SetAnimGroupMMove(edict_t *self, mmove_t *mmove, const mmove_t *mmove_base, const char *name, int select)`
- **Purpose**: Copies base `mmove` and remaps its `firstframe` and `lastframe` based on model frame group matching `name`.
- **Parameters**:
  - `self`: Monster edict entity.
  - `mmove`: Target `mmove_t` structure to initialize.
  - `mmove_base`: Static baseline `mmove_t` structure.
  - `name`: Model frame group name (e.g., `"stand"`, `"walk"`, `"run"`, `"pain"`, `"death"`).
  - `select`: Group index or variant selector (`0`, `1`, etc.).

### 2. `void M_SetAnimGroupMMoveOffset(edict_t *self, mmove_t *mmove, const mmove_t *mmove_base, const char *name, int select, int offset)`
- **Purpose**: Similar to `M_SetAnimGroupMMove`, but applies an frame offset adjustment when animations start offset from group beginnings or have sub-ranges.


