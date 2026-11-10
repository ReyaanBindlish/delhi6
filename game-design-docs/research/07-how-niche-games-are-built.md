# 07 - How the state-of-the-art games in our niche were built

Research date: 10 Nov 2026. Purpose: calibrate delhi6 (tiny team, UE 5.8.4, RTX 3070, Old Delhi Phase 1 per `implementation.md`) against how dense real-city open worlds were actually made.

**Confidence tags.** [V] = fetched or returned by search in this session (URL given). [R] = recalled from background knowledge (talks, Epic launch materials), not re-verified now; treat as leads. [U] = unverified or fan-sourced. Several primary pages (Epic blog 403, many GDC Vault talks paywalled) could not be read in full. Where a number is not publicly known, this says so rather than guessing.

---

## 1. Per-game findings

### 1.1 The Matrix Awakens / City Sample (Epic, UE5) - closest reference stack

| Aspect | Finding |
|---|---|
| Team / time | Built by Epic (with partners) as a tech demo, released Dec 2021. Team size and dev time not published in anything I could fetch. The point is that it was a demo, not a shipped game: no quests, no deep systems, a fixed visual target. |
| Engine | UE5 early-access/5.0 builds. Public as free "City Sample" on Fab. [V] https://dev.epicgames.com/documentation/unreal-engine/city-sample-project-unreal-engine-demonstration |
| City geometry | **Fully procedural in SideFX Houdini** (v18.5.532, Houdini Engine 3.5.2), exported as point clouds, then instanced in UE by the **Rule Processor**. Stages in dependency order: city layout, roads, freeway, lots and sidewalks, then traffic/buildings/ground, then street furniture, decals, audio. [V] same Epic doc. |
| Buildings | Volumes styled by a **shape grammar**; each building is "hundreds of instances" of modular meshes. Custom meshes only for collisions, freeway deck, road decals, non-rectangular roofs. A base style and separate top style per building. [V] |
| Scale numbers | From Epic launch materials [R]: roughly 4 x 4 km (~16 km2), ~260 km of road, ~7,000 buildings, ~45,000 parked cars, ~17,000 driven cars, ~35,000 pedestrians (most as simulated Mass entities, not full characters). The Epic doc itself confirms "hundreds of crowd characters visible in Drone mode" and "thousands of unique MetaHumans" but gives no counts. [V for doc, R for figures] |
| Crowd/traffic | **MassEntity** drives both. Spawners (crowd, intersections, traffic, parked vehicles) use Mass Entity Definition assets; spawn points are distributed along **ZoneGraph** lanes from Houdini data. Crowd behaviour runs on **StateTree**; traffic does not use StateTree. Near crowd = fully rigged MetaHumans, far = **vertex-animated static meshes**. [V] |
| Physics | **Chaos Vehicles** (engine, transmission, mass, CoM on vehicle movement component; data-only wheel BPs). Deformation via one shared Control Rig + constraint plasticity. Drivable cars carry a `_Sandbox` suffix, i.e. only a subset are physics-simulated. [V] |
| Streaming | **World Partition** with Data Layers (procedural objects, rooftop props, freeway), HLOD generation, One File Per Actor, level instancing. Big City starts with zero cells loaded in the editor. [V Epic doc + 80.lv] https://80.lv/articles/breakdown-creating-the-matrix-awakens-in-houdini-unreal-engine-5 |
| Rendering | Nanite on all static meshes, no traditional LODs, "billions of polygons across tens of thousands of objects"; Lumen HWRT uses fallback meshes (Fallback Triangle Percent). TSR from 1080p halved frame time vs native 4K (57.5 ms to 33.4 ms in Epic's test). [V] |
| Hardware | Recommended: 12-core 3.4 GHz, **64 GB RAM, RTX 2080 / 8 GB VRAM**, SSD. [V] Note this is the editor/demo, running on a 3070 at 60 fps needs cuts (see section 3). |
| Pipeline tools | Houdini (layout/roads/lots/buildings/furniture), Mass/ZoneGraph/StateTree, MetaHumans, Megascans-style kits, Chaos Vehicles. Houdini source shipped as `CitySample_HoudiniFiles.zip`; the City Building Generator needs a SideFX licence. [V] |
| Caveat | A SideFX forum user says only the building generator is meant to run via Houdini Engine inside UE; the rest are offline-export HDAs. [V, user opinion] https://www.sidefx.com/community/unreal-engine-5/ |

Other sources: quick-start guide https://dev.epicgames.com/documentation/unreal-engine/city-sample-quick-start-for-generating-a-city-and-freeway-in-unreal-engine-5 ; SideFX talk https://www.sidefx.com/ja/learn/talks/the-matrix-awakens-generating-a-world/ ; Epic announcement https://www.unrealengine.com/blog/introducing-the-matrix-awakens-an-unreal-engine-5-experience (403 to fetch).

**Key lesson for delhi6:** the "city" is a *rules + point cloud* problem. Hand-made assets are a small kit of modular pieces; the city is output of a generator, with the generator feeding both rendering (instances) and AI (ZoneGraph lanes). delhi6 already has the equivalent (OSM footprints + `lyari_build.py`), but it emits one baked FBX instead of a kit + point cloud.

### 1.2 GTA V and GTA VI (Rockstar, RAGE)

| Aspect | Finding |
|---|---|
| Team / time | Rockstar North core ~360 people, with other Rockstar studios, **1,000+ total** [V]. Full development ~3 years after preliminary work from 2008; analyst estimates up to 5 years of effort. Dev budget est. ~US$137 million; the oft-quoted $265M is development + marketing [V]. https://en.wikipedia.org/wiki/Development_of_Grand_Theft_Auto_V , https://mcvuk.com/over-1000-people-made-gta-v/ |
| Engine | RAGE, heavily overhauled for draw distance; Euphoria (procedural ragdoll/animation) and Bullet for physics/animation tasks [V Wikipedia]. TechRadar interview with Rockstar on "the new RAGE" https://techradar.com/news/gaming/the-tech-that-built-an-empire-how-rockstar-created-the-world-of-gta-5-1181281 |
| City geometry | Hand-built by large art teams from heavy reference (Rockstar sent teams to LA to photograph; Los Santos is "inspired by", not a map copy) [R]. Not procedural. |
| Crowd/traffic | Ambient population as streamed "population zones" with density budgets by area/time-of-day; peds/vehicles spawned out of view and despawned; scripted set-pieces separate from ambient (a developer quote says chase traffic was hand-scripted, secondhand [U]). No public hard counts; community mods show density is config-driven [U]. |
| Physics | Euphoria for hit reactions/falls; rigid-body vehicles with deformation [R]. |
| Streaming | Aggressive disc streaming of LODs and textures; famous for needing a cleaned-up asset hierarchy [R]. PC stutter lineage from RAGE noted by Digital Foundry [V, below]. |
| GTA VI | Officially confirmed: Vice City/Leonida, Trailer 2 (May 2025), launch trailer Oct 2026. Digital Foundry: trailer ~1440p upscaled to 4K, likely RT-based GI, expects 30 fps on consoles; Rockstar's Rob Nelson said 30 fps console target. Crowd tech: all detailed claims are fan sites or one ex-IO programmer's opinion [U]. https://www.pushsquare.com/news/2026/08/tech-experts-call-gta-6-the-most-impressive-example-of-real-time-rendering-weve-ever-seen , https://gtaintel.com/news/gta-6-launch-trailer-digital-foundry-analysis , https://gtaintel.com/news/gta-6-npc-density-and-crowd-tech |

**Lesson:** GTA-class density comes from 1,000+ people and a decade of engine work. What is copyable is the *architecture*: ambient population is a budgeted, out-of-sight spawner, separate from scripted moments.

### 1.3 Red Dead Redemption 2

~1,600 people (sources conflict, up to ~2,000), ~8 years including preliminary work, RAGE [V, low-quality sources]; Rockstar reported crunch (100-hour weeks) per AV Club https://www.avclub.com/rockstar-games-reveals-details-of-red-dead-redempt . Budget and ecosystem internals: **no reliable public figures found**. Recalled [R]: wildlife uses per-species behaviour trees with predator/prey and weather-linked spawn, volumetric clouds, and snow/mud deformation. Take-away: ecosystem and weather are hand-authored, rule-based, and tuned over years; do not expect a shortcut.

### 1.4 Assassin's Creed Unity (Paris, 2014) and Mirage (Baghdad, 2023) - Ubisoft Anvil

| Aspect | Unity | Mirage |
|---|---|---|
| Team | Multi-studio, hundreds (Montreal lead); a dedicated "crowd realisation team" existed for the first time [V TechRadar https://www.techradar.com/news/gaming/inside-assassin-s-creed-unity-ubisoft-s-leap-of-faith-1268435/2 ]. 800-person figure: **not confirmed** [U]. | Ubisoft Bordeaux lead, pulled staff from across Ubisoft; size not found [V]. |
| Crowd | Earlier AC ~200 on-screen NPCs; Unity 5,000+ (up to 10,000 in the king-beheading scene) [V]. GDC "Massive Crowd on AC Unity: AI Recycling": **AI level-of-detail with a pool that swaps characters between low/high-res versions; simplified AI "brain" for far agents; cap of 40 full AI agents and 120 high-res models while ~10,000 crowd NPCs appear** [V] https://gdcvault.com/play/1022141/Massive-Crowd-on-Assassin-s | Aims to restore Unity-like density with a more natural blend/recognition system [V] https://gamerant.com/assassins-creed-mirage-influences-gameplay-setting-interview/ , https://wccftech.com/assassins-creed-mirage-richer-denser-map-than-classic-games-stealth-blending-updated/ |
| City geometry | Hand-built by large teams; Notre-Dame alone ~5,000 man-hours (WSJ via Gameranx) [V] https://gameranx.com/updates/id/23650/article/assassin-s-creed-unity-creating-notre-dame-in-the-game-took-5-000-man-hours/ . Anvil toolset, hand-placed kits and rooftop/parkour paths. | Deliberately smaller ("richer, denser") than Valhalla, i.e. a scope cut to raise density per metre [V wccftech]. |
| Performance | Unity launched with severe frame-rate problems and bugs; crowds were a CPU cost. [R] | |

**Lesson:** the useful trick is **crowd LOD of the brain, not just the mesh**: few full agents, thousands of cheap ones, swap by distance. Mirage shows *shrinking the map to raise density* is a valid scope decision; directly relevant to Old Delhi.

### 1.5 Cyberpunk 2077 (REDengine 4)

~500 people (Wikipedia, [V search]); ~8 years; budget not reliably public. GDC "Building Night City: The Technology of Cyberpunk 2077": **prefab-based hierarchy to assemble the world, streaming approaches for different systems, CPU/memory management, REDengine 4 redesigned for scale** [V] https://gdcvault.com/play/1028981/Building-Night-City-The-Technology . Developers say they had to build a road system, traffic and crowd management and a hierarchical prefab pipeline from scratch [V search]. Crowds are placed "where it makes sense" and given flee/react rules rather than uniformly filling the city [V search]. Vertical city is hand-built; launch was a famous performance/bug failure on last-gen consoles [R]. CDPR is moving to UE5 for the sequel (job post targets "most realistic reactive crowd system") [V search] https://insider-gaming.com/cyberpunk-2077-sequel-is-working-on-the-most-realistic-and-reactive-crowd-system

**Lesson:** crowd density should be *authored per zone* (target density map), not global; and a prefab hierarchy was the key world-assembly tool.

### 1.6 Marvel's Spider-Man 2 (Insomniac)

Insomniac GDC 2024 talks: "10 Years of Procedural Content Creation" (over a dozen procedural systems to create and populate the open world), "Procedural Tool Development", and "Applied Mesh Analysis: Automating Distant City LODs" (NYC ~2x the size of earlier games; moved from manual distant-LOD to automated generation). Houdini is the procedural tool [V search]. https://schedule.gdconf.com/session/marvels-spider-man-2-10-years-of-procedural-content-creation/900288 , https://schedule.gdconf.com/session/applied-mesh-analysis-automating-distant-city-lods-in-marvels-spider-man-2/899260 , https://gamedeveloper.com/programming/video-procedurally-creating-manhattan-for-i-marvel-s-spider-man-i- . Not a 1:1 city copy: it "captures the essence" and uses procedural generation so artists can focus on art and atmosphere [V]. I found **no source** that it uses raw GIS/footprint data [U]. Ambient chatter was recorded from groups walking in circles [V gamesradar]. Lesson: procedural for fabric and *distant LOD automation*, hand-art budget on landmarks.

### 1.7 Watch Dogs: Legion (London)

Ubisoft Toronto lead. "Play as anyone": every NPC procedurally generated with a profile (traits, schedule, relationships); a GDC AI Summit talk covers missions that adapt to any played character [V] https://www.gdcvault.com/play/1027145/AI-Summit-Missions-Played-As , https://gamedeveloper.com/design/how-watch-dogs-legion-s-play-as-anyone-simulation-works . Team size not found. Review consensus: the premise is wide but shallow in play [V]. Lesson: a procedural-citizen simulation costs enormous content/narrative pipeline effort; delhi6 should only do cheap procedural *appearance and schedule*, not "everyone is a character".

### 1.8 Microsoft Flight Simulator (2020/2024, Asobo)

Bing Maps imagery + photogrammetry for selected cities, Azure-based synthesis of buildings elsewhere; procedural 3D buildings placed at real locations but "largely random" in look [V search] https://axios.com/microsoft-flight-simulators-lofty-aims-ff12e71a-af53-43cc-989e-5e223b1b1b91.html , https://t3n.de/news/microsoft-flight-simulator-gamer-bing-maps-google-earth-1329153/ . Team size not found. Lesson: real-data procedural buildings are accurate in *footprint* and wrong in *face*, which is exactly the risk in implementation.md section 10. Streamed cloud data is not shippable for us (see licence note below).

### 1.9 Hitman 3 (IO Interactive, Glacier)

Glacier crowd tech supports **up to ~300 active NPCs in a location** (Dubai opening); an earlier IO GDC talk covered "1,200 character crowds" for Glacier 2 [V] https://mcvuk.com/?p=99805 , https://gamespress.com/IO-Interactives-latest-HITMAN-3-trailer-introduces-new-features-and-re . Hitman crowds are interactive (every agent can notice/react) so counts are lower than Unity's. **Calibration for delhi6:** your ~600 visible agents is already 2x Hitman 3's interactive number.

### 1.10 A Plague Tale (Asobo) - swarms, our pigeons

>5,000 rats visible plus ~5,000 simulated behind the player; **four levels of detail** (full anim near, cheaper, fading into shadow, farthest a non-animated background mesh sharing animation loops); movement/reaction calculations merged across rats; multi-threaded [V] https://blog.playstation.com/archive/2019/05/09/how-a-plague-tale-innocences-horrifying-rat-swarm-was-created . Requiem scales to hundreds of thousands on new-gen [V] https://www.gamesradar.com/a-plague-tale-requiem-utilizes-new-gen-tech-for-hundreds-of-thousands-of-rats/ . Lesson: swarm = LOD ladder + shared animation loops + light shadow tricks; your ~2,000 pigeon GPU Niagara plan is well inside this precedent.

### 1.11 Indian / South Asian context

| Project | Facts | Takeaway |
|---|---|---|
| **Mumbai Gullies** (GameEon / Bombay Play, Pune) | Viral UE trailer Jan 2021; UE4 to UE5 (2022) to Unity (outsourced, 2024) back to Unreal; peak team **55**; ~$1.0M external + ~$244K self-funded; net loss ~$1.07M; **development halted May 2026**; no public build shipped, ~16K Steam wishlists. Causes per the founder-aligned article: open-world action-adventure far too large for budget, mid-project engine migration, raises, delayed funding with no milestone payments, four engine transitions, director pay ~43% of final-year salaries. [V] https://respawn.outlookindia.com/gaming/gaming-originals/mumbai-gullies-the-rise-and-fall-of-indias-most-hyped-game | The most relevant cautionary tale: real South Asian city, UE, small studio, ambition beyond team. Pin the engine; ship a slice. |
| **Raji: An Ancient Epic** (Nodding Heads, Pune) | UE4, team grew from ~3 to ~13; Epic MegaGrant/Dev Grant; sequel Raji: Kaliyuga on UE5 [V] https://www.rajithegame.com/post/making-it-in-unreal-action-adventure-goes-to-india-in-raji-an-ancient-epic , https://en.wikipedia.org/wiki/Raji:_An_Ancient_Epic | Not a city game (mythological, stylised, linear) but proves a 6-13 person Indian team can ship on UE with an authored art direction and grant funding. |
| **Indus** (SuperGaming, Pune) | Made-in-India battle royale, "Virlok" island, not a real-city sim; engine/team size not in results [V] https://earlygame.in/news/best-indian-games/in-conversation-with-roby-john-ceo-of-supergaming-developer-of-the-made-in-india-battle-royale-game-indus | No street-building pipeline published. |
| Delhi-set or published Indian "street-building" pipelines | **None found.** No Indian studio publication on building Indian streets/crowds/traffic at realistic density turned up. | delhi6 would be among the first public technical references; document your own pipeline. |

Also searched and not found: a "Kerala-set" open-world with published tech.

---

## 2. Cross-cutting patterns

1. **Real-city fabric is generated; landmarks are hand-made.** Matrix Awakens (full Houdini), Spider-Man 2 (a dozen+ procedural systems, automated distant LOD), MSFS (random facades on real footprints). Hand-built cities (GTA V, RDR2, Unity, Cyberpunk) needed 500-1,600 people.
2. **Crowd = one data source feeds many consumers.** In City Sample, Houdini lanes drive ZoneGraph, spawners and audio.
3. **LOD the AI, not only the mesh.** Unity (40 full agents / 10,000 crowd), City Sample (MetaHuman near, vertex-animated far), Plague Tale (4 LOD tiers).
4. **Density is authored by zone** (Cyberpunk, Hitman, GTA ambient budgets), not uniform.
5. **Ambient vs scripted separation** (GTA).
6. **Engine stability beats engine novelty** (Mumbai Gullies; Cyberpunk's rewrite pain; Unity 2014).
7. **Shrink the map to raise density** (Mirage).
8. **Performance targets drive art decisions from day one**; Cyberpunk and Unity launches suffered for failing this.

## 3. What delhi6 should copy - ranked (RTX 3070, tiny team)

Mapping to `implementation.md` section numbers / section 9 steps.

| # | Technique | Source | Maps to | How to adapt |
|---|---|---|---|---|
| 1 | **Generator emits a kit + point cloud, not baked meshes.** Output per-building lot data (footprint, floors, style id, facade seed) as CSV/JSON; spawn modular instances in UE. | City Sample (Rule Processor) | Step 3, 4, 6; sec. 3.1 | Keep Python/Blender generator but add a point-cloud export; UE import script instantiates Nanite modules (ISM/HISM), with per-building FBX only for hero/collision. |
| 2 | **Shape-grammar facades: base style + top style, hundreds of modular pieces** (jharokha, chhajja, shutter, flex-board slots). | City Sample buildings | Step 4, sec. 7 | Build ~30-50 modules; style ids per bazaar (Chawri, Kinari, Dariba). |
| 3 | **Same generated data feeds AI lanes**: export street centrelines as ZoneGraph shapes. | City Sample (Houdini to ZoneGraph) | Step 5, sec. 4.5 | OSM roads/footpaths to lane spline actors for crowd and rickshaw traffic. |
| 4 | **Two-tier crowd: ~40 full agents, rest vertex-animated instances** (AI LOD with swap pool). | AC Unity GDC; City Sample | sec. 4.5 | Matches your plan; add the pool/swap and a simplified far "brain"; cap full agents hard. |
| 5 | **Author crowd density by zone and time of day** (density map per gali/bazaar; shutters-up at 10-11). | Cyberpunk, Hitman, GTA | sec. 4.5, 4.3 | A data table keyed by lane tag and hour; feeds Mass spawners. |
| 6 | **Nanite on everything opaque, no traditional LODs, TSR from 1080p.** | City Sample | sec. 3.3, 8 | Already planned; verify the 3070 8 GB VRAM budget (Epic recommends 8 GB min for the full demo, so you must stay on a 500 m slice). |
| 7 | **World Partition + Data Layers + One File Per Actor; HLOD for buildings.** | City Sample | sec. 3.4 | Already planned; add layer for "procedural", "hero hand-made", "festival". |
| 8 | **Procedural fabric, hand-made hero landmarks (80/20).** | Spider-Man 2, MSFS | sec. 10 risk 1; Step 12 | Generator for 22,006 footprints, hand/photogrammetry for ~10 hero facades (Jama Masjid, Sis Ganj). |
| 9 | **Automate distant-LOD / skyline** from footprints (extruded massing, merged HLOD). | Spider-Man 2 GDC | sec. 4.2 content rule | Auto-generate low-poly skyline beyond the construct wall from OSM heights. |
| 10 | **Swarm LOD ladder** for pigeons: animated near, sprite/billboard far, shared animation loops, merged logic. | A Plague Tale | Step 9, sec. 4.4 | Matches ~2,000 GPU boids; add a distant non-animated tier. |
| 11 | **Ambient vs scripted separation**: ambient spawner out of sight, set pieces authored. | GTA | sec. 4.5 | Spawn/despawn in a ring behind the camera; festival events as scripted layers. |
| 12 | **Physics only on hero objects; vehicles subset flagged "sandbox".** | City Sample | sec. 4.1 | Already planned; only nearest ~10 vehicles simulated, others on rails. |
| 13 | **Shrink density radius, not quality** (Mirage). If frame budget slips, cut area/visibility (smog helps) before cutting detail. | Mirage, plan sec. 8 | sec. 1, 8 | Keep 500 m slice as the done-criterion; defer the 2.9 x 2.2 km box. |
| 14 | **Pin the engine version and freeze** after UE 5.8.4 install. | Mumbai Gullies | sec. 3, Step 1 | No mid-project engine/plugin migration; do not adopt Experimental features (MetaHuman Crowd, Control Rig Physics) in the critical path. |
| 15 | **Profile weekly; set perf gates per system** (Unreal Insights, `stat gpu`), and gate every new system on ms budget. | Cyberpunk/Unity launch failures | sec. 8 | Add a CPU game-thread budget table (crowd <3 ms) alongside the GPU table. |

## 4. Pitfalls these teams hit

- **Scope vs team** (Mumbai Gullies: 55 people, open-world action-adventure, halted after ~5.5 years). Keep Phase 1 = vertical slice only.
- **Engine migrations mid-project** (UE4/UE5/Unity/UE). Do not do this.
- **Funding runway/hiring** (delayed money, raises). Plan for sole/small-team cash flow, not a studio salary base.
- **Launch performance** (Cyberpunk on last-gen, Unity frame rate). Test on minimum spec early; your min spec is the 3070.
- **Trailer density vs shipped density** (GTA VI commentary, Cyberpunk). Define crowd targets from profiled builds, not render-farm shots.
- **Procedural look-alikes that are "real footprint, wrong face"** (MSFS). Hero facades need photo reference; avoid promising exact likeness.
- **Over-ambitious NPC simulation** (Watch Dogs Legion). Keep NPC "depth" to appearance and a schedule.
- **Crunch** (RDR2). Not a model for a tiny team.
- **Licence traps**: Houdini City Building Generator needs a SideFX licence [V]; Google 3D Tiles are authoring-only [implementation.md].
- **Hardware reality**: Epic's recommended demo spec is 64 GB RAM and 8 GB VRAM [V]; you have 32 GB and an 8 GB 3070, so expect to cut view distance, shadow pages and Lumen quality (consistent with `implementation.md` section 8).

## 5. Scope calibration (person-years)

Estimates; "person-years" computed as team x years, **order-of-magnitude only**.

| Project | Team | Time | ~Person-years | Source quality |
|---|---|---|---|---|
| GTA V | 360 core / 1,000+ total | ~3-5 yr | ~1,000-3,000 | [V] team, [V] range |
| RDR2 | ~1,600 (to 2,000) | ~8 yr (incl. preliminary) | ~5,000-10,000+ | [V] low-quality |
| Cyberpunk 2077 | ~500 | ~8 yr | ~2,000-4,000 | [V] team |
| AC Unity | several hundred, multi-studio | ~3 yr | unknown (hundreds+) | [U] |
| Matrix Awakens | not published | demo only | unknown | - |
| Mumbai Gullies | peak 55, avg far lower | ~5.5 yr, halted | ~100-150 (rough) | [V] peak only |
| Raji | 3 to 13 | ~2.5 yr (R) | ~15-25 | [V] team |
| **delhi6 Phase 1 (vertical slice)** | 1 + Claude | planned ~20 weeks per world-design-pipeline.md | ~0.4 person-years human | own plan |

**Honest read:** a shipped GTA-class game is thousands of person-years; Mumbai Gullies' ~100+ person-years still did not ship a Mumbai open world. delhi6's feasible target is *one playable 500 m slice at final quality using a generator plus AI-assisted implementation*, which sits closer to a tech demo like City Sample (small Epic team, no content game) than to any shipped game. Treat the full 2.9 x 2.2 km box and all four weather states as a stretch goal behind the slice.

## 6. Open questions / gaps

- No public breakdown of Anvil crowd tech for Mirage; no Rockstar GDC talk on ambient population surfaced; Epic Matrix blog (exact counts) and PlayStation/GDC full talks need manual reading.
- GTA VI technical claims beyond Digital Foundry remain unconfirmed.
- No Indian-studio publication on street/crowd tech was found; worth a manual check of IGDC and Unreal Fest talks.
- Re-fetch the Epic blog in a browser to confirm [R] counts in section 1.1.
