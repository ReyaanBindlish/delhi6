# Implementation: Old Delhi, Phase 1

This plan replaces the "Treeline" mountain-valley concept in `tasks/plan.md` as the active direction. It builds on `world-design-pipeline.md` (engine choice, pipeline stages) and the `realism/` docs (systems), and does not repeat them.

## 1. Scope

**Phase 1 = the walled city of Old Delhi (Shahjahanabad), playable at final quality.**

| Item | Decision |
|---|---|
| Playable area | Box lat 28.645–28.665, lon 77.215–77.245 (about 2.9 × 2.2 km): Chandni Chowk, Dariba Kalan, Kinari Bazar, Chawri Bazar, Jama Masjid, Khari Baoli, Fatehpuri, Red Fort edge, Old Delhi Junction |
| Vertical slice first | 500 × 500 m centred on 28.6555, 77.2320: Chandni Chowk from Sis Ganj Gurdwara to the Jalebi Wala corner, Dariba, Kinari, Paranthe Wali Gali |
| Engine | Unreal Engine **5.8.x** (5.8.4 is current; Epic calls 5.8 the last major UE5 release). Pin the project to it. |
| Hardware target | 60 fps at 1440p (TSR from ~1080p) on an RTX 3070 8 GB, 32 GB RAM |
| Out of scope for Phase 1 | Rest of Delhi, IGI airport (20 km away; represented indirectly, see §5), interiors beyond shop fronts and a few hero rooms |

**Phase 1 is done when** a player can walk the vertical slice in any of four weather states (clear summer, winter smog, monsoon rain, dust storm) at 60 fps, the city is bounded by the construct wall, and every surface reacts correctly to physics and ballistics.

## 2. What already exists

| Asset | Location | State |
|---|---|---|
| Old Delhi footprints | `world/delhi/source/old_delhi_chandni_chowk_overture_buildings.geojson` | 22,006 buildings (Overture: OSM + Google Open Buildings) |
| OSM roads, POIs, landuse | `world/delhi/source/old_delhi_chandni_chowk.osm` | Full Phase 1 box |
| Real POIs | `world/delhi/old_delhi_pois.csv` | 557 named places with lat/lon and tags |
| Elevation | `world/delhi/source/Copernicus_DSM_COG_10_N28_00_E077_00_DEM.tif` | 30 m DEM |
| Modeled slice | `world/delhi/old_delhi_slice_detailed.blend/.fbx` | 300 × 300 m, 537 buildings, 1.1 M faces, UCX collision, overhead wires |
| Blockouts | `world/delhi/old_delhi_chandni_chowk_blockout.*`, `connaught_place_blockout.*` | Context massing |
| Generator | `world/karachi/scripts/lyari_prep.py`, `lyari_build.py` (style `delhi`), `lyari_render.py` | Footprints to modeled facades |
| Textures | `world/textures_polyhaven_cc0/` | 9 CC0 PBR sets, 2K |
| Characters | `characters/export/*.fbx` | Two textured, unrigged, ~60k tris |

## 3. Architecture decisions

1. **FBX, one file per building.** UCX_ collision only survives through FBX; there is no Blender Datasmith exporter, and USD level import is still Experimental. Today's slice is one FBX for everything. That must change before import (Step 3 below).
2. **Interchange import preset:** Combine Meshes off, Build Nanite on, Generate Lightmap UVs off, Auto Generate Collision off, One Convex Hull per UCX on, Import Materials off (assign master-material instances). Always reimport into a clean folder: a UE 5.5 Interchange rename bug (UE-248981) breaks UCX links. Fallback is `Interchange.FeatureFlags.Import.FBX 0`.
3. **Nanite** on all opaque building geometry. Glass is not Nanite: use opaque reflective glass with interior mapping in shop fronts.
4. **World Partition:** main grid cell 6400 cm, loading range 19200–25600 cm; second grid for small props at ~6000 cm. Data Layers for festival and season dressing. HLOD: merged/approximated for buildings, instanced for props.
5. **Georeferencing:** GeoReferencing plugin, Flat Planet, projected CRS `EPSG:32643` (UTM 43N), geographic `EPSG:4326`. Every slice writes a manifest with its origin lat/lon so UE coordinates line up with the real city. Check alignment at Jama Masjid's central dome and the Lahori Gate.
6. **Cesium for Unreal as an authoring overlay only.** Stream Google Photorealistic 3D Tiles in the editor to check skyline and footprints against reality. Never ship them: the Map Tiles API terms forbid caching, and streaming is billed per request.
7. **One weather interface.** A C++ `UWorldSubsystem` owns time of day and weather and writes `MPC_Weather` (Wetness, PuddleLevel, DustAmount, SmogDensity, WindDir, WindStrength). Every master material, Niagara system and audio zone reads from it. Ultra Dynamic Sky & Weather (Fab, ~₹4,700, commercial licence) may be bought to prototype, but it sits behind our MPC so the Delhi presets stay ours.
8. **Fiction policy for the real world** (§6). Real geography and monuments; fictional brands, businesses and people.

## 4. Systems

### 4.1 Physics (Chaos)

**Physical materials.** One Surface Type and Physical Material per surface, assigned in each master material.

| Surface | Friction | Restitution | Density g/cm³ |
|---|---|---|---|
| Lakhori brick | 0.65 | 0.10 | 1.9 |
| Lime plaster | 0.60 | 0.05 | 1.6 |
| Red sandstone (Chandni Chowk paving) | 0.60 | 0.10 | 2.3 |
| Kota stone | 0.55 | 0.10 | 2.7 |
| Corrugated shutter | 0.40 | 0.30 | 7.8 |
| Glass | 0.30 | 0.20 | 2.5 |
| Asphalt | 0.75 | 0.05 | 2.3 |
| Mud / monsoon slush | 0.45 | 0.00 | 1.7 |

Mud uses Friction Combine = Multiply so vehicles slide. Wetness from `MPC_Weather` lowers effective friction on stone and asphalt.

**Ballistics.** Line traces with `bReturnPhysicalMaterial = true` look up a DataTable keyed by Surface Type for penetration depth, ricochet angle, decal, Niagara impact and MetaSound impact. Details follow `realism/damage-and-ballistics.md`.

**Destruction.** Geometry Collections only on chosen props: fruit and food carts, plastic chairs, glass counters, signboards, water tanks, and one breakable wall per set piece. Voronoi with 2–3 cluster levels, Remove On Break after 5–10 s, at most ~300 active pieces. Large collapses are baked with the Chaos Cache Manager. Load-bearing buildings stay static.

**Vehicles (ChaosVehicles).** E-rickshaws and auto-rickshaws: three-wheel `ChaosWheeledVehicleMovementComponent`, ~350 kg, raised centre of mass so they can tip. Cycle-rickshaws: same rig, pedal-limited torque. Two-wheelers: four simulated wheels or hidden outriggers, plus a PID lean torque in Async Physics Tick and visual lean from animation.

**Cloth.** Chaos Cloth only on awnings, tarps and Kinari Bazar tinsel curtains within ~15 m of the player. Laundry, distant tarps and the overhead wire tangles use World Position Offset driven by the wind MPC.

**Ragdolls.** Physics Asset plus Physical Animation Component for hit reactions; at most ~8 simultaneous ragdolls, then freeze. Test Control Rig Physics (5.8 Beta) for active ragdolls.

**Tick.** Tick Physics Async on, fixed step 0.0166 s (60 Hz), and enable Async Physics Tick on each vehicle and ballistics component too. Fallback: substepping at 0.0166 s, max 4 substeps.

**Never simulate:** buildings (static, query-only), PCG litter, crowd agents beyond capsules near the player, wire tangles, distant cloth, debris beyond 40 m.

### 4.2 World boundary: the construct wall

The edge of the playable city is invisible until the player is near it. Close up, a faint lattice shimmers into view, like the construct walls in *The Matrix Resurrections*, or a pale fading snowflake/hex pattern, so the player knows they've reached the edge without any UI.

**Build:**
- `BP_WorldBoundary` holds a closed spline around the Phase 1 box, following real streets rather than a perfect rectangle: Netaji Subhash Marg, the railway line, Shyama Prasad Mukherjee Marg, Asaf Ali Road.
- Spline-mesh wall 30 m tall, non-Nanite (translucent), no collision. A matching Blocking Volume blocks Pawn and Vehicle channels. Bullets pass through (excluded from traces, tagged NoDecal).
- Material `M_Boundary`: translucent, unlit, two-sided, refraction via pixel normal offset.
  1. World position ÷ 60 cm cell → hex-distance or 6-fold snowflake SDF → thin line mask (**Lattice**).
  2. `SphereMask(WorldPos, MPC_Boundary.PlayerPos, radius 12 m, hardness 0.4)` (**Reveal**): only the patch of wall nearest the player appears.
  3. Fresnel rim, depth fade at the ground (80 cm).
  4. Glitch: time-stepped row bands randomly shift by a fraction of a cell and flash; slow panning noise shimmer.
  5. Emissive = Lattice × Reveal × Rim × pale cyan-white. Opacity = Lattice × Reveal × DepthFade × 0.7. Slight refraction distortion × Reveal.
- `UBoundarySubsystem` (C++, 20 Hz): writes the player position to the MPC, computes distance to the spline, and drives:
  - a MetaSound hum and bit-crush that rise with proximity;
  - a gentle push back toward the city inside 2 m (impulse scaled by speed for vehicles);
  - a one-off glitch sting and controller rumble at 0.5 m.
- Optional post-process within 3 m: chromatic offset and faint scanlines, weighted by proximity.
- Content rule: skyline beyond the wall is filled with low-detail blockout (the existing `connaught_place_blockout` and OSM massing) plus distance fog, so the world doesn't visibly end.

### 4.3 Weather and atmosphere

Driven by the weather subsystem (§3.7). Preset values are starting points for tuning in-editor.

| State | Key settings | Real reference |
|---|---|---|
| Winter smog / fog (Nov–Jan) | Height fog density 0.08–0.2, falloff 0.05–0.1, inscatter ~(0.55, 0.50, 0.40); volumetric fog extinction ×2–3; Mie scattering ×4–6, Mie absorption raised and brown-tinted, Rayleigh ×0.5; sun 40–60k lux, capped exposure; sodium lamps with volumetric scattering 2–4; breath and brazier smoke | AQI 350–450+, visibility 600–800 m typical, 50–125 m on dense days |
| Summer (Apr–Jun) | Heat-shimmer post-process, high-key bleached light, hard shadows, dust ML up | 40–46 °C |
| Dust storm (aandhi) | Brown Mie absorption, Niagara dust sheet with flying litter and tarps, strong directional wind, cloud coverage 1 | Winds up to ~120 km/h, sky turns orange-brown, temperature drops up to 7 °C in 30 min |
| Monsoon (Jul–Sep) | Camera-attached GPU Niagara rain with depth-buffer splashes, wetness → roughness ~0.1 and darker porous albedo, puddles, ripple normals, gutter waterfalls off chhajjas, ankle-deep waterlogging decals near Jain Mandir | Chandni Chowk floods ankle-deep and drains in ~10 min |
| Kite season (15 Aug, Basant) | Dozens of kites over rooftops (Niagara/actors), cut kites drifting down, streets emptier | Lal Kuan kite market |

### 4.4 Fauna

| Animal | Implementation | Budget |
|---|---|---|
| Pigeons | GPU Niagara boids on Neighbor Grid 3D; AnimToTexture meshes within 30 m, sprites beyond; PCG perch points on ledges, wires and domes; state machine Circle → Land → Peck → Scatter (player within 4 m, gunshot, vehicle). Big feeding flocks at Jama Masjid | ~2,000 birds in ~0.5 ms GPU |
| Black kites (cheel) | 20–40 actors on orbits (radius 40–120 m, altitude 60–200 m) with banking; dive-and-catch near mosques and meat stalls | 10 Hz manager |
| Crows, mynas | Small Niagara groups on wires and rooftops | — |
| Rhesus macaques | Animated actors on rooftops and wires; steal food | Few, near player |
| Street dogs, cows, goats | MassAI on ZoneGraph lanes; dogs sleep in shade, cows at garbage points; goat market at Urdu Park around Bakrid (Data Layer) | Part of crowd budget |
| Rats | Night-only, near drains | — |

### 4.5 Crowds and traffic

MassCrowd + MassAI + ZoneGraph street lanes, StateTree behaviours, City Sample as reference. MetaHuman Crowd (5.8, Experimental) for LOD transitions. About 600 visible agents: ~40 full skeletal agents near the player, the rest vertex-animated instances. Under 3 ms game thread. Traffic on Chandni Chowk is rickshaws and e-rickshaws by law (9 am–9 pm) with illegal two-wheelers mixed in; side lanes add handcarts and porters.

### 4.6 Ground and micro-detail

- **PCG (GPU scatter in 5.8):** surface sampler on street meshes → filters (distance to footprint, kerb mask, market-zone tag) → ISM spawner, no collision. Scatter: pebbles and grit, kulhad shards, leaf plates, gutka sachets, plastic bags, marigold petals near temples, cow dung, string and jute.
- **DBuffer decals** (`r.DBuffer=1`): paan and gutka stains (~1 per 2–5 m² in lanes, densest at corners and wall bases), oil, water lines, torn layered posters, stencilled phone numbers.
- **RVT** per WP region so street dirt blends into wall plinths.
- **Nanite displacement** only on the redeveloped sandstone paving, cobbles and close-up brick; profile it (~2 ms), and note a 5.8 regression (UE-370671).
- **Material layers:** global dust/grime on all horizontal surfaces (heavier in summer, washed in monsoon), rising-damp tide line 0.5–1 m, black streaks under chhajjas and AC drips, moss at wall bases, lime plaster peeling to lakhori brick.

### 4.7 Audio

MetaSounds everywhere, using 5.8 MetaSound Templates. Audio Gameplay Volumes per lane and interior: reverb, low-pass inside shops, convolution submixes for narrow galli, courtyard and mosque. Soundscape plugin for one-shots (vendors, rickshaw bells, horns, pigeon wing claps, shutters rolling at 10–11 am and night).

**Azaan:** emitters on the Jama Masjid and Fatehpuri minarets, driven by time of day, neighbouring mosques staggered 10–40 s, 1.5–2.5 km attenuation with air absorption.

Plus temple bells and aarti (Gauri Shankar, Jain temples), and kirtan near Sis Ganj. Horn concurrency capped at 24.

## 5. Real locations

The full list is `world/delhi/old_delhi_pois.csv` (557 places from OSM). Hero locations for Phase 1:

| Group | Places (lat, lon) |
|---|---|
| Landmarks | Red Fort / Lahori Gate (28.6559, 77.2388), Jama Masjid (28.6507, 77.2335), Gurdwara Sis Ganj (28.6559, 77.2323), Digambar Jain Temple + bird hospital (28.6558, 77.2361), Gauri Shankar Temple (28.6558, 77.2357), Fatehpuri Masjid (28.6566, 77.2225), Town Hall (28.6573, 77.2275), Sunehri Masjid (28.6561, 77.2319), Central Baptist Church (28.6565, 77.2339), Delhi Gate, Ajmeri Gate, Kashmiri Gate |
| Food streets | Paranthe Wali Gali (28.6560, 77.2304), Gali Kababian by Jama Masjid Gate 1 (28.6494, 77.2337), Dariba corner jalebi stall (28.6560, 77.2335), Matia Mahal (28.6497, 77.2344) |
| Markets | Khari Baoli spices (28.6572, 77.2229), Chawri Bazar brass and wedding cards (28.6499, 77.2298), Kinari Bazar tinsel, Dariba Kalan silver, Bhagirath Palace lighting, Lajpat Rai Market electronics (28.6574, 77.2360), Meena Bazaar (28.6516, 77.2359), Lal Kuan kites |
| Hospitals | Kasturba Hospital (28.6485, 77.2385), Shroff Charity Eye Hospital (28.6459, 77.2421), Sant Parmanand Hospital (28.6631, 77.2379), Mohalla Clinic prefab cabin (repeatable asset) |
| Schools | IGDTUW (28.6648, 77.2322), Anglo Arabic School, est. 1692 (28.6457, 77.2224), Fatehpuri Muslim School (28.6566, 77.2226), Sarvodaya Kanya Vidyalaya (28.6576, 77.2350) |
| Police | Kotwali (28.6586, 77.2311), Jama Masjid PS (28.6525, 77.2337), Hauz Qazi PS (28.6498, 77.2258) |
| Transit | Metro: Chandni Chowk (28.6584, 77.2303), Chawri Bazar (28.6493, 77.2265), Jama Masjid (28.6502, 77.2375), Lal Qila (28.6575, 77.2367). Old Delhi Junction (28.6609, 77.2277), red brick and clock tower, with a DTC bus bay |

**Airport.** IGI is about 20 km away and is not modeled in Phase 1. It shows up as aircraft on the approach path overhead with distant engine noise, airport taxis and destination boards, and an Airport Express metro transition (from New Delhi station) used as a fast-travel or loading-screen link.

Food place names and their current operation need a site visit or map check before use. They are fictionalised in-game anyway (§6).

## 6. Advertising and the fiction policy

**Signage the city must have.** Signage density is a big part of why Old Delhi looks real.

| Type | Notes |
|---|---|
| Shop fascia boards | Dominant. Flex or painted tin, 0.6–1.2 m tall × shop width, often stacked 2–3 deep and overlapping windows |
| Flex banners | 1 × 3 m to 3 × 6 m, roped to balconies and wires |
| Hand-painted wall ads | Cement, paints, water tanks, clinics, coaching classes |
| Painted shutters | Shop name, phone number, sometimes a deity image |
| Political and festival posters | Rows of head-shot cutouts in party colours |
| Coaching, tuition, wedding-band flyers | Glued to poles and electrical boxes; torn layers |
| Back-lit acrylic and LED | Bhagirath Palace, Lajpat Rai Market; scrolling LED at pharmacies and jewellers |
| Government heritage boards | Uniform style on the redeveloped main road; compliance varies shop to shop |

- **Scripts:** Devanagari + English (most), Urdu Nastaliq around Jama Masjid, Matia Mahal and Ballimaran, Gurmukhi near Sis Ganj and Fatehpuri.
- **Colours:** red/maroon + yellow, saffron, royal blue + white, green, black + gold (jewellers). Big contrasting phone numbers on almost every sign.
- **Density:** 1–3 signs per metre of frontage on several floors in Chawri, Kinari and Dariba; moderate and more uniform on the main road.
- **Weathering:** sun-bleached flex (red → pink, blue → cyan), rust streaks from fixings, generator soot, pigeon droppings, monsoon stains, peeling poster layers.

**Policy.** This is a working rule, not legal advice; have a lawyer review before release.

- **Keep real:** geography, street, bazaar and station names, monuments, religious sites, and generic trades.
- **Fictionalise:** every brand, logo, shop and restaurant name, politician, party, coaching institute and newspaper. Indian trademark law (s.29(4), dilution) and strong Delhi High Court publicity-rights rulings make unlicensed real brands and faces the highest risk. Metro and railway keep their names but use fictional logos unless licensed.
- **Religious sites:** depicted respectfully; no combat inside mosques, temples or gurdwaras.
- **Photogrammetry inside ASI monuments** (Red Fort, Jama Masjid premises) needs ASI permission through the SAHMAT portal. Modeling from public-vantage photos does not. No drones near the Red Fort (no-fly zone).
- **Never ship Google imagery or tiles.** Real-place texture sources are our own photos and photogrammetry, Mapillary (CC-BY-SA: attribute, and share-alike applies to derived textures), and CC0 libraries.

**Fictional brand kit (clear each name with a trademark search before use):** Chandni Chai Co., Lal Kesar Sweets, Purani Haveli Paranthe, Masala Mahal Spices, Zaika-e-Dilli Kebab House, ChaloNet 5G, Bharat Taranga Mobile, Toofan Cola, Gyan Path IIT-JEE Academy, Safalta Classes, Rangoli Shield Paints, Mazboot Cement ("Ghar Ki Neev"), Shahi Kundan Jewellers, Chamak Silver House, Raftaar E-Rickshaw Batteries.

## 7. Micro-detail catalogue

Ranked by realism gained per unit of effort; build top-down.

1. Paan, spit and dung decal atlas
2. Overhead wire-nest spline kit with a per-street density parameter. Since Feb 2026, ~52.5 km of lines are being moved underground, so build "before" and "after" variants
3. Global dust/grime material layer + wetness/puddle layer
4. Lime plaster peeling over lakhori brick
5. Layered torn posters and stencilled phone-number decals
6. Pigeon flocks and circling kites
7. Winter smog preset (visibility 100–800 m)
8. Azaan and temple bells keyed to time of day
9. Tarps and awnings with wind sway
10. Plastic litter and marigold petals with wind drift
11. Cycle-rickshaw and e-rickshaw traffic with bells and horns
12. Deity tiles in wall corners (20–30 cm glazed tiles, 0.3–1.2 m up)
13. Cool-white tube-light shop interiors at night
14. Open drains (0.3–0.6 m) with grates and slab bridges
15. Kinari Bazar tinsel and gota curtains (cheap cloth, big colour payoff)

Further detail, all from field research: red sandstone paving with bollards on the main road, already worn; kota stone thresholds; manholes sunk, raised or patched; haveli kit (jharokhas, carved doors with brass studs, cusped arches) with modern shutters cut into old arches; porters with head loads; thelas; Khari Baoli spice-dust particles that make people sneeze; LPG cylinders; plastic chairs; chai stalls; wedding bands and dhol at night; shutters rolling up at 10–11 am.

## 8. Performance budget (RTX 3070, 1440p, 16.6 ms)

| Pass | GPU ms |
|---|---|
| Nanite + Virtual Shadow Maps | 4.5 |
| Lumen (Lumen Lite first, software RT otherwise) | 3.5 |
| Sky, clouds, volumetric fog | 1.5 |
| Translucency + Niagara (rain, birds, dust) | 1.5 |
| TSR + post | 2.0 |
| Headroom | 3.0 |

Settings: `r.Streaming.PoolSize 3000`, `r.Nanite.Streaming.StreamingPoolSize 512`, `r.Shadow.Virtual.MaxPhysicalPages 4096`. Textures 2K, 4K only on hero assets. Profile weekly with `stat gpu` and Unreal Insights. Smog nights are a gift: short visibility hides distance cost.

## 9. Immediate steps

| # | Step | Owner | Output |
|---|---|---|---|
| 1 | Install Epic Games Launcher → **UE 5.8.4**, and Visual Studio 2022 Community with "Game development with C++" (~100 GB total) | Claude installs; you sign in to Epic | Engine + compiler ready |
| 2 | `git init` the game repo with Git LFS rules (`*.uasset`, `*.umap`, `*.fbx`, `*.blend`, `*.png`, `*.jpg`); `gh auth login` | You (auth), Claude (repo) | Versioned project |
| 3 | Generator update: export **one FBX per building** with its `UCX_` pieces, write a slice manifest (origin lat/lon, CRS, building list), apply surface tags per material | Claude | `export/old_delhi_vs/*.fbx` + `manifest.json` |
| 4 | Build the **500 m vertical slice** around Chandni Chowk–Dariba–Kinari (replaces the current 300 m test slice), including the red sandstone main road and bollards | Claude | Vertical-slice FBX set |
| 5 | UE project `OldDelhi` (C++ Third Person), plugins: GeoReferencing, PCG, ChaosVehicles, Chaos Cloth, Niagara, MassAI/MassCrowd/ZoneGraph, StateTree, Audio Gameplay Volumes, Soundscape, Interchange; project settings from §3, §4.1 and §8 | Claude (Python/C++), you open the editor | Project skeleton |
| 6 | Python import script using the Interchange preset; master materials with physical materials and `MPC_Weather` hooks | Claude | Slice walkable in UE |
| 7 | `BP_WorldBoundary` + `M_Boundary` + `UBoundarySubsystem` | Claude | Construct wall |
| 8 | Weather subsystem + four presets (summer, smog, monsoon, aandhi) | Claude | Weather switching |
| 9 | Pigeon boids + kite orbits prototype | Claude | Birds |
| 10 | PCG ground scatter + decal atlas (top-15 items 1, 3, 5, 10) | Claude | Ground detail |
| 11 | Fictional signage kit: fascia boards, flex banners, posters in Devanagari, Urdu and English using the brand list | Claude | Ad/sign decals and meshes |
| 12 | Photo reference: a walk through the vertical slice with a phone (facades, ground, signs), plus RealityScan captures of 3–5 hero facades from public vantage | You | Reference set + hero scans |

Steps 1–3 can start today. Steps 5–11 need step 1 finished.

## 10. Risks

| Risk | Mitigation |
|---|---|
| "Exact likeness" expectations vs. procedural buildings | Procedural for the fabric, photogrammetry for hero facades and landmarks; review side-by-side with Cesium overlay |
| 8 GB VRAM | 2K textures, Nanite streaming pool caps, short-visibility weather presets, HLOD discipline |
| UCX lost on import | Per-building FBX, clean reimport folder, legacy-importer fallback |
| Trademark / publicity claims | Fiction policy in §6, legal review before release |
| Religious sensitivity | No combat in places of worship; respectful depiction; community review |
| Scope creep beyond Phase 1 | New districts go to `tasks/later.md`; Phase 2 starts only after the vertical slice passes the done-criteria in §1 |

## 11. Sources

Research reports compiled 8 Nov 2026 from field journalism, Epic documentation and legal sources, including:
- Down To Earth (wire nests, Chandni Chowk redevelopment)
- Millennium Post (2026 underground cabling)
- ThePrint (redevelopment, pigeons, kites)
- Epic UE 5.8 release notes and forums (UE-248981, UE-370671)
- Cesium community (5.8 support)
- Fab licensing
- Delhi High Court rulings via LiveLaw and Deccan Herald
- ASI SAHMAT portal
- *AM General v. Activision* coverage
