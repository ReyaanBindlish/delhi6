# Controls and Player Embodiment

## Why it matters for realism

Graphics make a world look real. Controls make it feel real. When a player must clear a jam, seat a magazine, shift a clutch or hold a lever, the act of doing it creates a sense of being the character. The same detail also builds a wall: every manual step is something a new player can fail at. The craft is in giving the player real agency over the actions that matter while hiding or assisting the ones that do not. Realism that nobody can learn is not realism the player experiences.

## Core techniques

**Map actions, not buttons.** Define the set of physical actions the character can do (raise, aim, fire, eject, insert, rack, release, brace) and give each a distinct input with a consistent gesture across weapons and vehicles. Escape from Tarkov separates removing a magazine, inserting a new one, chambering and checking the chamber, and tracks each magazine's rounds individually. Receiver goes further and makes each step of a handgun's cycle a separate key. Notes: keep an action state machine per tool (weapon, vehicle, device) with explicit states and legal transitions, so animation, audio and input stay in sync and you can cancel safely.

**Make the interaction interruptible.** A reload in a realistic game is a sequence of states, not a locked animation. Allow cancel at defined points, such as stopping a reload to fire a round already chambered. Use animation blending with notify events at the points where the magazine leaves and enters, and apply gameplay effects (ammo count change) at those events rather than at the start or end.

**Offer a tactical reload, not just an emergency one.** Tarkov keeps a partly used magazine and offers a faster drop-the-magazine path on double tap, which gives skilled players a shortcut while keeping the default faithful. Build a fast path and a careful path for any multi-step action.

**Weapon handling with weight.** Model aim sway that changes with stance and stamina, a visible time-to-aim, recoil that has a learnable pattern, and gun lowering near walls. Express weight through acceleration limits on the viewmodel, not through delayed input response. Input latency should stay minimal; the delay should come from the character's body, which the player can read and plan around.

**Vehicle controls.** Provide analogue steering and pedals with proper dead-zone and response curves, speed-sensitive steering reduction so a stick does not oversteer at speed, force feedback or haptics that convey grip loss, and cockpit switches that can be operated by mouse click or a button box. Flight sims such as DCS and Microsoft Flight Simulator expose clickable cockpits and per-aircraft checklists, with a configurable level of start-up automation.

**Gesture and hand-based input in VR.** Alyx makes hands the main interaction channel and gets a lot of capability from objects that behave as expected: hands collide, grab points snap, levers latch to the hand until released. Add generous grab radii, physically plausible two-handed holds with the off hand steering the weapon, and an option to hold-to-grab or toggle-grab.

**Teach through play, not tutorials.** Introduce one manual step at a time. Gate complexity with context: the first weapon has a simple cycle, later ones add chamber checks or jams. Use diegetic prompts such as a character demonstrating, a labelled control on the object, or a short contextual hint that appears on the first failed attempt and then retires.

## Assist and difficulty layers

Treat realism as a set of independent dials rather than one difficulty slider. Forza Motorsport splits assists into braking, throttle, steering, gear shifting, traction control, stability control and driving line, and every one can be set individually. Its support documentation says the Blind Driving Assists were never intended as an easy mode, which is the right framing: assists change how the player is helped, not how hard the target is.

A useful structure is three layers.

1. **Input assists:** one-touch or auto shifting, assisted throttle, aim assist, auto-reload on empty, toggle versus hold for any sustained input.
2. **Information assists:** driving line, reload prompts, ammo counts, threat markers, subtitles and audio descriptions, all off by default in the purist preset.
3. **Consequence settings:** damage taken, jam frequency, stamina drain, mistakes' cost.

Ship named presets (for example Arcade, Standard, Simulation) built from these dials, and let players edit any value. Naughty Dog's The Last of Us Part II shipped more than 60 accessibility settings, with presets for vision, hearing and motor needs. Lead systems designer Matthew Gallant said players did not fully trust the studio to configure everything for them, so individual control mattered.

## Accessibility

- Full remapping for every input, including controller and keyboard, plus support for adaptive hardware. This is a baseline recommendation in the Game Accessibility Guidelines.
- A toggle alternative for every hold, and a hold alternative for every repeated press. Allow adjustable hold times and trigger thresholds.
- Avoid actions that require simultaneous inputs, quick-time presses or precise timing without a relaxed mode. Provide one-handed layouts for VR and standard play.
- Separate analogue sensitivity, dead-zone and curve controls for each stick and trigger.
- Allow haptics, gyro and adaptive triggers to be turned off or remapped. Astro Bot and Returnal both do.
- Audio-first assists for blind players, as in Forza's stereo-panned driving cues.
- Save settings per profile and show a preview of a change before the player confirms it.

## Notable examples

Escape from Tarkov, Receiver and Hunt: Showdown (weapon handling at differing levels of strictness); Arma and DCS (infantry and cockpit simulation with layered difficulty); Half-Life: Alyx (VR hands); Forza Motorsport and Gran Turismo (assist-first driving); The Last of Us Part II (granular accessibility); Returnal (trigger semantics).

## Realism versus fun

Steps are worth keeping when they create a decision or a risk. Chambering a round in Tarkov matters because you can be caught mid-action; Hunt's slow, period-accurate reloads make every shot a commitment. Steps are worth cutting when they only repeat: pressing a key five times to start a vehicle for the fiftieth time is friction, not realism. Offer a skip, and let a one-time first run of the sequence stay interactive. Also watch input volume: if a typical firefight asks for more than a few distinct actions per second under stress, new players will fail on dexterity and not on judgement.

## Common pitfalls

- Locking the character in an animation with no cancel, which feels like lag rather than weight.
- Realistic complexity with no feedback. Players cannot learn a jam or stall they cannot see, hear or feel; show state through sound, animation and haptics.
- One global difficulty slider that changes enemy health and weapon damage but not the controls, so players who struggle with the input layer get no relief.
- Hiding assists in nested menus, or labelling them as cheating, which discourages the players who need them.
- Default bindings that put two critical actions on adjacent keys, or that assume a particular keyboard layout.
- Inconsistent gestures between weapons and vehicles, forcing re-learning.
- Tuning only with experts. Test with players new to the genre and with players using adaptive hardware, and measure time to first successful reload or first clean lap.

## Sources

- Escape from Tarkov controls guide, GameSpot: https://www.gamespot.com/articles/escape-from-tarkov-controls-guide-hotkeys-and-keybindings/1100-6472601/
- Escape from Tarkov, Internet Movie Firearms Database: https://www.imfdb.org/wiki/Escape_from_Tarkov
- Road to VR, VR Design Unpacked: Half-Life: Alyx: https://roadtovr.com/these-details-make-half-life-alyx-unlike-any-other-vr-game-inside-xr-design/2/
- Transform Interactive, What Half-Life: Alyx taught us: https://transforminteractive.com/what-half-life-alyx-taught-us/
- Forza Motorsport Accessibility Support: https://support.forza.net/hc/en-us/articles/46524064744851-Forza-Motorsport-Accessibility-Support
- Game Rant, accessibility in Forza Motorsport: https://gamerant.com/forza-motorsport-accessibility-blind-driving-remappable-controls-settings-realistic-good-why/
- Forza Motorsport and the struggle between realism and accessibility: https://zunogame.com/blog/forza-motorsport-and-the-struggle-between-realism-and-accessibility
- PlayStation Blog, The Last of Us Part II accessibility features: https://blog.playstation.com/2020/06/09/the-last-of-us-part-ii-accessibility-features-detailed/
- NPR, The Last of Us Part II presents an accessible apocalypse: https://www.npr.org/2020/07/20/889820512/the-last-of-us-part-ii-presents-an-accessible-apocalypse
- Game Developer, Naughty Dog accessibility options: https://www.gamedeveloper.com/design/naughty-dog-ramps-up-accessibility-options-for-i-the-last-of-us-part-ii-i-
- PlayStation Blog, Returnal DualSense effects: https://blog.playstation.com/2021/05/13/how-housemarque-created-returnals-immersive-dualsense-controller-effects/
- Game Accessibility Guidelines: https://gameaccessibilityguidelines.com
