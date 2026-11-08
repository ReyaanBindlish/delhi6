# NPC AI and Behavior

## Why it matters

Players forgive plain graphics faster than characters who behave stupidly. Believable NPCs perceive the world through limited senses, choose actions for understandable reasons, keep lives that continue when the player is absent, and react to what the player does. The aim is not intelligence, it is legibility: the player should be able to form a theory of why an NPC acted, and that theory should hold.

## Core techniques

### Perception: sight

Model vision as a cone with range, plus line-of-sight checks to several points on the target (head, torso, feet). Accumulate detection over time instead of flipping instantly:

- Rate scales with distance, angle from view centre, target movement, lighting and cover exposure.
- Use levels of awareness, for example unaware, suspicious, searching, alerted. Thief: The Dark Project used vague acknowledgement, definite acknowledgement (triggers a search) and definite acquisition (attack).
- Peripheral vision should be wide but slow; central vision fast.
- Spread raycasts across frames and cache results, since sight checks dominate AI cost in crowded scenes.

### Perception: hearing

Treat sound as an event with a type, loudness, position and age. NPCs within an effective radius, adjusted by obstruction, receive it. Thief propagated tagged sounds through a per-mission room database, so noise passed through doorways and not through solid walls, arriving from a believable direction. Practical rules:

- Each action has a loudness (sprinting, landing, gunfire, breaking glass), modified by surface.
- NPCs investigate the last known position, not the true one.
- Store a short memory of stimuli. Forgetting should be gradual, with a decaying confidence value.

### Decision-making

Pick the architecture by how many behaviors you have and how they must blend.

- **Behavior trees.** Hierarchical, readable, designer-friendly. Bungie's Halo 2 AI (Isla, 2005) popularised them in shipped games. Good for authored, predictable behavior. Weak at weighing many competing options.
- **GOAP.** The F.E.A.R. AI (Orkin, GDC 2006) used three states (go to, animate, use smart object) and an A* planner that chains actions to meet a goal. Enemies flank, flip tables for cover and re-plan when the world changes, with little scripted choreography. Planning costs CPU and can be hard to debug, so keep the action set small and cap plan depth.
- **Utility AI.** Score each option with response curves over considerations (health, distance, ammo, threat) and choose the highest, or sample among the best. Dave Mark and Mike Lewis's Infinite Axis Utility System is the standard reference. It handles gradual trade-offs and produces varied, plausible behavior, at the cost of tuning curves.

A hybrid is common: utility for choosing a goal, a behavior tree or planner to carry it out, and a simple state machine for animation.

### Schedules and routines

Give each NPC a daily plan of timed activities tied to places: sleep, work, eat, socialise. Implementation:

- Represent the schedule as a list of (time window, activity, smart object or location). Let the world clock select the active entry.
- Make entries interruptible, with a priority, so an NPC can react to danger and then resume.
- Add jitter to start times so a town does not move in lockstep.
- For off-screen NPCs, fast-forward by placing them where the schedule says they should be when the player arrives, instead of simulating every step.

Red Dead Redemption 2 is the widely cited example: players who followed individual townspeople for a full day saw them work, eat, drink at the saloon and sleep on regular timings. Rockstar has not published a technical breakdown, so details come from player observation.

### Group and crowd behavior

- Use a token or slot system so only a few enemies attack at once. Others circle, take cover or call out.
- Assign roles (suppressor, flanker, rusher) and let squad logic coordinate, not individual AIs.
- For crowds, combine steering (separation, alignment, avoidance) with local rules, and fall back to cheap animation at distance. Hitman's crowds are a notable shipped example of dense, reactive crowds that notice the player's disguise.
- Share information with delay and loss: NPCs call out sightings, and nearby allies update their knowledge after a short lag.

### Reactive dialogue

Dialogue makes NPC state audible. Use a rules database that matches a query (who, what happened, where, world facts) against the most specific rule and plays the line. Add barks for seeing the player, losing them, taking damage and spotting a body, plus cooldowns so lines do not repeat. Keep dialogue consistent with perception: an NPC who says "I heard something" should actually have heard it.

## Notable examples

- **F.E.A.R.:** GOAP squad combat that appears tactical.
- **Halo 2:** behavior trees with readable encounter behavior.
- **Thief: The Dark Project:** sensory system built around sound and graded awareness.
- **Red Dead Redemption 2:** routines that make towns feel lived in.
- **Hitman:** crowds as stealth gameplay.

## Realism versus fun

- Perfectly perceptive AI is unwinnable, and perfectly blind AI is trivial. Tune detection to give the player time to react, and telegraph it with barks, icons or posture.
- Real tacticians do not announce plans, but games do, because players need to read them. F.E.A.R.'s squad dialogue partly exists for that reason.
- Enemies that always flank feel clever the first time and unfair the fifth. Limit how often and add visible cues.
- Fully simulated schedules are costly and rarely noticed. Spend effort where the player can see.
- AI should lose sometimes. Deliberately miss the first shots and leave the player an opening.

## Common pitfalls

- Instant detection, or detection through walls.
- Omniscient AI that knows the player's position after losing sight.
- Behaviors that thrash between options frame to frame. Add commitment time or hysteresis.
- Repeated barks and identical reactions.
- Crowds that freeze or jam in doorways because steering rules did not account for choke points.
- Planners and perception raycasts with no per-frame budget.
- No debug visualisation. Draw view cones, memory markers, scores and active plans from day one.

## Sources

- Orkin, Three States and a Plan: The A.I. of F.E.A.R. (GDC 2006): https://www.gamedevs.org/uploads/three-states-plan-ai-of-fear.pdf
- GDC 2026 fireside chat, Isla and Orkin on behavior trees and planning: https://schedule.gdconf.com/session/game-ai-fireside-chat-with-damian-isla-and-jeff-orkin-celebrating-20-years-of-behavior-trees-and-automated-planning-systems/917672
- Are Behavior Trees a Thing of the Past? (Game Developer): https://www.gamedeveloper.com/programming/are-behavior-trees-a-thing-of-the-past-
- Utility system overview: https://en.wikipedia.org/wiki/Utility_system
- Building an AI Sensory System: Thief: The Dark Project (Game Developer): https://www.gamedeveloper.com/programming/building-an-ai-sensory-system-examining-the-design-of-i-thief-the-dark-project-i-
- A day in the life of a Red Dead Redemption 2 NPC: https://rockstarintel.com/a-day-in-the-life-of-a-red-dead-redemption-2-npc/
- Red Dead Redemption 2 NPC routines (Dexerto): https://www.dexerto.com/red-dead-redemption/what-happens-when-you-follow-a-red-dead-redemption-2-npc-for-an-entire-day-215958/
- Game AI Pro (free chapters): https://www.gameaipro.com/
