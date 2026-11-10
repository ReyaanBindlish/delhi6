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

START, END = D(2026, 11, 10), D(2027, 11, 10)
TITLE = "DHURANDHAR: THE GAME"
SUBTITLE = "Part 1 · Katora&nbsp;&nbsp;&nbsp;/&nbsp;&nbsp;&nbsp;Part 2 · Wapsi"

PHASES = [
    ("01", "Foundation", D(2026, 11, 10), D(2026, 12, 6), TEAL, "gear",
     "Engine, pipeline and tools",
     ["Unreal Engine 5.8.4, C++ project, Git LFS, CI", "Map pipeline: real footprints to modeled streets",
      "Unreal MCP for live editing", "Terrain, weather and crowd data imported"],
     "First lit street in Unreal"),
    ("02", "Core systems", D(2026, 12, 7), D(2027, 2, 28), CLAY, "path",
     "The rules the story depends on",
     ["Money ledger, health without regeneration", "Stamina and the disease curve, coughing fits",
      "Hidden suspicion, notebook map, daily saves", "Falls, rail heights, ballistics, katta misfire",
      "Calendar, weather, crowds, traffic"],
     "One in-game day playable end to end"),
    ("03", "Karachi · Act 4", D(2027, 3, 1), D(2027, 5, 30), SAFFRON, "city",
     "Half the game: Gulbahar Lines",
     ["District built from the Lyari slice", "Shrine, tankers, hawala shops, gambling den",
      "Sattar, Shahzaib, Qureshi; the branch choice", "The Rangers raid escape without a weapon"],
     "Act 4 playable start to finish"),
    ("04", "Delhi NCR · Acts 1, 2, 6", D(2027, 5, 31), D(2027, 8, 29), INK, "people",
     "The rise, the fall, the end",
     ["Vasundhara house and Bablu's builder flat", "Khoda Colony hub, Loni flyover and dhaba",
      "Nizamuddin platform survival loop, Mangal", "The hunt and the balcony fight"],
     "Acts 1, 2 and 6 playable"),
    ("05", "Acts 3, 5 & cinematics", D(2027, 8, 30), D(2027, 10, 3), TEAL, "boat",
     "Connective tissue and the big moments",
     ["Dehradun farm, Sarai Kale Khan pitch", "Korangi checkpoint, jetty, the tide-timed boat",
      "Performance capture and Hindi, Urdu, Punjabi voice", "The rewind and the coda"],
     "Content complete (alpha)"),
    ("06", "Polish & launch", D(2027, 10, 4), D(2027, 11, 10), CLAY, "flag",
     "Ship both parts",
     ["60 fps on an RTX 3070", "Playtests, sensitivity and legal review",
      "Title licence or fallback names", "Rating, store page, trailer, launch"],
     "Launch on 10 November 2027"),
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
        "boat": f'<path d="M12 38 h40 l-6 10 h-28 z" {st}/><path d="M32 38 V14 l12 18 h-12" {st}/>'
                f'<path d="M10 54 q6 -4 11 0 t11 0 t11 0 t11 0" {st}/>',
    }[kind]
    return base + body + "</svg>"


# ------------------------------------------------------------------ timeline
def timeline():
    W, L, R, top = 1000, 200, 20, 46
    days = (END - START).days
    X = lambda d: L + (d - START).days / days * (W - L - R)
    rowh = 50
    h = top + rowh * len(PHASES) + 70
    s = [f'<svg viewBox="0 0 {W} {h}" class="tl">']
    d = D(START.year, START.month, 1)
    while d <= END:  # month grid
        nx = D(d.year + (d.month == 12), d.month % 12 + 1, 1)
        if d >= START:
            s.append(f'<line x1="{X(d):.1f}" y1="{top - 10}" x2="{X(d):.1f}" y2="{h - 50}" class="grid"/>')
        mid = max(START, d) + (min(nx, END) - max(START, d)) / 2
        if (min(nx, END) - max(START, d)).days > 12:
            lab = d.strftime("%b") if d.month != 1 else d.strftime("%b %Y")
            s.append(f'<text x="{X(mid):.1f}" y="{top - 18}" class="wk" text-anchor="middle">{lab}</text>')
        d = nx
    for i, (code, name, a, b, col, *_rest) in enumerate(PHASES):
        y = top + i * rowh
        s.append(f'<text x="0" y="{y + 25}" class="pname"><tspan class="pcode" fill="{col}">{code}</tspan>  {name}</text>')
        bw = X(b + dt.timedelta(days=1)) - X(a) - 3
        s.append(f'<rect x="{X(a):.1f}" y="{y + 8}" width="{bw:.1f}" height="26" rx="13" fill="{col}"/>')
        weeks = round(((b - a).days + 1) / 7)
        lab = f"{weeks} wks"
        s.append(f'<text x="{X(a) + bw / 2:.1f}" y="{y + 25}" class="bar" text-anchor="middle">{lab}</text>')
    y = top + rowh * len(PHASES) + 8
    s.append(f'<line x1="{L}" y1="{y}" x2="{W - R}" y2="{y}" class="axis"/>')
    for code, name, a, b, col, *_rest in PHASES:
        s.append(f'<circle cx="{X(b + dt.timedelta(days=1)) - 2:.1f}" cy="{y}" r="5" fill="{col}"/>')
    s.append(f'<text x="{X(END) - 2:.1f}" y="{y + 26}" class="deadline">Launch · 10 Nov 2027</text>')
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

_ALL = _json.load(open(HERE / "moodboard" / "moodboard_story.json", encoding="utf-8"))
COVER, MB = _ALL[0], _ALL[1:]


def mood_page(items, title, eyebrow, num):
    figs = []
    for m in items:
        sw = "".join(f'<i style="background:{c}"></i>' for c in m["palette"])
        figs.append(f'<figure><div class="ph" style="background-image:url({m["file"]})"></div>'
                    f'<figcaption><span>{m["caption"]}</span><span class="sw">{sw}</span></figcaption></figure>')
    return f"""<section class="page">
  <div class="eyebrow">{eyebrow}</div><h2>{title}</h2>
  <div class="mood">{''.join(figs)}</div>
  <div class="foot"><span>photo references, credits on the last page</span><span>{num}</span></div>
</section>"""


def credits_page(num):
    rows = "".join(f'<tr><td>{m["caption"]}</td><td>{m["artist"]}</td><td>{m["license"]}</td>'
                   f'<td class="url">{m["page"].replace("https://", "")}</td></tr>' for m in _ALL)
    pins = ["Lyari Karachi street photography", "Karachi night street", "Nizamuddin station",
            "Karachi fishing boats dusk", "Ghaziabad builder floor", "Indian railway platform night"]
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
  <div class="foot"><span>build plan</span><span>{num}</span></div>
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
    <div class="eyebrow">Build plan · working title</div>
    <h1 style="margin-top:6px;font-size:46px">{TITLE}</h1>
    <p class="lede" style="letter-spacing:2px;text-transform:uppercase;font-size:11px">{SUBTITLE}</p>
    <p class="lede">A Delhi gangster turned beggar, recruited because he is invisible, sent into Karachi's underworld
      under a dead man's name. Six acts, 14 to 18 hours, built in Unreal Engine 5 from real streets.</p>
    <div class="facts"><div><b>10 Nov 2026</b>start</div><div><b>10 Nov 2027</b>launch</div>
      <div><b>6 phases</b>one milestone each</div><div><b>Tarun + Rayan</b>with Claude</div></div>
  </div>
  <div class="credit">Photo: {COVER['artist']}, {COVER['license']}, via Wikimedia Commons</div>
</section>

{mood_page(MB[:4], "Delhi and Karachi: the platform and the lanes", "Target look · 1", "02")}
{mood_page(MB[4:], "Karachi: tankers, the creek and the jetty", "Target look · 2", "03")}

<section class="page">
  <div class="eyebrow">Timeline</div>
  <h2>Six phases, one year</h2>
  <p class="lede">Systems first, because the story is told through them: money, stamina, suspicion, falls.
    Then Karachi, which is half the game, then Delhi, then the acts that connect them.</p>
  {timeline()}
  <div class="foot"><span>build plan</span><span>04</span></div>
</section>

<section class="page">
  <div class="eyebrow">Phases 01 – 03</div>
  <h2>Tools, rules, and Karachi</h2>
  <div class="cards three">{cards(PHASES[:3])}</div>
  <div class="foot"><span>build plan</span><span>05</span></div>
</section>

<section class="page">
  <div class="eyebrow">Phases 04 – 06</div>
  <h2>Delhi, the big moments, and launch</h2>
  <div class="cards three">{cards(PHASES[3:])}</div>
  <div class="foot"><span>build plan</span><span>06</span></div>
</section>

<section class="page">
  <div class="eyebrow">Scope and team</div>
  <h2>The whole story, with a cut list ready</h2>
  <p class="lede">The plan builds all six acts. If a phase slips, cut from the right-hand list first,
    in that order, so the story itself never loses a beat.</p>
  <div class="cols">
    <div class="col"><span class="pill">Ships</span><h3>Both parts, six acts</h3><ul>
      <li>Ghaziabad, Nizamuddin, Karachi, the creek</li><li>Money, stamina, disease, suspicion systems</li>
      <li>The Sattar or Shahzaib branch</li><li>The balcony, the rewind, the coda</li>
      <li>60 fps on an RTX 3070, 18+ rating</li></ul></div>
    <div class="col"><span class="pill">Cut first if late</span><h3>Cut list</h3><ul>
      <li>1 · Side content (tea stall, cricket, Kasur trip)</li><li>2 · Act 3 farm as a short cutscene</li>
      <li>3 · Smaller Delhi hubs, Karachi stays full</li><li>4 · Fewer crowd character types</li>
      <li>5 · Ship Part 2 (Wapsi) as a second release</li></ul></div>
    <div class="col"><span class="pill">Team</span><h3>Who does what</h3><ul>
      <li><b>Tarun</b> · story, script, direction, playtests</li>
      <li><b>Rayan</b> · engine, physics, crowds, performance, builds</li>
      <li><b>Claude</b> · generators, C++ and Python, imports, research, docs</li>
      <li><b>Hire</b> · voice, performance capture, legal review</li></ul></div>
  </div>
  <div class="rhythm">
    <div class="day"><b>Monday</b>Plan the week from the phase card</div>
    <div class="day"><b>Tue – Thu</b>Build; Claude ships daily changes</div>
    <div class="day"><b>Friday</b>Performance capture and screenshots</div>
    <div class="day"><b>Saturday</b>Playtest and bug list</div>
    <div class="day"><b>Sunday</b>Milestone review, tag a build</div>
  </div>
  <div class="foot"><span>build plan</span><span>07</span></div>
</section>
{credits_page("08")}
</body></html>"""

(HERE / "delhi6_build_timeline.html").write_text(HTML, encoding="utf-8")
print("wrote", HERE / "delhi6_build_timeline.html")


