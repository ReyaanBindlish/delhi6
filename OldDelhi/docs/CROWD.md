# Ambient crowd (5000 NPCs per location, about 600 visible)

Owner: crowd agent. Code: `Source/OldDelhi/Crowd/*`. Tools: `Tools/ue/setup_crowd.py`, `Tools/ue/build_zonegraph.py`.
Data: `Data/DT_Archetypes_Karachi.csv`, `Data/DT_Archetypes_DelhiNCR.csv`, `Data/DT_StreetDensity_Story.csv`
(the Old Delhi `DT_Archetypes.csv` and `DT_StreetDensity.csv` are reused as they are).
Research basis: `research/05` (population numbers, LOD tiers, governor), `research/07` (Mass + ZoneGraph), `research/08` (locations).

## 1. What it does

A pool of up to 5000 agents exists per location. Only the ones near the player are alive; the rest are "dormant" and
exist as the abstract population. `UDhCrowdSubsystem` asks the existing `UDelhiPopulationGovernor` how many people each
200 m street cell should hold right now (time of day from `UDelhiWeatherSubsystem`, rain from wetness, event multiplier,
the player's crowd setting), then activates or retires agents until the cells match, never popping anyone in view.

| Tier | Count (cap) | Radius | Representation |
|---|---|---|---|
| Dormant | rest of the 5000 | n/a | a struct in an array (about 200 bytes each) |
| Abstract | up to 2500 active | within 320 m | lane position only, ticked at 1 Hz in slices |
| Far instanced | rest of the visible cap | 60-150 m | instanced proxy mesh (or Mass entity) |
| Near instanced | 150 | 25-60 m | instanced proxy mesh with custom data tint and gait bob (or Mass entity) |
| Skeletal | 40 | 8-25 m | pooled `ADhCrowdCharacter`, 30 Hz pose, no RVO |
| Hero | 12 | under 8 m | pooled `ADhCrowdCharacter`, full rate, RVO avoidance |

Visible cap is 600 (`VisibleCap`), 750 while an event is active (`EventVisibleCap`, set by `SetEventActive`). Crowd density
setting is 0.5 / 0.75 / 1.0 (`SetCrowdDensity`, `dh.crowd.density`); it multiplies the governor density, so every number
above scales down with it. Tier hysteresis (25-30 percent) stops agents flickering between tiers.

Agents do not teleport between representations: the virtual agent walks the lane; a pooled actor, an instance or a Mass
entity is just the body that follows it. Promotion and demotion are therefore invisible.

## 2. Files

| File | Role |
|---|---|
| `DhCrowdTypes.h` | enums (tier, activity, group, backend), `FDhCrowdAgent`, `FDhCrowdArchetypeInfo`, `FDhCrowdStats` |
| `DhCrowdSettings.*` | Project Settings > Game > Dhurandhar Crowd (`UDeveloperSettings`, saved in DefaultGame.ini) |
| `DhCrowdLaneNetwork.*` | lane graph from JSON, 200 m cell index, random-walk successor, `FDhCrowdSim::Step` (shared movement, avoidance, panic) |
| `DhCrowdSubsystem.*` | the director: governor sync, per-cell targets, spawn and retire, tiers, ground traces, console commands |
| `DhCrowdCharacter.*` | pooled near-tier character, Manny/Quinn stand-in, single-node walk or idle animation, per-agent tint |
| `DhCrowdInstanceRenderer.*` | two instanced static mesh components with fixed slots and per-instance custom data |
| `DhCrowdMass.*` | Mass fragment, tag, `UDhCrowdAgentTrait`, `UDhCrowdMovementProcessor` |
| `DhCrowdMassBridge.*` | non-UObject adapter that spawns, reads, and destroys Mass entities for far-tier agents |

## 3. Setup, in order

1. Integrator applies the module dependencies, plugins and ini lines from the "Integration" section below, then compiles.
2. Lane files (already generated for all 14 locations in `Content/Crowd/Lanes/`; regenerate any time):
   `C:\Users\Rayaan\Desktop\game\characters\.venv\Scripts\python.exe Tools\ue\build_zonegraph.py lanes --location nizamuddin`
   (`--all` does every location with an OSM file; `--origin LAT LON` overrides the tile centre).
   Lanes are in UE world cm with the origin at the tile centre (same frame as `osm_to_blender.py` and `build_vertical_slice.py`).
   If a level places a tile elsewhere, set `LaneWorldOffsetCm` in the settings or call `SetLaneOffset` before `SetLocation`.
3. Editor assets: `UnrealEditor-Cmd.exe OldDelhi.uproject -run=pythonscript -script=Tools\ue\setup_crowd.py -unattended -nop4`
   creates `/Game/Crowd/Data` tables, copies Manny and Quinn into `/Game/Crowd/Mannequin`, makes the tint materials,
   a person-shaped proxy mesh (Blender), and the Mass entity config. Every step reports instead of failing.
4. Play. The subsystem exists in game and PIE worlds only. Console:

| Command | Effect |
|---|---|
| `dh.crowd.stats` / `bShowDebug` | tier counts, visible vs cap, backend |
| `dh.crowd.density 0.5` | Low / Medium / High = 0.5 / 0.75 / 1.0 |
| `dh.crowd.location nizamuddin` | switch tables and lanes (karachi_lyari, karachi_orangi, karachi_korangi_checkpoint, karachi_ibrahim_hyderi, nizamuddin, sarai_kale_khan, vasundhara, khoda_colony, loni_flyover, loni_town, sahibabad, dasna_jail, mamdot, dehradun_farm) |
| `dh.crowd.hour 18.5` | override time of day for tests (`-1` follows the weather subsystem) |
| `dh.crowd.warm` | fill the neighbourhood immediately (loading, teleports) |
| `dh.crowd.pause 1` | freeze the crowd |
| `dh.crowd.disturb` | everyone within 60 m runs (what a gunshot does; also `ReportDisturbance` from code) |

Blueprint and C++ API on `UDhCrowdSubsystem`: `SetCrowdDensity`, `SetLocation`, `SetEventActive`, `ReportDisturbance`,
`SetFocusActor`, `SetLaneOffset`, `WarmStart`, `GetStats`, `SetHoursOverride`, `SetPaused`.

## 4. Archetypes and mix

`DT_Archetypes_Karachi.csv` (K01-K38, Lyari, Gulbahar Lines, Orangi, Korangi, Ibrahim Hyderi, 2023-24) and
`DT_Archetypes_DelhiNCR.csv` (D01-D38, Nizamuddin, Ghaziabad, Loni, Sahibabad, Dasna, Dehradun, Mamdot) use the exact
column format of `FDelhiArchetypeRow` (same as `DT_Archetypes.csv`), so `UDelhiPopulationGovernor` tooling reads them.
Languages use `Ur=60;Sd=20;Bl=15;En=5` style codes: Ur Urdu, Sd Sindhi, Bl Balochi, Ps Pashto, Pa Punjabi, Sr Saraiki, Gu Gujarati and
Memoni, Hi Hindi, Bh Bhojpuri, Hr Haryanvi, Bn Bengali, En English, Ar Arabic.

Who appears where: weight = `group weight for the street type` x `time-of-day curve for the group` x `3 when the archetype's
Places and Label mention the street's purpose (shrine, harbour, station, checkpoint, market, jail, highway, school)`.
Tables are in `DhCrowdSubsystem.cpp` (`GroupStreetWeight`, `TimeFactor`); they replace the Old Delhi schedule CSV for the
story locations. Street types are the rows of `DT_StreetDensity_Story.csv`: the five Old Delhi types plus SectorStreet,
LyariGali, ShrineForecourt, HarbourFront, CheckpointApproach, HighwayVerge, JailGate. `build_zonegraph.py` assigns them from
OSM (highway class plus nearby mosques, shrines, stations, checkpoints, prisons and shop density). OSM misses some things
(jetties, Rangers posts), so add hand-placed points to `Tools/ue/crowd_poi_overrides.json`:
`{"karachi_ibrahim_hyderi": [{"kind": "harbour", "lat": 24.79, "lon": 67.14}]}` (kinds: jail, check, police, harbour,
station, shrine, mosque, shop), then rerun the lanes step.

Respectful depiction (research/05 section 3.4-3.6 applies): religious sites never host combat (their forecourts are
density-only zones), brands are fictional, no group is a caricature. The loafers archetypes (K32, D30) are a small share and
described as people. Rangers, police and guards wear plausible but non-branded uniforms (tint only until real outfits exist).

## 5. Backends

**Pooled (default, no extra setup beyond the assets).** Near tiers use pooled `ADhCrowdCharacter` actors; far tiers use
`ADhCrowdInstanceRenderer`. This is also the automatic fallback.

**Mass (opt-in).** Set Backend to `Mass` (or `Auto`) in the settings after `DA_DhCrowdEntityConfig` exists. Far-tier agents
then become Mass entities: `UDhCrowdAgentTrait` adds the fragments, `UDhCrowdMovementProcessor` runs the same
`FDhCrowdSim::Step` in the Behavior group, and the Crowd Visualization trait from MassCrowd draws them (set its static mesh
to `SM_CrowdProxy`, or a VAT mesh). If the config does not resolve, the Mass subsystems are missing, or the template is
invalid, the log says so once and the subsystem continues on the Pooled path. Manual recipe if `setup_crowd.py` cannot
build the config: Content Browser > Miscellaneous > Data Asset > `MassEntityConfigAsset`, add traits "Dhurandhar Crowd
Agent" and "Crowd Visualization" (and "Crowd Member"), save as `/Game/Crowd/Mass/DA_DhCrowdEntityConfig`.

**ZoneGraph (optional navigation for Mass).** `build_zonegraph.py` run inside the editor (set `DH_LOCATION`, optional
`DH_WORLD_OFFSET`) places one `ZoneShape` per OSM way and a `ZoneGraphData` actor. Then: add a lane profile named
`DhPedestrian` in Project Settings > Plugins > Zone Graph (two lanes of 150 cm, one forward, one backward), set it as the
shapes' lane profile, and build the graph from the ZoneGraph toolbar. To let MassCrowd's ZoneGraph navigation steer the
entities, add the MassCrowd navigation traits to the config and untick `bDriveMovement` on the Dhurandhar trait (it adds
`FDhCrowdExternalMovementTag`, which makes our lane processor skip them). The lane JSON remains the source for placement,
density and the pooled actors either way.

## 6. Stand-ins and the MetaHuman Crowd upgrade path

Now: UE Manny and Quinn (copied by `setup_crowd.py` from `/Game/Characters/Mannequins` or the engine's MoverExamples
content), one material instance per slot tinted by archetype (khaki police, desert Rangers, blue or orange sanitation,
school white or navy, security grey, otherwise a muted dye palette with jitter, off-white favoured for worshippers), skin
variation as a scalar, scale by age and gender. `BodyTint` and `SkinTone` are the parameter names (settings).

Upgrade to MetaHuman Crowd (research/05 section 4, `implementation.md` 5.8 experimental path):

1. Build the 96 crowd heads and outfit kit in MetaHuman Creator; produce crowd skeletal meshes per body type.
2. Replace the entity visualization: swap "Crowd Visualization" for the MetaHuman Mass crowd visualization trait
   (`MetaHumanMassCrowdVisualizationTrait` in the MetaHumanCrowd plugin) in `DA_DhCrowdEntityConfig`; keep our trait.
   Per-instance tint then becomes outfit selection: read `FDhCrowdAgentFragment::ArchetypeIndex`, `Tint` and `Skin`
   in a small processor that picks a MetaHuman variant from the archetype's outfit columns (Summer, Monsoon, Winter).
3. Pooled near tiers: add `FDhCrowdVisualSet` entries pointing at the MetaHuman meshes and an animation Blueprint instead of
   the single-node loops (`ADhCrowdCharacter::PlayLoop` is the only place that plays animations), then raise
   hero count budgets as profiling allows.
4. Anti-clone rule from research/05: no identical head, outfit and colour within 25 m at the instanced tiers; the
   `Seed`-less design here picks random per agent, so add a duplicate check in `InitAgent` when the real kit exists.

VAT upgrade for the instanced tiers: bake walk and idle to a vertex animation texture, make a material that reads the
four custom data floats already written (tint RGB, skin) plus a start offset, and point `InstancedMesh` and
`InstancedMaterial` in the settings at it. No code change; `ADhCrowdInstanceRenderer::SetSlot` bob is the placeholder.

## 7. Integration (for the integrator)

Module dependencies for `OldDelhi.Build.cs`: `DeveloperSettings`, `Json`, `MassEntity`, `MassCore`, `MassCommon`,
`MassSpawner`, `MassMovement`, `MassSimulation`. Plugins in `OldDelhi.uproject`: `MassGameplay`, `MassCrowd`, `MassAI`
(ZoneGraph, PythonScriptPlugin, EditorScriptingUtilities already enabled); optional `MoverExamples` for the mannequin copy.

`Config/DefaultGame.ini`:

```
[/Script/OldDelhi.DhCrowdSettings]
bEnableCrowd=True
DefaultLocation=karachi_lyari
CrowdDensity=1.0
Backend=Pooled

[/Script/UnrealEd.ProjectPackagingSettings]
+DirectoriesToAlwaysStageAsNonUFS=(Path="Crowd/Lanes")
+DirectoriesToAlwaysStageAsNonUFS=(Path="../Data")
```

(The second staging line ships the CSV fallbacks; unnecessary once `setup_crowd.py` has made the DataTable assets and the
settings point at them.)

Physics note: crowd characters use the Pawn collision profile with `bRunPhysicsWithNoController`, so the player can bump
them. Capsule radius 34 cm. If the player agent adds a custom collision channel for NPCs, set it on `ADhCrowdCharacter`.

## 8. Test checklist

1. Lane files exist: `Content/Crowd/Lanes/*.json`. Console `dh.crowd.stats` in PIE shows lanes above 0.
2. `dh.crowd.hour 6` then `dh.crowd.hour 18` at a main road: density rises toward the evening peak (governor numbers
   `DT_StreetDensity_Story.csv`), composition shifts (workers, students, commuters).
3. Walk toward a crowd: people sidestep and slow when head-on; hero count never above 12, skeletal above 40, visible
   above 600 (750 with `SetEventActive`).
4. `dh.crowd.density 0.5` halves the visible number within about 10 s.
5. `dh.crowd.disturb`: everyone near flees along lanes, then returns to normal.
6. Break Mass on purpose (delete the config): log shows the fallback line and the crowd continues pooled.

## 9. Known limits and next steps

* The C++ was written against the 5.8 headers on this machine but could not be built here (an editor with Live Coding
  was open). Expect a compile pass; the Mass files are the likeliest place for a small API fix, and a backend switch in
  settings (`Pooled`) keeps the rest independent of them at runtime.
* Lane Z is flat (0) and agents are snapped by line traces; elevated roads (Loni flyover) will place people on the first
  surface hit under the player's Z band. Add a layer mask for flyover decks when the level exists.
* No pedestrian crossing logic, no vehicle yielding, no smart objects. Vendors stand at the building line; they do not yet
  open and close stalls by schedule beyond the time curve.
* The archetype Schedule and BodyLanguage columns are documentation for animators and dialogue; the runtime uses group,
  age, gender share, Places keywords and Label keywords.
