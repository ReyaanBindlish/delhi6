# Pipeline: map data to playable Old Delhi

```
OSM / Overture / DEM  ->  Blender generator  ->  FBX + manifests  ->  UE import scripts  ->  MCP live tweaks  ->  screenshots  ->  version control
```

## 1. Map data
- Sources: `world\story\source\` (OSM + Overture per location, Karachi DEM), `world\karachi\`, `world\delhi\scripts\` (terrain, tiles, cameras).
- Never copy Google Maps/Earth imagery or 3D tiles into the game. Cesium + Google Photorealistic 3D Tiles is an editor-time visual reference only (Cesium plugin, own API key, not committed, not packaged).
- Real layout comes from OSM footprints/roads/heights; look comes from our own PBR textures and materials; weather from `DelhiWeatherSubsystem` (and historical records for reference).

## 2. Blender generators (headless)
```
"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b -P world\mumbai\scripts\osm_to_blender.py -- <args>
C:\Users\Rayaan\Desktop\game\characters\.venv\Scripts\python.exe world\karachi\lyari_prep.py
```
Outputs per tile: building/road/ground FBX, UCX_ collision hulls, and a JSON/CSV manifest (positions, tile ids, building heights, material slots).

## 3. Unreal import (headless)
```
powershell OldDelhi\Tools\ue\run_python.ps1 -Script OldDelhi\Tools\ue\build_vertical_slice.py
```
Imports textures, builds the master material, imports FBX with collision, places lighting; reads manifests to spawn actors. Build C++ first with `Tools\ue\build.ps1` when sources changed.

## 4. Live iteration with MCP
Editor open (`open_editor.ps1`), Claude Code connected to `unreal-mcp`: spawn/move actors, edit materials, tweak PCG, check Sequencer, run automation tests. Blender open with the BlenderMCP add-on for model fixes, then re-export FBX and re-run the import script (idempotent: reimport replaces assets).

## 5. Verification
- `build.ps1` exit code 0 and log in `Saved\Logs\tools`.
- `run_python.ps1` log scan for Error/Traceback.
- `screenshot.ps1 -Map /Game/Maps/<map>` writes PNGs to `Saved\Screenshots`; compare with reference photos.
- Automation tests via the AutomationTest toolset or `UnrealEditor-Cmd.exe ... -ExecCmds="Automation RunTests OldDelhi; Quit"`.

## 6. Version control
Project is not currently a git repo. When initialised: track `Source`, `Config`, `Content` (Git LFS for uasset/fbx/png/tif), `Tools`, `docs`, `.mcp.json`; ignore `Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`. Commit before each long MCP session.

## Script index
| Script | Role |
|---|---|
| Tools\ue\engine_status.ps1 | install and MCP readiness check |
| Tools\ue\build.ps1 | UBT build OldDelhiEditor Win64 Development (-WaitMutex) |
| Tools\ue\run_python.ps1 | headless Unreal Python |
| Tools\ue\open_editor.ps1 | open GUI with MCP server |
| Tools\ue\screenshot.ps1 | HighResShot capture |
| Tools\mcp\install_uv.ps1, get_blender_addon.ps1 | Blender MCP prerequisites |
