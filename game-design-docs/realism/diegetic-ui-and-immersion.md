# Diegetic UI and Immersion

## Why it matters for realism

A realistic world breaks the moment a floating health bar appears over it. Players read the interface as a statement about the world: if the world tells you your health with a number in the corner, you are playing a game about a character, not inhabiting one. Diegetic and minimal interfaces move information into the fiction (a gauge on a gun, a wound on a body, a voice line) so the player learns about the world the way the character would. The goal is not to remove information. It is to deliver it through channels that the simulated body, the camera and the audio already own.

## Core techniques

**Classify every element first.** Sort each piece of information into one of four buckets: diegetic (exists in the world and the character can see it), spatial (placed in the world but only the player sees it, such as a route marker), meta (damage vignette, blood on the lens) and non-diegetic (a classic HUD). Decide per element which bucket is the minimum that keeps the game playable. Most shipped "no HUD" games end up using spatial and meta elements as well.

**Put state on the avatar.** In Dead Space, health is a light strip on the spine of Isaac's RIG suit and stasis is a gauge on the arm, with inventory and menus projected as holograms in the world. Dino Ignacio's GDC 2013 talk describes the intent as removing the "wall of safety" between player and game. Implementation notes: render the hologram as real geometry and shaders in the scene, not a screen overlay, and keep it readable at the camera distance the player actually holds. Encode state in coarse colour and fill level rather than precise numbers.

**Read state from the body and the weapon.** Show ammo with a physical counter or by letting the player check the magazine, as Half-Life: Alyx does when the character mutters how many magazines are left. Show damage with limping, breathing, screen-edge desaturation and audio muffling. Implement these as continuous functions of the health value (for example, low-pass filter cutoff and saturation mapped to health) so the change reads before the player consciously notices it.

**First-person body awareness.** Render a full body with correct feet and shadow for the player, not a floating pair of arms. Mirror's Edge and Metro Exodus both show hands and arms that enter the frame to hold items such as a map, a compass or a wrist-mounted device. Notes: drive the arm rig with inverse kinematics from the camera, keep the near clip plane tight so the body does not clip, and use a separate field of view for the viewmodel so weapons do not distort at wide world FOV.

**Camera feel.** Use small, motivated motion: head bob tied to stride, a slight lag on turning, landing impulse scaled by fall speed, and breathing sway that increases with exertion. Keep amplitudes low and always provide sliders. Avoid acceleration on the look axis; motion that the player did not input is what causes motion sickness. Keep the horizon stable and avoid roll except for brief, authored events. Provide a field-of-view slider and a way to disable bob and blur.

**Audio as interface.** Footstep material, weapon tails, a character's breathing and short spoken status lines carry information without drawing the eye. The Last of Us and Alyx both use dialogue barks for ammo or threat state. Budget the mix so these cues stay audible in loud combat.

**Haptics.** Use controller feedback for state the player should feel in the hands: trigger resistance for weapon pull, texture for surfaces, a pulse for heartbeat. Housemarque's Returnal uses a half-pull trigger click to separate aim from alt-fire, with procedural rain haptics synthesised at runtime so they could be tuned live. Team Asobi's Astro's Playroom began as a shooting-range prototype to test trigger effects and found that surface textures could be distinguished without looking. Keep a clear threshold in trigger travel, reserve full intensity for important moments, and let players disable or remap everything.

**VR.** Presence replaces most of the above. Put information on the body: Alyx has ammo and health on the back of the wrist, magazines come from over the shoulder, and hands collide with the world and can lock onto levers and knobs. Notes for implementation: render hands at one-to-one tracking but allow a visual offset when they contact geometry; hold a stable 90 Hz or better; use snap or optional smooth turning with comfort vignettes; place UI inside arm's reach, not fixed to the head, because head-locked elements feel pasted on and cause discomfort. Offer one-handed and seated modes.

## Notable examples

- Dead Space (Visceral, 2008): the reference for an entirely in-fiction interface, with a spatial floor locator added after the diegetic map failed navigation testing.
- Half-Life: Alyx (Valve, 2020): hand-based interaction, wrist-mounted state, audio ammo barks. Valve also reduced shotgun reload time by pulling two shells per grab after finding repeated reaches tiring.
- Metro 2033 and Exodus (4A Games): wrist watch for filter time, gas mask that fogs and cracks, a hand-held lighter and compass.
- Escape from Tarkov, Receiver and Hunt: Showdown: minimal HUDs where counting rounds and checking the chamber is part of play.
- Returnal and Astro's Playroom: haptics as information.

## Realism versus fun

Information that a real person would have but a game camera cannot supply is the central problem. A soldier knows their ammo through touch and proprioception. A player cannot, so a purely diegetic design must substitute something: a visible counter, a bark, a gesture to check. The cost is time and attention. Over-the-shoulder ammo in Alyx draws praise from many players and complaints of fatigue from others, which is why it needs to be fast and forgiving. The rule of thumb is that diegetic delivery is right when the act of checking is itself interesting or tense, and wrong when the player needs the information many times a minute.

## Common pitfalls

- Treating "no HUD" as a goal rather than a method. Players who cannot find information will open the pause menu, which is worse than a small indicator.
- Hologram or gauge colours that fail contrast tests. Pale blue on dark corridors is hard for low-vision players; supply high-contrast and scalable alternatives.
- Information that only exists in audio or only in colour. Provide a second channel for every critical state.
- Camera effects with no off switch: head bob, motion blur, lens dirt, film grain, chromatic aberration.
- Viewmodel clipping through walls, and arms that disappear during interactions, which breaks the sense of a body.
- Haptics at full intensity all the time, which removes the contrast that makes them readable.
- Testing only with developers who already know where everything is. Run cold playtests and watch where eyes go.

## Sources

- Dino Ignacio, Dead Space UI talk (GDC 2013), Game Developer: https://www.gamedeveloper.com/design/video-designing-i-dead-space-i-s-immersive-user-interface
- Dead Space UI design lessons for VR: https://medium.com/inbeta/dead-space-ui-design-lessons-for-vr-39aa9e976ca8
- The diegetic UI of Dead Space: https://www.iabdi.com/designblog/2022/3/18/h04cs7ub04vkmyfcs3t2krmfey5mc3
- Road to VR, VR Design Unpacked: Half-Life: Alyx: https://roadtovr.com/these-details-make-half-life-alyx-unlike-any-other-vr-game-inside-xr-design/2/
- Transform Interactive, What Half-Life: Alyx taught us: https://transforminteractive.com/what-half-life-alyx-taught-us/
- UploadVR, Half-Life: Alyx one-handed mode: https://www.uploadvr.com/half-life-alyx-one-hand/
- PlayStation Blog, How Housemarque created Returnal's DualSense effects: https://blog.playstation.com/2021/05/13/how-housemarque-created-returnals-immersive-dualsense-controller-effects/
- Engadget, Astro's Playroom and the DualSense: https://www.engadget.com/astros-playroom-nicolas-doucet-team-asobi-interview-143025371.html
- Nasty Rodent, the four-type diegetic framework: https://nastyrodent.com/diegetic-and-non-diegetic-ui/
