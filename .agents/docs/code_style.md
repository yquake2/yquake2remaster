# Code Quality & Style

- **Language:** Code must be written in **C only** (no C++ or other languages).
- **Const & Static Correctness:**
  - Use `const` for parameters and variables whenever possible.
  - Declare file-local global variables and helper functions as `static` to restrict visibility.
- **Logging:** Use `Com_Printf` (or engine-provided logging equivalents) instead of `printf` for cross-platform consistency.
- **Compilation & Portability:** Code must compile cleanly **without warnings** on current GCC and Clang, and must work on both Unix-like systems and Windows.
- **Local Style & Legacy Code:** Match the coding style of the specific file being edited. **Do not perform unnecessary cleanups or run aggressive linters/formatters on untouched code.** Quake II's codebase has historical style quirks; extraneous refactoring introduces risks and pollutes diffs.

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
