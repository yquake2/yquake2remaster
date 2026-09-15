---
name: quake2-generators
description: A skill for running Yamagi Quake II savegame code generators (mmgen, fngen, mmproto, mmtable, fngenprotos, fngentables scripts). Use it when the user asks to update serialization, generate function tables, edict_t pointers, or mmove_t pointers.

## Scope & Rules
- **Root Repository Only:** These generators should be run **only** for the root repository (`src/game`). Missionpack sub-repositories (such as `missionpacks/ctf`, `missionpacks/rogue`, `missionpacks/xatrix`) must be kept without changes regarding these generators.
- **Sorted Spawn Functions:** Ensure that spawn function definitions and table entries in `src/game/savegame/tables/spawnfunc_list.h` and `src/game/savegame/tables/spawnfunc_decs.h` are kept sorted alphabetically.
---

## References
- [`stuff/mmgen/README.md`](../../stuff/mmgen/README.md)
- [`stuff/fngen/README.md`](../../stuff/fngen/README.md)

---

## 1. Mmove List Generator (`mmgen`)

Scans game source code for `mmove_t` objects to generate table entries and prototypes needed for savegame serialization.

### Usage
Run from the repository root or from `stuff/mmgen`:
```bash
cd stuff/mmgen
./mmproto.sh ../../src/game > ../../src/game/savegame/tables/gamemmove_decs.h
./mmtable.sh ../../src/game > ../../src/game/savegame/tables/gamemmove_list.h
```

### Implementation Details
- Uses `grep` to find `mmove_t (some_name) = {` patterns.
- Uses `awk` to deduplicate names.
- `mmproto.sh` generates prototypes: `extern mmove_t some_name;`
- `mmtable.sh` generates table entries: `{"some_name", &some_name},`

---

## 2. Function List Generator (`fngen`)

Generates function lists and prototypes for saving/loading function pointers within the savegame system (`edict_t` struct function pointers).

### Usage
Run from the repository root or from `stuff/fngen`:
```bash
cd stuff/fngen
./fngenprotos.sh ../../src/game > ../../src/game/savegame/tables/gamefunc_decs.h
./fngentables.sh ../../src/game > ../../src/game/savegame/tables/gamefunc_list.h
```

> **NOTE:** You need to manually inspect and remove `func` and `afterwaitfunc` from the `moveinfo.endfunc` lists if generated, due to how the script identifies points of interest.

### Implementation Details
- `fntable.sh` and `fnproto.sh`: Helper scripts utilizing `grep`, `awk`, and `sed` to extract, deduplicate, and format function pointers.
- `fngentables.sh`: Outputs function tables for all function pointers in `edict_t`.
- `fngenprotos.sh`: Outputs prototypes for functions referenced by those tables.
