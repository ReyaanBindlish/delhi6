"""Single source of truth for the principal cast of DHURANDHAR: THE GAME.

Run:  C:\\Users\\Rayaan\\Desktop\\game\\characters\\.venv\\Scripts\\python.exe build_cast_data.py

Writes
  OldDelhi/Data/DT_Cast.csv           DataTable source (row struct FCastMemberRow)
  characters/cast/cast_<id>.json      per-character recipe: look, body states, MetaHuman Creator parameters,
                                      UE mannequin fallback; read by OldDelhi/Tools/ue/setup_characters.py

Numbers are authored, not measured: skin U/V and body measurements are starting values to be judged by eye in
MetaHuman Creator (see docs/CHARACTERS.md). Heights and weights follow the storyline bible; where the bible is
silent they are chosen to be plausible for the age, trade and region of the character.
"""
import csv
import json
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
CSV_OUT = REPO / "OldDelhi" / "Data" / "DT_Cast.csv"

# Mannequin reference (SKM_Manny / SKM_Quinn): about 180 cm, reads as BMI 22.
MANNY_H, MANNY_BMI = 180.0, 22.0


def bmi(h, w):
    return w / ((h / 100.0) ** 2)


def measurements(h, w, sex):
    """Starting circumferences in cm from height, weight and sex (rough anthropometric fit)."""
    g = math.sqrt(bmi(h, w) / 22.0)
    if sex == "f":
        return {"height": h, "chest": round(0.50 * h * g, 1), "waist": round(0.40 * h * g ** 1.3, 1),
                "hips": round(0.55 * h * g, 1)}
    if sex == "c":  # child
        return {"height": h, "chest": round(0.46 * h * g, 1), "waist": round(0.41 * h * g, 1),
                "hips": round(0.46 * h * g, 1)}
    return {"height": h, "chest": round(0.52 * h * g, 1), "waist": round(0.44 * h * g ** 1.3, 1),
            "hips": round(0.52 * h * g ** 0.9, 1)}


def fallback_scale(h, w):
    g = max(0.7, min(1.35, math.sqrt(bmi(h, w) / MANNY_BMI)))
    return {"x": round(g, 3), "y": round(g, 3), "z": round(h / MANNY_H, 3)}


# ----------------------------------------------------------------------------------------------------------
CAST = []


def add(**kw):
    CAST.append(kw)


add(
    id="Daya", display="Dayanand 'Daya' Dhillon", role="Protagonist; the only playable character", faction="Dhillon family / RAW asset",
    age=38, height=178, weight=78, build="Wiry, narrow shoulders, long forearms; Act 1 fed and fit; changes by act (see states)",
    sex="m", capsule=(89, 34, 166, 45), gait="Lean", walk=150, run=470, playable=True, has_states=True,
    skin="Medium wheatish Punjabi; sun-weathered forearms and neck; pallor rises with disease",
    hair="Black, cropped short (cut hair, no turban), faint early grey at the temples; Act 2 matted and overgrown; Act 5-6 thinning",
    beard="Act 1 trimmed stubble; Act 2 three-week unkempt beard; Act 3 clean-shaven; Act 4 short full beard (legend); Act 5-6 sparse and patchy",
    outfit_summer="Half-sleeve checked shirt over a white vest; dark jeans; leather sandals",
    outfit_monsoon="Quick-dry polyester shirt; dark trousers; rubber chappals",
    outfit_winter="Cheap black faux-leather jacket over a sweatshirt; jeans; canvas sports shoes; wool muffler",
    outfit_by_act="A1=above (collection-run gangster: gold-plated chain removed); A2=faded kurta and loose pyjama in rags or one donated blanket and a plastic sheet in rain; A3=plain cotton shirt and trousers then tracksuit; A4=muted shalwar kameez and waistcoat (Karachi) with a topi only for Friday prayers in the legend; A5=the same kurta hanging loose and sweat-stained; A6=oversized hooded sweatshirt and cardigan that no longer fit",
    accessories="Steel kada on the right wrist; a steel katora (begging bowl) and cloth bag in Act 2; cheap digital watch; no phone from Act 2",
    gait_notes="Act 1: loose confident stride, slight forward lean, hands in pockets. Act 2: shortened stride, rounded shoulders, avoids eye contact, rests on one hip while standing. Act 4: deliberate neutral Karachi walk that he consciously holds. Act 5-6: hunched 9 degrees, slow, hand pressed to ribs, rests after stairs; a cough fit bends him double (input lock 2-4 s).",
    tells="Cracks his LEFT thumb when he lies (UCastTellComponent 'ThumbCrack'); counts everything under his breath (rupees, steps, chapatis); says little",
    voice="Low, dry, economical; Lahori Punjabi with a Kasur accent, functional Urdu; breath gets shorter as the disease progresses",
    languages="Pa-Lahori=60;Ur=20;Hi=20", acts="1;2;3;4;5;6",
    mh_assets="MH_Daya_A1|MH_Daya_A2|MH_Daya_A6", fallback="SKM_Manny",
    sens="Born in Mamdot; grandparents from Kasur. Faith practice is kept low-key and respectful: kada, no caricature; the disease is shown as a body, not a spectacle.",
    skin_uv=(0.46, 0.42),
)
add(
    id="Sukhi", display="Sukhwinder 'Sukhi' Dhillon", role="Daya's wife", faction="Dhillon family",
    age=35, height=163, weight=58, build="Medium, strong forearms from kneading and lifting tiffin carriers", sex="f",
    capsule=(81, 32, 152, 45), gait="Standard", walk=140, run=380, playable=False, has_states=False,
    skin="Medium wheatish; flushed cheeks from the kitchen heat; faint flour dust on forearms in Act 1-2",
    hair="Long black hair in one oiled braid; a few early silver strands; chunni drawn over head at the gurdwara",
    beard="None",
    outfit_summer="Light cotton salwar kameez in muted colours; thin dupatta",
    outfit_monsoon="Cotton salwar kameez; dupatta pinned; plastic slippers",
    outfit_winter="Salwar kameez with a hand-knitted cardigan and a shawl",
    outfit_by_act="A1=printed everyday salwar kameez with an apron over it at work; A2=the same suits, mended and worn; A5=the last day: a plain suit she would wear to the gurdwara",
    accessories="Thin gold earrings; steel or gold kada; bangles; steel tiffin carriers and a cloth bundle; no heavy makeup",
    gait_notes="Quick, practical walk carrying weight on the hip or head; stops to check the boy; stands with arms folded when worried",
    tells="Taps the rim of a steel tiffin lid when she is angry and not saying it",
    voice="Warm, direct, quick Punjabi; unsentimental; lowers her voice rather than raises it",
    languages="Pa=70;Hi=25;En=5", acts="1;2;5",
    mh_assets="MH_Sukhi", fallback="SKM_Quinn",
    sens="A working mother and small-business owner, shown with competence. Her death in Act 5 is never used as spectacle.",
    skin_uv=(0.48, 0.45),
)
add(
    id="Gurnoor", display="Gurnoor Dhillon", role="Daya's son", faction="Dhillon family",
    age=9, height=132, weight=28, build="Slight, quick, scabbed knees", sex="c",
    capsule=(66, 24, 122, 30), gait="Child", walk=120, run=360, playable=False, has_states=False,
    skin="Medium wheatish, a shade lighter than his father; sunburnt nose",
    hair="Black, uncut, kept in a top-knot (joora) under a small patka outdoors",
    beard="None",
    outfit_summer="Cotton T-shirt and shorts; rubber chappals; patka in a school or festival colour",
    outfit_monsoon="Plastic raincoat or a school jacket; shorts; sandals",
    outfit_winter="Hand-knit sweater and a woollen cap over the patka; track pants; canvas shoes",
    outfit_by_act="A1 and A2=the same cricket-and-school wardrobe, a size bigger in Act 2 (hand-me-downs); A5=school uniform",
    accessories="Plastic cricket bat and a rubber ball; school bag; a thin steel kada",
    gait_notes="Runs everywhere; bat swung over the shoulder; scuffs his feet; shy stop-and-stare when his father comes home",
    tells="Counts runs out loud like his father counts rupees",
    voice="Child Punjabi with a Ghaziabad Hindi edge; excitable, asks questions",
    languages="Pa=50;Hi=45;En=5", acts="1;2;5",
    mh_assets="MH_Gurnoor", fallback="SKM_Manny",
    sens="A child; never shown in a threatening or exploitative frame; the Act 5 scene is staged off screen.",
    skin_uv=(0.40, 0.50),
)
add(
    id="Bibi", display="Bibi (Daya's grandmother)", role="Daya's grandmother", faction="Dhillon family",
    age=84, height=151, weight=48, build="Small, frail, stooped; strong hands", sex="f",
    capsule=(73, 30, 138, 25), gait="ElderlyStooped", walk=70, run=130, playable=False, has_states=False,
    skin="Fair-wheatish, deeply lined; age spots on the hands; slightly cloudy left eye",
    hair="White, braided and pinned beneath the chunni",
    beard="None",
    outfit_summer="White cotton salwar kameez; white chunni over the head",
    outfit_monsoon="White cotton suit; plastic sandals; a cotton shawl",
    outfit_winter="White suit; thick woollen shawl; woollen socks; a hand-knit cardigan",
    outfit_by_act="Same wardrobe in every act; she wears white because she is a widow and she does not change it",
    accessories="Wooden walking stick; thick-framed glasses on a cord; a small prayer book in a cloth; a steel kada; a transistor radio",
    gait_notes="Short shuffling steps, one hand on the stick and the other on a wall or shoulder; sits down in stages; stands up with a grunt and a joke",
    tells="Calls the boy by his grandfather's name when tired; hums a Kasur lullaby",
    voice="Cracked, patient old Lahori Punjabi; the source of Daya's accent; sings when she thinks nobody listens",
    languages="Pa-Lahori=85;Ur=10;Hi=5", acts="1;2;5",
    mh_assets="MH_Bibi", fallback="SKM_Quinn",
    sens="Respected elder; head covering is correct and consistent; never played for pity.",
    skin_uv=(0.38, 0.60),
)
add(
    id="Bablu", display="Bablu Tyagi", role="Daya's former partner; the man on the balcony", faction="Ghaziabad gang",
    age=39, height=181, weight=98, build="Big, heavy-shouldered, soft belly over strong legs; 20 kg heavier than Daya's Act 1 weight", sex="m",
    capsule=(90, 38, 169, 45), gait="Heavy", walk=140, run=330, playable=False, has_states=False,
    skin="Fair wheatish, flushed; heavy jaw; shaving rash",
    hair="Thick black hair gelled back; thin patch at the crown",
    beard="Heavy moustache with a closely trimmed beard",
    outfit_summer="Branded half-sleeve shirt, unbuttoned one too many; formal trousers; leather slip-ons",
    outfit_monsoon="Dark shirt; cargo pants; rubber-soled shoes",
    outfit_winter="Quilted puffer jacket with a fictional logo; wool cap",
    outfit_by_act="A1=flashy, new money; A6=the same style, older and a bit too tight; he lives above a shop in a fourth-floor flat",
    accessories="Heavy gold chain; gold rings on three fingers; a smartphone; a fictional-brand watch; an SUV key fob (fictional)",
    gait_notes="Wide stance; chest first; sits with his legs apart; breathes through his mouth after one flight of stairs but wins a clinch because of mass",
    tells="Never looks at the person he is lying to; touches the chain when he decides",
    voice="Loud, jovial, Western UP Hindi, Haryanvi edges; threats are delivered softly",
    languages="Hi=85;Pa=10;En=5", acts="1;6",
    mh_assets="MH_Bablu", fallback="SKM_Manny",
    sens="A criminal, written as a person: friendly, afraid, defensive. No stereotype of a region or caste.",
    skin_uv=(0.55, 0.38),
)
add(
    id="Mangal", display="Mangal", role="Beggar at Nizamuddin; Daya's teacher of the platform economy", faction="Nizamuddin station and dargah",
    age=55, height=168, weight=62, build="Powerful chest, shoulders and forearms; bilateral above-knee amputee (about 18 cm of residual thigh); lower legs absent", sex="m",
    capsule=(52, 34, 98, 6), gait="WheeledBoard", walk=100, run=160, playable=False, has_states=False,
    skin="Dark wheatish, weathered; calloused palms",
    hair="Grey-black, short, combed",
    beard="Grey stubble; neat moustache",
    outfit_summer="Faded clean cotton kurta; the cut ends of his pyjama folded and secured with neat cloth wraps; a gamchha",
    outfit_monsoon="Plastic sheet over the shoulders; kurta; waterproof board cover",
    outfit_winter="Donated wool sweater; thick blanket tucked around the board; wool cap",
    outfit_by_act="A2=the Nizamuddin platform; the Coda=the same clothes, a better board, and a Rs 500 note",
    accessories="A low wooden board about 90 x 45 cm on four ball-bearing caster wheels; two wooden hand blocks (pushers) with rubber grips; a tin cup; a transistor; a laminated medical card",
    gait_notes="Moves seated on the board; pushes with the hand blocks, glides, pivots on a single hand; cannot climb stairs or curbs over 6 cm without help; speed about 1.0-1.4 m/s; stops at the same spots every day; sits upright with dignity, not slumped",
    tells="Counts the day's coins into his lap twice; always names the exact figure",
    voice="Dry, funny, exact; a lifetime of Delhi Hindi/Urdu; says 'fifty more than yesterday' like a prayer",
    languages="Hi=70;Ur=25;Pa=5", acts="2;Coda",
    mh_assets="MH_Mangal", fallback="SKM_Manny",
    sens="Depict with dignity: a working man with expertise, humour and agency. No pity framing, no leering camera. The board is maintained and personal. Lower-limb absence is a mesh/animation setup, not a visual gag.",
    skin_uv=(0.58, 0.28),
)
add(
    id="Khanna", display="Devraj Khanna ('Saab')", role="National Security Adviser (fictional)", faction="Government of India",
    age=72, height=175, weight=68, build="Spare, upright, narrow hands", sex="m",
    capsule=(87, 33, 163, 45), gait="Elderly", walk=110, run=240, playable=False, has_states=False,
    skin="Fair wheatish; fine lines; slight tan from the garden",
    hair="Silver, short, neatly combed",
    beard="Clean-shaven",
    outfit_summer="White cotton shirt, light trousers; soft shoes",
    outfit_monsoon="Same with a dark umbrella and a rain-darkened hem",
    outfit_winter="Charcoal or grey bandhgala (Nehru) jacket over a kurta; grey shawl",
    outfit_by_act="A3=station scene in a plain kurta and shawl (no uniform, no flag); A5-6=hospital and farm scenes in a grey sweater; Coda=the same bandhgala",
    accessories="Thin steel-framed glasses; an old wristwatch with a leather strap; a cloth handkerchief; a small notebook",
    gait_notes="Unhurried, upright, hands behind his back or folded; a slight hip stiffness on stairs; never rushes",
    tells="Remembers and repeats names; pauses before the second sentence; when tired a Lahori vowel slips (the seven years)",
    voice="Soft, never raised; unhurried; precise Hindi and Urdu, formal English, Lahori Punjabi he does not use in public",
    languages="Hi=45;Ur=25;En=20;Pa-Lahori=10", acts="3;5;6;Coda",
    mh_assets="MH_Khanna", fallback="SKM_Manny",
    sens="Fictional figure in the NSA mould, not any real official; no resemblance to a real face.",
    skin_uv=(0.35, 0.62),
)
add(
    id="Ravi", display="Ravindra 'Ravi' Saxena", role="Daya's RAW handler", faction="RAW",
    age=45, height=182, weight=88, build="Ex-army frame gone soft around the middle; powerful forearms", sex="m",
    capsule=(91, 36, 170, 35), gait="KneeLimp", walk=115, run=250, playable=False, has_states=False,
    skin="Wheatish; red cheeks and eye whites from drink; deep crow's feet",
    hair="Short army cut, salt and pepper",
    beard="Salt-and-pepper stubble, day-old",
    outfit_summer="Linen shirt with sleeves rolled; khaki chinos; leather loafers (Dubai) or kurta and loose trousers (Karachi)",
    outfit_monsoon="Dark shirt; cotton trousers; a light rain jacket",
    outfit_winter="Tweed or corduroy jacket; wool scarf",
    outfit_by_act="A3-A4=Dubai: pressed shirts, always a collar; A5=Karachi: stained shalwar kameez and a knee brace under it; the last day: the same, plus the lanyard of a fake fishing licence",
    accessories="Steel-and-neoprene knee brace on the RIGHT knee; a hip flask (fictional label); worn Casio-style watch; a battered notebook; reading glasses",
    gait_notes="Right knee wrecked: weight on the left, right leg swung slightly out and locked; stiff and cautious on stairs and slopes; sits whenever he can; the limp is permanent and slows him to 75% of normal (pairs with the injury rules of the vitals component)",
    tells="Taps two fingers on the brace when he does not want to say something; always checks exits",
    voice="Gravel; dry Delhi Hindi; the only person who calls Daya by his real name; softer when sober",
    languages="Hi=70;En=20;Pa=5;Ur=5", acts="3;4;5",
    mh_assets="MH_Ravi", fallback="SKM_Manny",
    sens="His drinking is a wound, not a joke; his death in Act 5 is the emotional climax and is not staged for shock.",
    skin_uv=(0.52, 0.42),
)
add(
    id="Qureshi", display="Major Faisal Qureshi", role="ISI counter-intelligence, Karachi station; the antagonist", faction="ISI (fictional station)",
    age=38, height=180, weight=76, build="Lean, athletic, upright; the build of a man who runs at dawn", sex="m",
    capsule=(90, 34, 168, 45), gait="Standard", walk=145, run=420, playable=False, has_states=False,
    skin="Fair-wheatish; clean complexion",
    hair="Short, neat, dark with a side parting",
    beard="Trimmed short beard and moustache, always groomed",
    outfit_summer="Pressed light-cotton shalwar kameez in white or ecru, or an open-collar shirt and chinos; polished shoes",
    outfit_monsoon="Dark shalwar kameez; a folded umbrella",
    outfit_winter="Wool waistcoat over a kurta; grey shawl; leather shoes",
    outfit_by_act="Same wardrobe across Act 4: he does not dress like an officer; the uniform appears only once",
    accessories="A slim metal watch; reading glasses; a plain fountain pen; a worn paperback; prayer beads in the pocket (he prays; this is private, never a prop)",
    gait_notes="Calm, measured, hands clasped behind his back; stops mid-stride to listen; turns the whole body toward a speaker",
    tells="Never raises his voice; waits eight seconds for an answer; stirs tea in complete circles",
    voice="Polite, precise Urdu with a Lahori cadence; excellent English; patient to the point of unease",
    languages="Ur=60;En=20;Pa-Lahori=20", acts="4;Coda",
    mh_assets="MH_Qureshi", fallback="SKM_Manny",
    sens="An intelligent professional, not a cartoon villain; his faith is shown with respect and is never the reason he is the antagonist; Karachi and Lahore are rendered as real places.",
    skin_uv=(0.38, 0.55),
)
add(
    id="Sattar", display="Haji Sattar", role="Boss of the Sattar network, Gulbahar Lines, Karachi", faction="Sattar network",
    age=64, height=168, weight=92, build="Heavy, round-shouldered, diabetic: swollen ankles, careful steps", sex="m",
    capsule=(84, 38, 157, 25), gait="Waddle", walk=105, run=170, playable=False, has_states=False,
    skin="Medium-brown, mottled; darkened shins; tired eyes",
    hair="White hair under a prayer cap",
    beard="Full white beard, trimmed at the moustache line (the Hajj title is real, not a costume)",
    outfit_summer="White cotton shalwar kameez; white crocheted prayer cap (topi); sandals; trouser hems above the ankle",
    outfit_monsoon="The same; a dark waistcoat; rubber sandals",
    outfit_winter="Cream shalwar kameez; woollen waistcoat; shawl; wool cap over the topi",
    outfit_by_act="A4=the same wardrobe; Friday: a spotless white suit and an attar fragrance; Eid: new clothes",
    accessories="Prayer beads (tasbih); a glucometer and an insulin pen in a leather pouch; a walking stick with a silver head; thick-rimmed glasses; a worn Quran in a velvet cover (never used as a prop)",
    gait_notes="Slow, rolling waddle with the weight on the heels; breath short on stairs; sits down in stages; the stick leans forward; pauses to check blood sugar",
    tells="Counts his tasbih beads when he decides; says 'Insha'Allah' in two different tones: one honest, one a refusal",
    voice="Deep, slow, courteous; Karachi Urdu with Sindhi turns; speaks of his obligations before his orders",
    languages="Ur=70;Sd=20;Pa=5;En=5", acts="4",
    mh_assets="MH_Sattar", fallback="SKM_Manny",
    sens="A devout man and a gang boss; faith is shown as sincere and distinct from the crime; attire follows correct practice; no religious site hosts combat.",
    skin_uv=(0.62, 0.30),
)
add(
    id="Shahzaib", display="Shahzaib", role="Sattar's nephew and enforcer; Daya's closest friend in Karachi", faction="Sattar network",
    age=28, height=175, weight=74, build="Gym-built, broad shoulders, tight waist", sex="m",
    capsule=(87, 34, 163, 45), gait="Standard", walk=160, run=500, playable=False, has_states=False,
    skin="Warm wheatish; gym-tanned forearms",
    hair="Short fade, gelled at the front",
    beard="Short sharp beard line",
    outfit_summer="Black polo; slim jeans; white sneakers; or a crisp shalwar kameez on Friday",
    outfit_monsoon="Black leather jacket; jeans; boots",
    outfit_winter="Dark puffer; jeans; sneakers",
    outfit_by_act="A4=the same through the act; the Rangers raid: a plain shalwar kameez",
    accessories="Sunglasses on the head; a large fictional-brand watch; a phone in a hard case; a gold ring; a motorbike helmet",
    gait_notes="Easy, bouncing athlete's walk; takes the space; quick to touch a shoulder; never still",
    tells="Taps the steering lock or his phone when he waits; laughs with the whole body before something violent",
    voice="Fast, warm, joking Karachi Urdu; switches to English for slang; calls Daya 'Lahori'",
    languages="Ur=65;En=20;Pa=10;Sd=5", acts="4",
    mh_assets="MH_Shahzaib", fallback="SKM_Manny",
    sens="Written as a likeable, ambitious young man whose loyalty is real; no stereotype of Karachi's youth.",
    skin_uv=(0.50, 0.40),
)

# ----------------------------------------------------------------------------------------------------------
# Daya's three body states (the bible: minus 14 kg in Act 2; the brief: minus 29 kg in Acts 5-6).
DAYA_STATES = [
    {"state": "Healthy2019", "acts": "Act 1 (2019); Act 4 Phase 1 reads a few kg lighter", "weight_kg": 78,
     "notes": "Fed, wiry and quick; gangster swagger; clean clothes with flashy details",
     "morphs": {}, "material": {}, "hunch_deg": 0, "gait_scale": 1.0, "frailty": 0.0},
    {"state": "Beggar", "acts": "Act 2 (2019-2022)", "weight_kg": 64,
     "notes": "Minus 14 kg: visible ribs under the kurta, hollow cheeks, sun-darkened neck and forearms, filthy nails, matted hair",
     "morphs": {"DayaBody_Lean14": 1.0, "DayaFace_Hollow": 0.45},
     "material": {"Gauntness": 0.5, "Dirt": 0.55, "Pallor": 0.15}, "hunch_deg": 4, "gait_scale": 0.92, "frailty": 0.3},
    {"state": "Diseased", "acts": "Acts 5-6 (2024-2025)", "weight_kg": 49,
     "notes": "Minus 29 kg: TB wasting: temporal hollows, prominent clavicles, night-sweat sheen, pallor, loose clothes, dull eyes",
     "morphs": {"DayaBody_Lean14": 1.0, "DayaBody_Lean29": 1.0, "DayaFace_Hollow": 1.0},
     "material": {"Gauntness": 1.0, "Dirt": 0.2, "Pallor": 0.7, "Sweat": 0.5}, "hunch_deg": 9, "gait_scale": 0.78, "frailty": 1.0},
]
DAYA_ACT_DELTAS = {"Act1": 0, "Act2": -14, "Act3": -6, "Act4": -4, "Act5": -4, "Act6": -4}
DAYA_DISEASE_MAX_LOSS = 25

MH_BODY_NOTE = ("MetaHuman Creator 5.8 body: use the Parametric Body tool, set the constraint targets listed below "
                "(cm; the tool shows the real constraint list for the model, use the ones it offers), then blend presets "
                "only to taste. Face: blend 2-3 South Asian presets then Sculpt/Move to the notes. Hair/beard from the Groom library.")

FACE_NOTES = {
    "Daya": "Long face, strong nose with a slight bend (broken once), deep-set tired eyes, thick eyebrows, small scar through the left eyebrow, prominent cheekbones; Act 2/6 variants hollow the cheeks and temples",
    "Sukhi": "Oval face, wide-set intelligent eyes, soft jaw, straight nose, a tiny scar on the chin; laugh lines",
    "Gurnoor": "Round face, large dark eyes, a gap between the front teeth, a small mole on the left cheek",
    "Bibi": "Narrow face, high cheekbones, deep wrinkles, a sunken mouth (missing teeth), slightly cloudy left eye",
    "Bablu": "Wide square face, heavy jaw, small eyes, thick neck, a broken nose that healed flat",
    "Mangal": "Lean weathered face, strong brow, deep nasolabial lines, a direct gaze; a healed scar along the right forearm",
    "Khanna": "Long thin face, high forehead, very calm eyes, fine wrinkles; the stillness is the character",
    "Ravi": "Square face, heavy lids, broken veins on the nose, red-rimmed eyes, a deep scar over the left ear",
    "Qureshi": "Angular, symmetrical face, intelligent dark eyes, a direct polite gaze, thin lips",
    "Sattar": "Round heavy face, hooded eyes, white eyebrows, full cheeks, a stern gentleness",
    "Shahzaib": "Strong jaw, bright eyes, white teeth, open expression; a small scar on the chin from a bike fall",
}

HAIR_GROOM = {
    "Daya": {"hair": "Short crop / buzz-cut family, black", "beard": "Stubble, medium; Act 2 long unkempt; Act 4 short full", "eyebrows": "Thick"},
    "Sukhi": {"hair": "Long braid, black with a few silver strands", "beard": "none", "eyebrows": "Natural, arched"},
    "Gurnoor": {"hair": "Top-knot (joora) with a patka cover; no free hair", "beard": "none", "eyebrows": "Natural"},
    "Bibi": {"hair": "White, long, braided and pinned under a chunni", "beard": "none", "eyebrows": "Thin, grey"},
    "Bablu": {"hair": "Thick black, gelled back", "beard": "Heavy moustache plus short beard", "eyebrows": "Heavy"},
    "Mangal": {"hair": "Short, grey-black", "beard": "Grey stubble plus a neat moustache", "eyebrows": "Bushy grey"},
    "Khanna": {"hair": "Short silver, side parted", "beard": "none (clean-shaven)", "eyebrows": "Fine silver"},
    "Ravi": {"hair": "Army cut, salt and pepper", "beard": "Day-old salt-and-pepper stubble", "eyebrows": "Heavy"},
    "Qureshi": {"hair": "Short, dark, side parting", "beard": "Trimmed short beard plus a moustache", "eyebrows": "Defined"},
    "Sattar": {"hair": "White (under the prayer cap)", "beard": "Full white beard", "eyebrows": "White"},
    "Shahzaib": {"hair": "Fade with a gelled front", "beard": "Sharp short beard line", "eyebrows": "Defined"},
}

EYES = {"Daya": "Dark brown, slightly bloodshot at A2/A6", "Sukhi": "Dark brown", "Gurnoor": "Dark brown",
        "Bibi": "Brown, cloudy left", "Bablu": "Brown", "Mangal": "Dark brown", "Khanna": "Brown-grey",
        "Ravi": "Brown, bloodshot", "Qureshi": "Dark brown", "Sattar": "Brown, hooded", "Shahzaib": "Light brown"}


def clean(s):
    return " ".join(str(s).split())


def row_for(c):
    h, w = c["height"], c["weight"]
    hh, rad, eye, step = c["capsule"]
    return {
        "Name": c["id"], "DisplayName": c["display"], "Role": c["role"], "Faction": c["faction"],
        "AgeAtStart": c["age"], "HeightCm": h, "WeightKg": w, "Build": c["build"],
        "CapsuleHalfHeightCm": hh, "CapsuleRadiusCm": rad, "EyeHeightCm": eye, "MaxStepHeightCm": step,
        "WalkSpeedCmS": c["walk"], "RunSpeedCmS": c["run"], "Gait": c["gait"],
        "Skin": c["skin"], "Hair": c["hair"], "Beard": c["beard"],
        "OutfitSummer": c["outfit_summer"], "OutfitMonsoon": c["outfit_monsoon"], "OutfitWinter": c["outfit_winter"],
        "OutfitByAct": c["outfit_by_act"], "Accessories": c["accessories"],
        "GaitNotes": c["gait_notes"], "Tells": c["tells"], "Voice": c["voice"], "Languages": c["languages"],
        "ActsPresent": c["acts"], "MetaHumanAssets": c["mh_assets"], "FallbackMesh": c["fallback"],
        "SensitivityNote": c["sens"], "bPlayable": "True" if c["playable"] else "False",
        "bHasBodyStates": "True" if c["has_states"] else "False",
    }


def recipe_for(c):
    h, w = c["height"], c["weight"]
    sex = c["sex"]
    u, v = c["skin_uv"]
    d = {
        "id": c["id"], "display": c["display"], "age_at_start": c["age"], "height_cm": h, "weight_kg": w, "bmi": round(bmi(h, w), 1),
        "gait": c["gait"], "languages": c["languages"], "voice": c["voice"],
        "metahuman": {
            "assets": c["mh_assets"].split("|"),
            "note": MH_BODY_NOTE,
            "face_notes": FACE_NOTES[c["id"]],
            "skin": {"description": c["skin"], "U": u, "V": v, "roughness": 1.06,
                     "note": "U/V are starting values for the Creator skin-tone picker; judge by eye under neutral light and adjust"},
            "eyes": EYES[c["id"]],
            "groom": HAIR_GROOM[c["id"]],
            "body_measurements_cm_start": measurements(h, w, sex),
            "age_feel": c["age"],
        },
        "fallback_mannequin": {"mesh": c["fallback"], "mesh_scale_xyz": fallback_scale(h, w),
                               "note": "Applied automatically by UBodyStateComponent when the mesh has no morph targets"},
        "outfits": {"summer": c["outfit_summer"], "monsoon": c["outfit_monsoon"], "winter": c["outfit_winter"], "by_act": c["outfit_by_act"]},
        "accessories": c["accessories"], "tells": c["tells"], "gait_notes": c["gait_notes"], "sensitivity": c["sens"],
    }
    if c["id"] == "Daya":
        states = []
        for s in DAYA_STATES:
            st = dict(s)
            st["asset"] = {"Healthy2019": "MH_Daya_A1", "Beggar": "MH_Daya_A2", "Diseased": "MH_Daya_A6"}[s["state"]]
            st["body_measurements_cm_start"] = measurements(h, s["weight_kg"], sex)
            st["bmi"] = round(bmi(h, s["weight_kg"]), 1)
            st["mannequin_scale_xyz"] = fallback_scale(h, s["weight_kg"])
            states.append(st)
        d["body_states"] = states
        d["act_weight_delta_kg"] = DAYA_ACT_DELTAS
        d["disease_max_loss_kg"] = DAYA_DISEASE_MAX_LOSS
        d["weight_model"] = "weight = Act1 weight + act delta - disease loss; Act 6 full disease: 78 - 4 - 25 = 49 kg (minus 29)"
    if c["id"] == "Mangal":
        d["wheeled_board"] = {
            "board_cm": [90, 45, 8], "ground_clearance_cm": 5, "wheels": "4 ball-bearing casters, 7.5 cm diameter",
            "mass_kg": {"board": 18, "rider": 62}, "speed_m_s": [1.0, 1.4], "max_curb_cm": 6,
            "mesh_setup": "Build the full-body MetaHuman, then in Blender cut both legs at about 18 cm below the groin, cap and close, and weight the stumps to thigh_l and thigh_r; hide the shin, foot and ball bones. Prototype alternative: scale calf_l, calf_r, foot_l, foot_r bones to 0.001 in the anim BP (Transform Modify Bone) and put a cloth wrap material on the stump verts.",
            "capsule": "Radius 34 cm, half-height 52 cm; eye height 98 cm; use a Wheeled nav agent profile with step height 6 cm.",
        }
    return d


def main():
    CSV_OUT.parent.mkdir(parents=True, exist_ok=True)
    rows = [row_for(c) for c in CAST]
    fields = list(rows[0].keys())
    with open(CSV_OUT, "w", encoding="utf-8", newline="") as f:
        wr = csv.DictWriter(f, fieldnames=fields, quoting=csv.QUOTE_MINIMAL)
        wr.writeheader()
        for r in rows:
            wr.writerow({k: clean(v) if isinstance(v, str) else v for k, v in r.items()})
    for c in CAST:
        with open(HERE / f"cast_{c['id'].lower()}.json", "w", encoding="utf-8") as f:
            json.dump(recipe_for(c), f, indent=2, ensure_ascii=False)
    # Invariants from the bible and the brief.
    byid = {c["id"]: c for c in CAST}
    assert byid["Bablu"]["weight"] - byid["Daya"]["weight"] == 20
    assert byid["Gurnoor"]["age"] == 9 and byid["Bibi"]["age"] == 84
    assert byid["Daya"]["weight"] + DAYA_ACT_DELTAS["Act2"] == DAYA_STATES[1]["weight_kg"]
    assert byid["Daya"]["weight"] - DAYA_STATES[2]["weight_kg"] == 29
    assert byid["Daya"]["weight"] + DAYA_ACT_DELTAS["Act6"] - DAYA_DISEASE_MAX_LOSS == DAYA_STATES[2]["weight_kg"]
    print(f"wrote {CSV_OUT} ({len(rows)} rows) and {len(CAST)} recipe JSON files")


if __name__ == "__main__":
    main()
