# Damage and Ballistics for High-Realism Games

## Why it matters

Gunfights are where realism-focused shooters are judged. A bullet that behaves like a bullet (travel time, drop, material-dependent penetration) and a body that reacts like a body (location-dependent harm, bleeding, incapacitation) create tension and make equipment choices matter. Done badly, the same systems produce opaque deaths and frustration. The goal is to simulate the variables the player can reason about and show them clearly through feedback.

## Core techniques

### Projectile simulation

Pick one of three models per weapon class. Hitscan is an instant ray: cheap and precise, fine for short range and competitive shooters. Simulated projectiles step position and velocity each tick, `v += g*dt - drag*v*|v|*dt`, then raycast from the old position to the new one so fast bullets cannot skip thin geometry. Hybrids use hitscan inside a short range and projectiles beyond it.

For drag, use a ballistic coefficient and a G1 or G7 drag table instead of a flat constant, so rifle rounds hold velocity better than pistol rounds. Add muzzle velocity per ammo type and gravity, and wind and zeroing if the game justifies it. Arma 3 and Escape from Tarkov simulate travel time and drop; Battlefield and Squad do the same for long-range fire. Run the simulation on a fixed step on the server and interpolate on the client. Networked versions still need lag compensation, which rewinds hit volumes to the shooter's perceived time.

### Hit detection

Use simplified hit volumes (capsules and boxes per body part) on a coarse skeleton, not the render mesh. Tag each volume with a body region and a material. Raycast against these first, then against world geometry that carries surface material data (steel, concrete, wood, glass, flesh).

### Penetration

Treat penetration as a budget. Each bullet has energy or a penetration value; each material subtracts an amount per unit thickness, scaled by impact angle. When the budget runs out the bullet stops. When it exits, spawn a reduced-damage, possibly deflected exit ray. Implementation notes:

- Find thickness by casting forward then backward through the collider to locate the exit point.
- Cap the number of penetrations per bullet to bound cost.
- Reduce damage in proportion to the budget spent.
- Add ricochet when the incidence angle is shallow and the material is hard.
- Mark thin or soft materials (drywall, doors, car panels) as penetrable and heavy cover as not.

Escape from Tarkov uses a probabilistic variant. Armor has a class and durability, ammo has a penetration value, and the chance of defeating armor depends on both and falls as the armor wears. Failed penetrations still damage the armor, and each layer a bullet passes reduces its damage. The community wiki documents these rules, which also shows the cost: players need guides and spreadsheets to reason about them.

### Wound and injury models

Choose a fidelity level. The simplest is a health bar with hitbox multipliers (head 2x to 4x, limbs lower). The next step is per-region health, as in Escape from Tarkov and the ACE3 medical system for Arma 3, where each limb and the torso hold their own hit points and a head or chest wound can kill outright. Beyond that, track blood volume, bleed rate, pain, shock and unconsciousness, with different wound types (bleeds, fractures, burns) applying different effects. Implement it as a small state per body part with a damage type, severity, bleed rate per second and a treatment that clears or reduces it. Keep bleed and pain as numbers the UI can show.

Damage should scale with energy and bullet type: full metal jacket penetrates, hollow points transfer more energy but penetrate less. Even a two-value model (damage, penetration) captures that trade-off and is what most shipped games use.

### Armor

Model armor as layers with coverage maps. Each layer holds a class, material, durability and per-zone coverage. Resolve a hit by finding which layer the ray enters, testing penetration, subtracting durability and passing the remainder to the next layer or the body. Add blunt-trauma transfer: a stopped round still hurts. Tarkov's modular plates mean a hit in the gap between plates bypasses the main protection, which makes coverage a real decision. Heavier armor should cost speed or stamina; that is the main design lever.

### Hit feedback

Realistic systems need loud feedback so damage reads. Combine distinct hit sounds for armor, flesh and kills; a directional damage indicator; a short camera kick; blood and impact decals chosen by surface material; and a visible flinch or limp on the target. Show penetration or ricochet results in the kill feed or killcam where the genre allows. Procedural hit reactions (additive animation or partial ragdoll at the hit bone) sell impact without custom animation per weapon.

## Notable examples

- Escape from Tarkov: armor class versus ammo penetration, per-limb health.
- Arma 3 with ACE3: travel time, ballistics, detailed medical system.
- Rainbow Six Siege: destructible surfaces and material-based penetration.
- Red Dead Redemption 2: location-sensitive hit reactions on people and animals.
- Sniper Elite series: X-ray kill cam that exposes internal damage.
- Squad: travel time and drop in team-based combat.

## Realism versus fun

Lethality is the main trade-off. Real rifle hits can end a fight in one shot, which makes games tense and fast but punishes latency and spawn luck. Many games lengthen time-to-kill with armor or tiered classes to keep fights readable. Hidden mechanics such as probabilistic penetration feel like random death unless the UI explains them. Detailed medical systems add depth in cooperative or milsim play but slow a deathmatch. Real recoil can feel unresponsive, so most games tune it as a learnable pattern. Aim for one clear rule per decision: players should be able to say why they died.

## Common pitfalls

- Bullets skipping thin walls because the step is too large and no swept ray is used.
- Penetration checks against the render mesh instead of simplified colliders, causing cost spikes.
- No cap on penetration count or ricochets.
- Server and client disagreeing on hit volumes, so shots that look right do not register.
- Hidden randomness with no feedback, which players read as cheating.
- Over-detailed injuries that nobody can read in the heat of a fight.
- Ignoring surface material data, so every wall sounds and behaves alike.
- Balancing by realism alone; accurate stats can still be unfun.

## Sources

- Escape from Tarkov Wiki, Ballistics: https://escapefromtarkov.fandom.com/wiki/Ballistics
- Escape from Tarkov penetration guide: https://www.ggrecon.com/guides/escape-from-tarkov-penetration/
- Tarkov ballistics explained: https://escapefromtarkov.party/en/guides/ballistics-and-penetration-explained/
- Escape from Tarkov: https://en.wikipedia.org/wiki/Escape_from_Tarkov
- ACE3 medical system: https://ace3.acemod.org/wiki/feature/medical.html
- Ballistic coefficient and drag models: https://en.wikipedia.org/wiki/Ballistic_coefficient
- Glenn Fiedler, Fix Your Timestep: https://gafferongames.com/post/fix_your_timestep/
- Valve, Source Multiplayer Networking (lag compensation): https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking
- Rainbow Six Siege: https://en.wikipedia.org/wiki/Tom_Clancy%27s_Rainbow_Six_Siege
- Sniper Elite: https://en.wikipedia.org/wiki/Sniper_Elite
