# South / Central Delhi districts: map scoping and data (Phase 2 candidates)

Data fetched 2026-11-10 into `world/delhi_south/source/`. All coordinates below are from OSM (Overpass) unless marked "est."; building counts are measured. Fabric numbers marked *est.* are estimates from general knowledge of Delhi colony planning, not measured: verify in-engine against Cesium/Google 3D Tiles.

## 1. Districts table

Bbox order is S, W, N, E (lat/lon). Overture bbox for the CLI is W,S,E,N. Boxes overlap slightly at the edges (GK-1/GK-2 at lat 28.542; Lajpat/Defence Colony at lon 77.233-77.237), which is intentional so tiles can be stitched.

| # | District | S, W, N, E | Size (m) | Area km² | Overture bldgs | OSM bldg ways | OSM mean levels (tagged) | Blockout "BUILT" |
|---|---|---|---|---|---|---|---|---|
| 1 | Greater Kailash I (incl. Kailash Colony, E/N/M blocks) | 28.5425, 77.2285, 28.5585, 77.2440 | 1516 x 1781 | 2.70 | 3,519 | 2,668 | 4.5 (379) | 2,720 |
| 2 | Greater Kailash II (incl. western edge of Chittaranjan Park) | 28.5255, 77.2345, 28.5420, 77.2500 | 1516 x 1837 | 2.78 | 3,509 | 2,337 | 4.4 (11) | 2,367 |
| 3 | Lajpat Nagar I-IV + Central Market | 28.5580, 77.2330, 28.5750, 77.2500 | 1662 x 1892 | 3.14 | 4,466 | 3,204 | 4.5 (201) | 3,261 |
| 4 | Sarojini Nagar Market + govt colony | 28.5680, 77.1890, 28.5800, 77.2040 | 1466 x 1336 | 1.96 | 1,679 | 776 | 6.9 (171) | 823 |
| 5 | Hauz Khas Village (+ IIT Delhi gate, Deer Park edge) | 28.5470, 77.1880, 28.5580, 77.2020 | 1369 x 1225 | 1.68 | 645 | 352 | 4.0 (132) | 386 |
| 6 | Khan Market (+ Lodhi Estate, Sikandar Lodi tomb) | 28.5940, 77.2170, 28.6080, 77.2350 | 1759 x 1558 | 2.74 | 1,447 | 661 | 3.8 (10) | 691 |
| 7 | Nehru Place (+ Lotus Temple, Kalkaji Mandir metro) | 28.5430, 77.2470, 28.5540, 77.2600 | 1271 x 1225 | 1.56 | 1,455 | 884 | 7.9 (71) | 906 |
| 8 | Defence Colony | 28.5650, 77.2260, 28.5790, 77.2370 | 1075 x 1558 | 1.67 | 1,855 | 1,021 | 4.3 (3) | 1,063 |
| 9 | INA Market / Dilli Haat / AIIMS-Safdarjung | 28.5680, 77.2040, 28.5780, 77.2160 | 1173 x 1113 | 1.31 | 928 | 490 | 6.0 (182) | 521 |

Totals: 19,500 Overture buildings, ~22.2 km² across nine tiles. Overture is richer than OSM everywhere (it merges Google Open Buildings and MS footprints), so use Overture as the footprint source and OSM for roads, POIs, landuse and the few height tags. Overture `num_floors` is populated for only 3-11% of GK-2/Defence/Khan, ~11% for GK-1 and Lajpat, so heights must be inferred (see section 3). Hauz Khas Village is the exception: Overture has `height` on 182 of 645 footprints.

Why these bboxes: GK-1 is centred on the OSM "Greater Kailash I" node (28.5513, 77.2360); GK-2 on "Greater Kailash II" (28.5338, 77.2425); Lajpat on the Lajpat Nagar suburb node (28.566, 77.243) pushed west to include Central Market (28.569, 77.241) and Defence Colony-facing edge; Sarojini on the "Sarojini Nagar" suburb node (28.5745, 77.1963). Caveat: the OSM "Sarojini Nagar Market" node at 28.5798, 77.1967 sits on the tile's north edge and is probably misplaced; the real market lanes are around Babu Market (28.5777, 77.1948) and the Metro gates (28.5759-28.5764, 77.1972).

## 2. Landmarks per district

### 2.1 Greater Kailash I (GK-1)

| Category | Name | lat, lon | Visual notes |
|---|---|---|---|
| Market | M-Block Market (the main GK-1 market) | 28.55011, 77.23541 | 1970s two-storey arcade shops around a central parking lot, ~200 shops/restaurants, pavement vendors, closed Tuesdays. Painted facades, dense signboards on first-floor fascia. |
| Market | N-Block Market | 28.55652, 77.23312 | Smaller neighbourhood market, north edge near the police station. |
| Market | Zamrudpur Market | 28.55688, 77.23686 | Village-origin retail strip. |
| Market | Kailash Colony Market / E-Block market | 28.5531, 77.2414 / 28.5570, 77.2430 | Eastern commercial edge by the metro. |
| Metro | Kailash Colony (Violet Line), gates 1-2 | 28.55533, 77.24196 | Elevated station straddling the road at the NE corner. |
| Hospital | SCI International Hospital / SCI IVF | 28.54986, 77.23400 / 28.54952, 77.23378 | Glass-fronted 5-6 storey medical blocks beside M-Block. |
| Hospital | Apollo Spectra; Indian Cancer Institute | 28.55559, 77.24124; 28.54838, 77.24197 | Mid-rise commercial. |
| Worship | Gurudwara Pahariwala | 28.55209, 77.23840 | White/marble gurdwara with gold-topped domes. |
| Worship | Chaar Sahibzaade Gurudwara; Shri Durga Mandir; Shiv Shakti Mandir | 28.55573, 77.23905; 28.55626, 77.23943; 28.55313, 77.24089 | Small neighbourhood temples. |
| Monument | Zamrudpur Tomb (Lodi/Mughal era, in a park) | 28.55437, 77.23393 | Single heritage tomb in an otherwise modern grid. |
| Police | Police Station GK-1 (two OSM nodes) | 28.55441, 77.23221; 28.54641, 77.24308 | Second node is likely the CR Park/GK-1 boundary post. |
| Schools | Mata Gujri Public School; The Study School; Oxford Kids | 28.55144, 77.23780; 28.55727, 77.24360; 28.55639, 77.24324 | |
| Parks | Kailash Colony Park, Ganga/Yamuna/Saraswati/Triveni parks (E-Block), E-block Basketball park, R-block park | 28.5532, 77.2416; 28.5561-28.5579, 77.2413-77.2437; 28.5508, 77.2412; 28.5470, 77.2426 | Mature neem, ashoka, gulmohar; each block has a central park. |
| Food | Known M-Block food names (Pind Balluchi, Sagar Ratna, Nathu's, Kakke Da Hotel, Big Chill, etc. per web listings) | around 28.5501, 77.2354 | Use fictional brand renames per the fiction policy. |

### 2.2 Greater Kailash II (GK-2)

| Category | Name | lat, lon | Visual notes |
|---|---|---|---|
| Market | M-Block Market GK-2 (restaurant/boutique strip, "Meghdoot" style complex) | 28.53340, 77.24289 | Two-level 1970s-80s market, now boutiques, cafes, galleries. More upmarket, less pavement-vendor clutter than GK-1. |
| Market | Chandan Market; Market 1 and Market 4 (CR Park) | 28.53073, 77.24600; 28.54028, 77.24863; 28.53734, 77.24851 | Bengali-colony markets with fish stalls; "Fish Market" at 28.54031, 77.24836. |
| Metro | Greater Kailash (Magenta Line), Gate 1 | 28.54191, 77.23849 | Underground station, on the Outer Ring Road side. |
| Hospital | Fortis Bloom; Netrayatan Hospital; Ariston Multispeciality; Talwar Medical Centre | 28.52878, 77.24379; 28.52998, 77.24519; 28.53799, 77.24826; 28.53639, 77.24417 | |
| Worship | Gurudwara Guru Singh Sabha | 28.54017, 77.24141 | |
| Worship | Kali Bari Mandir / Kali Mandir | 28.53681, 77.24753 | CR Park Bengali temple; Durga Puja ground at 28.54022, 77.24503 is a festival set piece. |
| Worship | Sanatan Dharam Mandir; Arya Samaj; Masjid Moth | 28.54045, 77.23862; 28.5366, 77.2455; 28.54156, 77.23548 | Masjid Moth is a Lodi-period mosque (heritage). |
| Cinema | DT Cinema | 28.54141, 77.24006 | Mall cinema at the metro. |
| Police | CR Park Police Station | 28.53673, 77.24934 | |
| Schools | Don Bosco; Kumar Mangalam; Balvantray Mehta Vidya Bhawan; St John's | 28.52851, 77.24733; 28.53244, 77.24823; 28.5351, 77.23585; 28.53704, 77.23541 | |
| Parks | Central Park, Rock Garden, S-Block park, I-Block park (CR Park) | 28.52991, 77.24899; 28.53127, 77.24607; 28.53161, 77.24219; 28.53801, 77.24980 | |

### 2.3 Lajpat Nagar (I-IV) incl. Central Market

| Category | Name | lat, lon | Visual notes |
|---|---|---|---|
| Market | Lajpat Nagar Central Market | 28.56899, 77.24083 (OSM also marks 28.56892, 77.24216) | Pedestrianised-feeling grid of 3-5 storey shop blocks, wall-to-wall signage, ~10,000 daily footfall (per web), garments/textiles/jewellery/street food. Hero area. |
| Market | Pushpa Market; Export Market; Gupta Market; Shagun Dept. Store | 28.56929, 77.24155; 28.56936, 77.23970; 28.56509, 77.23961; 28.56392, 77.24197 | Sub-markets feeding Central Market. |
| Metro | Lajpat Nagar (Pink Line) | 28.56958, 77.23851 | |
| Metro | Lajpat Nagar (Violet Line) | 28.57060, 77.23640 | Elevated over Ring Road, interchange with Pink. |
| Metro | Moolchand (Violet) | 28.56440, 77.23426 | |
| Metro | Vinobapuri (Magenta/Pink) | 28.56587, 77.24930 | East edge. |
| Hospital | Moolchand Khairati Ram Hospital | 28.56632, 77.23522 | Large 8-10 storey hospital block, Ring Road. |
| Hospital | Metro Hospital and Heart Institute; Institute of Brain and Spine; Lajpat Nagar Colony Hospital; GAMCA | 28.56472, 77.24379; 28.5656, 77.24425; 28.56895, 77.24506; 28.57032, 77.23694 | |
| Mall/cinema | PVR 3C's (3C's Mall) | 28.56770, 77.24102 | Landmark mall beside Central Market. |
| Worship | Satsang Guru Nanak Darbar (gurdwara) x2; Sindhi Sat Sangat Darbar; Shri Sanatan Dharm Laxmi Narayan Mandir; Jandi Mandir; Prachin Shiv Hanuman Mandir; Mata Ka Mandir | 28.56716, 28.56825; 28.56839; 28.56681; 28.57131; 28.57175; 28.56582 (lon 77.2402-77.2480) | Sindhi/Punjabi refugee-era devotional spaces. |
| Heritage | Langer Khan's Tomb Gate (Lodi/Mughal gate) | 28.56077, 77.23465 | Rare old gateway embedded in the grid. |
| Police | Pink Booth; Lajpat Nagar police posts | 28.56817, 77.24199 | Main stations lie just outside (Amar Colony). |
| Schools | Kendriya Vidyalaya; Frank Anthony Public School; Lady Shri Ram College (LSR); SS Khalsa Sr Sec | 28.5606, 77.2331; 28.56089, 77.23569; 28.55967, 77.23829; 28.56215, 77.24126 | |
| Parks | Lala Lajpat Rai Memorial Park; Family Park; I/J/L Block parks; N Block Park | 28.57011, 77.24837; 28.56149, 77.24140; 28.5658-28.5668, 77.2412-77.2435; 28.55910, 77.23373 | Small block parks, 30-60 m wide. |
| Food | Central Market street food (chaat, momos, chole bhature, Moti Mahal-style legacy joints per web) | 28.5690, 77.2410 | |

### 2.4 Sarojini Nagar Market

| Category | Name | lat, lon | Visual notes |
|---|---|---|---|
| Market | Sarojini Nagar Market lanes (Babu Market, Subzi Mandi, Central Market, Mandir wali side) | 28.57774, 77.19476 (Babu Market); Market gates 28.57588, 77.19722 and 28.57637, 77.19720 | Lanes 3-5 m wide, packed with tarpaulin stalls, hangers, mannequins; evenings "hundreds of street vendors". Dense sound/colour: export surplus, garments, footwear. |
| Memorial | 2005 Delhi Bomb Blasts memorial | 28.57741, 77.19519 | Sensitive: handle with care (mark as memorial, not a play space). |
| Metro | Sarojini Nagar (Pink Line) | 28.57582, 77.19808 | |
| Hospital/health | M.C.W. Centre NDMC Dispensary; CGHS dispensary L-Block; ESI dispensary | 28.57529, 77.20055; 28.57585, 77.19133; 28.57518, 77.19068 | |
| Worship | Pracheen Hanuman Mandir; Sri Sanatan Dharm Mandir; Shri Hanuman Mandir; Vinayak Mandir; Arya Samaj Mandir | 28.57572, 77.19637; 28.57388, 77.19275; 28.57325, 77.19365; 28.57139, 77.19931; 28.57732, 77.20262 | |
| Police | Police Booth; Facilitation Kiosk | 28.57570, 77.19524; 28.57585, 77.19737 | |
| Schools | Sarvodaya Kanya Vidyalaya No.1; Bharat Sevak Samaj Public School; NDMC School | 28.57481, 77.19655; 28.57667, 77.19493; 28.57717, 77.19477 | |
| Parks | Keshav Park; RBI Park | 28.57696, 77.19549; 28.57753, 77.19217 | |
| Library | Delhi Public Library | 28.57587, 77.19481 | |
| Nearby | Safdarjung Airport / Hyatt, Bhikaji Cama Place just west | 28.570, 77.186 | Outside tile. |

### 2.5 Extras (recommended additions)

**Hauz Khas Village** (28.5536, 77.1948 suburb node). Why: the most visually unique location in the set: a 14th-century Tughlaq complex (Feroz Shah's tomb 28.55302, 77.19342; madrasa wings 28.55318/28.55363, 77.1931-77.1937; reservoir tank) beside a dense 3-4 storey nightlife village; Deer Park next door. Also tonal contrast: ruins, lake, neon bars. Other points: Munda Gumbad 28.55662, 77.19170; Bagh-i-Alam ka Gumbad 28.55660, 77.19757; Chhoti/Sakri Gumti 28.5532, 77.2013; IIT Delhi metro 28.54701, 77.19455; Jagannath Temple 28.55336, 77.20002; St Mary Orthodox Cathedral 28.55024, 77.20043. Smallest data load (645 buildings).

**Khan Market** (Khan Market metro 28.60204, 77.22918; market ~28.6006, 77.2268). Why: Delhi's famous elite bookstore/cafe market (built 1951 for Partition refugees, two-storey arcades with lane parking) in the middle of Lodhi Colony's bungalow zone; strongly different from the commercial chaos of Lajpat/Sarojini, giving the player an "old money" Delhi. Adjacent Lodhi Garden/Sikandar Lodi tomb (28.59630, 77.22154), Rose Garden 28.59463, 77.22179, Pandara Market 28.60652, 77.22936, Judah Hyam synagogue 28.60328, 77.22569, Taj Mahal Hotel 28.60490, 77.22360, Lycée Français 28.60315, 77.22150. Low-rise, big trees, wide streets.

**Nehru Place** (Metro 28.55142, 77.25174). Why: Asia's biggest IT hardware market, a 1970s concrete plaza-and-tower modernist district (mean 7.9 storeys, max 19 in OSM) that anchors the "commercial high-rise" layer of South Delhi and sits 800 m from the **Lotus Temple** (28.55336, 77.25861) and **Kalkaji Mandir** metro/temple area (28.5495, 77.2591). Other points: Satyam Cineplex 28.54997, 77.25095; INOX 28.54990, 77.25126; Nehru Place PS 28.54968, 77.25537; Fortis C-DOC 28.54699, 77.24949; Eros hotel 28.55013, 77.24915; Computers Temple 28.54856, 77.25172; Acer Mall 28.54879, 77.25135.

**Defence Colony** (market 28.57351, 77.23031). Why: pairs with Lajpat Nagar (shares its west border) and is the canonical 1950s upper-class planned colony: 2-3 storey bungalows converting to 4-storey builder floors, DefCol Club 28.57173, 77.23321, Shri Sanatan Dharam Sabha 28.57274, 77.23217, Gurdwara Guru Singh Sabha 28.57305, 77.23235 and St Luke's Church 28.56987, 77.22730. Also contains Moolchand hospital (shared) and Oncoplus. Cheap to add as it is the missing link between Lajpat and INA/Sunder Nagar.

**INA Market / Dilli Haat / AIIMS** (Dilli Haat 28.57326, 77.20771; INA Market 28.57419, 77.21020). Why: Dilli Haat (open-air craft bazaar with brick-and-sandstone stalls, ticketed) is Delhi's tourist craft icon; INA Market is the spice/imported-grocery market; the INA metro is the Pink/Yellow interchange and is a major navigation hub. Includes VMMC & Safdarjung Hospital (28.56867, 77.20673) and the Kalam memorial 28.57330, 77.20590. Mean 6.0 storeys, tallest cluster 16. Adjacent to Sarojini Nagar (1 km west, same Pink Line), so the two stitch into one mid-sized district.

Other candidates considered (not fetched): Connaught Place (already blocked out in `world/delhi/`), Safdarjung Enclave/Green Park, Saket/Select Citywalk, Chittaranjan Park (partly covered by the GK-2 tile's east edge), Qutub/Mehrauli.

## 3. Character of the built fabric

Numbers from OSM/Overture: measured. Others *est.*: derived from typical DDA/private colony planning (plot sizes by category) and field-observation knowledge.

| District | Typology | Storeys | Plots / setbacks (*est.*) | Streets (*est.*) | Trees / greens |
|---|---|---|---|---|---|
| GK-1 | Planned private colony (DLF), 1960s-70s. Original 2-storey bungalows rebuilt since 2000s as 4-storey "builder floors" (stilt/ground parking + 3 floors), plus a few 6-7 storey blocks (OSM max 7). Gated sections with boom barriers on E-, N-, R-blocks. | OSM mean 4.5 | Plots 200-500 m² (typical 250-350), front setback 3-5 m, side 1.5-2 m; most 15-20 m frontage | Block roads 7-9 m carriageway in 12-15 m ROW; ring of 20-30 m arterials (Lala Lajpat Rai Marg / Outer Ring) | Central block parks, avenue neem/gulmohar, ~25-30% canopy |
| GK-2 | Same kind, 1960s-70s, larger plots, many more 4-6 storey rebuilds; adjacent CR Park is a 1970s Bengali refugee colony with 3-4 storey plain-plastered housing, shops under flats | OSM mean 4.4 (sparse tags) | Plots 250-500 m², lower density than GK-1 in the west; CR Park plots 60-120 m² | 7-9 m block roads; CR Park 5-6 m lanes | Many mature trees; Central Park, Rock Garden |
| Lajpat Nagar | Refugee colony (1950s, Karachi/Sindh Punjabis), plots 60-150 m² rebuilt as 4-5 storey mixed-use with shops on the ground floor; Central Market is a continuous commercial wall | OSM mean 4.5, Central Market 3-5 | Plots 50-150 m², zero setbacks at market, 0.5-1 m elsewhere | Market lanes 5-8 m; colony roads 6-8 m; Ring Road 40 m | Low: ~10-12% canopy; block parks |
| Sarojini Nagar | Government colony (ministerial staff housing, 1950s-60s, now being redeveloped into towers by NBCC: Sarojini Nagar GPRA redevelopment) + market lanes | OSM mean 6.9 (max 12), 2-3 storey stall shops in the core | Market shops 3-6 m frontage, stalls on road edge | Lanes 3-5 m; colony roads 7-10 m | Moderate: government colony trees |
| Hauz Khas Village | Medieval ruins (Tughlaq 1352) beside an urban village (Hauz Khas Village was a Muslim/Punjabi urban village) with 3-4 storey bars; heights tagged in Overture | Mean 4.0 | Narrow organic plots 40-120 m² | 3-5 m lanes, one-way loop | High: Deer Park, lake |
| Khan Market | 1951 refugee-built market, two-storey arcade, plus Lodhi Colony bungalows | 3.8 (low sample), 2 in the market | Market block ~110 x 60 m, back lanes; Lodhi Colony bungalows 500-1,000 m² | 8-12 m roads | Very high; Lodhi Garden |
| Nehru Place | 1970s planned CBD: concrete plazas + towers (Hemkunt, Eros, Siddharth); informal computer-market lanes at the base | OSM mean 7.9, max 19 | Large block sites | 15-30 m | Low in plaza; Lotus Temple lawns 4+ ha |
| Defence Colony | 1950s planned colony for ex-servicemen; lower-density; many 4-storey rebuilds | OSM mean 4.3 | Plots 200-400 m², 3-5 m setbacks | 8-10 m | ~25% canopy |
| INA / Dilli Haat | Dilli Haat is a low-rise brick/sandstone craft village; INA market is an old dense market; AIIMS/Safdarjung are large institutional blocks | OSM mean 6.0, max 16 | | 10-25 m | Dilli Haat has gardens |

Concrete modelling guidance:
- Heights: where `building:levels` / `num_floors` is missing, default residential to 4 floors x 3.0-3.2 m (stilt ground floor 2.8-3.2 m, parking, then 3 floors) in GK, Defence Colony and Lajpat Nagar; commercial to 3 floors at markets; hospitals 8-10; Nehru Place 8-16.
- Facade vocabulary: GK = painted/plastered or stone/marble-clad facades, glass railings, balconies, projecting bays, AC units, rooftop water tanks (black PVC), terraced mumty; Lajpat = bare brick/rough plaster, vertical signboards, hanging cable tangles, shutters; Sarojini = tarpaulin awnings, steel stall frames, hanging garments.
- Boom barriers and guard booths at block entrances are GK-1/GK-2 signature props.

## 4. Data files produced

All in `C:\Users\Rayaan\Desktop\game\world\delhi_south\source\`:

| Name | OSM XML | Overture GeoJSON | Features | Overpass size |
|---|---|---|---|---|
| gk1 | `gk1.osm` | `gk1_overture_buildings.geojson` | 3,519 | 7.9 MB |
| gk2 | `gk2.osm` | `gk2_overture_buildings.geojson` | 3,509 | 6.3 MB |
| lajpat_nagar | `lajpat_nagar.osm` | `lajpat_nagar_overture_buildings.geojson` | 4,466 | 10.7 MB |
| sarojini_nagar | `sarojini_nagar.osm` | `sarojini_nagar_overture_buildings.geojson` | 1,679 | 6.8 MB |
| hauz_khas_village | `hauz_khas_village.osm` | `hauz_khas_village_overture_buildings.geojson` | 645 | 2.6 MB |
| khan_market | `khan_market.osm` | `khan_market_overture_buildings.geojson` | 1,447 | 4.7 MB |
| nehru_place | `nehru_place.osm` | `nehru_place_overture_buildings.geojson` | 1,455 | 5.1 MB |
| defence_colony | `defence_colony.osm` | `defence_colony_overture_buildings.geojson` | 1,855 | 7.2 MB |
| ina_dilli_haat | `ina_dilli_haat.osm` | `ina_dilli_haat_overture_buildings.geojson` | 928 | 8.0 MB |

Blockouts (Blender 5.2 headless, `world/mumbai/scripts/osm_to_blender.py` unmodified, `.blend` + `.fbx` in `world\delhi_south\`): `gk1_blockout` BUILT 2,720; `gk2_blockout` 2,367; `lajpat_nagar_blockout` 3,261; `sarojini_nagar_blockout` 823; `hauz_khas_village_blockout` 386; `khan_market_blockout` 691; `nehru_place_blockout` 906; `defence_colony_blockout` 1,063; `ina_dilli_haat_blockout` 521. The log is `source\_blockout_log.txt`.

Notes and caveats:
- The Overpass query uses `nwr` + recursion, which pulls route/boundary relations and their far-away members; the `.osm` files therefore contain nodes well outside each bbox (hence 6-10 MB). The blockout script clips to the bbox, so it is fine, but any downstream tool must clip too. Overpass needed 2 and 3 retries on Khan Market and Lajpat; all files verified > 2.5 MB and not rate-limit text.
- OSM building ways within each bbox (column 7 of the table) is the clean number; the blockout counts include multipolygon building relations.
- Housekeeping logs left in `source\`: `_fetch_log.txt`, `_overture_log.txt`, `_blockout_log.txt`, plus Overture `.state` files may exist (safe to delete).
- Overture heights are mostly absent; only Hauz Khas has meaningful `height`.

## 5. Recommended build order and why

1. **Lajpat Nagar + Defence Colony (one stitched tile, ~4.8 km² combined)**: reuse the Old Delhi pipeline almost unchanged (dense market, shuttered shops, cable tangles, refugee-era housing), so it is the cheapest quality jump after Chandni Chowk. Central Market is a strong "second hub" with a metro interchange. Defence Colony gives the quiet-bungalow contrast at no extra pipeline cost.
2. **Sarojini Nagar + INA/Dilli Haat (stitch, ~3.3 km²)**: the street-stall system (awning/tarp, dense NPC crowd, hanging garments) is a new reusable system and Sarojini is the iconic example. INA's Dilli Haat is an easy-to-model, bounded, ticketed set piece and the Pink Line links them.
3. **GK-1 then GK-2**: the new "upper-middle-class colony" fabric: builder floors, stilt parking, boom barriers, tree-lined blocks, glass/stone facades. Largest building counts (3.5k each) and most repetitive, so it is best done once a procedural builder-floor generator exists; GK-1 first because M-Block market and the Kailash Colony metro form a compact slice.
4. **Hauz Khas Village**: smallest dataset, highest visual payoff (Tughlaq ruins + lake + nightlife), a "tourist/night" content layer; needs hero modelling of the madrasa and tomb.
5. **Khan Market**: low-rise, high trees; light data, high prestige; needs distinct elite-market props and Lodhi Garden.
6. **Nehru Place** last: only tile requiring mid- and high-rise towers (8-19 storeys), a different modelling pipeline (curtain-wall/concrete), and Lotus Temple is a single hero asset worth deferring.

The above order also lets the city grow contiguously: Defence Colony/Lajpat (east-centre) touches GK-1 to its south-east; INA/Sarojini (west-centre) neighbours Hauz Khas to its south; Khan Market connects north across the Lodhi Road corridor from Defence Colony.

## 6. Sources

- OSM data: Overpass API https://overpass-api.de/api/interpreter (queries described above); POI coordinates are OSM tags verified from the downloaded files.
- Overture Maps buildings: https://docs.overturemaps.org/ (CLI `overturemaps download --type=building`)
- Greater Kailash overview: https://en.wikipedia.org/wiki/Greater_Kailash
- GK-1 M-Block market: https://housing.com/news/gk-1-m-block-market-shoppers-guide/ , https://lbb.in/places/M-Block-Market-GK-1-greater-kailash-1-e63c19 , https://www.shopkhoj.com/markets-in-delhi/gk-m-block-market/page/4/
- GK-2 M-Block market: https://thedelhiwalla.blog/2022/07/24/city-walk-m-block-market-gk-ii/
- Lajpat Nagar history: https://en.wikipedia.org/wiki/Lajpat_Nagar , https://thepatriot.in/?p=83312 , https://thepatriot.in/?p=27933 , https://sindhcourier.com/refugee-colonies-changed-south-delhis-face/
- Sarojini Nagar market sections: https://wikipedia2007.classicistranieri.com/s/a/r/Sarojini_Nagar_b61c.html (older mirror: re-check)
- Property/colony character: https://housing.com/greater-kailash-i-new-delhi-overview-P38fgvvor9ioglwx

Verification status: markets, metro, hospitals, schools, parks, worship, police coordinates are OSM-verified. Web research confirmed colony histories and market descriptions but found nothing authoritative on builder-floor architecture or plot sizes: those figures are *est.* and flagged. Famous food-spot names are from listings and are not geolocated individually (use the market centroid; fictionalise brands).
