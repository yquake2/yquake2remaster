#!/usr/bin/env python3
"""Regression check for issue #71: 'give all' must not hand out CTF tech items.

Tech items (item_tech2 strength/quad, item_tech3 haste) are flagged IT_TECH and
are read through the inventory count by CTFApplyHaste() and
CTFApplyStrengthSound(). The give-all loop in Cmd_Give_f() must skip them, the
same way it already skips armor, weapons and ammo.

The game code cannot be linked into a standalone unit test, so this checks the
source of Cmd_Give_f() directly.
"""
import re
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "src" / "game" / "g_cmds.c"


def main():
    text = SRC.read_text(encoding="utf-8", errors="replace")

    start = text.find("Cmd_Give_f(edict_t *ent)")
    if start < 0:
        print("FAIL: Cmd_Give_f not found in g_cmds.c")
        return 1
    end = text.find("Cmd_ListItems_f", start)
    body = text[start:end]

    # The give-all loop is the one that sets inventory to 1 for non-excluded items.
    loop = re.search(
        r"if \(it->flags & \(([^)]*)\)\)\s*\{\s*continue;\s*\}\s*"
        r"ent->client->pers\.inventory\[i\] = 1;",
        body,
    )
    if not loop:
        print("FAIL: give-all loop exclusion not found in Cmd_Give_f")
        return 1

    excluded = {flag.strip() for flag in loop.group(1).split("|")}
    if "IT_TECH" not in excluded:
        print("FAIL: give all grants CTF tech items; exclusion mask is "
              + " | ".join(sorted(excluded)))
        return 1

    # IT_TECH must be defined in the header that g_cmds.c includes, or the build breaks.
    root = SRC.parent.parent.parent
    local_h = root / "src" / "game" / "header" / "local.h"
    if not re.search(r"#define\s+IT_TECH\s+", local_h.read_text(encoding="utf-8", errors="replace")):
        print("FAIL: IT_TECH is not defined in src/game/header/local.h")
        return 1
    if '#include "header/local.h"' not in text:
        print("FAIL: g_cmds.c does not include header/local.h")
        return 1

    print("OK: give all skips CTF tech items")
    return 0


if __name__ == "__main__":
    sys.exit(main())
