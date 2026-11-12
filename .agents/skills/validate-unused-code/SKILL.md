---
name: validate-unused-code
description: Validate unused code and internal functions using compiler warnings (-Wmissing-prototypes). Use when identifying functions that lack prototypes, finding unused functions/symbols, or refactoring internal C functions to be static in C source files (applies only to src/, ignoring missionpacks/).
---

# Validate Unused Code & Internal Functions

## Overview

This skill enables systematic detection and refactoring of internal C functions that lack forward prototype declarations or are unused within the core codebase (`src/`, ignoring `missionpacks/`). By compiling with `-Wmissing-prototypes` and `-Wunused-function`, Gemini CLI identifies candidate functions and helps refactor them to be `static` (if file-local), commented out with `#if 0` (if unused/prototype-only), or properly prototyped.

## Workflow

### Step 1: Run Build with Prototype Warnings
Execute `make clean` and compile with `-Wmissing-prototypes` to generate compilation warnings (restricted to `src/`):

```bash
bash .agents/skills/validate-unused-code/scripts/check_prototypes.sh
```

Or manually:
```bash
make clean
DEBUG=1 CC="ccache gcc -Wmissing-prototypes " make -j8
```

### Step 2: Analyze Warnings
Examine the filtered warnings or `/tmp/yq2_missing_prototypes.log` for files inside `src/` (ignoring `missionpacks/`):
- `warning: no previous prototype for ‘function_name’ [-Wmissing-prototypes]`
- `warning: ‘function_name’ defined but not used [-Wunused-function]`

### Step 3: Refactor Candidates
For each candidate function in `src/`:
1. **File-Local Functions (Missing Prototype)**: If the function is only called within its translation unit (`.c` file) and lacks a prototype, modify its definition to use `static`:
   ```c
   static void my_internal_helper(void) {
       ...
   }
   ```
2. **Unused / Prototype-Only Functions**: Validate usage of candidate functions across all `.c` files in `src/`. If a function has no real usage (even if it has a prototype) or is flagged as unused (`-Wunused-function`), **comment out both the function definition and its prototype** using `#if 0` ... `#endif`:
   ```c
   #if 0
   void unused_or_prototype_only_function(void) {
       ...
   }
   #endif
   ```
3. **Cross-File Functions & API Entry Points**: If a function lacks a prototype (`-Wmissing-prototypes`) but is used in other source files in `src/` or exported as module entry points (e.g., `GetGameAPI`, hooks, callbacks):
   - **Do NOT** make these functions `static`.
   - Ensure they have a proper prototype declaration in a shared header file.

### Step 4: Verify
Re-run the build to ensure all targeted warnings in `src/` are resolved and the binary compiles successfully.
```bash
make clean
DEBUG=1 CC="ccache gcc -Wmissing-prototypes " make -j8
```
