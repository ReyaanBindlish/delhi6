# 03 - Physics micro-engine: how physics, weather, objects and characters interact

Scope: Old Delhi Phase 1, UE 5.8.4, C++ core, RTX 3070 8 GB, 60 fps (16.6 ms). This doc extends `implementation.md` 4.1/4.3/4.5/8 and the `realism/` docs. It does not repeat their material tables or technique lists; it defines the runtime architecture that ties them together.

Verification legend: [V] verified in sources listed at the end; [A] assumption/engine knowledge to be verified in a spike before it is relied on (each has a Week 1-2 spike in section 9).

---

## 0. Engine facts that shape the design (UE 5.8)

| Fact | Status | Consequence |
|---|---|---|
| Chaos Cloth and Dataflow are Production-Ready in 5.8; cloth authored in the Dataflow Cloth Panel Editor | [V] | Cloth near player is safe to ship. Author tarps/awnings in Dataflow, not legacy APEX-style paint. |
| Control Rig Physics moved to Beta; Control Rig Dynamics (particle-based secondary motion) is Experimental | [V] | Use Control Rig Physics for porter loads / active ragdoll behind a feature flag; Dynamics only for non-critical secondary motion (dupatta ends, tinsel). |
| Mover is Experimental; supports rollback networking via Network Prediction or Chaos Networked Physics. Epic says CMC stays supported | [V] | Do not build the player on Mover for Phase 1. Ship CMC (or a thin custom character component) for the player and NPCs; keep the movement layer behind our own interface (`IDelhiMovementAdapter`) so Mover can be swapped later. |
| Chaos Mover runs on the physics thread at a fixed step, replicates via Network Physics "Resimulation" mode; forum reports crashes when joining sessions | [V] (forum) | Networked physics is not a Phase 1 deliverable. Design for readiness, not for shipping (section 7). |
| Async physics tick / fixed step exists (already chosen in 4.1) | [V] by implementation.md | Keep all force-applying gameplay code on `AsyncPhysicsTick` callbacks. |
| Fog Screen Space Scattering is Experimental (smoke/fog/dust only) | [V] | Optional for dust storm; the primary dust is Niagara sheets + height fog. |
| Significance Manager (`USignificanceManager`, engine module) | [A] long-standing engine feature | Core of tiering. |
| Niagara Data Channels, Neighbor Grid 3D, Fluids plugin | [A] | Data Channels used to push physics events (splashes, scatter) into Niagara without actor coupling. |
| Chaos Visual Debugger (CVD) | [A] shipped as a standalone tool since 5.4-5.5 | Primary physics profiling/replay tool. |

Design principle: **the micro-engine is a small number of C++ authoritative "field" subsystems (Wind, Wetness, Surface, Significance) that everything samples, plus tiered component implementations that read those fields.** No object asks the weather directly; it asks a field at a position. That is what makes the interaction matrix tractable (N consumers x M fields instead of N x M hand couplings).

---

## 1. Simulation tiers and Significance Manager

### 1.1 Tier definitions

Five tiers, assigned per *entity class* by `UDelhiSimTierSubsystem` (a thin policy layer over `USignificanceManager`). Distance is to the nearest local player, measured horizontally (street canyons make 3D distance misleading); a view-cone bonus and "in line of sight to street" bonus bias the score.

| Tier | Name | What runs | Typical radius |
|---|---|---|---|
| T0 | Full | Full Chaos rigid/cloth/skeletal physics, per-frame animation, full IK, contact-rich | 0-12 m (hero zone) |
| T1 | Active | Chaos rigid bodies with simplified collision (single convex/capsule/box), reduced-rate animation, physical animation on hit only, no cloth sim (WPO) | 12-35 m |
| T2 | Kinematic | Moves on rails/splines/Mass; no Chaos body (query collision only); animation at 10-15 Hz via URO/VAT; reacts to events by promotion to T1 | 35-90 m |
| T3 | Statistical | Mass entity data only (position, velocity, intent); rendered as instanced mesh/impostor, or not rendered if outside LOS; no collision | 90-250 m |
| T4 | Frozen / virtual | Data-only (saved state), or HLOD/static baked; zero cost | >250 m or unloaded |

Promotion (T2 -> T1 -> T0) is event-driven and immediate (a bullet, shove, vehicle brush, shout) in addition to distance-driven. Demotion has hysteresis (+20% radius, 1.5 s minimum dwell) to avoid flicker. Promotion budget is capped per frame (section 1.4).

### 1.2 Counts per tier (RTX 3070 budget, slice area 500x500 m, peak bazaar density)

| Class | T0 (0-12 m) | T1 (12-35 m) | T2 (35-90 m) | T3 (90-250 m) |
|---|---|---|---|---|
| Pedestrians | 25 full characters (skeletal mesh, capsule, IK, physical animation available) | 60 (capsule + reduced skeleton LOD, no IK except feet on stairs) | 250 (Mass + VAT/animated instanced, 10-15 Hz anim) | 1,500 (Mass data, instanced low-poly, culled aggressively) |
| Of which ragdoll-capable simultaneously | 6 | 2 (promotion only) | 0 (promote first) | 0 |
| Vehicles | 6 (all ChaosVehicles, full suspension) | 12 (simplified: rigid body with simple-wheel raycast model, no per-wheel solver) | 40 (spline-follow kinematic, pre-baked suspension wobble) | 150 (Mass data, instanced) |
| Cycle/e-rickshaws and autos (included above) | 4 | 8 | 25 | 100 |
| Two-wheelers (included above) | 3 | 6 | 15 | 60 |
| Dynamic props (rigid) | 150 awake max | 250 awake max (mostly sleeping) | 0 (static/ISM) | 0 |
| Props placed (sleeping, collision active) | 600 | 1,500 | static ISM only | none |
| Destructible Geometry Collections (intact) | 20 | 20 | 0 (swap to static mesh) | 0 |
| Active fracture pieces | 300 total across all tiers (matches 4.1), 150 within T0 | | | |
| Cloth (Chaos sim) | 6 pieces (awning/tarp/curtain) at <=15 m; up to 400 verts each | 0 | WPO only | WPO only |
| Wires/cables | 4 sag-sim spans (cable constraints) near player | 20 spans analytic (catenary spring, no solver) | shader WPO | static |
| Birds | 150 AnimToTexture meshes (<=30 m, per 4.4) | | 800 sprites | 1,050 sprites (matches ~2,000 total in 4.4) |
| Large animals (cows, dogs, goats, macaques) | 4 | 8 | 20 (Mass) | 50 |
| Liquids | 1 splash+ripple system, ~3,000 GPU particles; puddle decals 24 | 12 decals | none | none |
| Niagara particle budget (all gameplay-coupled) | 60k GPU particles total, 8k CPU | | | |
| Debris (litter, paper, leaves; wind-driven) | 200 physical bits (CPU Chaos, tiny sphere/box) | 600 Niagara GPU collide-by-depth | 2,000 GPU, no collision | none |

Hard global caps (enforced in `UDelhiSimBudgetSubsystem`): 250 awake rigid bodies, 300 fracture pieces, 8 ragdolls (6 active + 2 settling), 6 simulated cloths, 12 simulated vehicles with full ChaosVehicles solver. When a cap would be exceeded, the lowest-significance member is demoted (ragdoll -> frozen pose with 1 s blend; rigid -> force-sleep then convert to static if outside 12 m; vehicle -> kinematic).

### 1.3 Significance Manager usage

Registration: each simulatable actor/component registers with `USignificanceManager` using a **tag per class** (`Sim.Pedestrian`, `Sim.Vehicle`, `Sim.Prop`, `Sim.Cloth`, ...) and a class-specific significance function and post-function:

```cpp
// One policy object per class; registered in UDelhiSimTierSubsystem::OnWorldBeginPlay
struct FSimClassPolicy {
    FName Tag;
    TFunction<float(USignificanceManager::FManagedObjectInfo*, const FTransform& Viewpoint)> SigFn;
    TFunction<void(USignificanceManager::FManagedObjectInfo*, float Old, float New, bool bFinal)> PostFn;
    FSimTierThresholds Thresholds;   // per-tier significance cut points
    int32 MaxPromotionsPerFrame;
};
```

Significance function inputs (all cheap): horizontal distance, is-in-view-cone (dot product, no traces), LOS-to-street flag cached from `UStreetCanyonField` cell, "interest" bonus (on-screen target of player aim, current mission NPC +1 tier), "recent event" decay (bullet/shove within 3 s forces T0/T1), speed (fast vehicle higher since it is more visible when wrong). Post function applies the tier change by calling `ISimTierable::ApplyTier(ESimTier)` on the component, which enables/disables Chaos bodies, changes anim URO, swaps collision profiles, and toggles cloth/WPO.

Update cadence: `USignificanceManager::Update` is called from `UDelhiSimTierSubsystem::Tick` at **10 Hz**, staggered across classes (pedestrians frame 0, vehicles frame 1, props frame 2, other 3 within a 6-frame cycle) to avoid spikes. Cost target: <0.15 ms/frame amortized for ~2,000 managed objects (T3 Mass entities are NOT registered individually; Mass owns their LOD via its own `LODCollector`/`LODCalculator` processors, and only *promotion candidates* are surfaced to the Significance Manager).

### 1.4 Promotion/demotion budget

- Promote at most 3 pedestrians, 1 vehicle, 8 props per frame; queue the rest (FIFO weighted by significance). T2 -> T1 for a pedestrian costs spawning/enabling a physics asset (~0.2-0.4 ms); amortize across frames and use pooling (section 1.5).
- Never promote and demote the same entity within 1.5 s.
- On teleport/fast-travel/load: temporarily raise promotion cap x4 for 2 s under a loading screen fade or camera cut.

### 1.5 Pooling

Pools for: ragdoll-ready skeletal meshes (16), rigid prop proxies per archetype (crate 40, chair 60, bottle 100, fruit 200), fracture GC instances (10 per archetype), cloth instances (8). Pools avoid runtime body creation hitches; bodies are created at level load and put to sleep with collision disabled.

### 1.6 What is frozen

- Anything outside the player's World Partition range (24,000-25,600 cm main grid; props at 6,000 cm) is not loaded; its *persistent state* lives in `UDelhiPhysicsStateSubsystem` records (section 7.3).
- Sleeping bodies: a body that sleeps for >4 s and is >12 m away and not in view is converted to a static mesh in place (`bWasFrozenToStatic`), re-promoted on any overlap/event. Delhi streets are cluttered; this is the single largest saving.
- Wires/tangles, signboards, shutters in T2+: frozen at last pose; shaders animate them with the wind field.

---

## 2. Characters

### 2.1 Layers (one stack, different weights for NPC vs player)

```
Intent (player input / Mass-AI steering)
  -> UDelhiMovementComponent  (capsule, CMC-derived; custom modes: Wade, Slip, Squeeze, Carry, Stagger)
     -> Surface/traction sampler (UPhysicalSurfaceSampler + UWetnessField)
     -> Crowd contact resolver (UCrowdContactSubsystem)
  -> Animation: Motion Matching (PoseSearch) database selection + trajectory from movement
     -> Foot IK (UFootIKComponent / Control Rig)
     -> Physical Animation layer (UPhysicalAnimationComponent, profile-driven)
     -> Ragdoll blend (URagdollControllerComponent)
```

All physics-affecting decisions are made in the movement component and published as a small struct (`FCharacterPhysState`: SurfaceType, GripCoeff, SlipRisk, Load, StaggerVector, Wade depth, SupportHeight) that animation reads. Animation never feeds back into the capsule except via root motion in explicit scripted states (climb a kerb, sit). This one-way flow keeps motion matching and physics from fighting.

### 2.2 Capsule movement

- Standard capsule radius 34 cm, half-height 90 cm; pedestrians in dense crowd use a *squeezable* capsule: radius shrinks to 24 cm when lateral clearance < 40 cm (turning shoulders sideways, "Squeeze" movement mode) with a 0.25 s blend and a matching motion matching tag `Squeeze`. Porters with loads use a wider *composite* capsule (capsule + load box swept as one shape, section 2.6).
- Step-up height: 30 cm default (Delhi kerbs 15-25 cm, Chandni Chowk steps up to 40 cm to shop plinths). Use `MaxStepHeight` 45 cm with a *step-height-aware speed penalty* so stepping up a 35 cm plinth is a deliberate, slowed action (stumble if sprinting).
- Walkable slope 35 deg, but *surface-dependent slip threshold* computed per frame: `slipAngle = atan(mu_eff)` where `mu_eff` from section 4.3. On wet marble steps at mu_eff 0.3, anything over ~17 deg triggers slip.
- Ground detection: capsule sweep for movement (CMC floor find); a single downward line trace with `bReturnPhysicalMaterial` at 20 Hz (T0) for surface type; cache per tile for NPCs.
- Collision channels: `Pawn`, `Crowd` (new, for NPC-NPC soft contact, no blocking), `Vehicle`, `PhysBody`, `Debris` (ignores pawns), `Projectile`.

### 2.3 Foot IK on uneven kerbs and steps

- Per foot: two traces per foot per frame at T0 (heel and toe sphere sweeps, radius 4 cm, from knee height down 70 cm), at 30 Hz for T1, off for T2. Trace by `ECC_Visibility` against a dedicated `FootStep` channel that excludes props under 10 cm and PCG litter.
- Solve in Control Rig: two-bone IK + pelvis offset = min(left, right) offset with 0.12 s critically damped spring; foot rotation from averaged heel/toe normals, clamped to 25 deg; ankle roll limit 20 deg.
- Stairs and kerbs: don't rely on traces alone; `UStepProfileSubsystem` holds authored step meshes (kerb, ghat steps, shop plinths) as spline-aligned `FStepSegment` so IK targets are exact and motion matching selects `StepUp/StepDown` poses when the next 0.4 s trajectory crosses a segment with height delta > 12 cm. Without this, motion matching walks through steps as flat-ground cycles.
- Slippery foot: if SlipRisk > threshold, IK applies a "foot slide" (foot target lags capsule by up to 8 cm) to sell the skid.
- NPC difference: T1 NPCs use pelvis offset only (no per-foot) and snap foot height from a *precomputed per-tile heightfield* (`FWalkSurfaceGrid`, 25 cm cells) instead of traces. T2 uses nothing.

### 2.4 Physical animation for hits and shoves

- `UPhysicalAnimationComponent` with profiles per body region: `Hit_Light` (strength 800, damping 80, 0.25 s), `Hit_Heavy`, `Shove`, `Stagger`, `Brace`. Blend weight driven by a curve over time, not by instantaneous impulse, so repeated shoves in a bazaar never gives rag-doll jitter.
- Hit pipeline: `FDelhiImpactEvent {Instigator, Location, Impulse, Surface, Energy_J}` from ballistics/vehicle/prop collision -> `UCharacterReactionComponent`. Reaction class chosen by energy and body region:
  - < 15 J: additive anim flinch only (no physics).
  - 15-80 J: physical animation on hit bone chain + pelvis nudge; NPC may stagger (MM `Stagger` tag).
  - 80-400 J or loss of balance flag: partial ragdoll (upper body ragdoll, legs PD-driven) 0.4-1.2 s, then recover to get-up or full ragdoll.
  - > 400 J (vehicle impact, explosion): full ragdoll.
- Pedestrian energy from prop collisions: a plastic chair at 3 m/s (3 kg) is 13 J: flinch; a handcart (80 kg) at 1.5 m/s is 90 J: stagger. Use the same formulas for NPC-vs-player.
- Player difference: physical animation on the player is *capped* (max 60% weight for 0.3 s, no control loss under 80 J) so the camera and aim never get away from the player. NPCs get 100%. Player takes full ragdoll only in scripted/extreme events (vehicle hit > 300 J, fall > 3 m).

### 2.5 Crowd collision and pushing in dense bazaars

The expensive failure mode is hundreds of capsules fighting. Strategy:

- **NPC-NPC**: no physics collision at all in T1+. Mass avoidance (`UMassMovementAvoidance`/ORCA-like) with a personal-space radius and a "density slows speed" fundamental diagram (speed = v0 * (1 - rho/rho_max)^0.8, rho_max about 4.5/m2; Chandni Chowk peak is about 3-4/m2 at festival hours).
- **Player vs NPC contact**: `UCrowdContactSubsystem` runs at 30 Hz on T0/T1 NPCs within 6 m of the player. Overlap checks against a spatial hash (1 m cells, no physics queries). For overlaps it computes a soft push: `F = k * penetration`, with a cap of 120 N-equivalent, applied as (a) NPC steering displacement (they step aside or get irritated per npc-ai doc), and (b) to the player a *resistance factor* reducing max speed (`speedScale = 1 - 0.6*crowdPressure`) and a lateral nudge in the crowd flow direction. The player is never hard-blocked by pedestrians; a "squeeze through" mode (Shift) lets the player slide between with a 0.7 slowdown and a shoulder-turn animation. Hard blocking only for T0 NPCs standing still (shopkeepers, chokepoints) with a 0.3 s "excuse me" push-through voice event.
- **Crowd pressure** is a scalar per 4 m cell = local density x mean relative velocity; feeds: ambient audio, stumble risk (section 2.7), and pickpocket/jostle gameplay.
- **Jostling**: when pressure > 0.6 and the player is in a crowd flow, apply a randomized lateral impulse sequence (0.3-0.8 s period, 20-60 N) -> small camera sway and weapon sway increase; MM picks `Jostle` additive layer.
- **Carts and cycle-rickshaws through crowds**: they are T0/T1 rigid bodies; pedestrian avoidance treats them as large moving obstacles with a velocity-obstacle margin; contact below 1.5 m/s is "gentle push" (pedestrian steps aside, +8 J flinch), above it a real impact event.

### 2.6 Porters and carrying loads

Porters (Khari Baoli, Chawri Bazar: sacks, wooden boxes, cardboard stacks) are a signature Delhi sight.

- Load is a `UCarriedLoadComponent` with: mass (10-80 kg), COM offset (head-load 40 cm up, back-load 10 cm back), extents, dampening, "balance" scalar [0..1].
- Gameplay effects: max speed multiplier `1 - 0.006*mass_kg` (60 kg -> 0.64), acceleration cut x0.5, turn-rate cut, doorway/squeeze clearance uses load extents, step-up limit reduced to 25 cm.
- Visuals: MM database "Carry_Head", "Carry_Back" (mocap tagged) + a spring-damper on the load mesh (2-DOF, driven by capsule acceleration) so the load bobs and lags. Physical animation on spine/neck at low strength (200) with load-mass-based gravity.
- Balance model: `balance += (support_factor - disturbance) * dt`; disturbance from crowd pressure, slope, slip risk, bumps; at balance < 0.2 NPC does a wobble (MM `LoadWobble`), < 0 drops load (load becomes a rigid prop with the sacks' spill physics: sacks 25-50 kg as convex hull; boxes as boxes; individual items spawn from `FLoadSpillTable`).
- T1+: the load is an extra shape on the capsule (no sim); T0: the load is a kinematic body attached to a socket with the spring-damper above, and promoted to a dynamic rigid body only on drop.
- Player carrying: same component with the player having a "heavy carry" input mode (only for specific interactions: sack, water can). Slower, camera bob scaled by mass, stamina drain from survival doc.

### 2.7 Slipping on wet stone

- `GripCoeff = mu_eff` from the wetness/traction pipeline (section 4.3), sampled at the foot contact each 50 ms (T0 only; T1 per-tile).
- Slip risk per step: `risk = saturate((speed*turnRate*0.4 + slope_deg*0.04 + load_kg*0.01) / (mu_eff*2.2))`. Evaluated at foot plant events (anim notify / MM foot-plant tag). If `risk > 0.75` roll against RNG seeded by (entityId, stepIndex) -> deterministic (section 7.1). Outcomes: slide (movement mode `Slip`: friction cut to 30% for 0.4 s, MM `Slip_Recover`), stumble (partial ragdoll legs), fall (full ragdoll, 0.8 s, get-up animation).
- Typical values: dry sandstone mu 0.60; wet sandstone 0.35; wet polished marble (Jama Masjid forecourt, mosque steps) 0.20; wet tile/kota 0.30; asphalt wet 0.50; mud 0.45 with Multiply combine. Moss on drains/stepwells: 0.15 (via MPC `Moss` mask).
- NPC behavior: pedestrians reduce speed on wet stone (v *= clamp(mu_eff/0.6, 0.55, 1)), prefer dry paths (Mass lane cost +), shopkeepers put out "caution" signs/sacks (set-dressing hook), a rare NPC falls (about 1 per 10 min in monsoon near the player) for authenticity and as readable feedback.

### 2.8 Falling and ragdoll blend

States: `Animated -> PhysAnimPartial -> Ragdoll -> GetUp -> Animated`.

- Entry: by `FDelhiImpactEvent` energy (2.4), loss of balance, vehicle contact, death, or explicit scripted trigger.
- Ragdoll physics asset: 15 bodies standard, 10 for T1 (merged). Constraints with realistic limits and a joint-friction/damping profile; fingers/toes excluded; collision with `Pawn`/world but not with other T1+ characters' capsules.
- Blend in: capture velocity from capsule + hit impulse, 0.1 s blend from animation pose to ragdoll (`SetAllBodiesBelowSimulatePhysics` + `BlendPhysics` weight ramp). Blend out: after velocity < 15 cm/s for 0.5 s, check lying orientation (face up/down via pelvis forward vs up) -> choose `GetUp_Front/Back` anim, snapshot pose (`SavePoseSnapshot`) and blend over 0.4 s while capsule teleports to the pelvis projected to ground.
- Sleep: ragdoll sleeps after 4 s of rest; then become static pose (frozen skeletal mesh) if dead and outside 12 m and out of view.
- Budget: 8 ragdolls hard cap (4.1) = 6 active + 2 settling. 9th request converts the oldest *settled* body to frozen pose.
- Player getting up/falling: first-person camera reads from head bone with 0.3 s lag and clamped roll, so no nausea; see controls doc.
- Active ragdoll (Control Rig Physics, Beta) is behind `bUseActiveRagdoll` cvar; falls back to PD-driven physical animation.

### 2.9 Motion matching interaction

- Motion matching (PoseSearch) databases partitioned by *state* (not one huge DB): Locomotion, Crowd-squeeze, Carry, Stumble/Recover, Stairs, Crouch/Cover, Wade. Schemas share channels (pose, trajectory 0.2/0.4/0.8 s, velocity).
- **Physics-aware trajectory**: the trajectory the MM query sees comes from the movement component's *predicted* path after the crowd resolver, not raw input. So when a crowd slows the player, MM picks the slower cycle rather than foot-sliding.
- Tags used as *hard constraints* in the query: `OnStairs`, `Wet` (cautious gait), `Loaded_Head`, `Squeeze`, `Injured_Leg`. The Wet tag selects cautious cycles with shorter strides and lowered COM.
- MM cannot produce hits/falls; those are layers on top: physical animation layer and ragdoll controller blend *after* the MM pose node in the anim graph.
- NPC rate scaling: T0 MM at full rate with 30 Hz database search (every 2nd frame), T1 at 10-15 Hz search with pose interpolation, T2 no MM (VAT/URO pose from a few clips). Use MM Pose History cached; reduce channel set for T1 schema.
- Foot locking (Foot Placement/Foot IK) runs after MM and before physical animation.

### 2.10 NPC vs player summary

| Aspect | Player | NPC T0/T1 | NPC T2+ |
|---|---|---|---|
| Movement | CMC-derived custom, authoritative input | Same component, intent from Mass/AI | Mass fragment, no capsule |
| Physical animation | capped 60%/0.3 s, control retained | full | none |
| Ragdoll | rare, long recovery, camera-safe | per budget | promote first |
| Slip/stumble | deterministic risk, reduced fall chance (x0.5) | full, RNG | none |
| Crowd contact | never hard-blocked | avoids/blocks | n/a |
| Foot IK | full per foot | pelvis only (T1) | none |
| Load carry | explicit | porter profile | statistical |

---

## 3. Object taxonomy (Delhi street objects)

### 3.1 Class model

Each object is a data asset `UDelhiObjectDef` (section 8) selecting a **behaviour archetype**:

| Archetype | Description | Sim |
|---|---|---|
| `Static` | Never moves | query-only/complex-as-simple collision |
| `Rigid` | Single Chaos rigid body | T0/T1 dynamic, sleeps |
| `RigidChain` | Rigid bodies joined by constraints (chains, hinged signboards) | T0 only |
| `Cloth` | Chaos cloth or WPO | T0 sim, else WPO |
| `Rope` | Cable/rope constraint chain or analytic catenary | T0 sim, else analytic |
| `Destructible` | Geometry Collection | T0 only |
| `Kinematic` | Animated/moved by script, pushes physics bodies | any |
| `Particle` | Niagara-only (no gameplay collision) | GPU |
| `Soft` | Deformable look with Rigid proxy + vertex/morph animation (sacks, fruit pile) | fake |

Interaction verbs (bitmask `EInteractVerb`): `Push`, `Pull`, `PickUp`, `Carry`, `Kick`, `KnockOver`, `Open`, `Break`, `Throw`, `Sit`, `Climb`, `Steal`.

### 3.2 Object table

Mass in kg; collision: C=convex hull, B=box, S=sphere/capsule, CC=compound convex, T=trimesh (static only), GC=Geometry Collection. Sleep = threshold linear/angular (cm/s, deg/s) and time to sleep. "Break" = destructibility.

| Object | Mass | Collision | Archetype | Sleep rule | Destructible | Interaction |
|---|---|---|---|---|---|---|
| Handcart (thela, wood + steel wheels) | 90 empty / 250 loaded | CC (box bed + 2 wheel cylinders) | Rigid with wheel constraint (2 hinge) | 5 cm/s, 3 s; never sleeps while loaded AND on slope | Cargo spills at tilt > 40 deg | Push (player 400 N), pull, tip, steal |
| Cycle-rickshaw | 60 (+rider 70+pass 140) | ChaosVehicle (3 wheel) | Vehicle | Parked: sleep after 3 s | Wheels/seat can detach on big hit | Ride, push, knock over |
| Wooden crate (apple/fruit) | 2-6 | B | Rigid | 8 cm/s, 2 s | Break at > 40 J (slats) via GC pool | Pick up, kick, stack, throw |
| Plastic chair (monobloc) | 2.5 | CC (seat + 4 leg capsules) | Rigid | 10 cm/s, 2 s | Crack at 60 J, GC | Kick, sit, throw, drag |
| Plastic stool | 1.2 | CC | Rigid | 10, 2 s | GC | Same |
| Wooden bench (charpai/bench) | 12-25 | B/CC | Rigid | 5, 3 s | Break at 400 J | Push, sit |
| Signboard hanging (painted tin, 1-3 m^2) | 3-15 | B | RigidChain (hinge x2 at top chain) | n/a (moves in wind) | Detach if wind > 25 m/s or impact | Wind swings, shoot off |
| Signboard standing (A-frame/hoarding) | 5-30 | B | Rigid | 5, 3 s | Falls over, folds | Push over, kick |
| Overhead cable bundle (telco/electrical) | 0.5/m | analytic/cable constraint | Rope (T0 sim; else analytic) | Never sleeps (wind) | Cut by bullet/blast: breaks into 2 ropes, sparking VFX | Shoot, swing (not climbable), tangle prop |
| Tarp/plastic sheet (awning) | 1-8 | Cloth + B proxy collision | Cloth | Never (wind) | Tear on impact > threshold: remesh to 2 cloths | Push aside (Move through = drag), pull down |
| Tinsel curtain (Kinari Bazar) | 0.2/strand | Cloth strands (WPO beyond 15 m) | Cloth | Never | n/a | Walk through (physics push) |
| Glass pane (shop window/counter) | 15-40 | B + GC | Destructible | Static until hit | Break at 30 J (thin) / 120 J (display) with 150-300 shards cap total | Shoot, throw into, vault |
| Glass bottle (soda, water, beer) | 0.4-1.0 | C | Rigid | 10 cm/s, 1.5 s | Break at 12 J: 6-12 shards + liquid decal | Pick up, throw, kick |
| PET bottle | 0.03-0.6 | C | Rigid (light; sim as wind-driven when empty) | 15 cm/s, 1 s | No (crushes: swap mesh) | Kick, pick up, crushes underfoot |
| Metal shutter (roller) | 60-150 | Kinematic B + sheet | Kinematic w/ spring | n/a | Dents on hit (material dent mask), force-open | Open/close, bang on, bullet dents; slam sound |
| Water tank (overhead plastic Syntex 500-1000 L) | 25 empty, 500-1000 full | C | Destructible (liquid release) | Static on roof | Burst at > 600 J: water release (Niagara) + wetness injection to surfaces below | Shoot, collapse |
| Water drum/matka (clay pot) | 1-15 | C | Rigid | 8, 2 s | Clay shatters at 25 J, water decal | Carry, pour |
| Gas cylinder (LPG 14 kg) | 30 full | S/C | Rigid | 10, 3 s | Explodes on severe heat (rare, scripted) | Roll, carry |
| Sack (grain/spice 25-50 kg) | 25-50 | C (squashed ellipsoid) | Rigid + Soft look | 5 cm/s, 3 s | Spills (particle), no GC | Carry (porter), drag, throw |
| Cardboard box (carton) | 1-12 | B | Rigid | 8, 2 s | Crush at heavy impact | Stack, kick |
| Fruit (mango, apple, orange, banana bunch) | 0.1-0.25 | S | Rigid, instance-pooled | 10, 1 s | Splat > 20 J (decal) | Pick up, throw, steal |
| Fruit/veg pile (stall display) | 20-80 | Collision proxy hull + ISM visuals | Soft (proxy) | Static | Scatters into 15-40 rigid fruit on hit | Grab, shove stall |
| Newspaper/paper litter | 0.005 | S flat (T1) / Niagara | Particle/Rigid light | 20, 0.5 s | n/a | Wind-blown, step on |
| Plastic bags (kachra) | 0.01 | Niagara + 6 CPU bits near player | Particle | n/a | n/a | Wind-blown, snag on wires |
| Dust, leaves, petals | 0.001 | Niagara depth-collide | Particle | n/a | n/a | Wind-blown |
| Kite (patang) | 0.02 | Niagara/Actor with aerodynamic model | Kinematic-aero | Never while airborne | Cut string: free-fall + drift | Wind-driven, fight (kite season), land on roofs/wires |
| Kite string (manjha) | n/a | Analytic line + 8-segment rope near player | Rope | n/a | Cuts | Player can be cut by glass-coated string (rare event) |
| Cow (T0) | 400 | Capsule (compound) | Character (animal) | n/a | n/a | Push (immovable 80 N/cm), stops traffic |
| Pole (electric/street light) | static | CC | Static | n/a | Rare scripted topple (dust storm) | Collide |
| Parked scooter (static prop) | 90 | CC | Rigid | 5, 3 s | Dents | Push, knock over, steal (scripted) |
| Pothole/uneven slab | n/a | Mesh | Static + height | n/a | n/a | Vehicle jolt, foot IK |
| Manhole cover | 40 | C | Kinematic | n/a | Open in flood (scripted) | Steps |
| Sandbags/stacked brick | 5-10 each | B | Rigid | 5, 2 s | n/a | Pickup, stack |
| Wooden pushcart goods (tava, thali) | 0.3-2 | CC | Rigid | 10, 1 s | n/a | Knock/spill |

### 3.3 Collision profile rules

- Dynamic props use *simple collision only* (box/convex/sphere compound; max 6 primitives; convex < 32 vertices). Trimesh only on static.
- Channels/presets: `Prop_Light` (<5 kg: ignores `Pawn`; pawns pass through and apply "kick" by overlap event for stability), `Prop_Medium` (5-30: blocks Pawn but is movable by physics push), `Prop_Heavy` (>30: blocks Pawn, pushed only by vehicles/`Push` interaction), `Debris` (ignores Pawn, ignores itself in T1).
- Player push: `UPhysicsInteractionComponent` applies a push force capped by mass tier (`F = min(450 N, 15*mass)`), per-frame; objects > 150 kg unmovable by player; NPC pushes use same with cap 300 N.
- Stack stability: Chaos solver iterations raised to 8 position/2 velocity (cvar-tunable), fruit/crates sleeping in stacks within a stall use `bStartAwake=false` and wake on neighbor contact only.
- CCD enabled for projectiles, bottles thrown > 15 m/s, never for others.

### 3.4 Sleep and wake policy

- Default Chaos sleep thresholds are raised for small props (see table) and **forced sleep timers**: any body awake > 8 s without player interaction is force-slept (unless wind-driven or on a moving platform).
- Wake sources: collision with awake body (impulse > threshold), player/NPC `Push`, explosion/bullet, wind gust > prop's `WindWakeSpeed`, `Support removed` (sweep when a supporting body moves - done via a "support graph" per stall: stacked items register their parent; on parent move, children are woken).
- **Awake-body cap** (250) is enforced by demoting oldest least-significant to sleep or static.
- Support graph for stalls: a stall is a `ADelhiStallActor` that owns its children (fruit/veg, cartons) as one `UStallPhysicsGroup`: all children sleep/wake together, and the group is a single significance entity. That cuts 200 fruit to one tier decision.

### 3.5 Destruction rules (adds to 4.1)

- Glass: break patterns by thickness (shop window radial, shatter-proof laminated crazing). Shard cap 300 total, shards live 6 s then fade/settle-to-decal; shards in T1 (12-35 m) spawn as Niagara mesh particles with ground collide.
- Damage model: energy-based `Energy_J = 0.5 * m * v_rel^2 * coupling(material)`. Each object def has `BreakEnergy_J`, `DentEnergy_J`. Threshold events produce `FBreakEvent` -> GC activation -> sound (Wwise/MetaSound by Surface type) -> particles.
- Reuse: destructibles have *two states* (intact static mesh + GC) with GC spawned only on break, from pool; reset on level stream.

---

## 4. Weather <-> physics coupling

### 4.1 Architecture: fields, not couplings

```
UWeatherSubsystem (UWorldSubsystem, 10 Hz sim, 60 Hz interpolated outputs)
 |-- owns FWeatherState (target+current; presets: ClearSummer, WinterSmog, Monsoon, DustStorm, KiteSeason, + transitions)
 |-- writes:
 |     MPC_Weather       (Wetness, PuddleLevel, DustAmount, SmogDensity, WindDir, WindStrength, GustPhase, Temperature) [existing, per 3.7]
 |     MPC_Wind          (wind field texture handle params: origin, cell size, time)
 |     Niagara Data Channels: NDC_Wind (field sampler), NDC_Rain, NDC_Events
 |     Gameplay tags (GameplayTagContainer on world state): Weather.Rain.Heavy, Weather.Dust, Weather.Smog, Weather.Wet, Weather.Gust, Weather.Heat
 |     Audio params (to MetaSound), Post-process (heat haze, smog)
 |
 |-- exposes C++ field queries (thread-safe, read from double-buffered snapshots):
 |     UWindField::Sample(Pos) -> FVector Wind (m/s), turbulence
 |     UWetnessField::Sample(Pos, SurfaceType) -> {Wetness01, WaterDepth_cm, Mu_eff, Moss}
 |     UTemperatureField (heat shimmer / fire)
 |
 |-- Physics consumers (components, sampling at their own rate):
       UWindDrivenComponent (cloth, props, kites, wires, birds)
       UTractionComponent   (characters, vehicles)
       UWaterDepthComponent (wade, vehicle stall)
```

Per frame the `UWeatherSubsystem` does zero per-object work. Per-object work lives in consumers pulling the field at their own rate and tier. One-frame latency is acceptable; fields are double-buffered so async physics reads the last-completed snapshot (no locks).

### 4.2 Wind field

Three layers summed into a **world-aligned 3D grid** (the *street canyon field*):

1. **Global wind** `W_g(t)`: direction and speed from the weather state. Delhi baselines: summer loo 6-12 m/s from W/NW; monsoon 4-9 m/s from E/SE with squalls; winter 0.5-2 m/s (smog stagnation); dust storm aandhi 15-33 m/s (up to ~120 km/h; matches 4.3) with ramp-up over 60-90 s.
2. **Gusts**: pseudo-random scalar via a seeded Perlin/Ornstein-Uhlenbeck process, with characteristic time 2-6 s and amplitude 0.3-0.8 x mean; gust *fronts* travel across the map at 0.6 x W_g speed (a 2D gust phase texture advected by the global wind; gives the visible "wave" through awnings and trees). Per-object phase offsets from position so everything doesn't flap in sync.
3. **Street canyon channelling** (the Delhi-specific part): pre-baked **canyon atlas** from the OSM building footprints + heights (world-design pipeline already has them). Baked offline (Python/PCG or C++ commandlet) into a 2D grid of 8 m cells (about 63 x 63 x several height layers for the 500 m slice; 1 km tile in full): for each cell store (a) a *channel direction* = principal axis of the street (from road centerline segments), (b) a *channel gain* `G` = 1.0-1.6 when the wind direction aligns within 30 deg (Venturi/funnel effect), 0.3-0.6 when perpendicular (sheltered), (c) *shelter factor* from building height / street width aspect ratio (narrow Delhi gallies: aspect 3-6 -> deep sheltering, wind at street level often < 25% of rooftop speed), (d) *corner vortex flag* at junctions. Runtime: `W(pos) = W_g * G(pos, windDir) * Shelter(pos, z) + Gust(pos,t) + VortexNoise(pos,t)`, with height profile `(z/z_ref)^0.25` (urban roughness) and rooftop z >= 12 m unsheltered.
   - Storage: 2 x RGBA8 textures (direction.xy packed, gain, shelter, vortex flag) = 2 x 64 x 64 x 4 B = 32 KB for the slice; scales to Phase 1 full box (2.9 x 2.2 km at 8 m = 360 x 275 cells) = ~400 KB. Also uploaded as an MPC-referenced texture for shader WPO sampling (same data CPU/GPU).
   - Cross-check with explicit CFD: optional offline coarse run (e.g. OpenFOAM/Palabos on a footprint extrusion) to seed the atlas; Week 4+ stretch goal, atlas works without it.
4. **Local effects**: vehicles/bus wake (add gust impulses along track, radius 6 m, 0.8 s), fans/exhaust/AC drips optionally, pigeons flapping, explosions (impulse wave via `NDC_Events`).

Consumers and what they take:

| Consumer | Sample point | Rate | Use |
|---|---|---|---|
| Chaos cloth (awning, tarp, tinsel) | per-cloth centre | 30 Hz | `SetWindVelocity` on cloth config (drag/lift coefficients per asset) |
| Cloth beyond 15 m | WPO in material | GPU | Same field texture sampled with world pos + per-instance phase |
| Overhead wires | per-span | 10 Hz T0 / shader | Lateral force on cable constraint nodes; sway angle `theta ~ atan(F_wind/(m g))` |
| Hanging signboards | per-sign | 20 Hz | torque from projected area x Cd(1.2) x 0.5 rho v^2; exceed hinge limit -> detach |
| Light props (litter, bags) | per-bit | 20 Hz T0, GPU T1/T2 | Drag force `F = 0.5 rho Cd A (v_w - v)^2`; lift for sheets; bags snag on wires/poles (overlap -> constraint) |
| Kites | per-kite | 30 Hz | Aero model: lift L = 0.5 rho v^2 A Cl(alpha); tether tension; cut -> free flight with wind drift |
| Birds | per-flock | 10 Hz | Glide drift, takeoff direction into wind, pigeons stay perched when wind > 12 m/s (shelter in niches) |
| Rain | rain system | per-frame | Rain angle = atan(W_horiz / terminal 6 m/s) clamped 0-60 deg; splash sprites tilt; canyon makes rain angle vary by street (sample field at camera) |
| Smoke/steam/dust | Niagara | GPU | Wind in advection, canyon channel flows |
| Vehicles (high-sided: bus, auto) | CoM | 30 Hz | Lateral gust force `0.5 rho Cd A v^2`; only matters in dust storm > 20 m/s |
| Characters | capsule | 20 Hz | Sway of dupattas/hair/clothing; at > 22 m/s lateral push 20-60 N and stumble risk (in dust storm on open spots like Jama Masjid forecourt) |
| Audio | listener | 10 Hz | Whistle/rumble parameter from local speed |

### 4.3 Rain -> wetness -> friction/traction pipeline

State per world cell (the **wetness field**: 4 m cells, GPU+CPU mirrored, 16-bit):

- `Wet` [0..1] surface moisture; `Pond` [cm] standing water depth; `Moss` [0..1]; `Mud` [0..1]; `Dust` [0..1]; plus `Dry rate` constants.
- Inputs: rainfall rate `R` (mm/h) from weather; roof overhang/**shelter mask** (baked: under chhajjas/awnings/arcades gets 0.05-0.2 of R, with dripping lines at edges adding a runoff term); surface **porosity** and **absorption** per Surface Type (brick/plaster absorb then re-release; stone/asphalt shed); drainage (low points fill, gutter lines drain).
- Update at 2 Hz (CPU) on a coarse grid, GPU texture upload as `T_WetnessField` for materials (puddle masks, roughness, darkening, ripples). Mixing with decals/virtual heightfields for puddles.
- Time constants (tuned for Delhi): sandstone wet-up 20-40 s at 10 mm/h, dry in 20-40 min post rain in summer, 60-120+ min in smog/winter; asphalt 5 s / 15 min; mud accumulates (Mud += dust x water), slush layer at Mud>0.6.

Friction model (applied to both Chaos physical materials and character grip):

```
mu_eff = mu_dry(surface) * lerp(1, wet_ratio(surface), Wet) * (1 - 0.5*Mud) * (1 - Moss*0.7) * (1 + Dust*0.1 [dust grips] - Dust*Wet*0.15 [muddy film])
wet_ratio:   sandstone 0.58, marble 0.33, kota 0.55, asphalt 0.67, brick 0.75, metal 0.45, plastic 0.5, concrete 0.7, mud (already low) 1.0
Pond depth: > 0.5 cm -> hydroplaning factor for vehicles: mu_eff *= 1 - clamp(speed_kmh/80,0,1)*clamp(Pond/3cm,0,1)*0.6
```

Mechanism of applying it:
- **Characters**: sampled directly in the movement component (section 2.7). No physical-material swapping.
- **Vehicles**: ChaosVehicles has tire friction multiplier and `FrictionForceMultiplier` per wheel: set per wheel at 30 Hz in async tick from `UWetnessField::Sample` at the contact patch. A wheel entirely on dry asphalt gets 1.0.
- **Rigid props**: `UPhysicalMaterial` overrides via `UPhysicalMaterialMask`/ per-component `SetPhysMaterialOverride` swapped between `PM_Sandstone_Dry` and `PM_Sandstone_Wet` *classes* (not per-frame lerp: pre-built wet variants at Wet 0, 0.5, 1.0 buckets with hysteresis 0.1). Property: Chaos reads materials at contact creation; frequent swapping is wasteful, so only 3 buckets and swap only for bodies within 20 m. Surface-type-level overrides apply to *static world geometry*: since we cannot change a static mesh's material per tile cheaply, the `UTractionComponent` computes `mu_eff` itself and feeds Chaos by *friction override on the mover* (character/vehicle/wheel) rather than the floor. Rigid props use the floor bucket (sub-case: the 3 variants exist only for the 4 street stone surfaces).
- **Audio/VFX**: footstep and tire sounds use the same `Wet` + `Pond` parameters (splash if Pond>1 cm).

### 4.4 Puddles and waterlogging

- Water depth field `Pond[cm]` from the same grid; fed by rain accumulation minus drainage; chokepoint list (`FFloodSpot`: Fatehpuri chowk, Jain Mandir stretch, rail underpass in Old Delhi): authored basins with capacity curves and drain rates. Chandni Chowk floods ankle-deep (about 15-25 cm) and drains in ~10 min per 4.3; implement as a fill/drain ODE: `dPond/dt = R*catch*(1-shelter) - k_drain*sqrt(Pond)`.
- Visualization: flat water plane per basin is NOT used; instead a *water depth decal/virtual-heightfield blend* in the road material plus Niagara ripples. Depth-dependent: ripple frequency, reflections, foam at kerbs.
- Gameplay thresholds:
  | Depth | Characters | Vehicles |
  |---|---|---|
  | 0-2 cm | Splash VFX, wet feet, slip bonus | Hydroplane risk > 60 km/h |
  | 2-10 cm | Slower 0.9, splash audio, footprint ripples | Water spray VFX; fine |
  | 10-20 cm (ankle) | Slower 0.75 (Wade mode: MM `Wade` DB, no sprint) | Bikes may stall: engine-intake model: `P(stall) = (depth - 12cm)/10cm` per second for scooters; autos 20 cm threshold; cars 35 cm; buses 60 cm |
  | 20-40 cm (shin/knee) | 0.55, cannot sprint, items may float | Cycle-rickshaws slow/stuck; e-rickshaw motor cutoff (battery pack) above 25 cm: lethal/danger flag |
  | > 40 cm | Swim-lite/blocked | Impassable for small vehicles |
- Drag force on vehicles: `F_drag = 0.5 * rho_w * Cd * A_wet * v^2` where `A_wet = width * depth`; applied to the chassis as a quadratic resistance; plus `bow wave` Niagara. Buoyancy for light props: plastic bottles, chairs, fruit crates, sandals float and drift with a slow (0.2-0.4 m/s) flow vector along gutters (flow field from slope of the Pond field). Drift is kinematic along the flow field (no fluid sim).
- Liquids simulation policy: **no real-time fluid sim.** Liquid is surface decals + Niagara particles + height-based ripple shaders + the depth/flow field. Splash events (bottle break, tank burst) spawn Niagara GPU particles with depth-buffer collision plus a Wetness field stamp (`UWetnessField::InjectWater(Pos, Radius, Volume)`), so a burst water tank wets the street below with a decaying puddle.

### 4.5 Dust storms (aandhi)

- Timeline: `T-90 s` sky orange tint + wind ramp + birds hide -> `T0` front arrives (gust front visible as a dust wall, 20-33 m/s peak, visibility 20-50 m) -> 10-20 min -> rain tail optionally (mud stage).
- Physics coupling: global wind ramps; `Dust` field fills (0->1) then deposits on surfaces (adds `Dust` layer to material, grip +10% dry/-15% wet -> mud). Litter pool upsizes to 600 CPU bits near the player + GPU sheets; signboards detach (E > hinge threshold), tarps/awnings tear or rip free (cloth constraints released at tension limit), light chairs slide (wind force vs friction: 3 kg plastic chair, A=0.25 m^2, Cd 1.0: F = 0.5*1.2*1*0.25*20^2 = 60 N vs friction 3*9.8*0.5 = 15 N -> slides, tumbles), wires whip, kites/birds vanish.
- Characters: stagger/push in gusts > 22 m/s in open spots (forecourts), squint/cover animation layer, speed -15%, breath/cough (survival doc), reduced NPC density (they shelter; Mass lane cost reroute to arcades/shop fronts).
- Vehicles: visibility-limited AI (reduce speed to 15 km/h), wipers.
- Sim cost control: dust storm is a *scripted weather preset* whose physical coupling uses the same fields (no extra systems). Only particle count rises (cap 60k GPU, 8k CPU unchanged: reduce other categories while storm is on, via a `WeatherBudgetProfile`).

### 4.6 Heat haze

- No physics coupling except as a *consumer* of the `UTemperatureField`: hot surfaces (asphalt in 46 C sun, tin roofs, tandoors, exhaust, brazier fires) -> post-process refraction distortion mask + Niagara heat ribbons near tandoor/jalebi pans; vertical distortion strength = (T_surface - T_air)/30 x 0.004 UV. Also feeds: tire friction (hot asphalt +3% grip? skip), melting gameplay none. Gameplay: player heat/thirst (survival doc) reads the same. Cost: one post-process pass, masked.

### 4.7 Gameplay tags and events

`UWeatherSubsystem` broadcasts through a `UGameplayTagsManager`-based world tag container and also dispatches **events** on transitions (`OnWeatherStateChanged`, `OnGustFront`, `OnFloodSpotThreshold`), consumed by:
- Mass AI (NPCs open umbrellas, run for cover; Mass `Weather` shared fragment),
- Animation (tags `Wet`, `Windy` select MM sub-DBs),
- Audio (MetaSound params),
- Gameplay (stealth bonus in heavy rain/dust; vision reduction in smog).

### 4.8 Weather data flow, concretely

1. `UWeatherDirector` (data-driven, `UDelhiWeatherPresetDataAsset`) selects current preset by date/time/scenario with blend durations.
2. `UWeatherSubsystem::Tick(10 Hz)` integrates `FWeatherState`, advects gust phase, updates the `UWetnessField` CPU grid slice near the player (ring of 5x5 tiles = 20 m cells... full 4 m cells within 250 m; coarse elsewhere), and pushes `MPC_Weather`/`MPC_Wind` each frame.
3. `UWindField` rebuilds a GPU texture at 15 Hz for shaders & Niagara; a CPU copy provides `Sample()`.
4. Consumers pull by tier: T0 at their rate, T1 coarser, T2+ no physics but shader WPO.
5. Physics results feed back **only via events/stamps** (`InjectWater`, `InjectDust`, bursts, vehicle wakes), never by modifying weather state directly.

---

## 5. Vehicles

### 5.1 Traffic physics vs scripted

Principle: **traffic is Mass-driven kinematic data by default, promoted to physics only where it matters.**

| Tier | Representation | Physics |
|---|---|---|
| T3 (>90 m) | Mass entity on ZoneGraph lane | None |
| T2 (35-90 m) | Spline-follow kinematic actor, simple suspension wobble (spring-damper on pitch/roll from lane curvature & acceleration) | Query collision only. Hits promote to T1 |
| T1 (12-35 m) | Rigid body with raycast-wheel simplified model (4/3 ray springs); AI via steering/throttle inputs identical to T0 | Chaos rigid, sleeps when parked |
| T0 (<12 m) and player-driven/any interactable | Full ChaosVehicles (`ChaosWheeledVehicleMovementComponent`) | Full |

Switching T1 <-> T0 occurs only at near-zero relative closing speed or hidden by occlusion; otherwise keep T1 until the vehicle comes to rest or leaves the player's focus. Vehicles driven by AI at T0 keep AI as input (throttle/brake/steer) into the same vehicle component, so the AI handles the real physical vehicle including slides.

Traffic behaviour in Old Delhi is slow and congested (Chandni Chowk: cycle/e-rickshaws and carts, speed 5-12 km/h; outer ring 20-40 km/h). Chandni Chowk is partly pedestrian/restricted in reality but historically vehicle-sharing; mixed traffic is a design choice, not a physics matter. The Mass traffic flow uses a **cooperative gridlock model**: density-based speed with intentional horn/gesture events; physical collisions are rare but visible (scrapes).

### 5.2 Per-vehicle specifics

| Vehicle | Mass (kg) | Sim notes |
|---|---|---|
| Cycle-rickshaw | 60 + load up to 250 | 3-wheel ChaosVehicle (2 rear drive-by-torque-limit, front steer), pedal-torque curve (rider power ~150 W sustained, 400 W peak), pass load shifts COM (2 passengers: +0.2 m rearward); tipping risk at lateral accel > 3 m/s^2 with tall COM; speed cap 12 km/h; brake lag; rider is an animated NPC with a pedalling anim phase locked to wheel angular velocity |
| E-rickshaw (battery) | 350-450 (+ 4-6 pass 300-400) | 3-wheel; motor 650-1,000 W torque curve; 25 km/h cap; high COM + soft suspension: tip-over at lateral accel > 4 m/s^2 when loaded asymmetrically; braking: hydraulic drum, 6 m stop from 20 km/h; flood cutoff (4.4); hits with pedestrians low energy (0.5*400*(5.5)^2 = 6 kJ at 20 km/h: real danger -> ragdoll) |
| Auto-rickshaw (CNG, Bajaj/TVS style) | 330 (+ 3 pass) | 3-wheel, 7-8 hp, 50 km/h cap, handlebar steering (steer rate high), rear-engine, famously tippy at turns; sliding in wet (rear wheels) |
| Scooter / motorbike | 100-200 | 2-wheel: ChaosVehicles lacks native 2-wheeler dynamics. Implement `UTwoWheelerComponent`: 4 wheels (2 front-pairs shadow) or hidden outriggers plus a PID lean controller (4.1) in async tick: `phi_target = atan(v^2 * kappa / g)` (steady turn lean), PD gains Kp 600, Kd 80 on roll torque about the contact line; countersteer input transforms to lean command; rider as ragdoll-capable skeleton with `Rider` physics asset attached; fall when `|phi| > 55 deg` or low-speed (<4 km/h) with rider-foot support timer 0.6 s (rider puts foot down); lowside slide when `mu_eff` < 0.35 at lean > 25 deg; visual lean via animation/pose-driven bone, collision remains upright capsule-pair for sanity |
| Car (Maruti Alto, Swift, taxi, Innova) | 800-1,600 | ChaosVehicles 4-wheel; slow speeds; low-speed dent/scrape system (decal + mesh deformation morph); stuck in jams; honk events (audio, NPC reaction) |
| DTC bus (low-floor, CNG; 11-12 m) | 11,000-16,000 | ChaosVehicles with 6-wheel layout (rear double axle); heavy damping; Ackermann; `kinematic-dominant` default (T2-T3 spline) because mass makes pedestrian physics responses trivial (pedestrian is knocked aside); T0 only if player is boarding/driving/hitting; collision with cars deals large impulses -> use *pushing semantic*: the bus kinematic path continues; the car/pedestrian is pushed (mass ratio > 5 -> treat bus as kinematic infinite mass in that contact) |
| Delhi Metro | scripted | Pure scripted: spline/timeline trains along elevated line (Chandni Chowk station) with doors/animation and audio; collision volumes only for station platform kill-zones. No Chaos. Physics-correct rattles are audio+camera shake |
| Trucks/tempo (goods carriers) | 1,500-8,000 | Same as car archetype; loads (sacks) are visual + COM shift |
| Horse cart / bullock cart | 300+animal | Kinematic-dominant animal drawn spline, cart pivot via hinge constraint |

### 5.3 Collisions with pedestrians and carts

- **Pedestrian contact classification** with vehicle's `FDelhiImpactEvent`: relative speed v, vehicle mass m; `E = 0.5 * m_eff * v^2` with `m_eff = m_ped*m_veh/(m_ped+m_veh)`; cycle-rickshaw 5 km/h: 70 J -> stagger; e-rickshaw at 15 km/h: 3 kJ-equivalent -> full ragdoll + injury; cars/buses -> severe. Vehicle AI brakes 1.2 s early when pedestrians in path *with a "Delhi acceptance"* -- vehicles brush within 40 cm of pedestrians at slow speeds without events (gesture + horn), a major piece of Delhi authenticity: brush threshold `v < 6 km/h` and lateral gap > 15 cm = no contact event.
- **Pedestrian pushing a cart** (thela): the cart is a rigid body; the NPC attaches via a `PushHandle` constraint (a distance/hinge soft constraint with maximum force 600 N); steering comes from the NPC movement; cart rolls on 2 wheels + frame skid; speed cap 4 km/h loaded; on slope without a handler it rolls (brake bias). If the vehicle hits it, constraint breaks (force > 2 kN), cargo scatters (spill table).
- **Cart vs vehicle**: both rigid; mass ratio manages outcomes; use *CCD off*, sub-stepping through async tick at 60 Hz suffices for < 5 m/s relative speeds.
- **Pedestrian-as-obstacle for T1 vehicles**: use capsule overlap query in the vehicle controller's lookahead (cost 0.01 ms/vehicle), not physics; actual contact only when T0.
- Damage: dent decals + morph targets driven by impulse; windshield crack at 50 J-equivalent; bumper detach only T0.

### 5.4 Vehicle-weather coupling

Traction (4.3), wade/stall (4.4), wind gust (4.2), fog speed reduction (Mass), rain: wiper/auditory, hot-engine stall (summer, queue idling; optional).

---

## 6. Performance budget (ms per subsystem, 16.6 ms frame)

Frame budget shares with graphics (implementation.md section 8 owns the total; this is the CPU/game-thread and physics-thread portion). Target: **physics+simulation total <= 4.2 ms CPU on the game thread critical path, physics thread parallel <= 3.5 ms, GPU sim <= 0.9 ms.**

| Subsystem | Thread | Budget (ms) | Notes / rate |
|---|---|---|---|
| Chaos solver (rigid, constraints, CCD, vehicles, ragdolls) | Physics thread (async) | 3.0 | 60 Hz fixed; 250 awake rigid + 12 vehicles + 8 ragdolls |
| Chaos cloth (6 meshes) | Worker | 0.4 | 30 Hz clothing ticks acceptable, 60 for hero cloth |
| Geometry Collection (300 pieces) | Physics thread | 0.5 (inside solver budget above when active) | bursts only |
| Game-thread physics glue (async tick callbacks, forces, queries) | GT | 0.6 | Traction, wind apply, push forces |
| Significance/tier update | GT | 0.15 amortized | 10 Hz staggered |
| WeatherSubsystem + fields (CPU) | GT | 0.25 | 10 Hz sim, 2 Hz wetness grid slices |
| Character movement (25 T0 + 60 T1) | GT/worker | 1.2 | CMC 25 x 0.03 + 60 x 0.01 plus IK traces |
| Foot IK traces (50/frame) | GT/async | 0.15 | async traces batched |
| Crowd contact resolver | GT | 0.2 | 30 Hz near-player |
| Physical animation/ragdoll blend | Anim worker | 0.35 | |
| Animation (MM, 85 chars) | Anim workers | share of anim budget in implementation.md; MM search cost <= 1.0 ms total across workers | Reduced rates T1 |
| Mass traffic/crowd (T2/T3) | Mass workers | 0.8 | Counts in implementation.md 8; physics contribution only here |
| Vehicle AI + simplified T1 | GT | 0.3 | |
| Interaction/object logic (support groups, wake events, break events) | GT | 0.25 | |
| Niagara gameplay-coupled (rain, debris, birds, dust) | GPU | 0.9 | Birds ~0.5 per 4.4 included |
| Wind/wetness texture upload | GPU/RHI | 0.05 | 128 KB per update max |
| Slack/spike margin | - | 0.8 | |

Spike rules: any single event (explosion/GC break) costs <= 1.5 ms (budget enforced by pooling; if budget exceeded over 3 frames, quality fallback: fewer shards, no new ragdolls). Dynamic scalability (`UDelhiSimScalability`): measured frame time smoothed over 0.5 s; when physics ms > budget, lower caps stepwise (awake props 250 -> 180 -> 120, ragdolls 8 -> 5, cloth 6 -> 3, pedestrians T0 radius 12 -> 9 m).

Memory (VRAM/RAM): pools ~120 MB RAM; wind/wetness fields < 2 MB; Chaos bodies ~1.5 KB each (about 400 KB for 250); no VRAM pressure from physics beyond Niagara buffers (about 60 MB).

### 6.1 Profiling plan

1. **Unreal Insights**: custom trace channel `DelhiSim` with scoped events per subsystem (`TRACE_CPUPROFILER_EVENT_SCOPE_STR`). Weekly capture of 3 canonical benches: (a) peak bazaar walk (Dariba, 25 NPC T0 + crowd), (b) monsoon junction with 8 vehicles, (c) dust storm with maximal debris. Fail the build if any subsystem exceeds budget x 1.25 in the Insights regression script (headless, `-trace=cpu,frame,log` -> `UnrealInsights.exe -ExecOnAnalysisCompleteCmd`).
2. **Chaos Visual Debugger**: record `p.Chaos.DebugDraw`/CVD capture of the physics thread in the bad frames: inspect awake body counts, contact pairs, constraint solve times, island sizes. Specific checks: islands > 40 bodies (stall fruit piles -> splits), CCD overuse, sleeping failures (bodies that never sleep), cloth self-collision cost, jitter from stacked crates.
3. **stat commands**: `stat Chaos`, `stat ChaosCounters`, `stat PhysicsUpdate` (names to be confirmed in 5.8), `stat Anim`, `stat Niagara`, `stat Mass`. Console `p.Chaos.Solver.*` iteration cvars for the 8/2 setting.
4. **Automated perf tests**: Gauntlet / Functional Test maps spawn N dynamic props and verify the awake cap and ms; replayable from fixed RNG seeds (determinism below).
5. **GPU**: RenderDoc/Insights GPU for Niagara; `stat GPU`, `ProfileGPU` for the rain/dust passes.
6. **Budget dashboard**: in-game dev overlay (`DelhiSim.Debug`) showing live counts per tier, awake bodies, ragdolls, promotions/s, per-subsystem ms from Insights scopes.

---

## 7. Determinism, networking readiness, save/load

### 7.1 Determinism

- Chaos is **not** guaranteed bit-deterministic across machines/builds (float order, threading, SIMD paths). Therefore: **do not rely on physics determinism for gameplay-critical outcomes** in single-player. Use determinism where it is cheap:
  - All gameplay RNG (slips, reactions, spills, crowd micro-decisions) comes from `FDelhiRandomStream` seeded by `(WorldSeed, EntityStableId, EventIndex)`: counter-based RNG (Philox-like) so results don't depend on call order.
  - Weather is deterministic from `(WorldSeed, GameTime)`: gust process is a pure function of time and cell, never an accumulated RNG state, so loading a save and re-entering at time T reproduces the same gusts.
  - Fixed 60 Hz step (async physics) with clamped max steps; no variable dt in physics-affecting code.
  - Replays/debug: record inputs + events (not physics), use CVD recordings for debugging, not for playback.
- Same-machine record/replay test harness (for regression): run a scripted scenario twice and compare coarse results (positions within 5 cm tolerance after 10 s). Use as a smoke test, not equality.

### 7.2 Networking readiness (Chaos Networked Physics in 5.8)

Findings: Mover (experimental) supports rollback networking via Network Prediction **or Chaos' Networked Physics system** [V Epic docs]; Chaos Mover runs on the physics thread at a fixed timestep and replicates through the Network Physics Component in *Resimulation* mode [V Epic docs/forums]; Epic's Chaos team states the main step before Beta is locking down API/data format, plus Iris delta compression [V forum]; forum reports a client crash on joining a session with Chaos Mover resimulation [V forum]. Conclusion: **networked physics exists but is Experimental in 5.8; the multiplayer plan is not to use it for Phase 1.**

Readiness rules (cheap to follow now, expensive to retrofit):
1. Keep simulation logic in **async physics tick** with explicit input structs (`FPhysInput`) separate from state (`FPhysState`); this maps to Network Physics' input/state model if adopted later.
2. All gameplay authority behind `HasAuthority()` checks; physics-affecting RPC-free design: the authoritative server (or the host) owns T0/T1; T2/T3 are cosmetic client-side from replicated Mass seeds (`FMassReplicationFragment` / seeded RNG -> clients regenerate identical background crowd/traffic with the same seed + time).
3. Fields (wind/wetness) are deterministic from `(seed,time)` so clients compute them locally without replication; only discrete events (`InjectWater`) replicate.
4. Prop state replication policy: awake props replicate via Iris with quantized transforms (position 1 cm, rotation 8-bit/axis) at 10-20 Hz with distance-based priority; sleeping props not at all (sleep flag + final transform once).
5. Ragdolls: replicate the *trigger* (impulse, time) not the pose; each client simulates its own ragdoll (cosmetic); server captures final location for the capsule.
6. Vehicles: if co-op driving is pursued, evaluate Chaos Vehicles network prediction in a later spike; do not couple the player pawn to Mover until Mover leaves Experimental and the data format is frozen.
7. Keep `IDelhiMovementAdapter` so CMC <-> Mover can be swapped.

### 7.3 Save/load of physics state

Do not serialize Chaos internal state. Save *intent-level* state in `UDelhiPhysicsStateSubsystem` (USaveGame-compatible, versioned):

- Per persistent entity: `StableId (FGuid from level-baked id)`, class, **tier-independent pose** (transform + linear velocity quantized), **sleep flag**, damage/destruction state (GC bitmask of broken clusters, dents mask), carried-by/attached-to ids, owner (stall), inventory.
- World fields: wetness grid snapshot (RLE-compressed 16-bit, 4 m cells -> about 150 KB for the slice, downsized to 8 m for save), pond depths, dust deposition, `WeatherState` (preset, blend progress, game time), gust seed/time.
- Transient stuff (debris, shards, litter bits, active ragdolls, particles) is **not** saved: on load, active ragdolls are converted to a pose snapshot (final lying pose) and frozen; debris regenerates procedurally from the PCG seed.
- Load flow: stream cells -> spawn entities in `FrozenStatic` mode with the saved transform -> when the entity's significance enters T1, convert to the active tier (with the saved velocity zeroed). This eliminates the "everything settles on load" explosion.
- Delta saves: only entities whose state differs from the level default are stored (dirty flags set on movement > 5 cm, break, interaction). Budget: < 200 KB/typical save for physics; 1 MB worst case. Write on autosave at a calm moment (not during ragdolls), async serialization.

---

## 8. C++ class list

Naming prefix `Delhi`. Module split: `DelhiSimCore` (fields, tiers, state), `DelhiCharacterPhysics`, `DelhiObjects`, `DelhiVehicles`, `DelhiWeatherCoupling`. Update rates: "GT" game thread tick; "Async" async physics tick (60 Hz fixed); "Event" on demand.

### 8.1 Subsystems

| Class | Base | Responsibilities | Rate |
|---|---|---|---|
| `UDelhiSimTierSubsystem` | `UTickableWorldSubsystem` | Wraps `USignificanceManager`; policies per class; promotion queue; hysteresis | 10 Hz staggered |
| `UDelhiSimBudgetSubsystem` | `UWorldSubsystem` | Hard caps (awake bodies, ragdolls, cloth, shards), scalability stepping, Insights scopes, debug overlay | GT 4 Hz + event |
| `UWeatherSubsystem` | `UTickableWorldSubsystem` | State machine, presets, MPC writing, tags/events, Niagara Data Channels, audio params | 10 Hz sim, per-frame MPC |
| `UWindField` (owned by Weather, also a `UObject` interface) | `UObject` | Global+gust+canyon sample; double-buffered; GPU texture | 15 Hz rebuild, Sample() anywhere |
| `UStreetCanyonField` | `UObject`/DataAsset-backed | Loads baked canyon atlas; shelter factor lookup; rain shelter mask | Static |
| `UWetnessField` | `UObject` | Wetness/Pond/Mud/Moss/Dust grid, flood spots, `Inject*` APIs, `mu_eff` sampling, GPU texture | 2 Hz sim near, 0.5 Hz far |
| `UPhysicalSurfaceSampler` | `UWorldSubsystem` | Cached surface type per tile (from landscape/road materials & baked grid) + trace fallback; returns `FSurfaceSample` | Event/cached |
| `UCrowdContactSubsystem` | `UTickableWorldSubsystem` | Spatial hash, player/NPC soft contact, crowd pressure per cell | 30 Hz |
| `UStepProfileSubsystem` | `UWorldSubsystem` | Authored step/kerb segments for IK & MM | Static + query |
| `UDelhiPhysicsStateSubsystem` | `UGameInstanceSubsystem` + World | Persistent entity registry, save/load, freeze-on-load, delta tracking | Event |
| `UDelhiPoolSubsystem` | `UWorldSubsystem` | Pools: ragdoll meshes, props, GCs, cloth | Event |
| `UDelhiImpactSubsystem` | `UWorldSubsystem` | Converts Chaos collision events (`UChaosEventListenerComponent` / OnComponentHit with filtering) into `FDelhiImpactEvent`; dispatch to reaction, destruction, audio, decals | Event + 30 Hz batch |
| `UDelhiTrafficBridgeSubsystem` | `UWorldSubsystem` | Mass traffic entity <-> vehicle actor promotion/demotion | 10 Hz + event |
| `UDelhiRandomService` | `UWorldSubsystem` | Counter-based RNG streams | Event |

### 8.2 Components

| Class | On | Responsibilities | Rate |
|---|---|---|---|
| `UDelhiMovementComponent` | Character | CMC-derived: modes Wade/Slip/Squeeze/Carry/Stagger; step-up; traction; publishes `FCharacterPhysState` | GT per-move |
| `UTractionComponent` | Character/Vehicle/Prop | Samples `UWetnessField`/surface; outputs mu_eff, wheel friction multipliers, slip risk | 20-30 Hz (T0), Async for vehicles |
| `UFootIKComponent` | Character | Traces, pelvis offset, IK targets to anim BP via interface | 30-60 Hz T0 |
| `UCharacterReactionComponent` | Character | Impact -> reaction selection, physical animation profiles, partial ragdoll | Event |
| `URagdollControllerComponent` | Character | Blend in/out, pose snapshot, get-up selection, sleep/freeze, pooling | Event + 30 Hz |
| `UCarriedLoadComponent` | Character | Load mass/COM/spring-damper/balance/spill | 30 Hz |
| `UCrowdAgentPhysicsComponent` | Character | Squeeze capsule, push responses, jostle | 30 Hz |
| `UPhysicsInteractionComponent` | Player/NPC | Push/pickup/carry of props with force caps | Async |
| `UWindDrivenComponent` | Prop/Cloth/Wire/Sign | Samples wind, applies drag/lift (rigid) or cloth wind; WindWakeSpeed; detach threshold | 20-30 Hz by tier |
| `UDelhiClothDriverComponent` | Cloth actors | Sim<->WPO tier switching, wind coefficients, tear thresholds | 30 Hz |
| `UCableSpanComponent` | Wire spans | Analytic catenary + optional cable constraint chain, wind sway, cut | 10 Hz / Async T0 |
| `UKiteComponent` | Kite actor | Aero lift/drag, tether, cut | 30 Hz |
| `UWaterDepthComponent` | Any | Reads Pond; buoyancy/drag; wade; engine flooding | 10-30 Hz |
| `USimTierComponent` | All simulatables | Registers with significance; implements `ISimTierable` | Event |
| `UStallPhysicsGroupComponent` | Stall actors | Group sleep/wake, support graph, spill | Event |
| `UDestructibleObjectComponent` | Destructibles | Energy thresholds, GC swap-in from pool, shard budget | Event |
| `UTwoWheelerComponent` | Bikes/scooters | Lean PID, rider handling, low-speed support | Async |
| `UDelhiVehicleComponent` | Four/three-wheeled | Wraps ChaosWheeledVehicleMovementComponent: tip risk, flood, hydroplane, damage | Async |
| `USimplifiedVehicleComponent` | T1 vehicles | Ray-spring model, AI inputs | 30-60 Hz |
| `UVehicleAIInputComponent` | Vehicles | Throttle/brake/steer from Mass lane + avoidance; "Delhi brush" tolerance | 20 Hz |

### 8.3 Data assets (all `UPrimaryDataAsset`)

| Asset | Contents |
|---|---|
| `UDelhiObjectDef` | Mass, collision archetype, sleep rules, BreakEnergy/DentEnergy, wind Cd/Area, WindWakeSpeed, interaction verbs, spill table, physics material buckets, tier overrides |
| `UDelhiSurfaceDef` | Surface type, mu_dry, wet_ratio, porosity, drain rate, dry time, hydroplane params, sound/VFX/footstep, ballistic tie-in (reuses the 4.1 DataTable) |
| `UDelhiWeatherPresetDA` | Wind mean/gust params, rain rate, wetness rates, dust, smog, temperature, transition curves, budgets profile |
| `UDelhiSimClassPolicyDA` | Per-class significance thresholds, radii, caps, promotion limits, anim rates |
| `UDelhiCharacterPhysProfileDA` | Capsule/squeeze params, step heights, physical animation profiles, energy thresholds, slip params, MM DB references |
| `UDelhiVehicleDefDA` | Mass, COM, wheel/engine/torque curves, tip & stall thresholds, T1 model params, damage |
| `UDelhiCanyonAtlasDA` | Baked street canyon grid (direction/gain/shelter), rain shelter mask, resolution/origin |
| `UDelhiFloodSpotDA` | Basins, capacities, drain rates, hazard flags |
| `UDelhiLoadProfileDA` | Porter/load types, mass ranges, COM, animation tags, spill |

### 8.4 Editor/tooling

`UDelhiCanyonAtlasBuilder` (commandlet from OSM/footprints), `FDelhiSimDebugDrawer` (tier colors, wind arrows, wetness heat map), console variables `DelhiSim.*`, Insights trace channel, Functional test base class `ADelhiSimTest`.

### 8.5 Key interface sketches

```cpp
UENUM() enum class ESimTier : uint8 { T0_Full, T1_Active, T2_Kinematic, T3_Statistical, T4_Frozen };

UINTERFACE() class USimTierable : public UInterface { GENERATED_BODY() };
class ISimTierable { GENERATED_BODY() public:
  virtual void ApplyTier(ESimTier Old, ESimTier New) = 0;   // enable/disable bodies, anim rate, collision profile
  virtual float GetSignificanceBias() const { return 0.f; }
  virtual FGuid GetStableId() const = 0;
};

USTRUCT() struct FSurfaceSample { GENERATED_BODY()
  TEnumAsByte<EPhysicalSurface> Surface; float MuDry, Wet, PondCm, Mud, Moss, Dust, MuEff; };

USTRUCT() struct FDelhiImpactEvent { GENERATED_BODY()
  TWeakObjectPtr<AActor> Instigator, Victim; FVector Loc, Impulse; float EnergyJ; EPhysicalSurface Surface; FName Bone; };

// Fields: lock-free, double-buffered, safe from async physics:
FVector UWindField::Sample(const FVector& WorldPos, float Time, float PhaseOffset) const;
FSurfaceSample UWetnessField::Sample(const FVector& WorldPos, EPhysicalSurface Surf) const;
void UWetnessField::InjectWater(const FVector& Pos, float RadiusCm, float VolumeL);
```

---

## 9. Build order, first 4 weeks

Team assumption: 1 physics/gameplay programmer lead + 1 tech-artist/engineer support. Every week ends with a profiled build (Insights) and a demo scenario.

**Week 1: Foundations and spikes**
- Project setup: async physics tick at 60 Hz, collision channels/presets, `UDelhiSurfaceDef` + `PM_*` with 3 wetness variants for the 4 street stones, `FDelhiImpactEvent` and impact subsystem.
- `UDelhiSimTierSubsystem` skeleton on `USignificanceManager` with the 5 tiers and the `ISimTierable` interface; debug overlay colouring by tier. `UDelhiSimBudgetSubsystem` counts.
- Spikes (timeboxed 1 day each, record results in this doc): (a) Chaos solver cost with 250 awake props + 12 vehicles on the 3070; (b) `USignificanceManager` update cost at 2,000 objects; (c) Chaos cloth wind API on a tarp and CVD capture; (d) CMC vs Mover decision confirmed = CMC; (e) 5.8 `stat Chaos` names and CVD workflow.
- Exit: tier assignment working on 500 test props; caps enforced.

**Week 2: Weather fields + characters baseline**
- `UWeatherSubsystem` with presets (clear/monsoon/dust/smog), MPC writes, `UWindField` (global + gust, no canyon yet), `UWetnessField` (wetness/pond/dry only), `mu_eff` sampling; `UWindDrivenComponent` on a signboard, a tarp and litter.
- `UDelhiMovementComponent` with step-up, traction and slip; `UFootIKComponent` (traces) on the slice kerbs; `UStepProfileSubsystem` minimal.
- Exit: playable: walk on sandstone, rain turns it wet, slip risk visibly changes; wind moves signs/tarps; 3 Insights captures.

**Week 3: Reactions, crowds, objects**
- `UCharacterReactionComponent` + physical animation profiles + `URagdollControllerComponent` with blend in/out and pooling (8 cap); `UCrowdContactSubsystem` + squeeze/jostle; porter `UCarriedLoadComponent` v1.
- Object framework: `UDelhiObjectDef`, `UPhysicsInteractionComponent` (push/pick up), `UStallPhysicsGroupComponent`, first 10 classes (crate, chair, bottle, fruit, sack, cart, signboard, glass, shutter, litter), sleep/wake rules, `UDestructibleObjectComponent` with GC pool for chair/glass.
- Exit: Dariba-style test alley with crowd, porter, stall; knock-over interaction; CVD review of island sizes.

**Week 4: Vehicles, canyon wind, flooding, persistence**
- Vehicles: `UDelhiVehicleComponent` on the e-rickshaw and auto (3-wheel), `USimplifiedVehicleComponent` (T1), traffic bridge (T2 <-> T1 <-> T0 promotion) with Mass lanes, `UTwoWheelerComponent` prototype with the PID lean (scooter), vehicle-pedestrian impact classification; cycle-rickshaw after.
- Weather: canyon atlas builder v1 (from footprints, direction+shelter) + shelter mask for rain; flood spots + wade slowdown + vehicle stall; dust storm preset coupling (sign detach, chair slide).
- `UDelhiPhysicsStateSubsystem` save/load (poses, sleep, damage, wetness snapshot) and freeze-on-load.
- Perf: `DelhiSim` Insights scopes, three canonical benches, the dynamic scalability stepper; first budget report vs section 6.
- Exit: 500x500 slice at 60 fps in all four weather presets with target counts or documented gaps; save/load round trip test passing; networking-readiness checklist (section 7.2) signed off.

Deferred (Week 5+): Mover evaluation, active ragdoll (Control Rig Physics), DTC bus T0, cable sim constraints, kites, CFD-seeded canyon, net replication prototype.

---

## 10. Risks

| Risk | Mitigation |
|---|---|
| Chaos cost explodes in stalls (many contacting small bodies, large islands) | Stall groups, sleeping-by-default, convert to static beyond 12 m, island-size monitor in CVD |
| CMC + MM + physical animation conflict (foot sliding, jitter) | One-way data flow (2.1); physics layers after MM; per-class MM DBs |
| Wet-material swapping cost / physical material changes at runtime | 3 buckets + mover-side friction override; no per-frame swaps |
| Experimental features (Mover, Control Rig Physics Beta, Net Physics) shift in 5.8.x | Adapter interfaces; feature flags; CMC primary |
| Tier popping visible (T2 -> T1 anim/pose pops) | Hysteresis, in-view pop suppression (promote when behind camera if possible), blend frames |
| Canyon wind plausibility | Start with analytic atlas; validate against measured data (IMD/Delhi gust records) and art direction; tuneable per street |
| 8 GB VRAM pressure from Niagara | Hard particle caps; weather budget profiles |
| Determinism expectations from design | Doc'd: gameplay RNG deterministic, physics is not |

Open verification items (not confirmed in web sources, to check in the Week 1 spikes): exact 5.8 cvar/stat names; whether Chaos Vehicles support a native 2-wheeler (assumed not); `USignificanceManager` API changes; Niagara Data Channel 5.8 API; Chaos cloth runtime wind API surface.

---

## Sources

- Unreal Engine 5.8 Tidies Up (Digital Production, 18 Jun 2026): Chaos Cloth and Dataflow Production-Ready; Control Rig Physics Beta; Control Rig Dynamics Experimental; Fog Screen Space Scattering Experimental. https://digitalproduction.com/2026/06/18/unreal-engine-5-8-tidies-up/
- Mover in Unreal Engine (Epic documentation): Mover experimental; rollback networking via Network Prediction or Chaos Networked Physics; CMC to remain supported. https://dev.epicgames.com/documentation/unreal-engine/mover-in-unreal-engine
- Chaos Mover documentation (UChaosMoverBackendComponent API page): https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/ChaosMover/UChaosMoverBackendComponent
- Unreal Engine 5.8 Released (Epic forum announcement): https://forums.unrealengine.com/t/unreal-engine-5-8-released/2729274
- Chaos Mover forum thread (physics-thread fixed step, Network Physics resimulation): https://forums.unrealengine.com/t/chaos-mover/2733017
- Question about the next steps for Chaos Mover (Beta plan: API/data format, Iris delta compression): https://forums.unrealengine.com/t/question-about-the-next-steps-for-chaos-mover/2743117
- Chaos Mover resimulation crash report: https://forums.unrealengine.com/t/guaranteed-client-crash-shortly-after-joining-a-session-with-chaos-mover-resimulation-access-violation-in-testmodeparticledisabled/2742359
- Unreal Engine 5.8 Preview (Epic forum): https://forums.unrealengine.com/t/unreal-engine-5-8-preview/2721597
- Project docs: `implementation.md` (4.1, 4.3, 4.4, 4.5, 8), `realism/physics-simulation.md`, `realism/damage-and-ballistics.md`, `realism/animation-and-character-motion.md`, `realism/world-simulation.md`.
- Numerical values in sections 2-5 (masses, thresholds, friction ratios, time constants, wind speeds) are design starting points derived from engineering rules of thumb and the climate figures already cited in `implementation.md`; they are tunables, not measured data, and are labelled as such.
