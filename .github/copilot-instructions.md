# GitHub Copilot Instructions for Yamagi Quake II Remaster

These instructions are intended to guide GitHub Copilot (and similar
assistant models) when generating or modifying code in this repository.
They are derived from the human contributor guidance found in `doc/`.

## Principles

- **Scope:** Only propose changes that are appropriate for Yamagi Quake II.
  If you are uncertain about scope or intent, ask instead of guessing.

- **Ask before big changes:** For non-trivial or large feature work, request
  clarification before producing large patches.

- **No new dependencies:** Do not introduce new third-party libraries or
  build dependencies. If a helper is required, prefer existing code or
  ask for permission first.

- **Small, focused commits:** Prefer a sequence of small, self-contained
  changes rather than a single large patch. Present changes as
  incremental commits when possible.

## Code Quality

- **Language:** Code must be written in **C only** (no C++ or other languages).
  Use `const` for parameters and variables whenever possible to improve clarity
  and safety.

- **Global variables:** If a global variable is only used within a single file,
  declare it as `static` to limit its scope and avoid unintended external access.

- **Functions:** If a function is only used within a single file, declare it as
  `static` to restrict its visibility and prevent accidental external linkage.

- **Printing:** Use `Com_Printf` (or similar engine-provided logging functions)
  instead of `printf` to ensure consistent output handling across platforms and
  subsystems.

- **Compilation:** Generated code must compile cleanly with current GCC and
  Clang (avoid generating code that purposefully creates warnings).

- **Portability:** Avoid non-portable assumptions. Generated code should
  work on Unix-like systems and Windows unless the change targets a
  platform-specific subsystem.

- **Local style:** Match the coding style in the file being edited (spacing,
  brace placement, naming). Use nearby code as the canonical example.

- **Avoid unrelated cleanups:** Do not perform cosmetic refactors or
  wide-ranging formatting changes unless those are explicitly asked for.

## Documentation & Workflow

- **Update docs:** If you add or change functionality, update the relevant
  document in `doc/` (installation, configuration, commands, cvars,
  packaging, etc.) unless the change is trivial.

- **Contribution path:** Assume contributions are delivered via GitHub forks
  and pull requests. Do not instruct users to send patches by email or
  pastebin.

- **Commit messages:** Suggest clear, concise commit messages that reflect
  small, logical steps.

## Interaction Hints

- When inserting comments for maintainers, reference these instructions
  or `doc/080_contributing.md`.

- If unsure about design decisions, produce a short design note and a
  minimal proof-of-concept rather than a full large implementation.

---

## Best Practices Example

```c
/* Example of applying const, static globals, static functions, and Com_Printf */

/* Static global variable limited to this file */
static const int max_players = 64;

/* Static helper function only used in this file */
static void
print_player_name(const char *name)
{
    /* Use Com_Printf instead of printf */
    Com_Printf("Player: %s\n", name);
}

/* Public function with const parameter */
void
add_player(const char *name)
{
    if (name == NULL)
    {
        return;
    }

    print_player_name(name);
}
```
