# MCP setup: Claude Code driving Unreal Engine 5.8 and Blender 5.2

## 1. What UE 5.8 offers (summary)
Epic's "AI features, tools and plugins" page links to: Unreal MCP (drive the editor from MCP-compatible agents), PCG + LLM workflows via Unreal MCP, Semantic Search (AI mode in the Content Browser), Unreal Canvas. The public page is currently the 5.9 revision; the local 5.8.4 install ships the same plugin family under `Engine\Plugins\Experimental`:

| Plugin | Purpose |
|---|---|
| ModelContextProtocol ("Unreal MCP") | MCP server inside the editor (Streamable HTTP + SSE). Experimental, NoRedist. |
| ToolsetRegistry | Auto-enabled dependency; toolsets register tools here. |
| AllToolsets (Toolsets\AllToolsets) | Enables every toolset: Editor, Physics, PCG, Niagara, Sequencer, UMG, StateTree, GAS, GameplayTags, AutomationTest, LiveCoding, MetaHumanGenerator, SemanticSearch, SlateInspector, Plugin, ConfigSettings, etc. |
| MCPClientToolset | Lets the editor's own agent (AIAssistant / EDA) connect OUT to other local MCP servers (for example the Blender one). Optional. |
| AIAssistant | Editor-side assistant (experimental). Optional. |

Server facts (from the plugin source and Epic docs):
- Default endpoint `http://127.0.0.1:8000/mcp`, loopback only. Transports: Streamable HTTP and SSE. No stdio, no WebSocket.
- Settings: Editor Preferences > Agentic Workflows > Model Context Protocol (`UModelContextProtocolSettings`, stored in `EditorPerProjectUserSettings.ini`): `ServerUrlPath` (/mcp), `ServerPortNumber` (8000), `bAutoStartServer` (default false), `bEnableToolSearch` (default true).
- Tool Search mode: `tools/list` returns 3-4 meta-tools (`list_toolsets`, `describe_toolset(s)`, `describe_tools`, `call_tool`). The agent discovers and calls real tools through them. Set `bEnableToolSearch=False` to register every tool natively (large tool list).
- Console commands: `ModelContextProtocol.StartServer [port]`, `StopServer`, `RefreshTools`, `GenerateClientConfig <ClaudeCode|Cursor|VSCode|Gemini|Codex|All>`.
- Command line: `-ModelContextProtocolStartServer`, `-ModelContextProtocolPort=N`.
- Tool calls execute one at a time on the game thread: never send overlapping calls.

Security:
- No authentication. Any local process, or a browser page from an allowed Origin, can drive the editor (spawn, delete, run scripts). Keep it on 127.0.0.1 (`[HTTPServer.Listeners] DefaultBindAddress`), never port-forward it, and do not open untrusted web pages while it runs.
- Origin allow-list: localhost, 127.0.0.1, [::1] plus "Additional Allowed Origins".
- Commit work to version control before long agent sessions; the agent can delete assets.
- Blender MCP's `execute_blender_code` runs arbitrary Python in Blender on port 9876 (localhost). Same caution. Telemetry disabled via `DISABLE_TELEMETRY=true` in `.mcp.json`.

## 2. Step by step

1. Enable plugins in `OldDelhi.uproject` (the integrator applies this): ModelContextProtocol, AllToolsets, PythonScriptPlugin, EditorScriptingUtilities, plus optionally MCPClientToolset, AIAssistant. Or Edit > Plugins in the GUI. Restart.
2. Add to `OldDelhi\Config\DefaultEditorPerProjectUserSettings.ini` so the server starts with the editor:
   ```
   [/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]
   ServerUrlPath=/mcp
   ServerPortNumber=8000
   bAutoStartServer=True
   bEnableToolSearch=True
   ```
   Alternative with no ini: launch via `Tools\ue\open_editor.ps1` (passes `-ModelContextProtocolStartServer`).
3. Build: `powershell Tools\ue\build.ps1`. Open the editor (`open_editor.ps1`). In the editor console run `ModelContextProtocol.GenerateClientConfig ClaudeCode` to regenerate a config (installed builds write `.mcp.json` into the project root; the repo-level `C:\Users\Rayaan\Desktop\game\.mcp.json` already contains the same `unreal-mcp` entry).
4. Start Claude Code from `C:\Users\Rayaan\Desktop\game` so `.mcp.json` is picked up. Approve the project MCP servers when prompted; `/mcp` should show `unreal-mcp` connected (UE editor must be running) and `blender`.
5. Blender MCP (ahujasid/blender-mcp):
   - uv is installed with `winget install --id astral-sh.uv -e --silent` (done on this machine; open a NEW terminal for PATH). Script: `Tools\mcp\install_uv.ps1`.
   - Get the add-on: `powershell Tools\mcp\get_blender_addon.ps1` writes `Tools\mcp\blender_mcp_addon.py`. In Blender 5.2: Edit > Preferences > Add-ons > Install from Disk, enable it, then 3D viewport sidebar (N) > BlenderMCP > Connect to Claude (listens on localhost:9876).
   - Compatibility: the add-on targets Blender 3.0+, but 5.x is NOT verified by its author. If it fails to register on 5.2 (API removals), fall back to headless scripting: `blender.exe -b -P script.py`, which is how all generators in this repo already run. Treat MCP-to-Blender as a convenience, not a dependency.
   - Optional integrations in the add-on (PolyHaven, Sketchfab, Hyper3D) need their own keys; leave off.
6. Verify: `powershell Tools\ue\engine_status.ps1` (checks launcher list, editor exe, plugins, uv, ports).

## 3. Headless alternative (no GUI, no MCP)
`Tools\ue\run_python.ps1 -Script <file.py>` runs Unreal Python through `UnrealEditor-Cmd.exe -run=pythonscript`. Use this for batch import, level construction, NPC spawn data. MCP is for live tweaking with an open editor.

## 4. Other tools worth adding later
- Cesium for Unreal: editor-time reference overlay of Google Photorealistic 3D Tiles only (never ship or copy tiles; needs a Google Map Tiles API key and Cesium ion token).
- GeoReferencing plugin (already present) for lat/long to world transforms.
- Filesystem/GitHub MCP are unnecessary: Claude Code has these natively.
- Remote Control API plugin (HTTP 30010) as a fallback to read/set properties if MCP is unavailable.

## 5. Troubleshooting
- Port busy: `ModelContextProtocol.StartServer 8010` and edit the URL in `.mcp.json`.
- Claude Code shows unreal-mcp failed: the editor is not running or plugin not enabled; run `engine_status.ps1`.
- Tools list looks tiny: expected in Tool Search mode; ask the agent to call `list_toolsets`.
- Engine launcher list is empty (`LauncherInstalled.dat` has no entries): the engine is a manual/unregistered install; scripts use `C:\Program Files\Epic Games\UE_5.8` directly (override with `$env:UE_ROOT`).
