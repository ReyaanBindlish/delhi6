"""delhi6 two-month build plan (10 Nov 2026 -> 10 Jan 2027). Minimal layout, hand-made vector illustrations.

python make_timeline.py  -> delhi6_build_timeline.html  (print to PDF with Edge headless)
"""
import datetime as dt
import math
import random
from pathlib import Path

HERE = Path(__file__).resolve().parent
D = dt.date
INK, SAND, CLAY, SAFFRON, TEAL, HAZE, MUTE = "#1f1b16", "#e9dfcf", "#c2603a", "#e0a23a", "#2f6b66", "#f7f2ea", "#8b8177"

START, END = D(2026, 11, 10), D(2027, 1, 10)

PHASES = [
    ("01", "Foundation", D(2026, 11, 10), D(2026, 11, 15), TEAL, "gear",
     "Engine running, slice in Unreal",
     ["Unreal Engine 5.8.4 installed, OldDelhi compiled", "Terrain + 1,329 buildings imported",
      "Unreal MCP connected for live editing", "Daily builds and screenshots"],
     "Chandni Chowk renders in Unreal"),
    ("02", "World", D(2026, 11, 16), D(2026, 11, 29), CLAY, "city",
     "Make the city look real",
     ["Landscape, materials, Lumen lighting", "Weather: haze, smog, rain, dust storm",
      "Boundary wall at the city edge", "Ground detail, wires, fictional signage"],
     "Walkable slice at final look"),
    ("03", "Life", D(2026, 11, 30), D(2026, 12, 13), SAFFRON, "people",
     "Fill the streets",
     ["Physics core: materials, objects, wind", "Crowds from 12 character types",
      "Rickshaws, e-rickshaws, scooters", "Pigeons, kites, azaan and bazaar sound"],
     "Streets alive day and night"),
    ("04", "Gameplay", D(2026, 12, 14), D(2026, 12, 27), INK, "path",
     "Turn the slice into a game",
     ["Player movement and camera", "Short story set in Diwali week",
      "3 to 5 missions through real landmarks", "Save, menus, settings"],
     "Story playable start to finish"),
    ("05", "Polish & ship", D(2026, 12, 28), D(2027, 1, 10), TEAL, "flag",
     "Ship version 1.0",
     ["60 fps on an RTX 3070", "Playtests and bug fixing", "Trailer, store page, screenshots",
      "Release build"],
     "Launch on 10 January 2027"),
]


# ------------------------------------------------------------------ illustrations (hand-built SVG)
def skyline(w=1200, h=520, seed=6):
    rnd = random.Random(seed)
    s = [f'<svg viewBox="0 0 {w} {h}" xmlns="http://www.w3.org/2000/svg" class="art">',
         '<defs><linearGradient id="sky" x1="0" y1="0" x2="0" y2="1">'
         f'<stop offset="0" stop-color="#f3e3c8"/><stop offset="1" stop-color="{HAZE}"/></linearGradient></defs>',
         f'<rect width="{w}" height="{h}" fill="url(#sky)"/>',
         f'<circle cx="{w * 0.78:.0f}" cy="150" r="62" fill="{SAFFRON}" opacity="0.85"/>']
    gy = h - 70
    # far layer: Jama Masjid silhouette, centred
    mx = w * 0.42
    far = "#d9c3a3"
    s.append(f'<rect x="{mx - 170}" y="{gy - 120}" width="340" height="120" fill="{far}"/>')
    for k in range(7):  # arches
        ax = mx - 150 + k * 45
        s.append(f'<path d="M{ax},{gy} v-55 a18,18 0 0 1 36,0 v55 z" fill="#cdb593"/>')
    for dx, r in ((-95, 38), (0, 56), (95, 38)):  # onion domes
        cx, base = mx + dx, gy - 120
        s.append(f'<path d="M{cx - r},{base} C{cx - r},{base - r * 1.25} {cx - r * 0.25},{base - r * 1.5} {cx},{base - r * 2.05} '
                 f'C{cx + r * 0.25},{base - r * 1.5} {cx + r},{base - r * 1.25} {cx + r},{base} z" fill="#eadcc6"/>')
        s.append(f'<line x1="{cx}" y1="{base - r * 2.05}" x2="{cx}" y2="{base - r * 2.05 - 16}" stroke="{far}" stroke-width="3"/>')
    for dx in (-215, 215):  # minarets with bands
        x0 = mx + dx
        s.append(f'<rect x="{x0 - 11}" y="{gy - 300}" width="22" height="300" fill="{far}"/>')
        for b in range(4):
            s.append(f'<rect x="{x0 - 15}" y="{gy - 290 + b * 70}" width="30" height="7" fill="#c9ad86"/>')
        s.append(f'<path d="M{x0 - 16},{gy - 300} q16,-34 32,0 z" fill="#eadcc6"/>')
    # mid layer: buildings with windows and rooftop tanks
    x = -10
    while x < w:
        bw = rnd.randint(55, 120); bh = rnd.randint(110, 230)
        if abs(x + bw / 2 - mx) < 260:          # keep the mosque's domes visible
            bh = rnd.randint(55, 95)
        col = rnd.choice(["#e2c9a6", "#d6b08a", "#e8d6bd", "#cfa27f", "#dcc2a0", "#b9c4bb"])
        s.append(f'<rect x="{x}" y="{gy - bh}" width="{bw}" height="{bh}" fill="{col}"/>')
        s.append(f'<rect x="{x}" y="{gy - bh}" width="{bw}" height="6" fill="#00000014"/>')
        for wy in range(gy - bh + 22, gy - 30, 34):
            for wx in range(x + 10, x + bw - 16, 22):
                if rnd.random() < 0.8:
                    s.append(f'<rect x="{wx}" y="{wy}" width="10" height="15" rx="1.5" fill="#5b4a3c" opacity="0.55"/>')
        if rnd.random() < 0.6:
            tx = x + rnd.randint(8, max(9, bw - 30))
            s.append(f'<rect x="{tx}" y="{gy - bh - 18}" width="16" height="18" rx="4" fill="{INK}" opacity="0.85"/>')
        x += bw + rnd.randint(-6, 4)
    # ground
    s.append(f'<rect x="0" y="{gy}" width="{w}" height="{h - gy}" fill="#d7c4a8"/>')
    s.append(f'<rect x="0" y="{gy}" width="{w}" height="4" fill="#00000018"/>')
    # overhead wires
    for k in range(6):
        y0 = 250 + k * 18; sag = 40 + k * 6
        s.append(f'<path d="M-10,{y0} Q{w * 0.3},{y0 + sag} {w * 0.62},{y0 - 6} T{w + 10},{y0 + 10}" '
                 f'fill="none" stroke="{INK}" stroke-width="1.6" opacity="0.7"/>')
    # kites and birds
    for kx, ky, c in ((180, 120, CLAY), (300, 70, TEAL), (980, 260, CLAY), (1080, 90, SAFFRON)):
        s.append(f'<path d="M{kx},{ky - 16} l12,16 l-12,16 l-12,-16 z" fill="{c}"/>')
        s.append(f'<path d="M{kx},{ky + 16} q-8,20 4,36 q10,14 0,28" fill="none" stroke="{INK}" stroke-width="1" opacity="0.6"/>')
    for bx, by in ((620, 110), (650, 95), (690, 120), (560, 140), (760, 80)):
        s.append(f'<path d="M{bx - 9},{by} q9,-7 9,0 q0,-7 9,0" fill="none" stroke="{INK}" stroke-width="2" stroke-linecap="round"/>')
    s.append("</svg>")
    return "\n".join(s)


def icon(kind, col):
    base = f'<svg viewBox="0 0 64 64" class="icon"><circle cx="32" cy="32" r="30" fill="{col}" opacity="0.12"/>'
    st = f'fill="none" stroke="{col}" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"'
    body = {
        "gear": f'<circle cx="32" cy="32" r="8" {st}/>' + "".join(
            f'<line x1="{32 + 13 * math.cos(a):.1f}" y1="{32 + 13 * math.sin(a):.1f}" x2="{32 + 19 * math.cos(a):.1f}" '
            f'y2="{32 + 19 * math.sin(a):.1f}" {st}/>' for a in [i * math.pi / 4 for i in range(8)]),
        "city": f'<path d="M14 46 V30 h8 V22 h10 V34 h6 V26 h12 V46 z" {st}/><path d="M27 22 q5 -10 10 0" {st}/>',
        "people": f'<circle cx="24" cy="24" r="5" {st}/><circle cx="40" cy="24" r="5" {st}/>'
                  f'<path d="M16 46 q8 -16 16 0 M32 46 q8 -16 16 0" {st}/>',
        "path": f'<path d="M16 48 C24 36 40 44 32 30 C26 20 40 16 48 16" {st}/><circle cx="16" cy="48" r="3" fill="{col}"/>'
                f'<circle cx="48" cy="16" r="3" fill="{col}"/>',
        "flag": f'<path d="M22 50 V14" {st}/><path d="M22 16 h22 l-6 7 l6 7 h-22" {st}/>',
    }[kind]
    return base + body + "</svg>"


# ------------------------------------------------------------------ timeline
def timeline():
    W, L, R, top = 1000, 150, 20, 46
    days = (END - START).days
    X = lambda d: L + (d - START).days / days * (W - L - R)
    rowh = 54
    h = top + rowh * len(PHASES) + 70
    s = [f'<svg viewBox="0 0 {W} {h}" class="tl">']
    mondays = [START]
    d = START + dt.timedelta(days=(7 - START.weekday()) % 7 or 7)
    while d <= END:
        mondays.append(d); d += dt.timedelta(days=7)
    for wk, d in enumerate(mondays, 1):  # week grid: W1 starts today, then every Monday
        s.append(f'<line x1="{X(d):.1f}" y1="{top - 10}" x2="{X(d):.1f}" y2="{h - 50}" class="grid"/>')
        s.append(f'<text x="{X(d) + 4:.1f}" y="{top - 18}" class="wk">W{wk} · {d.day} {d.strftime("%b")}</text>')
    for i, (code, name, a, b, col, *_rest) in enumerate(PHASES):
        y = top + i * rowh
        s.append(f'<text x="0" y="{y + 25}" class="pname"><tspan class="pcode" fill="{col}">{code}</tspan>  {name}</text>')
        s.append(f'<rect x="{X(a):.1f}" y="{y + 8}" width="{X(b + dt.timedelta(days=1)) - X(a) - 3:.1f}" height="26" rx="13" fill="{col}"/>')
        lab = f"{a.day}–{b.day} {b.strftime('%b')}" if a.month == b.month else f"{a.day} {a.strftime('%b')} – {b.day} {b.strftime('%b')}"
        s.append(f'<text x="{X(a) + 10:.1f}" y="{y + 25}" class="bar">{lab}</text>')
    y = top + rowh * len(PHASES) + 8
    s.append(f'<line x1="{L}" y1="{y}" x2="{W - R}" y2="{y}" class="axis"/>')
    for code, name, a, b, col, *_rest in PHASES:
        s.append(f'<circle cx="{X(b + dt.timedelta(days=1)) - 2:.1f}" cy="{y}" r="5" fill="{col}"/>')
    s.append(f'<text x="{X(END) - 2:.1f}" y="{y + 26}" class="deadline">Deadline · 10 Jan 2027</text>')
    s.append(f'<text x="{L}" y="{y + 26}" class="today">Start · 10 Nov 2026</text>')
    s.append("</svg>")
    return "\n".join(s)


def cards(phases):
    out = []
    for code, name, a, b, col, ic, goal, items, exit_ in phases:
        li = "".join(f"<li>{t}</li>" for t in items)
        out.append(f"""
<div class="card">
  <div class="cardtop">{icon(ic, col)}
    <div><div class="code" style="color:{col}">Phase {code}</div><h3>{name}</h3>
    <div class="dates">{a.strftime("%d %b")} – {b.strftime("%d %b %Y")} · {(b - a).days + 1} days</div></div></div>
  <p class="goal">{goal}</p>
  <ul>{li}</ul>
  <p class="exit"><span style="background:{col}"></span>{exit_}</p>
</div>""")
    return "\n".join(out)


import json as _json

MB = _json.load(open(HERE / "moodboard" / "moodboard.json", encoding="utf-8"))
COVER = MB[0]


def mood_page(items, title, eyebrow, num):
    figs = []
    for m in items:
        sw = "".join(f'<i style="background:{c}"></i>' for c in m["palette"])
        figs.append(f'<figure><div class="ph" style="background-image:url({m["file"]})"></div>'
                    f'<figcaption><span>{m["caption"]}</span><span class="sw">{sw}</span></figcaption></figure>')
    return f"""<section class="page">
  <div class="eyebrow">{eyebrow}</div><h2>{title}</h2>
  <div class="mood">{''.join(figs)}</div>
  <div class="foot"><span>delhi6 · photo references, credits on the last page</span><span>{num}</span></div>
</section>"""


def credits_page(num):
    rows = "".join(f'<tr><td>{m["caption"]}</td><td>{m["artist"]}</td><td>{m["license"]}</td>'
                   f'<td class="url">{m["page"].replace("https://", "")}</td></tr>' for m in MB)
    pins = ["Old Delhi cinematic", "Chandni Chowk night", "Jama Masjid fog", "Delhi winter smog photography",
            "Indian street matte painting", "Old Delhi monsoon"]
    plinks = "".join(f'<li>pinterest.com/search/pins/?q={p.replace(" ", "%20")}</li>' for p in pins)
    return f"""<section class="page">
  <div class="eyebrow">Credits and further inspiration</div><h2>Photo credits</h2>
  <p class="lede">Reference photos from Wikimedia Commons, used under their Creative Commons licences.
    They show the target look; the game's own assets are built separately.</p>
  <table class="credits"><tr><th>Image</th><th>Photographer</th><th>Licence</th><th>Source</th></tr>{rows}</table>
  <h3 style="margin-top:9mm">Pinterest boards to browse</h3>
  <p class="lede">For extra mood, browse these searches. Pinterest images belong to their creators,
    so we use them to look at, not to copy into the game or this repo.</p>
  <ul class="pins">{plinks}</ul>
  <div class="foot"><span>delhi6</span><span>{num}</span></div>
</section>"""


HTML = f"""<!doctype html><html><head><meta charset="utf-8"><title>delhi6 Build Plan</title>
<style>
@page {{ size: A4 landscape; margin: 0; }}
* {{ box-sizing: border-box; margin: 0; padding: 0; }}
body {{ font-family: "Segoe UI", "Helvetica Neue", Arial, sans-serif; color: {INK}; background: #fff; }}
.page {{ width: 297mm; height: 210mm; padding: 22mm 24mm; page-break-after: always; position: relative; overflow: hidden; }}
.page:last-child {{ page-break-after: auto; }}
.eyebrow {{ font-size: 10px; letter-spacing: 3px; text-transform: uppercase; color: {MUTE}; }}
h1 {{ font-size: 54px; font-weight: 700; letter-spacing: -1.5px; line-height: 1; }}
h2 {{ font-size: 26px; font-weight: 600; letter-spacing: -0.5px; margin: 6px 0 4px; }}
h3 {{ font-size: 17px; font-weight: 600; }}
.lede {{ font-size: 13px; color: {MUTE}; line-height: 1.6; max-width: 150mm; }}
.foot {{ position: absolute; left: 24mm; right: 24mm; bottom: 12mm; display: flex; justify-content: space-between;
  font-size: 9px; color: {MUTE}; letter-spacing: 1px; }}
/* cover */
.cover {{ padding: 0; background: {HAZE}; }}
.cover .art {{ position: absolute; left: 0; right: 0; bottom: 0; width: 100%; height: auto; }}
.cover .title {{ position: absolute; left: 24mm; top: 22mm; }}
.cover .title .lede {{ margin-top: 10px; }}
.cover .facts {{ display: flex; gap: 40px; margin-top: 18px; font-size: 11px; color: {MUTE}; }}
.cover .facts b {{ display: block; font-size: 14px; color: {INK}; font-weight: 600; margin-bottom: 2px; }}
/* timeline */
.tl {{ width: 100%; margin-top: 16mm; }}
.tl .grid {{ stroke: #ece5da; stroke-width: 1; }}
.tl .axis {{ stroke: {SAND}; stroke-width: 2; }}
.tl .wk {{ font-size: 9px; fill: {MUTE}; letter-spacing: .5px; }}
.tl .pname {{ font-size: 13px; font-weight: 600; fill: {INK}; }}
.tl .pcode {{ font-weight: 700; }}
.tl .bar {{ font-size: 10px; fill: #fff; font-weight: 600; letter-spacing: .3px; }}
.tl .deadline {{ font-size: 11px; fill: {CLAY}; font-weight: 700; text-anchor: end; }}
.tl .today {{ font-size: 11px; fill: {TEAL}; font-weight: 700; }}
/* cards */
.cards {{ display: grid; gap: 9mm; margin-top: 12mm; }}
.cards.three {{ grid-template-columns: repeat(3, 1fr); }}
.cards.two {{ grid-template-columns: repeat(2, 1fr); max-width: 170mm; }}
.card {{ border: 1px solid #ece5da; border-radius: 10px; padding: 7mm 7mm 6mm; background: #fff; }}
.cardtop {{ display: flex; gap: 12px; align-items: center; }}
.icon {{ width: 44px; height: 44px; flex: none; }}
.code {{ font-size: 9px; letter-spacing: 2px; text-transform: uppercase; font-weight: 700; }}
.dates {{ font-size: 10px; color: {MUTE}; margin-top: 2px; }}
.goal {{ font-size: 12.5px; font-weight: 600; margin: 5mm 0 3mm; }}
.card ul {{ list-style: none; font-size: 11.5px; line-height: 1.75; color: #3a332c; }}
.card li::before {{ content: "—  "; color: {SAND}; }}
.exit {{ display: flex; align-items: center; gap: 8px; margin-top: 5mm; padding-top: 4mm; border-top: 1px solid #f0eae1;
  font-size: 11px; font-weight: 600; }}
.exit span {{ width: 8px; height: 8px; border-radius: 50%; display: inline-block; }}
/* scope page */
.cols {{ display: grid; grid-template-columns: repeat(3, 1fr); gap: 12mm; margin-top: 12mm; }}
.col h3 {{ margin-bottom: 4mm; }}
.col ul {{ list-style: none; font-size: 11.5px; line-height: 1.8; color: #3a332c; }}
.col li::before {{ content: "—  "; color: {SAND}; }}
.pill {{ display: inline-block; font-size: 9px; letter-spacing: 1.5px; text-transform: uppercase; padding: 3px 9px;
  border-radius: 20px; background: {HAZE}; color: {MUTE}; margin-bottom: 3mm; }}
.rhythm {{ display: grid; grid-template-columns: repeat(5, 1fr); gap: 5mm; margin-top: 12mm; }}
.day {{ border-top: 3px solid {SAND}; padding-top: 3mm; font-size: 11px; color: #3a332c; line-height: 1.6; }}
.day b {{ display: block; font-size: 12px; color: {INK}; margin-bottom: 1mm; }}
/* photo cover + mood board */
.photocover {{ padding: 0; background: #0b0d16 center/cover no-repeat; }}
.photocover .shade {{ position: absolute; inset: 0; background: linear-gradient(90deg, rgba(8,8,14,.88) 0%, rgba(8,8,14,.55) 42%, rgba(8,8,14,0) 72%); }}
.photocover .title {{ position: absolute; left: 24mm; bottom: 26mm; color: #fff; max-width: 140mm; }}
.photocover .eyebrow {{ color: #e8c58f; }}
.photocover .lede {{ color: #d9d4cc; margin-top: 10px; }}
.photocover .facts {{ display: flex; gap: 36px; margin-top: 18px; font-size: 11px; color: #b9b2a8; }}
.photocover .facts b {{ display: block; font-size: 14px; color: #fff; font-weight: 600; margin-bottom: 2px; }}
.photocover .credit {{ position: absolute; right: 24mm; bottom: 12mm; font-size: 8px; color: #8f8a84; }}
.mood {{ display: grid; grid-template-columns: 1fr 1fr; gap: 7mm 8mm; margin-top: 9mm; }}
.mood figure {{ margin: 0; }}
.mood .ph {{ height: 56mm; border-radius: 6px; background-size: cover; background-position: center; }}
.mood figcaption {{ display: flex; justify-content: space-between; align-items: center; margin-top: 2.5mm;
  font-size: 11px; color: #3a332c; }}
.sw {{ display: flex; gap: 3px; }}
.sw i {{ width: 12px; height: 12px; border-radius: 3px; display: inline-block; }}
table.credits {{ border-collapse: collapse; width: 100%; margin-top: 7mm; font-size: 10px; }}
.credits th, .credits td {{ text-align: left; padding: 2.4mm 2mm; border-bottom: 1px solid #f0eae1; vertical-align: top; }}
.credits th {{ font-size: 9px; letter-spacing: 1.5px; text-transform: uppercase; color: {MUTE}; font-weight: 600; }}
.credits .url {{ color: {MUTE}; word-break: break-all; }}
.pins {{ list-style: none; columns: 2; font-size: 10.5px; color: {TEAL}; line-height: 1.9; margin-top: 3mm; }}
</style></head><body>

<section class="page photocover" style="background-image:url({COVER['file']})">
  <div class="shade"></div>
  <div class="title">
    <div class="eyebrow">delhi6 · build plan</div>
    <h1 style="margin-top:6px">Old Delhi, alive.</h1>
    <p class="lede">A realistic open-world slice of Chandni Chowk, built from real maps, real weather and real streets
      in Unreal Engine 5. Two developers, one AI implementer, eight and a half weeks.</p>
    <div class="facts"><div><b>10 Nov 2026</b>start</div><div><b>10 Jan 2027</b>deadline</div>
      <div><b>5 phases</b>one milestone each</div><div><b>Rayan + co-developer</b>with Claude</div></div>
  </div>
  <div class="credit">Photo: {COVER['artist']}, {COVER['license']}, Wikimedia Commons</div>
</section>

{mood_page(MB[:4], "The final look: light and landmarks", "Target look · 1", "02")}
{mood_page(MB[4:], "The final look: streets, mist and colour", "Target look · 2", "03")}

<section class="page">
  <div class="eyebrow">Timeline</div>
  <h2>Five phases to launch</h2>
  <p class="lede">Each phase ends with something playable. After a short first week, phases run Monday
    to Sunday, two weeks each, so every phase closes with a Sunday review and a build.</p>
  {timeline()}
  <div class="foot"><span>delhi6</span><span>04</span></div>
</section>

<section class="page">
  <div class="eyebrow">Phases 01 – 03</div>
  <h2>Build the city, then bring it to life</h2>
  <div class="cards three">{cards(PHASES[:3])}</div>
  <div class="foot"><span>delhi6</span><span>05</span></div>
</section>

<section class="page">
  <div class="eyebrow">Phases 04 – 05</div>
  <h2>Make it a game, then ship it</h2>
  <div class="cards two">{cards(PHASES[3:])}</div>
  <div class="foot"><span>delhi6</span><span>06</span></div>
</section>

<section class="page">
  <div class="eyebrow">Scope and team</div>
  <h2>What "complete" means on 10 January</h2>
  <p class="lede">Two months is enough for one polished, playable district. The rest of Old Delhi and South Delhi
    come after launch as updates, using the same pipeline.</p>
  <div class="cols">
    <div class="col"><span class="pill">In version 1.0</span><h3>Chandni Chowk slice</h3><ul>
      <li>500 m around Chandni Chowk, Dariba and Kinari</li><li>Four weathers and a full day–night cycle</li>
      <li>Crowds, rickshaws, birds, street sound</li><li>A short Diwali-week story, 3 to 5 missions</li>
      <li>60 fps on an RTX 3070</li></ul></div>
    <div class="col"><span class="pill">After launch</span><h3>Updates</h3><ul>
      <li>The full walled city (2.9 × 2.2 km)</li><li>All 52 character types</li>
      <li>Hero landmarks from photogrammetry</li><li>South Delhi: Lajpat, Sarojini, GK, Hauz Khas</li>
      <li>More stories and festivals</li></ul></div>
    <div class="col"><span class="pill">Team</span><h3>Who does what</h3><ul>
      <li><b>Rayan</b> · art direction, story, reference photos and sound from Delhi</li>
      <li><b>Co-developer</b> · engine, physics, crowds, performance, builds</li>
      <li><b>Claude</b> · generators, C++ and Python, imports, research, docs</li>
      <li><b>Together</b> · weekly review, playtests, launch</li></ul></div>
  </div>
  <div class="rhythm">
    <div class="day"><b>Monday</b>Plan the week from the phase card</div>
    <div class="day"><b>Tue – Thu</b>Build; Claude ships daily changes</div>
    <div class="day"><b>Friday</b>Performance capture and screenshots</div>
    <div class="day"><b>Saturday</b>Playtest and bug list</div>
    <div class="day"><b>Sunday</b>Milestone review, tag a build</div>
  </div>
  <div class="foot"><span>delhi6</span><span>07</span></div>
</section>
{credits_page("08")}
</body></html>"""

(HERE / "delhi6_build_timeline.html").write_text(HTML, encoding="utf-8")
print("wrote", HERE / "delhi6_build_timeline.html")

