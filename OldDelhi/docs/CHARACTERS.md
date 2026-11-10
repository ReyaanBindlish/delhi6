# Principal cast: characters, body states and MetaHuman recipes

Scope: the eleven principal characters of DHURANDHAR: THE GAME as playable / NPC-ready assets. Missions come later. Crowd NPCs (the 5,000) are not covered here; they use the archetypes in `Data/DT_Archetypes.csv`.

Source of truth: `characters/cast/build_cast_data.py` (run it with the characters venv) writes `Data/DT_Cast.csv` and one `characters/cast/cast_<id>.json` recipe per character. Edit the Python, regenerate, re-run `Tools/ue/setup_characters.py`. Numbers are authored starting values; judge the faces and bodies by eye in MetaHuman Creator.

## 1. What was built

| Piece | File | Purpose |
|---|---|---|
| Row struct, enums | `Source/OldDelhi/Characters/CastTypes.h` | `FCastMemberRow` (DT_Cast), `ECastAct`, `ECastSeason`, `ECastBodyState`, `ECastGait` |
| Cast asset | `Characters/DhurandharCastDataAsset.h/.cpp` | `UDhurandharCastDataAsset`: table link, per-character visuals, outfits, Daya's three anchors, invariant check |
| Body state | `Characters/BodyStateComponent.h/.cpp` | `UBodyStateComponent`: weight by act and disease, morphs, material scalars, hunch/gait values for the anim BP, mesh swap, outfits |
| Tell | `Characters/CastTellComponent.h/.cpp` | `UCastTellComponent`: Daya's left-thumb crack, feeds `USuspicionComponent` |
| Table | `Data/DT_Cast.csv` | 11 rows, columns equal `FCastMemberRow` properties |
| Editor setup | `Tools/ue/setup_characters.py` | Creates DT_Cast, DA_Cast, MetaHuman Character assets (guarded) |

Module dependencies: none beyond `Core, CoreUObject, Engine` (MetaHuman assets are referenced as `UObject` soft pointers). The headers include their neighbours with relative paths (`"../DayaVitalsComponent.h"`), so no extra include path is needed.

## 2. The cast at a glance

Heights and weights are the baseline (first appearance). Capsule, speeds and notes are in `DT_Cast.csv`.

| Id | Name | Age | Height | Weight | BMI | Gait | Acts |
|---|---|---|---|---|---|---|---|
| Daya | Dayanand "Daya" Dhillon | 38 | 178 cm (5 ft 10) | 78 kg (49 kg in Act 6) | 24.6 | Lean | 1-6 |
| Sukhi | Sukhwinder Dhillon | 35 | 163 | 58 | 21.8 | Standard | 1, 2, 5 |
| Gurnoor | Gurnoor Dhillon | 9 | 132 | 28 | 16.1 | Child | 1, 2, 5 |
| Bibi | Daya's grandmother | 84 | 151 | 48 | 21.1 | ElderlyStooped | 1, 2, 5 |
| Bablu | Bablu Tyagi | 39 | 181 | 98 (Daya + 20) | 29.9 | Heavy | 1, 6 |
| Mangal | Mangal | 55 | 168 (seated about 98 on the board) | 62 | 22.0 | WheeledBoard | 2, coda |
| Khanna | Devraj Khanna, "Saab" | 72 | 175 | 68 | 22.2 | Elderly | 3, 5, 6, coda |
| Ravi | Ravindra "Ravi" Saxena | 45 | 182 | 88 | 26.6 | KneeLimp | 3, 4, 5 |
| Qureshi | Major Faisal Qureshi | 38 | 180 | 76 | 23.5 | Standard | 4, coda |
| Sattar | Haji Sattar | 64 | 168 | 92 | 32.6 | Waddle | 4 |
| Shahzaib | Shahzaib | 28 | 175 | 74 | 24.2 | Standard | 4 |

Invariants checked in code (`ValidateCastInvariants`) and in the setup script: Bablu = Daya + 20 kg; Gurnoor 9; Bibi 84; Daya Act 2 = -14 kg; Daya Act 5-6 = -29 kg; Mangal uses the wheeled-board gait.

## 3. Daya's three body states

One face identity, three anchor bodies. `UBodyStateComponent` computes the weight

```
weight = Act 1 weight (DT_Cast, 78) + act delta - disease loss
act delta:   A1 0   A2 -14   A3 -6   A4 -4   A5 -4   A6 -4
disease loss = (BaseMaxStamina - GetMaxStamina()) / (BaseMaxStamina - MinMaxStamina) * 25 kg
               (read live from UDayaVitalsComponent; NotifyDiseaseStage gives a floor if the story sets a stage first)
Act 2: 78 - 14 = 64 kg.   Act 6, full disease: 78 - 4 - 25 = 49 kg (minus 29).
```

| State | Weight / BMI | Look | Morphs (anchor value) | Material scalars | Hunch / gait scale |
|---|---|---|---|---|---|
| Healthy2019 (Act 1) | 78 kg / 24.6 | Fed, wiry, quick; clean flashy clothes; trimmed stubble | none | none | 0 deg / 1.00 |
| Beggar (Act 2) | 64 kg / 20.2 | Ribs under the kurta, hollow cheeks, filthy nails, matted hair, unkempt beard | `DayaBody_Lean14` 1, `DayaFace_Hollow` 0.45 | Gauntness .5, Dirt .55, Pallor .15 | 4 deg / 0.92 |
| Diseased (Acts 5-6) | 49 kg / 15.5 | TB wasting: temporal hollows, clavicles, night-sweat sheen, pallor, oversized clothes | `DayaBody_Lean14` 1, `DayaBody_Lean29` 1, `DayaFace_Hollow` 1 | Gauntness 1, Dirt .2, Pallor .7, Sweat .5 | 9 deg / 0.78 |

Between anchors everything blends linearly by weight, so the loss in Act 4 Phase 2 and 3 appears gradually as max stamina shrinks. Act changes use `SetAct(Act, bSnap=true)` (time skips) or `false` (slow visual change at 0.25 kg per real second).

Anim Blueprint hooks (all `BlueprintPure` on the component): `GetHunchDegrees` (spine lean), `GetGaitSpeedScale` (stride and play rate; the actual movement speed stays with `UDayaVitalsComponent::GetMoveSpeedMultiplier`), `GetFrailty` (breathing amplitude, idle choice), `GetBodyState`. A cough fit from the vitals component fires `OnBodyCough` and plays the optional `CoughMontage`.

Authoring contract for the art: morph names `DayaBody_Lean14`, `DayaBody_Lean29`, `DayaFace_Hollow` on the body mesh; material parameters `Gauntness`, `Dirt`, `Pallor`, `Sweat` (scalar) on the skin materials. Missing names are ignored, so the system degrades to mesh swapping.

Mesh swap: with MetaHuman the three assembled meshes (`MH_Daya_A1`, `_A2`, `_A6`) go into the anchors' `Mesh` fields; the nearest one is swapped in only while Daya has not been rendered for 0.5 s (no visible pop), or after 20 s, or immediately on `SnapToTarget`. Use the combined face-and-body mesh (`CreateCombinedFaceAndBodyMesh`) so a single skeletal mesh component carries it.

Fallback without morphs (UE mannequin): the component scales the mesh component by height and BMI: Z = height / 180, XY = sqrt(BMI / 22) clamped to 0.7 to 1.35. For Daya that is XY 1.06 (Act 1), 0.96 (Act 2), 0.84 (Act 6).

Tell: `UCastTellComponent::CrackThumb(Observers)` plays the left-hand montage and adds `ESuspicionCause::ThumbTell` to every observer within 3.5 m and a 70 degree cone. Call it from the dialogue system when the player lies, never from a prompt.

## 4. Gait, physics and animation per character

| Character | Capsule (r / half-height) | Walk / run | Animation set and physics notes |
|---|---|---|---|
| Daya | 34 / 89 | 150 / 470 cm/s | Lean set. Act 1 loose stride; Act 2 shorter, rounded shoulders, rests on one hip; Act 4 held neutral walk; Act 5-6 hunched, hand to ribs, rests after stairs. Cough fit locks input 2-4 s (vitals) |
| Sukhi | 32 / 81 | 140 / 380 | Practical walk carrying weight on the hip or head; arms folded when worried |
| Gurnoor | 24 / 66 | 120 / 360 | Child set; runs everywhere; bat over the shoulder |
| Bibi | 30 / 73 | 70 / 130 | Stooped elder set: short shuffle, stick in one hand, one hand on a wall; sits and stands in stages; step height 25 cm |
| Bablu | 38 / 90 | 140 / 330 | Heavy set: wide stance, mouth-breathing on stairs; mass 98 kg: wins a clinch unless the player breaks it in the first seconds (bible) |
| Mangal | 34 / 52 (seated) | 100 / 160 | Wheeled-board set (below) |
| Khanna | 33 / 87 | 110 / 240 | Unhurried, hands behind back, stiff on stairs |
| Ravi | 36 / 91 | 115 / 250 | Right knee wrecked: weight on the left, right leg swung out and locked; speed 75 percent of normal; a permanent injury, consistent with the vitals injury rules |
| Qureshi | 34 / 90 | 145 / 420 | Calm measured walk, stops to listen, turns the whole body |
| Sattar | 38 / 84 | 105 / 170 | Rolling diabetic waddle on the heels; breath short on stairs; checks glucose; step height 25 cm |
| Shahzaib | 34 / 87 | 160 / 500 | Easy bouncing athlete's walk, never still |

### Mangal and the wheeled board (depict with dignity)

Mangal is a working man with expertise, humour and agency, not an object of pity. His board is maintained and personal; the camera never lingers on his body.

- Build: bilateral above-knee amputee (about 18 cm of residual thigh), strong chest, shoulders and forearms, calloused palms.
- Board: 90 x 45 x 8 cm, four 7.5 cm ball-bearing casters, 18 kg; two wooden hand blocks with rubber grips. Rider 62 kg.
- Motion: seated, upright. Push-glide with the hand blocks, single-hand pivot turns. Speed 1.0 to 1.4 m/s. Cannot climb curbs above 6 cm or stairs without help (nav agent "Wheeled": step height 6 cm). Stops at the same spots every day.
- Capsule: radius 34 cm, half-height 52 cm, eye height 98 cm. Drive the board visually from a kinematic mesh attached to the pawn; wheels spin from ground speed. A Chaos rigid-body board is not needed for locomotion and would fight the character movement component.
- Mesh: build the full MetaHuman, then in Blender cut both legs about 18 cm below the groin, cap, and weight to `thigh_l` and `thigh_r`; hide shin, foot and ball bones. Prototype alternative: scale `calf_l`, `calf_r`, `foot_l`, `foot_r` to 0.001 in the anim BP and apply a cloth-wrap material to the stump vertices.
- Animations: sit idle, push, glide, pivot, hand-signal greeting, hold out a cup, count coins into the lap.

## 5. MetaHuman Creator (UE 5.8) recipe

Per character: create a **MetaHuman Character** asset (Content Browser > MetaHuman > MetaHuman Character; `setup_characters.py` does this for all 13 assets). Open it and:

1. **Face.** Blend 2 to 3 South Asian presets in Blend, then Move/Sculpt to the "face notes" in the recipe JSON. Keep the identity recognisable across Daya's three assets: copy `MH_Daya_A1`, then sculpt hollows (cheeks, temples) for A2 (about 45 percent) and A6 (full).
2. **Skin.** Skin tone U/V from the JSON (starting values; set by eye under neutral light), roughness 1.06, texture index per taste. Add freckles/accents only for sun and flush (Bablu, Ravi, Sattar). The setup script writes U/V through `MetaHumanCharacterEditorSubsystem.CommitSkinSettings` when the Python API is exposed.
3. **Eyes, teeth.** Eye colour from the JSON; teeth defaults; Bibi gets an uneven bite.
4. **Hair, beard, brows.** From the Groom library using the descriptors in the JSON (`groom`): hair, beard/moustache, eyebrows. Gurnoor: top-knot only; Sattar: white beard plus the prayer-cap clothing item.
5. **Body.** Parametric Body: set the height and the measurement constraints to `body_measurements_cm_start` (the tool offers its own list; use the ones it shows, in cm). Do not rely on presets for the final look; weight loss for Daya is height-constant and girth-only.
6. **Clothing.** Wardrobe / Outfit assets or the project's own kit, one entry per act and season (see `Outfits` in `DA_Cast`). Pieces share the body skeleton and follow it by leader pose.
7. **Assemble** (Build) to get the skeletal mesh, then assign it to `DA_Cast` (`DayaAnchors[i].Mesh`, `Visuals[<id>].BodyMesh`).

Measurement starting values (cm): Daya A1 178 / chest 97.9 / waist 84.3 / hips 97.4; A2 88.7 / 74.1 / 89.1; A6 77.6 / 62.3 / 79.0. Others are in `cast_<id>.json` (`body_measurements_cm_start`).

What the Python API does not give (checked against the 5.8 headers): face sculpting, parametric body constraints and groom selection are editor-UI operations or C++ only (`FMetaHumanCharacterBodyConstraint` / `EvaluateBodyConstraints` live in the editor module). The script writes the full recipe onto each asset as metadata tags (`CastRecipeFile`, `CastBodyMeasurementsCm`, `CastFaceNotes`, `CastGroom`, `CastBodyState`) so the artist sees it in the editor.

Fallback with the UE mannequins: `SKM_Manny` (men, boy, elders) and `SKM_Quinn` (Sukhi, Bibi). `UBodyStateComponent` scales them by height and BMI at runtime (values in each JSON under `fallback_mannequin`). Outfits are leader-pose clothing meshes on the mannequin skeleton, or colour-coded material slots for blockout. Mangal and Ravi need an anim-BP leg treatment (bone scale for Mangal, locomotion limp for Ravi) in the fallback too.

## 6. Using it in a level

1. Compile the module, then run `UnrealEditor-Cmd.exe OldDelhi.uproject -run=pythonscript -script=Tools/ue/setup_characters.py -unattended -nop4`. Needs the MetaHumanCharacter plugin enabled for the MetaHuman assets, and the UE third-person template content for the mannequins.
2. On the player pawn (Daya): add `UDayaVitalsComponent`, `UBodyStateComponent` (CastData = DA_Cast, CastId = Daya) and `UCastTellComponent`. For any NPC: `UBodyStateComponent` with the matching CastId; non-Daya characters keep their DT_Cast weight and get outfits by act and season.
3. The story drives it: `SetAct(...)` at act transitions; the vitals component's disease stage and months drive the weight; set `bAutoSeasonFromCalendar` to follow `UStoryCalendarSubsystem`.
4. Apply `DT_Cast` capsule and speed values to the pawn at spawn (read the row with `UDhurandharCastDataAsset::GetCastRow`).

## 7. Sensitivity

- Everyone is depicted as a person: competence, fatigue, humour, family. No caricature of region, caste, faith, disability or poverty.
- Religious attire is exact and consistent: Bibi and Sukhi's chunni at the gurdwara; Gurnoor's patka over a top-knot; Sattar's white prayer cap, hems above the ankle, tasbih and a Quran kept respectfully (never a prop); Qureshi's private prayer beads. Religious sites never host combat.
- Mangal: dignity rules above. His mobility aid is his tool.
- Karachi characters (Qureshi, Sattar, Shahzaib) are written as real people; the ISI officer is polite, intelligent, patient; his faith is not why he is the antagonist.
- Khanna is a fictional figure in the NSA mould, with no resemblance to a real official's face.
- Deaths of Sukhi, Gurnoor, Bibi and Ravi are staged without spectacle.
- All brands (watches, jackets, SUV) are fictional.

## 8. Assumptions to confirm

- Daya's age: the bible says 38 at the start but also a birth year of 1988 (31 in 2019). The cast uses 38 (the look); change `age` in the Python if the story wants 31.
- Act 1 weight 78 kg (BMI 24.6) is chosen so that minus 14 and minus 29 stay plausible (64 and 49 kg). Act 3 recovers 8 kg (training, regular meals).
- The Dhillon household is shown as Punjabi Sikh (names, the Mamdot gurdwara pension): Daya has cut hair (consistent with passing as a Pakistani Muslim), Gurnoor a top-knot under a patka. Confirm with the writers.
- Skin U/V, face-sculpt notes and body measurements are starting values, not measured data.
