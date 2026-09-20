---
name: clean-room-porting
description: Clean-room software development process for copying/porting functionality from mods with GPL-incompatible licenses. Use when porting features or code from proprietary, non-commercial, or restrictive mods into GPL-licensed projects (such as Quake II remasters) while maintaining strict legal separation via functional specifications and isolated black-box implementation.
---

# Clean-Room Porting Guide

This skill provides a rigorous, legally sound workflow for porting functionality, mechanics, or features from mods carrying GPL-incompatible licenses (e.g., proprietary, source-available, non-commercial, restrictive custom licenses) into a GPL-licensed codebase (such as `yquake2remaster`).

## Legal & Compliance Principles

When porting code from a non-GPL-compatible codebase into a GPL codebase, direct copying, translation, or adaptation of source code constitutes copyright infringement. To maintain legal safety, you must use a **Two-Team / Clean-Room Design** paradigm (simulated or strictly sequenced):

1. **The Information Analyst (Specification Writer):** Analyzes the source mod's behavior, public interfaces, data structures, network protocols, console commands, and user-facing features to produce a purely functional specification document (`SPEC.md`). **This person/phase MUST NOT view, copy, or write any target codebase code, nor let source code directly cross over.**
2. **The Clean-Room Implementer:** Writes new code from scratch based *exclusively* on the functional specification (`SPEC.md`), without ever reading the original source mod's source code.

---

## Clean-Room Porting Workflow

### Step 1: Scope & License Verification
1. Inspect the source mod's `LICENSE`, `README`, or header files.
2. Confirm the license is **not** compatible with the GNU General Public License (e.g., proprietary, non-commercial use only, restrictive redistribution).
3. Define precisely *what* features or components are to be ported (e.g., specific game mechanics, netcode extensions, UI menus, file formats, or monster AI routines).

### Step 2: Functional Specification Generation (Black-Box Analysis)
Create a `SPEC.md` document that defines the external behavior without referencing or containing copyrighted source code implementations.

**Strict Rules for `SPEC.md`:**
- **NEVER** mention or keep original function names, internal routine names, callback names, or internal variable names from the source mod.
- `SPEC.md` must contain ONLY:
  - High-level descriptions of actions, input/output states, and state machine transitions.
  - **Weapon Animation & Firing Behavior:** Descriptions of frame sequences, activation/firing/idle timing flows, view/hand model switching logic, and ammo decrement triggers during weapon use.
  - Numerical constant values.
  - Constants common to all mods defined in `game.h`, `local.h`, or `shared.h`.
  - Values of text fields such as sound names, model names, classnames, icons, and pickup strings.

*Crucial Rule:* The spec writer must **never** write the target implementation code.

### Step 3: Clean-Room Implementation
1. Review the generated `SPEC.md` specification.
2. Implement the feature in the target codebase from scratch, strictly adhering to target project coding styles, architectural patterns, and licensing headers (GPLv2+).
3. Ensure variable names, function structures, and internal implementations are independently derived and structurally distinct from the source mod.

### Step 4: Compliance & Verification Audit
1. Verify that no source code from the GPL-incompatible mod exists in the commit history or codebase.
2. Test the ported feature against the functional specification to ensure full compatibility and correctness.
3. Document the clean-room process and reference `SPEC.md` in commit messages or internal documentation if necessary.
