# 05. NPC population and characters: Old Delhi, Phase 1

Author: NPC/character design lead. Date: 10 Nov 2026. Builds on `implementation.md` §4.5 (crowds), §5 (locations), §6 (fiction policy) and `old_delhi_pois.csv` (557 POIs).

**Evidence grading used below.** [V] = verified in a cited source. [E] = my estimate or design number, derived from [V] figures plus standard crowd-density rules. [U] = unverified, check before relying on it. Be aware that no source I could find gives ward-level 2026 population or station-level metro ridership for the Phase 1 box. Everything at street level in §1 is therefore [E], calibrated to the [V] anchors. Footfall claims for Chandni Chowk come mostly from a developer's marketing and vary 4x between sources.

---

## 0. Executive summary

- Real residents in the box: about 160,000 to 250,000 [V range, walled city], planning figure **180,000**. Real daily presence on a busy weekday is **400,000 to 700,000** people (residents, workers, shoppers, rail and metro passengers passing through) [E].
- The game never simulates that many. Budget: **~9,000 simulated agents** held as cheap Mass entities in the loaded World Partition cells (peaks ~2,500 active near the player), **~600 visible** at peak (matching `implementation.md` §4.5; hard cap 750 on festival bursts), **12 LOD0 + ~40 LOD1** full skeletal characters near the player.
- **52 archetypes** catalogued (§2), each with season clothing, StateTree states, POI mapping and language mix.
- To avoid visible clones: **~96 unique heads, ~170 garment meshes, ~55 hair/beard pieces, and a material-parameter system** giving well over 100,000 distinct combinations (§4). Estimated 12-month cost: about **US$38k to 70k** (about ₹33 to 62 lakh at ₹88/US$), mostly animation capture and artist time.
- Biggest risks are not technical: stereotype and religious sensitivity (§3) and trademark of anything on clothing (§3.5).

---

## 1. Population numbers

### 1.1 Real-world anchors

| Quantity | Figure | Grade | Source |
|---|---|---|---|
| Central Delhi district, Census 2011 | 582,320 residents, ~25,759/km², decline of 10.48% over 2001-2011 | V (two copies of the same article disagree: 25,759 vs 23,149/km²) | Wikipedia, Central Delhi district |
| Walled city residents over time | ~0.42 M (1961) → ~0.25 M (2001, MPD-2001) → ~0.16 M (a later, undated estimate) | V (year of last figure truncated in extract) | IIPA paper on Chandni Chowk redevelopment |
| Walled city density, 1961 / 1991 | 740 / 616 persons per hectare | V (old) | French IRD study |
| Chandni Chowk daily footfall | 4-6 lakh (developer-promoted); ~4 lakh (Tribune, post Red Fort blast coverage); 1.5-2 lakh (The Better India) | V but inconsistent | Omaxe, Tribune, Better India |
| Footfall change | +60-70% over recent years credited to Delhi Metro | V (claim) | Real estate press |
| Delhi Metro network | 64.6 lakh average daily journeys in 2025; record 82.45 lakh on 17 Sep 2026 | V | DMRC via PSU Connect, Uniindia, Tribune |
| Chawri Bazar metro station | ~30,307 average daily (Jan 2015) | V (old) | DMRC ridership via Wikipedia |
| Chandni Chowk / Jama Masjid / Lal Qila metro | No figure found | U | check DMRC station-wise data |
| Old Delhi Junction (DLI) | 1.8 lakh to 3.5 lakh passengers a day (mirrors disagree); ~190-250 trains a day | V but inconsistent | Wikipedia Delhi Junction |
| Jama Masjid courtyard | ~25,000 worshippers | V | Britannica |
| Eid al-Fitr 2026 at Jama Masjid | Large crowds, "sea of devotees", four police teams plus paramilitary; security tightened after a Delhi High Court directive; Alvida (last Friday of Ramadan) shoppers filled surrounding markets | V (qualitative) | Deccan Herald, Tribune, NewKerala |
| Red Fort blast aftermath | Chandni Chowk trade reportedly hit by ~Rs 400 crore; heightened security | V | Tribune |
| Khari Baoli porters | Mostly ex-farmers from rural north India, many live at the Gadodia Market building | V | Roads & Kingdoms, Dodho, Scroll, Sarah K. Khan's portraits |

### 1.2 Phase 1 box: residents and visitors (design figures)

Box is about 2.9 x 2.2 km, ~6.5 km² including the Red Fort grounds, rail yards and some low-density institutional land, so average density is far below the 25,000/km² district figure.

| Group | Planning figure | Grade | Reasoning |
|---|---|---|---|
| Residents | **180,000** (range 160k-250k) | E | IIPA 0.16 M to 0.25 M for the walled city, scaled for box edges |
| Resident-workers who work in the box | ~90,000 | E | Many residents work locally; walled city is a 12-hour wholesale economy |
| Non-resident workers (shop staff, karigars, porters, hawkers, drivers) | ~120,000 | E | Wholesale markets draw staff from outside |
| Shoppers and traders (domestic, incl. wholesale buyers) | 250,000 weekday, 400,000 busy Saturday or wedding or festival days | E | Taken between the independent 1.5-2 lakh and the promoted 4-6 lakh, because the box is larger than Chandni Chowk Road alone |
| Rail passengers through Old Delhi Junction | ~250,000 per day (arrivals plus departures) | E | Midpoint of the 1.8-3.5 lakh range |
| Metro boardings and alightings, 4 box stations | ~120,000 per day [U] | E | Chawri Bazar alone was ~30k in 2015; ridership has grown about 2x since |
| Tourists (domestic plus foreign) | 15,000-30,000 per day, up to 60,000 on holidays | E | Red Fort, Jama Masjid, food walks |
| Worshippers | Daily: several thousand across mosques, Sis Ganj, Gauri Shankar, Jain temples. Friday Jama Masjid: 8,000-15,000 typical, 25,000 full (Eid, Alvida) | E (capacity V) | |
| **Peak-day unique people present** | **400,000 to 700,000** | E | Sum, with overlap removed |

Gender and age mix for planning (not Census; adjust by zone):

- Bazaar streets by day: **68% male, 32% female** (female share rises to 40-45% in evenings, wedding season and in Kinari and Chawri Bazar fabric zones). Residential galli: roughly even, with more women and children in daytime.
- Age, all pedestrians: children under 14 about 13% (higher in residential lanes), 15-24 about 20%, 25-44 about 38%, 45-59 about 19%, 60+ about 10%.
- Community mix is zonal, not uniform. Muslim-majority residential and commercial zones around Jama Masjid, Matia Mahal, Ballimaran, Urdu Bazar; Hindu, Jain and Baniya trading zones around Chandni Chowk, Dariba, Kinari, Khari Baoli, Chawri Bazar; Sikh presence around Sis Ganj and Fatehpuri; mixed on the main road, at the station and at the metro. Use these as spawn weights per zone tag, not as global percentages.

### 1.3 How many NPCs must be visible: design targets per street type and time

Method: take real pedestrian presence per 100 m of street (E), set from Fruin-style level-of-service bands (comfortable 0.3 p/m², crowded 0.7-1.0 p/m², dense 1.5+ p/m²), then apply a **game density factor** of about 25-40%, because the camera sees a corridor of roughly 100 m ahead and a 600-agent ceiling applies. Perceived density is carried by sound, sidewalk occupation (stalls, thelas, seated vendors), and tight crowd flow, not by raw count.

Rickshaw, e-rickshaw, handcart and two-wheeler vehicles are counted separately by the traffic system. Each vehicle carries 1 driver plus 0-3 passengers already counted below only if seated.

**Real presence per 100 m of street, both sides, pedestrians only [E]**

| Street type | Width | 5 am | 9 am | 1 pm | 6 pm | 10 pm | 2 am |
|---|---|---|---|---|---|---|---|
| Main road (Chandni Chowk Rd) | ~20 m | 15 | 120 | 350 | 600 (weekend 900) | 60 | 5 |
| Bazaar galli (Kinari, Dariba, Chawri, Khari Baoli) | 3-5 m | 10 | 40 | 250 | 400 | 40 | 3 |
| Residential galli (Ballimaran, Matia Mahal side lanes) | 2-3 m | 12 | 60 | 25 | 70 | 25 | 3 |
| Mosque steps and gate zone (within 50 m of gate, Jama Masjid) | n/a | 150 | 300 | 700 (Friday 8,000-25,000) | 600 | 200 | 15 |
| Station forecourt (Old Delhi Junction, ~100 x 60 m) | n/a | 600 | 1,500 | 1,000 | 2,000 | 1,200 | 400 |

Notes: Shutters roll up at 10-11 am, so bazaars at 9 am are staff, porters and delivery, not shoppers. Khari Baoli is busiest before 9 am (wholesale loading). Main road is rickshaw-only 9 am-9 pm. At 2 am the city is porters, sleepers, dogs, sanitation crews, rail passengers, and Ramadan night markets in season.

**Game-visible agents per 100 m of street (target in view) [E]**

| Street type | 5 am | 9 am | 1 pm | 6 pm | 10 pm | 2 am |
|---|---|---|---|---|---|---|
| Main road | 8 | 50 | 120 | 170 | 25 | 3 |
| Bazaar galli | 6 | 25 | 100 | 140 | 18 | 2 |
| Residential galli | 8 | 30 | 12 | 35 | 12 | 2 |
| Mosque steps and gate zone | 40 | 80 | 150 | 130 | 50 | 5 |
| Station forecourt (whole forecourt view) | 120 | 200 | 150 | 220 | 140 | 60 |

Add on top: seated and static dressing (shopkeepers inside shops, vendors on pavements, sleepers, beggars) is about 40% of the above and is cheap because it uses idle loops.

Composition at 6 pm main road (170 visible): 55 shoppers (families, couples, groups), 25 tourists and influencers, 20 commuters and students, 18 vendors and staff, 14 porters and handcart pushers, 10 rickshaw or e-rickshaw passengers on foot, 8 police and guards, 20 mixed (worshippers, elderly, kids, beggars, delivery riders).

### 1.4 Special days and conditions (multipliers on the table above) [E]

| Day or condition | Effect on population and behaviour |
|---|---|
| **Friday prayers** (~12:30-14:00, Jama Masjid) | Mosque zone: 8,000-15,000 real, 25,000 at Alvida. Game: **300-500 worshippers** in kneeling or standing rows as shared-animation instances in the courtyard, plus 150-250 flowing along Urdu Bazar and Meena Bazaar and overflowing onto roads with prayer mats. Surrounding streets x1.5 before, then a surge outbound at ~14:00. Heavier police presence, barricades, traffic diversions. Non-Muslim shops see trade dip around the prayer window, kebab and itr shops peak after. |
| **Eid al-Fitr** (about 20-21 Mar 2026, repeats yearly) | Day before ("Chaand Raat") and Alvida Friday: Jama Masjid area and Matia Mahal x2.5-3, shopping until 3 am. Eid morning: 25,000 capacity courtyard plus rows spilling to streets; children in new clothes, sheer kurta, sherwani, topi, attar, sewaiyan stalls; police with paramilitary (V). |
| **Eid al-Adha / Bakrid** (late May 2026 [U]) | Goat market in Urdu Park (Data Layer per `implementation.md` §4.4), butchers' stalls, many goats on lanes, morning prayer crowd like Eid. |
| **Ramadan nights** (about 18 Feb-19 Mar 2026 [U]) | Daytime quieter (x0.7), then from 7 pm iftar rush, Matia Mahal and Gali Kababian x2-3, open until 3 am. Sehri at 3-4 am keeps lanes alive. |
| **Diwali shopping** (Diwali 8 Nov 2026 [U]; shopping surge for ~2 weeks before) | Main road x1.6, Chawri Bazar (lamps, diyas), Kinari, Khari Baoli (dry fruit gifting) x1.8, sweets shops queued. Evening peak lasts to 11 pm. Police crowd control, one-way pedestrian lanes. On the day itself, shops close early and the area empties by 8 pm; crackers in residential lanes (ban enforcement variable). |
| **Wedding season** (Nov-Feb, peaks on auspicious dates) | Wedding-card, lehenga, jewellery buying x1.4 on Dariba, Chawri, Kinari; processions (baraat) at night in residential lanes. |
| **Weekend and Sunday** | Sunday is a weekly closure for many wholesale markets (varies, verify per market) [U]: Khari Baoli and Kinari drop x0.5, while food streets, Jama Masjid, and Red Fort tourist routes rise x1.4. Red Fort is closed Mondays [U]. |
| **Dussehra / Ramlila, Independence Day (15 Aug), Guru Tegh Bahadur day (Nov), Muharram** | Red Fort security cordon, Ramlila crowds on the Red Fort side lawns, kite-flying on rooftops with emptier streets (Aug 15), Sikh nagar kirtan and langar near Sis Ganj, Muharram tazia procession with road closures. Use as Data Layers with their own spawn tables. |
| **Monsoon rain** (Jul-Sep) | Pedestrians x0.55 during heavy rain, rising to x0.9 after 20-30 min as waterlogging drains (Chandni Chowk floods ankle-deep and drains in ~10 min per `implementation.md`). Umbrellas (black, multicolour, branded), plastic ponchos (hawkers appear within minutes), people shelter under chhajjas and awnings and in shop doors, rickshaw hoods up. Fewer tourists, more delivery riders. |
| **Smog days** (Nov-Jan, AQI 350-450+) | Pedestrians x0.8 (many still must work). Masks: ~35% wear cloth or N95 style masks outdoors on severe days, 60% of rickshaw and traffic staff; cough animations; scarf over face; fewer influencers, more elderly staying in. Visibility short, so visible agent budget can drop to 400 with no loss. |
| **Heat wave** (Apr-Jun, 40-46 °C) | Midday x0.6, shade-seeking, water vendors and nimbu-pani carts, wet towels on heads, gamchas; people congregate in evening, 5-9 pm x1.2. |
| **Winter fog / cold** | Morning 5-9 am x0.7, shawls, braziers with 4-6 people around each, tea stalls busy. |
| **Dust storm (aandhi)** | Scatter: everyone seeks cover within 90 seconds; cloth, tarps and hoardings fly (see `implementation.md` §4.3). |
| **Security alert (post Red Fort blast context)** | Barricades and metal-detectors at gates, police x2 near Red Fort, Jama Masjid and metro; tourist count x0.7 (V: trade impact reported by Tribune). Depict non-sensationally. |

### 1.5 Game budget

All numbers are for RTX 3070 8 GB, 60 fps, under the "<3 ms game thread" ceiling in `implementation.md` §4.5.

| Layer | Count (peak) | Technology | Notes |
|---|---|---|---|
| **Abstract population** (whole 6.5 km², unloaded cells) | 180,000 residents represented as zone density maps | Data only (counters per ZoneGraph lane / per cell) | Never spawned. Drives target density per cell, and spawn on entering streaming range. |
| **Simulated agents** (Mass entities in loaded cells, ~250 m radius, no mesh) | ~9,000 pool, **~2,500 active** near the player | MassAI processors, ZoneGraph lane following, StateTree | Mesh-less entities consume ~microseconds each. Off-screen ones tick at reduced LOD (Mass simulation LOD: far at 1-5 Hz). |
| **Visible agents** | **600 typical peak, 750 hard cap** | Mass representation LOD | Cap enforced by the Spawn Governor (see §5). Smog nights can run 400. |
| LOD0 full-detail characters | **12** within ~8 m | Full MetaHuman skeleton, full facial rig, strands hair on up to 4, cloth sim on 2, IK, contextual interaction | Conversation partners, vendors the player haggles with, hero NPCs. |
| LOD1 skeletal | **~40** within ~8-25 m | Skeletal mesh (MetaHuman LOD 2-3 equivalent, ~10-15k tris), hair cards, simplified rig, anim LOD (30 fps update), no face rig | Matches `implementation.md` (~40 full skeletal). |
| LOD2 crowd instances | **~150** at 25-60 m | Vertex-animation-texture (VAT) or Anim Sharing ISM, ~3-5k tris, 1K atlas materials | Walk, idle, haggle variants baked. |
| LOD3 | **~300-400** at 60-150 m | Low-poly VAT ~600-1,000 tris, 512 atlas, no shadow cast beyond 80 m | |
| LOD4 | up to ~100 | Billboard or Niagara colour-dot impostors on the main road beyond 150 m | Crowd feel only. |
| Seated and static dressing | ~250 additional | Skeletal with idle-only anim, tick off when not visible | Shopkeepers in shops, vendors, sleepers. Counted separate from the 600. |
| Prayer rows (Friday/Eid) | 300-500 | Single shared animation via instanced skeletal or VAT with sync | Added as a special layer while Friday or Eid Data Layer is on. |

Texture memory cap: about 450 MB for all NPC textures combined (leaves ~2.5 GB for the rest under the 3000 MB streaming pool in `implementation.md` §8). Achieved with atlased outfits and masks (§4).

**Mass LOD transition**: use the MetaHuman Crowd (5.8 Experimental per `implementation.md`) path for LOD0 to LOD2, with a pre-baked fallback if it regresses.

---

## 2. Archetype catalogue (52)

Seasons: **S** = summer (Apr-Jun, hot dry), **M** = monsoon (Jul-Sep, humid), **W** = winter (Nov-Feb, cool and smog). "Sp" = spring and autumn shoulder months use S unless noted. Languages: Hi = Hindi (Delhi Hindi, Hinglish), Ur = Urdu (Old Delhi register), Pa = Punjabi, Bn = Bengali, En = English, Hr = Haryanvi, Bh = Bhojpuri/Maithili/Awadhi, Ra = Rajasthani/Marwari.

Names are archetype labels, not individual identity. Spawn weights are for the zone tags in §2.4.

### 2.1 Identity and look

| ID | Archetype | Age | Gender (M/F/other) | Summer | Monsoon | Winter | Accessories |
|---|---|---|---|---|---|---|---|
| A01 | Spice shop owner / wholesaler (Khari Baoli) | 40-70 | 95/5/0 | Cotton kurta, dhoti or pyjama, gamcha | Same plus plastic sheet over stock | Kurta plus waistcoat or woollen shawl, monkey cap | Cash box (galla), calculator, mobile on a thread, wooden stool (gaddi cushion) |
| A02 | Spice sorter / packer (indoor) | 18-55 | 70/30/0 | T-shirt or kurta, cotton cloth over head | Cloth, wet floors | Cardigan, shawl | Face cloth, scoops, weighing scale |
| A03 | Silver and jewellery shopkeeper (Dariba Kalan) | 30-65 | 90/10/0 | Crisp white kurta pyjama, gold ring and chain | Same, closed umbrella | Bandhgala jacket or sweater plus shawl | Jeweller's loupe, scales, phone with payment QR, small safe |
| A04 | Karigar (jeweller or silver artisan, many from West Bengal [V: widely documented]) | 20-55 | 98/2/0 | Vest or half-sleeve shirt, lungi or trouser | Same | Sweater, scarf | Tools, tweezers, charcoal blower, small plastic tiffin |
| A05 | Wedding-card seller (Chawri Bazar) | 25-60 | 90/10/0 | Short kurta or shirt-trouser | Same | Sweater, jacket | Sample card books, stapler, receipt book |
| A06 | Tinsel and gota (Kinari Bazar) seller | 25-60 | 85/15/0 | Kurta, bright dupatta draped over shoulder for women | Same | Shawl | Tinsel garlands, turban cloth rolls, measuring tape |
| A07 | Brass and utensil trader (Chawri) | 35-65 | 95/5/0 | Kurta, dhoti | Same | Sweater, shawl | Hammers, scrap bins |
| A08 | Electronics and lighting shopkeeper (Lajpat Rai Market, Bhagirath Palace) | 22-55 | 92/8/0 | Branded-looking (fictional) T-shirt, trouser | Same | Hoodie or jacket | Phone, ledger, multimeter, soldering station |
| A09 | Saree and lehenga showroom salesperson | 20-45 | 70/30/0 | Shirt-trouser, kurta | Same | Sweater | Measuring ribbon, bundles of cloth |
| A10 | Attar / perfumery seller (Jama Masjid) | 30-65 | 100/0/0 | White kurta pyjama, topi, soft beard | Same | Waistcoat, shawl | Glass vials, cotton buds, pocket cases |
| A11 | Kite shop owner (Lal Kuan, season-based) | 25-55 | 100/0/0 | T-shirt or kurta | Same | Sweater, scarf | Manjha spools, kites bundles |
| A12 | Meat and kebab stall cook (Jama Masjid / Gali Kababian) | 25-60 | 98/2/0 | Banian or kurta, apron, topi optional | Same | Sweater | Charcoal fan, skewers, towel on shoulder |
| A13 | Chai-wallah | 18-65 | 95/5/0 | Shirt, gamcha | Plastic sheet, umbrella over kettle | Monkey cap, sweater, shawl | Kettle, kulhad or glass tray, stove, stool |
| A14 | Street food vendor (chaat, golgappa, jalebi, kulfi) | 20-55 | 85/15/0 | Vest or half-sleeve shirt, apron, cap | Plastic cover | Sweater, shawl | Thela, trays, steel bowls, gas cylinder, tongs |
| A15 | Fruit and vegetable hawker (thela) | 20-60 | 80/20/0 | Vest, gamcha, lungi or trousers | Plastic poncho | Sweater | Handcart, scale (tarazu), baskets |
| A16 | Balloon / toy / trinket hawker | 18-60 | 75/25/0 | Shirt, saree for women | Plastic cover | Shawl, jackets | Pole with hanging toys, sack |
| A17 | Palledar / porter (Khari Baoli and bazaar loading) | 25-60 | 100/0/0 | Vest and lungi or shorts, cloth pad (gaddi) on head, gamcha | Same, wet sacks | Sweater, monkey cap, shawl | Sacks 40-80 kg on head or back, hook, rope, cloth pad |
| A18 | Handcart (thela) puller | 20-55 | 100/0/0 | Vest, trousers, gamcha | Plastic sheet | Jacket, scarf | Cart loaded with cartons or iron |
| A19 | Cycle-rickshaw puller | 25-60 | 100/0/0 | T-shirt or vest, lungi or trousers, gamcha, flip-flops | Plastic sheet poncho, hood | Sweater, shawl, monkey cap | Bicycle bell, plastic water bottle, passenger cushions |
| A20 | E-rickshaw driver | 20-55 | 94/6/0 | Shirt, trousers, cap | Same, plastic sheet | Jacket, muffler | Phone mount, battery cable, whistle, cash pouch |
| A21 | Auto-rickshaw and taxi driver (edges, station) | 25-60 | 98/2/0 | Khaki shirt and trousers (auto uniform), cap | Same | Jacket, muffler | Meter, vest or ID badge, CNG sticker |
| A22 | Delivery rider (food and quick-commerce; fictional brands) | 18-35 | 94/6/0 | Branded T-shirt, jeans, helmet, thermal bag | Rain jacket, waterproof bag cover | Padded jacket, gloves, balaclava | Smartphone on handlebar, thermal bag, power bank |
| A23 | Cart or goods carrier (tempo) driver and loader | 25-55 | 100/0/0 | Shirt, lungi | Raincoat | Sweater | Tarp, rope |
| A24 | Delhi Police constable / sub-inspector | 22-55 | 90/10/0 | Khaki shirt, trousers, black belt, cap or blue beret, ID badge, lathi (baton) | Rain jacket | Jacket, woollen cap | Wireless set, whistle, body-worn camera (since 2020s) [U], notebook |
| A25 | Traffic police | 25-55 | 88/12/0 | Delhi Traffic Police uniform with hi-vis vest, white gloves, peaked cap, sunglasses [U verify with reference photos] | Rain cape | Jacket | Whistle, e-challan device, baton |
| A26 | Paramilitary or security at Red Fort and gates | 22-45 | 92/8/0 | Camouflage or olive uniform, body armour, rifle on sling | Rain jacket | Jacket | Radio, metal detector wand |
| A27 | Private security guard (shops, banks, Omaxe-type malls) | 25-60 | 95/5/0 | Grey or blue uniform, cap | Rain jacket | Jacket | Whistle, stool, stick |
| A28 | MCD sanitation worker (sweeper, "safai karmachari") | 25-55 | 55/45/0 | Orange or green hi-vis jacket over uniform, scarf | Rubber boots, plastic sheet | Jacket, woollen cap | Broom (jhadu), handcart, mask, gloves [V]. Depict with dignity. |
| A29 | Electrician on wire pole / cable crew | 25-50 | 100/0/0 | Cotton shirt, trousers, helmet, harness | Rain jacket, harness | Jacket | Ladder, tester, cable cutters, cable rolls (underground cabling since Feb 2026 per `implementation.md`) |
| A30 | Water-tanker / civic works labour | 20-55 | 85/15/0 | Vest, trousers, cap | Rain jacket | Sweater | Hose, shovel |
| A31 | Student (school, uniform) | 6-17 | 50/50/0 | School uniform: white shirt, blue or grey trousers, skirt or pinafore, tie, shoes | Raincoat, umbrella | Blazer or sweater, cap | Backpack, water bottle, tiffin, ID card |
| A32 | College student / IGDTUW (female-only), Delhi University crowd | 17-25 | 45/55/0 | Jeans, tee, kurti, sneakers | Umbrella, jacket | Hoodie, denim jacket, scarf | Phone, earphones, tote bag, lanyard |
| A33 | Office worker / bank clerk | 25-58 | 70/30/0 | Formal shirt, trousers, saree or kurta set for women | Umbrella, lunch bag | Blazer or sweater | Laptop bag, ID lanyard, tiffin (dabba), phone |
| A34 | Domestic tourist (families, groups) | 5-70 | 55/45/0 | Shorts, tees, sundresses, kurtas, caps, sunglasses | Ponchos | Jackets, hoodies | Phone, backpack, water bottle, selfie stick |
| A35 | Foreign tourist | 20-70 | 50/50/0 | T-shirt, loose trousers, hat, scarf to cover head at mosques | Raincoat | Fleece, down jacket | Daypack, camera, guidebook, phone |
| A36 | Pilgrim (Sis Ganj, Gauri Shankar, Jain mandir, church) | 20-80 | 50/50/0 | Traditional wear: kurta, salwar kameez, saree, turban (Sikh men), orange scarf, head cover | Same | Same plus shawl | Offering plate (prasad), flowers, rumala, mala |
| A37 | Namazi (worshipper at Jama Masjid and neighbourhood mosques) | 8-85 | 95/5/0 (women mostly pray at home or ladies' section) | White or light kurta pyjama, topi (taqiyah), prayer mat | Same | Waistcoat, shawl | Prayer mat, prayer beads, attar, ablution towel |
| A38 | Imam / muezzin / maulvi | 35-75 | 100/0/0 | Long kurta or sherwani, turban or topi, beard | Same | Heavier shawl | Prayer beads, Quran cover |
| A39 | Pandit / temple priest | 30-75 | 100/0/0 | Dhoti, angavastra, tilak, sacred thread, saffron or white | Same | Shawl, wool | Bell, aarti plate, flowers |
| A40 | Sikh sevadar (Sis Ganj Gurdwara) | 18-80 | 80/20/0 | White or saffron kurta pyjama, turban (dastar), blue or orange chola for nihang style rare, kara, kirpan | Same | Turban plus shawl | Sewa apron, langar tray, bucket, ladle. Respect rules in §3. |
| A41 | Beggar / mendicant (including elderly or disabled) | 8-85 | 50/45/5 | Worn salwar kameez, kurta, saree, shawl | Plastic sheet | Blanket, shawl | Bowl, bag, crutch, sleeping mat. See §3 depiction rules. |
| A42 | Transgender / hijra community member | 20-60 | n/a (self-identified) | Bright sarees or salwar kameez, heavy jewellery, bindi | Saree, dupatta over head | Shawl | Clapping gesture, dholak (rare), small purse. See §3. |
| A43 | Wedding party (baraat): groom, dhol, band, relatives | 5-80 | 55/45/0 | Sherwani, safa, lehenga, silk sarees, glitter | Umbrella | Sherwani with shawl, heavy jackets | Horse or decorated car, dhol, brass band, lights (generator), flowers |
| A44 | Children (non-school, playing, working at stalls) | 3-14 | 55/45/0 | T-shirt and shorts, frock, kurta, rubber sandals | Raincoats, bare feet, paper boats | Sweater, monkey cap, rubber boots | Kites, marbles, plastic ball, toy |
| A45 | Elderly (residents) | 60-90 | 55/45/0 | Kurta pyjama, saree, cardigan, cap | Umbrella, stick | Shawl, cap, mufflers | Walking stick, prayer beads, hearing aid, bag of vegetables |
| A46 | Housewife / homemaker resident | 25-65 | 5/95/0 | Salwar kameez, saree, burqa or hijab (Muslim neighbourhoods), dupatta | Umbrella, plastic bag over head | Shawl, cardigan | Cloth bag (jhola), steel container, phone |
| A47 | Muslim women (burqa, abaya, hijab, niqab or simple dupatta) | 15-80 | 0/100/0 | Black burqa, coloured abaya, headscarf; worn by choice, widely varied | Same | Shawl | Handbag, shopping bags, phone, children |
| A48 | Influencer / reel filmer (solo or crew) | 18-35 | 50/50/0 | Trendy streetwear, ethnic fusion, sunglasses | Waterproof, umbrella | Hoodie, jacket | Phone gimbal, ring light (hand-held), mic, tripod, drone not allowed near Red Fort (no-fly zone, per `implementation.md`) |
| A49 | Photographers (street, wedding, tourist) | 25-60 | 85/15/0 | Vests, hats | Rain cover | Jacket | DSLR, tripod, reflector |
| A50 | Local gang / loafer group (young men sitting at chabutra, bike) | 16-30 | 100/0/0 | T-shirt, jeans, kurta, sneakers, chain | Hoodie | Jackets | Bikes, phones, chewing gutka. Light touch only. |
| A51 | Doctors and clinic staff (Mohalla Clinic, Kasturba etc.) | 25-65 | 55/45/0 | White coat, stethoscope, formal wear | Umbrella | Sweater or coat | Medical bag, ID |
| A52 | Beauty, tailoring and craft workers (tailor, mehendi artist, tattoo, shoe-shiner, cobbler, ear-cleaner, barber on pavement) | 20-65 | 70/30/0 | Everyday clothing, aprons | Tarpaulin cover | Shawl | Sewing machine, chair, razor, brushes, shoe polish tin, cone of mehendi |

### 2.2 Behaviour, schedule, places, language

StateTree state names are shown in `monospace`; each archetype shares a base tree (`Commute` → `Work/Idle` → `Eat` → `Pray` → `Return`) and swaps in specialised leaf states.

| ID | Typical daily schedule (StateTree states) | Body language | Typical POIs / zones | Languages (weights) |
|---|---|---|---|---|
| A01 | 09:30 `OpenShop`, 10-20 `Sit&Manage` (talk to buyers, `Haggle`), 13:00 `Lunch`, 20:30 `CloseShop` | Seated, relaxed, hands counting, firm negotiation | Khari Baoli (28.6572, 77.2229), Gadodia Market | Hi 50, Ra 25, Hr 10, En 5, Ur 10 |
| A02 | 09:00-19:00 `Sort/Pack` indoors, `Break` chai | Bent, rhythmic hand work, cough, wipe eyes (spice dust) | Khari Baoli warehouses | Hi 50, Bh 30, Hr 10, Ur 10 |
| A03 | 10:30 `OpenShop`, 11-20 `ShowGoods` (careful, quiet voice), 20:30 close | Quiet, formal, measured hand gestures, polite | Dariba Kalan (28.6560, 77.2335) | Hi 55, En 20, Ur 15, Pa 10 |
| A04 | 09-20 `Craft` at bench (seated, tiny motions), `Chai` | Hunched, focus, sweat; bidi or phone breaks | Dariba back rooms, upper floors | Bn 40, Hi 45, Ur 15 |
| A05 | 10:30-21:00 `Pitch` (call to passing buyers), `Haggle`, peak in wedding season | Beckoning, flipping cards, bundling | Chawri Bazar (28.6499, 77.2298) | Hi 70, Pa 10, Ur 10, En 10 |
| A06 | 10:30-21:00 `Display` (shakes tinsel), `Pitch` | Light, bright, swaying fabric | Kinari Bazar | Hi 70, Ur 15, Pa 10, En 5 |
| A07 | 10-20 `Sit&Manage`, `Polish` | Mostly seated, polish brass | Chawri Bazar | Hi 70, Ur 20, Ra 10 |
| A08 | 11-20:30 `Pitch`, `Demo`, `Repair` | Quick, phone-glued, "Bhaiya, yeh dekhiye" | Lajpat Rai Market (28.6574, 77.2360), Bhagirath Palace | Hi 70, En 15, Pa 10, Ur 5 |
| A09 | 10:30-21:00 `Unroll&Show` fabric | Dramatic unrolling, courtesy for women customers | Chandni Chowk main road, Kinari | Hi 70, Ur 10, En 10, Pa 10 |
| A10 | 10-22 `Offer attar` on a hand; `Sit` | Soft, ceremonial, "Aap ke liye" | Jama Masjid Gate 1 lane | Ur 60, Hi 35, En 5 |
| A11 | Seasonal 09-21 `Stack&Sell` | Bundles, flag colours, rooftops | Lal Kuan | Hi 60, Ur 30, Pa 10 |
| A12 | 17:00-01:00 `Grill`, `Call out` | Hot, hand flourish, fanning, shouting orders | Gali Kababian (28.6494, 77.2337), Matia Mahal | Ur 60, Hi 40 |
| A13 | 05:00 `Set up`, 06-21 `Serve` (stir, pour from height), `Chat with regulars` | Cheerful, easy talk | Corners everywhere, metro and station exits | Hi 70, Bh 15, Ur 10, Pa 5 |
| A14 | 11-23 `Prepare&Serve` | Rapid hand work, spooning, stacked leaf plates | Paranthe Wali Gali (28.6560, 77.2304), Dariba jalebi corner, Chaina Ram etc. (fictional names per §6) | Hi 75, Ur 10, Pa 10, En 5 |
| A15 | 05:00 `LoadFromMandi`, 06-20 `Push&Sell` | Calling chants, pushing cart | Bagh Diwar Sabzi Mandi (28.6590, 77.2247), residential lanes | Hi 70, Hr 10, Bh 15, Ur 5 |
| A16 | 09-21 `Wander&Sell` | Holding toys aloft, following tourists | Red Fort approach, Jama Masjid, Chandni Chowk | Hi 80, Bh 10, Ur 10 |
| A17 | 04:00 `Queue at shop`, 05-12 `Carry loads` (steady bend gait, 40-80 kg, short breaths), 12-14 `Rest on sack stacks`, 15-20 `Carry loads` again, 22 `Return to quarters` | Deep forward lean, small quick steps, shouts "hatt jao, hatt jao" for way, sweat, minimal speech. Show strength, not caricature. | Khari Baoli, Gadodia Market, Fatehpuri | Hi 40, Bh 30, Hr 20, Ra 10 (many from rural UP, Bihar, Haryana, Rajasthan, Uttarakhand) |
| A18 | 06-22 `Push cart` | Bracing, leaning, shouting way, steering with hip | All bazaar lanes, Khari Baoli | Hi 60, Bh 30, Hr 10 |
| A19 | 08-22 `Wait at stand`, `Pedal` (rise from seat on hills), `Negotiate fare` | Standing on pedals, bell ringing, shouting "bhaiya, aaiye" | Chandni Chowk main road (rickshaw-only 9-21), metro stands | Hi 70, Bh 20, Ur 10 |
| A20 | 07-22 `Wait/Fill seats`, `Drive`, `Charge` at night | Horn-heavy, weaving, quick fare negotiation | Side lanes, metro gates | Hi 80, Ur 10, Bh 10 |
| A21 | 24h shifts; night around station | Sits in vehicle, calls out | Old Delhi Junction forecourt (28.6609, 77.2277), Delhi Gate | Hi 70, Pa 15, Ur 10, En 5 |
| A22 | 10-23 `Wait`, `Ride to pickup`, `Handover` (checks phone, hurries) | Hunched on phone, helmet, quick weaving | All streets (edges), residential galli, metro gates | Hi 70, En 15, Ur 10, Bn 5 |
| A23 | 04-10 `Unload`, 20-24 `Load` | Heavy shoving, shouting | Khari Baoli, Fatehpuri, Bagh Diwar | Hi 60, Hr 25, Pa 15 |
| A24 | Beat patrol 06-22, `Barricade`, `Checkpost`, `Crowd manage` | Stern or casual, hands behind back, radios, scanning | Kotwali (28.6586, 77.2311), Jama Masjid PS (28.6525, 77.2337), Hauz Qazi PS, Red Fort, metro gates | Hi 90, En 5, Pa 5 |
| A25 | 08-22 `Direct`, `Whistle`, `Challan` | Sharp hand signals, white gloves | Chandni Chowk cross, Delhi Gate, Netaji Subhash Marg | Hi 90, En 10 |
| A26 | 24 h shifts at fixed posts | Alert, rigid | Red Fort / Lahori Gate (28.6559, 77.2388), Jama Masjid gates | Hi 70, En 15, Pa 15 |
| A27 | 24 h shifts | Seated, sleepy at night | Banks, malls, jewellers | Hi 85, Bh 15 |
| A28 | 04:30-10:00 `Sweep` (lane by lane), 14-17 `Collect` | Bending, repetitive, rest on broom handle | All lanes, Chandni Chowk, mosque approaches | Hi 80, Bh 10, Pa 10 |
| A29 | 09-17 `Climb`, `Splice`, `Argue with locals` | Climbing, balance on pole, cautionary shouts | Wire-heavy lanes (Kinari, Dariba, Chawri) | Hi 85, En 5, Ur 10 |
| A30 | 07-16 | Shovel, lift | Drains, road works (Phase 2 of Chandni Chowk redevelopment) | Hi 70, Bh 20, Hr 10 |
| A31 | 06:45-08:00 `Walk to school`, 14:00-15:30 `Return`, `Snack`, `Play` | Running, chatter, backpack sag | Anglo Arabic School (28.6457, 77.2224), Fatehpuri Muslim School (28.6566, 77.2226), Sarvodaya Kanya Vidyalaya (28.6576, 77.2350) | Hi 60, Ur 20, En 15, Pa 5 |
| A32 | 09-17 `Study`, `Lunch`, `Chat`, `Shop`, `Reels` | Group chat, phone, quick walk | IGDTUW (28.6648, 77.2322), Chawri Bazar metro, DU-adjacent crowds | Hi 55, En 40, Pa 5 |
| A33 | 08:30 `Commute`, 13:00 `Lunch/dabba`, 18:00 `Commute home` | Brisk walk, phone, earphones | Banks, Town Hall (28.6573, 77.2275), metro stations | Hi 65, En 30, Pa 5 |
| A34 | 10-20 `Sightsee` (Red Fort, Jama Masjid, Chandni Chowk, Paranthe Wali Gali), `Shop`, `Eat`, `Photo` | Pointing, group stops, sun-protected | Red Fort, Jama Masjid, main road | Hi 60, En 15, Bn 8, Pa 10, Ra 4, Hr 3 (Bengali, Gujarati, Punjabi tourist groups common) |
| A35 | 09-18 `Guided tour`, `Photo`, `Eat street food` | Cautious, looking up, photographing, covering head at mosques | Jama Masjid, Red Fort, Chandni Chowk, food walks | En 90, other 10 |
| A36 | 05-20 `Visit shrine`, `Prasad`, `Queue` | Hands folded, head covered, shoe-removal | Sis Ganj Gurdwara (28.6559, 77.2323), Gauri Shankar (28.6558, 77.2357), Digambar Jain Temple (28.6558, 77.2361), Sunehri Masjid | Hi 50, Pa 30, Ur 10, En 10 |
| A37 | 5 daily prayer windows (fajr 05:00, zuhr 13:00, asr 16:30, maghrib sunset, isha ~20:00) with `Wudu` at hauz, `Prayer` (qiyam, ruku, sujud, tashahhud), Friday big congregation; `Post prayer chat` | Orderly rows, calm; quick rolling of mats; shoes at the gate | Jama Masjid (28.6507, 77.2335), Fatehpuri Masjid (28.6566, 77.2225), local mosques (Amrood wali Masjid, Anjuman Masjid, Sunehri Masjid) | Ur 70, Hi 30 |
| A38 | 05:00, 13:00, etc. `Lead prayer`, `Teach madrasa`, `Receive visitors` | Calm, dignified, measured walk | Mosques, madrasas | Ur 80, En 5, Hi 15 (Arabic recitation) |
| A39 | 05-12 `Puja`, `Aarti`, 17-20 `Evening aarti`, rings bells | Reverent, deliberate | Gauri Shankar, Digambar Jain Temple, small deity niches | Hi 90, Sanskrit 10 |
| A40 | 06-22 `Langar prep`, `Serve`, `Clean shoes`, `Guard gate` | Humble, quick, warm; hand over chest or fold | Gurdwara Sis Ganj | Pa 70, Hi 30 |
| A41 | 08-22 `Sit` at temple, mosque steps and crossing; `Approach` and `Withdraw` | Quiet or pleading; dignified default. Never comedic. | Jama Masjid steps, Gauri Shankar, metro exits (Fridays near mosques, Tuesdays and Saturdays near temples) | Hi 70, Ur 20, Bh 10 |
| A42 | 10-18 `Bless` at shops and traffic signals, wedding and newborn blessings, `Chat as group` | Confident, rhythmic clap, jewellery chime. Not a joke. | Shop fronts, crossings, wedding houses | Hi 90 (community argot Hijra Farsi sparingly) |
| A43 | 17-01 `Baraat procession` (dhol, dance, horse), `Reception` | Joyful, loud, dance, sparkle | Residential lanes, banquet halls, Meena Bazaar | Hi 60, Pa 30, Ur 10 |
| A44 | 07:00 school, 15:30-19:30 `Play` (cricket, kites, marbles), some `Help stall` | Running, hopping, laughing, kite flying | Lanes, rooftops, parks | Hi 80, Ur 15, En 5 |
| A45 | 05:00 `Walk`, `Sit at chabutra`, 18:00 `Chat`, `Visit temple/mosque` | Slow, resting, observing | Chabutras, courtyards, parks | Ur 40, Hi 50, Pa 10 |
| A46 | 07-11 `Market`, 15-17 `Chores`, 19 `Sit` | Quick market walk, bargaining, family in tow | Residential lanes, Chawri Bazar fabric shops | Hi 60, Ur 35, Pa 5 |
| A47 | Varies as A46 | Same as A46; do not treat as a single caricature | Matia Mahal, Jama Masjid, Chitli Qabar | Ur 70, Hi 30 |
| A48 | 11-20 `Scout location`, `Film`, `Talk to camera`, `Retake`, `Buy snack` | Self-conscious, stops in the path, holds camera at arm length | Paranthe Wali Gali, Jama Masjid steps, Chandni Chowk, Red Fort | Hi 55, En 45 |
| A49 | 09-18 `Pose` and `Shoot` | Backs to the crowd, kneeling, squatting | Jama Masjid, Red Fort | Hi 60, En 30, Ur 10 |
| A50 | 14-23 `Hang around` | Slouch, bikes, banter | Lanes, chabutras, gate corners | Hi 75, Ur 20, Pa 5 |
| A51 | 09-21 | Briskly walking | Kasturba (28.6485, 77.2385), Sant Parmanand (28.6631, 77.2379), Mohalla Clinic, small clinics | Hi 50, En 40, Ur 10 |
| A52 | 10-20 `Work at pavement` | Hands-on craft, quiet | Pavements, shop steps, markets | Hi 70, Ur 20, Bh 10 |

### 2.3 Mix rules (what a convincing street contains)

- A bazaar scene is dominated by A01-A15 (stationary) plus A17-A19 (moving workers). Moving crowd is mostly shoppers (A34, A46, A47, A09 customers) rather than a single type.
- Always have at least one **worker in motion with a load** (A17 or A18) within 30 m on a bazaar view. They give the right pace and the right "make way" cadence.
- Police are rare in lanes (about 1 per 150 pedestrians) and common at gates, crossings, metro entrances, Red Fort, Jama Masjid.
- Women are underrepresented in the historic streets at night. Use a lower share (15-20%) at 10 pm and 2 am, but remain present (families, couples, wedding guests).
- Children appear with an adult nearby on main roads, and unaccompanied in residential lanes, rooftops and schools.

### 2.4 Zone tags (spawn weight sources)

`Z_MainRoad`, `Z_Kinari`, `Z_Dariba`, `Z_Chawri`, `Z_KhariBaoli`, `Z_JamaMasjid`, `Z_MatiaMahal_Ballimaran`, `Z_SisGanj_Fatehpuri`, `Z_RedFort`, `Z_Station`, `Z_Metro`, `Z_School`, `Z_Hospital`. Derive from `old_delhi_pois.csv` tags (amenity=place_of_worship;religion=muslim etc.) and the hero locations in `implementation.md` §5.

---

## 3. Look-development guide

### 3.1 Skin tone and face

- Use a **continuous range**, not discrete "ethnic" slots. Target Fitzpatrick type III-V for most (Delhi's population spans light wheatish to dark brown, with strong variation within each family), with tails to II (some Kashmiri, Punjabi, Afghan-origin and foreign tourists) and VI (some workers, East African or Southern Indian). Within one street, expect all within-range tones.
- Authoring via MetaHuman Creator skin-tone controls plus a **per-instance skin tint parameter** (hue-locked, range ±12% value, ±8% saturation). Add **sun and dirt**: darker backs of hands and neck for outdoor workers, cheek flush for heat, nose-bridge sunburn on foreign tourists.
- Avoid the "same face, different hair" problem. Vary nose bridge, jaw width, ear shape, brow spacing, eye fold, lip fullness, and facial hair growth across the head set (§4).
- Age rendering: wrinkles and sun spots from age maps, grey in beard and eyebrows, stubble density from texture masks. Elderly bodies: stoop, thin limbs, uneven weight. Children: proportion changes (larger head, shorter limbs), not scaled adults.
- Body shape range: lean (workers, rickshaw pullers), medium, stocky, heavier (older traders), with **real nutrition variance**. Porters: strong and sinewy, not cartoon muscle.

### 3.2 Hair, beards, headwear

| Group | Styles |
|---|---|
| Men | Short oiled hair with side parting, crew cut, medium fade, receding hairline, grey, bald, long hair on youth, moustache only (thick "Rajasthani" curled, thin line), full beard (trimmed, long, henna-dyed on older men), clean-shaven, stubble |
| Women | Long braid (single or two), bun, open hair, middle parting with sindoor for some married Hindu women, short bob, hijab and dupatta covers, burqa/niqab, grey hair |
| Kids | Bowl cut, pigtails, shaved head (mundan), cropped |
| Headwear | Gamcha wrapped as turban, Sikh dastar (tied, many colours and styles: standard, patka for kids, patiala for youth), topi/taqiyah (white crochet, embroidered, black felt), Gandhi topi (political, fictional only), monkey cap (winter), skull cap, hard hat, baseball cap, turban of Rajasthani safa (weddings), gulabi dupatta |
| Facial hair types | Common among Muslim elders: full trimmed beard without moustache, often dyed with mehndi (orange or red tint), skullcap. Sikh men: uncut beard, rolled or set, in mesh or tied. Do not use these as stereotypes for anyone's behaviour. |

### 3.3 Regional diversity (without caricature)

Old Delhi is itself a migrant city. In practice: Delhi-born "Dilliwale" (Hindu Baniya and Jain traders, Muslim families of Mughal-era crafts), Partition-era Punjabi families (especially around Sis Ganj and Fatehpuri), Marwari and Rajasthani traders (Khari Baoli, Dariba), Bihar and UP workers (rickshaw, porters, delivery, kitchen), Haryana and Western UP farmers and drivers, Bengali karigars (jewellery), Kashmiri traders (shawls, dry fruit), Afghan and Central Asian visitors (dry fruit, tourists, medical visitors) [U on volume], Nepali workers (guards), plus northeast Indian students. Spread them as **individuals** with slight clothing, speech and accent differences (see §2.2).

### 3.4 Avoiding stereotypes and caricature (hard rules)

1. **No uniform "type" for a community.** Every religious or regional group appears in many roles (traders, students, doctors, police, delivery riders, tourists, sweepers), including positive and neutral roles in most of the cast.
2. **No criminalisation by clothing.** Do not link beards, burqas, skull caps, turbans, tilak or saffron to threat or comedy. Antagonists (if any) come from varied backgrounds and are defined by behaviour, not attire.
3. **Poverty with dignity.** Porters, sweepers, rickshaw pullers, beggars are shown with competence, humour, tiredness, family and agency. Avoid "poverty porn": no leering shots, no exaggerated filth, no objectifying poses. Photo essays on Khari Baoli porters (Sarah K. Khan, Mattia Mignani, Roads & Kingdoms) stress their role as the "invisible engine" of the market.
4. **Hijra community.** Depict as full community members with varied roles (blessing groups, shopkeepers, performers, ordinary passers-by). No sexualised or mocking animations. Hire a trans community consultant (e.g. via an Indian LGBTQ+ organisation) to review dialogue, costume and animations before shipping.
5. **Women.** Show women in many roles (traders, doctors, students, police, sanitation staff, tourists). Respect real-world constraints: fewer women on bazaar streets at night, but not none. No leering camera, no harassment mini-game. If harassment appears as a theme, keep it as consequence-bearing narrative, never ambient comedy.
6. **No national or accent mockery.** Dialogue uses real speech patterns with accuracy; no mock accents.
7. **Beggars and disabled characters.** Avoid "prop" uses. Do not let the player kick or abuse them without consequence; avoid timed interactions that treat them as resources.
8. **Children.** Never put kids in danger as a gameplay mechanic. No gore near children. Child labour depicted soberly if at all (stalls), never as a joke.
9. **Review.** Have a Delhi-based sensitivity reader per community (Muslim, Sikh, Hindu/Jain, trans community, Dalit, Bihari and UP migrant) before release, matching the "community review" risk row in `implementation.md` §10.

### 3.5 Religious attire and ritual: respectful depiction

- **Islam.** Prayer posture accuracy: standing (qiyam), bowing (ruku), prostration (sujud), sitting (tashahhud), with row alignment shoulder to shoulder, shoes removed, head covered. Wudu at the hauz or taps. Prayer mats facing qibla (west in Delhi, about 265°). Women pray in separate sections or at home. Do not use real Quranic verses or the name of God as decoration on trash or footwear. Calligraphy on signs should be neutral (shop names, "Bismillah" only on appropriate surfaces) and rendered correctly (get an Urdu or Arabic typographer to check). Azaan audio is a respectful, time-keyed asset, never comedic.
- **Sikhism.** Turban and uncut hair are religious articles. Accurate tying styles. Kirpan sheathed and worn correctly (gatra). No jokes with turban removal. No combat in the gurdwara per `implementation.md` §6. Head cover for everyone inside, shoes removed. Sevadars wear clean orange or white attire, serve langar sitting in rows.
- **Hinduism and Jainism.** Tilak, sacred thread, saffron, bindi, sindoor, marigold garlands, bells. Jain monks and nuns (white clothing, mouth cloth) appear only if handled with research. Do not use deity imagery on shoes, bins, or as comedic props. Deity tiles in wall corners are used as the real "do not urinate here" device but are kept clean.
- **Christianity.** Central Baptist Church: plain, Sunday service dress.
- **General.** No combat or violent set pieces inside places of worship. Crowd density near worship centres should never be used as a "target-rich" setting.
- **Mourning and funerals.** If shown, accurate and short (white clothing, funeral processions). Likely out of scope in Phase 1.

### 3.6 Fictional brands, logos, signage on clothing

Follow `implementation.md` §6: all brands, logos and shop names are fictional.

- Use the existing fictional kit: Chandni Chai Co., Lal Kesar Sweets, Purani Haveli Paranthe, Masala Mahal Spices, Zaika-e-Dilli Kebab House, ChaloNet 5G, Bharat Taranga Mobile, Toofan Cola, Gyan Path IIT-JEE Academy, Safalta Classes, Rangoli Shield Paints, Mazboot Cement, Shahi Kundan Jewellers, Chamak Silver House, Raftaar E-Rickshaw Batteries.
- **Delivery uniforms** (A22): invent two fictional services, e.g. **"Tezz Tiffin"** (red and cream) and **"Jhat-Pat Basket"** (violet and yellow, quick-commerce), with fictional glyph logos, not any real app's colours-and-shape combination. Run each through a trademark search; avoid a red cursive wordmark that resembles any real brand.
- **T-shirts and caps**: fictional cricket-team-style jerseys with invented city names and sponsor names (no IPL team colours + sponsor combinations), invented sports brands ("Raftaar Sports", "Jeet Gear"), fictional fashion labels ("Mod Dilli"), invented slogan tees (Hinglish: "Chill Kar Yaar"). Bootleg, "Adibas" style knock-offs are real in India but use obvious fictional mashups of invented brand names, not near-copies of real ones.
- **School uniforms**: fictional school crests (generic shield, sun, lamp). Keep colours realistic (white shirt, navy or grey), no real school's actual crest.
- **Police and Delhi Government insignia**: Real Delhi Police uniform is a public-order safety attire. Use accurate colours and rank-badge types, but **fictional or generic badge crest** and no actual real officer names or ID numbers. Name tags with fictional names. Check with a lawyer on use of state emblems (State Emblem of India Act 2005 restricts use of the emblem; use a stylised fictional emblem).
- **Political cap and shawl** colours: fictional party colours only; no real party flags or symbols (per §6).
- **Language on garments**: Devanagari, English, Urdu, Gurmukhi where relevant; no slurs, no real religious scripture.

### 3.7 Wear, dirt, and age on clothing

Use a **layered material system** per garment (one master material, parameterised):

| Layer | Controls |
|---|---|
| Base fabric | Albedo by pattern atlas (check, stripe, print, plain, embroidery), roughness per fabric (cotton 0.85, silk 0.45, polyester 0.6, wool 0.9, rubber/plastic 0.4) |
| Dye variation | Hue shift (±8%) and saturation per instance from a Delhi-specific palette (muted pinks, mustards, teals, maroons, whites that are never pure) |
| Sun fade | 0-0.6, stronger on shoulders and upper back; red→pink, blue→cyan |
| Dust and grime | Vertical gradient from hem up, dark armpits and collar, knee darkening. Heavier in summer and winter smog. |
| Sweat | Dark patches in S season on back, armpits, chest |
| Wet | Monsoon wetness mask driven by `MPC_Weather.Wetness` (darker albedo, lower roughness, cling in normal) |
| Stains | Spice powder (turmeric yellow, chilli red) on Khari Baoli staff, grease on cooks, soot on chai-wallahs, paan red specks at collar |
| Wear and repair | Patches, mended seams, frayed hems, missing buttons, loose threads; deliberately varied by occupation and income |
| Winter | Layers: kurta, sweater, shawl, monkey cap; pilling on wool; a **mask** slot for smog days (cloth, surgical, N95) |
| Rain | Poncho, umbrella (black dominant, floral for women), plastic sheet, carrier bags |

Weight everything by income and occupation so the crowd is not uniformly worn or uniformly pristine. Imperfection must be plausible, not messy for its own sake.

---

## 4. Production plan

### 4.1 Diversity math: how many uniques to avoid visible clones

Humans spot clones when the **same silhouette, same colour and same face** appear within about 5 s or 25 m of each other, so the target is: *no identical (head + outfit + colour) within a 25 m radius at LOD0-LOD2*.

Calculation for LOD0-LOD2 (≈200 characters in view):

- Need ≥ 400 distinct combinations active in the pool (2x view count) to make collisions rare.
- Combos = bodies × heads × outfits × palette variants × accessories. Using **4 body types per gender, 96 heads, 170 garments (about 85 male and 85 female), 8 colour/pattern variants per garment, and 40 accessories** gives ≫ 10⁵ combinations without needing 400 unique modelled characters.
- **Archetype-lock** each combination (a rickshaw puller never rolls a sherwani), by outfit category tags, which still leaves ≥ 15-40 unique options per archetype for the common ones (A17, A19, A33, A34, A46 each need ≥ 40 outfit-colour options; rare ones like A40 need ≥ 8).

### 4.2 Asset counts

| Asset | Count | Notes |
|---|---|---|
| **Body bases** (MetaHuman) | 4 male (lean, medium, stocky, heavy) + 4 female + 3 child (small, medium, large) + 2 elderly (stooped, frail) = **13** | Cloth physically fits each |
| **Unique heads** | **96**: 48 male, 32 female, 8 children, 8 elderly extra | 40 hero-grade (LOD0 faces, MetaHuman Creator, hand-tuned) + 56 crowd-grade (Creator then reduce via LOD recipes). Reuse via morph sliders (MetaHuman Animator not needed for crowd). |
| **Hair** | **30 hairstyles** (male 14, female 12, child 4) | 12 with Strands for LOD0, cards for the rest |
| **Beards and moustaches** | **14** | Stubble masks as texture layers |
| **Headwear** | **20** (dastar 5 styles, topi 4, monkey cap, gamcha wrap, hat, cap, hard hat, dupatta, burqa/hijab 5 variants, helmets 2) | |
| **Garment meshes** | **~170** (see kit below) | Each at LOD0-LOD3 |
| **Footwear** | **14** (flip-flops, sandals, chappals, sneakers, leather shoes, rubber boots, school shoes, jutti, barefoot, heels) | |
| **Accessories and props** | **~60** (bags, tiffins, phones, earphones, sacks, brooms, baskets, umbrellas, ponchos, masks, cameras...) | |
| **Materials** | 1 master character material, 1 master hair, 1 master skin; **~220 material instances** | Colour/pattern atlas 2K shared; per-instance parameters via Mass fragments or Custom Primitive Data |
| **Texture memory** | Heads 2K (LOD0), 1K (crowd), garments atlased 1K, masks packed (ORM, RGB mask), VT off for characters | ≈ 450 MB total in pool |

**Rule**: each LOD0 vendor or hero NPC is a unique face; crowd uses the 96-head set.

### 4.3 Modular outfit kit list (≈170 meshes)

Male (≈85): kurta short, kurta long, kurta with waistcoat, pathani suit, sherwani, kurta pyjama (cotton), churidar, dhoti and angavastra, lungi (3 patterns), shirt half-sleeve, shirt full-sleeve, polo, T-shirt (plain, printed), vest/banian, shorts, jeans, chinos, formal trousers, cargo, track pants, blazer, waistcoat, sweater (round, V, cardigan), hoodie, puffer, denim jacket, leather jacket, shawl (wool, Kashmiri), muffler, monkey cap, gamcha (shoulder, head, waist), uniforms (khaki police, traffic police, security, school, auto, delivery, MCD hi-vis, electrician), rain gear (poncho, raincoat, plastic sheet), gloves, lab coat, apron, chef whites, turban cloth, sacks.
Female (≈85): saree (3 draping styles, 6 border weights), salwar kameez, churidar kurti, lehenga, sharara, palazzo and kurti, jeans and top, western dress, abaya (3), burqa (3), hijab (3), niqab, dupatta (3 drapes), shawl, cardigan, jacket, school uniform, nurse/doctor coat, police uniform (female), sanitation hi-vis, wedding finery, hijra festive attire (saree plus jewellery).
Children (≈ small sets): school uniform (boy/girl), frock, shorts and T-shirt, kurta-pyjama, sweater, raincoat, topi, sherwani for Eid.

Build rules: each garment has 3 mesh LODs plus VAT-compatible crowd LOD, blendshapes for body types, tight UV layout (shared 2K atlas), clothing sim **only** for hero LOD0 (saree pallu, dupatta, shawl) using Chaos Cloth, with the rest via skinned physics bones or WPO wind (consistent with `implementation.md` §4.1 cloth rules).

### 4.4 Animation set list

Group by **use** (locomotion, labour, trade, social, religious, daily life, reactions), sharing a retargeted skeleton (MetaHuman skeleton, IK Retargeter).

| Group | Clips (all with idle/start/stop variants where applicable) |
|---|---|
| **Locomotion** | Walk (5 styles each for M/F/child/elder/heavy), stroll, brisk walk, hurry, jog, stop-start shuffle in crowd (sideways squeeze), lean into wind, walk in rain (hunched), walk with umbrella, walk with mask, walk while on phone (head down), look around (tourist), limp, stick walk, cane walk, crutch |
| **Load carrying** | Porter head-load walk (40-80 kg, bend and shuffle), back-load walk, shoulder-load, two-handed sack carry, bucket carry, jar carry on head, tiffin-box carry, cart push (loaded, uphill), cart pull, bicycle-cart steer, bags in both hands, child on hip, child on shoulder |
| **Transport** | Cycle-rickshaw pedal (seated, standing), mount and dismount, push uphill; e-rickshaw drive, auto drive; passenger seated (rickshaw, e-rickshaw), bike ride (rider, pillion, lean); scooter, cycle ride |
| **Trade and haggling** | Beckon, call-out (3 variants), show goods (fabric roll, jewellery, attar), weigh goods, pour chai (high pour), stir, fry (jalebi, parantha), grill, serve, count cash, tally on calculator, tap phone for QR payment, haggle (negotiation: shrug, "walk away", counter gesture, hand slap close deal), bargain with fingers, argue politely, package item, tie string bundle, bend to counter, polish brass, hammer, solder, sew |
| **Social** | Chat (standing 4 variants, seated 3), laugh, point, greet (namaste, salaam, hand-on-heart, handshake, hug), argue, whisper, take selfie, film reel (walk and talk), wave to camera, phone call (3), scroll phone (stand, walk, sit), earphones, text, group photo pose, tout / street call |
| **Religious** | **Muslim**: wudu (hands, mouth, nose, face, arms, head, feet), qiyam, ruku, sujud, jalsa, tashahhud, salaam left-right, dua with open hands, Quran reading, prayer mat roll, row-forming; **Sikh**: matha tekna (prostrate), sit-cross-legged, langar serve, kar-seva; **Hindu**: namaste, aarti, bell ring, flower offering, prostrate (dandavat), tilak apply; **Jain**: mouth cloth, offering; **Church**: kneel, sit. |
| **Daily life** | Eat (sit-cross-legged on floor, standing street food, from leaf plate, eat parantha with hand), drink (kulhad, glass, bottle), nimbu-pani, spit paan, smoke bidi, scratch, yawn, stretch, sleep (on cart, on mat, on step), sit on haunches (squat, "ukdu"), sit on stool, sit on kerb, wash face at tap, brush teeth, shave on pavement, comb hair, braid hair |
| **Labour** | Sweep (broom), pick litter, climb pole, splice wire, pull cable, shovel, lay bricks, wash vessels, hose pavement, shake out a carpet, hang laundry, carry water |
| **Police** | Stand guard (3), patrol, whistle and wave, direct traffic (4 signals), frisk, stop vehicle, radio, write challan, lathi tap |
| **Reactions** | Startle (horn, shout, crack), dodge (side-step, jump back), cough (smog and spice dust, 2), sneeze, cover nose, shield eyes from sun, shake off rain, shelter running, look up (kites), flinch (gunshot, explosion), duck, run (panic), freeze, point, call for help, hands up, crouch, fall, get up, hit react (ragdoll via Physical Animation) |
| **Children** | Run, skip, play cricket (bat, throw, catch), fly kite, marbles, jump rope, hold parent's hand, tug, cry, laugh |
| **Idle** | 12 idle variants (weight shifts, scratch, look around, check watch, adjust turban, adjust dupatta) for LOD1 crowd |

Target counts: **~420 unique clips** (includes variants and start/stop/loops, ~130 hero-quality, ~290 crowd-quality), plus blend spaces, Motion Matching database for LOD0 and LOD1, and Anim Sharing for LOD2/LOD3.

### 4.5 Sourcing

| Source | Use | Notes and licence |
|---|---|---|
| **MetaHuman Creator** (Epic) | All 96 heads and bodies. Customise via Mesh to MetaHuman for heroes | MetaHumans are free to use in UE-based projects including commercial games. Not usable in non-Unreal engines. Check current EULA at release. |
| **Fab marketplace** | South Asian clothing packs (saree, kurta, sherwani), base hair packs, props and bags, animation libraries, rain gear | Check each licence for commercial use, AI-training clauses and redistribution. Heavy re-texture required to avoid a "stock asset" look. Expect to rebuild 50-60% of clothing for Old Delhi accuracy. |
| **Marvelous Designer / CLO3D + Blender / Maya** | Custom Indian garments (saree, dupatta, turban tying) | Needs a pattern-drafting consultant for authentic drape. |
| **ZBrush / Substance** | Wear, stains, embroidery | |
| **Motion capture** | Hero and unusual motions (porter head-load, rickshaw pedal, haggling, prayer, Sikh langar) | Options: Rokoko Smartsuit (US$2.5k to 3.5k) or Xsens (US$12k+), or rented studio. Use **real practitioners** (porters, imam or namaz-trained actors, chai-wallahs) for authentic motion. Consent and fair pay. |
| **Marketplace mocap libraries** | Base locomotion, idles, daily life | Fab animation packs (US$50-300 each) and Mixamo-style libraries. Retarget via IK Rig. |
| **Video reference (AI-assisted mocap)** | Crowd-quality clips from reference video (Move.ai, DeepMotion-type) | Cheap but needs cleanup. |
| **Voice** | Hindi, Urdu, Punjabi, Bengali, English, Haryanvi, Bhojpuri bark sets | Hire native voice actors in Delhi. Record ~3,000 crowd lines (cheers, bargains, calls, chatter). AI voice (ElevenLabs-class) for placeholder only, never shipping without actor consent and a clear legal position. |

### 4.6 Estimated counts and costs (12-month Phase 1, 1-2 character artists, 1 animator, 1 tech-artist, contractors)

| Item | Qty | Unit cost (US$) | Total (US$) |
|---|---|---|---|
| MetaHuman heads (Creator work, tuning, LODs) | 96 | 60-150 (artist time) | 6k-14k |
| Custom garments (design, sim, LOD, UV) | 170 | 80-200 | 14k-34k (cut to ~9k with Fab purchases) |
| Hair and beards (grooms and cards) | 44 | 40-100 | 2k-4k |
| Fab purchases (clothes, props, anim) | | | 1.5k-3k |
| Mocap: studio or suit rental, performers (3-5 sessions), cleanup | ~420 clips | 15-40 per hero clip, 5-10 crowd | 4k-9k |
| Anim tech (retarget, blend spaces, Motion Matching, VAT bake) | | | 3k-6k |
| Voice recording (6-8 languages, ~3,000 lines, ~20 voices) | | | 4k-9k |
| Sensitivity readers and consultants (6 communities, 2 rounds) | | | 2k-4k |
| Photo/video reference (field day in Old Delhi, with permission) | | | 0.5k-1k |
| **Total** | | | **≈ US$38k-70k (≈ ₹33-62 lakh at ₹88/US$)** |

These exclude salaries of the core team if already counted elsewhere. Reduce cost by starting with the **vertical slice** (500 x 500 m): 40 heads, 70 garments, 12 hair, 100 clips cover ~80% of what the player sees in Chandni Chowk-Dariba-Kinari. Re-use heavily for GK-1/2, Lajpat Nagar and Sarojini Nagar later (different clothing mix: more jeans, tees, branded fashion; fewer porters; more students and shoppers; add retail staff, mall security, gym-goers).

### 4.7 Anti-clone safeguards in engine

1. **Mass fragment `FNPCAppearance`**: head id, hair id, beard id, body type, outfit slot ids, 8 colour params, wear 0-1, sweat 0-1, mask flag, accessory ids. Rolled at spawn with **archetype constraints** and a **25 m anti-duplicate check** (reroll if identical head or identical top colour within radius).
2. **Colour palette tables** per archetype and season (data assets), so a chai-wallah has believable tones.
3. **Per-instance variation** in gait speed (±12%), start-phase offset, step width, arm swing, posture lean. Never play the same animation in sync for neighbours, except in prayer rows.
4. **LOD pop prevention**: keep same appearance across LOD0-LOD3 (same atlas indices, same colours). Cross-fade at 0.3 s.
5. **Heavy duty uniques**: pre-authored named NPCs (shopkeepers, vendors the player meets repeatedly) get a unique face, outfit, voice and routine, flagged "never clone".

---

## 5. Time-of-day and event schedule table (drives spawns)

Implement as a **DataTable `DT_PopulationSchedule`** keyed by time block × zone tag. Each row = target density per 100 m (from §1.3) × archetype weight × event multiplier. A `UPopulationGovernor` subsystem reads game time and the `MPC_Weather` / event Data Layers and writes target density per ZoneGraph cell; Mass spawners and despawners converge to it over 60-120 s. All times IST, real-day compressed or tied to game clock.

### 5.1 Normal weekday (spring/autumn baseline), archetype weights by zone

Percent = share of visible agents.

| Time | Main road | Bazaar galli | Residential galli | Mosque / gate | Station forecourt |
|---|---|---|---|---|---|
| **05:00** | A28 sweepers 25, A17 porters 15, A13 chai 8, A37 namazis (fajr) 8, A36 pilgrims 6, A15 hawkers 8, A45 elderly 8, A41 beggars 6, A22 riders 4, dogs/cows, sleepers (static) 12 | A17 25, A18 20, A15 15, A28 12, A13 8, A23 12, static sleepers 8 | A45 20, A46 20 (fetching milk, water), A28 15, A37 10, A44 5, A13 10, others 20 | A37 40 (fajr), A41 10, A38 3, pigeon feeders A16 7, A36 5, A28 10, others 25 | A21 20, A19 10, A20 8, A13 10, travellers (A34, A33, A36) 40, A28 5, A24 7 |
| **09:00** | A33 commuters 20, A31 students 12, A19/A20 drivers 15, A17 porters 10, A13 8, A28 5, shopkeepers opening 15, A34 5, A24 5, A22 5 | Staff arriving (A02, A04, A09) 30, A17 20, A18 15, A15 10, shopkeepers (static) 15, A22 10 | A31 25 (school run), A33 15, A46 15, A45 10, A44 10, A22 10, A28 5, others 10 | A34 25, A16 15, A10 5, A41 8, A24 10, A37 10, A36 5, A28 5, others 17 | A33 20, A34 20, A21 15, A19 10, A20 10, A13 5, A24 8, others 12 |
| **13:00** | A34 20, A46 15, A47 10, A09 customers 10, A19 passengers 10, A14 vendors 8, A13 5, A24 5, A22 8, A50 5, others 4 | Shoppers 55 (A46, A47, A34, A08 buyers), vendors/staff 15, A17 8, A18 6, A22 6, A48 4, A24 3, A52 3 | A44 15, A46 20, A45 20, A37 10 (zuhr), A22 10, A31 10, A52 5, others 10 | A37 (zuhr) 50 (**Friday 90**), A34 15, A41 8, A24 10, A16 5, others 12 | A33 20, A34 20, A36 10, A21 10, A19 8, A13 5, others 27 |
| **18:00** | A34 25, A46 12, A47 8, A48 5, A19/A20 8, A14 8, A22 10, A33 12 (commute), A24 5, A50 5, A43 (weddings, seasonal) 2 | Shoppers 55, vendors 12, A22 12, A48 4, A33 6, A17 4, A24 3, A50 4 | A44 25, A45 15, A46 15, A50 10, A37 10 (maghrib), A22 10, A33 8, others 7 | A37 (maghrib) 30, A34 30, A48 8, A49 4, A16 5, A24 10, A41 7, A12 (kebab) 6 | A33 20, A34 15, A21 12, A19 8, A20 8, A36 10, A24 8, A13 5, others 14 |
| **22:00** | A34 15, A14 12 (night food), A22 18, A50 10, A19/A20 10, A24 8, A43 (baraat) 10, A12 5, others 12 | Shutters down, A22 20, A12 20 (food streets), A14 15, A50 10, A24 10, others 25 | A45 15, A46 10, A50 20, A44 5, A37 (isha) 10, A22 15, others 25 | A12 20, A34 20, A24 10, A41 15, A37 (isha) 15, A22 10, others 10 | Travellers 50, A21 12, A20 10, A24 8, A13 8, A41 6, others 6 |
| **02:00** | Sleepers (static) 40, A28 15, A24 10, A22 5, A18 10, A23 (tempo drivers) 10, dogs, others 10 | Sleepers (static) 35, A17 (early start) 20, A18 15, A23 15, A28 10, A24 5 | A50 (rare) 15, A24 10, A22 15, dogs, cows, others 60 | A41 (sleepers) 40, A24 20, dogs, pigeon sleepers, A28 10, A38 5, others 25 | Travellers (waiting) 50, A21 15, A20 10, A24 10, A41 10, A13 5 |

Simplify in the implementation by grouping archetypes into ~12 **spawn groups** (Worker, Shopper, Tourist, Commuter, Student, Worshipper, Vendor, Driver, Police, Sleeper/Static, Resident, Special) with each spawn group having a sub-table.

### 5.2 Event layer (Data Layers + subsystem flags)

| Event | Time window (IST) | Zones | Population effect | Added archetypes | Audio / visual |
|---|---|---|---|---|---|
| Friday prayers | Fri 12:00-14:30 (ramp: 11:30; peak 13:00; surge out 14:00-14:30) | Jama Masjid, Urdu Bazar, Meena Bazaar, Matia Mahal | Mosque ×40-100 (300-500 visible rows), side lanes ×1.5 | A37 (rows), A38, A24 ×3, A26, A41 ×2 | Khutba loudspeaker, azaan at 12:15-12:45 |
| Fajr, Zuhr, Asr, Maghrib, Isha | Per astronomical times, shifted ~25 min across the year | All mosques | Local lane +10-20 worshippers | A37, A38 | Azaan with stagger (per `implementation.md` §4.7) |
| Alvida (last Friday of Ramadan), Chaand Raat, Eid | Per lunar calendar | Jama Masjid, Matia Mahal, Urdu Bazar, Chandni Chowk | ×2.5-3 night before; Eid morning courtyard 25,000 real | A37 (kids in new clothes), A44, A46, A47, A24 ×4, A26 | Greeting hugs, sewaiyan stalls |
| Ramadan evenings | Maghrib to 03:00 | Matia Mahal, Jama Masjid lanes, Gali Kababian | ×2-3 at 19:00-01:00, daytime ×0.7 | A12 ×3, A14, A34, A22 | Smoke, drums, call to sehri |
| Diwali shopping | 14 days before Diwali, 10:00-23:00 | Main road, Chawri, Kinari, Khari Baoli, Dariba | ×1.6-1.8 | A34, A46, A24 ×2, A22 | Lights, diyas, sweet shops, crackers |
| Wedding season | Nov-Feb, nights 18:00-02:00 | Residential lanes, Dariba, Chawri, Kinari | Local ×1.4 | A43, A05, A06, A03 | Dhol, band, fireworks |
| Gurpurab / Sis Ganj events | Specific days | Sis Ganj, main road | Local ×2 | A36, A40 (kirtan, nagar kirtan) | Kirtan |
| Dussehra / Ramlila | Oct, evenings | Red Fort lawns, main road | Local ×1.5 | A34, A44, A24 ×2 | Effigy burning (Red Fort) |
| Independence Day (15 Aug) | 05:00-12:00 closed to traffic; afternoon kites | Red Fort, main road | Red Fort cordon, streets emptier | A24 ×4, A26 ×3, A11 | Kites |
| Bakrid | Morning prayer + 3 days | Jama Masjid, Urdu Park goat market | Mosque ×40-100, goats | A37, A12 | Goats |
| Muharram (tazia) | Specific day | Residential lanes | Procession closure | Local | Drums |
| Monsoon rain | Rain state | All | ×0.55 pedestrians, +30% delivery riders, +10% rickshaw demand | A22, A19, ponchos | Rain |
| Smog day (GRAP 3-4) | Nov-Jan | All | ×0.8 pedestrians, masks 35%, school closures on GRAP-4 [U], construction bans | A24 ×1.2 | Haze |
| Heat wave | Apr-Jun, 11:00-16:00 | All | ×0.6, shade clusters | A15 (nimbu-pani) | Heat shimmer |
| Winter fog | Dec-Jan, 05:00-09:00 | All | ×0.7, braziers | A13, A28 | Fog |
| Dust storm | Random, 30-60 min | All | Everyone shelters | | Wind |
| Security alert | Scripted or random | Red Fort, Jama Masjid, metro | Police ×2, tourists ×0.7, barricades | A24, A26 | |
| Red Fort closed (Monday) [U] | Mondays | Red Fort | Tourist ×0.5 at gate | | |
| Weekly market closure (Sunday) [U] | Sundays | Khari Baoli, Kinari, Chawri | ×0.5, with food streets ×1.4 | A34 | |

### 5.3 Governor rules

1. **Cap**: visible agents ≤ 600 (750 on event). If above, drop LOD3 and LOD4 first, then thin crowd by despawning the farthest first.
2. **Density smoothing**: spawn or despawn only out of view (behind camera, around corners, inside doors, up stairs, from metro entrances, rickshaw stops).
3. **Time continuity**: persistent named NPCs follow their schedule regardless of player presence (fast-forward to correct location on stream-in).
4. **Weather response**: StateTree transitions `Shelter`, `OpenUmbrella`, `PutOnMask`, `WearPoncho` trigger from weather state with randomised delay (2-20 s) so reactions aren't synchronised.
5. **Special-event rows**: prayer rows spawn as a **pre-baked formation actor** (row, mat, rows in rows) with 1 shared animation and per-agent offset, only activated when the player is within 120 m.
6. **Accessibility and difficulty**: provide a "crowd density" setting (Low 50%, Medium 75%, High 100%) and a "reduce motion blur" option, helping 3070 players.

---

## 6. Open items and next steps

1. Obtain DMRC station-wise ridership for Chandni Chowk, Chawri Bazar, Jama Masjid, Lal Qila (RTI or DMRC site) [U].
2. Get Census 2011 ward-level data (Chandni Chowk, Ballimaran, Sadar Bazar, Jama Masjid wards, Delhi Gate) from the District Census Handbook, and MCD 2022 ward delimitation for residents. [U]
3. Verify current Delhi Police and Delhi Traffic Police uniform and any 2025-2026 changes with reference photos. [U]
4. Verify market weekly closing days and Red Fort closed day; verify festival dates (Eid al-Adha 2026, Diwali 8 Nov 2026) against an official calendar. [U]
5. Do a **field day**: 2 hours at each of the five street types in the five time slots to calibrate §1.3 counts against real counts (clicker counts per 100 m).
6. Commission 6 sensitivity reviews (§3.4) before first public build.
7. Make a Phase 2 cast delta (GK-1/2, Lajpat Nagar, Sarojini Nagar) list.
8. Prototype the **Spawn Governor**, appearance fragment and 12 archetypes in the 500 m vertical slice before building all 52.

---

## 7. Sources

Web results consulted (10 Nov 2026). Several are secondary or promotional, which is flagged above.

- Chandni Chowk footfall and redevelopment: [Realty N More](https://realtynmore.com/chandni-chowk-continues-to-rule-delhis-commercial-market-roster-for-decades/), [Omaxe blog](https://www.omaxe.com/blog/redevelopment-of-chandni-chowk-to-revive-its-glory-as-commercial-real-estate-hotspot/), [Business World](https://businessworld.in/article/omaxe-chowk-having-wholesale-retail-stores-food-court-to-comes-up-at-chandni-chowk-321942), [Retail4Growth](https://www.retail4growth.com/news/omaxe-chowk-is-the-new-buzz-in-delhis-oldest-retail-market-5898), [The Better India](https://thebetterindia.com/157580/chandni-chowk-delhi-news), [ThePrint, redevelopment](https://theprint.in/india/delhis-chandni-chowk-gets-new-look-a-restoration-of-not-just-facade-but-400-yr-old-legacy/697319/)
- Red Fort blast trade impact: [Tribune India](https://www.tribuneindia.com/news/delhi/red-fort-blast-chandni-chowk-market-has-taken-rs-400-cr-hit-mp-khandelwal/amp); peak-season trade: [Tribune India](https://www.tribuneindia.com/news/delhi/trade-takes-a-hit-at-chandni-chowk-during-peak-season/amp)
- Population: [Wikipedia, Central Delhi district](https://en.wikipedia.org/wiki/Central_Delhi_district); [IIPA paper on walled city](https://www.iipa.org.in/cms/public/uploads/367121667465530.pdf); [IRD study, walled-city density](https://horizon.documentation.ird.fr/exl-doc/pleins_textes/divers14-11/010023836.pdf); [Delhi Planning Dept demographic profile](https://delhiplanning.delhi.gov.in/sites/default/files/Planning/2012-13/es_2012-13_demographic_profile.pdf); [Wikipedia, Shahjahanabad](https://en.wikipedia.org/wiki/Shahjahanabad)
- Metro: [Wikipedia, Chandni Chowk metro](https://en.wikipedia.org/wiki/Chandni_Chowk_metro_station_(Delhi)), [Wikipedia, Chawri Bazar metro](https://en.wikipedia.org/wiki/Chawri_Bazar_metro_station), [PSU Connect, 82.45 lakh record](https://www.psuconnect.in/psu-news/dmrc-records-highest-ever-82-45-lakh-passenger-boardings-in-a-single-day), [UNI, 2025 journeys](https://www.uniindia.com/delhi-metro-crosses-record-235-crore-passenger-journeys-in-2025/india/news/3774805.html), [Millennium Post, 235.8 crore](https://www.millenniumpost.in/delhi/delhi-metro-ridership-rises-to-2358-crore-in-2025-network-remains-largest-in-india-651962)
- Rail: [Wikipedia, Delhi Junction](https://en.wikipedia.org/wiki/Delhi_Junction_railway_station), [Wikipedia, New Delhi station](https://en.wikipedia.org/wiki/New_Delhi_railway_station)
- Jama Masjid and Eid: [Britannica](https://www.britannica.com/place/Jami-Masjid-mosque-Delhi-India), [Deccan Herald, Eid prayers amid tight security](https://www.deccanherald.com/amp/story/india%2Fdelhi%2Fdevotees-offer-prayers-at-delhis-jama-masjid-on-eid-amid-tight-security-3470239), [Tribune, Eid celebrations](https://www.tribuneindia.com/news/delhi/bonhomie-prayers-mark-eid-celebrations), [NewKerala, Eid-ul-Fitr 2026](https://www.newkerala.com/news/a/eid-ul-fitr-2026-devotees-gather-delhis-jama-masjid-offer-235.htm), [NewKerala, police tighten security](https://www.newkerala.com/news/a/delhi-police-tightens-security-around-jama-masjid-ahead-146.htm), [Babushahi, Eid al-Adha](https://babushahi.com/full-news.php?id=205401), [ETV Bharat, Bakrid](https://www.etvbharat.com/en/!state/people-from-all-walks-of-life-offer-prayers-on-bakrid-across-delhi-enn25060701603)
- Porters and Khari Baoli: [Wikipedia, Khari Baoli](https://en.wikipedia.org/wiki/Khari_Baoli), [Scroll, Khari Baoli spice workers](https://scroll.in/magazine/880172/scroll_in), [Roads & Kingdoms](https://roadsandkingdoms.com/2015/relying-on-the-kindness-of-strangers-in-asias-largest-spice-market/), [Dodho, Mattia Mignani: Khari Baoli, the invisible engine](https://www.dodho.com/?p=173656), [The Quint, seasoned workers of Khari Baoli](https://www.thequint.com/photos/the-seasoned-workers-of-khari-baoli-asias-largest-spice-market), [Outlook Traveller on Khari Baoli](https://www.outlooktraveller.com/amp/story/News/experiences/shopping/all-about-khari-baoli-from-a-mughal-stepwell-to-a-spice-market), [This Decisive Moment, "The Porter"](https://www.thisdecisivemoment.com/moments-1/2020/5/5/the-porter)
- Project docs: `implementation.md` §3-§6, §10; `world/delhi/old_delhi_pois.csv`.
- Engine and tooling facts (MetaHuman licence, Mass, MetaHuman Crowd status) come from `implementation.md` and Epic documentation as understood at my knowledge cutoff; re-verify against current Epic docs and the Fab EULA before purchasing.
