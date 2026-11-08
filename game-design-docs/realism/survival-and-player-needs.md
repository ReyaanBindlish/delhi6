# Survival and Player Needs

## Why it matters

Needs systems give the player a body. When a character can be cold, hungry, tired or hurt, every journey, shelter and meal has a cost, and the world becomes a set of problems to solve rather than a backdrop. The risk is that the same systems become chores. The craft is making needs create decisions and stories while staying out of the way the rest of the time. This document covers the main needs, health and injury, weight, scarcity, and how to keep it all from turning into tedium.

## Core techniques

### Hunger, thirst, fatigue, temperature

- Use continuous meters with thresholds, not just a bar. Each meter has bands (fine, warning, impaired, critical) and each band has a clear effect. Project Zomboid and The Long Dark both tie states to concrete penalties such as weaker combat, slower movement, or health drain.
- Tune drain rates against activity, not just the clock. Walking, sprinting, carrying weight and cold all raise calorie burn. The Long Dark ties calories to movement and load, so the player feels the cost of a long haul.
- Treat temperature as the most interesting need, because it ties into place, weather and clothing. Compute body temperature from air temperature, wind, wetness, clothing insulation and activity. Wetness plus wind should be worse than either alone. Give the player ways to read it: shivering, visible breath, UI hints.
- Make fatigue produce choices. Sleep needs a safe place and time, which pulls the player into the world's shelter and risk systems. Add guardrails against abuse; The Long Dark's hardest tier wakes the player once rested and refuses sleep until they are tired enough.
- Keep psychological states optional and clearly signalled. Project Zomboid's boredom and stress feed a mood spiral rather than killing the player, a gentler use of the same structure.

### Health and medical

- Separate damage from injury. Hit points tell you how close to death you are; injuries tell you why and what to do. Distinct wound types (bleeding, fracture, infection, burn), each with its own treatment, give the player a diagnostic puzzle.
- Make treatment a short, readable loop: notice, diagnose, apply the right item, wait. If treatment takes more than a few seconds of input, it must pay off in drama.
- Use time-based progression for untreated problems. Bleeding ticks, infection grows, a sprain slows you. Escalation gives a reason to act now and a reason to be careful earlier.
- Kingdom Come: Deliverance ties survival needs to a medieval setting: you must eat and sleep to stay healthy, and Warhorse described its later survival mode as a realism mode rather than a hardcore one. The point is a ruleset that supports the fiction, not just punishment.
- Death should teach. Project Zomboid's "this is how you died" approach works because the cause is traceable to a decision.

### Inventory weight and encumbrance

- Choose a model that matches the fiction: weight only (simple, intuitive), slots (readable, forces choices), or both (heavy but very physical). Weight with soft thresholds works well for realism: unencumbered, burdened, overloaded, immobile.
- Tie carrying to consequence, not a hard stop. Heavier packs raise stamina drain, slow movement and increase noise. The player trades capacity for speed.
- Show weight in context: per item and as a total against the limit, and warn before the threshold, not after. Some Long Dark players find its fatigue and overload messages intrusive, which shows how much delivery matters.
- Give players ways to offload: caches, vehicles, pack animals, storage at home. These create the loop of expedition and return.

### Resource scarcity

- Scarcity is a distribution, not a single number. Make common items plentiful, key items rare, and place them where the player must take risk or travel to get them.
- Make scarcity respond to the player. If the player is struggling, bias spawn tables slightly toward survival basics; if they are comfortable, tighten. Do it quietly so it does not feel like a trick.
- Offer multiple sources and substitutes. Water can come from rain, rivers, boiling or bottles; heat from a fire, clothing or shelter. Redundancy lets different play styles succeed.
- Spoilage, wear and consumption keep stockpiles from solving the game permanently, but give the player tools (salting, canning, repair) to fight decay.

## Keeping it from becoming tedious

- Make needs slower than attention. A meter that demands action every two minutes is a chore. Aim for an action every 10 to 20 minutes of play, grouped so one stop at camp covers several needs.
- Batch the actions. One cooking interaction that feeds and hydrates beats three button presses. Let the player queue, or auto-eat with a setting.
- Make failure gradual and forgiving early. Early warning bands let a player ignore a need for a while without punishment, which respects different moods.
- Offer difficulty as a menu of toggles, not one slider: drain rates, injury frequency, death penalties, UI hints. The Long Dark's difficulty tiers and Kingdom Come's optional survival mode follow this approach.
- Make the UI a quiet instrument. Show state through the character (animation, audio, screen effects) and keep numbers behind a key press.
- Reward maintenance with texture, not only safety. A cooked meal, a warm fire and a night's sleep should feel good, not just stop a bar from emptying.
- Remove dead time. Allow a skip or fast-forward for waiting, with the world's risks still running.

## Notable examples

- The Long Dark: temperature, calories, weight and fatigue in a tightly tuned loop with tiered difficulty.
- Project Zomboid: deep needs and injuries with permadeath; stress, boredom and fatigue add a mental layer.
- Kingdom Come: Deliverance: needs fit a grounded medieval RPG; wounds and sleep shape play.
- S.T.A.L.K.E.R. series: radiation, bleeding, weight and food in a hostile Zone.
- Red Dead Redemption 2: cores for health and stamina, with food and clothing tied to weather.

## Realism versus fun

Warhorse has said openly that a full simulation is not possible and that the team keeps balancing historical accuracy against fun. A good rule: simulate what creates decisions, abstract what creates only waiting. Eating matters; chewing does not. Getting cold matters; counting degrees does not. Make the realistic option the interesting one, and let the player opt into more harshness rather than forcing it. Warhorse has also said it will not soften difficulty to keep players who bounce off, which is a legitimate stance as long as the rules are legible.

## Common pitfalls

- Meters that only punish. If a need has no pleasure attached, it feels like a tax.
- Stacking penalties into a death spiral where one mistake cannot be recovered from.
- Unreadable causes. If the player cannot tell why they are losing health, they blame the game.
- Hard weight cutoffs that interrupt looting at the worst moment.
- Constant intrusive notifications that cannot be switched off.
- Scarcity that is really just low drop rates, producing grind instead of tension.
- Sleep or wait exploits that skip all danger.
- Balancing for the first hour only. Check pacing after ten hours, when stockpiles are full.

## Sources

- The Long Dark: https://en.wikipedia.org/wiki/The_Long_Dark
- The Long Dark survival guide (Steam): https://steamcommunity.com/sharedfiles/filedetails/?id=822680889
- Project Zomboid: https://en.wikipedia.org/wiki/Project_Zomboid
- Project Zomboid needs guide: https://gamers.wiki/en/games/project-zomboid/guides/project-zomboid-needs-hunger-thirst-fatigue-boredom
- "This Is How You Died" philosophy: https://medium.com/@kjacobss/the-this-is-how-you-died-philosophy-why-project-zomboid-redefines-survival-gaming-182f21075593
- Warhorse interview (Fextralife): https://fextralife.com/kingdom-come-deliverance-interview-with-warhorse-development-update-dlc-taking-chances-building-a-community/
- Kingdom Come: Deliverance DLC and survival mode (Red Bull): https://www.redbull.com/int-en/kingdom-come-deliverance-dlc
- Kingdom Come: Deliverance review (Shacknews): https://www.shacknews.com/article/103645/kingdom-come-deliverance-review-fantastically-frustrating
- Warhorse on difficulty (HappyGamer): https://happygamer.com/warhorse-wont-soften-kingdom-come-deliverance-difficulty-151724/
