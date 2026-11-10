# 06 - Games, simulators and engines Claude can drive (API / scripting / MCP)

Researched 2026-11-10 for delhi6 (UE 5.8.4, Windows 11, RTX 3070 8 GB, 32 GB RAM, Python 3.11, Node 26). Nothing was installed or downloaded. Facts come from web search results and doc pages fetched during research. Items marked UNVERIFIED rest on a single secondary source or on prior knowledge and need a check against the official page before relying on them.

## 0. Bottom line

1. **CARLA 0.9.16 (UE4.26 build)** is the best hands-on target. It has a full Python API for weather, time of day, traffic, pedestrians and sensors, and it runs on 8 GB VRAM. The UE5 builds (0.10.0 and the newer "1.0" docs) want 16 GB VRAM and will not run well on a 3070.
2. **Epic's City Sample for UE 5.8 plus Epic's experimental Unreal MCP.** It is the only inspectable shipped-quality city with crowds and traffic in the same engine version as delhi6. Epic rebuilt it procedurally in August 2026 and used Claude to drive it.
3. **BeamNG.tech with BeamNGpy and the community BeamNG MCP server.** Best soft-body vehicle physics available through a scripting API. Licence is non-commercial research only, so use it to observe and learn, not to ship its data.
4. Everything else is useful but narrower (see the table in section 2).

Constraint to keep in mind: the 3070 has 8 GB VRAM. CARLA UE5, City Sample at full settings, and BeamNG.tech with sensors can each need most of it. Run only one heavy simulator at a time and never alongside the UE editor.

## 1. Key legal point for delhi6

Observing behaviour and learning techniques is fine. Do not extract or reuse shipped assets, maps, AI data tables or code from closed products (GTA V, BeamNG, MSFS, CS2) in delhi6. CARLA (MIT code, CC-BY assets, some third-party asset licences), City Sample (Epic content licence, usable in UE projects) and the open MCP servers are the only items here whose content can legitimately be borrowed, and each licence still needs reading for the specific asset.

## 2. Summary table

| Product | Control / observe | How Claude connects | Cost / licence | Windows, 3070 8 GB | Fit for delhi6 |
|---|---|---|---|---|---|
| CARLA 0.9.16 (UE4.26) | Weather, sun, fog, wetness, traffic manager, walkers, vehicles, sensors (RGB, depth, semantic, lidar), screenshots, recorder/replay | Python package `carla` (pip), RPC on port 2000; community CarlaMCP | Open source, free | Yes, 6 GB min, 8 GB recommended | High |
| CARLA 0.10.0 / "1.0" (UE5.5) | Same API family, Lumen, Nanite, Chaos vehicles, ROS2 | Python wheel from the release package (not PyPI) | Free | Docs say 16 GB VRAM, RTX 3000+, driver 560+, 225 GB disk | Too heavy for 3070 |
| Epic City Sample 5.8 | Full UE project: Mass crowds, traffic, PCG city, MetaHumans | Unreal MCP (Epic, experimental) or any UE MCP server; Python in editor | Free on Fab / Launcher | Heavy, needs scalability tuning | High (same engine) |
| BeamNG.tech 0.38/0.39 | Vehicle physics params, traffic AI, weather/time, sensors, scenarios, Lua | `pip install beamngpy`, TCP; MIT MCP server with 57 tools (community) | Free for research on request; commercial needs licensing@beamng.gmbh | Yes (Windows) | Medium (physics reference) |
| BeamNG.drive (retail) | Mods, Lua; no official Python API | Modding MCP (community) | Steam price not verified | Yes | Low |
| MSFS 2024 | SimConnect SimVars, events, AI traffic; photogrammetry cities | SimConnect SDK (C/C++/.NET), Python wrappers target MSFS 2020; no weather-injection API | Game licence plus free SDK | Yes, heavy | Low, reference only |
| Cities: Skylines II | Official mod tools via Paradox Mods, C# code mods, ECS | In-game mod SDK; community Cities2-MCP exists | Game licence | Yes | Medium (citizen/traffic sim ideas) |
| Cities: Skylines I | Large C# modding ecosystem | Mod DLLs | Game licence | Yes | Low-medium |
| GTA V | ScriptHookV (native scripting), FiveM (Lua/C#/JS), RageMP | In-process scripts; no official agent API | See section 3.6 | Yes | Observation only |
| Roblox Studio | Full DataModel, Luau execution, playtest, screenshots, player input | Official built-in MCP server (stdio) | Free | Yes | Low (not UE) but good MCP design reference |
| Unity MCP | Editor automation | CoplayDev/unity-mcp (community); Unity AI Assistant MCP reported pre-release | Free | n/a | None directly |
| Godot MCP | Editor automation | Coding-Solo/godot-mcp, godot-ai (April 2026) | Free | n/a | None directly |
| Unreal MCP | Editor automation | Epic official (5.8, experimental, opt-in); community: ChiR24/Unreal_mcp, db-lyon/ue-mcp | Free | n/a | Core tool, covered by another report |
| Minecraft | Full bot control, world, chat | Mineflayer MCP (yuniko-software, hibukki); Java server 1.21.x | Apache-2.0 servers; game licence | Yes | Low (agent-behaviour testing) |
| ETS2 / ATS | Telemetry read-out (speed, rpm, position, damage, game time, weather) | SCS telemetry plugin (RenCloud scs-sdk-plugin) writes shared memory | Free plugin, game licence | Yes | Low (traffic/road-feel observation) |
| Assetto Corsa / rF2 | Physics telemetry via shared memory | AC shared memory map; TheIronWolfModding rF2SharedMemoryMapPlugin | Plugin free | Yes | Low-medium (tyre/vehicle model reference) |
| BALROG and similar agent benchmarks | Text/2D game envs (NetHack, Crafter, BabyAI, TextWorld) | Python framework | Open | Yes | None for the city, useful for agent eval design |

## 3. Per-product detail

### 3.1 CARLA

- **Versions.** 0.10.0 (released 2024-12-19) moved from UE 4.26 to UE 5.5, with Lumen, Nanite, Chaos vehicle physics, a new Town 10 and a native ROS 2 interface. The release post says GPUs under 12 GB VRAM may fail to load the default map and recommends 16 GB. A UE5 documentation site now documents "CARLA 1.0" (Windows 11 or Ubuntu 22.04/24.04, RTX 3000 series or better with 16 GB VRAM, driver 560+ on Windows, 225 GB free disk, ports 2000-2002, VC++ 2022 redistributable, Python 3.10-3.15 wheels from `CARLA_ROOT/PythonAPI/carla/dist/`; `pip install carla` does not work for it). A GitHub tag "1.0" exists (note on the tag: UE5 dev Docker image fix); I could not read its date or asset sizes. I found no independent release announcement for 1.0, so treat 1.0 status as UNVERIFIED.
- **Pick 0.9.16 / 0.9.x for this machine.** Unversioned quick-start docs state 6 GB GPU minimum, 8 GB recommended, about 20 GB disk for the package (UNVERIFIED for exactly 0.9.16; a mirror lists 0.9.16 as a release). Python client is `pip install carla` for 0.9.x. 0.9.16 binding Python versions should be checked on the release page (the community CarlaMCP pins `carla==0.9.15`).
- **What you can control** (from the CARLA Python API; the exact field list below is from prior knowledge of `carla.WeatherParameters` and should be confirmed on the docs page): cloudiness, precipitation, precipitation_deposits, wind_intensity, sun_azimuth_angle, sun_altitude_angle, fog_density, fog_distance, fog_falloff, wetness, scattering_intensity, mie_scattering_scale, rayleigh_scattering_scale, dust_storm. Plus synchronous mode and fixed time step, Traffic Manager (speed vs limit, lane changes, following distance, ignoring lights), `walker` and `WalkerAIController` (pedestrians with navigation mesh), RGB/depth/semantic/instance/lidar/radar cameras, recorder and replay, OpenDRIVE import, `-nullrhi` headless.
- **Community MCP.** `codebymov/CARLA-MCP` (MIT, 7 tools: get_status, list_maps, run_scenario, set_weather, validate_scenario, save_scenario, replay_scenario; needs CARLA 0.9.15 binding). Small, so Claude could also just write Python scripts directly, which is simpler and more flexible.
- **What delhi6 learns.** (a) How a shipped sim parameterises weather as one struct, which maps onto our `MPC_Weather` design. (b) Traffic Manager and pedestrian controller parameters to copy into our own Mass/StateTree design. (c) Performance cost per pedestrian and vehicle on a 3070. (d) Sensor and synchronous-mode patterns for automated capture and regression tests of our own scenes.
- **Licence.** Code MIT; assets mostly CC-BY with some exceptions. Read the CARLA licence page before reusing any asset.
- Sources: https://carla.org/2024/12/19/release-0.10.0 , https://carla-ue5.readthedocs.io/en/latest/start_quickstart/ , https://carla.readthedocs.io/start_quickstart/ , https://carla.readthedocs.io/en/latest/build_windows/ , https://github.com/carla-simulator/carla/releases/tag/1.0 , https://glama.ai/mcp/servers/codebymov/CARLA-MCP

### 3.2 AirSim / Colosseum / Project AirSim

- AirSim (Microsoft, UE4) was archived. Colosseum is the community successor; one README says UE 5.03 or later, a fork says only UE 5.4 and 5.5 on main. Project AirSim (iamaisim) is Microsoft's successor for aircraft, with docs that list UE 5.2 setup.
- Controls: Python API for drone and car, cameras and screenshots, weather API (rain, snow, fog, dust, wind), time of day, lidar, segmentation. Free, MIT-style licences.
- Windows works but needs a UE source/plugin build (large disk, long build). Colosseum compiles against UE 5.5 at most, so it cannot be dropped into UE 5.8.4 without porting.
- For delhi6: low priority. Its weather and segmentation APIs are a subset of what CARLA offers, and the engine-version mismatch makes it a distraction. Useful only for aerial camera capture ideas.
- Sources: https://iamaisim.github.io/ProjectAirSim , https://www.github.com/nearthlab/Colosseum , https://GitHub.com/CodexLabsLLC/Colosseum

### 3.3 BeamNG.drive / BeamNG.tech

- BeamNG.tech is the research build with the BeamNGpy Python API (`pip install beamngpy`, versions must match: tech 0.39 pairs with beamngpy 1.36, 0.38 with 1.35.1). Controls: scenarios, vehicles and their JBeam/parts, traffic and AI, weather and time of day, sensors (camera, lidar, radar, ultrasonic, IMU, electrics, damage), Lua execution, mesh/soft-body data, road and map queries.
- Licence: free for non-commercial research and education, requested at register.beamng.tech (email with download link and key; place `tech.key` in a workspace dir). Companies and commercial uses must contact licensing@beamng.gmbh. Their Database Licensing Agreement defines "as a service" access as commercial. delhi6 is a commercial game, so do not claim research use as cover for borrowing data. Use it to observe soft-body behaviour only, and confirm with BeamNG if unsure.
- Community MIT MCP server: 57 tools across simulator, scenario, traffic, environment, vehicle, sensor, map, Lua, mod, soft-body authoring, job and autonomous-driving domains; targets 0.38 on Windows. A separate BeamNG modding MCP exists for the retail game.
- For delhi6: crash deformation, tyre/suspension tuning values, traffic AI behaviours, and how soft-body results compare with our Chaos rickshaw (three-wheel, raised centre of mass) plan.
- Sources: https://documentation.beamng.com/beamng_tech/install/ , https://documentation.beamng.com/api/beamngpy/master/readme.html , https://register.beamng.tech , https://www.beamng.tech/policies/database-licensing/ , https://pypi.org/project/beamngpy/

### 3.4 Microsoft Flight Simulator 2024

- SimConnect SDK (docs.flightsimulator.com/msfs2024) is for out-of-process add-ons in C, C++ or .NET. Python access is through community wrappers (`SimConnect`, `simcon`, `pysimconnect`), which are documented for MSFS 2020 and not confirmed on 2024.
- Observe and set SimVars, events, AI traffic, and read weather-related variables. Asobo has said it does not plan an API to inject weather data or alter weather rendering (forum answer, older than MSFS 2024, UNVERIFIED as current). Weather presets via SimConnect were not available in the thread I found.
- Only a documentation-search MCP for the MSFS SDK was found (unofficial). No SimConnect controller MCP was found.
- For delhi6: lets you study photogrammetry city streaming, cloud/haze look at low altitude, and Delhi at altitude (the real city is in MSFS with Bing photogrammetry in some areas, not verified here). No data reuse. Low priority; heavy install (100+ GB, not verified).
- Sources: https://docs.flightsimulator.com/msfs2024/retail/programming-apis/simconnect/simconnect-sdk/index.md , https://devsupport.flightsimulator.com/t/simconnect-weather-preset-api/5216 , https://devsupport.flightsimulator.com/t/weather-sdk-for-3rd-party-developers/5118 , https://pypi.org/project/SimConnect

### 3.5 Cities: Skylines II / I

- CS2 got official modding tools on Paradox Mods in 2024 (map editor plus code tools; mods from CS1 do not work). Developer is now Iceflake Studios and a 2026 blog re-focused on modding tools. Official API documentation completeness is UNVERIFIED. Community code mods are C# against the game's ECS (DOTS) simulation, so you can read citizen, traffic path-finding and service data. A community "Cities2-MCP" exists (listing only; maturity UNVERIFIED).
- CS1 has a mature C# mod API (ICities) and thousands of mods; easier to script but older.
- For delhi6: study of agent-based citizen and traffic aggregation at scale (LOD of simulation), which informs how much of Old Delhi's population should be simulated versus faked. Cannot be used as an in-engine reference for rendering.
- Sources: https://pcgamer.com/games/city-builder/cities-skylines-2-will-finally-start-getting-modding-support-alongside-some-performance-fixes-and-its-first-wave-of-paid-dlc , https://gamesradar.com/cities-skylines-2-finally-debuts-first-wave-of-mods-in-beta-next-week-with-coding-and-map-editing-tools-the-first-to-launch , https://www.pcgameshardware.de/Cities-Skylines-2-Spiel-74219/News/Update-Modding-Tools-1551021/

### 3.6 GTA V and GTA VI

- ScriptHookV (Alexander Blade) loads native-calling scripts (C++/.NET ScriptHookVDotNet, Lua variants). FiveM is Cfx.re's multiplayer client with Lua, C# and JS server and client scripting. RageMP and alt:V are rivals; alt:V began a structured shutdown in 2026 at Take-Two's request (secondary reporting).
- Rockstar's stance (support article first published 2017, reportedly revised January 2025): generally tolerant of reasonable non-commercial single-player mods, revocable at any time; extended to roleplay servers in November 2022; Rockstar acquired Cfx.re in 2023. A 2026 consolidation of the policy reportedly exists but is reported by one outlet only (UNVERIFIED). Read Rockstar's current support page before doing anything. Never use mods online in GTA Online (ban risk). Take-Two has a history of takedowns (Liberty City / Re3 / Revolution cases, not researched here).
- For research use: only offline single-player, non-commercial, no redistribution of anything extracted. Treat as a place to observe traffic density, pedestrian reactions and wanted-system behaviour, not to copy.
- GTA VI: console date reported as 2026-11-19; PC reported for Q1 2027 by industry sources and not officially confirmed. No official modding or developer API.
- No MCP server for GTA V was found in my searches. Claude would write ScriptHookVDotNet C# or Lua. Not recommended given legal exposure and Delhi-specific irrelevance.
- Sources: https://www.pcgamer.com/games/grand-theft-auto/rockstar-celebrates-official-gta-mod-support-by-nuking-non-official-mod-platform , https://www.club386.com/rockstar-acquires-gta-v-and-rdr-roleplaying-mod-maker-cfx-re/ , https://gtaintel.com/news/rockstar-mod-guidelines-explained-gta-6 , https://gamesbeat.com/?p=336622 , https://www.generationamiga.com/2026/09/22/rockstars-new-gta-modding-rules-could-change-everything/

### 3.7 Engine MCP servers (list only)

- **Roblox Studio (official, built in).** Enable under Assistant > Manage MCP Servers. Quick connect for Claude Code, Claude Desktop, Cursor, Codex CLI, Gemini CLI and VS Code; stdio, local only. Tools: read/edit scripts, search, data-model explore, run Luau in Edit/Client/Server, start/stop playtest, console output, screenshots, player movement and keyboard/mouse input, asset and mesh generation, docs fetch, subagents. Older standalone Rust server is deprecated. https://create.roblox.com/docs/studio/mcp
- **Unity.** CoplayDev/unity-mcp is the widely used community server; Unity's own AI Assistant MCP is reportedly pre-release (UNVERIFIED). https://mcpservers.org/hi/servers/github-com-coplaydev-unity-mcp
- **Godot.** Coding-Solo/godot-mcp and godot-ai (launched April 2026) for Claude Code and Cursor. https://mcp.directory/blog/godot-vs-unity-vs-blender-mcp-skills-2026
- **Unreal.** Epic shipped an experimental, opt-in first-party MCP in UE 5.8 (Editor Preferences > General > Model Context Protocol; reported default port 8000, path `/mcp`, HTTP + SSE; settings are from one secondary source, UNVERIFIED). Community: ChiR24/Unreal_mcp, db-lyon/ue-mcp. https://www.pcgamer.com/software/unreal-engine-5-8-launches-with-improved-terrain-and-vegetation-tools-a-lumen-lite-option-for-faster-global-illumination-and-for-the-times-we-now-live-in-an-open-standard-plugin-for-llm-systems/ , https://dev.to/gamedevtoollab/using-unreal-mcp-in-ue-58-from-codex-setup-toolsets-and-safe-editor-automation-4pon
- Rankings and tool counts from third-party blogs conflict, so check each GitHub repo for recent commits before adopting.

### 3.8 Minecraft

- Mineflayer-based MCP servers: `yuniko-software/minecraft-mcp-server` (Apache-2.0, Node 20.10+, needs a running Java server on e.g. localhost:25565, Claude Desktop config via npx, tested around 1.21.4 to 1.21.8; another listing says 1.21.11). Claude moves, mines, builds and chats as a bot.
- For delhi6: no city realism. Useful only as a cheap sandbox to prototype agent loops (perception, action, memory) for NPC testing. Low priority.
- Sources: https://glama.ai/mcp/servers/@yuniko-software/minecraft-mcp-server , https://lobehub.com/en/mcp/hibukki-minecraft-mcp-server

### 3.9 ETS2 / ATS telemetry, Assetto Corsa, rFactor 2

- ETS2/ATS: SCS provides a telemetry SDK; the common plugin is RenCloud/scs-sdk-plugin which exposes values via shared memory; Rust wrapper `scs-sdk-telemetry` and `simetry` exist. Read-only: speed, rpm, gear, position, damage, game time, fuel; scene control is not offered (UNVERIFIED for weather read-out).
- Assetto Corsa: writes physics, graphics and static blocks to Windows shared memory read by any process; AC Evo covered by Rust crate `kerb`. rFactor 2 / Le Mans Ultimate: TheIronWolfModding `rF2SharedMemoryMapPlugin`.
- Connect: Python `mmap`/ctypes over the named shared-memory blocks, or Rust crate `simetry`. Free; no ToS issue for reading telemetry on your own PC.
- For delhi6: compare tyre slip, suspension and load-transfer outputs of established vehicle models against our ChaosVehicles rickshaw setup. Low-medium priority; Claude cannot place cameras or change the world.
- Sources: https://github.com/SeriousOldMan/rF2SharedMemoryMapPlugin , https://docs.rs/crate/simetry/latest , https://docs.rs/crate/scs-sdk-telemetry/latest , https://habr.com/en/articles/973024

### 3.10 Game-agent benchmarks

- BALROG (ICLR 2025) bundles BabyAI, Crafter, TextWorld, Baba Is AI, MiniHack and NetHack with a Python client; code and leaderboard are public. Best model in the first paper reached about 1.5 percent NetHack progress. https://arxiv.org/abs/2411.13543v2
- No official shipped-game LLM API with 3D city content was found. The nearest items are the engine MCPs above and Epic's City Sample demonstration with Unreal MCP.
- For delhi6: borrow the idea of a text/state observation API plus action API for automated playtests; design our own game with a small "agent API" from day one.

### 3.11 Epic City Sample (Matrix Awakens) for UE 5.8

- Updated August 27-28, 2026 for UE 5.8: two new PCG-built levels (a full city and a simplified one), PCG graph examples that double as references for the Unreal MCP server. Epic says it used Claude and that other models work. Free via Fab, Epic Games Launcher or the Editor home panel. Contains vehicles and MetaHuman crowds.
- Download size for the 5.8 version was not found. The original 2021 project was reported as 93 GB (content) and an 18 GB compiled demo; do not assume the same. Check the Fab page before downloading.
- Docs: https://dev.epicgames.com/documentation/unreal-engine/city-sample-pcg-and-mcp-server-interaction-in-unreal-engine and https://dev.epicgames.com/documentation/unreal-engine/city-sample-pcg-for-unreal-engine . The Epic blog page returned 403 to my fetch; details come from search snippets: https://www.unrealengine.com/learning/city-sample-gets-a-major-update-with-pcg-and-unreal-mcp-workflows , https://automaton-media.com/articles/newsjp/20260828-463849/
- For delhi6: this is the closest working template for the whole pipeline: PCG graphs generating a city from rules and data, Mass-based crowds and traffic, LOD and HLOD on a large World Partition map, and an MCP workflow that Claude already drives. The Mass AI crowd and traffic setup (ZoneGraph lanes, Mass processors, StateTree) is worth reading even though Delhi's chaotic mixed traffic will need new behaviours (lane-less flow, honking, jaywalking). Verify which Mass/ZoneGraph assets the 5.8 version still ships (not confirmed in my sources).

### 3.12 Indian-city / South Asian projects

- No open-source Delhi, Mumbai or other Indian city driving game or sim on GitHub turned up. Found only closed or hosted items: Jabali AI "Delhi Drift: Open Streets" and "Mumbai Street Flow" (hosted web games), itch.io "Indian Trucking Simulator" (small Unity project, licence unstated), a SourceForge "indian cars the game", and an IIT Jodhpur research project on a driving simulator for Indian traffic (ongoing). None is a reuse candidate; licences are unverified and quality is unknown.
- Better data than games: OSM, Overture, Copernicus DEM already in the delhi6 repo; academic Indian heterogeneous-traffic datasets (not searched here) could feed rickshaw/two-wheeler flow tuning. Search "heterogeneous traffic India dataset" as a follow-up.
- Sources: https://www.jabali.ai/game/1cb1d7f7-978b-4c1d-a4dc-321ecf804f5f/3d-open-world/delhi-drift-open-streets/ , https://derpy153.itch.io/indian-trucking-simulator , https://scholar.iitj.ac.in/entities/project/0b7be7b5-d42d-414e-9ddd-c864553f15e8

## 4. Top 3 to install and experiment with

### Pick 1: CARLA 0.9.16 (UE4.26 build)

Why: only free sim with a complete Python world API, and it runs on the 3070.

Setup (PowerShell; do these yourself after confirming the current release page and file name):
```
# 1. Download the Windows package CARLA_0.9.16.zip (and optional AdditionalMaps) from
#    https://github.com/carla-simulator/carla/releases  (about 20 GB installed; confirm size on page)
Expand-Archive CARLA_0.9.16.zip -DestinationPath D:\sims\carla
# 2. Install VC++ 2022 x64 redistributable if missing
# 3. Isolated Python env (use 3.11 or the version listed on the release page)
py -3.11 -m venv D:\sims\carla-env
D:\sims\carla-env\Scripts\activate
python -m pip install --upgrade pip
pip install carla==0.9.16 numpy pygame opencv-python
# 4. Run at low quality first (8 GB VRAM)
D:\sims\carla\CarlaUE4.exe -quality-level=Low -ResX=1280 -ResY=720 -carla-rpc-port=2000
# 5. Smoke test
cd D:\sims\carla\PythonAPI\examples; python generate_traffic.py -n 50 -w 50
```
If `pip install carla==0.9.16` fails, install the wheel in `PythonAPI\carla\dist\`.

Experiments:
1. **Delhi smog look.** Script `world.set_weather(carla.WeatherParameters(...))` sweeping fog_density 20-80, fog_distance 0-30, scattering/Mie scale up, sun_altitude_angle 5-25 deg (low winter sun), cloudiness 20, wetness 0. Capture an RGB sensor frame per setting and save a table of parameter to image with mean luminance and horizon contrast. Output: concrete starting values for delhi6's `SmogDensity` and sun presets, plus how CARLA couples fog to sky colour.
2. **Crowd stress test.** Spawn 300 walkers with `WalkerAIController` and 80 TM-controlled vehicles in Town10 in synchronous mode at fixed 0.05 s step. Log server tick time, GPU frame time and VRAM (via `nvidia-smi`) at 50/100/200/300 walkers, with and without sensors. Output: cost per agent curve to size delhi6's Mass crowd budget on the 3070.
3. **Export AI parameters.** Dump Traffic Manager settings (global distance to leading vehicle, speed percentage difference, lane-change probabilities, red-light running percentage, hybrid physics radius), walker settings (speed, `set_max_speed`, crossing percentage via `set_pedestrians_cross_factor`), and recorded runs (`client.start_recorder`) to JSON/CSV. Then set the red-light-running and lane-change percentages high to imitate Delhi traffic and record what breaks. Output: a parameter sheet to seed delhi6's vehicle and pedestrian behaviour tables.

### Pick 2: Epic City Sample (UE 5.8) with Epic's Unreal MCP

Why: same engine version as delhi6, shipped procedural city, Claude-driven workflow.

Setup:
```
# Epic Games Launcher > Unreal Engine > 5.8.x (match 5.8.4 if offered) > Library
# Fab: search "City Sample", Add to Library, Create Project (check the displayed size first)
# In editor: Edit > Plugins: enable Unreal MCP plus its Editor Toolset (experimental, opt-in)
# Edit > Editor Preferences > General > Model Context Protocol: start server (reported port 8000, path /mcp)
claude mcp add --transport http unreal http://localhost:8000/mcp
claude mcp list
```
Verify plugin names and port in the 5.8 docs page above, since sources disagree. Run on a spare drive and drop scalability to Medium or Low on the 3070. Do not mix this with the delhi6 project; keep a separate copy.

Experiments:
1. **Read the city recipe.** Ask Claude (via MCP) to list the PCG graphs, their inputs (splines, density, biome/lot rules) and parameters, and summarise how roads, lots and building kits become a city. Map each stage to the delhi6 Old Delhi GeoJSON/OSM pipeline and write down what we can replace with real footprints.
2. **Profile the crowd and traffic.** Use Unreal Insights and `stat unit`, `stat Mass` style commands (verify names) at several crowd densities; ask Claude to change density through MCP and record frame time, tick cost per Mass processor and VRAM. Output: realistic Mass agent counts for a 3070 at 1440p with TSR.
3. **Weather and lighting regression rig.** Have Claude write an editor Python or MCP routine that sets time of day and a fog/height-fog preset, takes high-res screenshots from five fixed cameras, and saves them with their settings. This rig is directly reusable on delhi6 for the four weather states.

### Pick 3: BeamNG.tech with BeamNGpy (and the community MCP)

Why: richest vehicle physics and AI-traffic observable through a Python API on Windows.

Setup:
```
# 1. Request a free research licence at https://register.beamng.tech (state non-commercial research purpose)
# 2. Extract the BeamNG.tech zip to D:\sims\beamng, place tech.key in the workspace dir as per
#    https://documentation.beamng.com/beamng_tech/install/
py -3.11 -m venv D:\sims\beamng-env
D:\sims\beamng-env\Scripts\activate
pip install beamngpy==1.36    # match BeamNG.tech 0.39; use 1.35.1 for 0.38
# 3. Optional community MCP server (MIT): clone the repo found via the glama listing, follow its README
```
Licence reminder: research and education only. If the licence is granted for non-commercial research, do not feed outputs straight into delhi6 content; use results to guide our own parameters and keep notes. Ask licensing@beamng.gmbh if in doubt.

Experiments:
1. **Rickshaw proxy.** Load a light narrow vehicle (small kei-type car or trike mod), set mass and centre of mass height through the vehicle/JBeam parameters, and run lateral-g tip-over tests over a ramp of speeds on a skidpad. Record the speed at which wheels lift. Output: sanity-check numbers for the ChaosVehicles three-wheeler (350 kg, raised COM).
2. **Surface friction and wetness.** Drive the same braking test on dry asphalt, wet asphalt, mud and gravel; log stopping distance and slip from the electrics sensor. Compare with the delhi6 friction table (asphalt 0.75, mud 0.45) and note how wetness should scale friction.
3. **Dense traffic AI.** Enable the AI traffic with 30-60 vehicles on a dense map, vary aggression and lane-following parameters via the API, record near-miss and collision counts, and measure CPU and GPU use. Output: behaviour parameter ranges and cost data for Delhi's high-contact traffic.

## 5. Runners-up and when to use them

- ETS2 telemetry or Assetto Corsa shared memory if only vehicle dynamics comparison is needed (tiny Python script, no licence request).
- Roblox Studio MCP only as a reference for how a polished MCP exposes playtesting, screenshots and input (our Unreal MCP workflow could copy that tool design).
- Cities: Skylines II mods if you decide to model citizen-level economy or district traffic aggregation.
- Skip: GTA V (legal exposure, no API for agents), MSFS (no weather API, heavy), AirSim/Colosseum (engine version mismatch), Minecraft (not relevant), Indian-city games (no reusable open source found).

## 6. Open items to verify before installing

- CARLA latest stable tag and whether 0.9.16 Windows package and size match this report; confirm whether "CARLA 1.0" is an official release.
- Exact size and requirements of City Sample 5.8 on Fab.
- Epic's official Unreal MCP plugin names, port and transport (the Epic doc pages were not fetched directly).
- BeamNG.tech current version pairing and licence form wording.
- Rockstar's current mod policy text on support.rockstargames.com.
- Search tool results were US-only and sometimes spammy or from mirrors; star counts and tool counts are not stable.
