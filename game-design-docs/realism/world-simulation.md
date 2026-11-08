# World Simulation for High-Realism Games

## Why it matters

A world feels real when it keeps going without the player. Rain arrives whether or not you are watching, a deer flees a wolf you never saw, a shopkeeper closes up at dusk. The player reads these small independent events as proof that the world has its own logic. Visual fidelity alone does not do this. A photoreal street with static, always-open shops feels like a film set, and a rough-looking game with believable cause and effect can feel more alive. This document covers five areas: weather and time, ecology, economies, systemic interaction, and persistence.

## Core techniques

### Weather and day/night

- Drive everything from one clock. Keep a single world-time value and derive sun angle, temperature, light colour, ambient sound, shop hours and NPC schedules from it. Anything that reads the clock stays consistent for free.
- Model weather as a state machine with weighted transitions (clear, overcast, rain, storm) rather than a random pick per frame. Add per-region climate tables so a desert and a swamp do not share odds. Blend states over minutes, and let the transition itself carry cues such as wind rising before rain.
- Make weather change rules, not just visuals. Rain should lower visibility, dampen noise, wet surfaces and raise slip chance; fog should shorten AI sight distance. If weather only changes the skybox, players learn to ignore it.
- Separate the sky from the simulation. Rockstar's atmospheric rendering for Red Dead Redemption 2 used physically based light transport for clouds, fog and ambient light, while giving artists direct controls so mood could still serve a mission (see the SIGGRAPH 2019 course). Plan both layers: a physical default and an authored override.
- Seasons are a longer clock layered over the day. Use them to gate resources, animal behaviour and travel difficulty.

### Ecology and wildlife

- Give each species a small set of needs (food, water, rest, safety) and a priority order, then let behaviour fall out of the highest unmet need. Predators, prey and scavengers sharing the same perception model produces chains the designer never scripted.
- Use perception, not omniscience: sight cones, hearing radius, smell with wind direction. Wind matters: a hunter who ignores it should lose the deer.
- Build the ecosystem in layers around the player. Rockstar's GDC 2020 talk on animating the animals of Red Dead Redemption 2 described the wildlife as a planned ecosystem that evolved from GTA V's animals, with much of the work in animation and reactions rather than population maths. Run full simulation only near the player; keep abstract population counts far away, then spawn consistent individuals as the player approaches.
- Cap and respawn by region and habitat, not by timer alone, so overhunting has a visible effect without leaving the area permanently empty.

### Economies

- Choose between a simulated and a managed economy. A simulated one tracks stock, production, demand and transport between towns and lets prices emerge. A managed one uses authored price curves with events layered on top. Most shipped games use the managed form and fake the rest.
- Make prices respond to player actions in both directions: dumping goods lowers the local price, and a closed road raises it. Add a decay so one trade does not distort the world forever.
- Anchor every faucet (loot, quests, production) to a sink (repairs, food, taxes, upkeep). Realism and balance both break when currency only enters the world.
- Keep NPC wealth finite when it is visible. A merchant with unlimited gold breaks the illusion on the first large sale.

### Systemic and emergent interaction

- Build a small number of properties (flammable, wet, conductive, heavy, noisy) and a small set of rules for how they interact. Breath of the Wild's chemistry engine is the standard example: fire spreads through grass, updrafts carry gliders, metal conducts lightning. Nintendo's GDC talk on the game's design describes consistent rules applied everywhere rather than one-off scripts.
- Prefer rules that apply to the player and NPCs equally. When enemies also slip on ice and burn in grass, players trust the world.
- Use NPC schedules as the cheapest strong systemic layer: sleep, work, eat, travel. Oblivion's Radiant AI promised much more than it shipped, but the schedule layer survived into later Bethesda games, and it is what players still remember (see the Radiant AI references below).

### Persistence and consequence

- Persist state that the player caused and could notice: killed NPCs, moved objects, burned buildings, opened doors, depleted resources. Do not persist what nobody can observe.
- Save by delta. Store only differences from the authored world, keyed by stable object IDs. This keeps save files small and lets you patch the base world without breaking saves.
- Let consequences arrive late and indirectly. A looted village starves; a murdered quest-giver closes a questline. Delayed, legible consequences read as realism; instant punishments read as a rules system.
- Dwarf Fortress is the extreme case: it simulates history, individual grudges and injuries, and treats the long-term record as content.

## Notable examples

- Red Dead Redemption 2: ecosystem, weather rendering, and animal reactions that feed into hunting and the economy.
- The Legend of Zelda: Breath of the Wild: material and element rules that generate unscripted solutions.
- The Elder Scrolls IV and V: NPC schedules and the Radiant systems.
- Dwarf Fortress: deep simulation of bodies, history and emotion.
- S.T.A.L.K.E.R.: Shadow of Chernobyl: the A-Life system simulates factions and creatures off-screen.

## Realism versus fun

Simulation depth is invisible unless it surfaces. A fully modelled economy the player cannot read is wasted effort, so give every system a way to be noticed: a rumour, a shop sign, a price change, a corpse. Weather and darkness that stop play (total night blindness, storms that block all travel) need an out, such as a light source, shelter or a skip. Persistent consequences should be recoverable when the player is exploring rather than being punished; offer a way to repair, rebuild or reconcile.

## Common pitfalls

- Simulating at full resolution everywhere. Use level-of-detail simulation and abstract the far field.
- Emergent chaos that breaks quests. Mark essential NPCs, or give quests fallbacks when someone dies.
- Many systems that do not talk to each other. Ten isolated systems feel less real than three connected ones.
- Unseeded randomness that makes bugs impossible to reproduce. Seed weather and ecology and log the seed.
- Over-promising. Radiant AI's pre-release claims set expectations the shipped game could not meet.
- Persistence without cleanup. Dropped items and bodies accumulate and hurt performance; expire what has no meaning.

## Sources

- Rockstar wildlife talk summary, GDC 2020: https://gdconf.com/article/learn-how-rockstar-breathed-life-into-the-wildlife-of-red-dead-redemption-2-at-gdc/
- Tatarchuk, "Creating the Atmospheric World of Red Dead Redemption 2", SIGGRAPH 2019 Advances in Real-Time Rendering: https://advances.realtimerendering.com/s2019/index.htm
- Radiant AI overview: https://en.wikipedia.org/wiki/Radiant_AI
- What was Radiant AI, anyway? https://blog.paavo.me/radiant-ai/
- Oblivion Radiant AI, UESP: https://en.uesp.net/wiki/Oblivion:Radiant
- Breath of the Wild design talk, GDC Vault: https://www.gdcvault.com/
- Dwarf Fortress: https://en.wikipedia.org/wiki/Dwarf_Fortress
- S.T.A.L.K.E.R. A-Life: https://en.wikipedia.org/wiki/S.T.A.L.K.E.R.:_Shadow_of_Chernobyl
