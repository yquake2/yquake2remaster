# Agent Instructions for Yamagi Quake II Remaster

These instructions are intended to guide AI agents when generating or modifying
code in this repository. They are derived from the human contributor guidance
found in `doc/080_contributing.md`.

If you use any AI tool during development, please include a `Co-authored-by`
line in your commit message, just as you would when crediting human collaborators.
All code must be reviewed and validated by you, and you bear full responsibility
for its correctness and quality.

Example:
```
Co-authored-by: Copilot <https://copilot.microsoft.com>
```
or
```
Co-authored-by: Gemini <gemini@google.com>
```

## Principles & Scope

- **Ask before writing code:** As a golden rule, always check whether an idea or feature is in scope for Yamagi Quake II before implementation. An early "no" saves wasted effort.
- **Scope:** Only propose changes that fit the Yamagi Quake II project scope. If uncertain, ask instead of guessing.
- **No new dependencies:** Do not introduce new third-party libraries or build dependencies. If a helper is required, prefer existing code or ask for permission first.
- **Small, focused commits:** Prefer a sequence of small, self-contained commits rather than a single large patch.

## Code Quality & Style

- **Language:** Code must be written in **C only** (no C++ or other languages).
- **Const & Static Correctness:**
  - Use `const` for parameters and variables whenever possible.
  - Declare file-local global variables and helper functions as `static` to restrict visibility.
- **Logging:** Use `Com_Printf` (or engine-provided logging equivalents) instead of `printf` for cross-platform consistency.
- **Compilation & Portability:** Code must compile cleanly **without warnings** on current GCC and Clang, and must work on both Unix-like systems and Windows.
- **Local Style & Legacy Code:** Match the coding style of the specific file being edited. **Do not perform unnecessary cleanups or run aggressive linters/formatters on untouched code.** Quake II's codebase has historical style quirks; extraneous refactoring introduces risks and pollutes diffs.

## Documentation & Workflow

- **Update docs:** If you add or change functionality, update the relevant document in `doc/` (installation, configuration, commands, cvars, packaging, etc.) unless the change is trivial.
- **Contribution path:** Deliver contributions via GitHub forks and pull requests.
- **Commit messages:** Suggest clear, concise commit messages that reflect small, logical steps, focusing on *why* changes were made.

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
