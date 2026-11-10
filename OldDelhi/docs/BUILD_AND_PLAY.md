# Build and play (DHURANDHAR: THE GAME, world + player + NPCs)

Engine: Unreal Engine 5.8 at `C:\Program Files\Epic Games\UE_5.8`. Project: `OldDelhi\OldDelhi.uproject`, C++ module `OldDelhi`.
Missions are not in this build. This build gives you a world (two real-layout locations), Daya as a playable pawn, a 5000-agent crowd, physics, vehicles, weather and a sky.

## 1. Build the C++

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" OldDelhiEditor Win64 Development -Project="C:\Users\Rayaan\Desktop\game\OldDelhi\OldDelhi.uproject" -WaitMutex -NoHotReloadFromIDE
```

Or `Tools\ue\build.ps1`. Status at integration: this command succeeded with no errors and no warnings (OldDelhiEditor, Development, Win64).
Close the editor first if Live Coding is on, otherwise UBT refuses to link. If UBT reports "Application Control policy has blocked" on `OldDelhiModuleRules.dll`, that is Windows Smart App Control; build from the real project folder (it worked there) or allow it.

## 2. One-time editor setup (headless)

Close the editor GUI, then:

```powershell
C:\Users\Rayaan\Desktop\game\characters\.venv\Scripts\python.exe Tools\ue\setup_all.py
```

It runs, in order, each in its own `UnrealEditor-Cmd` process: `setup_input.py` (Enhanced Input assets), `setup_characters.py` (DT_Cast, DA_Cast, MetaHuman Character assets), `setup_crowd.py` (archetype DataTables, mannequin copy, tint materials, crowd config), `import_location.py gulbahar_lines`, `import_location.py nizamuddin` (levels, meshes, UCX collision, materials), `setup_physics.py` (PhysicalMaterials onto the MI_* instances), `setup_lighting.py` per level (sky rig, weather controller, boundary).
Useful flags: `--list`, `--only input,physics`, `--skip crowd`, `--limit 50` (import only 50 buildings per location: do this first to shake out Unreal Python API drift). Logs go to `Saved\Logs\setup_all\`.
None of the Unreal Python scripts has run in this editor build yet (only syntax-checked), so expect small property-name fixes on the first run; they log WARN lines instead of crashing where possible.

## 3. Open and play

1. Open `OldDelhi.uproject` (double click, or `Tools\ue\open_editor.ps1`). First open compiles shaders; allow several minutes.
2. Open `/Game/Maps/Gulbahar_Lines` (Karachi, Lyari side) or `/Game/Maps/Nizamuddin` (Delhi). Default map and editor start map are set to Gulbahar_Lines.
3. Press Play. `ADhurandharGameMode` is the global default game mode, so you spawn `ADayaCharacter` at the Player Start. If no start is placed, use the manifest's `player_starts` (street near centre; Platform 2/3 at Nizamuddin) and place a PlayerStart there.
4. Console (`~`) crowd tools: `dh.crowd.stats`, `dh.crowd.density`, `dh.crowd.location <id>`, `dh.crowd.hour`, `dh.crowd.warm`, `dh.crowd.pause`, `dh.crowd.disturb`.

Daya has no mesh until one is assigned. Make a Blueprint child `BP_Daya` of `ADayaCharacter`, assign a skeletal mesh (Manny from `/Game/Characters/Mannequins` or a MetaHuman) and an AnimBP that reads gait, velocity, vaulting, first person. Set it as the default pawn in a Blueprint game mode child if you want it by default.

## 4. Controls

Full table in `docs\CONTROLS.md`. Summary:

| Action | Keyboard and mouse | Gamepad |
|---|---|---|
| Move / look | WASD / mouse | Left stick / right stick |
| Sprint | Hold Shift | Click L3 (toggle) |
| Walk (slow) | Hold Alt | Hold LB / L1 |
| Crouch | C or Ctrl | B / Circle |
| Interact | E | A / Cross |
| Vault / mantle | Space | X / Square |
| Notebook | Tab or N | View |
| Shoulder swap | Q | Click R3 |
| First / third person | V | D-pad up |
| Pause | Esc | Menu / Options |

No jump. Gaits: walk 1.4, jog 3.0, sprint 5.5 m/s (stamina), crouch 1.1 m/s. Cough fits and injuries slow or lock input (DayaVitalsComponent).

## 5. MCP (so Claude can drive the editor)

See `docs\MCP_SETUP.md`. Short version: start the editor with `Tools\ue\open_editor.ps1` (starts the Model Context Protocol server on `http://127.0.0.1:8000/mcp`), start Claude Code from `C:\Users\Rayaan\Desktop\game` so `.mcp.json` is picked up. Headless Python: `Tools\ue\run_python.ps1 -Script <py>`. Keep the MCP server on loopback; it has no authentication.

## 6. Integration changes made

- `OldDelhi.uproject`: enabled EnhancedInput, MassGameplay, MassCrowd, MassAI; editor-only: MetaHumanCharacter, MoverExamples, ModelContextProtocol, AllToolsets.
- `OldDelhi.Build.cs`: `PublicIncludePaths.Add(ModuleDirectory)` (files include each other by `Player/...`, `Crowd/...`); modules EnhancedInput, DeveloperSettings, Json, ChaosVehicles, ChaosVehiclesCore, MassEntity, MassCore, MassCommon, MassSpawner, MassMovement, MassSimulation.
- `DefaultEngine.ini`: GlobalDefaultGameMode, default maps, the 13 PhysicalSurfaces (order must match `EDelhiSurface`), async physics 60 Hz, sky atmosphere/cloud/fog settings.
- `DefaultInput.ini`: Enhanced Input classes. `DefaultGame.ini`: DhCrowdSettings (5000 agents, 600 visible) and Crowd/Lanes staging.
- Code fixes after the first real compile: `URailingComponent::EvaluateOverRail` parameter shadowing a member (renamed `InPushEfficiency`); `bUsedAsAtmosphereSunLight` is `bAtmosphereSunLight` in 5.8; cloud material is a `TSoftObjectPtr` (`LoadSynchronous`); motorbike `StepLean` local `Mesh` hid a member; Mass entity handle include (`Mass/EntityHandle.h`); added `MassCore` module.

## 7. Known gaps and risks

- Unreal Python scripts (`setup_input`, `setup_characters`, `setup_crowd`, `setup_physics`, `setup_lighting`, `import_location`) are untested in the 5.8 editor. First run with `--limit 50`.
- `import_location.py` is the only source of `/Game/Maps/Gulbahar_Lines` and `/Game/Maps/Nizamuddin`; until it runs those default maps do not exist and the editor opens an empty level.
- Both imports use the same material set; the world agent's materials land in `/Game/Delhi/Materials` if present, else `/Game/World/Materials`. `setup_physics.py` expects the `/Game/Delhi/Materials` MI_* instances.
- Weather subsystem and crowd subsystem both write `UDelhiPopulationGovernor` fields; set `bDriveGovernorFromWeather=False` on the crowd if that fights with a mission system.
- Crowd lane files are tile-centred. If the imported level origin differs, set `LaneWorldOffsetCm` in DhCrowdSettings. Crowd default location is `karachi_lyari` (Lyari slice), so on the new Gulbahar level call `dh.crowd.location karachi_lyari` or add a lane file for the Gulbahar tile.
- Mass backend is off by default (`Backend=Pooled`); Mass needs a configured `DA_DhCrowdEntityConfig` and a visual mesh.
- Characters are mannequin or MetaHuman Character assets with face and groom authoring still manual (see `docs\CHARACTERS.md` section 5). Vehicles need skeletal meshes with the documented wheel bones.
- Not modelled: Nizamuddin station hall detail, rolling stock, electrification; Orangi and the other story locations are not imported yet.
- Google Maps / Earth imagery is never copied into the game. Cesium + Google Photorealistic 3D Tiles is an editor-time reference overlay only (`OLDDELHI_CESIUM_REF=1`, needs your Cesium ion token, plugin not enabled by default).
- Only the OldDelhiEditor target was built. The game target (`OldDelhi`) and packaging are untested.
