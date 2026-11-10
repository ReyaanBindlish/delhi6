# Physics and Vehicles (Chaos, UE 5.8)

Implements the bible's "Physics and realism rules" and `implementation.md` 4.1 / research 03. Code lives in
`Source/OldDelhi/Physics` and `Source/OldDelhi/Vehicles`; editor setup in `Tools/ue/setup_physics.py`.
Everything is C++ with Blueprint-callable functions, no weapon system yet, no missions.

## 1. Module map

| File | What it is |
|---|---|
| `Physics/DelhiSurfaceTypes.h` | `EDelhiSurface` (value == `EPhysicalSurface` value), `FDelhiSurfaceData` |
| `Physics/DelhiSurfaceLibrary` | Surface table, `GetSurfaceFromHit`, wetness-aware friction (`UDelhiWeatherSubsystem::GetWetness`) |
| `Physics/DelhiBallisticsLibrary` | Segmented-arc bullet trace, penetration, ricochet, katta misfire. Library only |
| `Physics/RailingComponent` | `URailingComponent`: 90 cm builder rail vs 110 cm code rail, COM-over-rail test, shove |
| `Physics/FallTrackerComponent` | Measures capsule or ragdoll fall height, calls `DayaVitalsComponent::ApplyFall` |
| `Physics/HitReactionComponent` | Partial physical-animation hit reaction, full ragdoll, ragdoll budget subsystem (cap 8) |
| `Vehicles/DelhiVehicleWheels` | Wheel classes (CDO-configured) for every vehicle |
| `Vehicles/DelhiVehiclePawn` | `ADelhiVehiclePawn` base + E-rickshaw, cycle-rickshaw, Fortuner-class SUV, motorbike |
| `Vehicles/DelhiBoat` | `UBoatBuoyancyComponent`, `ADelhiFishingBoat` (Act 5), `UDelhiBoatMath` |

Build.cs additions are listed in section 8.

## 2. Physical surfaces and materials

Surface type numbers are fixed: `EDelhiSurface` value N is `SurfaceTypeN`. Put these lines in
`Config/DefaultEngine.ini` (the integrator applies them):

```ini
[/Script/Engine.PhysicsSettings]
+PhysicalSurfaces=(Type=SurfaceType1,Name="LakhoriBrick")
+PhysicalSurfaces=(Type=SurfaceType2,Name="LimePlaster")
+PhysicalSurfaces=(Type=SurfaceType3,Name="RedSandstone")
+PhysicalSurfaces=(Type=SurfaceType4,Name="KotaStone")
+PhysicalSurfaces=(Type=SurfaceType5,Name="CorrugatedShutter")
+PhysicalSurfaces=(Type=SurfaceType6,Name="Glass")
+PhysicalSurfaces=(Type=SurfaceType7,Name="Asphalt")
+PhysicalSurfaces=(Type=SurfaceType8,Name="Mud")
+PhysicalSurfaces=(Type=SurfaceType9,Name="Wood")
+PhysicalSurfaces=(Type=SurfaceType10,Name="SheetMetal")
+PhysicalSurfaces=(Type=SurfaceType11,Name="Water")
+PhysicalSurfaces=(Type=SurfaceType12,Name="Concrete")
+PhysicalSurfaces=(Type=SurfaceType13,Name="Flesh")
bTickPhysicsAsync=True
AsyncFixedTimeStepSize=0.0166667
```

Fallback if async physics misbehaves: `bTickPhysicsAsync=False`, `bSubstepping=True`, `MaxSubstepDeltaTime=0.0166667`,
`MaxSubsteps=4`.

| Surface | Friction | Restitution | Density g/cm3 | Wet friction x (wetness 1) | Pen. J/cm | Ricochet below |
|---|---|---|---|---|---|---|
| Lakhori brick | 0.65 | 0.10 | 1.9 | 0.85 | 90 | 12 deg |
| Lime plaster | 0.60 | 0.05 | 1.6 | 0.80 | 40 | none |
| Red sandstone | 0.60 | 0.10 | 2.3 | 0.72 | 95 | 14 deg |
| Kota stone | 0.55 | 0.10 | 2.7 | 0.55 | 110 | 16 deg |
| Corrugated shutter | 0.40 | 0.30 | 7.8 | 0.70 | 60 | 18 deg |
| Glass | 0.30 | 0.20 | 2.5 | 0.70 | 8 | none |
| Asphalt | 0.75 | 0.05 | 2.3 | 0.65 | 120 | 10 deg |
| Mud / slush | 0.45 | 0.00 | 1.7 | 1.00 (already slick) | 55 | none |
| Wood | 0.50 | 0.25 | 0.7 | 0.75 | 25 | none |
| Sheet metal (car body) | 0.35 | 0.25 | 7.8 | 0.70 | 12 | 20 deg |
| Water | 0.05 | 0.00 | 1.0 | 1.00 | 5 | none |
| Concrete | 0.70 | 0.05 | 2.4 | 0.80 | 150 | 12 deg |
| Flesh | 0.60 | 0.05 | 1.0 | 1.00 | 35 | none |

Friction, restitution and density of the first eight rows are from implementation.md 4.1. Wet scale, penetration and
ricochet columns are tuning starting points (not sourced); change them in `DelhiSurfaceLibrary.cpp` and in one place only.
Mud uses Friction Combine = Multiply (set by `setup_physics.py`).

Run once the project has the ini lines and the `/Game/Delhi/Materials` instances:

```
UnrealEditor-Cmd.exe OldDelhi.uproject -run=pythonscript -script="Tools/ue/setup_physics.py" -unattended -nop4
```

`DELHI_PHYS_DRYRUN=1` only logs. Names from `build_vertical_slice.py` map exactly (`MI_Brick` -> Lakhori brick,
`MI_Asphalt` -> asphalt, `MI_Ground` -> mud, `MI_RollingShutter` -> shutter, ...); later materials fall back to keywords.
Characters need the `PM_Flesh` physical material on their Physics Asset bodies for bullets to read "Flesh".

Wetness: `UDelhiSurfaceLibrary::GetEffectiveFriction(World, Surface)` = dry friction x lerp(1, wetScale, Wetness).
Vehicles read it every 0.5 s.

## 3. Ballistics

`UDelhiBallisticsLibrary::FireShot` (Blueprint or C++). Rules from the bible:

- Bullet drop: the path is traced in 10 m segments with world gravity. 9 mm drops about 10 cm at 50 m, a katta about 31 cm.
- Penetration: after a hit, a reverse trace against the same component finds the exit face, thickness t. Energy lost =
  J/cm x t. The bullet stops if loss >= remaining energy. Examples: a half-brick (11 cm) = 990 J, so a 9 mm (500 J) and a
  katta (250 J) stop; a car door (sheet metal, 12 J/cm, ~15 cm of collision) costs 180 J, so a 9 mm passes through with 320 J
  left. `WouldStop()` exposes the same test.
- Ricochet: below the material's grazing angle the bullet reflects with 40% energy and a 5 degree scatter.
- No aim assist: the direction is exactly the caller's plus the profile's spread cone. There is no target lookup anywhere.
- Katta: `MisfireOneIn = 8`, drawn from a caller-owned `FRandomStream` (seed per weapon for reproducible saves).
- Actors tagged `BallisticsIgnore` (the construct wall) are skipped.
- Output is `FDelhiShotResult` with per-impact actor, bone, surface, energy before/after. Damage, decals, Niagara and
  MetaSound lookups are keyed by `EDelhiSurface` in the future weapon system.

Weapon profiles: 9 mm 500 J / 350 m/s / 0.6 deg; katta 250 J / 200 m/s / 2.5 deg; 7.62 2500 J / 710 m/s / 0.3 deg.
Gunfire summoning police (3 to 5 minutes) is mission/world logic, not here.

## 4. Falls and rails

`URailingComponent` models the character as an inverted pendulum pivoting on the rail top:

```
lever = COM height - rail height          r = sqrt(d^2 + lever^2)
needed speed v = sqrt( 2 g (r - lever) * grip / efficiency )
```

`d` is the horizontal COM distance behind the rail (about 15 cm when the body touches it), efficiency 0.7. COM height is
57% of standing height above the floor the rail stands on. While `DayaVitalsComponent::IsInputLocked()` (cough lock) he
cannot grab the rail (grip 1.0), otherwise grip 1.5.

| Case (172 cm man, COM 98 cm) | lever | Required shove speed |
|---|---|---|
| 90 cm builder rail, coughing | +8 cm | ~1.6 m/s (a hard shove while staggering) |
| 90 cm builder rail, can grab | +8 cm | ~2.0 m/s |
| 110 cm code rail, coughing | -12 cm | ~3.0 m/s (not reachable by a shove) |

So the balcony is plausibly lethal and plausibly the rail's fault. The component origin is the middle of the rail,
the run is along local X, the drop side is local +Y.

On `TryShove`: if the COM goes over, the rail lets that capsule through for 2.5 s, launches him, adds a
`UFallTrackerComponent` if missing, and ragdolls him if a `UHitReactionComponent` exists. `UFallTrackerComponent` reports peak-to-landing
height in metres to `DayaVitalsComponent::ApplyFall` (which owns the 3 m injury / 6 m death thresholds; the tracker also
broadcasts `EFallSeverity`). For Act 6 the fourth-floor balcony is ~10 m: lethal.

## 5. Ragdolls and hit reactions

`UHitReactionComponent` needs a skeletal mesh with a Physics Asset (bone names `pelvis` by default).

- Impulse < 180 N s: physical animation on the bone chain below the hit bone, blend 0.6 down to 0 over 0.8 s.
- Impulse >= 180 N s (or dead): full ragdoll, capsule collision off, movement disabled, mass rescaled to 70 kg if the
  Physics Asset is more than 25% off.
- `UDelhiRagdollBudgetSubsystem` keeps at most 8 active ragdolls; the oldest is put to sleep (pose kept).
- `RecoverFromRagdoll()` stands the character back up (not for the dead).
- Typical real impulses: bullet ~3 N s, punch ~30, hard shove ~80 to 150, tackle ~250.

## 6. Vehicles

All vehicles are `AWheeledVehiclePawn` children. Chaos needs the wheel setup before physics state creation, so each class
configures its wheels, engine and gearbox in the constructor. A Blueprint child supplies the skeletal mesh, Physics Asset
and Anim BP (parent `UChaosVehicleAnimationInstance`). Controls are not bound here: call
`SetDriveInput(Throttle, Brake, Steer, Handbrake)` from the player controller (keyboard, gamepad, touch).

| Vehicle | Class | Bones | Mass | Top speed | COM height (origin at ground) | Rollover (lateral g) |
|---|---|---|---|---|---|---|
| E-rickshaw | `ADelhiERickshawPawn` | Wheel_F, Wheel_RL, Wheel_RR | 350 kg (+70 per passenger, up to 4) | 25 km/h | 55 cm (66 cm with 3 pax) | 0.91 empty, 0.76 with 3 pax |
| Cycle-rickshaw | `ADelhiCycleRickshawPawn` | same | 160 kg + passengers (~300 kg loaded) | 12 km/h | 60 cm | 0.88 |
| Fortuner-class SUV | `ADelhiSuvPawn` | Wheel_FL/FR/RL/RR | 2,100 kg | 170 km/h | 75 cm | 1.05 |
| Motorbike | `ADelhiMotorbikePawn` | Wheel_F, Wheel_R, Outrigger_L/R | 265 kg with rider | 110 km/h | 60 cm | held up by lean PD |

Facts and derivations:

- E-rickshaw: 14 Nm motor, 9:1 reduction, RPM cap 2,300 (= 25 km/h on 28 cm wheels), rear drive, brake 150 / 220 / 220 Nm. A 5 m
  radius turn at 25 km/h is ~1 g lateral, above the loaded rollover threshold, so it tips if the player throws it round.
- Cycle-rickshaw: same rig, 7 Nm, 5:1, RPM cap 520 (= 12 km/h), about 150 W of pedalling.
- SUV braking: target 80 km/h (22.2 m/s) in 40 m needs 6.17 m/s^2 (v^2/2d). Wheel torque for that: 2,100 kg x 6.17 x 0.378 m =
  4,900 Nm total; with 1,500 Nm per front wheel and 1,000 Nm per rear, force at the tyres is 13.2 kN = 6.3 m/s^2, and dry-asphalt
  friction allows 7.4. Wet asphalt (x0.65) limits it to 4.8 m/s^2: ~51 m, which `EstimateStoppingDistanceM()` reflects.
  `StartBrakeTest(80)` launches, brakes fully and logs the real distance; tune `MaxBrakeTorque` in `DelhiVehicleWheels.cpp` if
  the Chaos result is not 36 to 44 m.
- Motorbike: front, rear and two small low-friction outrigger wheels that sit ~15 cm above the road line and touch only past
  ~25 degrees of lean. A PD controller adds torque about the forward axis to hold the bike upright at rest and bank it
  into turns (`MaxLeanDeg` 35). `LeanTorqueSign` may need flipping after the first test; read `GetLeanAngleDeg()` for the
  visual lean. Karachi traffic average of 18 km/h comes from the traffic layer, not the vehicle.
- No ramps or jumps: no stunt code exists. Mass and COM change with `SetPassengerCount()` (recreates the physics state: do it
  while stopped).
- Weather: wheel `FrictionForceMultiplier` is scaled by wet asphalt every 0.5 s.

### Act 5 boat

`ADelhiFishingBoat`: 2,500 kg hull, 40 hp, 50% shaft-to-thrust efficiency, so 14.9 kW at the propeller. Quadratic drag is sized so
full throttle through still water reaches exactly 25 km/h; thrust is constant power (P/v) capped at 3,900 N static.
`UBoatBuoyancyComponent` samples 8 hull points against a flat water level plus a two-wave swell and floats at half
submersion. `WaterCurrentKmh` is the ebb (default 4 km/h toward -X). Speed through water and over ground are separate
(`GetWaterSpeedKmh`, `GetGroundSpeedKmh`). Note: 25 km/h against a 4 km/h ebb leaves 21 km/h over ground, so physics alone allows
turning back; Ravi's death is therefore a mission rule: `StoryTurnBackPenaltySeconds = 360` (6 minutes) belongs to the Act 5
mission script, with `UDelhiBoatMath::EstimateReturnSeconds` for the honest number.

## 7. Performance budget (RTX 3070, 60 fps = 16.6 ms frame)

Figures for physics-owned work. Counts match research 03 section 1.2 caps. Costs are design targets to verify with `stat Physics`,
`stat Chaos` and the Chaos Visual Debugger; none are measured yet.

| Item | Cap | Budget (game thread unless noted) | Notes |
|---|---|---|---|
| Chaos vehicles with full solver | 12 (6 inside 12 m) | 0.12 ms each, 1.5 ms total | beyond cap demote to kinematic splines |
| Wheel friction weather update | per vehicle | 2 updates/s, negligible | |
| Motorbike lean controller | per bike | < 0.01 ms | one torque per tick |
| Boat buoyancy | 1 hull, 8 points | < 0.02 ms | |
| Active ragdolls | 8 (6 + 2 settling) | 0.15 ms each, 1.2 ms total | oldest sleeps |
| Partial physical animation | 8 at once | 0.05 ms each | |
| Ballistics trace | 1 shot | up to 25 segment traces + 1 thickness trace per impact, < 0.3 ms | single shots only, never per-frame bursts |
| Fall tracker | per character with the component | < 0.005 ms | add only to player and hero NPCs |
| Railing evaluation | on shove only | negligible | |
| Awake rigid bodies (all systems) | 250 | 2.0 ms physics thread | |
| Total physics budget | | 4.0 ms of 16.6 ms | rest is render and AI |

Never simulate: static buildings, PCG litter, crowd agents beyond capsules near the player, debris beyond 40 m.

## 8. Integrator checklist

- `OldDelhi.Build.cs`: add `"ChaosVehicles"`, `"ChaosVehiclesCore"` to `PublicDependencyModuleNames` (`PhysicsCore` is present).
- `.uproject` plugins: `ChaosVehiclesPlugin` (already present per project notes).
- `DefaultEngine.ini`: lines in section 2.
- Run `setup_physics.py` after materials exist.
- Compile risk areas (verify on the first build; the 5.8 Chaos vehicle API drifts): `UChaosVehicleWheel` property names in
  `DelhiVehicleWheels.cpp`, `EngineSetup` / `TransmissionSetup` / `DifferentialSetup` members in
  `ADelhiVehiclePawn::SetupPowertrain`, `FChaosWheelSetup` fields in `AddWheel`.
- Assumption to confirm in play: Chaos honours a runtime change of `UChaosVehicleWheel::FrictionForceMultiplier`; if not, wet
  braking only shows in the stopping-distance estimate.
- Assumption to confirm: `CenterOfMassOverride` is relative to the mesh origin; check with `p.Chaos.DebugDraw.Enabled`.
- Async physics tick: the lean controller and boat forces run in the pre-physics actor tick. Move them to
  `AsyncPhysicsTickActor` after the first profile if the game thread is the bottleneck.
- Player controls for driving belong to the character/controls module: bind keyboard (W/S/A/D, Space handbrake), gamepad
  (RT throttle, LT brake, left stick steer, A handbrake) and call `SetDriveInput`.

## 9. Tests to run in the editor

1. Place `ADelhiSuvPawn` Blueprint child on a flat road, call `StartBrakeTest(80)`: expect 36 to 44 m dry.
2. Set weather to Monsoon (Wetness 1): expect about 50 m.
3. Balcony: two rails, `TryShove` with 160 cm/s toward the drop on 90 cm (goes over) and 110 cm (caught).
4. Fire `FireShot` with the 9 mm at a 23 cm brick wall (`Stopped`) and through a car door (`Penetrated`); roll the katta 1,000 times (about 125 misfires).
5. Spawn 12 characters and ragdoll them: only 8 stay active.
