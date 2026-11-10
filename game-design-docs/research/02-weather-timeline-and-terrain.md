# 02 - Weather and Time Timeline (real data) + UE 5.8 Terrain and Base Graphics Spec

Researched 10 Nov 2026. Builds on `implementation.md` (§3.7 weather subsystem, §4.3 presets, §8 budget), `world-design-pipeline.md` (terrain step) and `realism/graphics-and-lighting.md` (PBR, exposure). It does not repeat them.

**Confidence tags used below.** `[V]` = verified in a fetched or searched source this session (URL in §Sources). `[D]` = derived by me from verified numbers, or computed (math shown). `[E]` = engineering estimate or domain knowledge, not verified here; check before relying on it. Where sources disagreed, both numbers are given.

---

# PART A - WEATHER AND TIME

## A1. Month-by-month Delhi table

Station: Safdarjung (IMD, city reference) and Palam (airport, runs hotter and windier; it is where fog RVR and squall gusts are measured).

### A1.1 Climatology (IMD-derived 1991-2020 normals, Safdarjung) `[V]`

| Mon | Mean max C | Mean daily min C | Rain mm | Rainy days | RH 17:30 % | Record high C | Record low C |
|---|---|---|---|---|---|---|---|
| Jan | 20.1 | 7.5 | 19.1 | 1.7 | 57 | 32.5 | -0.6 |
| Feb | 24.2 | 10.6 | 21.3 | 1.5 | 46 | 34.1 | 1.6 |
| Mar | 29.9 | 15.6 | 17.4 | 1.7 | 37 | 40.6 | 4.4 |
| Apr | 36.5 | 21.3 | 16.3 | 1.0 | 25 | 45.6 | 10.7 |
| May | 39.9 | 25.8 | 30.7 | 2.7 | 28 | 47.2 | 15.1 |
| Jun | 39.0 | 27.7 | 74.1 | 4.8 | 43 | 46.7 | 17.6 |
| Jul | 35.6 | 27.5 | 209.7 | 9.7 | 63 | 45.0 | 20.3 |
| Aug | 34.2 | 26.7 | 233.1 | 10.2 | 68 | 42.0 | 20.7 |
| Sep | 34.1 | 25.0 | 123.5 | 5.5 | 60 | 40.6 | 16.1 |
| Oct | 33.0 | 19.5 | 15.1 | 0.8 | 47 | 39.4 | 9.4 |
| Nov | 28.4 | 13.0 | 6.0 | 0.4 | 52 | 36.1 | 3.9 |
| Dec | 22.8 | 8.4 | 8.1 | 0.6 | 59 | 30.0 | 0.0 |

Year: 774.4 mm, 40.6 rainy days. Note the RH column is the 17:30 IST reading; morning RH (08:30) is far higher in winter and monsoon (fog and dew), roughly 80-95 % `[E]`. Normal monsoon-season (Jun-Sep) rain is quoted as 640.4 mm (Skymet/IMD-based) or 629.7 mm (IMD 2026 release) depending on baseline year.

### A1.2 Sun geometry (Old Delhi 28.656 N, 77.232 E, IST) `[D]`

Computed with NOAA-style solar position equations (accurate to about +/-2 min and +/-0.3 deg; no atmospheric refraction beyond the standard 90.833 deg zenith). Use for the sun-sky rig; swap in IMD/timeanddate tables if you want civil-calendar exactness.

| Date (15th) | Sunrise | Sunset | Day length | Solar noon (IST) | Noon elevation | Noon shadow / object height |
|---|---|---|---|---|---|---|
| Jan | 07:14 | 17:44 | 10.5 h | 12:29 | 40.1 | 1.19 |
| Feb | 07:00 | 18:10 | 11.2 h | 12:35 | 48.4 | 0.89 |
| Mar | 06:32 | 18:29 | 11.9 h | 12:30 | 58.9 | 0.60 |
| Apr | 05:56 | 18:46 | 12.8 h | 12:21 | 70.8 | 0.35 |
| May | 05:30 | 19:03 | 13.6 h | 12:17 | 80.0 | 0.18 |
| Jun | 05:22 | 19:19 | 14.0 h | 12:21 | 84.6 | 0.10 |
| Jul | 05:32 | 19:21 | 13.8 h | 12:26 | 83.0 | 0.12 |
| Aug | 05:49 | 19:01 | 13.2 h | 12:25 | 75.6 | 0.26 |
| Sep | 06:05 | 18:27 | 12.4 h | 12:16 | 64.7 | 0.47 |
| Oct | 06:20 | 17:52 | 11.5 h | 12:06 | 53.1 | 0.75 |
| Nov | 06:43 | 17:28 | 10.7 h | 12:05 | 43.0 | 1.07 |
| Dec | 07:06 | 17:26 | 10.3 h | 12:16 | 38.1 | 1.27 |

Anchors: 21 Jun noon elevation 84.8 (sun 5 deg from zenith, almost no shadows); 20 Mar and 23 Sep about 61; 21 Dec 37.9. Earliest sunset about 17:24 (1 Dec); latest sunrise about 07:14 (early-mid Jan); the evening is shortest in the first half of Dec. **Design consequence:** in Nov-Jan a 4 m wide gali with 12 m walls gets zero direct sun (the sun at 38-43 deg needs a gap/height ratio above 1.1:1), so smog scenes in the lanes are lit by sky and bounce only; in May-Jun the noon sun is nearly overhead and reaches the lane floor for 2-3 h.

### A1.3 Observed 2025 and 2026 and notable events

Columns: temperature/rain, AQI and visibility, dust/storm, events. "mm" values are Safdarjung unless stated.

| Month | 2025 observed `[V]` | 2026 observed `[V]` | AQI (CPCB/CAQM monthly mean unless stated) | Fog/smog/visibility | Dust/storm | Notable |
|---|---|---|---|---|---|---|
| **Jan** | AQI mean 306 | Mean min 2.9 C on 15 Jan (IMD; press said 16 Jan); Palam 2.3 C; cold wave 11-15 and 26 Jan; very dense fog 18 Jan; AQI mean 307 (2nd best Jan in 5 yrs); AQI 386 on 2 Jan | 250-400 typical; mean 306-307 | Normal foggy days Jan: 21. Palam/Safdarjung 500 m at 05:30 on 16 Jan; 50 m reported in the worst spells; CAT III flight ops 1-2 Jan and 16-18 Jan | none | Coldest morning in 3 yrs; IGI cancels 100+ flights/day in worst fog spells |
| **Feb** | - | Mean max 27.1 (+2.9), peak 32.5 on 28 Feb (8th highest Feb since 1951); rain 0.5 mm vs 21.3 normal (-98 %); dense fog 2-3 Feb, vis 100 m at Palam and Safdarjung on 3 Feb | about 150-250 `[D]` (Feb-Apr 2026 mean about 197 derived from Jan-May mean 211) | fog fades by mid-month | none | Winter cut short, warm dry Feb |
| **Mar** | - | 7 Mar 35.7 C = hottest first week of March in 5 decades; wetter spell in the second half (about "wettest March in 3 yrs" per press, unverified month total) | about 150-220 `[D]` | little | few | Holi 4 Mar |
| **Apr** | Mean max 39.0 (+2.5); rain 0.7 mm vs 16.3; heatwave 7-9 Apr; peak 42.1 on 26 Apr; warmest April night in 6 yrs (27.2 C on 27 Apr) | Mean max 37.1 (+0.6); peak 42.8 on 25 Apr (press up to 44.5); western disturbance 28-29 Apr: rain, 30-60 km/h gusts and dust | about 150-250 `[D]`; "most polluted April in 3 yrs" in 2025 | haze + dust, vis 2-5 km `[E]` | Apr dust events (60 km/h, reduced vis) | Heat to 45 C reported late Apr |
| **May** | Wettest May since 1901: 185.9-188.9 mm (normal 30.7); no heatwave days | Rain 17.6 mm (-43 %); mean max 39.5 (-0.4); mean min 25.8; highest min 32.4 on 25 May (warmest May night in about 14 yrs); **heatwave days 18, 19, 20, 21, 24, 27 May**; warm nights 21 and 25 May; Ridge 46.5 C on 19 May (Safdarjung 43.4-45.1, sources disagree); record power demand 8,439 MW on 25 May | 157 (2026) vs 170 (2025); 85 on 31 May after rain (cleanest May in 5 yrs) | heat haze, vis 3-6 km `[E]` | **1 May squall: Palam 96 km/h, Safdarjung 80 km/h**; storms 14-23 May | Bakrid 28 May |
| **Jun** | Monsoon reached Delhi 29 Jun (normal 27 Jun) | Rain 41.8 mm (-44 % vs 74.1); mean max 39.2 (+0.2); peak 43.5 on 9 Jun; **dust storms 15 Jun (Palam 91-93 km/h) and 23 Jun (Palam 91 km/h, Safdarjung 52; 40 C to 29 C in minutes, IMD red alert)**; cool start to June | no verified monthly mean | dust haze, thunderstorm outflow | 2+ major aandhi | Monsoon delayed |
| **Jul** | about 259 mm `[V weak; one blended source]` | **Onset 2 Jul** (normal 27 Jun; IMD declared despite light rain); rain 258.0 mm (+23 % vs 209.7); 9 Jul: Safdarjung 72.6 mm/24 h, Mayur Vihar 164 mm | 117.8 (2026) vs 78.4 (2025) | low cloud, humid | few | Kanwar Yatra 30 Jul-11 Aug |
| **Aug** | About 400 mm (399.8-400.8), wettest Aug in 15 yrs; peak day about 80 mm on 15 Aug | 354.1-381.7 mm; wettest first week of Aug in 16 yrs (127 mm in 7 days); **25 Aug: 99.1 mm in 24 h**; waterlogging, traffic jams | worse than 2025 (per independent analysis) | low | - | Independence Day 15 Aug |
| **Sep** | 136.1 mm (normal 123.5); **withdrawal 24 Sep** (earliest since 2012); season total 902.6 mm vs 640.4 normal (+41 %) | Season 728.6 mm vs 629.7 to 23 Sep (+16 %); **withdrawal 23 Sep** (normal 25 Sep); first late-Sept rain (4.3 mm 27 Sep) since 2019; cool late-Sept spell | 102 (2026, lowest Sep in 5 yrs) vs 105 | - | - | |
| **Oct** | 89.3 mm (normal 15.1; 3rd highest Oct since 1961); mean max 31.5 (-1.5, coolest Oct since 2022); first min < 20 C on 10 Oct | Min 17.1 C on 19 Oct (season low then); no "good" AQI day; mean AQI 166 (CAQM) or 191.5 for 1-29 Oct (another analysis); max 283 on 21 Oct | 166-192 (2026); 2025 PM2.5 mean 107 ug/m3; Diwali morning 2025 AQI 417 | haze begins late Oct | none | Dussehra 20 Oct, Karwa Chauth 29 Oct |
| **Nov** | AQI mean **357** (2024: 374; 2023: 366); 24 "very poor" + 3 "severe" days (PTI/CPCB) or 23 + 6 (CREA, PM2.5-based); severe 11-13 Nov; **Chandni Chowk station 445 and Wazirpur 447 on 14 Nov**; PM2.5 mean 215 ug/m3 | 6 Nov: AQI 363 ("very poor"), IQAir ranks Delhi most polluted major city; **Diwali 8 Nov**; green crackers only 7-8 Nov, full ban from 9 Nov; Chhath 15 Nov (forecast) | 300-450 typical; 350-370 mean | Normal foggy days Nov: 12; smog visibility 600-800 m typical, 100-300 m worst (see A3) | none | Wedding season opens 21 Nov |
| **Dec** | AQI mean 349-351 (worst Dec since 2018); peak **461** (mid-Dec); coldest Dec day in 6 yrs: Tmax 14.2 C on 31 Dec; fog from 15 Dec: CAT III ops, 118-152 flight cancellations on bad days; Safdarjung 100-200 m on 18 and 30 Dec | - | 300-460 | Normal foggy days Dec: 19; zero visibility spells of up to 9 h reported at Palam in a prior season (Jan 2025) | none | Marriage muhurats pause 13 Dec-14 Jan (Kharmas) |

**Yamuna and floods** `[V]`: Old Railway Bridge (ORB) gauge marks are warning 204.5 m, danger 205.33 m, evacuation 206 m. All-time high 208.66 m on 13 Jul 2023 (previous 207.49 m in 1978). In late Aug-early Sep 2025 it exceeded 207 m and peaked at 207.48 m; Okhla barrage gates opened, about 10,000 people moved to tents/shelters. I found **no 2026 ORB readings** (check the CWC flood portal); 2026 monsoon was near-normal, so assume no major flood. The gauge datum is height above sea level, so it also tells you the floodplain geometry: the east edge of Old Delhi near the Red Fort and the river bank sits within a few metres of 205-210 m (see B1).

**Fog climatology**: IMD-type normals are about 12 foggy days in Nov, 19 in Dec, 21 in Jan `[V from press baseline]`. "Dense" (50-200 m) and "very dense" (<50 m) are subsets; use the Markov table in A4 rather than these totals directly.

**Dust storms**: a May-June phenomenon plus April. 2026 gave at least four damaging squalls (about 25 Apr, 1 May, 15 Jun, 23 Jun) with Palam gusts of 91-96 km/h and Safdarjung 50-80 km/h. Frequency per month for the game is an estimate `[E]`: Apr 2-3, May 3-4, Jun 2-3, Mar/Jul 0-1 big events.

## A2. 2026 festival and events calendar (look and crowd effects)

Dates that depend on moon-sighting or panchang vary by a day between sources; I give the primary and flag conflicts. "Layer" = World Partition Data Layer proposed for it.

| Date 2026 | Event | Delhi/Old Delhi look and crowd effect | Layer |
|---|---|---|---|
| 13-14 Jan | Lohri, Makar Sankranti `[E]` | bonfires, kites in some neighbourhoods | minor |
| 23 Jan `[E]` | Basant Panchami | yellow, kites | minor |
| 26 Jan | Republic Day parade, Kartavya Path | road closures in Central Delhi, security cordons, tricolour on stalls; Old Delhi is outside the parade route but sees checkpoints `[E]` | `L_RepublicDay` |
| about 19 Feb-19 Mar | Ramzan (30 days, derived from Eid dates) `[D]` | Jama Masjid / Matia Mahal / Gali Kababian night crowds, iftar stalls, sheer khurma, lights, evening 17:45-18:30 sunset break | `L_Ramzan` (big) |
| 3 Mar / **4 Mar** | Holika Dahan / Holi `[V]` | bonfire 3 Mar evening; colour-stained walls, gulal in air, water balloons, streets empty by afternoon, shops shuttered | `L_Holi` |
| **20 or 21 Mar** | Eid al-Fitr `[V: 20 Mar bank list, 21 Mar in some states]` | Jama Masjid overflow prayers, crowds in galis, seviyan, new clothes, Meena Bazaar late-night | `L_Eid` |
| about 26 Mar `[E]` | Ram Navami | processions, saffron flags | minor |
| 14 Apr `[E]` | Baisakhi | Sis Ganj Gurdwara crowds | minor |
| Mar-May | IPL 2026, Delhi Capitals at Arun Jaitley Stadium: home games 4 Apr (MI), 8 Apr (GT), 5 more `[V]` | evening match traffic near Delhi Gate/ITO, crowd chatter, TV/phone audio | `L_Cricket` |
| 10 Feb-Mar | T20 World Cup, Delhi hosts 6 matches incl. India v Namibia 12 Feb `[V]` | same | `L_Cricket` |
| 29 Apr | MCD mayor election `[V]`; ward-committee polls: 23 May or 15 Jul (sources conflict) | banners and rally traffic only; no general election in 2026. Delhi assembly election was Feb 2025 `[E]`; next MCD general around 2027 `[E]` | `L_Politics` |
| **28 May** | Eid al-Adha / Bakrid (Jama Masjid Shahi Imam announcement; 27 May in J&K) `[V]` | goat market at Urdu Park and Matia Mahal from about 20 May, qurbani, red-stained gutters (handle sensitively), 40-46 C heat | `L_Bakrid` |
| about 25-26 Jun `[E]` | Muharram/Ashura | tazia processions | minor |
| **30 Jul-11 Aug** | Kanwar Yatra / Sawan, Shivratri 11 Aug `[V]` | NH-44/Ring Road diversions, saffron-clad kanwariyas, DJ trucks, shut non-veg shops on routes, monsoon | `L_Kanwar` |
| 31 Jul-31 Aug | Delhi Premier League (cricket) `[V]` | minor | - |
| **15 Aug** | Independence Day, Red Fort address; **kite day** `[V date]` | Red Fort perimeter cordon (Lahori Gate side), heavy security in Chandni Chowk, rooftop kites, cut-kite lines hurt scooter riders; tricolour bunting | `L_Aug15` |
| 28 Aug `[V single source]` | Raksha Bandhan | sweet shops, rakhi stalls | minor |
| 4 Sep `[V single source; conflicts]` | Janmashtami | temple lights, dahi handi at some places | minor |
| 13-17 Sep | India v Afghanistan T20Is at Arun Jaitley `[V]` | cricket traffic | `L_Cricket` |
| about 14 Sep `[E]` | Ganesh Chaturthi | minor in Delhi | - |
| **11 Oct** | Navratri begins `[V]` | fasting-food stalls, garba/Durga pandals, red-yellow decoration | `L_Navratri` |
| 19-20 Oct | Durga Ashtami (Durga Puja peak) `[V]` | pandals (CR Park, not Old Delhi), bhog queues | `L_Puja` |
| **20 Oct** | Dussehra: Ramlila and effigies of Ravan at Red Fort lawns/Ramlila Maidan `[V date; effigy sites E]` | huge evening crowds, fireworks, effigy burning at dusk | `L_Dussehra` |
| 29 Oct | Karwa Chauth `[V]` | mehndi and bangle stalls, evening moon rituals | minor |
| 6 Nov | Dhanteras `[V]` | jewellers, utensil and silver markets (Dariba) at peak crowd | `L_Diwali` |
| **8 Nov** | **Diwali** `[V]` | markets lit with lakhs of fairy lights, diyas, rangoli, marigold; firecrackers on Diwali night (certified green crackers allowed only 7-8 Nov, ban again from 9 Nov) so smoke build-up; peak shopping 1-7 Nov in Chandni Chowk, Sadar, Khari Baoli | `L_Diwali` |
| 10 or 11 Nov | Bhai Dooj `[V conflicts 9-11 Nov]` | sweets | minor |
| **13-16 Nov** | **Chhath Puja**: Nahay Khay 13, Kharna 14, evening arghya **15 Nov**, morning arghya 16 Nov `[V; sources differ]` | Yamuna ghats, decorated soops, devotees knee-deep in river water (polluted, foam), sugarcane and bamboo baskets, loud devotional songs; Delhi govt announces a public holiday each year | `L_Chhath` (on the Yamuna layer) |
| **21 Nov onward** | Wedding season: Dev Uthani Ekadashi 20 Nov; muhurats 21, 24, 25, 26 Nov; Dec 1-6 and 11-13 (sources differ on exact days); pause 13 Dec-14 Jan (Kharmas) `[V]`. Spring season about Jan-Mar and again from mid-Jun/Jul-pre-Chaturmas; none Aug-Oct (Chaturmas) | baraat with band, dhol, mare, lit chandeliers on carts, wedding-card shops (Chawri Bazar) packed, fireworks at night, traffic jams | `L_Wedding` |
| 24 Nov `[E]` | Guru Nanak Jayanti (Gurpurab) | Sis Ganj Gurdwara lit, nagar kirtan, langar | minor |
| 25 Dec | Christmas | Central Baptist Church, minor | - |

**Take-aways:** (1) the densest "look-changing" window is **20 Oct to 24 Nov** (Dussehra, Karwa Chauth, Dhanteras, Diwali, Chhath, wedding season, Gurpurab), and it coincides with the worst smog; (2) summer festivals (Bakrid, Kanwar) land in heat/monsoon; (3) Ramzan and Holi are the strongest spring crowd/colour events; (4) no general election or Test match was found for 2026.

## A3. AQI, smog and visibility mapping for the weather system

Delhi uses the Indian NAQI (PM2.5 breakpoints in ug/m3, 24-h): 0-30 good (AQI 0-50); 31-60 satisfactory (51-100); 61-90 moderate (101-200); 91-120 poor (201-300); 121-250 very poor (301-400); >250 severe (401-500) `[E; NAQI standard]`. The first three months of 2026 and Nov 2025 give these sanity anchors `[V]`: AQI 363 (pre-Diwali), 417 (Diwali morning 2025), 445-447 (14 Nov 2025 at Chandni Chowk), 461 (Dec 2025 peak).

Drive the sky from AQI and a fog flag rather than from visibility alone:

| AQI | PM2.5 ug/m3 | Visibility (m) target `[E]`, consistent with implementation.md | Sun disc | UE Mie scale (below) |
|---|---|---|---|---|
| 50-100 | 30-60 | 6000-10000 | crisp | 1x |
| 200 | about 90 | 3000-5000 | soft | 3x |
| 300 | about 120 | 1500-2500 | orange-white disc visible | 5x |
| 400 | about 250 | 600-1000 | dull orange disc | 8x |
| 450+ | >300 | 300-600 | red disc barely seen | 12x |
| Fog or fog+smog (RH>95 %) | n/a | 50-200 (dense), <50 (very dense) | hidden | fog layer on top |

Extinction from visibility (Koschmieder, 2 % contrast): beta = 3.912 / V per metre. V = 800 m gives beta = 4.9e-3 /m; V = 100 m gives 0.039 /m. Because UE's ExponentialHeightFog density is not in physical units, build a **visibility calibration level**: place dark cubes at 50, 100, 200, 500, 1000, 2000 m and tune `FogDensity`/Mie scale until the cubes drop to 2 % contrast at the target V. Store the resulting curve as a `UCurveFloat` (AQI -> fog density, Mie scale, inscatter colour).

## A4. Recommended in-world timeline and clock

### A4.1 When to set the story

**Primary campaign window: Sat 24 Oct to Tue 24 Nov 2026 (31 days).** Reasons: (a) it is the 'now' of the project and the richest festival stretch (A2); (b) Old Delhi is at its most distinctive (Diwali lighting, Dhanteras silver, wedding-card trade, Chhath on the river); (c) smog and winter-sun geometry are consistent with the Phase 1 preset in implementation.md §4.3 (AQI 350-450, visibility 100-800 m); (d) the weather space is narrow (no monsoon, no real dust storm), so one tuned preset family covers it, which de-risks the first playable. Set pieces: 20 Oct Dussehra (prologue, effigy burning at dusk), 29 Oct Karwa Chauth, 6 Nov Dhanteras, **8 Nov Diwali (climax)**, 13-16 Nov Chhath, 21 Nov first wedding.

**Season packs (DLC or later Data Layers), each pinned to a real reference week so tuning has ground truth:**

| Pack | Reference days (real) | Use |
|---|---|---|
| Winter fog and cold wave | 11-18 Jan 2026 (min 2.9 C on 15 Jan; very dense fog 18 Jan; CAT III flights) | fog preset, breath vapour, braziers, 4 C mornings |
| Spring | 4 Mar (Holi), 20-21 Mar (Eid), Ramzan 19 Feb-19 Mar | colour, crowds |
| Heat and dust storm | 19-27 May (46 C Ridge, heatwave days 18-21, 24, 27), 1 May squall (96 km/h), 15 Jun and 23 Jun aandhi | the "clear summer" and "dust storm" states |
| Monsoon | 9 Jul (72.6 mm), 25 Aug (99.1 mm), 15 Aug kites + Independence Day | rain and waterlogging; floods only as an event (Yamuna 207+ m as in 2025) |

This delivers all four Phase 1 states (clear summer, winter smog, monsoon, dust storm) while keeping the main story in a single season.

### A4.2 Clock speed

- **Day-night:** 1 real second = 30 game seconds in daylight (about 11 h of Nov daylight = 22 real min), 1 real second = 60 game seconds at night (about 13 h = 13 real min), so a full Delhi day is about 35 real minutes. Smoothstep the rate through dawn and dusk (05:00-07:30, 17:00-19:00) so the 40-min "golden smog sunset" is not skipped. Expose a debug scalar and a pause. Calendar date is advanced by story beats (and by sleep/skip), not by the clock, so a 31-day campaign is not forced into 18 hours of waiting.
- **Weather dwell:** a state lasts at least 10 real minutes except storms (see A4.4).
- **Sun position:** drive the directional light from the table in A1.2 via the GeoReferencing origin (28.656 N, 77.232 E, UTC+5:30, no DST). Do not use UE's default "Sun Position Calculator" time zone default; set 5.5 explicitly.

### A4.3 Weather states and data-driven climatology

States (daily dominant): `CLR` clear, `HAZE` hazy sun, `OVC` overcast dry, `RAIN`, `STORM` (thunderstorm or aandhi), `FOG` (dense morning fog), `SMOG` (persistent AQI>300). Probabilities are per-day climatology derived from the normals in A1.1 (rainy-day fractions) and fog/AQI facts above; **dust/storm shares are estimates `[E]` and must be calibrated** against IMD's thunderstorm/dust-storm day counts. Rows sum to 1.

| Mon | CLR | HAZE | OVC | RAIN | STORM | FOG | SMOG | Persistence rho (daily) |
|---|---|---|---|---|---|---|---|---|
| Jan | .22 | .15 | .10 | .06 | .00 | .30 | .17 | 0.80 |
| Feb | .35 | .20 | .10 | .05 | .02 | .15 | .13 | 0.65 |
| Mar | .50 | .20 | .10 | .06 | .06 | .03 | .05 | 0.50 |
| Apr | .55 | .20 | .06 | .03 | .12 | .00 | .04 | 0.50 |
| May | .45 | .27 | .05 | .08 | .15 | .00 | .00 | 0.50 |
| Jun | .25 | .25 | .22 | .16 | .12 | .00 | .00 | 0.55 |
| Jul | .08 | .17 | .42 | .31 | .02 | .00 | .00 | 0.65 |
| Aug | .08 | .17 | .40 | .33 | .02 | .00 | .00 | 0.65 |
| Sep | .22 | .28 | .30 | .18 | .02 | .00 | .00 | 0.60 |
| Oct | .45 | .35 | .08 | .03 | .00 | .00 | .09 | 0.65 |
| Nov | .17 | .20 | .03 | .02 | .00 | .20 | .38 | 0.80 |
| Dec | .15 | .15 | .03 | .02 | .00 | .35 | .30 | 0.80 |

**Transition rule (one tunable per month):** `P(next=j | cur=i) = rho * [i==j] + (1-rho) * pi_month[j]`, so the long-run frequency equals the table and episodes last about 1/(1-rho) days (about 5 days in Jan/Nov/Dec, matching the 24-day "very poor" streak when combined with the AQI regime variable below). Add hard constraints: after `STORM` the next day is `HAZE`/`CLR`/`RAIN` only; `RAIN` cannot occur on a `SMOG` day (a washout day first resets AQI to about 100-150); `FOG` and `SMOG` only between Oct 25 and Feb 25.

**Year-level "AQI regime"** (rolled once per campaign start): `Clean` 25 % (Nov mean about 250, SMOG weight x0.5), `Typical` 50 % (Nov mean about 350), `Bad` 25 % (Nov mean about 400, weight x1.4, up to 24-day streaks). Anchor: Nov 2023/2024/2025 means 366/374/357; Oct 2026 was the best October in eight years (166).

**Intraday rules (the part that makes it feel real):**

| State | Rule |
|---|---|
| FOG | forms 22:00-02:00, dense 03:00-09:00, lifts to haze by 10:00-12:00, visibility 50-200 m dense, <50 m for the worst 3-9 h spells; sun appears as a white disc by 11:00; min temp 3-8 C |
| SMOG | AQI follows a diurnal curve: lowest 15:00-17:00, rising after sunset to peak 22:00-08:00 (mixing-layer collapse) |
| STORM (aandhi) | trigger 15:00-21:00 (pre-monsoon) or after 14:00 on hot days (Tmax >= 40); warning 20-40 min of rising wind and orange sky, gust ramps 20-90 km/h in 5 min, temp falls 10-11 C in minutes (40 to 29 C observed 23 Jun 2026), duration 20-60 min, may end in 5-15 mm of rain |
| RAIN (monsoon) | bursty: 2-4 episodes of 20-90 min, 5-25 mm/h; heavy burst can reach 70-100 mm per 24 h (9 Jul, 25 Aug 2026); waterlogging after 30 mm in 3 h |
| HEAT | Tmax 43-46 C, loo (hot west wind 20-40 km/h) 12:00-17:00, nights not below 28-32 C (32.4 C on 25 May) |
| CLEAR winter | after rain: crisp, AQI 80-150 for 1-2 days |

**Implementation:** a CSV `DT_ClimateMonth` (rows = month, columns = the table above plus tmax, tmin, rh, sunrise/sunset offsets) imported as a Data Table; `UWeatherSubsystem` samples a seeded RNG at game-day rollover; a second layer picks the intraday timeline. Write the generated state sequence to the save game, so the same seed gives the same Diwali. Outputs go to `MPC_Weather` (Wetness, PuddleLevel, DustAmount, SmogDensity, WindDir, WindStrength per implementation.md §3.7) plus new scalars `AQI` and `Visibility`. Add a **reference-day override** (debug and for the season packs): force any of the dates in A4.1 so you can compare the render with archive photos.

---

# PART B - UE 5.8 TERRAIN AND BASE-GAME GRAPHICS

UE 5.8 launched June 2026 and is the last major UE5 release. What I verified for 5.8: **Mesh Terrain is Experimental**; **Lumen Medium Quality ("Lumen Lite") is Beta**, about 2x faster than High, based on irradiance fields with probe occlusion, enabled by setting GI and reflection quality to Medium (`sg.GlobalIlluminationQuality 1`, `sg.ReflectionQuality 1`); **MegaLights is Production-Ready**; Lumen applies height fog to reflection hits by default (`r.Lumen.HeightFog 1`). The official release-notes page would not render for my fetcher, so check it in the editor for the CVar list.

## B1. Terrain from the Copernicus 30 m DEM

### B1.1 What the data is (and is not good for)

File: `world/delhi/source/Copernicus_DSM_COG_10_N28_00_E077_00_DEM.tif`, 41.7 MB, the 1 deg x 1 deg tile covering 28-29 N, 77-78 E (3600 x 3600 samples at 1 arc-second). I did not open it (no GDAL on this machine); open it before trusting any value below.

- Ground sampling at 28.65 N: **30.9 m N-S x 27.1 m E-W** `[D: 30.87 m x cos(28.65 deg)=0.878]`. The Phase 1 box (0.02 deg x 0.03 deg, about 2.2 x 2.9 km) is only about **72 x 108 samples**. Real relief across the whole box is a few metres (Delhi is about 200-220 m; the Yamuna low-flow bed is about 198-200 m, the flood warning level at ORB is 204.5 m), which is smaller than the sensor's vertical noise (Copernicus GLO-30 spec: about 4 m absolute, 90 % `[E]`).
- It is a **DSM**: heights include rooftops and tree canopy. In Old Delhi's dense 8-15 m fabric, each 30 m cell is mostly roof. The signal you want (ground) is not recoverable from the raw tile. The Ridge (Kamla Nehru/Northern Ridge, NW of the box) and the Yamuna floodplain are the real relief features and are visible at 30 m.
- **Verdict:** use the DEM for the **regional shape** (Ridge, Yamuna valley, direction of drainage, flood geometry), not for streets. Street-level terrain must come from the road mesh, kerbs, plinth heights and OSM/survey data, as the pipeline doc's step 4 intends.

### B1.2 Getting bare earth (DTM)

1. **FABDEM** (Forest And Buildings removed Copernicus DEM, Hawker et al. 2022). Machine-learning corrections subtracted from GLO-30; 1 arc-second grid, 60 S-80 N. Reported mean absolute error vs Copernicus: 5.15 m to 2.88 m in forest, 1.61 m to 1.12 m in built-up areas `[V]`. Tile named `N28E077_FABDEM_V1-2.tif` by convention (1 deg tiles named by SW corner; I could not confirm the exact V1-2 file listing). **Licence: CC BY-NC-SA 4.0 in most listings (non-commercial; share-alike); the data.bris V1-0 record shows a different "Non-Commercial Government Licence"; for commercial use contact fabdem@fathom.global.** `[V]` Use FABDEM for prototyping and visual reference only, and plan a licence or an alternative before shipping.
2. **Own filtering (commercial-safe) on the Copernicus DSM**, which is what I would build:
   - Rasterise the 22,006 Overture building footprints (`old_delhi_chandni_chowk_overture_buildings.geojson`) and OSM landuse (`.osm`) to the DEM grid; dilate by 1 cell; set to NoData.
   - Fill by interpolation from the surrounding road/open-ground cells (GDAL `gdal_fillnodata` with 100 px search, or thin-plate spline), then low-pass (Gaussian sigma 1-1.5 cells) to a smooth surface.
   - For vegetation (Ridge, parks): apply a progressive morphological opening (Zhang 2003) or Cloth Simulation Filter, window 3-9 cells.
   - **Sanity bounds:** clamp so DTM <= DSM everywhere, and clamp slopes in the flat core to <= 1:300 (Old Delhi to the Yamuna falls only a few metres over 1.5 km).
3. **Calibrate** with known levels: ORB gauge thresholds above, the flood edge of 2023 (208.66 m reached Ring Road/ITO area) and, in Phase 1, spot heights from the Cesium/Google tiles used as an editor-only check (never shipped, per implementation.md §3.6).
4. Then add **designed micro-relief** by rule, not data: road crown (2-2.5 %), kerb 10-15 cm, drains 0.3-0.6 m, plinths 0.3-0.6 m above gali, slab bridges over drains.

Tools: install GDAL (OSGeo4W, or `conda install gdal rasterio`) to reproject (EPSG:4326 to EPSG:32643, UTM 43N, matching the GeoReferencing setup), clip, resample and write the 16-bit PNG/R16.

### B1.3 Landscape vs Mesh Terrain

| | Landscape (heightfield) | Mesh Terrain (5.8) |
|---|---|---|
| Maturity | production | **Experimental** (plugins `MeshPartition` + `MeshTerrain`; Epic KB 19 Mar 2026) `[V]` |
| Strength | simple, fast, RVT blending, splines/water, Nanite Landscape option | true 3D: overhangs, caves, arches; non-destructive; PCG-native `[V]` |
| Limits | 2.5D only | World Partition only (guardrails planned); converting back to heightfields not always possible; community reports of Lumen dark patches; RVT with overhangs "work in progress"; production readiness "too soon to tell" `[V]` |
| Delhi need | exactly right: flat plain, Yamuna bank, Ridge slope | not needed; only useful for quarry cuts, Ridge rock faces, or stepped river ghats |

**Decision: Landscape for Phase 1.** Keep a spike branch with Mesh Terrain for the Ridge and ghat steps only if a visual need appears; import path is "export the Landscape heightmap/layers as textures and apply with a TextureModifier on a MeshPartition grid" `[V]`.

### B1.4 Heightmap sizes and numbers

UE-recommended landscape sizes `[V, Epic Landscape Technical Guide]`: 8129 x 8129 (127 quads/section, 2x2 sections, 1024 components), 4033 x 4033 (63 quads/section, 2x2, 1024 components), 2017 x 2017 (256 comp.), 1009 x 1009 (64 comp.), 505 x 505, 253 x 253, 127 x 127. Maximum about 1024 components per landscape. Heights are 16-bit; stored range is -256 to +255.992 cm at Z scale 1, so **Z scale 100 gives +/-256 m (about 0.78 cm per step)**; step size = Z scale / 128 cm.

| Layer | Extent | Resolution | UE size | Z handling |
|---|---|---|---|---|
| **`LS_Phase1`** (playable base) | 4032 x 4032 m centred on 28.655 N, 77.230 E (covers the 2.9 x 2.2 km box plus margin) | **1 m/quad (XY scale 100)** | **4033 x 4033, 63 quads, 2x2 sections, 1024 comps** | landscape actor at Z = 21,000 cm (210 m); **Z scale 12.5** (range +/-32 m, step 0.098 cm) |
| `LS_Context` (Ridge, Yamuna, horizon) | 60 x 60 km around the city | 30 m (the native DEM) | 2017 x 2017 (256 comps) | Z scale 100 (+/-256 m) or the Ridge-range DTM; actor Z at 20,000 cm so Yamuna bed 198 m sits near zero |
| Optional hero-district upgrade | 1 km square | 0.25-0.5 m | 4033 | only where the surface is visible mud/sandstone |

Vertical datum: Copernicus heights are EGM2008 orthometric `[E]`; UE only needs consistency. Put real elevation in the UE Z via the actor offset and keep the GeoReferencing origin at the same lat/lon as the slice manifest. Resample the DTM into the 4033 grid with bicubic interpolation, **not** nearest, then bake the designed micro-relief in a second pass; keep the heightmap as a layer so it can be re-baked when the road network changes.

**World Partition settings:**
- Enable World Partition on the level; use the **World Partition Landscape** path (auto-splits into LandscapeStreamingProxy actors; do not hand-split).
- Streaming proxy size: **4 x 4 components (about 504 m at 126 m/component)** gives 8 x 8 = 64 proxies over 4 km; the building/prop runtime grid stays as in implementation.md §3.4 (cell 6400 cm, load range 19200-25600 cm). Landscape and props stream independently.
- Landscape LOD: `r.LandscapeLODBias 0`, Nanite Landscape enabled if it is stable in your 5.8 build (verify in the Landscape Mode panel; it cuts CPU cost and gives continuous LOD), otherwise `LandscapeLODDistributionScale 1.0`.
- Physical material layers: `PM_Asphalt`, `PM_Mud`, `PM_RedSandstone` per landscape layer so ballistics and footsteps (implementation.md §4.1) read them.
- For the floodplain: **Yamuna and canals as Water-plugin spline bodies** cut into the landscape with the water brush, low-flow level about 199-200 m, flood stage events driven by the weather subsystem (warning 204.5, danger 205.33, 2025 peak 207.48, record 208.66).
- Layers: keep **8 or fewer** (RVT-friendly): road dust, packed earth, dry grass, mud/monsoon slush, ash/soot, riverbank sand, leaf litter, gravel/ballast. Use **Runtime Virtual Texture** for landscape-to-asphalt/plinth blending (as implementation.md §4.6).

**Biomes for PCG (outside the dense core):** Northern Ridge = scrub with kikar/vilayati babool (Prosopis juliflora) and kachnar, rocky quartzite outcrops; Yamuna Khadar floodplain = mustard (Dec-Feb), wheat (Nov-Apr), vegetable plots, sand, reed beds, Chhath ghats; Old Delhi core = paving, no soil.

## B2. Base-game graphics checklist (concrete numbers)

Target from implementation.md §8: 60 fps (16.6 ms) at 1440p, TSR from about 1080p, RTX 3070 8 GB. Treat every value as a starting point and verify with `stat gpu` / Insights; items marked `[E]` are engineering estimates.

### B2.1 Project-level setup

`Config/DefaultEngine.ini` (starting block; verify each CVar exists in 5.8 via the console `?` autocompletion):

```ini
[/Script/Engine.RendererSettings]
r.DynamicGlobalIlluminationMethod=1      ; Lumen GI
r.ReflectionMethod=1                      ; Lumen reflections
r.Shadow.Virtual.Enable=1
r.AntiAliasingMethod=4                    ; TSR
r.DBuffer=1                               ; decals (implementation.md §4.6)
r.GenerateMeshDistanceFields=1            ; needed for Lumen software RT
r.Nanite.ProjectEnabled=1
r.AllowStaticLighting=0                   ; fully dynamic time of day
r.SupportSkyAtmosphere=1
r.SupportSkyAtmosphereAffectsHeightFog=1
r.VolumetricFog=1
r.VolumetricCloud=1                       ; keep the support flag; clouds are optional
r.Lumen.HardwareRayTracing=0              ; start software; test HWRT later (3070 supports it)
r.Lumen.TraceMeshSDFs=1
r.Lumen.HeightFog=1                       ; verified default in 5.8 per release notes

[/Script/Engine.StreamingSettings]
r.Streaming.PoolSize=3000                 ; implementation.md §8

[ConsoleVariables]
r.Nanite.Streaming.StreamingPoolSize=512  ; implementation.md §8
r.Shadow.Virtual.MaxPhysicalPages=4096    ; implementation.md §8
```

### B2.2 Lumen / Lumen Lite

| Setting | Value | Reason |
|---|---|---|
| Mode A (default) | Lumen software RT, GI + reflection quality `High` (sg.* = 2) | day exterior, quality |
| Mode B (performance) | **Lumen Medium ("Lumen Lite", Beta)**: `sg.GlobalIlluminationQuality 1`, `sg.ReflectionQuality 1` | about 2x cheaper than High per release notes `[V]`; use for lane interiors or if frame >16.6 ms |
| Scene capture | `r.Lumen.SceneLightingQuality` / `r.LumenScene.SurfaceCache.*` leave default | verify names in editor |
| Reflections | `r.Lumen.Reflections.MaxRoughnessToTrace 0.4`, `r.Lumen.Reflections.DownsampleFactor 2` `[E]` | hide cost on rough sandstone, keep wet roads sharp |
| Hardware RT | off at start; test `r.Lumen.HardwareRayTracing 1` on the 3070 for night wet streets only | 8 GB VRAM limit |
| Budget | 3.5 ms (implementation.md §8) | |
| Delhi-specific | thin walls (lakhori brick 22-30 cm) leak GI: keep wall thickness >= 15 cm in meshes and avoid single-sided planes; shutters should be opaque meshes | |
| Smog nights | Lumen is cheap-ish when visibility is 100-300 m because distant scene is hidden by fog | |

### B2.3 MegaLights (Production-Ready in 5.8)

Delhi nights have hundreds of shop lights, street lights, tube lights, fairy lights and braziers in view; MegaLights makes light count nearly free so every shop can carry real lights.

- Enable: Project Settings > Rendering > Direct Lighting > **MegaLights**; also enable **Support Hardware Ray Tracing** (recommended even if Lumen stays software) `[V]`.
- Per-light: set "Allow MegaLights" on shop/street/diwali lights; keep sun on VSM (`r.MegaLights.DirectionalLights 0`, the default) `[V]`.
- Tuning CVars `[V]`: `r.MegaLights.NumSamplesPerPixel` (start 2 to 4), `r.MegaLights.DownsampleMode` (half-res first), `r.MegaLights.Volume.GridPixelSize` and `r.MegaLights.Volume.GridSizeZ` for volumetric fog/translucency lighting, `r.MegaLights.Allow 0` to kill per device profile.
- Limits `[V]`: not with the forward renderer; Water, Volumetric Clouds and Heterogeneous Volumes do not receive MegaLights; alpha-masked surfaces use screen traces only (turn on `r.MegaLights.HardwareRayTracing.EvaluateMaterialMode 1` for wire meshes/tarps at a cost); front-layer translucent cost can double; many overlapping lights per pixel produce blur/ghosting (cluster Diwali strings into emissive meshes plus a few lights).
- Budget start: 2 ms `[E]` out of the 3 ms headroom; profile before committing.

### B2.4 Virtual Shadow Maps and Nanite

| Item | Value |
|---|---|
| VSM | `r.Shadow.Virtual.Enable 1`; physical pages `4096` (about 270 MB at 128x128 x 32 bit; reduce to 3072 if VRAM tight); `r.Shadow.Virtual.SMRT.RayCountDirectional 4`, `SamplesPerRayDirectional 2`, drop to 2/1 for performance `[E]`; keep Nanite foliage/WPO clipmaps invalidation low (animated tarps and wires cost) |
| Non-Nanite shadow caching | mark glass and translucent as no-cast; shutters as Nanite |
| Nanite | all opaque buildings; `r.Nanite.MaxPixelsPerEdge 1` (default) or 2 if over budget; Fallback target "Relative error 1 %" for kit pieces; mask Nanite **WPO only** where needed (tarps) because WPO invalidates VSM pages; displacement/tessellation only on paving and hero brick (budget 2 ms, regression UE-370671 in implementation.md); foliage Nanite for trees with WPO disabled beyond 30 m |
| Streaming | `r.Nanite.Streaming.StreamingPoolSize 512`; textures 2K by default; `r.Streaming.PoolSize 3000` |
| Instancing | ISM/HISM with Nanite for repeated kit; PCG outputs as instanced static meshes |

### B2.5 TSR and post

- `r.AntiAliasingMethod 4` (TSR); render scale `r.ScreenPercentage 75` at 1440p (about 1080p internal); `r.TSR.History.UpdateQuality 3`, `r.TSR.History.ScreenPercentage 100`.
- Thin wires and fine signage shimmer: keep overhead wires as meshes >= 1.5 cm radius with a slight emissive or "wire kit" material that fades to 3 cm at distance, or they will break up under TSR.
- Post: bloom intensity 0.2-0.4, **vignette 0.2**, film grain 0.03-0.06 (after tone map), motion blur 0.25 with 180 deg shutter, chromatic aberration <= 0.1 (see graphics-and-lighting.md). Tone-mapper: UE filmic/AgX-like; do not grade hard to hide smog colour.
- Heat shimmer (summer): refraction post material with noise-driven UV offset 0.5-1.5 px above Tmax 40 C and in view distance > 30 m.

### B2.6 Exposure and light units for the Delhi sun

Use **physical units**: directional light "Intensity" in lux, "Source Angle 0.53", Atmosphere Sun Light on; skylight real-time capture; Exposure Method **Auto Exposure Histogram**, **Min/Max EV100 = [1, 15]** for exteriors with a slow bias to bright (speed up 3, down 1) `[E]`.

Direct-normal sun illuminance (what UE needs) from a Beer-Lambert model: `E_dn = 127,500 lux * exp(-tau_total * m)`, `m = 1/sin(elevation)` (air mass), `tau_total = tau_clean (0.18-0.22: Rayleigh + ozone + water vapour) + AOD(550 nm)`. Delhi AOD is **a typical-literature, not verified here `[E]`**: post-rain 0.3-0.4, normal clear 0.5-0.8, summer dust 0.8-1.4, winter smog 1.5-3+. Resulting values at noon (15th of month):

| Condition | Elev | m | tau_total | Direct sun (lux) | Sky/diffuse horizontal (lux) | Suggested EV100 (scene bright areas) |
|---|---|---|---|---|---|---|
| June clear (AOD 0.7) | 84.6 | 1.00 | 0.9 | about 52,000 | 25,000-35,000 | 14.5-15.0 |
| June dust haze (AOD 1.2) | 84.6 | 1.00 | 1.4 | about 31,000 | 30,000-40,000 (milky) | 14.0 |
| July/Aug monsoon overcast | 83-76 | 1.0-1.03 | cloud | direct about 0-10,000 | 15,000-30,000 | 12.5-13.5 |
| Oct clear (AOD 0.6) | 53 | 1.25 | 0.8 | about 47,000 | 15,000-20,000 | 14.0 |
| Nov typical smog (AOD 2.0) | 43 | 1.47 | 2.2 | about 5,000 | 8,000-14,000 | 12.0-12.5 |
| Nov severe smog (AOD 3.0, AQI 450) | 43 | 1.47 | 3.2 | about 1,100 | 5,000-9,000 | 11.0-11.5 |
| Jan clear cold morning (AOD 0.8) | 40 | 1.56 | 1.0 | about 27,000 | 10,000-15,000 | 13.5 |
| Winter dense fog | any | - | fog | direct 0 | 3,000-12,000 | 10.0-11.5 |

(The older doc suggested 40-60k lux for the smog sun. That fits a "hazy but visible" day; at AQI 400+ the direct beam is far weaker. Use 5-20k for the orange disc and let sky luminance carry the scene.) Typical sun colour: June clear about 5800-6000 K, winter smog sun 3500-4500 K orange, dusk smog 2000-2500 K. Typical ambient interiors and lanes: EV100 6-8; night lanes with LED street light EV100 2-4; shop interiors with 6500 K tube light EV100 6-7; Diwali market night EV100 4-5 `[E]`.

### B2.7 Sky Atmosphere values for haze and smog

Defaults to know `[V]`: Mie anisotropy 0.8, Rayleigh scale height 8 km, ground albedo 0.4 (linear), multi-scattering factor 1 for physically based sky. I did not find Epic's exact default Mie/Rayleigh scale numbers in the docs, so scale relative to default 1.0.

| State | Rayleigh scale | Mie scattering scale | Mie absorption scale | Mie anisotropy | Mie scale height (km) | Height fog (density / falloff) |
|---|---|---|---|---|---|---|
| Clear spring/post-rain | 1.0 | 1.0 | 1.0 | 0.8 | 1.2 | 0.01 / 0.2 |
| Summer dust haze | 0.9 | 4-6 | 2-4 (tan/brown) | 0.7 | 2.0 | 0.02 / 0.1 |
| Aandhi | 0.8 | 12-18 | 8-12 (orange-brown) | 0.6 | 3.5 | 0.06 / 0.05 |
| Winter smog AQI 300 | 0.6 | 5 | 3-4 | 0.7 | 1.5 | 0.06 / 0.08 |
| Winter smog AQI 400+ | 0.5 | 8-12 | 5-8 (brown-grey) | 0.65 | 1.2 | 0.08-0.20 / 0.05-0.10 (matches implementation.md) |
| Dense fog | 1.0 | 6-8 | 0 | 0.4 (white) | 0.4 | 0.2-0.5 / 0.3, inscatter (0.75, 0.78, 0.82) |
| Monsoon overcast | 1.0 | 2 | 0.5 | 0.8 | 1.5 | 0.03 / 0.1, plus Volumetric Clouds thick |

All values `[E]`; calibrate with the A3 visibility chart. Volumetric fog extinction scale 1.5-3x for smog (implementation.md). `r.VolumetricFog.GridPixelSize 12` (8 for quality), `r.VolumetricFog.GridSizeZ 128`, `r.VolumetricFog.HistoryMissSupersampleCount 4`; MegaLights fog volume cost from `r.MegaLights.Volume.*`.

### B2.8 Master material layering

Single `M_Master_Surface` with layered parameter groups, driven by `MPC_Weather` and per-instance data (PCG/ISM custom data):

| Layer | Inputs | Notes |
|---|---|---|
| Base PBR | 2K albedo/normal/ORM (CC0 sets) | albedo 30-240 sRGB |
| Age/wear | curvature, AO, edge mask | lime plaster peeling over lakhori brick (mask threshold 0.45) |
| Rising damp | world Z above plinth 0.5-1.0 m, noise | darkens albedo 25 %, roughness -0.2 |
| Soot/streaks | below chhajja/AC, vertical noise | grey-black, roughness +0.1 |
| **Dust** (`DustAmount`) | upward normal, sheltered AO | summer 0.6-0.9, monsoon washed to 0.1; brown-tan (0.45, 0.38, 0.28) albedo overlay, roughness 0.85 |
| **Wet** (`Wetness`) | porosity | roughness 0.1-0.3 where wet, albedo x0.6-0.7 for porous stone; sandstone darker/redder |
| **Puddles** (`PuddleLevel`) | landscape/mesh height or vertex mask, RVT | normal flat, roughness 0.02-0.05; ripple normal when raining |
| Smog film | `SmogDensity` | adds 0.05-0.1 desaturation to distant faces via fog, not in material |
| Moss/algae | wall base, north-facing, monsoon-triggered | Jul-Sep |
| Festival | `L_Diwali` decals, marigold, colour (Holi) | via decals not materials |

Substrate is optional and heavier; stay on the legacy shading model first.

### B2.9 Decals

DBuffer on; atlas of paan stains, oil, water lines, poster layers, stencilled phone numbers (implementation.md §4.6). Budget: decal draw count under 500 visible, max fade distance 12-20 m, `r.Decal.FadeScreenSizeMult 1`. Holi colour splashes 5-10 % of walls on 4-5 Mar and fade to 50 % in a week by `MPC` decay; Diwali soot and burn marks on roads.

### B2.10 Foliage and vegetation calendar (Delhi)

Old Delhi's core has very few trees (shrine peepals/neem, a few in courtyards, avenue trees on Netaji Subhash Marg and near the Red Fort/Ridge). Bloom/leaf data drive a `SeasonPhase` scalar (DayOfYear) into foliage materials. Amaltas, gulmohar and jacaranda `[V]` from Krishen/Delhi blogs; the rest `[E]`, check against the Delhi Forest Department "Common Trees of Delhi" booklet.

| Species | Look in game | Leaf | Flower/fruit | Notes |
|---|---|---|---|---|
| **Neem** (Azadirachta indica) | fine pinnate, light | new flush Feb-Mar, semi-evergreen | tiny white flowers Mar-Apr; yellow nimboli May-Jul | the commonest street/shrine tree |
| **Peepal** (Ficus religiosa) | heart-shaped, drip-tip, flutters | leaf-fall Feb-Mar, coppery-pink flush Mar-Apr | figs Apr-Jun | shrine trees, the "rustle" audio asset |
| **Banyan** (Ficus benghalensis) | huge, aerial roots | evergreen, red flush Mar-Apr | figs Apr-Jun | few in Old Delhi; temples/ridge |
| **Gulmohar** (Delonix regia) | feathery, umbrella canopy | semi-deciduous | **orange-red late Apr-Aug, peak May-Jun** `[V]`; black pods | flowering irregular |
| **Amaltas** (Cassia fistula) | yellow chains | bare Mar-Apr | **yellow from late Apr to Jun, peak mid-May** `[V]`; long dark pods | also seen with monsoon bloom |
| **Jacaranda** | blue-purple | semi-deciduous | **Apr-May**, first of the three `[V]` | only avenues (not core) |
| **Ashoka** (mostly Polyalthia longifolia, false ashoka; true Saraca asoca is rare) | columnar, wavy leaves | evergreen | small greenish flowers Mar-Apr; black fruit Jun-Jul | temple/school boundary rows |
| Semal, Palash, Kachnar | red/orange/pink bare-tree blooms | bare | Feb-Mar | Ridge, parks |
| Alstonia (saptaparni) | whorled dark leaves | evergreen | strong scent Oct-Nov | autumn colour |
| Kikar (Prosopis juliflora), babool | thorny scrub | evergreen | - | **Ridge biome** |
| Mustard (Khadar) | yellow fields | - | Jan-Feb | floodplain PCG |
| Marigold, rose, bougainvillea | cut flowers/hedges | - | marigold Oct-Nov (Diwali), bougainvillea Feb-Apr and again after rain | garlands, vendors |

Settings: Nanite foliage trees with WPO wind (disable WPO beyond 30 m), billboard impostors for LOD last; leaf transmission/subsurface with 0.1-0.2 opacity; wind speed from `MPC_Weather.WindStrength`; leaf colour curves per species vs `SeasonPhase`; leaf-litter PCG scatter in Nov-Feb under neem and peepal, bright new leaves in Mar-Apr.

### B2.11 Road markings (Indian practice) `[E; verify IRC:35-2015 and local PWD standards]`

- White centre and edge lines, **100-150 mm wide**; broken centre line typically 3 m stripe / 6 m gap on urban roads; yellow for no-overtaking/median kerb; zebra crossings white, **500 mm bars 3-4 m long, 500 mm gaps**; stop lines 200-300 mm; thermoplastic with glass-bead retro-reflectivity, so night headlight response is strong but patchy.
- Wear: break up with alpha noise (40-70 % lost on busy corridors), tyre-track dirt, patched asphalt blocks (different albedo), manhole covers, speed-breakers (humps, painted bands), cat-eye road studs. **Chandni Chowk's pedestrian-priority stretch has no lane markings, just sandstone paving, bollards and tactile strips; Netaji Subhash Marg and Asaf Ali Road carry the full marking set.**
- Kerb painting: alternating black/yellow or black/white at medians; blue/white blockers near the metro.
- Signs: reflective green direction boards (white text, Hindi/English), blue information, police barricades (red/white), DTC bus stops, auto stands.
- Implementation: lines as **decals/spline-mesh with RVT height sampling**, not baked into the landscape; Material: albedo 0.8 white (not pure), roughness 0.45, retro-reflection by adding a view-dependent emissive term at night when a vehicle spotlight angle < 10 deg.

### B2.12 Street lights in Delhi, 2026 `[V where noted]`

- **PWD 96,000-light smart-LED programme**, Rs 473 crore (some outlets 93,000), covering PWD major roads; existing stock about **45,000 HPSV (sodium) + 51,000 older LEDs** on 51,000+ poles; CM target completion by **Diwali 2026**; tender floated 26 May 2026; includes monitoring and automatic fault detection `[V]`. **MCD** plans over **4 lakh smart streetlights** with LED fittings and centralised remote monitoring `[V]`. I found no Kelvin spec.
- So in Oct-Nov 2026 the city is a **visible mix**: amber HPSV (about 2000-2100 K, CRI 20-25) on un-replaced PWD roads, cool white LED (typically 4000-6500 K `[E]`) on new ones, a 9-12 m pole at 30-35 m spacing `[E]`, 20-30 m high-mast at junctions. This colour mix is a strong realism cue: **side streets amber, main roads white, flats alternating**.

| Fixture | Lumens | Colour | UE setup | Where |
|---|---|---|---|---|
| HPSV 150 W / 250 W | 14,000-28,000 lm | 2000-2100 K, orange | Spot 120 deg, IES Type II/III, intensity in lumens, source radius 5 cm; **MegaLights** | older PWD roads, colonies |
| LED cobra 60-120 W | 8,000-15,000 lm | 4000-5700 K, cool white | same, 100-130 lm/W; asymmetric IES; MegaLights | new PWD and MCD |
| High-mast LED 300-400 W | 40,000-60,000 lm | 5700 K | Spot, 2 or 4 heads | junctions, ISBT, Red Fort perimeter |
| Heritage lamp post (Chandni Chowk redevelopment) | 3,000-6,000 lm | 3000 K | Point + shade mesh | main road |
| Shop tube lights / LED bulbs | 1,000-3,000 lm each | 6500 K cool white | emissive + 1 small light per 3-4 shops | galis |
| Fairy-string/diya strips | emissive only | 2200-3000 K | emissive instances, no lights | Diwali period |
| Vehicle headlights | 1,000-1,500 lm | 3500-6000 K | spot with MegaLights | traffic |

Add sodium-vapour scattering in fog (volumetric scattering 2-4 per implementation.md) and **flicker/outage** (5 % of fixtures dead or buzzing at night; flicker rate 100 Hz sodium start-up fade over 5 min).

## B3. First playable base: editor setup sequence

1. **Install and project:** UE 5.8.4, C++ Third Person template, name `OldDelhi`. Plugins on: GeoReferencing, PCG, Water, ChaosVehicles, Niagara, MassAI/MassCrowd/ZoneGraph, StateTree, Soundscape, Audio Gameplay Volumes, Interchange, **MeshTerrain/MeshPartition left OFF** (Experimental). Apply the `DefaultEngine.ini` block (B2.1), enable Nanite/Lumen/VSM in Project Settings.
2. **Create `L_OldDelhi_Master`** as World Partition level, cell size 6400 cm, loading range 19200-25600 cm, and a second runtime grid for small props at 6000 cm (implementation.md §3.4). Add **GeoReferencing** system: Flat Planet, EPSG:32643, origin at 28.655 N, 77.230 E; note this lat/lon in the manifest.
3. **Terrain:** produce the 4033 x 4033 R16 heightmap (B1.2/B1.4) and the 2017 x 2017 context heightmap. Landscape Mode > Import from File; Section 63 quads, 2x2, **Scale (100, 100, 12.5)**, Location Z = 21,000 cm, enable **World Partition (streaming proxies 4x4)**; second Landscape for context (Scale 3000, 3000, 100). Paint the base layers; add Yamuna as a Water body.
4. **Sky and light rig `BP_SkyRig`:** Directional Light (Atmosphere Sun, Intensity from the B2.6 table, Source Angle 0.53, Cast VSM), Sky Atmosphere (defaults, then the B2.7 smog preset), Sky Light (real-time capture), Exponential Height Fog (volumetric fog on), Volumetric Clouds (on), PostProcessVolume Unbound (manual EV100 bracket 8-15 initially). Hook time and weather to `UWeatherSubsystem` (A4). Test with the four reference days.
5. **Import kit buildings** with the Interchange preset from implementation.md §3.2 (one FBX per building, Nanite on, one convex hull per UCX). Assign `M_Master_Surface` instances.
6. **Place a 500 x 500 m vertical slice** around Chandni Chowk-Dariba-Kinari with sandstone road, bollards, shutters, overhead wires.
7. **Lighting pass:** street lights per B2.12 with MegaLights; shop interiors; Diwali emissive strips; confirm night EV100 4-5. Switch Lumen between High and Medium (B2.2) and A/B screenshots.
8. **Foliage and ground:** peepal/neem shrine trees, ridge scrub off-slice; PCG graph for litter and marigold petals; decal atlas.
9. **Weather presets:** smog, fog, clear summer, monsoon, aandhi through the MPC (A4); verify visibility chart (A3) and `stat gpu` per preset.
10. **Profiling gates:** at each preset hold 16.6 ms at 1440p/TSR 75 %: Nanite+VSM 4.5, Lumen 3.5, sky/fog 1.5, translucency+Niagara 1.5, TSR+post 2.0, headroom 3.0; log VRAM (target < 7 GB), `stat unit`, `stat gpu`, Unreal Insights. If MegaLights plus Lumen go over, drop to Lumen Medium first, then MegaLights samples to 2, then VSM SMRT rays.

---

## Open items and flags

- The `statistics 2026` monthly rows for **Mar, Jun, Aug** are partial (no full IMD mean maximum for March; June/July AQI means not found). IMD's `Observational_table.pdf` and `safdarjung2.pdf` (mausam.imd.gov.in/newdelhi/mcdata/) hold the daily source; my fetcher could not decompress them. A local script with `pdftotext` or Python is the way to get exact figures.
- Conflicting numbers kept as ranges: May 2025 rain 185.9-188.9 mm; Aug 2026 354.1-381.7 mm; Oct 2026 AQI 166 vs 191.5; Dec 2025 AQI 349 vs 351; Safdarjung 19 May 2026 max 43.4-45.1 C; Janmashtami 4 Sep vs mid-Aug; Chhath 15 vs other dates; Bhai Dooj 9-11 Nov.
- Date unverifiable for: Republic Day chief guests, Basant Panchami, Ram Navami, Muharram, Ganesh Chaturthi, Guru Nanak Jayanti. Delhi government publishes the official holiday list; check it.
- Mesh Terrain and Lumen Lite are Experimental/Beta; both can change before UE6.
- FABDEM is non-commercial (CC BY-NC-SA 4.0 in most listings); do not ship it without Fathom's agreement. Copernicus DEM itself requires the standard "Contains modified Copernicus data" attribution.
- AOD, EV100 and Mie scale values are first-principles estimates to be tuned against reference photos in-engine.
- No 2026 Yamuna ORB readings found.

## Sources

Weather and climate
- IMD Delhi monsoon withdrawal release, 23 Sep 2026: https://mausam.imd.gov.in/newdelhi/archive/2026-09-23/press_release_en.pdf
- IMD monthly releases (Delhi): https://mausam.imd.gov.in/newdelhi/archive/2026-02-03/press_release_en.pdf , .../2026-03-03/... , .../2026-05-06/... , .../2026-06-03/... , .../2026-07-08/... , .../2026-08-18/... , .../2025-11-03/...
- Down To Earth, Delhi welcomes monsoon 2026 (onset 2 Jul): https://www.downtoearth.org.in/climate-change/delhi-welcomes-monsoon-2026
- Skymet, 2025 withdrawal (24 Sep, 902.6 mm): https://www.skymetweather.com/content/monsoon-update/delhi-monsoon-withdrawal-2025-arrives-close-to-normal-date-with-excess-rainfall-and-bay-of-bengal-depression-likely-to-bring-light-sprinkles
- Wikipedia Climate of Delhi (normals): https://en.wikipedia.org/wiki/Climate_of_Delhi
- Business Today, 2026 heat and power demand: https://www.businesstoday.in/india/story/delhis-power-demand-crosses-7700-mw-as-temperatures-race-past-45degc-highest-peak-in-2026-so-far-532393-2026-05-20
- Dust storms 2026: https://www.businesstoday.in/india/story/monsoon-2026-dust-storms-strong-winds-sweep-delhi-imd-issues-red-alert-538722-2026-06-23 ; https://www.uniindia.com/dust-storms-blanket-delhi-palam-records-wind-speeds-of-93-kmph-causing-flight-disruptions/india/news/3877560.html ; https://www.ptcnews.tv/delhi-nation/massive-dust-storm-sweeps-parts-of-delhi-ncr-heavy-downpour-affects-flight-operations-4410081
- Business Today, April 2026: https://www.businesstoday.in/india/story/heatwave-tightens-grip-delhi-sizzles-as-temperatures-hit-45deg-when-is-relief-expected-527502-2026-04-26
- Jan 2026 cold: https://www.businesstoday.in/india/story/cold-wave-in-north-india-dense-fog-impacts-delhi-flights-mercury-hits-43degc-in-safdarjung-511282-2026-01-17 ; Dec 2025 coldest day: https://www.businesstoday.in/india/story/delhi-freezes-into-2026-year-end-chill-tightens-grip-on-national-capital-coldest-december-day-in-6-years-508990-2026-01-01
- Fog and flights: https://www.businesstoday.in/amp/india/story/delhi-weather-update-fog-blankets-capital-airlines-issue-advisories-aqi-slips-to-386-509048-2026-01-02 ; https://www.businesstoday.in/india/story/winter-fog-disrupts-delhi-airport-over-60-flights-cancelled-and-hundreds-delayed-506667-2025-12-15
- Aug 2026 rain: https://www.business-standard.com/india-news/delhi-crosses-august-rainfall-average-more-rain-likely-126080900050_1.html ; https://www.nationalheraldindia.com/national/delhi-records-wettest-day-in-over-2-yrs-as-heavy-rain-triggers-waterlogging
- 2025 rain: https://www.theweek.in/wire-updates/national/2025/08/29/des54-dl-rain-data.html ; https://www.theweek.in/wire-updates/national/2025/05/30/nrg9-dl-ld-weather.html ; https://www.newsbytesapp.com/news/delhi/delhi-records-coolest-and-wettest-oct-in-3-years-imd/story ; https://www.wionews.com/india-news/delhi-finally-welcomes-monsoon-showers-rainy-season-covers-entire-india-9-days-early-1751274044668/amp
- April 2025: https://www.theweek.in/wire-updates/national/2025/04/09/des58-dl-2ndld-weather.html
- Yamuna: https://theprint.in/india/yamuna-swells-past-207-metres-in-delhi-floodwater-inundates-nearby-areas/2735642 ; https://www.editorji.com/india-news/delhi/yamuna-river-water-levels-drop-after-peak-1757043907656 ; https://www.siasat.com/yamuna-again-breaches-danger-mark-in-delhi-2668348/ ; https://thepatriot.in/delhi-ncr/yamuna-nears-warning-mark-in-delhi-74251/amp
- AQI: https://www.dailypioneer.com/news/delhi-records-cleanest-january-may-air-quality-in-8-years ; https://www.theweek.in/wire-updates/national/2026/01/31/delhi-with-average-aqi-of-307-2026-sees-second-best-january-in-5-years.html ; https://organiser.org/2026/10/01/383796/bharat/delhis-september-air-turns-cleaner-aqi-hits-five-year-low-as-poor-air-days-fall/ ; https://thecsrjournal.in/delhi-records-no-good-air-days-in-october-2026-despite-improvement-in-air-quality-index-compared-to-previous-years ; https://www.thestar.com.my/news/world/2026/11/07/delhi-world039s-most-polluted-major-city-on-eve-of-diwali-swiss-group-says ; https://www.tribuneindia.com/news/delhi/city-witnesses-best-jan-nov-average-aqi-in-8-yrs-panel ; https://www.theweek.in/news/india/2025/12/06/stubble-burning-down-but-pollution-worse-in-delhi-ncr-ghaziabad-most-polluted-in-november.html ; https://energyandcleanair.org/wp/wp-content/uploads/2025/12/Monthly-ambient-air-quality-snapshot-for-India-November-2025.pdf ; https://www.freepressjournal.in/india/delhi-wakes-up-to-choking-air-on-diwali-morning-as-aqi-hits-417-grap-ii-curbs-in-place ; https://www.businesstoday.in/india/story/delhi-ncr-air-pollution-december-on-track-to-be-dirtiest-in-8-yrs-amid-severe-fog-aqi-to-top-400-today-507521-2025-12-20 ; https://www.devdiscourse.com/article/science-environment/3744425-delhis-air-quality-crisis-a-deepening-dilemma

Festivals and events
- Holiday lists / dates: https://www.csb.bank.in/pdf/Holiday-List-2026-India-03022026.pdf ; https://hindutone.com/festivals/hindu-festival-calendar-2026-complete-guide-nri/ ; https://hindutone.com/festivals/chhath-puja-2026/ ; https://happyfares.in/blog/dussehra-diwali-2026-dates-festive-calendar/ ; https://dekhopanchang.com/en/festivals/chhath-puja
- Bakrid 28 May: https://www.indiatvnews.com/news/india/bakrid-2026-eid-al-adha-to-be-celebrated-in-india-on-may-28-announces-jama-masjid-shahi-imam-2026-05-17-1041496
- Kanwar Yatra: https://businesstoday.in/india/story/kanwar-yatra-2026-kicks-off-key-routes-major-highway-closures-diversions-to-avoid-in-delhi-ncr-546398-2026-07-31
- Wedding muhurats: https://hindutone.com/astrology/vivah-muhurat-2026-27-shubh-marriage-dates/ ; https://theweddingfocus.com/blog/wedding-dates-2026/
- Cricket: https://www.wisden.com/series/icc-mens-t20-world-cup-2026/cricket-news/t20-world-cup-2026-venue-wise-schedule-full-fixtures-list-and-match-timings-for-all-eight-grounds ; https://www.outlookindia.com/sports/cricket/india-vs-afghanistan-t20i-series-2026-schedule-arun-jaitley-stadium-check-dates-here ; https://www.mykhel.com/cricket/delhi-premier-league-2026-kicks-off-arun-jaitley-stadium-014-448825.html
- MCD polls: https://www.indiatvnews.com/delhi/mcd-announces-election-schedule-for-posts-of-mayor-deputy-mayor-and-standing-committee-members-check-dates-and-other-details-2026-04-16-1037787

UE 5.8, terrain, DEM
- UE 5.8 release notes: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes
- UE 5.8 announcement: https://www.unrealengine.com/news/unreal-engine-5-8-is-now-available ; 80.lv: https://80.lv/articles/unreal-engine-5-8-is-out-today-with-big-optimization-improvements-and-mesh-terrain
- Mesh Terrain knowledge base thread: https://forums.unrealengine.com/t/knowledge-base-introduction-to-mesh-terrain/2707493
- Landscape technical guide: https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-technical-guide-in-unreal-engine
- MegaLights: https://dev.epicgames.com/documentation/en-us/unreal-engine/megalights-in-unreal-engine
- Sky Atmosphere: https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-atmosphere-component-in-unreal-engine
- FABDEM paper: https://iopscience.iop.org/article/10.1088/1748-9326/ac4d4f ; Fathom: https://www.fathom.global/academic-papers/a-30-m-global-map-of-elevation-with-forests-and-buildings-removed/ ; GEE catalog: https://gee-community-catalog.org/projects/fabdem/
- Street lights: https://www.dailypioneer.com/news/government-to-replace-96000-street-lights-with-smart-leds ; https://thepatriot.in/?p=83703 ; https://thepatriot.in/?p=85699
- Trees: https://thedelhiwalla.blog/2026/04/28/city-season-first-sight-summertime-amaltas-flowers/ ; https://www.tribuneindia.com/news/features/the-many-colours-of-summer-blooms-243919 ; https://forest.delhi.gov.in/sites/default/files/generic_multiple_files/delhi_treesss.pdf
