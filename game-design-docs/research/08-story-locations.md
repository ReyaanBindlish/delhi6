# Story locations: map scoping and data (Delhi NCR, Ghaziabad, Karachi, Dehradun, Mamdot)

Data fetched 2026-11-10 into `world/story/source/`. Coordinates are from OSM (Overpass) unless marked "proposed" (a story placement the world team should confirm) or "est." (general knowledge, not measured). Bbox order is S, W, N, E (lat/lon); the Overture CLI wants W,S,E,N. Method and file layout copy `01-south-delhi-maps.md`.

Read this first: **OSM building coverage is thin outside central Delhi.** Overture has 2 to 100 times more footprints than OSM in Khoda, Loni, Orangi, Ibrahim Hyderi, Sahibabad, so the Blender blockouts (which read OSM only, script unmodified) are close to empty for those tiles. Use Overture as the footprint source and OSM for roads, POIs and landuse, exactly as for the Lyari slice (`lyari_prep.py` already does this). Neither source has building heights in these areas (Overture `num_floors` is present on well under 2% of footprints except Nizamuddin/Sarai Kale Khan at about 2.5 to 3%, mean 4.7 to 4.8 storeys), so heights are inferred (see notes per location).

## 1. Locations table

| # | Location | Acts | S, W, N, E | Size (m) | Area km² | Overture bldgs | OSM bldg ways | Blockout BUILT |
|---|---|---|---|---|---|---|---|---|
| 1 | Hazrat Nizamuddin station + basti, dargah edge | 2 (core loop), 3 (Khanna's bench), 6 (coda) | 28.5830, 77.2400, 28.5950, 77.2570 | 1662 x 1327 | 2.20 | 2,378 | 1,019 | 1,047 |
| 2 | Sarai Kale Khan ISBT + dhaba strip | 3 (pitch) | 28.5810, 77.2520, 28.5930, 77.2680 | 1564 x 1327 | 2.08 | 1,676 | 290 | 312 |
| 3 | Vasundhara, Ghaziabad (Sectors 4-13) | 1, 6 | 28.6540, 77.3620, 28.6690, 77.3790 | 1661 x 1659 | 2.75 | 3,879 | 1,738 | 1,792 |
| 4 | Khoda Colony, Ghaziabad | 2, 5 (end), 6 | 28.6210, 77.3300, 28.6350, 77.3500 | 1954 x 1548 | 3.03 | 5,939 | 127 | 145 |
| 5a | Loni flyover (Delhi-UP border corridor, Loni Gol Chakkar) | 1 (arrest) | 28.6980, 77.2840, 28.7210, 77.3020 | 1757 x 2543 | 4.47 | 7,950 | 174 | 182 |
| 5b | Loni town (dhaba, tree, east fringe) | 1, 6 | 28.7260, 77.2900, 28.7440, 77.3120 | 2147 x 1990 | 4.27 | 6,284 | 98 | 99 |
| 6 | Sahibabad Industrial Area + railway | 1 (operations) | 28.6600, 77.3450, 28.6740, 77.3640 | 1856 x 1548 | 2.87 | 3,312 | 700 | 733 |
| 7 | Dasna jail (exterior only) | 1 (arrival), 2 (flashes) | 28.6890, 77.5270, 28.6990, 77.5390 | 1172 x 1106 | 1.30 | 225 | 0 | 0 |
| 8 | Orangi Town (Gulbahar Lines hill and grid sub-district) | 4 | 24.9300, 66.9850, 24.9480, 67.0050 | 2019 x 1990 | 4.02 | 30,935 | 49 | 50 |
| 9 | Korangi Creek Road / Qayyumabad (Rangers checkpoint) | 5 | 24.8220, 67.0760, 24.8340, 67.0930 | 1718 x 1327 | 2.28 | 1,468 | 195 | 218 |
| 10 | Ibrahim Hyderi fishing village, jetty and creek | 5 (climax) | 24.7720, 67.1300, 24.8000, 67.1600 | 3032 x 3096 | 9.39 (about 40% water) | 7,459 | 40 | 42 |
| 11 | Dehradun farmhouse (Sahaspur / Tons river fringe) | 3 | 30.3625, 77.8135, 30.3775, 77.8305 | 1633 x 1659 | 2.71 | 721 | 0 | 0 |
| 12 | Mamdot, Ferozepur (reference only) | 6 coda, 3 (Bibi) | 30.8630, 74.4150, 30.8760, 74.4300 | 1433 x 1437 | 2.06 | 1,554 | 4 | 4 |
| L | Lyari / Gulbahar Lines (existing, `world/karachi/source/`) | 4 | 24.8547, 66.9846, 24.8853, 67.0155 (measured from Overture) | 3100 x 3400 | about 10.5 | 40,727 | n/a | existing |

Total new data: about 49 km² (including about 4 km² of water), 74,000 Overture footprints. Tile 5a/5b and 1/2 overlap or abut deliberately so they can be stitched. Tile 3 and 6 overlap by about 170 m (lon 77.362 to 77.364). `source/_tiles_bbox.csv` holds the final bboxes.

## 2. Per-location detail

### 2.1 Hazrat Nizamuddin station (Act 2 core loop)

Real station: Hazrat Nizamuddin Junction (NZM), OSM node 28.58896, 77.25352. It is unusually well mapped in OSM, which is lucky because this is the single most-played space in the game after Karachi.

Size justification: Act 2 is a 2 to 3 hour survival loop run on a game clock. Daya works a platform, the overbridge, the subway, the forecourt and the chai stall, is moved on by police drives and walks home. The playable core is about 0.8 x 0.8 km around the station; the rest of the 2.2 km² box is context so the walk to the shelter, the Nila Gumbad lane and the Dargah (the real beggars' economy) exist. Dargah Nizamuddin at the west edge (28.5914, 77.2419) is 1.1 km from the station and is where the free langar food would logically come from.

Story beats hosted: Act 2 platform loop, Mangal's teaching, police drives, pickpocket ring, SIM-card seller, Khanna's chai-and-bench scene; Act 3 handoff; Act 6 coda (Mangal's Rs 500 note).

Key OSM landmarks:

| Category | Name | lat, lon | Use |
|---|---|---|---|
| Station | Hazrat Nizamuddin Junction (ref NZM) | 28.58896, 77.25352 | Station building polygon (516 m perimeter) |
| Platform | Platform 1 | 28.58996, 77.25363 | North, platform "owned by a group that pays a constable" |
| Platform | Platforms 2-3 ("Hazrat Nizamuddin") | 28.58881, 77.25400 | Island platform |
| Platform | Platforms 4-5 | 28.58878, 77.25424 | Island platform |
| Platform | Platforms 6-7 | 28.58801, 77.25448 | South, newer |
| Footbridge | FOB 1 (110 m, not covered) | 28.58957, 77.25406 | Newcomers' spot in the story |
| Footbridge | FOB 2 (151 m, covered, paving) | 28.58850, 77.25412 | |
| Steps | Covered steps to FOB | 28.58943, 77.25395 / 28.58965, 77.25366 | |
| Forecourt | Nizamuddin Railway Station bus station / forecourt (west) | 28.58640, 77.25316 | Auto and bus stand |
| Ticketing | Rail reservation centre; Unreserved ticket window; Parcel office; Cloak room; Sleeper waiting area | 28.58561, 77.25365; 28.58862, 77.25483; 28.58585, 77.25498; 28.58837, 77.25355; 28.58846, 77.25497 | Daily-loop stops |
| Police | Railway police station (GRP) / Hazrat Nizamuddin PS | 28.58798, 77.25361 / 28.59233, 77.24383 | Police drives |
| Food | Aneja Restaurant, Cafe Turtle, Comesum (fast food) | 28.58945, 77.25252; 28.59037, 77.25250; 28.58835, 77.25369 | Chai-stall cluster opposite the spot (proposed: place the stall at 28.5895, 77.2525) |
| Worship | Station masjid; Raghunath Mandir; Gurdwara Damdama Sahib | 28.58859, 77.25329; 28.58985, 77.25242; 28.59478, 77.25254 | Langar and prayer cycle |
| Heritage | Nila Gumbad (tomb right beside the station) | 28.59190, 77.25314 | Iconic skyline element |
| Heritage | Nizamuddin Dargah complex; Mirza Ghalib's tomb; Chausath Khamba; Humayun's Tomb ticket counter | 28.59139, 77.24189; 28.59143, 77.24301; 28.59122, 77.24302; 28.59306, 77.24444 | Western context |
| Subway | SKK subway gates 2 and 3 (east side) | 28.58887, 77.25671; 28.58956, 77.25680 | Link to tile 2 |
| Roads | Baba Banda Singh Bahadur Setu (trunk viaduct, 3 lanes) | 28.5845, 77.2461 | Overhead noise and shade |
| Rail signals | HNZM S-24 to S-40 | 28.5905 to 28.5925, 77.2536 to 77.2544 | Real signal posts to place |

Fabric: OSM mean 4.7 levels (57 tagged, max 6). East of the station a government/railway colony and the Pant Nagar and Jangpura refugee colonies (3 to 5 storey brick/plaster); west, the dense Nizamuddin basti around the dargah (2 to 4 storeys, 2 to 4 m lanes, est.). Highways: residential 254, footway 289 (many informal), steps 50. Bridge layering is real: viaduct trunk at layer 2, station roads at layer 1, an under-track road at layer -1 (28.5844, 77.2542).

Hand-modelled vs generated: Hand-model the station (building, all 4 platform islands with canopies, tracks and ballast, signals, FOB 1 and FOB 2 with their stair cores, the pedestrian subway, which OSM does not map at all, the forecourt, ticket windows, cloak room, the chai stall, Mangal's spot). Generate the surrounding basti, Jangpura, Pant Nagar and road network from Overture and OSM. Dargah and Nila Gumbad are hero assets (Nila Gumbad's blue-tile dome is small and single).

### 2.2 Sarai Kale Khan and its dhaba (Act 3 pitch)

Real: Sarai Kale Khan ISBT, OSM way 80439683 (center 28.58508, 77.25905); the "Inter State Bus Terminus" platform way is at 28.58877, 77.25905. The station area is 600 m east of Nizamuddin and the two are walkable.

Size justification: the pitch is a single dhaba conversation, about 10 minutes of play, but it needs a believable approach (the bus-stand edge, the nala, the fly-over noise), so 2.1 km² is generous. Cheapest of the Delhi tiles.

Date note (Act 3 is 2022): both RRTS lines are mapped as `railway=construction` through this tile (Delhi-Meerut at 28.5914, 77.2572; Delhi-Alwar at 28.5873, 77.2586). That is historically correct for 2022: raw concrete viaduct piers, hoardings and barricades. Do not build the finished RRTS station.

Key OSM landmarks:

| Name | lat, lon | Use |
|---|---|---|
| Sarai Kale Khan ISBT (bus station) | 28.58493, 77.25894 | Main set |
| "Brij Vasi" fast food at the ISBT approach | 28.58707, 77.25906 | Proposed dhaba site: Ravi's table (fictionalise the name) |
| Sarai Kale Khan stops / Gate 1 and 2 subway entrances | 28.58983, 77.25705; 28.59006, 77.25734; 28.58912, 77.25736 | Pedestrian approaches |
| Barapulla Nala (open drain) | 28.58319, 77.25750 | Atmosphere, smell, a back-lane |
| Lotus Pond, Baansera Musical Fountain and park | 28.58931, 77.26015; 28.59257, 77.25997; 28.5928, 77.26094 | Greenery N of ISBT (Baansera park has Peepal trees at 28.5905, 77.2601) |
| Guru Harkrishan Hospital; RTO | 28.58148, 77.26196; 28.58344, 77.26087 | Context |
| Indian Oil / IGL fuel stations | 28.58596, 77.26023; 28.58488, 77.26022 | Bus-stand edge |

Fabric: low density (OSM 290 building ways, Overture 1,676): ISBT sheds, RTO, depots, a few 4 to 6 storey apartment pockets (Sidhartha Extension). Heavy elevated infrastructure: Ring Road (trunk, 3 to 4 lanes each way), RRTS piers, Barapulla elevated corridor.

Hand vs generated: hand-model the ISBT terminal shed and bays, the dhaba (charpoys, steel utensils, tandoor, truckers) and RRTS construction piers; generate everything else.

### 2.3 Vasundhara, Ghaziabad (Acts 1 and 6)

Real: Vasundhara is a GDA (Ghaziabad Development Authority) planned township, numbered Sectors 1 to 15. OSM tags Sectors 4 to 13 at their label positions: Sector 4 (28.6672, 77.3783), 5 (28.6608, 77.3779), 6 (28.6656, 77.3746), 7 (28.6590, 77.3740), 8 (28.6650, 77.3700), 9 (28.6581, 77.3700), 10 (28.6616, 77.3671), 11 (28.6568, 77.3666), 12 (28.6615, 77.3646), 13 (28.6552, 77.3637). The suburb node is 28.66197, 77.3733.

Size justification: Act 1 is a 90-minute prologue: the home hub (Daya's three-storey house, Sukhi, Bibi, Gurnoor with the cricket bat), the Fortuner drive out, and in Act 6 the 2 a.m. approach to Bablu's flat on foot. The bible says the flat is "two streets away", so the entire geography fits in about 400 x 400 m; the rest is the drive out (Fortuner leaving via the sector roads) and ambient hub. 2.75 km² gives 10 sectors of context.

Proposed placement (fictional, confirm with narrative): Daya's house on a plotted block in Sector 5 (about 28.6605, 77.3775); Bablu's builder flat in the 4 to 5 storey apartment cluster "Olive County" (28.65909, 77.37887), about 170 m away. The rail on the 4th-floor balcony and the pavement below must be hand-built to the exact 90 cm rail height the bible specifies.

Landmarks: Sahibabad Delhi Metro (Blue line extension) and RRTS station at the NW corner (28.66756, 77.36797 / gates at 28.66735, 77.36683 and 28.66645, 77.36736); Prem Dharam Hospital 28.66009, 77.36796; Ram Mandir 28.65788, 77.36634; Dronagiri Apartments and playground 28.65718, 77.36593; Aadarsh Park 28.66617, 77.37480; Amity International School 28.66529, 77.37527; Vidya Bal Bhawan school 28.65543, 77.36820; DPS Vasundhara 28.65777, 77.37059; society apartments (Aditya Garden City 28.66512, 77.37336; Kautilya, Swarna Ganga, Janhitkari, Vasundhara Valley around 28.663 to 28.666, 77.372 to 77.374); Nihal Sweets 28.66354, 77.37807 (a corner shop for the milk-delivery shutter in the final shot); water tank 28.66426, 77.37821. 

Fabric (est.; OSM has no `building:levels`): sector plots of about 90 to 300 m² built up as 3 to 4 storey independent houses and builder floors with stilt parking; cooperative/GDA flat societies of 4 to 5 storeys at the centre (the "builder flat" Bablu lives in); newer towers (10 to 14) at the edges. Sector roads about 18 to 24 m, internal streets 7 to 12 m, a lot of parked cars and AC condensers, water tanks on the roof, tiled and paint-peeled facades, thin builder rails. Highways in OSM: residential 241, service 195, tertiary 36, secondary 23. Overture density is highest in the west (Sectors 9 to 13) and east (6, 4).

Hand vs generated: hand-model Daya's house (full interior, 3 floors; hub revisited three times), Bablu's flat interior and balcony (shared fight set), 12 m fall column and pavement; generate the other sector housing from Overture with a plotted-house generator.

### 2.4 Khoda Colony, Ghaziabad (Acts 2, 5 end, 6)

Real: Khoda Colony / Khora Colony, Ghaziabad, PIN 201020; Apple Maps places it at 28.62787, 77.33530; OSM "Khora" suburb node at 28.62625, 77.34658. It is an unplanned urban village that has grown into an unauthorised colony on the Delhi-Noida-Ghaziabad borderlands.

Size justification: this is the family's one-room rental: a lane (where Gurnoor learns to bowl), the room, the gate with the police tape at the end of Act 5, and the 4-minute empty-house walk in Act 6. The player returns three times, each time poorer, so the hub is visited from the lane inward; about 400 x 400 m of hand-built lanes is the core. The 3 km² box catches the whole colony, the Sunday market (name from PIN listing, geolocation unverified) and the Noida Sector 62 edge. The bbox was shifted east after the first pass because the dense core (200+ footprints per 200 m cell) sits at lon 77.333 to 77.346 and is cut off by the first box.

Landmarks (OSM): AEZ Carnival Mall 28.63378, 77.33227; Max Super Speciality Hospital 28.63461, 77.33302; Noida Sector 62 edge 28.63058, 77.33324; Gaur Green Avenue club 28.63446, 77.34191; GDA Park 28.63314, 77.34304; OYO Aman Hotel 28.63187, 77.34502; Om Prem Charitable Hospital 28.63175, 77.34988; R K Medicare 28.62678, 77.34839; Indian Oil 28.62589, 77.33367; Vaishali edge 28.63344, 77.33638. Highway edges: trunk and motorway ways (about 38) run along the north and west.

Fabric (est.): plots 25 to 60 m², 3 to 5 storeys of unplastered brick and RCC frame, open rebar on roofs, 2 to 3.5 m lanes, tangled overhead cabling, exposed meter boards, shopfronts at the ground floor, water tanks. The contrast is the story: gated Noida Sector 62 and Gaur towers (12 to 20 storeys) loom on the northwest horizon of a lane where the family cannot afford 10 percent monthly loans.

Hand vs generated: hand-model the rental building (one room, door, the lane frontage, the neighbour doors and the police tape); generate the colony from Overture with a brick-and-RCC infill generator; Noida towers as simple LOD silhouettes.

### 2.5 Loni (Act 1 arrest, Act 6 dhaba and the tree)

Real: Loni lies at the Delhi-UP border. Two tiles are needed:

**5a: Loni flyover corridor** (28.6980, 77.2840, 28.7210, 77.3020): OSM "Delhi Loni Road Flyover" ways at about 28.7010, 77.2913 and 28.7007, 77.2940 (trunk, 2 lanes each way); Nand Nagari Flyover 28.69962, 77.30170; Loni Gol Chakkar Park 28.70198, 77.29075; Gokulpuri metro (Pink line) 28.70248, 77.28613; Johri Enclave metro 28.71288, 77.29013; Bharat Petroleum 28.70163, 77.28969; Gokulpuri Fire Station 28.70045, 77.29114; Siddharth International Public School 28.70000, 77.29387. This flyover is technically on the Delhi side of the border at Loni Gol Chakkar, but it is the Loni Road flyover people mean by "Loni flyover"; a UP-police arrest here is plausible. This is the arrest set: a flyover, a Fortuner, traffic and a police nakabandi.

**5b: Loni town** (28.7260, 77.2900, 28.7440, 77.3120): Loni town node 28.73173, 77.29009; Naipura Bijli Ghar 28.73649, 77.29674; Chai Point 28.73611, 77.29618; a secondary-road bridge at 28.74187, 77.30864 and more at 28.73647, 77.31934. Proposed (not in OSM): the Loni dhaba on the Loni Road trunk near 28.7370, 77.2960 and the tree with the buried katta at the east fringe (about 28.738, 77.311), where Overture density drops from about 200 per cell to under 80, near fields and the Hindon canal/river bank. Choose a single old pipal or banyan on a bund or canal bank; the tile has no OSM `natural=tree` entries so place it by hand.

Size justification: the arrest is a 60 second non-interactive scene on the flyover plus the drive in; the 4.5 km² of 5a lets the Fortuner approach from Vasundhara along a continuous road corridor. Act 6's 8-month hunt returns to a Loni dhaba (a fight "up close"), the muster-roll clerk, and the tree dig, so 5b is the investigation hub.

Fabric: low-rise dense UP town (2 to 4 storeys, 7,950 + 6,284 footprints), narrow residential grid (843 residential ways in 5b), bare brick, tin-shed workshops (garment and plastics), transport-union yards (important to Act 6: union tattoo and muster roll), dhabas on the trunk road.

Hand vs generated: hand-model the flyover (OSM has it as 5 trunk ways; the bridge deck, barriers, lamp posts, lanes and a police checkpoint), the dhaba, the tree and the dig site. Generate the rest.

### 2.6 Sahibabad (Act 1 operations)

Real: Sahibabad Industrial Area (OSM suburb 28.66585, 77.35115) and Sahibabad railway (platform at 28.67391, 77.36344, Howrah-Delhi main line along the north edge of the tile). Landmarks: Sabzi Mandi 28.66302, 77.35584; Jain Hospital 28.66168, 77.36014; Cocoon Restaurant 28.66196, 77.36051; 8 industrial landuse polygons.

Size justification: the operations side of Act 1 (illegal liquor across the UP border, a transport-union yard, a godown) is a handful of locations connected by roads, mostly driven past or entered in missions. 2.9 km² includes industrial lanes, a mandi, a rail crossing. Treat as a low-detail "mission pocket" tile: build 3 to 4 interiors and generate the rest.

Fabric: factories and godowns (single storey sheds with tin roofs, 1 to 3 storey offices), warehouse yards with truck parking, brick boundary walls with razor wire. OSM 700 buildings.

### 2.7 Dasna jail (exterior only)

Real: Ghaziabad District Jail, known as Dasna Jail. Wikimapia gives it as 28°41'39"N 77°31'59"E (about 28.694, 77.533); Justdial gives a different point (28.6642, 77.4293, probably a geocoding error). The Dasna toll plaza on NH-9 is about 28.685, 77.522. **Unverified**: no OSM feature carries the jail's name or `amenity=prison` here, so confirm against Google 3D Tiles or satellite before placing. The Overture cluster of about 225 footprints centres at about 28.6927, 77.5337.

Size: 1.3 km² is the minimum useful frame. Act 1 ends with "Daya in Dasna jail, counting the bars" (interior not required; "exterior only" per brief) and Act 2 flashes the release gate (nobody there). So this is a gate, a wall, a road and a bus stop.

OSM content: 14 motorway ways (Delhi-Meerut Expressway) and farmland only, so everything is hand-built: perimeter wall (5 to 6 m, concrete, razor wire), watchtowers, gate and visitor shed, the tea stall outside, the NH-9 service road, farmland around.

### 2.8 Karachi: Gulbahar Lines (fictional), based on Lyari and Orangi

Gulbahar Lines is fictional, so the team picks real fabric to base it on. Recommendation: **do not extend the Lyari box; promote a core sub-area of it and add Orangi as a second sub-district.**

- **Core (reuse, do not refetch): Lyari core**, sub-box S 24.8640, W 66.9850, N 24.8800, E 67.0010 (about 2.9 km², estimated roughly 11,000 footprints, ~4,000/km²). It contains Chakiwara (NADRA office 24.86555, 66.99819), Lyari Football Ground (24.87124, 66.99200), Muhammadi Ground (24.87073, 67.01154), a Sindh Rangers post (24.87626, 66.98848), Baloch and Bihar mohallas, Masjid Haji Shah (24.87709, 66.99017), and the existing 300 m slice centred at 24.8752, 66.9944 (789 buildings, mode 3 storeys, range 2 to 6). Eight-by-ten density grid over the existing Lyari Overture: 150 to 960 footprints per 340 x 311 m cell, so the whole box is uniformly dense; no extension is needed.
- **Shrines for Phase 1 ("sleeping outside a shrine")**: a cluster of "مزار" shrine nodes at the north edge of the existing box around 24.8815 to 24.8835, 67.0105 to 67.0125, including Dargah Hazrat Alhaaj Amber Ali Shah Warsi (24.88166, 67.01073) and Shrine of Faizi Baba (24.88117, 67.01372).
- **Water tankers (Phase 1 and the tracking mission)**: Jamila Pumping Station (24.86963, 67.01089, `drinking_water`) is a real node in the box; the tanker economy exists in the real city, so the tracking route can run from here.
- **Hawala shops (Phase 2)**: the old money markets are at Kharadar / Mithadar / Dandia Bazar (24.855 to 24.857, 67.003 to 67.005 in the Lyari box's SE corner) and Kharadar in the existing `karachi_saddar_kharadar` tile. Dandia Bazar (24.85528, 67.0046) and Jama Cloth Market (24.85842, 67.01402) are real nodes.
- **Cricket ground (11-minute meeting, plus the "lose the match convincingly" side mission)**: Kakri Ground, Mithadar (OSM node, 24.8563, 66.99929), a real open ground just inside the south edge of the Lyari box (S edge 24.8547) and adjacent to Kharadar; it is the cheapest choice because the surrounding streets already exist. Alternatives in the box: Muhammadi Ground (24.87073, 67.01154) and Rixer Line Gymkhana ground (24.88672, 67.01551, outside the box's NE corner). For a meeting where two men can plausibly watch a match with a crowd, Kakri is best.
- **Orangi Town (new tile 8)**: Overture 30,935 footprints in 4.0 km² (about 7,700/km², the densest data in the project; no row of the 8 x 8 grid falls below 200 per cell), a regular grid of 120 m² plots in sectors, built up as 1 to 2 storey (some 3) cement-block houses with exposed rebar on roofs. OSM has 704 residential ways and almost no buildings (49). **Relief is the reason to add it**: the Copernicus DSM shows 41 to 105 m of elevation across the tile (mean 56 m) versus 0.5 to 40 m (mean 10 m) across Lyari, so Orangi has hills, nullahs and stepped lanes that Lyari (flat, on the old Lyari riverbed) cannot provide. That matters for the raid: Daya escapes a cordoned neighbourhood on foot, at night, with no weapon, and hillside stairs, nullah crossings and rooftops are the escape verbs. Real names: Orangi Sectors 4, 6, 11, 12-L, 14-C, 15-C (centres at 24.93295, 66.99883 to 24.95689, 66.99613); 10 Number Market (24.94055, 66.99252); Al-Khidmat Hospital Orangi 5 (24.93786, 67.00482); Hilal-e-Ahmer (24.93975, 67.0021); Aik Minar Allah Waali Masjid (24.93152, 66.98551); roundabouts Muhajir Chowk (24.93774, 66.99426), Urdu Chowk (24.9431, 66.98872), Nishan-e-Haider Chowk (24.94722, 66.99403), Chowrangi 5 (24.94054, 67.00133).

Fabric notes (est., from general knowledge of Karachi katchi abadis; verify in 3D tiles): Lyari: 2 to 4 storey, brick and RCC, party walls, 3 to 6 m lanes, tangled wire, hand-painted shop signs, Baloch football murals, wall posters, water-tanker lanes, collapsing old Lyari havelis in Kharadar. Orangi: 1 to 2 storey, concrete block, 6 to 8 m lanes in the planned grid, 3 m lanes in infill, open nullahs, stacks of water drums on the roofs. Material vocabulary to share with the existing `lyari_slice_detailed` textures (damaged plaster, rusted shutter, concrete worn).

Size justification: Act 4 is 6 to 8 hours in one open district, half the game, with no minimap; the player must learn a neighbourhood well enough to draw their own map. A 2.9 km² Lyari core plus a 4 km² Orangi hill district gives about 7 km² of fully built Gulbahar Lines. Fewer than ~3 km² would not support six to eight hours of tracking, runner routes, dead drops and a cordon escape.

Hand vs generated: hand-model the shrine and its forecourt, the chai tea stall and the gambling den (interiors), Sattar's office, the hawala shops (counter, ledger, tokens), the water-tanker yard, Qureshi's tea room, the chalk wall, the mosque (existing Overture class `mosque`/`religious` 97 buildings in Lyari and 35 in Orangi). Generate the mass of housing from Overture via the existing `lyari_prep.py` and `lyari_build.py` pipeline.

### 2.9 Korangi Road Rangers checkpoint (Act 5)

Real corridor: the run from Lyari to Ibrahim Hyderi follows Korangi Road (OSM primary 24.83185, 67.07751; Main Korangi Road 24.84109, 67.05982), then Korangi Creek Road (primary, 3 lanes, e.g. 24.82681, 67.08653 and 24.81907, 67.10416) and Ibrahim Hyderi Road (secondary, 24.8007, 67.16894). No Rangers checkpoint on Korangi Road is mapped in OSM (the nearest mapped ones are PNS Karsaz Checkpost 24.87999, 67.09315 and a Rangers Check Post at 24.94827, 67.09078), so the checkpoint is a **proposed placement**: on Korangi Creek Road near 24.8267, 67.0865, at the edge of Qayyumabad (bus station 24.82942, 67.08077). West of that point the road is dense urban; east (toward the creek) it is industrial scrub and sand. That gives the three player options (talk, bribe, walk): the walk-around is across the scrub.

Size justification: the checkpoint is a single 5 to 10 minute encounter, but a "walk" option needs a believable bypass, so about 1.7 x 1.3 km. Between this tile and Ibrahim Hyderi the player drives a roughly 10 to 15 km corridor; build that as a road-only strip (no buildings, mid/far LOD) along Korangi Creek Road, not as another tile.

Fabric: Qayyumabad/Defence Phase 1 to 2 apartment blocks (OSM tags 69 buildings with levels, mean 2.8, max 27: Center Point 15 floors at 24.83238, 67.08056; Defence Skyline at 24.83138, 67.07949; Empire Boulevard 24.83396, 67.0779; Express Media Group HQ 7 floors 24.83188, 67.07926; Shaukat Khanum 24.82538, 67.07716), plus clinics and shops along the road; residential streets about 6.4 m wide. Overture has `num_floors` on 85 of 1,468 footprints (mean 2.7, max 27).

Hand-model: the checkpoint itself (sandbags, barrier, concrete blocks, Rangers pickup, tin shed, floodlight, Pakistan flag), nothing else.

### 2.10 Ibrahim Hyderi jetty and creek (Act 5 climax)

Real: Ibrahim Hyderi is a Sindhi/Baloch fishing village at the head of Korangi Creek, east of Karachi. OSM:

| Feature | lat, lon |
|---|---|
| Ibrahim Haidery Port (`leisure=marina`) | 24.78519, 67.14287 |
| Ibrahim Haidery Fishry (`shop=seafood`) | 24.78451, 67.14098 |
| Slipways (2) | 24.78205, 67.13814 (36 m) and 24.78139, 67.13751 (41 m) |
| Korangi Creek heliport (PAF) | 24.78220, 67.13640 |
| "Korangi Creek 01" (`leisure=fishing`) | 24.77842, 67.15550 |
| Tidal channel (کورنگی کریک) | 24.77546, 67.15756 |
| Ibrahim Hyderi bus stop | 24.79754, 67.13589 |
| Football stadium; Balouch Mujahid soccer ground | 24.79371, 67.14825; 24.79169, 67.13831 |
| Fatma masjid (Ibrahim Tail wali) | 24.78664, 67.14126 |
| Graveyard | 24.79526, 67.13772 |
| Salt pond | 24.79921, 67.15819 |
| Other fishing: Korangi Fisheries Harbour / NBP Fish Harbour (east, outside the tile) | 24.80537, 67.20206 / 24.80121, 67.20140 |

**Coast and tide geometry** (the Act 5 physics: a 20-minute real-time window, a 25 km/h boat against a 4 km/h ebb, no turning back): OSM `natural=coastline` runs west to east along the village waterfront (a single 17.9 km way from 24.7789, 67.1284 to 24.7883, 67.1815), so the shore is a straight east-west line at about lat 24.785 with water to the south. Copernicus DSM over the tile: 55% of the area at or below 0.5 m (water and tidal flats; DSM reads 0.0 over water), a min of -5.3 m and a max of 26.3 m (the high values are the PAF Airmen Academy buildings and trees on the north shore, a DSM artefact, not terrain). The southern 1.5 km is flat open water and mangrove/tidal channel, with the exposed mud at low tide. Karachi's tide is semi-diurnal with a spring range of roughly 2.5 to 3 m (est.; verify against Pakistan Navy Hydrography tide tables, which is the right source for the exact time of the ebb the story needs). The creek runs south and east before reaching the open sea, so the actual distance from jetty to open water is likely about 10 km or more (est.; measure along the OSM coastline and the Copernicus 30 m DEM before fixing the 20-minute window). The Act 5 boat should be modelled in the water-surface plane with a tide level driver (±1.3 m) rather than baked.

Size justification: the climax is a 20-minute timed sequence from the road end to the jetty to the boat leaving, with Qureshi's men arriving 8 minutes after, plus Ravi shot on the shore and the receding view. The village needs about 1.5 km of waterfront for a believable night run (alleys, boat yards, ice factory, a chokepoint at the jetty), and the camera must see the shore recede from the water for a few kilometres. Hence 9.4 km² where about 40% is low-LOD water. If you need to trim, cut the east third (footballs, salt pond).

Fabric (est.): lanes 2 to 4 m; 1 to 2 storey concrete-block houses; wooden-and-tin shacks and net-drying yards along the waterfront; ice and fish-processing sheds; rows of wooden dhows (launches and trawlers) pulled up on the mud; diesel drums, ropes, nets; mosque minarets and green banners. Overture: 7,459 footprints (cell density 200 to 450 in the dense core around lon 67.138 to 67.150), classes mostly unclassified; 12 schools and 4 religious.

Hand-modelled: jetty (wood-and-concrete pier, bollards, tyres as fenders), boats (6 to 10 fishing dhows and trawlers, one hero boat with a 40 HP outboard, radio), the shoreline mud and mangroves, the ice shed, Ravi's cover position, lighting rig for night. Generate: village housing from Overture, the creek surface from the coastline and DSM.

### 2.11 Dehradun farmhouse (Act 3)

Pick: the rural Doon valley fringe between Sahaspur and the Tons river, about 20 km west of Dehradun city (30.3165, 78.0322; box centre 30.370, 77.822). Chosen because it is flat valley-floor farmland at 476 to 510 m (Copernicus), with scattered farmhouses and mango/sal groves, a river, and the Shivalik foothills close to the north; it is a plausible place for a rented farmhouse where a foreign trainee lives for four months without attracting attention. OSM nodes nearby: Sahaspur (village, 30.3923, 77.8079), Hasanpur (hamlet, 30.3436, 77.8414), Tons Nadi at 30.3862, 77.8210, CHC Sahaspur (30.3874, 77.8121).

Size justification: Act 3 is 60 to 90 minutes with no gym montage; the training spaces are the farmhouse yard, a barn, a field and the road to the market. The 2.7 km² tile is far more than the farm needs (a 300 m compound); the point is to have a believable approach and horizon. OSM in the tile is nearly empty (5 roads, 0 buildings); Overture has 721 scattered footprints, with clusters in the north half.

Fabric: farmhouse compound with one single-storey main house (brick, tin or asbestos roof), a courtyard, a cow shed, a tube well, a water tank; walled mango grove; kachcha tracks; sugarcane and wheat fields; Himalayan foothills as skybox. Hand-model the compound (house, yard, tube well, barn); everything else is terrain plus vegetation.

### 2.12 Mamdot, Punjab (reference only)

Real: Mamdot village, Ferozepur district, OSM 30.86956, 74.42281 (village), 30.8697, 74.4206 (CHC Mamdot), Mamdot Hithar 30.87224, 74.39453. Overture 1,554 footprints; OSM 4 buildings and 66 roads. Hospitals: Guru Nanak Dev Hospital 30.86932, 74.41852; Sindhi Hospital 30.86992, 74.41968; Wadhwa Hospital 30.87043, 74.41988. The Mamdot gurdwara named in the coda is not tagged in OSM; locate it from satellite imagery. The border is about 6 km away per the bible (not re-measured).

Purpose: reference only. Daya's origin is mentioned in dialogue and the coda ("the pension goes to the gurdwara in Mamdot"); no gameplay occurs here. Use the tile for a matte-painting or a single-shot cutscene, if at all. No blockout needed (4 buildings built).

## 3. Data produced

All in `C:\Users\Rayaan\Desktop\game\world\story\source\` (Overture GeoJSON and OSM XML per tile, plus two DEM tiles):

| Tile | OSM XML | Overture GeoJSON (features) | OSM size |
|---|---|---|---|
| nizamuddin | `nizamuddin.osm` | `nizamuddin_overture_buildings.geojson` (2,378) | 41 MB |
| sarai_kale_khan | `sarai_kale_khan.osm` | `sarai_kale_khan_overture_buildings.geojson` (1,676) | 39 MB |
| vasundhara | `vasundhara.osm` | `vasundhara_overture_buildings.geojson` (3,879) | 3.2 MB |
| khoda_colony | `khoda_colony.osm` | `khoda_colony_overture_buildings.geojson` (5,939) | 32 MB |
| loni_flyover | `loni_flyover.osm` | `loni_flyover_overture_buildings.geojson` (7,950) | 33 MB |
| loni_town | `loni_town.osm` | `loni_town_overture_buildings.geojson` (6,284) | 1.3 MB |
| sahibabad | `sahibabad.osm` | `sahibabad_overture_buildings.geojson` (3,312) | 22 MB |
| dasna_jail | `dasna_jail.osm` | `dasna_jail_overture_buildings.geojson` (225) | 5.3 MB |
| karachi_orangi | `karachi_orangi.osm` | `karachi_orangi_overture_buildings.geojson` (30,935) | 1.1 MB |
| karachi_korangi_checkpoint | `karachi_korangi_checkpoint.osm` | `karachi_korangi_checkpoint_overture_buildings.geojson` (1,468) | 1.2 MB |
| karachi_ibrahim_hyderi | `karachi_ibrahim_hyderi.osm` | `karachi_ibrahim_hyderi_overture_buildings.geojson` (7,459) | 1.5 MB |
| dehradun_farm | `dehradun_farm.osm` | `dehradun_farm_overture_buildings.geojson` (721) | 1.1 MB |
| mamdot | `mamdot.osm` | `mamdot_overture_buildings.geojson` (1,554) | 0.3 MB |

DEM (Copernicus GLO-30): `Copernicus_DSM_COG_10_N24_00_E067_00_DEM.tif` (Karachi coast: Korangi, Ibrahim Hyderi; the Orangi and Lyari boxes sit in the existing `world/karachi/source/...N24_E066` tile), `Copernicus_DSM_COG_10_N30_00_E077_00_DEM.tif` (Dehradun). The Ghaziabad/Delhi tiles (Nizamuddin, Vasundhara, Khoda, Loni, Sahibabad, Dasna, all lon below 78) are covered by the existing `world/delhi/source/Copernicus_DSM_COG_10_N28_00_E077_00_DEM.tif`; Mamdot would need `N30 E074` (not fetched, reference only). Also `_tiles_bbox.csv` (final bboxes) and `_blockout_log.txt`.

Blockouts (Blender 5.2.3 headless, `world/mumbai/scripts/osm_to_blender.py` unmodified, `.blend` + `.fbx` in `C:\Users\Rayaan\Desktop\game\world\story\`). "BUILT" lines (OSM only; see caveat at top):

| Blockout | BUILT |
|---|---|
| nizamuddin_blockout | BUILT 1047 buildings; area 1662 x 1336 m |
| sarai_kale_khan_blockout | BUILT 312 buildings; area 1564 x 1336 m |
| vasundhara_blockout | BUILT 1792 buildings; area 1661 x 1670 m |
| khoda_colony_blockout | BUILT 145 buildings; area 1954 x 1558 m |
| loni_flyover_blockout | BUILT 182 buildings; area 1757 x 2560 m |
| loni_town_blockout | BUILT 99 buildings; area 2147 x 2004 m |
| sahibabad_blockout | BUILT 733 buildings; area 1856 x 1558 m |
| dasna_jail_blockout | BUILT 0 buildings; area 1172 x 1113 m |
| karachi_orangi_blockout | BUILT 50 buildings; area 2019 x 2004 m |
| karachi_korangi_checkpoint_blockout | BUILT 218 buildings; area 1718 x 1336 m |
| karachi_ibrahim_hyderi_blockout | BUILT 42 buildings; area 3032 x 3117 m |
| dehradun_farm_blockout | BUILT 0 buildings; area 1633 x 1670 m |
| mamdot_blockout | BUILT 4 buildings; area 1433 x 1447 m |

`_blockout_log.txt` also contains the superseded first-pass runs for khoda_colony (117, old bbox) and karachi_ibrahim_hyderi (39, old bbox); the later 145 and 42 lines are the final ones.

Notes and caveats:
- Overpass `nwr` + recursion pulls relation members far outside the bbox, so some `.osm` files are 30 to 40 MB (same effect as in the south-Delhi report); clip to the bbox downstream. The blockout script already clips.
- Overpass returned several 502/504 responses; the fetch helper retried until each file ended in `</osm>`. All files verified.
- Boxes that changed after the first pass (data re-fetched, superseded files overwritten): Khoda Colony shifted east (old W 77.327 to 77.345 cut the dense core; Overture went from 4,862 to 5,939 footprints) and Ibrahim Hyderi extended south to include the waterfront (old S 24.787 stopped at the shore; Overture 10,364 reduced to 7,459 since the box now has more water).
- Overture `num_floors`/`height` are essentially empty: all heights are inferred.
- Intermediate `.state` files and Blender `.blend1` backups were deleted.

## 4. Recommended build order, tied to the acts

Act 4 (Karachi) is half the game, so its pipeline has the largest content volume and the most risk; the Delhi/Ghaziabad locations are small and cheap by comparison but the hand-modelled hero spaces (the station, the balcony) are long-lead. Build in this order:

1. **Gulbahar Lines core: Lyari core sub-box (existing data) + Kakri Ground + the shrine at the north edge (Act 4, about 7 h of play).** Start immediately; the data, the slice pipeline (`lyari_prep.py`, `lyari_build.py`) and the textures already exist. Deliver one walkable 1 km² first (Chakiwara to the football ground), then expand to the 2.9 km² core. Establish the housing generator and texture set that every other low-income tile reuses.
2. **Hazrat Nizamuddin (Act 2 core loop, 2 to 3 h).** Start in parallel with step 1 because the station is a long-lead hand-modelled set (platforms 1 to 7, two footbridges, the subway, the forecourt) and its mapping in OSM is excellent. Sarai Kale Khan (tile 2) can follow right after it with only a bus terminal and a dhaba added, because the Act 3 pitch is short.
3. **Khoda Colony + Vasundhara (Acts 1, 2, 5 and 6 hubs: used 3 times).** Vasundhara is the Act 1 start of the game, so the first playable build must include at least the Daya house, the Sector 5 street and the Fortuner drive to the Loni flyover; Khoda is revisited in Acts 2, 5 and 6. Build both hubs to a state with reusable interiors, and use the variants (cleaner and richer in Act 1, poorer in Act 2, police tape in Act 6) as dressing changes, not new levels. The balcony set belongs here (Act 6 finale, plus a 12 m drop column).
4. **Loni (flyover then town) + Sahibabad (Act 1, then Act 6 hunt).** Loni flyover for the Act 1 arrest (a scripted non-interactive scene, so needs only the flyover set and 300 m of road), Loni town for the Act 6 investigation, the dhaba fight and the tree. Sahibabad is mission pockets and can be a minimal set: a godown, a yard and the road.
5. **Orangi sub-district (Act 4 Phases 2 and 3 and the raid).** After the Lyari core is playable, add Orangi for the hillside escape set piece. It reuses the Lyari generator with a different building set (lower, bare block) and a terrain pass from the Copernicus DEM.
6. **Korangi checkpoint + Ibrahim Hyderi + the corridor (Act 5, 90 minutes).** Late in the schedule because it needs the boat and tide systems and night lighting, but it is the emotional climax of Part 1's payoff, so allow a full milestone. Build the road corridor as a low-LOD strip.
7. **Dehradun farm (Act 3).** Small (one compound) and self-contained; build it any time as a filler task.
8. **Dasna jail** (exterior only: gate, wall, road) and **Mamdot** (reference; matte or skip). Last.

Dependencies: the housing generator (steps 1, 3, 5) and the hand-modelled hero set list (station, balcony, jetty, jetty boats, flyover) should be scheduled separately from the generated tile work, since the generated tiles are bulk and the hero sets are the visible quality bar.

## 5. Hand-modelled vs generated: summary

| Location | Hand-model | Generate |
|---|---|---|
| Nizamuddin | Station, 4 platform islands with canopies, tracks and signals, FOB 1 and 2, subway, forecourt, ticket windows, chai stall, Nila Gumbad, Dargah | Basti, colonies, roads, trees |
| Sarai Kale Khan | ISBT sheds and bays, dhaba, RRTS construction piers | Surroundings, roads |
| Vasundhara | Daya's 3-storey house (full interior), Bablu's 4th-floor flat and balcony (90 cm rail), pavement, a few corner shops | Sector housing, roads, parks |
| Khoda Colony | The rental room, lane frontage, tape, neighbours' doors | Colony fabric, towers as LOD silhouettes |
| Loni | Flyover deck and barriers, police nakabandi, dhaba, the tree and dig site, transport-union yard | Town fabric |
| Sahibabad | A godown, a yard, rail crossing | Industrial sheds, roads |
| Dasna jail | Gate, perimeter wall, towers, visitor shed, road | Farmland |
| Gulbahar Lines (Lyari + Orangi) | Shrine forecourt, tea stall, gambling den, Sattar's office, hawala shops, tanker yard, Qureshi's tea room, chalk wall | Housing via Overture generator, roads, terrain (Orangi) |
| Korangi checkpoint | The checkpoint (sandbags, barrier, pickup, shed) | Road corridor, urban edge |
| Ibrahim Hyderi | Jetty, 6 to 10 boats incl. a hero boat, ice shed, shore mud and mangroves, night lighting, tide driver | Village housing, creek surface |
| Dehradun farm | Farmhouse compound | Terrain, fields, vegetation |
| Mamdot | None (reference) | None |

## 6. Sources

- OSM data: Overpass API https://overpass-api.de/api/interpreter (full-bbox `nwr` plus recursion queries, plus the per-location name queries described above); all POI coordinates are OSM tags read from the downloaded files unless marked proposed or est.
- Overture Maps buildings: https://docs.overturemaps.org/ (CLI `overturemaps download --type=building`)
- Copernicus GLO-30 DEM: https://copernicus-dem-30m.s3.amazonaws.com/ (tiles N24 E067 and N30 E077 downloaded; elevations are DSM, including buildings and trees)
- Dasna jail location: https://wikimapia.org/7234959/Ghaziabad-District-Jail ; https://www.justdial.com/Ghaziabad/Dasna-Jail-Dasna-Gate/011PXX11-XX11-110318154858-F4Z9_BZDET ; https://en.wikipedia.org/wiki/Dasna (the two sources disagree by about 6 km; Wikimapia chosen; verify)
- Khoda/Khora Colony location: https://maps.apple.com/place?auid=13807506313024166559 (28.62787, 77.33530) ; https://ghaziabad.nic.in/en/public-utility/nagar-palika-parishad-khoda-makanpur/ ; https://en.wikipedia.org/wiki/Khora
- Sahibabad overview: https://en.wikipedia.org/wiki/Sahibabad ; Sajwan Nagar: https://en.wikipedia.org/wiki/Sajwan_Nagar
- Storyline: `game-design-docs/story/storyline-bible.md`; earlier report format: `game-design-docs/research/01-south-delhi-maps.md`.

Verification status: station, platforms, footbridges, subway entrances, Rangers posts and fishing-port features, Lyari/Orangi landmarks, and Ibrahim Hyderi coastline are OSM-verified. Not verified (no authoritative source found): Dasna jail's exact position, all building heights and street widths (est.), the tide range and the creek-to-sea distance, the Mamdot gurdwara, any Sunday-market position in Khoda, and all fictional placements marked "proposed" (Daya's house, Bablu's flat, the Loni dhaba and tree, the Korangi checkpoint, the cricket meeting, Gulbahar Lines' internal layout).
