# Controls: Daya (PC and gamepad)

Setup: run `Tools/ue/setup_input.py` once (UnrealEditor-Cmd, `-run=pythonscript`) to create `/Game/Input/IA_*` and `IMC_Default`.
Then set `ADhurandharGameMode` as the default game mode. Both keyboard+mouse and gamepad work at the same time; the last device used wins.
Gamepad glyphs below are Xbox / PlayStation. Unreal reports them as the same keys.

## Control table

| Action | Keyboard and mouse | Xbox | PlayStation | Notes |
|---|---|---|---|---|
| Move | W A S D | Left stick | Left stick | Stick tilt is analog: a light push walks, a full push jogs. Radial dead zone 0.22. |
| Look / turn | Mouse | Right stick | Right stick | Mouse 0.10 deg per count; stick 140 deg/s, dead zone 0.18. Multiplier `LookSensitivity`, option `bInvertLookY`. |
| Sprint | Hold Left Shift | Click L3 (toggle) | Click L3 (toggle) | Drains stamina. Locked after exhaustion until stamina recovers to 25%. A toggled sprint ends when you stop. |
| Hold to walk | Hold Left Alt | Hold LB | Hold L1 | 1.4 m/s. For crowds, for not being noticed. |
| Crouch (toggle) | C or Left Ctrl | B | Circle | Slower, lower camera, lower profile. |
| Interact | E | A | Cross | Aim at an object (crosshair); prompt shows from `UInteractableComponent`. Range 2.5 m. |
| Vault / mantle | Space | X | Square | Only obstacles 0.35 to 1.0 m high with room on top. Slow, costs stamina, can be refused. |
| Notebook | Tab or N | View | Share (touchpad area) | Opens the notebook UI; the world keeps running. |
| Shoulder swap | Q | Click R3 | Click R3 | Camera moves to the other shoulder. |
| First / third person | V | D-pad up | D-pad up | |
| Pause | Esc | Menu | Options | Press again to resume. |

There is no jump button.

## Speeds and numbers (all tunable on `ADayaCharacter`)

| Gait | Speed |
|---|---|
| Walk | 1.4 m/s |
| Jog (default) | 3.0 m/s |
| Sprint | 5.5 m/s, 10 stamina per second |
| Crouch | 1.1 m/s |

Final speed is the gait speed times `UDayaVitalsComponent::GetMoveSpeedMultiplier()` (a baton-struck knee halves it). Speed eases between gaits (450 cm/s per second), and the body turns at 380 deg/s, so he cannot spin on the spot.

## Design reasoning

The bible is explicit: Daya is a man, not a superhero. The control scheme enforces that by what it leaves out.

- **No jump, no double jump, no wall run, no slide.** The only way over an obstacle is the vault/mantle, which only works on things a fit man could climb (under 1 m), takes about half a second, costs stamina, and fails if he is winded or there is no room on top.
- **Stamina is the one meter.** Sprinting, vaulting (and later fighting and climbing) draw on the same pool, so every sprint is a decision. Max stamina shrinks with the disease stage, so the same controls feel heavier as the story goes on without any new button.
- **The body can refuse you.** During a cough fit `IsInputLocked()` freezes movement, interaction and vaulting for 2 to 4 seconds. The player does not get to decide this.
- **Falls are real.** `Landed()` reports fall height to `ApplyFall`: injury above 3 m, likely death above 6 m. There is no fall-damage immunity.
- **Walk is a first-class gait.** Hold-to-walk exists because blending into a crowd (suspicion system) is a core verb, not an afterthought.
- **Gamepad parity.** Every action is reachable on a pad without chords. Sprint is a click-toggle on pad because holding L3 while steering is awkward; it cancels itself when he stops, so it cannot run away with the player.
- **Camera.** Third-person over-the-shoulder with a shoulder swap for corners, and a first-person toggle for close work and reading rooms. The interact trace starts level with his eyes along the camera ray, so the crosshair is honest in third person.
- **Mouse/stick scaling lives in the mapping context**, not in code, so designers can retune feel in `IMC_Default` without recompiling.

## For other systems

- Bind HUD prompts to `ADayaCharacter::OnFocusedInteractableChanged` and read `UInteractableComponent::Prompt`.
- Missions bind `UInteractableComponent::OnInteracted`.
- Animation Blueprint reads `GetGait()`, velocity, `IsVaulting()`, `IsFirstPerson()`.
- UI binds `ADayaPlayerController::OnNotebookToggled` and `OnPauseToggled`.
