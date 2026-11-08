# Audio Realism

## Why it matters

Players judge a space by ear before they judge it by eye. A believable game world needs sound that obeys the same rules as the geometry: a gunshot behind a wall is muffled, a shout in a stairwell rings, a distant explosion arrives late and dull. When audio ignores the level, the world feels like a set. When it follows the level, players locate threats, read room size and infer what is happening off screen. Realistic audio is also a gameplay channel, which is why stealth and tactical shooters invest in it most.

## Core techniques

### Spatialisation and HRTF

Panning alone gives left-right placement. To convey elevation and front-back, use binaural rendering with a head-related transfer function (HRTF), which filters each source by direction using measured ear and head responses. Steam Audio, Microsoft Spatial Sound, Wwise and FMOD all support this. Implementation notes:

- Spatialise only mono sources. Pre-mixed stereo loses directionality.
- Offer HRTF as an option for headphones and fall back to plain panning on speakers.
- Model distance with an inverse-distance curve plus air absorption (a low-pass that deepens with range), not volume alone.
- Apply Doppler only to fast sources such as vehicles and projectiles, and cap the shift.

### Occlusion, obstruction and propagation

Distance attenuation fails when a source is close but in another room. Two common models:

- **Room and portal graphs.** Treat rooms as nodes and doors or windows as portals. Sound travels along the shortest portal path, and the source is re-positioned at the last portal so it appears to come from the doorway. Hitman's system works this way and had to fit inside roughly one millisecond of main-thread audio time (Boev, GDC).
- **Raycast or geometry-based.** Cast rays from the listener to the source and, on a hit, apply a low-pass filter and gain reduction by material. Steam Audio adds baked or real-time propagation through actual geometry, including diffraction around corners.

Separate three cases: occlusion (no direct path, sound is muffled), obstruction (direct path blocked but the room is shared, so reverb stays clean) and exclusion (path partly open). Give materials different transmission values, so a wooden door passes more than concrete. Smooth every parameter over 100 to 300 ms so filters do not click as the player moves.

### Reverb

Use a small set of reverb zones tied to room volume and surface type, and feed sources into them with per-source send levels. Key points:

- Early reflections carry room shape and size, and the late tail carries decay. Both matter for outdoor-to-indoor transitions.
- Blend between zones. Hard switches at trigger volumes are audible.
- Outdoors needs slap-back echo from nearby buildings or trees, not a generic hall.
- Reverb sends should drop with occlusion so a muffled source does not also sound wet and close.

### Dynamic mixing

Real loudness spans a huge range, but speakers and living rooms do not. Two proven approaches:

- **HDR audio.** DICE's Frostbite mixer assigns sounds real-world loudness values and slides a window of audibility over them. A loud explosion pushes quieter sounds below the window, so they are culled for free. Battlefield: Bad Company used it to make big moments feel big without a hand-tuned ducking matrix (Clerwall, GDC 2009).
- **State-driven mixing.** Naughty Dog's The Last of Us drove dynamic range from tension and game state with its "Parametric Dynamic Range" approach instead of loudness-based HDR, partly because propagation made the loudness model hard to apply (Lanier, GDC 2013).

Add ducking for dialogue, a combat-state snapshot and a short low-pass "ear ringing" after nearby explosions.

### Foley and variation

Footsteps, cloth, holster and prop sounds sell presence. Record per surface and per movement type (walk, sprint, crouch, land, turn), and drive playback from physics material queries on the foot. Use many round-robin variations, randomised pitch within a few cents, and layers (heel, toe, cloth). Pair weapon handling with matching foley so reloads and lifts are not silent.

### Gunfire and distance acoustics

A gunshot is not one sample at different volumes. Layer it by distance:

- **Close:** sharp transient, mechanical action, full spectrum.
- **Mid:** transient softens, high frequencies drop, environment tail appears.
- **Far:** mostly low-frequency thump and reflections, delayed by the speed of sound (about 3 ms per metre).

Crossfade between close, mid and distant layers by listener distance, and apply a separate tail layer by environment (forest, city canyon, interior, open field). Supersonic bullets produce a crack that precedes the muzzle report and a whiz when they pass nearby. Games that simulate this teach players to judge direction and range by ear.

## Notable examples

- **Hitman:** portal-based propagation across large crowded levels.
- **Battlefield series:** HDR mixing and heavy use of distance layers and suppression effects.
- **The Last of Us:** state-driven dynamic range and strong obstruction handling in close quarters.
- **Thief: The Dark Project:** sounds propagate through a room database that the AI also hears, so audio is both feedback and mechanic.

## Realism versus fun

- True muffling can hide enemy footsteps. Many shooters exaggerate footsteps and push them above the physical mix, since competitive fairness beats accuracy.
- Realistic dynamic range makes dialogue unintelligible in a living room. Compress for the target listening environment and offer a night mode.
- Physically correct explosion delay can feel like lag. Shorten it for nearby events and keep it for far ones.
- Stealth needs readable, predictable sound rules, so the player can learn what is quiet.

## Common pitfalls

- Occlusion filters that change instantly and click or pump.
- Too many reverb zones with audible seams.
- Spending the CPU budget on propagation for sounds the player cannot notice. Cull by audibility first.
- Identical footsteps and gunshots repeating, which breaks immersion fast.
- Mixing on studio monitors only. Test on headphones, TV speakers and laptop speakers.
- Accidental audio-AI links: if enemies hear what the player hears, test that the rules match.

## Sources

- Boev, Sound Propagation in Hitman (GDC): https://www.gdcvault.com/play/1022774/Sound-Propagation-in
- Hitman propagation slides: https://www.gdcvault.com/play/mediaProxy.php?sid=1022824
- Lanier, Aural Immersion: Audio Technology in The Last of Us (GDC 2013): https://www.gdcvault.com/play/1020444/Aural-Immersion-Audio-Technology-in
- The Mix in The Last of Us (Audio Gang interview): https://www.audiogang.org/the-mix-in-the-last-of-us/
- Clerwall, How HDR Audio Makes Battlefield: Bad Company Go BOOM (Frostbite): https://www.ea.com/frostbite/news/how-hdr-audio-makes-battlefield-bad-company-go-boom
- Clerwall HDR Audio slides (GDC 2009): https://media.gdcvault.com/gdc09/slides/GDC09_Clerwall_HDRAudio.pdf
- Building an AI Sensory System: Thief: The Dark Project: https://www.gamedeveloper.com/programming/building-an-ai-sensory-system-examining-the-design-of-i-thief-the-dark-project-i-
- Steam Audio documentation: https://valvesoftware.github.io/steam-audio/
