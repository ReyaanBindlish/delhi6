"""Turn the research reports into Unreal DataTable CSVs (OldDelhi/Data/*.csv).

Sources:
  research/05-npc-population-and-characters.md  sections 1.3, 2.1, 2.2, 5.1, 5.2
  research/02-weather-timeline-and-terrain.md   section A4.3 (monthly weather Markov table)

Outputs (first column = RowName, as UE DataTable CSV import expects):
  DT_Archetypes.csv        52 NPC archetypes (look by season, schedule, places, languages, spawn group)
  DT_StreetDensity.csv     visible agents per 100 m by street type x time block
  DT_PopulationSchedule.csv  archetype share per (time block, street type)
  DT_PopulationEvents.csv  event / condition multipliers
  DT_ClimateMonth.csv      per-month weather-state probabilities + persistence
"""
import csv
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
R05 = (REPO / "game-design-docs" / "research" / "05-npc-population-and-characters.md").read_text(encoding="utf-8")
R02 = (REPO / "game-design-docs" / "research" / "02-weather-timeline-and-terrain.md").read_text(encoding="utf-8")
OUT = REPO / "OldDelhi" / "Data"
OUT.mkdir(parents=True, exist_ok=True)


def section(text, start, end):
    i = text.index(start)
    j = text.index(end, i + len(start))
    return text[i:j]


def table_rows(block):
    rows = []
    for line in block.splitlines():
        if line.startswith("|") and not re.match(r"^\|\s*-", line):
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            rows.append(cells)
    return rows[1:]  # drop header


def clean(s):
    return re.sub(r"\s+", " ", s.replace("**", "").replace("`", "")).strip()


SPAWN_GROUP = {}
for ids, grp in (
    (range(1, 12), "Vendor"), (range(12, 17), "Vendor"), ([17, 18, 23, 28, 29, 30], "Worker"),
    ([19, 20, 21, 22], "Driver"), (range(24, 28), "Police"), ([31, 32], "Student"), ([33, 51], "Commuter"),
    ([34, 35, 48, 49], "Tourist"), (range(36, 41), "Worshipper"), ([41], "Static"),
    ([42, 43, 50], "Special"), (range(44, 48), "Resident"), ([52], "Vendor")):
    for i in ids:
        SPAWN_GROUP[f"A{i:02d}"] = grp

# ---------------------------------------------------------------- archetypes (2.1 + 2.2)
look = {r[0]: r for r in table_rows(section(R05, "### 2.1 Identity and look", "### 2.2")) if re.match(r"A\d\d", r[0])}
beh = {r[0]: r for r in table_rows(section(R05, "### 2.2 Behaviour", "### 2.3")) if re.match(r"A\d\d", r[0])}
with open(OUT / "DT_Archetypes.csv", "w", newline="", encoding="utf-8") as f:
    w = csv.writer(f)
    w.writerow(["Name", "Label", "SpawnGroup", "AgeMin", "AgeMax", "MaleShare", "FemaleShare", "OtherShare",
                "OutfitSummer", "OutfitMonsoon", "OutfitWinter", "Accessories", "Schedule", "BodyLanguage",
                "Places", "Languages"])
    for aid in sorted(look):
        L = look[aid]; B = beh.get(aid, [aid, "", "", "", ""])
        ages = re.findall(r"\d+", L[2]); g = re.findall(r"\d+", L[3])
        langs = ";".join(f"{k}={v}" for k, v in re.findall(r"\b([A-Z][a-z])\s+(\d+)", B[4]))
        w.writerow([aid, clean(L[1]), SPAWN_GROUP.get(aid, "Special"),
                    ages[0] if ages else 18, ages[1] if len(ages) > 1 else 60,
                    g[0] if g else 50, g[1] if len(g) > 1 else 50, g[2] if len(g) > 2 else 0,
                    clean(L[4]), clean(L[5]), clean(L[6]), clean(L[7]),
                    clean(B[1]), clean(B[2]), clean(B[3]), langs])
print("archetypes:", len(look))

# ---------------------------------------------------------------- street density (1.3 game-visible table)
STREETS = {"Main road": "MainRoad", "Bazaar galli": "BazaarGalli", "Residential galli": "ResidentialGalli",
           "Mosque steps and gate zone": "MosqueGate", "Station forecourt": "StationForecourt"}
vis = section(R05, "**Game-visible agents per 100 m", "Add on top")
with open(OUT / "DT_StreetDensity.csv", "w", newline="", encoding="utf-8") as f:
    w = csv.writer(f)
    w.writerow(["Name", "H05", "H09", "H13", "H18", "H22", "H02", "StaticDressingFactor"])
    for r in table_rows(vis):
        key = next((v for k, v in STREETS.items() if r[0].startswith(k)), None)
        if key:
            nums = [int(re.sub(r"[^\d]", "", c)) for c in r[1:7]]
            w.writerow([key, *nums, 0.4])

# ---------------------------------------------------------------- schedule (5.1)
NAMED = {"shoppers": ["A46", "A47", "A34"], "travellers": ["A34", "A33", "A36"], "vendors": ["A14", "A15"],
         "shopkeepers": ["A01", "A03", "A05", "A06", "A09"], "sleepers": ["STATIC_SLEEPER"],
         "staff": ["A02", "A04", "A09"], "others": ["MIXED"], "dogs": [], "pigeon": []}
TIMES = {"05:00": "H05", "09:00": "H09", "13:00": "H13", "18:00": "H18", "22:00": "H22", "02:00": "H02"}
COLS = ["MainRoad", "BazaarGalli", "ResidentialGalli", "MosqueGate", "StationForecourt"]
sched = section(R05, "### 5.1 Normal weekday", "Simplify in the implementation")
rows_out = []
for r in table_rows(sched):
    t = clean(r[0])
    if t not in TIMES:
        continue
    for col, cell in zip(COLS, r[1:6]):
        for part in re.split(r",\s*(?![^()]*\))", cell):
            part = part.strip()
            m = re.search(r"(\d+)\s*(?:\(.*\))?\s*$", re.sub(r"\(\*\*.*?\*\*\)", "", part))
            nums = re.findall(r"\b(\d+)\b", re.sub(r"\(.*?\)", "", part))
            if not nums:
                continue
            weight = float(nums[-1])
            ids = re.findall(r"A\d\d", part)
            if not ids:
                low = part.lower()
                ids = next((v for k, v in NAMED.items() if k in low), ["MIXED"])
            for aid in ids:
                rows_out.append([f"{TIMES[t]}_{col}_{aid}", TIMES[t], col, aid, round(weight / len(ids), 2)])
with open(OUT / "DT_PopulationSchedule.csv", "w", newline="", encoding="utf-8") as f:
    w = csv.writer(f)
    w.writerow(["Name", "TimeBlock", "Street", "Archetype", "Weight"])
    seen = set()
    for row in rows_out:
        if row[0] in seen:
            row[0] += "_b"
        seen.add(row[0]); w.writerow(row)
print("schedule rows:", len(rows_out))

# ---------------------------------------------------------------- events (5.2)
ev = section(R05, "### 5.2 Event layer", "### 5.3")
with open(OUT / "DT_PopulationEvents.csv", "w", newline="", encoding="utf-8") as f:
    w = csv.writer(f)
    w.writerow(["Name", "Label", "Window", "Zones", "Multiplier", "Effect", "AddedArchetypes"])
    for r in table_rows(ev):
        label = clean(r[0])
        mult = re.findall(r"[×x]\s*(\d+(?:\.\d+)?)", r[3])
        name = re.sub(r"[^A-Za-z0-9]+", "_", label).strip("_")[:40]
        w.writerow([name, label, clean(r[1]), clean(r[2]), mult[0] if mult else 1.0, clean(r[3]),
                    ";".join(re.findall(r"A\d\d", r[4]))])

# ---------------------------------------------------------------- climate (02 A4.3)
cl = section(R02, "| Mon | CLR", "**Transition rule")
MONTHS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
with open(OUT / "DT_ClimateMonth.csv", "w", newline="", encoding="utf-8") as f:
    w = csv.writer(f)
    w.writerow(["Name", "Clear", "Haze", "Overcast", "Rain", "Storm", "Fog", "Smog", "Persistence"])
    for line in cl.splitlines():
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if cells and cells[0] in MONTHS:
            w.writerow([cells[0]] + [float(c) for c in cells[1:9]])
print("wrote tables to", OUT)
