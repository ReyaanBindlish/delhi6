# Treeline: Task List

Plan and rationale: [plan.md](plan.md). Days are targets; the order is fixed by dependencies.

Standard verification for every code task: `just build` succeeds (UnrealBuildTool, Development Editor) and `just smoke` passes (automation smoke tests, Task 2).

---

## Phase 1: Foundation (days 1-4)

## Task 1: Lock the brief and scope (day 1)

**Description:** Write a one-page brief that fixes concept, region, run length, player verbs, systems list (core / never) and the performance budget. This is the contract every later task is judged against.

**Acceptance criteria:**
- [ ] `docs/brief.md` exists, one page, answers every open question in plan.md
- [ ] A "never" list names at least five features that will not be built
- [ ] Reference board of 100+ real photos of the chosen region in `ref/`

**Verification:**
- [ ] Manual check: brief read aloud in under three minutes; no section says "TBD"

**Dependencies:** None
**Files likely touched:** `docs/brief.md`, `ref/`
**Estimated scope:** Small

## Task 2: Project skeleton, version control and build script (day 1)

**Description:** Create the UE5 C++ project from the First Person template with the required plugins, Git LFS, and a `justfile` for building, packaging and running smoke tests.

**Acceptance criteria:**
- [ ] Plugins enabled: World Partition, Lumen, Nanite, PCG, Chaos, MetaSounds, StateTree, Mass, Water, Python Editor Script Plugin
- [ ] `.gitattributes` tracks `*.uasset` and `*.umap` with LFS; One File Per Actor is on
- [ ] `just build`, `just package`, `just smoke` and `just shots` recipes work

**Verification:**
- [ ] Build succeeds: `just build`
- [ ] Tests pass: `just smoke` runs one trivial automation test
- [ ] Manual check: Play in Editor spawns the first-person pawn

**Dependencies:** None
**Files likely touched:** `Treeline.uproject`, `justfile`, `.gitattributes`, `Source/Treeline/`, `Config/DefaultEngine.ini`
**Estimated scope:** Medium

## Task 3: Shared contracts (day 2)

**Description:** Define the interfaces every system talks through, so parallel agents never edit each other's code: a world clock subsystem, a weather state struct, and the player vitals struct.

**Acceptance criteria:**
- [ ] `UTreelineClock` (world subsystem) exposes time of day, day number, time scale and a change delegate
- [ ] `FWeatherState` (cloud cover, precipitation, wind vector, temperature °C) with a getter and broadcast delegate
- [ ] `FPlayerVitals` (core temperature, fatigue, hunger, wetness, all 0-1) with documented meaning per field

**Verification:**
- [ ] Build succeeds: `just build`
- [ ] Tests pass: unit tests advance the clock and assert day rollover and delegate firing

**Dependencies:** Task 2
**Files likely touched:** `Source/Treeline/Core/TreelineClock.h/.cpp`, `Source/Treeline/Core/WeatherState.h`, `Source/Treeline/Core/PlayerVitals.h`, `Source/Treeline/Tests/`
**Estimated scope:** Medium

## Task 4: Real terrain imported (days 2-3)

**Description:** Download elevation data for the chosen valley, crop to about 1 km² playable plus 3 km² backdrop, convert to a 16-bit heightmap, and import as a World Partition landscape through a Python script.

**Acceptance criteria:**
- [ ] `Scripts/terrain/build_heightmap.py` reproduces the heightmap from the raw DEM in one command
- [ ] Landscape imported at true scale (1 unit = 1 cm), with a lower-detail backdrop ring
- [ ] Playable boundary defined by natural barriers (cliffs, river), not invisible walls

**Verification:**
- [ ] Manual check: walk the valley floor to the ridge; elevation gain matches the real map within 10%
- [ ] Manual check: `just shots` renders the five fixed reference cameras

**Dependencies:** Tasks 1, 2
**Files likely touched:** `Scripts/terrain/`, `Content/World/Valley.umap`, `Scripts/shots.py`
**Estimated scope:** Medium

## Task 5: Look development and Steam page (days 3-4)

**Description:** Settle the visual target with World Labs Marble scenes and reference photos, then submit the Steam store page so the two-week coming-soon clock starts.

**Acceptance criteria:**
- [ ] `docs/look.md` with 6-10 target frames (dawn, noon, storm, dusk, night, snow) and a palette
- [ ] Steamworks app created, store page submitted for review with capsule art and description
- [ ] itch.io page drafted as fallback

**Verification:**
- [ ] Manual check: Steam page status is "in review" or "coming soon" by end of day 4

**Dependencies:** Task 1
**Files likely touched:** `docs/look.md`, `marketing/`
**Estimated scope:** Small

### Checkpoint A (end of day 4)
- [ ] `just build` and `just smoke` pass
- [ ] Terrain walkable; reference shots rendered
- [ ] Steam page submitted
- [ ] Developer reviews brief and look frames before world work starts

---

## Phase 2: World (days 5-11)

## Task 6: Landscape auto-material (days 5-6)

**Description:** One landscape material that picks grass, dirt, rock, scree and snow by slope, altitude and noise, with runtime virtual texturing so meshes blend into the ground, and a wetness parameter driven later by weather.

**Acceptance criteria:**
- [ ] Layers assigned automatically, with no hand painting required
- [ ] `Wetness` and `SnowCover` scalar parameters exist on a material parameter collection
- [ ] Albedo values within 30-240 sRGB on every layer

**Verification:**
- [ ] Manual check: reference shots at noon and dusk compared against `docs/look.md`
- [ ] Manual check: setting Wetness to 1 visibly darkens and glosses soil

**Dependencies:** Task 4
**Files likely touched:** `Content/World/Materials/M_Landscape`, `Content/World/MPC_Environment`
**Estimated scope:** Medium

## Task 7: Asset kit (days 5-8)

**Description:** Build the full asset kit: scan or source 8-10 rocks, 4 tree species, ground cover, deadfall, and 2 modular cabins, all imported as Nanite with physical materials. Every third-party asset is logged.

**Acceptance criteria:**
- [ ] `Scripts/import_assets.py` imports a folder with correct scale, Nanite, collision and physical material
- [ ] Physical materials: wood, rock, soil, snow, water, metal
- [ ] `docs/asset-licences.csv` lists source and licence for every external asset

**Verification:**
- [ ] Manual check: each asset in a lineup scene beside a 1.8 m reference figure; nothing off-scale

**Dependencies:** Task 2
**Files likely touched:** `Scripts/import_assets.py`, `Content/World/Assets/`, `docs/asset-licences.csv`
**Estimated scope:** Large (split by asset type across agents)

## Task 8: PCG forest, rocks and ground cover (days 7-9)

**Description:** PCG graphs that scatter trees, rocks, deadfall and ground cover by slope, altitude and distance to water and trail, in three biomes: valley forest, treeline scrub, alpine scree.

**Acceptance criteria:**
- [ ] Three biome graphs with exposed density parameters
- [ ] Vegetation clears around trail and river splines automatically
- [ ] Regenerating the whole valley takes under two minutes

**Verification:**
- [ ] Manual check: reference shots show natural clustering, no grid patterns, no floating meshes
- [ ] Perf: `stat unit` at the densest point within budget on the dev machine

**Dependencies:** Tasks 6, 7
**Files likely touched:** `Content/World/PCG/`, `Scripts/pcg/`
**Estimated scope:** Medium

## Task 9: Trail, river and cabins (days 8-10)

**Description:** Lay the broken trail and the river with splines, then hand-dress the six stop points: crash site start, river ford, hunter's cabin, collapsed bridge, high pass, ranger station.

**Acceptance criteria:**
- [ ] River uses the Water plugin with flow matching terrain slope
- [ ] Each of the six stop points is fully dressed and has one environmental story element (note, radio, gear)
- [ ] Sight line to the ranger station ridge visible from at least three stop points

**Verification:**
- [ ] Manual check: walk the full route; record time (target 25-35 min without stops)

**Dependencies:** Task 8
**Files likely touched:** `Content/World/Valley.umap` (actors), `Content/World/Story/`
**Estimated scope:** Medium

## Task 10: Sky, time of day and lighting (days 5-8)

**Description:** Sky Atmosphere, Volumetric Clouds, Exponential Height Fog and the sun and moon driven by `UTreelineClock`, using physical light units and tuned auto exposure.

**Acceptance criteria:**
- [ ] Full 24-hour cycle with correct sun path for the region's latitude and season
- [ ] Sun about 100,000 lux at noon; nights dark but readable with the headlamp
- [ ] Cloud cover and fog density read from `FWeatherState`

**Verification:**
- [ ] Manual check: time-lapse capture of 24 hours at 1 minute per second; no exposure popping
- [ ] Tests pass: clock-to-sun-angle test at four times of day

**Dependencies:** Task 3
**Files likely touched:** `Source/Treeline/World/SkyController.h/.cpp`, `Content/World/Sky/`
**Estimated scope:** Medium

## Task 11: Ambient audio and footsteps (days 8-11)

**Description:** MetaSounds ambient beds per biome and time of day, river proximity sound, wind tied to weather, and footsteps chosen by physical material and wetness.

**Acceptance criteria:**
- [ ] Ambient beds for forest, scrub, scree, by day and night
- [ ] Footsteps for soil, rock, wood, snow, water, with wet variants
- [ ] Interior reverb inside cabins; occlusion through walls

**Verification:**
- [ ] Manual check: walk forest to cabin interior to river with headphones; no hard cuts, no silence gaps

**Dependencies:** Tasks 3, 7
**Files likely touched:** `Content/Audio/`, `Source/Treeline/Audio/FootstepComponent.h/.cpp`
**Estimated scope:** Medium

### Checkpoint B (end of day 11)
- [ ] `just build`, `just smoke` pass
- [ ] The valley looks finished in all reference shots at dawn, noon and dusk
- [ ] Full route walkable; perf within budget at the three densest points
- [ ] Developer signs off the look; world changes after this are fixes only

---

## Phase 3: Systems (days 12-20)

## Task 12: Player body, movement and camera (days 12-14)

**Description:** First-person player with a visible body, walk, jog, crouch and scramble, slope-dependent speed, foot IK on uneven ground, and subtle camera motion. A headlamp is the only light the player carries.

**Acceptance criteria:**
- [ ] Speed drops on steep uphill slopes; slopes over 40° block jogging
- [ ] Legs and hands visible when looking down; feet plant on rocks via IK
- [ ] Headlamp toggles, with a battery that drains

**Verification:**
- [ ] Tests pass: slope-speed function tested at 0°, 20°, 40°
- [ ] Manual check: scramble up scree and descend; no foot sliding, no camera nausea in a 10-minute session

**Dependencies:** Task 3
**Files likely touched:** `Source/Treeline/Player/` (character, movement component, camera), `Content/Characters/Player/`
**Estimated scope:** Medium

## Task 13: Weather system (days 12-14)

**Description:** A weather director that moves `FWeatherState` through clear, overcast, rain, sleet and snow on a scripted-plus-random schedule across the three days, with the storm building towards day three.

**Acceptance criteria:**
- [ ] Transitions blend over 5-15 in-game minutes, never instantly
- [ ] Rain and snow particles, wetness and snow cover on the landscape material, wind on foliage
- [ ] Day 3 storm always arrives; days 1-2 vary with a seed

**Verification:**
- [ ] Tests pass: seeded weather schedule is deterministic; day 3 always reaches storm
- [ ] Manual check: watch a full rain-to-snow transition in game

**Dependencies:** Tasks 3, 6, 10
**Files likely touched:** `Source/Treeline/Systems/Weather/`, `Content/World/Weather/`
**Estimated scope:** Medium

## Task 14: Vitals and diegetic feedback (days 14-16)

**Description:** Cold, fatigue, hunger and wetness update from weather, shelter, activity and food. No HUD meters: feedback comes through breath fog, shivering camera, slowed movement, muffled audio and a wristwatch thermometer the player can raise.

**Acceptance criteria:**
- [ ] Core temperature falls with wind, wetness and low air temperature; rises by fire and in shelter
- [ ] Collapse at critical cold or fatigue triggers a fail state, never instant death
- [ ] Every vital has at least two non-HUD cues

**Verification:**
- [ ] Tests pass: vitals rates tested for dry/wet, sheltered/exposed, still/moving
- [ ] Manual check: a playtester can name their condition without any meter

**Dependencies:** Tasks 12, 13
**Files likely touched:** `Source/Treeline/Systems/Vitals/`, `Content/Characters/Player/Feedback/`
**Estimated scope:** Medium

## Task 15: Interaction, inventory and shelter (days 15-17)

**Description:** Look-at interaction for picking up, reading notes, opening doors and lighting fires. A small weight-limited backpack shown as an in-world open-pack view. Fires and cabins act as shelter.

**Acceptance criteria:**
- [ ] Items: food, matches, dry clothes, battery, rope, map; each with weight
- [ ] Fire needs matches and dry wood and fails in heavy rain unless sheltered
- [ ] Map is a physical paper map held in hand; no minimap

**Verification:**
- [ ] Tests pass: inventory weight limit and fire-lighting rules
- [ ] Manual check: light a fire in a cabin during rain, dry off, warm up

**Dependencies:** Tasks 12, 14
**Files likely touched:** `Source/Treeline/Systems/Interaction/`, `Source/Treeline/Systems/Inventory/`, `Content/UI/Backpack/`
**Estimated scope:** Medium

## Task 16: Wildlife (days 15-18)

**Description:** Deer herds, birds and one predator (wolf pack heard far more than seen), using StateTree for behaviour and Mass for birds. Animals react to the player's noise, headlamp and fire.

**Acceptance criteria:**
- [ ] Deer graze, flee from sprinting or light; birds scatter from sudden noise
- [ ] Wolves howl at night, follow at distance, and are driven off by fire or shouting
- [ ] No more than 40 active animals; distant ones simulated in Mass at low tick rate

**Verification:**
- [ ] Manual check: approach a herd quietly and noisily; behaviour differs
- [ ] Perf: wildlife costs under 1.5 ms CPU in the busiest area

**Dependencies:** Tasks 9, 12
**Files likely touched:** `Source/Treeline/AI/`, `Content/AI/StateTrees/`, `Content/Characters/Wildlife/`
**Estimated scope:** Large (split deer/birds and wolves across two agents)

## Task 17: Objective, route and endings (days 17-19)

**Description:** The full run: wake at the crash site, three in-game days (time-scaled to about 15-20 real minutes each), reach the ranger station. Three endings: rescued, rescued with injury, and collapse.

**Acceptance criteria:**
- [ ] Day and night structure with sleep in shelter to skip the night
- [ ] Two alternative routes (river gorge or high pass) with different risks
- [ ] Three endings with a short in-world closing scene and an end screen with stats

**Verification:**
- [ ] Manual check: complete one full run by each route; record time (target 45-60 min)

**Dependencies:** Tasks 9, 14, 15
**Files likely touched:** `Source/Treeline/Game/` (game mode, objective subsystem), `Content/World/Story/`
**Estimated scope:** Medium

## Task 18: Save, load and main menu (days 18-20)

**Description:** Autosave when sleeping and entering shelter; manual continue. A main menu rendered over a live view of the valley.

**Acceptance criteria:**
- [ ] Save restores position, vitals, inventory, clock, weather seed and story flags
- [ ] Menu: Continue, New Run, Settings (stub replaced in Task 21), Quit
- [ ] Corrupt or missing save falls back to New Run with a clear message

**Verification:**
- [ ] Tests pass: save then load round-trip equality test
- [ ] Manual check: quit mid-run, relaunch, continue

**Dependencies:** Task 17
**Files likely touched:** `Source/Treeline/Game/SaveGame.h/.cpp`, `Content/UI/MainMenu/`
**Estimated scope:** Medium

### Checkpoint C (end of day 20)
- [ ] `just build`, `just smoke`, `just package` succeed
- [ ] Two people outside the project complete a full run in the packaged build
- [ ] Perf capture at all six stop points
- [ ] **Day 21: feature freeze.** Only fixes, tuning and content polish from here.

---

## Phase 4: Polish (days 22-27)

## Task 19: Performance pass (days 22-24)

**Description:** Hit the performance budget on both target machines: HLODs for World Partition, foliage tuning, shadow and Lumen settings per scalability preset, and PSO cache collection.

**Acceptance criteria:**
- [ ] 60 fps at 1440p High on an RTX 3070-class GPU at every stop point
- [ ] 30 fps at 1080p Low on a GTX 1660 Super-class GPU
- [ ] No hitch over 50 ms during a full run (after PSO cache)

**Verification:**
- [ ] Unreal Insights capture of a full run attached to `docs/perf/`
- [ ] Manual check: run on the minimum-spec machine, not the dev machine

**Dependencies:** Checkpoint C
**Files likely touched:** `Config/DefaultScalability.ini`, `Config/DefaultEngine.ini`, `Content/World/` HLOD settings
**Estimated scope:** Medium

## Task 20: Playtest rounds and fixes (days 22-27)

**Description:** Three playtest rounds of three to five fresh players each, watched without help. Fix confusion, pacing and immersion breaks in priority order.

**Acceptance criteria:**
- [ ] Notes per round in `docs/playtests/round-N.md`
- [ ] Every "stuck for over two minutes" moment fixed or explicitly accepted
- [ ] At least 80% of round-3 players finish a run

**Verification:**
- [ ] Manual check: round 3 notes show no new blocking issues

**Dependencies:** Checkpoint C
**Files likely touched:** varies; one commit per fix
**Estimated scope:** Medium per round

## Task 21: Settings, accessibility and subtitles (days 24-26)

**Description:** A complete settings menu and the accessibility basics.

**Acceptance criteria:**
- [ ] Graphics presets plus resolution, frame cap, FOV, motion blur toggle, head-bob intensity
- [ ] Full key rebinding and gamepad support; mouse sensitivity and invert
- [ ] Subtitles and captions for radio, notes read aloud and key sounds (wolves, thunder); text size option

**Verification:**
- [ ] Manual check: complete a run on gamepad only, with motion blur and head-bob off

**Dependencies:** Task 18
**Files likely touched:** `Content/UI/Settings/`, `Source/Treeline/UI/`, `Config/DefaultInput.ini`
**Estimated scope:** Medium

### Checkpoint D (end of day 27)
- [ ] All acceptance criteria in Tasks 1-21 checked
- [ ] Packaged build passes a full run on the minimum-spec machine
- [ ] Zero known crashes; save/load verified

---

## Phase 5: Launch (days 28-30)

## Task 22: Trailer, screenshots and store assets (day 28)

**Description:** A 60-90 second trailer and the final store assets, all captured in-engine from the release build.

**Acceptance criteria:**
- [ ] Trailer recorded with the Sequencer or high-res gameplay capture, edited in DaVinci Resolve (free)
- [ ] Eight or more screenshots, capsule images in every Steam size
- [ ] Store description final, with honest system requirements from Task 19

**Verification:**
- [ ] Manual check: trailer watched by two people unfamiliar with the game; both can say what it is about

**Dependencies:** Checkpoint D
**Files likely touched:** `marketing/`
**Estimated scope:** Small

## Task 23: Release build and launch (days 29-30)

**Description:** Final shipping build with PSO cache, uploaded through SteamPipe (and to itch.io via butler), smoke-tested from the store download, then released.

**Acceptance criteria:**
- [ ] Shipping configuration, PSO cache bundled, no debug output
- [ ] Downloaded store build completes a full run on a clean machine
- [ ] Launch announcement posted; crash reporting route documented in `docs/support.md`

**Verification:**
- [ ] Build succeeds: `just package` (Shipping)
- [ ] Manual check: install from Steam on a second PC and finish a run

**Dependencies:** Task 22
**Files likely touched:** `justfile`, `Config/`, `docs/support.md`
**Estimated scope:** Small

### Checkpoint E: Shipped
- [ ] Game live on Steam (or itch.io if Steam review slipped)
- [ ] Day-one patch window kept free on day 30
