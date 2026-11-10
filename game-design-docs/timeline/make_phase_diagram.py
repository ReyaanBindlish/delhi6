"""Print-ready A3 phase diagram: DHURANDHAR: THE GAME, whole game in 9 weeks (10 Nov 2026 -> 10 Jan 2027).

python make_phase_diagram.py  -> phase_diagram.html, then Edge headless prints a vector A3 PDF.
Page 1: one-sheet phase diagram (tracks x weeks, owners, milestones, act coverage, cut line).
Page 2: week-by-week deliverables and exit criteria.
"""
import datetime as dt
from pathlib import Path

HERE = Path(__file__).resolve().parent
D = dt.date
START, END = D(2026, 11, 10), D(2027, 1, 10)

INK, PAPER, RULE, MUTE = "#16130f", "#fbf8f3", "#e7dfd2", "#857b70"
C = {"setup": "#2f6b66", "sys": "#b5532c", "world": "#c98a1b", "acts": "#16130f", "cast": "#6b4f8a", "ship": "#3c6e9f"}

# weeks: W1 is the short first week (Tue 10 Nov - Sun 15 Nov), then Monday-Sunday
WEEKS = [(START, D(2026, 11, 15))]
d = D(2026, 11, 16)
while d <= END:
    WEEKS.append((d, min(d + dt.timedelta(days=6), END)))
    d += dt.timedelta(days=7)

# track rows: (group, label, owner, colour key, [(start, end, bar label)])
TRACKS = [
    ("Setup", "Engine, tools, pipeline", "Claude + Rayan", "setup",
     [(D(2026, 11, 10), D(2026, 11, 15), "UE 5.8 · MCP")]),
    ("Systems", "Body, money, suspicion", "Rayan + Claude", "sys",
     [(D(2026, 11, 16), D(2026, 11, 29), "Vitals · disease · ledger · suspicion · notebook")]),
    ("Systems", "Physics, combat, vehicles", "Rayan", "sys",
     [(D(2026, 11, 16), D(2026, 12, 6), "Falls · rails · ballistics · katta · cars · boat")]),
    ("Systems", "Crowds, weather, calendar", "Claude", "sys",
     [(D(2026, 11, 16), D(2026, 11, 29), "Governor · climate · story clock")]),
    ("World", "Karachi · Gulbahar Lines", "Claude", "world",
     [(D(2026, 11, 16), D(2026, 12, 6), "From Lyari slice · shrine · tankers · hawala")]),
    ("World", "Delhi NCR locations", "Claude", "world",
     [(D(2026, 11, 23), D(2026, 12, 13), "Vasundhara · Khoda · Loni · Nizamuddin")]),
    ("World", "Small sets", "Claude + Rayan", "world",
     [(D(2026, 11, 30), D(2026, 12, 13), "Farm · dhaba · jetty · Dasna")]),
    ("Cast", "Principal characters", "Tarun + Claude", "cast",
     [(D(2026, 11, 23), D(2026, 12, 13), "10 MetaHumans · Daya's 3 body states")]),
    ("Cast", "Crowds, voice, motion", "Tarun + contract", "cast",
     [(D(2026, 11, 30), D(2026, 12, 27), "12 archetypes · Hindi/Urdu/Punjabi VO · mocap")]),
    ("Acts", "Act 4 · Karachi", "Tarun + Rayan", "acts",
     [(D(2026, 11, 30), D(2026, 12, 13), "Nobody · Runner · Inside · raid")]),
    ("Acts", "Acts 1 & 2 · Ghaziabad, platform", "Tarun + Rayan", "acts",
     [(D(2026, 12, 7), D(2026, 12, 20), "Collection run · fall · begging loop")]),
    ("Acts", "Acts 3, 5, 6 · return and end", "Tarun + Rayan", "acts",
     [(D(2026, 12, 14), D(2026, 12, 27), "Farm · jetty · hunt · balcony · rewind")]),
    ("Ship", "Polish, playtest, release", "Everyone", "ship",
     [(D(2026, 12, 28), D(2027, 1, 10), "60 fps · fixes · rating · store · build")]),
]

MILESTONES = [
    (D(2026, 11, 15), "M1", "Engine running"),
    (D(2026, 11, 29), "M2", "Systems in"),
    (D(2026, 12, 13), "M3", "Act 4 playable"),
    (D(2026, 12, 27), "M4", "All acts playable"),
    (D(2027, 1, 5), "M5", "Release candidate"),
    (D(2027, 1, 10), "M6", "Launch"),
]

WEEKLY = [
    ("W1", "Setup", "Unreal 5.8.4 compiled with our C++; terrain, Lyari and Delhi data imported; Unreal MCP live.",
     "First lit street in engine"),
    ("W2", "Systems", "Vitals, disease curve, money ledger, hidden suspicion, notebook map, story calendar; Karachi blockout.",
     "One in-game day runs end to end"),
    ("W3", "Systems + world", "Falls and rail heights, ballistics, katta misfire, hand-to-hand; Karachi dressed; Delhi blockouts.",
     "Systems feature-complete"),
    ("W4", "Act 4", "Act 4 Nobody and Runner phases; principal cast in; shrine, tankers, hawala shops, gambling den.",
     "Act 4 first half playable"),
    ("W5", "Act 4 + Acts 1-2", "Inside, the branch, the Rangers raid; Act 1 collection run and wedding; Delhi locations dressed.",
     "Act 4 playable start to finish"),
    ("W6", "Acts 1-2", "Arrest, Dasna, Khoda Colony, the Nizamuddin begging loop with Mangal; Khanna on the platform.",
     "Part 1 playable"),
    ("W7", "Acts 3, 5, 6", "Farm, goodbye, jetty and tide boat, empty house, hunt, balcony fight, rewind, coda.",
     "All acts playable"),
    ("W8", "Polish", "Performance to 60 fps on RTX 3070; playtests; bug triage; voice and subtitles locked.",
     "Release candidate"),
    ("W9", "Ship", "Rating, legal and sensitivity sign-off, store page, trailer, final build.",
     "Launch on 10 January 2027"),
]

CUTS = ["Side content: tea stall, cricket, Kasur trip", "Act 3 farm and route in as cinematics",
        "Delhi hubs reduced to playable interiors", "Crowds limited to 12 archetypes",
        "Part 2 (Wapsi) as a later release"]

# ------------------------------------------------------------------ geometry (SVG units = px at 96 dpi on A3)
W, H = 1520, 790
LABEL_W, OWNER_W = 300, 0
GX0, GX1 = 330, W - 24
TOP = 112
ROW = 38
DAYS = (END - START).days + 1


def X(day):
    return GX0 + (day - START).days / DAYS * (GX1 - GX0)


def diagram():
    s = [f'<svg viewBox="0 0 {W} {H}" class="dg" xmlns="http://www.w3.org/2000/svg">']
    grid_bottom = TOP + ROW * len(TRACKS) + 6
    # week columns, alternating shading
    for i, (a, b) in enumerate(WEEKS):
        x0, x1 = X(a), X(b + dt.timedelta(days=1))
        if i % 2 == 0:
            s.append(f'<rect x="{x0:.1f}" y="{TOP - 54}" width="{x1 - x0:.1f}" height="{grid_bottom - TOP + 54 + 150}" fill="#f3eee6"/>')
        s.append(f'<text x="{(x0 + x1) / 2:.1f}" y="{TOP - 34}" class="wk">W{i + 1}</text>')
        lab = f"{a.day}–{b.day} {b.strftime('%b')}" if a.month == b.month else f"{a.day} {a.strftime('%b')}–{b.day} {b.strftime('%b')}"
        s.append(f'<text x="{(x0 + x1) / 2:.1f}" y="{TOP - 18}" class="wkd">{lab}</text>')
    # month band
    for m0, name in ((D(2026, 11, 10), "November 2026"), (D(2026, 12, 1), "December 2026"), (D(2027, 1, 1), "January 2027")):
        s.append(f'<text x="{X(m0) + 6:.1f}" y="{TOP - 66}" class="month">{name}</text>')
        s.append(f'<line x1="{X(m0):.1f}" y1="{TOP - 80}" x2="{X(m0):.1f}" y2="{TOP - 60}" class="mrule"/>')
    # track rows
    group_prev = None
    for r, (group, label, owner, ck, spans) in enumerate(TRACKS):
        y = TOP + r * ROW
        if group != group_prev:
            if group_prev is not None:
                s.append(f'<line x1="0" y1="{y - 2}" x2="{GX1}" y2="{y - 2}" class="grule"/>')
            s.append(f'<text x="0" y="{y + 23}" class="grp" fill="{C[ck]}">{group.upper()}</text>')
            group_prev = group
        s.append(f'<text x="86" y="{y + 18}" class="trk">{label}</text>')
        s.append(f'<text x="86" y="{y + 31}" class="own">{owner}</text>')
        for a, b, txt in spans:
            x0, x1 = X(a), X(b + dt.timedelta(days=1)) - 2
            s.append(f'<rect x="{x0:.1f}" y="{y + 7}" width="{x1 - x0:.1f}" height="24" rx="5" fill="{C[ck]}"/>')
            s.append(f'<text x="{x0 + 9:.1f}" y="{y + 23}" class="bar">{txt}</text>')
    # milestones
    my = grid_bottom + 26
    s.append(f'<line x1="{GX0}" y1="{my}" x2="{GX1}" y2="{my}" class="axis"/>')
    s.append(f'<text x="0" y="{my + 5}" class="grp" fill="{INK}">MILESTONES</text>')
    for d_, code, name in MILESTONES:
        x = X(d_ + dt.timedelta(days=1)) - 2
        s.append(f'<line x1="{x:.1f}" y1="{TOP - 6}" x2="{x:.1f}" y2="{my - 9}" class="msl"/>')
        s.append(f'<path d="M{x:.1f},{my - 8} l8,8 l-8,8 l-8,-8 z" fill="{INK}"/>')
        anchor = "end" if code in ("M5", "M6") else "middle"
        dx = -10 if code == "M6" else (-4 if code == "M5" else 0)
        s.append(f'<text x="{x + dx:.1f}" y="{my + 26}" class="msc" text-anchor="{anchor}">{code} · {d_.day} {d_.strftime("%b")}</text>')
        s.append(f'<text x="{x + dx:.1f}" y="{my + 40}" class="msn" text-anchor="{anchor}">{name}</text>')
    # act coverage strip
    ay = my + 70
    s.append(f'<text x="0" y="{ay + 15}" class="grp" fill="{INK}">ACTS PLAYABLE</text>')
    acts = [("Act 4", D(2026, 12, 13)), ("Acts 1-2", D(2026, 12, 20)), ("Acts 3, 5, 6", D(2026, 12, 27))]
    for i, (name, d_) in enumerate(acts):
        x0 = X(d_ + dt.timedelta(days=1))
        y0 = ay + i * 19
        s.append(f'<rect x="{x0:.1f}" y="{y0}" width="{GX1 - x0:.1f}" height="15" rx="4" fill="{C["setup"]}"/>')
        s.append(f'<text x="{x0 + 8:.1f}" y="{y0 + 11}" class="actlab">{name}</text>')
    s.append("</svg>")
    return "\n".join(s)


LEGEND = "".join(f'<span><i style="background:{C[k]}"></i>{n}</span>' for k, n in
                 (("setup", "Setup"), ("sys", "Systems"), ("world", "World"), ("cast", "Cast"), ("acts", "Acts"), ("ship", "Ship")))

ROWS = "".join(f'<tr><td class="wkc">{w}</td><td class="foc">{f}</td><td>{dl}</td><td class="ex">{ex}</td></tr>'
               for w, f, dl, ex in WEEKLY)

HTML = f"""<!doctype html><html><head><meta charset="utf-8"><title>Dhurandhar Phase Diagram</title>
<style>
@page {{ size: A3 landscape; margin: 0; }}
* {{ box-sizing: border-box; margin: 0; padding: 0; }}
body {{ font-family: "Segoe UI", "Helvetica Neue", Arial, sans-serif; color: {INK}; background: {PAPER};
  -webkit-print-color-adjust: exact; print-color-adjust: exact; }}
.sheet {{ width: 420mm; height: 297mm; padding: 16mm 18mm 14mm; position: relative; page-break-after: always; background: {PAPER}; }}
.sheet:last-child {{ page-break-after: auto; }}
header {{ display: flex; justify-content: space-between; align-items: flex-end; border-bottom: 2px solid {INK}; padding-bottom: 5mm; }}
.eyebrow {{ font-size: 9pt; letter-spacing: 3.5px; text-transform: uppercase; color: {MUTE}; }}
h1 {{ font-size: 30pt; font-weight: 700; letter-spacing: -0.8px; line-height: 1.05; margin-top: 2mm; }}
.sub {{ font-size: 10.5pt; color: {MUTE}; margin-top: 2mm; }}
.facts {{ display: flex; gap: 12mm; text-align: right; }}
.facts div {{ font-size: 8pt; color: {MUTE}; letter-spacing: 1px; text-transform: uppercase; }}
.facts b {{ display: block; font-size: 15pt; color: {INK}; letter-spacing: 0; text-transform: none; font-weight: 600; }}
.dg {{ width: 100%; margin-top: 8mm; }}
.dg .wk {{ font-size: 12px; font-weight: 700; text-anchor: middle; fill: {INK}; }}
.dg .wkd {{ font-size: 9.5px; text-anchor: middle; fill: {MUTE}; }}
.dg .month {{ font-size: 11px; font-weight: 600; letter-spacing: 1.5px; text-transform: uppercase; fill: {INK}; }}
.dg .mrule {{ stroke: {INK}; stroke-width: 1.5; }}
.dg .grp {{ font-size: 9.5px; font-weight: 700; letter-spacing: 2px; }}
.dg .trk {{ font-size: 12.5px; font-weight: 600; fill: {INK}; }}
.dg .own {{ font-size: 9.5px; fill: {MUTE}; }}
.dg .bar {{ font-size: 10.5px; font-weight: 600; fill: #fff; }}
.dg .grule {{ stroke: {RULE}; stroke-width: 1; }}
.dg .axis {{ stroke: {INK}; stroke-width: 1.5; }}
.dg .msl {{ stroke: {INK}; stroke-width: 0.8; stroke-dasharray: 3 4; opacity: .45; }}
.dg .msc {{ font-size: 11px; font-weight: 700; fill: {INK}; }}
.dg .msn {{ font-size: 10px; fill: {MUTE}; }}
.dg .actlab {{ font-size: 10.5px; font-weight: 600; fill: #fff; }}
footer {{ position: absolute; left: 18mm; right: 18mm; bottom: 12mm; display: grid; grid-template-columns: 1.1fr 1.4fr 1fr; gap: 12mm;
  border-top: 1px solid {RULE}; padding-top: 5mm; font-size: 9pt; color: #3d362f; line-height: 1.55; }}
footer h4 {{ font-size: 8pt; letter-spacing: 2.5px; text-transform: uppercase; color: {MUTE}; margin-bottom: 2mm; font-weight: 600; }}
.legend {{ display: flex; flex-wrap: wrap; gap: 4mm 7mm; }}
.legend span {{ display: flex; align-items: center; gap: 2mm; }}
.legend i {{ width: 10px; height: 10px; border-radius: 3px; display: inline-block; }}
ol {{ padding-left: 5mm; }}
table {{ width: 100%; border-collapse: collapse; margin-top: 9mm; font-size: 10.5pt; }}
th {{ text-align: left; font-size: 8pt; letter-spacing: 2.5px; text-transform: uppercase; color: {MUTE}; font-weight: 600;
  padding: 0 4mm 3mm 0; border-bottom: 2px solid {INK}; }}
td {{ padding: 4.2mm 4mm 4.2mm 0; border-bottom: 1px solid {RULE}; vertical-align: top; line-height: 1.5; }}
td.wkc {{ font-weight: 700; width: 16mm; font-size: 13pt; }}
td.foc {{ width: 48mm; font-weight: 600; }}
td.ex {{ width: 80mm; font-weight: 600; color: {C['setup']}; }}
.note {{ margin-top: 8mm; font-size: 9.5pt; color: {MUTE}; line-height: 1.6; max-width: 300mm; }}
</style></head><body>

<section class="sheet">
  <header>
    <div><div class="eyebrow">Production phase diagram · working title</div>
      <h1>DHURANDHAR: THE GAME</h1>
      <div class="sub">Part 1 · Katora&nbsp;&nbsp;/&nbsp;&nbsp;Part 2 · Wapsi&nbsp;&nbsp;·&nbsp;&nbsp;six acts, Unreal Engine 5.8</div></div>
    <div class="facts"><div><b>10 Nov 2026</b>Start</div><div><b>10 Jan 2027</b>Launch</div><div><b>9 weeks</b>Duration</div>
      <div><b>Tarun · Rayan · Claude</b>Team</div></div>
  </header>
  {diagram()}
  <footer>
    <div><h4>Legend</h4><div class="legend">{LEGEND}</div>
      <p style="margin-top:3mm">Bars run Monday to Sunday. Diamonds are Sunday reviews with a tagged build.
      Tracks run in parallel; owners are accountable, not exclusive.</p></div>
    <div><h4>How nine weeks works</h4>
      <p>World geometry is generated from real maps, so the team spends its time on systems and story.
      Systems land first because the story is told through them. Karachi (half the game) is built before Delhi.
      Every Sunday ends with a playable build.</p></div>
    <div><h4>Cut line, in order, if a milestone slips</h4><ol>{"".join(f"<li>{c}</li>" for c in CUTS)}</ol></div>
  </footer>
</section>

<section class="sheet">
  <header>
    <div><div class="eyebrow">Week by week</div><h1>Deliverables and exit criteria</h1>
      <div class="sub">A week is done only when its exit criterion is met in a playable build.</div></div>
    <div class="facts"><div><b>6</b>Milestones</div><div><b>13</b>Tracks</div><div><b>25</b>Story beats</div></div>
  </header>
  <table><tr><th>Week</th><th>Focus</th><th>Deliverables</th><th>Exit criterion</th></tr>{ROWS}</table>
  <p class="note">Assumptions: two full-time developers directing Claude as the main implementer; voice, motion capture and legal
    review contracted in weeks 4 to 9; Unreal Engine 5.8.4 on an RTX 3070. Scope is the full storyline bible.
    If two consecutive milestones slip, apply the cut line from the top.</p>
</section>
</body></html>"""

(HERE / "phase_diagram.html").write_text(HTML, encoding="utf-8")
print("wrote", HERE / "phase_diagram.html")
