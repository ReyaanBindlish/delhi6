# Implementation Plan: Thirty-Day Realistic Game (working title "Treeline")

Author: Ritesh Roshan

## Overview

A first-person survival-exploration game in Unreal Engine 5, built and shipped in 30 days by one developer directing parallel Claude agents. The player is a hiker stranded in a remote mountain valley in late autumn after a storm wipes out the trail. They have three in-game days to reach a ranger station on the far ridge, managing cold, fatigue and daylight while weather turns. One 1 km² valley, 45-60 minutes per run, with replay value from weather and route choice. Release: Steam (with itch.io as fallback).

The concept is the default for this plan and can be swapped in Task 1. Every later task depends only on "one valley, one main loop, few humans", so another quiet concept fits the same plan.

## Architecture Decisions

- **Unreal Engine 5 (latest 5.x), C++ core with Blueprint glue.** Agents write C++ and Python as text, which they edit reliably; Blueprints only wire assets together.
- **Editor automation in Python.** Asset import, landscape setup, PCG parameters and builds run as scripts in `Scripts/`, so agents can change the world without hand-editing binary `.uasset` files.
- **Agent ownership by folder.** Each parallel agent owns one folder (`Source/Treeline/Systems/Weather/`, `Scripts/pcg/` …). Shared contracts (the `UTreelineClock` time/weather interface and `FPlayerVitals` struct) are written first, in Task 3, before parallel work starts.
- **No humans on screen.** Avoids the hardest realism problem (characters and faces). Life comes from wildlife, weather, sound and traces of people (cabins, notes, radio).
- **Dynamic everything.** Lumen GI, Sky Atmosphere and Volumetric Clouds, driven by one clock, so time of day and weather are core gameplay rather than decoration.
- **Real terrain.** Copernicus 30 m DEM (or national lidar) for a real alpine valley, upsampled and detailed in-engine.
- **Version control:** Git + Git LFS, one commit per task, with a tag at each checkpoint.
- **Performance budget (hard):** 60 fps at 1440p on an RTX 3070 with High preset; 30 fps at 1080p on a GTX 1660 Super with Low preset.

## Schedule

| Days | Phase | Outcome |
|---|---|---|
| 1-4 | Foundation | Project, contracts, terrain, look locked, Steam page submitted |
| 5-11 | World | The valley is beautiful and walkable at final quality |
| 12-20 | Systems | Full run playable start to finish |
| 21 | Feature freeze | No new features after this day |
| 22-27 | Polish | Performance, playtests, menus, audio mix |
| 28-30 | Launch | Trailer, build, release |

## Task List

Full task cards with acceptance criteria live in [todo.md](todo.md).

### Phase 1: Foundation (days 1-4)
- [ ] Task 1: Lock the brief and scope
- [ ] Task 2: Project skeleton, version control and build script
- [ ] Task 3: Shared contracts (clock, weather, vitals)
- [ ] Task 4: Real terrain imported
- [ ] Task 5: Look development and Steam page

### Checkpoint A (end of day 4)

### Phase 2: World (days 5-11)
- [ ] Task 6: Landscape auto-material
- [ ] Task 7: Asset kit (scans and library)
- [ ] Task 8: PCG forest, rocks and ground cover
- [ ] Task 9: Trail, river and cabins (hand-dressed spots)
- [ ] Task 10: Sky, time of day and lighting
- [ ] Task 11: Ambient audio and footsteps

### Checkpoint B (end of day 11)

### Phase 3: Systems (days 12-20)
- [ ] Task 12: Player body, movement and camera
- [ ] Task 13: Weather system
- [ ] Task 14: Vitals (cold, fatigue, hunger) and diegetic feedback
- [ ] Task 15: Interaction, inventory and shelter
- [ ] Task 16: Wildlife
- [ ] Task 17: Objective, route and endings
- [ ] Task 18: Save, load and main menu

### Checkpoint C (end of day 20) and feature freeze (day 21)

### Phase 4: Polish (days 22-27)
- [ ] Task 19: Performance pass
- [ ] Task 20: Playtest rounds and fixes
- [ ] Task 21: Settings, accessibility and subtitles

### Checkpoint D (end of day 27)

### Phase 5: Launch (days 28-30)
- [ ] Task 22: Trailer, screenshots and store assets
- [ ] Task 23: Release build and launch

### Checkpoint E: Shipped

## Parallel Agent Lanes

The developer is art director and reviewer; agents implement. At most four agents at once, each on one folder.

| Days | Lane A (C++ systems) | Lane B (editor Python / world) | Lane C (audio / UI) | Lane D (tooling / QA) |
|---|---|---|---|---|
| 1-4 | Task 3 | Task 4 | — | Task 2 |
| 5-11 | Task 10 | Tasks 6, 8, 9 | Task 11 | Task 7 import scripts |
| 12-20 | Tasks 12, 13, 14 | Tasks 16, 17 | Task 15 UI, Task 18 | Automated smoke tests, perf captures |
| 22-27 | Task 19 fixes | Task 20 world fixes | Task 21 | Playtest bug triage |

Sequential, never parallel: Task 3 before any system; Task 4 before Tasks 6-9; Task 13 before the weather parts of Tasks 11 and 14.

## Risks and Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Scope creep | High | Feature freeze on day 21 is absolute. New ideas go to `tasks/later.md`. |
| Steam's two-week "coming soon" rule and review | High | Steam page submitted in Task 5 (day 4); itch.io release is the fallback if Steam slips. |
| Agents cannot see the viewport | High | Developer reviews screenshots daily; scripts render fixed camera shots (`Scripts/shots.py`) so changes can be compared. |
| Binary asset merge conflicts | Med | One File Per Actor; agents change the world through scripts, not by hand-editing maps. |
| Performance misses budget late | High | Perf capture every checkpoint, not only in Task 19. |
| Shader compile stutter on launch | Med | PSO cache gathered during playtests (Task 20), bundled in Task 23. |
| Asset licence problems | Med | Every third-party asset logged in `docs/asset-licences.csv` at import. |
| Run feels empty | Med | Wildlife, weather shifts and placed story traces every ~150 m of route. |

## Open Questions

- Keep the "stranded hiker, three days, reach the ranger station" concept, or swap it in Task 1?
- Which real region for the terrain (default: a Norwegian or Canadian Rockies alpine valley)?
- Price point (default: $9.99) and whether to ship a free demo.
