# Animation and Character Motion for High-Realism Games

## Why it matters

Players forgive a lot in a rendered image but very little in how a body moves. People are tuned to detect wrong weight, wrong timing and wrong eye behaviour within a fraction of a second. A character with perfect skin that slides across the ground or turns instantly reads as fake, while a modest model with convincing weight and a good stop feels alive. Realistic motion has three jobs: carry weight and momentum, respond to the world (ground, obstacles, other characters), and stay responsive to the player. Most of the craft is balancing the first two against the third.

## Core techniques

### Motion capture

Mocap gives natural timing, secondary motion and posture that hand-keying struggles to match. Optical marker systems (Vicon, OptiTrack) are the standard for body work, with inertial suits for quick iteration.

Implementation notes:
- Capture to a standard skeleton and retarget in-engine. Keep a reference pose per actor.
- Record dense coverage for gameplay: starts, stops, turns at several angles, speed changes, and transitions between stances. Motion matching needs this breadth.
- Clean up foot contacts and mark them as events or curves. IK, locking and matching all depend on reliable contact data.
- Capture props with the performer (weapons, ladders, stairs of the right height) so contact points are real.
- Record reference video of every take. Cleanup goes faster with it.

### Motion matching

Instead of a hand-wired state machine, a motion matching system searches a database of mocap frames every few frames and plays the one whose pose, velocity and predicted trajectory best match what gameplay wants. Clavet's For Honor talk (GDC 2016) brought it to the industry. Gameplay code decides the desired trajectory and animation becomes a cosmetic layer on top, which keeps the character responsive while retaining mocap quality.

Implementation notes:
- Feature vector: positions and velocities of a few bones (feet, hips, sometimes hands or weapon) plus future trajectory samples (for example at 0.2, 0.4 and 0.6 seconds), normalised per feature so no term dominates.
- Search every 0.1-0.2 seconds, not every frame, and bias toward continuing the current clip to avoid jitter. Blend with inertialisation rather than cross-fade; it is cheaper and removes popping.
- Use acceleration structures (k-d tree, pruning) once the database passes a few thousand frames.
- Tag segments (locomotion, combat, injured) so gameplay context restricts the search.
- Learned Motion Matching (Holden et al., Ubisoft La Forge, 2020) replaces the search and decompression with small neural networks, which keeps memory flat as data grows.
- Unreal Engine ships Motion Matching with Pose Search, a reasonable base for prototypes.

### Inverse kinematics

IK adapts authored motion to the actual world.
- Foot IK: raycast or sphere-sweep under each foot, move the foot to the surface, rotate it to the normal, and lower the pelvis by the larger offset so the legs do not over-extend. Smooth with critically damped springs.
- Foot locking: pin the foot while the clip says it is in contact so it does not slide. For Honor locks the toe this way. Fade the lock out over a few frames at contact release.
- Hand IK: place hands on ladders, weapon grips, doors and ledges. Use two-bone IK with a pole vector, or full-body IK (Unreal's Control Rig) when the spine and shoulders should contribute.
- Look-at and aim: distribute rotation across eyes, head, neck and spine, in that order, with limits and lag so the eyes lead and the body follows.

### Procedural and physics-based animation

Pure playback looks canned when the world pushes back. Layer physics on top:
- Secondary motion: spring or verlet chains for hair, cloth, straps and soft tissue, and partial ragdoll blending on hits.
- Active ragdoll: Rockstar's Euphoria, used in Grand Theft Auto IV and Red Dead Redemption, drives joint motors toward target poses with behaviours (brace, grab, stumble), so falls are never identical.
- Learned physical control: DeepMimic (Peng et al.) and Ubisoft's DReCon train controllers to imitate mocap inside a physics simulation, giving characters that react to shoves and uneven ground while still looking captured.
- Additive hit reactions, leaning into acceleration, and procedural stride adaptation fill gaps that no clip covers.

### Facial animation

Faces need the most care. Use a blendshape or joint rig based on FACS action units, capture with head-mounted cameras (Hellblade, MetaHuman Animator) or per-frame 3D capture (L.A. Noire's MotionScan), and keep the performance in sync with the voice.

Implementation notes:
- Eyes matter most: saccades, blinks timed to head turns and thoughts, and a wet reflective cornea with a correct tear line.
- Layer procedural behaviour under authored performance: micro-saccades, breathing, idle blinks, brow and head noise.
- Drive lip sync from audio-to-viseme models for secondary dialogue, and from capture for hero scenes.
- Add wrinkle maps and blood-flow tinting driven by blendshape weights.

### Weight and momentum

Realistic weight comes from timing, not only pose.
- Anticipation and recovery: heavier actions need a longer wind-up and a slower return to neutral.
- Deceleration and turning: do not allow an instant stop or a 180-degree turn at speed. A character needs to plant, lean and step to stop.
- Centre of mass: keep the pelvis ahead of the feet when accelerating and behind when braking, and lean into turns.
- Impact feedback: pair heavy hits with camera shake, hit-stop of a few frames, and body compression, then let the body settle.
- Carry state through transitions. Inertialisation keeps velocity continuous when switching clips.

## Notable examples

- For Honor (Ubisoft): first shipped game to use motion matching.
- The Last of Us Part II and Uncharted 4 (Naughty Dog): dense mocap, careful IK, and hand-polished transitions and facial work.
- Red Dead Redemption 2: heavy, deliberate locomotion and Euphoria-based reactions; Arthur's weight is a design pillar.
- Grand Theft Auto IV: active ragdoll as the signature of the series.
- Hellblade: Senua's Sacrifice: real-time facial capture with Cubic Motion and 3Lateral.
- L.A. Noire: MotionScan face capture.
- The Matrix Awakens and MetaHuman: modern facial rig and animation tooling.

## Trade-offs

- Responsiveness against realism. Red Dead's deliberate movement feels grounded but sluggish to some players. Games that need fast reactions (fighters, shooters) cut anticipation and let the body catch up after input is accepted.
- Memory and authoring cost. Motion matching needs hours of well-covered mocap and a large database; learned variants save runtime memory at a training cost.
- Predictability. Physics-driven motion is harder to debug, and QA cannot enumerate every outcome. Limit it to reactions and keep authored fallbacks.
- Networked games. Animation should not drive hit detection, or must be deterministic enough to reconcile; many titles decouple a fast gameplay capsule from the visual pose.
- CPU budget. Crowds need animation LOD: lower update rates, and skip IK and facial layers at distance.

## Common pitfalls

- Foot sliding, caused by speed mismatch between locomotion clips and capsule velocity, or by missing contact locking.
- Floating or hovering after landing because pelvis drop and IK weight are not blended.
- Uncanny valley: near-real faces with dead eyes, unsynchronised blinks, locked jaws, or lip sync that ignores emotion. Mori's original curve says a movement mismatch can be worse than a stylised look. Either commit to full-performance capture or stylise.
- Over-smoothed trajectories that remove the weight cues you captured.
- Offset hands and jitter after retargeting between differently proportioned characters; fix with IK and per-character hand-offset curves.
- Pose popping in motion matching when the search switches clips without inertialisation.
- Physics fighting animation: a ragdoll blend that snaps to a get-up pose from a different position.

## Sources

- Clavet, Motion Matching and The Road to Next-Gen Animation (GDC 2016): https://www.gdcvault.com/play/1023280/Motion-Matching-and-The-Road
- Game Anim, Motion Matching in Ubisoft's For Honor: https://www.gameanim.com/2016/05/03/motion-matching-ubisofts-honor/
- Game Developer, Most Inspiring Game Animation Tech Talks of 2016: https://www.gamedeveloper.com/programming/most-inspiring-game-animation-tech-talks-of-2016
- Holden et al., Learned Motion Matching (SIGGRAPH 2020): https://theorangeduck.com/media/uploads/other_stuff/Learned_Motion_Matching.pdf
- Ubisoft La Forge, Learned Motion Matching: https://www.ubisoft.com/en-us/studio/laforge/news/3NWqwcMU9lMfumgfPAEfka/learned-motion-matching
- Ubisoft La Forge, DReCon: https://www.ubisoft.com/en-us/studio/laforge/news/VjEIwquaIyEZZSw5RZI0V/drecon-datadriven-responsive-control-of-physicsbased-characters
- Peng et al., DeepMimic: https://xbpeng.github.io/projects/DeepMimic/index.html
- Epic, Motion Matching in Unreal Engine: https://dev.epicgames.com/documentation/en-us/unreal-engine/motion-matching-in-unreal-engine
- Epic, Control Rig in Unreal Engine: https://dev.epicgames.com/documentation/en-us/unreal-engine/control-rig-in-unreal-engine
- Epic, MetaHuman: https://www.unrealengine.com/en-US/metahuman
- Euphoria (software): https://en.wikipedia.org/wiki/Euphoria_(software)
- Mori, The Uncanny Valley (IEEE Spectrum translation): https://spectrum.ieee.org/the-uncanny-valley
- Game Anim (Jonathan Cooper): https://www.gameanim.com/
