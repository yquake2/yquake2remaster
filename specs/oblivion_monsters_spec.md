# Functional Specification: Oblivion Monsters

## 1. Overview
This specification covers Oblivion monster types:
- `monster_spider` (`m_spider.cpp`)
- `monster_badass` (`m_badass.cpp`)
- `monster_cyborg` (`m_cyborg.cpp`)
- `monster_kigrax` (`m_kigrax.cpp`)
- `monster_soldier_deatom` (`m_soldier.cpp` / deatomizer variant)

---

## 2. Implementation & Behavior
- **AI & Combat:** Each monster implements dedicated spawn functions, sight/idle sounds, attack routines (melee, projectile, energy beam), pain reactions, and death states.
- **Disguise & Activation:** Supports spawnflags such as disguise/pop animations responding to trigger activation.
- **Damage types:** Fully integrated with Oblivion damage mods and monster health pools.
