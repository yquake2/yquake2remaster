# Dynamic Monster Animations, Entity Database (`models/entity.dat`), and Spawn Fallbacks

## Overview
Quake II Remaster (Yquake2) supports dynamic monster and entity animations alongside traditional static `mmove_t` / `mframe_t` frame tables. Dynamic animations allow entities and monsters to look up frame ranges directly from model metadata (`dmdxframegroup_t`) via `gi.GetModelInfo()` rather than hardcoding static C frame arrays.

Furthermore, Yquake2 features a dynamic entity definitions system (`models/entity.dat`) loaded at startup. This system allows custom entities, mod assets (such as Quake 1 or Half-Life content), and re-release vault entities to spawn correctly even if they lack hardcoded C spawn functions (`ED_CallSpawn` fallback).

---

## The Entity Database (`models/entity.dat`)

### Purpose
`models/entity.dat` contains pipe-delimited (`|`) lines defining dynamic entity properties, model paths, collision bounding boxes, health, mass, speeds, and spawn animation states. When an entity is spawned via `ED_CallSpawn(edict_t *ent)`:
1. The engine checks item definitions.
2. The engine checks static C spawn functions (`StaticSpawnSearch`).
3. If no static C spawn function is found, it checks dynamic definitions via `DynamicSpawnSearch(ent->classname)`.
4. If a matching dynamic entity entry is found (`dyn_id >= 0`), `DynamicSpawnUpdate()` and `DynamicSpawn()` are invoked to configure the entity.

### File Format & Fields
Each line in `models/entity.dat` follows a pipe-delimited format (`|`):
1. **Classname**: Entity classname (e.g., `monster_zombie`, `monster_shambler`).
2. **Model Path**: Relative path to the model (`models/.../tris.mdl` or multi-model paths separated by `;`).
3. **Scale**: 3-vector scale (`x|y|z`).
4. **Entity Type**: Category or behavior type (e.g., `general`, `charfly`).
5. **Mins**: Bounding box minimums (`xmin|ymin|zmin`).
6. **Maxs**: Bounding box maximums (`xmax|ymax|zmax`).
7. **No Shadow**: Shadow flag.
8. **Solid Flag**: Collision solidity (`1` for `SOLID_BBOX`, `0` for `SOLID_NOT`).
9. **Walk Speed**: Default walking speed (`walk_dist`).
10. **Run Speed**: Default running speed (`run_speed`).
11. **Speed**: Movement speed.
12. **Lighting**: Lighting flags.
13. **Blending**: Blending options.
14. **Target Sequence**: Target animation sequence.
15. **Misc Value**: Miscellaneous flags.
16. **No Mip**: Mipmapping flag.
17. **Spawn Sequence**: Initial spawn animation sequence/group.
18. **Description**: Human-readable description (used for entity touch inspection messages).
19. **Color**: RGB color vector (`r|g|b`).
20. **Health**: Default entity health.
21. **Mass**: Default entity mass.
22. **Damage**: Default attack damage (`dmg`).
23. **Damage Range**: Damage range (`dmg_range`).
24. **Damage Aim**: Aim adjustment vector.
25. **Gib Health**: Health threshold for gibbing.
26. **Gib Type**: Gib model/debris type.

---

## Default Spawn Values for Entities Without a Spawn Function

When an entity is loaded from a map file or spawned via console commands (`spawnentity`) and **does not have a hardcoded static spawn function** (`StaticSpawnSearch` returns `NULL`), Yquake2 relies on dynamic entity definitions (`models/entity.dat`) or fallback defaults defined in `g_spawn.c`:

### 1. Fallback Defaults (`DynamicSpawn` & `DynamicSpawnUpdate`)
If an entity classname matches an entry in `models/entity.dat`, `DynamicSpawnUpdate` and `DynamicSpawn` apply the following robust defaults when entity fields are uninitialized (`0` or unset):

| Property | Default Value / Fallback Rule | Description |
| --- | --- | --- |
| **Health** (`self->health`) | `data->health` (from `entity.dat`), defaulting to `100` if `self->health <= 0`. | Ensures monsters and breakables are killable. |
| **Mass** (`self->mass`) | `data->mass` (from `entity.dat`), defaulting to `100` if `self->mass <= 0`. | Determines knockback and physics response. |
| **Damage** (`self->dmg`) | `data->damage` | Attack damage. |
| **Damage Range** (`self->dmg_range`) | `data->damage_range` | Damage variation range. |
| **Bounding Box** (`mins` / `maxs`) | Loaded from `entity.dat` or queried from model frame info (`gi.GetModelFrameInfo`). | Collision box dimensions. |
| **Scale** (`self->rrs.scale`) | `data->scale` (or `st.scale` if provided via spawn spawnvars). | Entity scaling factor. |
| **Solidity** (`self->solid`) | `SOLID_BBOX` if `solidflag > 0`, else `SOLID_NOT`. | Collision state. |
| **Movement Type** (`self->movetype`) | Automatically determined by inspecting model animation groups (`walk`, `run`, `swim`, `fly`). | Sets `MOVETYPE_STEP` for walking/swimming/flying monsters, or `MOVETYPE_NONE` for stationary entities. |
| **Run Speed** (`self->monsterinfo.run_dist`) | `data->run_speed`, defaulting to model width (`maxs[0] - mins[0]`) if `<= 0`. | Distance traveled per tick when running. |
| **Walk Speed** (`self->monsterinfo.walk_dist`) | `data->walk_speed`, defaulting to half-width (`speed / 2`) if `<= 0`. | Distance traveled per tick when walking. |
| **Think / Idle Behavior** | If model has an `"idle"` animation group and is not a stepping monster, assigns `dynamicspawn_think` to loop idle frames. | Ambient animation loop for static props/decorations. |
| **Touch Interaction** | `dynamicspawn_touch` | Prints entity classname or description when touched by players (if no custom message is set). |

### 2. SiN / Custom .def Fallback
If an entity defines a `.def` model path in its `model` field (e.g. `model "some.def"`), Yquake2 automatically loads the definition file, sets default bounding boxes (`-16, -16, -16` to `16, 16, 16`), and dynamically spawns the entity.

---

## Dynamic Monster Animation Architecture & Mechanics

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

## AI Movement Functions (`ai_run`, `ai_walk`, `ai_stand`, `ai_move`, `ai_charge`) & Distances

In dynamic actions, AI movement behavior is governed by the monster's state functions and AI helper functions:
1. **`ai_move(edict_t *self, float dist)`**: Moves the entity forward/backward by `dist` at current heading.
2. **`ai_stand(edict_t *self, float dist)`**: Used while standing or idle.
3. **`ai_walk(edict_t *self, float dist)`**: Handles walking movement and obstacle avoidance.
4. **`ai_run(edict_t *self, float dist)`**: Handles running movement towards the current enemy.
5. **`ai_charge(edict_t *self, float dist)`**: Advances towards the enemy during attacks.

---

## Replacing Static Frame Tables with Dynamic Actions

To replace static `mmove_t` and `mframe_t` tables with dynamic actions:
1. Remove the static `mframe_t` array definition and associated `mmove_t` structure.
2. Set `self->monsterinfo.currentmove = NULL;`, configure AI function pointers (`self->monsterinfo.ai_stand = ai_stand;`, etc.), and set `self->monsterinfo.action` / `firstframe` / `numframes`.

---

## Hybrid MMove Group Helpers

For moves requiring static `mmove_t` structures with dynamic frame lookup:
1. `void M_SetAnimGroupMMove(edict_t *self, mmove_t *mmove, const mmove_t *mmove_base, const char *name, int select)`
2. `void M_SetAnimGroupMMoveOffset(edict_t *self, mmove_t *mmove, const mmove_t *mmove_base, const char *name, int select, int offset)`
