# World Design Pipeline for a High-Realism Game

Author: Ritesh Roshan

## The decision

Build the game in **Unreal Engine 5**. Use World Labs (Marble) only as a concept and blockout tool, not as the source of your shipped world. Skip Three.js + Spark for this project.

### Why not the others

| Option | What it is good at | Why it loses here |
|---|---|---|
| **Unreal Engine 5** | Photoreal lighting (Lumen), film-density geometry (Nanite), huge streamed worlds (World Partition), procedural placement (PCG), physics (Chaos), audio (MetaSounds), realistic humans (MetaHuman), C++ and Blueprints | Heavy on hardware and disk; steeper start. Free until a product earns $1M gross, then 5% royalty. |
| **Unity 6** | Lighter, C#, strong for mobile, stylised and mid-fidelity games | Matching UE5 realism means HDRP plus third-party lighting, terrain and character tools. More assembly, weaker results out of the box. |
| **World Labs Marble** | Turns a prompt or image into an explorable 3D scene in minutes | Output is mainly Gaussian splats: no clean topology, weak collision, baked lighting, no destruction, no dynamic time of day. A splat cannot be a door you open, a wall you breach, or a tree that bends in wind. Good mood board, bad game level. |
| **Three.js + Spark** | Rendering splats in a browser, web demos, no install | WebGL/WebGPU has a tight performance ceiling, no built-in physics, AI, animation retargeting, audio occlusion or streaming. You would rebuild an engine before building a game. |

Realism in a game is mostly systems (physics, AI, audio, lighting that changes) rather than a pretty static image. UE5 ships most of those systems; the alternatives make you write them. That settles it.

The one case to choose Three.js + Spark: a browser walking-sim or interactive showcase where nothing needs to react. If that is the goal, say so and the pipeline changes.

## Machine and tools (all free)

- **Unreal Engine 5** (latest 5.x) via the Epic Games Launcher.
- **Visual Studio 2022 Community** with the "Game development with C++" workload.
- **Blender** for modelling, UVs, retopology and cleanup.
- **RealityScan** (Epic, free) for photogrammetry of your own photos.
- **Git + Git LFS**, or Perforce (free for small teams), for versioning large binaries.
- **Quixel Bridge / Fab** for scanned assets. Check each asset's licence and price; the library is no longer fully free.
- **World Labs Marble** for concept scenes (free tier is enough for this use).
- Hardware floor for comfortable UE5 work: 32 GB RAM, an RTX 3060-class GPU or better, SSD with 200 GB free.

## The pipeline, end to end

### 1. Define the world on paper (week 1)

Write a one-page world brief before opening any tool:

- **Place and time**: real region, climate, season, era. Realism needs a reference you can photograph or look up.
- **Scale**: playable area in km², and how much is dense versus open. A 2 km × 2 km area is a large, achievable first target; 16 km² is a studio-sized one.
- **Player verbs**: what the player does to the world (walk, climb, drive, break, hunt). Every verb becomes a system requirement.
- **Systems list**: pull from the realism docs in `realism/`: weather, day/night, AI routines, survival needs, ballistics. Mark each as core, later, or never.
- **Performance target**: e.g. 60 fps at 1440p on an RTX 3070. Everything later is judged against this.

Collect a reference board: 200+ real photos of the region (terrain, plants, rock, buildings, light at different hours). Realism is copied from the real world, not imagined.

### 2. Concept and mood with World Labs (week 1-2)

Use Marble to generate 5-10 scenes from your reference photos and prompts. Walk through them and decide:

- the lighting mood and colour palette,
- the composition of key spaces (a valley, a town square, an interior),
- sight lines and landmark placement.

Export a screenshot set and, if useful, a rough mesh export as scale reference in Unreal. Do not ship splats in the game. Their job ends here.

### 3. Project setup in Unreal (week 2)

- New project from the **Third Person** or **First Person** C++ template.
- Enable: World Partition, Lumen (GI and reflections), Nanite, Virtual Shadow Maps, PCG, Chaos, MetaSounds, Mass Entity, StateTree, Water.
- Set up version control on day one, with LFS rules for `.uasset` and `.umap`.
- Folder convention: `Content/World/`, `Content/Systems/`, `Content/Characters/`, `Content/Audio/`, `Content/Megascans/`.
- Turn on **One File Per Actor** so several people (or branches) can edit the same map.

### 4. Terrain (week 3-4)

Real terrain beats sculpted terrain.

1. Download real elevation data for your region: free DEMs from USGS EarthExplorer, Copernicus DEM (30 m), or national lidar where available (often 1 m).
2. Crop and resample to a heightmap in a size Unreal's landscape accepts (e.g. 4033 × 4033 for about 4 km at 1 m per pixel), as 16-bit PNG or RAW.
3. Import as a World Partition landscape.
4. Add erosion and detail in Unreal's landscape tools or Blender, keeping the real silhouette.
5. Build one **auto-material** for the landscape that picks layers by slope, height and noise (grass, dirt, rock, scree, snow), with runtime virtual texturing for blending with meshes.

### 5. Assets: scan, buy, model (ongoing)

Use three sources, in this order of preference:

- **Scan your own**: photograph rocks, bark, ground and props on an overcast day with RealityScan, clean in Blender, import as Nanite meshes. This gives a world nobody else has.
- **Library scans** from Fab/Megascans for fill, within licence.
- **Hand modelled** for anything mechanical or architectural (doors, vehicles, buildings), built modular on a metric grid so pieces snap.

Rules that keep it realistic: real-world scale (1 unit = 1 cm), calibrated PBR values (no albedo darker than about 30 sRGB or brighter than about 240), and every interactive object gets a simple collision mesh and a physical material (wood, metal, concrete) so ballistics and footsteps know what they hit.

### 6. Populate with procedural rules (week 5-8)

Hand-placing a forest does not scale. Use the **PCG framework**:

- Graphs that scatter trees, shrubs, rocks and debris by slope, height, wetness and distance from roads.
- Spline-driven roads, rivers, fences and power lines that clear vegetation around themselves.
- Biome volumes that swap rule sets (forest, meadow, riverbank, town edge).

Then hand-dress every place the player will stop and look: landmarks, interiors, story spots. A good split is 80% procedural, 20% hand-crafted, with the hand work where attention goes.

### 7. Lighting and atmosphere (week 6-8)

- Lumen for fully dynamic GI so time of day and weather can change.
- **Sky Atmosphere + Volumetric Clouds + Exponential Height Fog**, driven by a single time-of-day controller.
- Physical light units: sun around 100,000 lux at noon, auto exposure tuned to real EV ranges, not a fixed brightness.
- Post-process kept honest: mild film grain, real lens flare and bloom, motion blur matched to shutter speed. Avoid heavy colour grading that hides bad materials.

### 8. Systems that make it feel alive (week 8-16)

Build in this order, each against the matching doc in `realism/`:

1. **Player movement and camera**: motion matching for locomotion, IK feet on uneven ground, first-person body visible.
2. **Weather and time**: one controller driving sun, clouds, wind (which drives foliage and audio), rain wetness on materials, puddles, snow coverage.
3. **Audio**: MetaSounds plus occlusion and reverb zones per space; ambient beds that change with time and weather.
4. **Physics and damage**: Chaos destruction on chosen objects only; ballistics with penetration by physical material.
5. **AI and life**: StateTree for individual NPC and animal behaviour, Mass Entity for crowds and herds, Smart Objects for routines (sit, work, drink).
6. **Survival needs and UI**: only if they are core; keep the HUD minimal and in-world.

### 9. Streaming and performance (continuous, not at the end)

- World Partition grid cells sized for your traversal speed; HLODs generated for distant cells.
- Nanite for static geometry; foliage as Nanite where supported, otherwise tuned LODs.
- Profile weekly with **Unreal Insights** and `stat gpu`. Hold the budget from step 1. A realistic world that runs at 25 fps feels less real than a slightly simpler one at 60.

### 10. Vertical slice, then scale (week 16-20)

Before building the whole map, finish one 500 m × 500 m area to final quality: terrain, assets, lighting, weather, AI, audio, performance on target. Play-test it with people who have not seen it. Fix what breaks immersion. Only then copy the rules (PCG graphs, materials, systems) across the full world.

### 11. Ship

- Package for Windows with shader precompilation (PSO caching) to avoid stutter.
- Test on the minimum-spec machine, not your own.
- Distribute on itch.io (free) or Steam (one-off $100 fee per app, recouped after sales).

## Summary of the stack

```
Reference photos + real DEM data
        │
World Labs Marble  ──► mood, layout, scale reference (not shipped)
        │
Blender + RealityScan + Fab ──► Nanite meshes, PBR materials
        │
Unreal Engine 5
  ├─ Landscape + World Partition   (terrain, streaming)
  ├─ PCG                           (forests, roads, biomes)
  ├─ Lumen + Sky/Clouds/Fog        (dynamic light, time of day)
  ├─ Chaos                         (physics, destruction)
  ├─ MetaSounds                    (audio)
  ├─ StateTree + Mass + SmartObj   (AI, crowds, routines)
  └─ Insights                      (performance)
        │
Packaged build ──► itch.io / Steam
```

## Biggest risks

- **Scope**: realism multiplies work per square metre. Start small (step 10) and grow.
- **Performance**: decided by asset discipline, not by the final optimisation pass.
- **Uncanny details**: one wrong-scale door or floating rock breaks a whole scene. Review in-game, at eye height, under several lighting conditions.
- **Asset licences**: track the source and licence of every third-party asset in a spreadsheet from day one.
