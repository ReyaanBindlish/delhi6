# Physics Simulation for High-Realism Games

## Why it matters

Players read realism through motion before they read it through graphics. A crate that slides like a polygon, a car that turns like a hovercraft or a wall that vanishes into a pre-baked puff of dust breaks the illusion faster than a low-resolution texture. Physics is also the layer players push on hardest, so it is where systems either hold up or visibly fail. The craft is less about simulating everything and more about choosing which quantities must be correct (mass, friction, momentum, contact) and faking the rest in ways that stay consistent.

## Core techniques

### Time stepping and determinism

Run physics at a fixed step (commonly 60 to 240 Hz), carry leftover frame time in an accumulator, and interpolate rendered poses between the last two physics states. This is the standard from Glenn Fiedler's "Fix Your Timestep" and removes frame-rate dependent behaviour, which is the root of many tunnelling and jitter bugs.

Determinism is a separate and harder goal. It means bit-identical results from identical inputs, and it is required for replays, lockstep multiplayer and reproducible bug reports. Floating point is the obstacle: compilers, instruction sets and debug versus release builds can all change results. Fiedler's lockstep write-up notes that same-machine determinism is achievable but cross-platform determinism has no silver bullet. Practical options are fixed-point or software-float math, a single pinned compiler and flags, fixed iteration order for contacts, no threading that reorders solver work, and seeded random numbers. If you do not need lockstep, prefer state synchronisation with snapshots and accept non-determinism.

### Rigid bodies

Use a sequential-impulse (projected Gauss-Seidel) solver with warm starting, 8 to 20 velocity iterations and a few position iterations. Stabilise stacking with Baumgarte or split impulses. Use continuous collision detection (swept shapes or speculative contacts) for fast bodies. Keep convex hulls or primitives for dynamic bodies and reserve triangle meshes for static geometry. Put sleeping thresholds on linear and angular velocity so resting piles cost nothing. Havok, PhysX and Bullet are the mainstream engines; Jolt is a modern open-source option.

### Soft bodies

Two practical families exist. Node-beam (mass-spring) models treat a structure as point masses linked by beams with stiffness, damping, and break or plastic-deformation thresholds. BeamNG.drive uses this: vehicles are defined in JBeam, a JSON-derived format of nodes and beams, and crash behaviour emerges from the structure instead of from animated damage states. Position-based dynamics (PBD) and its extended form XPBD trade physical accuracy for stability and are the usual choice for cloth, rope, soft tissue and deformable props, because they stay stable at large timesteps. Finite element methods are accurate but rarely affordable at runtime.

### Vehicle dynamics

Model the vehicle as a rigid chassis with suspension raycasts or shapecasts per wheel. The tyre is where realism lives. A linear friction cone feels like a shopping trolley; use a slip-based curve such as Pacejka's Magic Formula, with longitudinal slip ratio and lateral slip angle feeding a combined force that saturates. Add load sensitivity (grip does not scale linearly with vertical load), a drivetrain with engine torque curve, gearbox, clutch and differential, and aerodynamic downforce and drag. Simulation racers such as Assetto Corsa and iRacing push this further with thermal tyre models and wear. Forza Horizon keeps the same structure with forgiving assists layered on top.

### Destruction

Pre-fractured meshes (Voronoi pieces generated offline and held together by breakable constraints) are the cheapest and most controllable approach and appear in most shipped games. Voxel destruction, as in Teardown, makes every object fully dynamic at the cost of memory and a custom renderer. Terrain deformation goes back to Red Faction's Geo-Mod. DICE's Frostbite titles (Battlefield: Bad Company 2 onward) use scripted structural collapse of buildings with debris pieces. Whichever you choose, budget debris: cap active fragments, convert them to static or remove them after a timeout, and swap small pieces for particles.

### Fluid and cloth

Real-time fluids are mostly faked: heightfield water for oceans and ponds with buoyancy sampled at hull points, flow maps for rivers, and particle (SPH or FLIP-like) systems for contained splashes on high-end hardware. Cloth uses PBD or XPBD on a coarse mesh collided against capsule proxies of the character, then skinned onto the render mesh. Expect hundreds to a few thousand particles per object.

## Notable examples

- BeamNG.drive: node-beam soft-body vehicles with emergent crash damage.
- Teardown: voxel world where every object can be broken.
- Red Faction and Red Faction: Guerrilla: destructible terrain and structures.
- Battlefield: Bad Company 2 and later: building destruction in multiplayer.
- Assetto Corsa, iRacing, Forza Motorsport: tyre-model-driven driving.
- Half-Life 2: Havok-driven rigid bodies used as a gameplay tool.

## Realism versus fun

Full realism often hurts. Real cars are hard to drive on a gamepad, so most games add steering assists, speed-sensitive steering lock and stability control. Real destruction erases the level, so designers protect critical paths, limit debris and sometimes restore structure. Ragdolls that react to every hit look good but can make hit detection feel unfair, so many games blend animation and physics (active ragdoll). Physics props need tuning for interaction: slightly lower friction, higher damping and capped impulses stop objects from launching or jittering. Keep physics honest where the player feels it and quietly cheat everywhere else.

## Common pitfalls

- Variable timesteps causing tunnelling, exploding stacks and different behaviour per machine.
- Huge mass ratios (a heavy body resting on a light one) destabilising the solver.
- Using triangle meshes for dynamic bodies.
- Letting debris counts grow without caps, causing frame spikes.
- Tuning stiffness for one frame rate, then changing the step.
- Assuming float determinism across platforms.
- Tyre models with no saturation, giving unrealistic grip at the limit.
- Networking raw physics state without smoothing, producing visible snapping.

## Sources

- Glenn Fiedler, Fix Your Timestep: https://gafferongames.com/post/fix_your_timestep/
- Glenn Fiedler, Deterministic Lockstep: https://gafferongames.com/post/deterministic_lockstep/
- Glenn Fiedler, Floating Point Determinism: https://gafferongames.com/post/floating_point_determinism/
- BeamNG.drive (soft-body node-beam, JBeam): https://en.wikipedia.org/wiki/BeamNG.drive
- Erin Catto, Solver2D: https://box2d.org/posts/2024/02/solver2d/
- Macklin et al., XPBD: https://matthias-research.github.io/pages/publications/XPBD.pdf
- Pacejka Magic Formula: https://en.wikipedia.org/wiki/Hans_B._Pacejka
- Teardown: https://en.wikipedia.org/wiki/Teardown_(video_game)
- Red Faction (Geo-Mod): https://en.wikipedia.org/wiki/Red_Faction_(video_game)
- Battlefield: Bad Company 2: https://en.wikipedia.org/wiki/Battlefield:_Bad_Company_2
