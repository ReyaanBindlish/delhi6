"""Cinematic A3 phase poster: DHURANDHAR: THE GAME, whole game in 9 weeks (10 Nov 2026 -> 10 Jan 2027).

python make_phase_poster.py -> phase_poster.html, then Edge headless prints a vector A3 PDF (2 pages).
"""
import datetime as dt
from pathlib import Path

HERE = Path(__file__).resolve().parent
D = dt.date
START, END = D(2026, 11, 10), D(2027, 1, 10)

import os
BW = os.environ.get("BW") == "1"   # black-and-white print version
BG, BG2, INK, DIM, LINE = "#0a0c12", "#11141d", "#f3ede2", "#8c8a92", "#232736"
GOLD, RED, TEAL, VIOLET, BLUE, SAFF = "#e9b44c", "#e2553a", "#3fb5a3", "#9b7bd8", "#4f8fe0", "#f08a24"
OVL = "10,12,18"     # header overlay colour (rgb)
BARTXT = "#0a0c12"   # text on bars
GLOWBG, BODY, IMGOP, IMGF = "#1b2236", "#d0cbc2", ".55", "none"
if BW:
    GLOWBG, BODY, IMGOP, IMGF = "#ffffff", "#1a1a1a", ".28", "grayscale(1)"
    BG, BG2, INK, DIM, LINE = "#ffffff", "#ffffff", "#000000", "#5a5a5a", "#cfcfcf"
    GOLD, RED, TEAL, VIOLET, BLUE, SAFF = "#000000", "#000000", "#222222", "#4a4a4a", "#6e6e6e", "#3a3a3a"
    OVL, BARTXT = "255,255,255", "#ffffff"

# Ops: code, codename, subtitle, start, end, gradient (a, b), objectives, exit, milestone label
OPS = [
    ("01", "COLLECTION RUN", "Setup", D(2026, 11, 10), D(2026, 11, 15), (TEAL, "#2a7f9e"),
     ["Unreal Engine 5.8.4 compiled with our C++", "Terrain, Lyari and Delhi data imported", "Unreal MCP live in the editor"],
     "First lit street in engine"),
    ("02", "THE LEDGER", "Core systems", D(2026, 11, 16), D(2026, 11, 29), (RED, SAFF),
     ["Vitals, disease curve, coughing fits", "Money ledger, hidden suspicion, notebook", "Falls, rail heights, ballistics, katta misfire"],
     "One in-game day runs end to end"),
    ("03", "GULBAHAR LINES", "Karachi · Act 4", D(2026, 11, 16), D(2026, 12, 13), (GOLD, SAFF),
     ["District built from the Lyari slice", "Shrine, tankers, hawala shops, gambling den", "Sattar or Shahzaib, the Rangers raid"],
     "Act 4 playable start to finish"),
    ("04", "KATORA", "Delhi NCR · Acts 1 & 2", D(2026, 11, 23), D(2026, 12, 20), (VIOLET, "#c45fb0"),
     ["Vasundhara, Khoda Colony, Loni flyover", "Nizamuddin begging loop with Mangal", "Collection run, the wedding, the arrest"],
     "Part 1 playable"),
    ("05", "WAPSI", "Acts 3, 5 & 6", D(2026, 12, 7), D(2026, 12, 27), (BLUE, TEAL),
     ["Farm, goodbye, jetty and the tide boat", "Empty house, the hunt, the balcony fight", "The rewind and the coda"],
     "All acts playable"),
    ("06", "THE BALCONY", "Polish & launch", D(2026, 12, 28), D(2027, 1, 10), (INK, GOLD),
     ["60 fps on an RTX 3070", "Playtests, rating, legal and sensitivity", "Store page, trailer, final build"],
     "Launch · 10 January 2027"),
]

# detailed tracks under each op (row label, owner, op index, start, end, bar text)
TRACKS = [
    ("Engine, tools, pipeline", "Claude · Rayan", 0, D(2026, 11, 10), D(2026, 11, 15), "UE 5.8 · MCP"),
    ("Body, money, suspicion", "Rayan · Claude", 1, D(2026, 11, 16), D(2026, 11, 29), "Vitals · disease · ledger · suspicion"),
    ("Physics, combat, vehicles", "Rayan", 1, D(2026, 11, 16), D(2026, 12, 6), "Falls · rails · ballistics · cars · boat"),
    ("Crowds, weather, calendar", "Claude", 1, D(2026, 11, 16), D(2026, 11, 29), "Governor · climate · clock"),
    ("Karachi world", "Claude", 2, D(2026, 11, 16), D(2026, 12, 6), "Lyari slice · shrine · tankers · hawala"),
    ("Act 4 gameplay", "Tarun · Rayan", 2, D(2026, 11, 30), D(2026, 12, 13), "Nobody · Runner · Inside · raid"),
    ("Delhi NCR world", "Claude", 3, D(2026, 11, 23), D(2026, 12, 13), "Vasundhara · Khoda · Loni · Nizamuddin"),
    ("Acts 1 & 2 gameplay", "Tarun · Rayan", 3, D(2026, 12, 7), D(2026, 12, 20), "Collection run · fall · begging loop"),
    ("Cast, voice, motion", "Tarun · contract", 3, D(2026, 11, 23), D(2026, 12, 27), "10 MetaHumans · 12 crowd types · VO · mocap"),
    ("Small sets", "Claude · Rayan", 4, D(2026, 11, 30), D(2026, 12, 13), "Farm · dhaba · jetty · Dasna"),
    ("Acts 3, 5, 6 gameplay", "Tarun · Rayan", 4, D(2026, 12, 14), D(2026, 12, 27), "Farm · jetty · hunt · balcony · rewind"),
    ("Polish and release", "Everyone", 5, D(2026, 12, 28), D(2027, 1, 10), "60 fps · fixes · rating · build"),
]

MILESTONES = [
    (D(2026, 11, 15), "M1", "FIRST LIGHT"), (D(2026, 11, 29), "M2", "LEDGER OPEN"),
    (D(2026, 12, 13), "M3", "GULBAHAR LIVE"), (D(2026, 12, 27), "M4", "ALL ACTS"),
    (D(2027, 1, 5), "M5", "RELEASE CANDIDATE"), (D(2027, 1, 10), "M6", "LAUNCH"),
]

CUTS = ["Side content: tea stall, cricket, Kasur trip", "Act 3 farm and route in as cinematics",
        "Delhi hubs reduced to playable interiors", "Crowds limited to 12 archetypes", "Part 2 Wapsi as a later release"]

WEEKS = [(START, D(2026, 11, 15))]
d = D(2026, 11, 16)
while d <= END:
    WEEKS.append((d, min(d + dt.timedelta(days=6), END)))
    d += dt.timedelta(days=7)

W, H = 1520, 830


def gcol(c):
    """Colour in colour mode; luminance-matched grey (darkened for print) in BW mode."""
    if not BW:
        return c
    c = c.lstrip("#")
    r, g, b = int(c[0:2], 16), int(c[2:4], 16), int(c[4:6], 16)
    y = int((0.299 * r + 0.587 * g + 0.114 * b) * 0.55)
    return f"#{y:02x}{y:02x}{y:02x}"


def GLOW(i):
    return "" if BW else f'filter="url(#glow{i})"'
GX0, GX1 = 300, W - 20
DAYS = (END - START).days + 1


def X(day):
    return GX0 + (day - START).days / DAYS * (GX1 - GX0)


def grad_defs():
    out = ['<defs>']
    for i, op in enumerate(OPS):
        a, b = gcol(op[5][0]), gcol(op[5][1])
        out.append(f'<linearGradient id="g{i}" x1="0" x2="1"><stop offset="0" stop-color="{a}"/><stop offset="1" stop-color="{b}"/></linearGradient>')
        out.append(f'<filter id="glow{i}" x="-20%" y="-80%" width="140%" height="260%"><feGaussianBlur stdDeviation="5" result="b"/>'
                   f'<feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge></filter>')
    out.append('</defs>')
    return "".join(out)


def poster():
    s = [f'<svg viewBox="0 0 {W} {H}" class="dg" xmlns="http://www.w3.org/2000/svg">', grad_defs()]
    top = 150
    # week columns
    for i, (a, b) in enumerate(WEEKS):
        x0, x1 = X(a), X(b + dt.timedelta(days=1))
        if i % 2 == 0:
            s.append(f'<rect x="{x0:.1f}" y="40" width="{x1 - x0:.1f}" height="{H - 40}" fill="#ffffff" opacity="0.025"/>')
        s.append(f'<line x1="{x0:.1f}" y1="40" x2="{x0:.1f}" y2="{H - 70}" stroke="{LINE}" stroke-width="1"/>')
        s.append(f'<text x="{(x0 + x1) / 2:.1f}" y="62" class="wk">W{i + 1:02d}</text>')
        lab = f"{a.day}-{b.day} {b.strftime('%b').upper()}" if a.month == b.month else f"{a.day} {a.strftime('%b').upper()}-{b.day} {b.strftime('%b').upper()}"
        s.append(f'<text x="{(x0 + x1) / 2:.1f}" y="78" class="wkd">{lab}</text>')
    for m0, name in ((D(2026, 11, 10), "NOV 2026"), (D(2026, 12, 1), "DEC 2026"), (D(2027, 1, 1), "JAN 2027")):
        s.append(f'<text x="{X(m0) + 5:.1f}" y="30" class="month">{name}</text>')
        s.append(f'<line x1="{X(m0):.1f}" y1="16" x2="{X(m0):.1f}" y2="40" stroke="{GOLD}" stroke-width="2"/>')
    # op bands (big)
    oy = 98
    for i, (code, name, sub, a, b, *_r) in enumerate(OPS):
        x0, x1 = X(a), X(b + dt.timedelta(days=1)) - 3
        y = oy + (i % 2) * 0
        s.append(f'<rect x="{x0:.1f}" y="{y}" width="{x1 - x0:.1f}" height="4" fill="url(#g{i})"/>')
    # op lane headers on the left + op chevrons
    y = top
    for i, (code, name, sub, a, b, *_r) in enumerate(OPS):
        rows = [t for t in TRACKS if t[2] == i]
        block_h = 24 + len(rows) * 27
        s.append(f'<rect x="0" y="{y}" width="4" height="{block_h - 10}" fill="url(#g{i})"/>')
        s.append(f'<text x="16" y="{y + 15}" class="opn">OP {code}</text>')
        s.append(f'<text x="62" y="{y + 15}" class="opc">{name}</text>')
        x0, x1 = X(a), X(b + dt.timedelta(days=1)) - 3
        s.append(f'<rect x="{x0:.1f}" y="{y + 3}" width="{x1 - x0:.1f}" height="16" rx="3" fill="url(#g{i})" opacity="0.18"/>')
        s.append(f'<text x="{x0 + 8:.1f}" y="{y + 15}" class="opsub">{sub.upper()}</text>')
        ry = y + 24
        for label, owner, _op, ta, tb, txt in rows:
            s.append(f'<text x="16" y="{ry + 14}" class="trk">{label}</text>')
            s.append(f'<text x="16" y="{ry + 26}" class="own">{owner}</text>')
            bx0, bx1 = X(ta), X(tb + dt.timedelta(days=1)) - 3
            s.append(f'<rect x="{bx0:.1f}" y="{ry + 4}" width="{bx1 - bx0:.1f}" height="20" rx="10" fill="url(#g{i})" {GLOW(i)}/>')
            s.append(f'<text x="{bx0 + 11:.1f}" y="{ry + 18}" class="bar">{txt}</text>')
            ry += 27
        y += block_h
    # milestones
    my = y + 22
    s.append(f'<line x1="{GX0}" y1="{my}" x2="{GX1}" y2="{my}" stroke="{GOLD}" stroke-width="1.5"/>')
    s.append(f'<text x="16" y="{my + 4}" class="opn">MILESTONES</text>')
    for d_, code, name in MILESTONES:
        x = X(d_ + dt.timedelta(days=1)) - 2
        s.append(f'<line x1="{x:.1f}" y1="40" x2="{x:.1f}" y2="{my - 10}" stroke="{GOLD}" stroke-width="0.8" stroke-dasharray="2 5" opacity="0.55"/>')
        s.append(f'<path d="M{x:.1f},{my - 9} l9,9 l-9,9 l-9,-9 z" fill="{GOLD}" {GLOW(0)}/>')
        anchor = "end" if code == "M6" else "middle"
        dx = -12 if code == "M6" else 0
        drop = 30 if code == "M5" else 0          # second line so labels never collide
        if drop:
            s.append(f'<line x1="{x:.1f}" y1="{my + 10}" x2="{x:.1f}" y2="{my + 46}" stroke="{GOLD}" stroke-width="0.8" opacity="0.6"/>')
        s.append(f'<text x="{x + dx:.1f}" y="{my + 28 + drop}" class="msc" text-anchor="{anchor}">{code} · {d_.day} {d_.strftime("%b").upper()}</text>')
        s.append(f'<text x="{x + dx:.1f}" y="{my + 43 + drop}" class="msn" text-anchor="{anchor}">{name}</text>')
    s.append("</svg>")
    return "\n".join(s)


def cards():
    out = []
    for i, (code, name, sub, a, b, (ca, cb), objs, exit_) in enumerate(OPS):
        li = "".join(f"<li>{o}</li>" for o in objs)
        days = (b - a).days + 1
        out.append(f"""<div class="card" style="--a:{gcol(ca)};--b:{gcol(cb)}">
  <div class="num">{code}</div>
  <div class="cb"><div class="kick">OPERATION · {sub.upper()}</div><h3>{name}</h3>
  <div class="dt">{a.strftime('%d %b').upper()} &rarr; {b.strftime('%d %b %Y').upper()} &nbsp;·&nbsp; {days} DAYS</div>
  <ul>{li}</ul><div class="exit"><span>EXIT</span>{exit_}</div></div></div>""")
    return "\n".join(out)


HTML = f"""<!doctype html><html><head><meta charset="utf-8"><title>Dhurandhar Phase Poster</title>
<link href="https://fonts.googleapis.com/css2?family=Bebas+Neue&family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400;600&display=block" rel="stylesheet">
<style>
@page {{ size: A3 landscape; margin: 0; }}
* {{ box-sizing: border-box; margin: 0; padding: 0; }}
body {{ background: {BG}; color: {INK}; font-family: Inter, "Segoe UI", sans-serif; -webkit-print-color-adjust: exact; print-color-adjust: exact; }}
.sheet {{ width: 420mm; height: 297mm; position: relative; overflow: hidden; page-break-after: always;
  background: radial-gradient(1200px 500px at 85% -10%, {GLOWBG} 0%, {BG} 60%); }}
.sheet:last-child {{ page-break-after: auto; }}
.hero {{ position: relative; height: 62mm; padding: 13mm 16mm 0; }}
.hero::before {{ content: ""; position: absolute; inset: 0; background: url(moodboard/st_41.jpg) center 62%/cover; opacity: {IMGOP}; filter: {IMGF}; }}
.hero::after {{ content: ""; position: absolute; inset: 0; background: linear-gradient(90deg, {BG} 8%, rgba({OVL},.55) 55%, rgba({OVL},.2)),
  linear-gradient(0deg, {BG} 0%, rgba({OVL},0) 55%); }}
.hero > * {{ position: relative; z-index: 1; }}
.mono {{ font-family: "JetBrains Mono", Consolas, monospace; }}
.class {{ font-family: "JetBrains Mono", monospace; font-size: 8pt; letter-spacing: 3px; color: {GOLD}; }}
.class b {{ color: {RED}; }}
h1 {{ font-family: "Bebas Neue", Impact, sans-serif; font-size: 66pt; letter-spacing: 3px; line-height: .9; margin-top: 3mm; font-weight: 400; }}
h1 span {{ color: {GOLD}; }}
.tag {{ font-family: "JetBrains Mono", monospace; font-size: 8.5pt; letter-spacing: 2px; color: {DIM}; margin-top: 2.5mm; }}
.stats {{ position: absolute; right: 16mm; top: 14mm; display: flex; gap: 9mm; text-align: right; z-index: 2; }}
.stats div {{ font-family: "JetBrains Mono", monospace; font-size: 7pt; letter-spacing: 2px; color: {DIM}; }}
.stats b {{ display: block; font-family: "Bebas Neue", sans-serif; font-weight: 400; font-size: 30pt; letter-spacing: 1.5px; color: {INK}; line-height: 1; }}
.stats .g b {{ color: {GOLD}; }}
.stamp {{ position: absolute; right: 18mm; top: 42mm; z-index: 2; border: 2px solid {RED}; color: {RED}; font-family: "JetBrains Mono", monospace;
  font-weight: 600; font-size: 9pt; letter-spacing: 4px; padding: 1.5mm 4mm; transform: rotate(-4deg); opacity: .85; }}
.dg {{ width: calc(100% - 28mm); margin: 2mm 14mm 0; display: block; }}
.dg .wk {{ font-family: "Bebas Neue", sans-serif; font-size: 19px; letter-spacing: 1.5px; text-anchor: middle; fill: {INK}; }}
.dg .wkd {{ font-family: "JetBrains Mono", monospace; font-size: 8.5px; text-anchor: middle; fill: {DIM}; letter-spacing: .5px; }}
.dg .month {{ font-family: "JetBrains Mono", monospace; font-size: 10px; font-weight: 600; letter-spacing: 3px; fill: {GOLD}; }}
.dg .opn {{ font-family: "JetBrains Mono", monospace; font-size: 10px; font-weight: 600; letter-spacing: 2px; fill: {DIM}; }}
.dg .opc {{ font-family: "Bebas Neue", sans-serif; font-size: 19px; letter-spacing: 1.8px; fill: {INK}; }}
.dg .opsub {{ font-family: "JetBrains Mono", monospace; font-size: 8.5px; letter-spacing: 2px; fill: {INK}; opacity: .85; }}
.dg .trk {{ font-size: 11.5px; font-weight: 600; fill: {INK}; }}
.dg .own {{ font-family: "JetBrains Mono", monospace; font-size: 8.5px; fill: {DIM}; }}
.dg .bar {{ font-size: 10px; font-weight: 700; fill: {BARTXT}; letter-spacing: .2px; }}
.dg .msc {{ font-family: "JetBrains Mono", monospace; font-size: 10px; font-weight: 600; fill: {GOLD}; letter-spacing: 1px; }}
.dg .msn {{ font-family: "Bebas Neue", sans-serif; font-size: 15px; letter-spacing: 1.5px; fill: {INK}; }}
.foot {{ position: absolute; left: 16mm; right: 16mm; bottom: 9mm; display: grid; grid-template-columns: 1fr 1.25fr 1fr; gap: 12mm;
  border-top: 1px solid {LINE}; padding-top: 4mm; font-size: 8.5pt; color: {BODY}; line-height: 1.55; }}
.foot h4 {{ font-family: "JetBrains Mono", monospace; font-size: 7.5pt; letter-spacing: 3px; color: {GOLD}; font-weight: 600; margin-bottom: 1.5mm; }}
.foot ol {{ padding-left: 4.5mm; }}
.coords {{ font-family: "JetBrains Mono", monospace; font-size: 7.5pt; color: {DIM}; letter-spacing: 1.5px; }}
/* page 2 */
.p2head {{ padding: 15mm 16mm 0; display: flex; justify-content: space-between; align-items: flex-end; }}
.p2head h2 {{ font-family: "Bebas Neue", sans-serif; font-weight: 400; font-size: 54pt; letter-spacing: 3px; line-height: .9; }}
.p2head h2 span {{ color: {GOLD}; }}
.grid {{ display: grid; grid-template-columns: repeat(3, 1fr); gap: 7mm; padding: 9mm 16mm 0; }}
.card {{ position: relative; background: linear-gradient(160deg, {BG2}, {BG2}); border: 1px solid {LINE}; border-radius: 4mm;
  padding: 7mm 7mm 6mm; overflow: hidden; min-height: 92mm; }}
.card::before {{ content: ""; position: absolute; left: 0; right: 0; top: 0; height: 3px; background: linear-gradient(90deg, var(--a), var(--b)); }}
.num {{ position: absolute; right: 6mm; top: 2mm; font-family: "Bebas Neue", sans-serif; font-size: 96pt; line-height: 1;
  color: var(--a); opacity: .16; }}
.cb {{ position: relative; }}
.kick {{ font-family: "JetBrains Mono", monospace; font-size: 7.5pt; letter-spacing: 2.5px; color: var(--a); font-weight: 600; }}
.card h3 {{ font-family: "Bebas Neue", sans-serif; font-weight: 400; font-size: 34pt; letter-spacing: 2px; line-height: 1; margin-top: 2mm; }}
.dt {{ font-family: "JetBrains Mono", monospace; font-size: 7.5pt; letter-spacing: 1.5px; color: {DIM}; margin-top: 2mm; }}
.card ul {{ list-style: none; margin-top: 5mm; font-size: 10pt; line-height: 1.75; color: {BODY}; }}
.card li::before {{ content: "▸  "; color: var(--a); }}
.exit {{ margin-top: 5mm; padding-top: 3.5mm; border-top: 1px dashed {LINE}; font-size: 10pt; font-weight: 600; }}
.exit span {{ font-family: "JetBrains Mono", monospace; font-size: 7pt; letter-spacing: 2px; color: {BARTXT}; background: var(--a);
  padding: .6mm 2mm; border-radius: 1mm; margin-right: 3mm; }}
</style></head><body>

<section class="sheet">
  <div class="hero">
    <div class="class">PRODUCTION SCHEDULE // <b>EYES ONLY</b> // WORKING TITLE</div>
    <h1>DHURANDHAR<span>:</span> THE GAME</h1>
    <div class="tag">PART 1 · KATORA&nbsp;&nbsp;/&nbsp;&nbsp;PART 2 · WAPSI&nbsp;&nbsp;//&nbsp;&nbsp;SIX ACTS&nbsp;&nbsp;//&nbsp;&nbsp;UNREAL ENGINE 5.8</div>
  </div>
  <div class="stats"><div><b>10.11.26</b>START</div><div class="g"><b>10.01.27</b>LAUNCH</div><div><b>09</b>WEEKS</div><div><b>06</b>OPS</div><div><b>25</b>BEATS</div></div>
  <div class="stamp">9-WEEK OPERATION</div>
  {poster()}
  <div class="foot">
    <div><h4>THE TEAM</h4>Tarun · story, script, direction<br>Rayan · engine, gameplay, physics<br>Claude · worlds, C++, data, tools<br>Contract · voice, motion capture, legal
      <div class="coords" style="margin-top:2mm">28.65°N 77.23°E &nbsp;//&nbsp; 24.87°N 66.99°E</div></div>
    <div><h4>HOW NINE WEEKS WORKS</h4>Worlds are generated from real maps, so the team's time goes into systems and story.
      Systems land first because the story is told through them. Karachi, half the game, is built before Delhi.
      Every Sunday ends with a playable build and a milestone review.</div>
    <div><h4>CUT LINE · IF A MILESTONE SLIPS</h4><ol>{"".join(f"<li>{c}</li>" for c in CUTS)}</ol></div>
  </div>
</section>

<section class="sheet">
  <div class="p2head"><div><div class="class">OPERATIONS BRIEF // 06 PHASES</div><h2>SIX OPS<span>.</span> NINE WEEKS<span>.</span> ONE STORY<span>.</span></h2></div>
    <div class="stats" style="position:static"><div><b>13</b>TRACKS</div><div><b>06</b>MILESTONES</div><div class="g"><b>01</b>LAUNCH</div></div></div>
  <div class="grid">{cards()}</div>
  <div class="foot" style="grid-template-columns:2fr 1fr">
    <div><h4>RULES OF ENGAGEMENT</h4>A phase is done only when its exit criterion is met in a playable build. One engine version for the whole
      project. 60 fps on an RTX 3070 at every milestone. Real places; fictional brands and people. If two milestones slip in a row,
      apply the cut line from the top.</div>
    <div><h4>CREDITS</h4>Header image: India–Pakistan border at night, ISS Expedition 45 crew, NASA, public domain.</div>
  </div>
</section>
</body></html>"""

(HERE / "phase_poster.html").write_text(HTML, encoding="utf-8")
print("wrote", HERE / "phase_poster.html")
