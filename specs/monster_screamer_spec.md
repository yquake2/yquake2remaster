# Functional Specification: Monster Screamer (`monster_screamer`)

## 1. Overview
The Screamer (`monster_screamer`) is an agile, multi-modal enemy from the Infinity mod (`infinity/src/infinity/im_scrmr.c`). It supports walking, wall crawling, swimming, jumping attacks, and sonic scream attacks.

---

## 2. Entity Definition & Properties
- **Classname:** `monster_screamer`
- **Source File:** `infinity/src/infinity/im_scrmr.c`
- **Hit Damage:** `HITDAMAGE` = 12
- **Source Offsets:** `scream_source` = `{16, 0, 12.5}`, `scream_source2` = `{0, 0, 12.5}`
- **Animation Ranges:**
  - Stand (`standa` to `standc`), Idle (`idlea`, `idleb`)
  - Run (`run`, `runattack`), Jump (`jump`, `jumpattack`)
  - Scream (`scream`), Wall Crawl (`wallcrawl`, `wallidle`)
  - Attack (`attacka`, `attackb`), Swim (`swim`, `swimb`, `swimattack`)
  - Pain & Death (`paina`, `painb`, `deatha`, `deathb`, `deathc`)

---

## 3. Combat & AI Behaviors
- **Locomotion Modes:** Can transition between standard ground movement, wall crawling (`im_screamer_wallcrawl`, `im_screamer_wallnext`), swimming (`im_screamer_swim`), and leaping/jumping (`im_screamer_jump`, `im_screamer_prejump`).
- **Sonic Attack:** Unleashes close-range or ranged sonic scream attacks (`im_screamer_attack`, `im_screamer_melee`).
