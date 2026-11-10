# 04 - Tech stack, AI control of Unreal, APIs, outsourcing, architecture

Research date: 10 Nov 2026. Scope: Phase 1 (Old Delhi vertical slice) from `implementation.md`.

**Confidence labels.** [V] = verified in a primary or near-primary source this session. [S] = secondary source (blog, forum, aggregator) only. [E] = my estimate or recall, not verified; check before spending money. Epic's own MCP setup page could not be fetched, so everything about Epic's MCP plugin is [S].

---

## 0. Corrections to existing docs

1. **Visual Studio version.** `implementation.md` says VS 2022. Epic's current docs now say "Use Visual Studio 2026 for general development", but the 5.8 compatibility row lists both VS 2022 "17.14 or later" and VS 2026 "18.0 or later" [V]. A forum report on 5.8.1 shows project generation failing with "Visual Studio 2022 x64 must be installed" when only VS 2026 is present, and the 5.8 release notes list Epic's build farm as VS 2022 17.14 / MSVC 14.44.35207 / Windows SDK 10.0.22621.0 [S]. **Recommendation: install VS 2022 17.14+ (the toolchain Epic builds 5.8 with).** Add VS 2026 later only if Epic fixes this.
2. **Windows SDK.** Epic says 10.0.22621.0 is the correct version for 5.8.0 builds; 10.0.26100 is inside the supported range, and the release notes that list 26100 predate a rollback [V, Epic reply on forum]. Epic's setup table: MSVC min 14.38, recommended 14.50; SDK min 10.0.22621.0, recommended 10.0.26100 or newer [V]. **Install both SDK 22621 and 26100; the engine defaults to 22621.**
3. **"~100 GB total"** in `implementation.md` step 1 is a fair planning number for engine + VS, but I could not verify Epic's install size. Budget 150 GB for the engine/VS/DDC and a separate fast drive for project + DDC growth (the DDC often grows larger than the source assets [S]).
4. Epic's 5.8 hardware page recommends Windows 11, 32 GB RAM, 8 GB VRAM, and says local compiles want 12-16 cores [V]. Your 3070 8 GB / 32 GB is at the recommended floor, not above it. Core count is unknown to me; if it is 8 cores or fewer, expect slow C++ builds and prefer Live Coding plus Blueprint-exposed C++.

---

## 1. Setup

### 1.1 Install plan (Windows 11)

| Step | Action | Notes |
|---|---|---|
| 1 | Install **Visual Studio 2022 Community** (17.14+), or only **Build Tools 2022** if you edit in VS Code | Build Tools is enough for UnrealBuildTool; VS Code + Claude Code is your editor. Full VS gives debugger and Unreal-aware IntelliSense, which helps for crash debugging. |
| 2 | Workloads: **Game development with C++**; **Desktop development with C++**; **.NET desktop development** | Epic's list [V]. Epic's page also lists ".NET Multi-platform App UI development"; skip unless the editor complains. |
| 3 | Under Game development with C++ tick: **C++ profiling tools, C++ AddressSanitizer, Windows 10/11 SDK, Unreal Engine installer** | Epic's list [V]. |
| 4 | Individual components: **MSVC v143 14.44 toolset** (VS 2022 17.14 latest x64/x86), **Windows 11 SDK 10.0.22621.0** and **10.0.26100.x**, **.NET Framework 4.8 targeting pack**, **C++ CMake tools**, **MSBuild** | Epic's table says .NET 10.0 is required for VS 2026 only [V]. |
| 5 | Install **Epic Games Launcher**, sign in (user does this), Library > Engine Versions > **5.8.4**. Pick only the Windows target platform; leave out Linux, Android, iOS, debug symbols, and templates you will not use. Add "Editor symbols for debugging" later only if a crash needs it. | Saves tens of GB. |
| 6 | Create the project on the large NVMe drive. Short path, e.g. `D:\dev\delhi6\OldDelhi` (UE has path-length issues over ~140 chars). | |

**Launcher vs source build.** Use the Launcher build. A source build (GitHub access, `Setup.bat`, `GenerateProjectFiles.bat`, several hours, and well over 200 GB) is only justified for engine patches, dedicated servers, or Unreal Horde. [E] Nothing in Phase 1 needs it. Revisit only if a 5.8 bug blocks you and Epic's fix is in `release` but not in a hotfix.

### 1.2 Project template and plugins

- **Template:** Games > **Blank**, C++, no Starter Content, Raytracing off, Target Windows/Desktop, Maximum quality. `implementation.md` says "Third Person C++"; Blank is cleaner because you will replace the pawn and game mode with your own, but Third Person is acceptable if you want a walking character on day one. Either way, name it `OldDelhi`.
- **Enable (from `implementation.md` §9 step 5):** GeoReferencing, PCG, ChaosVehicles, Chaos Cloth, Niagara, MassAI / MassCrowd / ZoneGraph, StateTree, Audio Gameplay Volumes, Soundscape, Interchange, plus **Python Editor Script Plugin** and **Editor Scripting Utilities** (needed for automation), and **Automation Driver / Functional Testing**.
- **Add for AI control (section 2):** Epic's `Unreal MCP` + `Toolset Registry` + `All Toolsets` [S]. Third-party: **Remote Control API** and Remote Control Web Interface.
- **Cesium for Unreal 2.28.0** adds UE 5.8 support (July 2026); it is also the last release supporting 5.5 [V]. Authoring-only per your policy. Do not ship it.
- **Disable at first:** hardware ray tracing, Substrate (unless you want it), Lumen hardware RT, Path Tracer, Chaos Destruction unless used. 8 GB VRAM is your constraint.
- Keep plugins in the `.uproject` list minimal; each one adds shader and DDC cost.

### 1.3 DDC and shader cache on an 8 GB GPU

- UE 5.4+ uses the **Zen storage server as the local DDC by default** (`AppSettingsDir/Zen/Data`), with automatic garbage collection [V]. The Japanese forum report says the local Zen data directory is machine-wide (`%LOCALAPPDATA%\UnrealEngine\Common\Zen\Data`) and can be wiped when different projects use different DataPaths [S].
- **Put the Zen data path on the fastest NVMe with 200+ GB free.** Env var reports are inconsistent (`UE-LocalDataCachePath` vs `UE-ZenDataPath`); [S] test it, because forum users disagree on whether the first one works. Verify by checking where `Zen\Data` grows after a cold start.
- **Shared DDC** is for teams or for a CI box. Epic's docs describe a Shared Zen Storage Server on your LAN, configured under `[StorageServers] Shared=(Host=..., Namespace=...)` in config [S]. For one developer plus one CI runner, run Zen on the dev machine and point the runner at it; otherwise skip.
- **Shader cost:** the first open of the vertical slice compiles tens of thousands of shaders. Mitigate: keep a small set of master materials with instances (already your plan), limit static switches, and do not duplicate materials per building. Never delete the DDC casually; re-warming costs hours.
- **VRAM discipline** (already in `implementation.md` §8): `r.Streaming.PoolSize 3000`, Nanite pool 512, VSM pages 4096, 2K textures. Add `r.Shaders.Optimize` default, and keep the editor viewport at 50-75% screen percentage while authoring.
- **PSO caching** for packaged builds: collect PSO bundle in a dedicated playthrough (`r.ShaderPipelineCache.*`), as step 11 of the pipeline doc states.

### 1.4 Version control

| Option | Cost | Fit |
|---|---|---|
| **Git + LFS on GitHub** (current repo) | GitHub Free/Pro include 10 GiB LFS storage and 10 GiB bandwidth; overage $0.07/GiB storage, $0.0875/GiB bandwidth; Team plan includes 250 GiB each [V] | Fine for code, docs, small content. A UE project plus FBX plus source Blender files will pass 10 GiB quickly. Budget ~$10-30/month at 100-300 GiB. LFS file locking works but is clunky. |
| **Perforce Helix Core** | Free for up to 5 users, 20 workspaces; some sources mention a 1 GB limit for that free tier, which looks wrong or old [S]; cloud-hosted is about $39/user/month [S] | The industry default for UE; real file locking and cheap huge-repo handling. Needs a host (home PC or a $20-40/month VPS with 500 GB). Claude Code has no first-class Perforce workflow; use `p4` CLI via shell. |
| **Diversion** | Indie free (up to 5 users, 100 GB storage), +$10 per extra 100 GB; paid tiers $25-35 per user/month [S, G2 listing; verify on diversion.dev]. Has a UE plugin on Fab. | Cloud-native, easiest for a tiny team with no server. Less ecosystem than Git/P4; Claude Code can't treat it as plain Git. |

**Recommendation:** stay on **Git + LFS**, but split storage: (a) `delhi6` repo holds code, config, `Content/` (LFS), docs, scripts; (b) **do not commit** raw source data (DEM tiles, OSM, Overture, Mapillary images, scans, Blender working files over ~200 MB) - keep those on an external drive or S3/Backblaze B2 with a manifest and checksum script. Move to **Perforce (free tier)** the moment Content/ exceeds ~50 GiB or a second artist joins; otherwise LFS bills and merge conflicts on `.umap` hurt.

**Ordering rule [S]:** commit `.gitignore` and `.gitattributes` before any assets, or large binaries are baked into history.

`.gitignore` essentials: `Binaries/ Intermediate/ Saved/ DerivedDataCache/ .vs/ *.sln *.suo *.VC.db .idea/ Build/Receipts/` and `Plugins/*/Binaries Plugins/*/Intermediate`. Keep `Saved/Config` out, but keep `Config/*.ini`. Upstream template: GitHub's `UnrealEngine.gitignore` and the community `MOZGIII/ue5-gitignore` [S].

`.gitattributes` (LFS): `*.uasset *.umap *.upk *.udk *.fbx *.blend *.png *.jpg *.tga *.exr *.hdr *.wav *.ogg *.mp3 *.mp4 *.psd *.abc *.usd* *.bin` with `filter=lfs diff=lfs merge=lfs -text`; add `*.uasset lockable`. Note: `implementation.md` LFS list omits `.wav`, `.exr`, `.tga`, `.blend1`; add them. Add `*.blend1` and `*.blend@` to `.gitignore`.

Turn on **One File Per Actor** (OFPA) with World Partition so level edits are per-actor files and diffs/locks are granular (already in the pipeline doc).

---

## 2. AI control of Unreal Editor and Blender

### 2.1 The four control surfaces

| Surface | What it is | Needs editor running? | Good for | Limits |
|---|---|---|---|---|
| **Epic's built-in MCP (UE 5.8, experimental)** | "Unreal MCP" plugin runs an MCP server inside the editor (HTTP, `127.0.0.1:8000/mcp`) with a Toolset Registry / "AllToolsets" plugin [S]. Toolsets reported: Scene, Actor, MaterialInstance, Object; also Blueprints, Niagara, Sequencer and automation tests per some sources [S]. Studios can register their own tools in C++ or Python [S]. | Yes | Official, no external server, extensible with your own tools | Epic labels it experimental/incomplete and not for remote use; no authentication; tool calls run serially on the game thread; new tools need an editor restart (Live Coding can't pick them up); read-back verification is limited [S]. Details unconfirmed because Epic's docs weren't reachable. |
| **Python Editor Script Plugin + Remote Execution** | Editor runs Python (`unreal` module). Remote Execution (Project Settings > Python > "Enable Remote Execution") lets an outside process send Python to the editor over a local protocol (UDP multicast discovery, TCP command channel) [V for the setting; ports from memory, verify] | Yes (or headless, below) | Everything scriptable: import FBX with the Interchange preset, create materials, set up PCG graphs, spawn actors, build levels, run asset validation | Python only reaches what Epic exposes; no C++ compile; Blueprint graph editing is limited |
| **Remote Control API (HTTP 30010 / WebSocket 30020)** | Plugin that exposes UObject properties and functions over HTTP/WS [V that it exists; ports from memory] | Yes | Property get/set, function calls, presets. Python via an exposed class: a forum thread reports `POST /remote/python/execute` returns 404 [S]. | Batch calls crash in some wrappers [S] |
| **Command line / headless** | `UnrealEditor-Cmd.exe Project.uproject -run=pythonscript -script=...` or `-ExecutePythonScript=...`; `-ExecCmds="Automation RunTests ...; Quit" -unattended -nullrhi`; `RunUAT.bat BuildCookRun`; `UnrealBuildTool` for C++ compile [commands E; verify against the 5.8 command-line reference] | No | Imports, cooks, tests, validation in CI; reproducible; no GUI to fight | No viewport; no rendering unless you drop `-nullrhi`; startup cost of 1-3 minutes per run |

### 2.2 MCP servers compared

| Server | How it works | Maturity (as of Nov 2026) | Install friction | Security |
|---|---|---|---|---|
| **Epic "Unreal MCP"** (built-in, 5.8 only) | In-editor MCP server; `ModelContextProtocol.GenerateClientConfig ClaudeCode` console command writes `.mcp.json` in the project root [S] | Experimental, first-party, will track engine; thin toolset today [S] | Low: enable plugins, restart, run command | Loopback only, no auth. Fine on a dev machine; never forward the port. |
| **chongdashu/unreal-mcp** | C++ plugin in the editor + external Python MCP server (TCP to plugin) | 2.1k stars, 349 forks, MIT; targets UE 5.5+; Python 3.12+; "production use not recommended"; actors, Blueprint creation/node graph, editor control [V from the README page]; last commit date unknown (one tracker said Apr 2025 [S]) | High: copy plugin, compile it against 5.8, install uv, edit config | Local TCP bridge, no auth. Likely needs porting to 5.8 APIs. |
| **flopperam/unreal-engine-mcp** | Python MCP server; adds high-level world-building tools (towns, castles, mazes in the README) [S] | Smaller, advertised for UE 5.5+ [S] | Medium: uv + Python | Same as above |
| **runreal/unreal-mcp** | npm package `@runreal/unreal-mcp`; uses Unreal **Python Remote Execution**, no plugin to compile [V from README summary] | v0.1.4, small usage (~61 weekly downloads) [S]; MIT | Lowest: enable Python plugin + Remote Execution, then `npx` | Executes arbitrary Python in the editor by design |
| **ChiR24/Unreal_mcp** (`unreal-engine-mcp-server` on npm) | TypeScript server + native C++ "Automation Bridge" plugin | Active, broad tool list claims [S] | Medium: plugin + Node | Review before use |
| **PyPI `unreal-engine-mcp` / `uemcp`** | Remote Control API + in-editor Python, "no C++ plugin to compile"; one says developed and tested on 5.8 [S] | Young, one-maintainer projects | Low | Arbitrary Python execution |
| **StraySpark "Unreal MCP Server"** (paid) | Commercial, claims 455-523 tools, bearer auth, catalog mode [S, vendor marketing] | Commercial | Fab/vendor install | Has auth; vendor-sourced claims are unverified |
| **Blender: ahujasid/blender-mcp** | Blender add-on opens a local socket; `uvx blender-mcp` is the MCP side [V from PyPI page] | Most popular Blender MCP; active | Low | `execute_blender_code` runs arbitrary Python in Blender; optional Sketchfab / Hyper3D features need keys; telemetry can be disabled with `DISABLE_TELEMETRY=true` [S] |

**Capabilities by task:**

| Task | Epic MCP | chongdashu | runreal / Python-based | Headless Python |
|---|---|---|---|---|
| Spawn / move actors | yes [S] | yes | yes (via Python) | yes |
| Edit material instances | yes [S] | limited | yes | yes |
| Create master materials / graphs | unclear | no | partly (Python material editing library) | partly |
| Run arbitrary Python | custom tool or Terminal plugin [S] | no | **yes** | **yes** |
| Blueprint graph edit | some [S] | **yes** | limited | limited |
| Compile C++ | no | no | no | **no, use UBT from shell** |
| Read logs / screenshots | limited | viewport only | via Python | via log files |

**Maturity verdict.** None of these is production-grade. The reliable core is *Python scripts that Claude writes into the repo and runs*, not a large MCP tool catalogue. MCP is for interactive "do this in the open editor" tasks.

### 2.3 Exact setup commands

Run from the repo root on Windows Claude Code (native). Prefer `--scope project` so `.mcp.json` is shared with the repo; do not commit secrets.

```powershell
# One-time prerequisite for Blender and some UE MCP servers: uv (not installed yet)
winget install --id astral-sh.uv -e

# 1. Blender (ahujasid)
claude mcp add --scope project blender -- uvx blender-mcp
#   In Blender: uvx blender-mcp install-addon   (or install addon.py via Preferences > Add-ons)
#   Enable the add-on, open the N panel > BlenderMCP > "Start MCP Server" (default localhost:9876)
#   Test on Blender 5.2 first: add-on compatibility with 5.x was not verified in this research.

# 2. Epic official Unreal MCP (UE 5.8 editor must be open)
#   Editor: Edit > Plugins > enable "Unreal MCP", "Toolset Registry", "All Toolsets" (+ optionally "Unreal Terminal"), restart.
#   Editor console: ModelContextProtocol.GenerateClientConfig ClaudeCode   (writes .mcp.json in project root)
#   Or manually, if the above isn't available / port differs:
claude mcp add --scope project --transport http unreal-epic http://127.0.0.1:8000/mcp

# 3. Python-remote-execution server (no C++ plugin). Needs Python Editor Script Plugin + Remote Execution enabled.
claude mcp add --scope project unreal-py -- cmd /c npx -y @runreal/unreal-mcp

# 4. chongdashu (only if you need Blueprint node-graph editing; plugin must be built for 5.8)
git clone https://github.com/chongdashu/unreal-mcp C:\tools\unreal-mcp
claude mcp add --scope project unreal-bp -- uv --directory C:\tools\unreal-mcp\Python run unreal_mcp_server.py

# Check
claude mcp list
```

Notes: (a) on native Windows, `npx` needs the `cmd /c` wrapper [E, standard Claude Code guidance]; (b) the Epic command and endpoint come from secondary sources - confirm with the plugin's own console output; (c) the run line for chongdashu is from the README's config example; (d) launch Claude Code from the project root so it reads the right `.mcp.json` (the top failure mode in the guides [S]).

### 2.4 Security rules

1. Every one of these tools lets the model run code on your machine (Python in editor/Blender). Use a dedicated working branch, commit before sessions, and do not point them at a project with credentials in it.
2. Bind to loopback only. Epic's plugin has no auth [S]. If you ever need remote control, use SSH tunnels, not an open port. runreal reports a fix of binding to `0.0.0.0` [S] - do not do that.
3. Keep API keys in environment variables, not in `.mcp.json`, `Config/*.ini`, or Blender files.
4. Treat tool output (asset names, level data, scraped web text) as untrusted: prompt injection can arrive through imported asset metadata.
5. Pin versions: engine 5.8.4, plugin commit, `uvx blender-mcp==x.y.z`. Re-test after every update.
6. Use Claude Code permission rules to require approval on `execute_*` / Python tools if you run unattended.

### 2.5 Recommended workflow

1. **Default to headless scripts in the repo.** Claude writes `Tools/ue/*.py` (import, material setup, PCG volume spawn, validation) and runs them with `UnrealEditor-Cmd -run=pythonscript`. They are reviewable, versioned, and CI-repeatable. This matches steps 6 and 10 of `implementation.md`.
2. **C++:** edit in VS Code, compile with UBT from the shell (`Engine\Build\BatchFiles\Build.bat OldDelhiEditor Win64 Development -Project=...`); close the editor first or use Live Coding for small changes. No MCP does compilation.
3. **Live editor + Python MCP (runreal or Epic's)** for exploratory work: "move this actor", "show me the material params", viewport screenshots. Treat results as drafts and then codify them in a script.
4. **Blender MCP** for modelling help and one-off fixes; **production generation stays in the existing Python generator scripts** (`world/karachi/scripts/lyari_*.py`), run headless with `blender -b -P script.py` [E]. Never rely on a socket session for building 500 buildings.
5. **Verification loop:** after each change, Claude runs the automation tests (section 5) and takes a screenshot via a headless/`HighResShot` command or the MCP; humans only review.

---

## 3. APIs worth using

### 3.1 Claude API for NPC dialogue and behaviour

Current prices, per million tokens (input / output / cache read) [V, Anthropic pricing page]:

| Model | Input | Output | Cache read | Use |
|---|---|---|---|---|
| Haiku 5.5 (prompts up to 100k) | $0.10 | $0.50 | $0.01 | Barks, ambient chatter, vendor patter, classification |
| Sonnet 5.5 | $2 | $10 | $0.10 | Named NPC conversations, quest dialogue |
| Opus 5.5 | $4 | $20 | $0.20 | Offline content authoring, not runtime |
| Sonnet 5 | $2 | $10 | $0.20 | Same standard price; the $3/$15 increase will not occur [V] |

Batch is about 50% off for offline generation (widely cited; verify on the page).

**Cost per hour of play (my arithmetic, [E] assumptions).** Per conversational turn: ~1,500 input tokens (1,200 of which are a cached persona + world + memory prefix), ~80 output tokens.

| Scenario | Model | Turns/hour | Cost/hour |
|---|---|---|---|
| Ambient barks and crowd lines | Haiku 5.5 | 200 | about $0.04 |
| Active conversations | Haiku 5.5 | 60 | about $0.01 |
| Active conversations | Sonnet 5.5 | 60 | about $0.09 (about $0.0015/turn with caching, about $0.004 without) |
| Heavy mixed (20% Sonnet, 80% Haiku, 300 turns) | mix | 300 | about $0.10-0.15 |

So **LLM text costs roughly $0.01-$0.15 per player-hour**. It is small; the real costs are TTS (section 3.2), latency (aim for <1.5 s first token, stream it), and the fact that a shipped game needs either (a) a backend that holds your API key, rate-limits per player, and bills the studio, or (b) a bring-your-own-key option. **Never embed the API key in the game build.** For Phase 1 (vertical slice, not shipped) call the API from a small local proxy or directly from a C++/Python editor tool.

Design notes: pre-generate and cache as much as possible (batch API, Opus-authored templates); keep the model in a state-machine "intent" role (StateTree selects behaviour, the LLM only picks from a closed set and writes surface text); make the game fully playable offline with authored fallback lines; log every call for tuning; add a strict content filter, especially for religious and political topics (see the fiction policy).

### 3.2 Speech

- **ElevenLabs.** API list prices from third-party summaries of its pricing page: Multilingual v2 and v3 about $0.10 per 1,000 characters; Flash/Turbo about $0.05 per 1,000 [S, Sep 2026]; subscription tiers from about $6 to $990/month [S]. Pricing is per character regardless of language [S]; Hindi support in v3 and Flash was not confirmed in search, so test it with a Delhi-accented Hindi/Urdu sample first. At 120 characters per line and 30 lines per hour: about $0.18-$0.36 per player-hour on Flash/v3. That is much more than the LLM. Mitigate by pre-rendering common lines and using TTS only for dynamic names/numbers.
- Your Claude session has an ElevenLabs connector that requires authorization; it is not usable until the user authorises it in claude.ai connector settings. Use it for scratch voices and previews only.
- **Alternatives to evaluate:** Sarvam, Cartesia, MiniMax, Gemini TTS (an Aug 2026 comparison exists: invideo.io [S]); Sarvam is India-focused and likely stronger on Hindi/Indic code-mixing [E].
- **Licensing:** AI voices need a commercial plan, and clone only voices you have written consent for. Consider the NO FAKES-style legal climate [S]; do not clone real Delhi personalities.
- **STT (player speech):** optional; skip in Phase 1.

### 3.3 Cesium ion

- Cesium for Unreal is free; 2.28.0 supports UE 5.8 [V]. Google Photorealistic 3D Tiles are included in Cesium ion with plan-dependent limits and no overage charges per a community answer [S]; I did not find 2026 prices, so check cesium.com/pricing.
- Per your own policy: **authoring overlay only, never shipped, never cached** (Google terms). Fine.
- Cesium World Terrain and OSM Buildings tilesets are separate Cesium ion assets with their own attribution.

### 3.4 Mapillary

- API v4 requires a developer-portal token (v3 client IDs don't work). Metadata uses a header token; tiles use the token in the query string [V].
- Licence: user content is CC BY-SA unless stated otherwise, attribution required, and share-alike applies [V]. Your doc already notes that derived textures inherit share-alike - this is a **serious constraint for shipped game textures**; use Mapillary as reference only (modelling, signage placement, colour sampling), not as directly baked textures in the shipped game. [E]: get a lawyer opinion.
- Delhi coverage was not verifiable here. Query a bbox: `GET https://graph.mapillary.com/images?bbox=77.215,28.645,77.245,28.665&fields=id,captured_at,geometry,thumb_2048_url` with `Authorization: OAuth <token>` [E, verify against the docs].

### 3.5 OSM and Overture

- You already hold Overture buildings + OSM. OSM data is ODbL (attribution; share-alike on derived *databases*, not on rendered games, generally [E]). Overture's buildings theme combines OSM and Google Open Buildings, so check the per-source licence in `sources`/`license` fields of each feature; keep that column in the GeoJSON and ship a credits screen. [E, confirm with Overture docs]
- Get new Overture releases via `overturemaps` CLI (pip). Not an install of an application; fine.

### 3.6 MetaHuman licence (2026)

- MetaHuman Creator, Animator and Mesh to MetaHuman are licensed under the **Unreal Engine EULA** for UE 5.6+; the older standalone MetaHuman EULA applies only to UE 5.5 and earlier [V, Epic EULA page]. They are free to use under the UE terms for products under $1M gross revenue; Epic says MetaHumans can be used with other engines, and a secondary source reports an Unreal seat licence is required for rendering outside UE above $1M [S - verify at unrealengine.com/license].
- Epic's EULA update lets developers choose whether their UE and MetaHuman projects train AI models [V].
- A UE forum thread raises an unresolved question about shipping MetaHuman Animator markerless-capture runtime models in a Steam game (Fab listing says free including commercial use under Fab Standard License, but the EULA interplay is unanswered) [S]. Ask Epic support before relying on it.
- For a South Asian crowd you must **customise** (skin tones, hair, clothing); stock presets skew Western. MetaHuman Crowd (5.8 Experimental) is already in your plan.

### 3.7 Fab / Megascans

- The full Megascans library is free again, and licence terms were updated to allow AI training [S, CG Channel / 80.lv]; a Nov 2026 note says Fab listings update by 12 Nov [S]. Epic says the Fab Standard License allows use in any engine/tool; assets downloaded earlier from Quixel/Bridge keep the old Epic Content License (UE only) [S].
- Fab licences: Personal vs Professional tiers. The studio is a company; buy **Professional** where there is a choice. **Track every asset's licence in a spreadsheet from day one** (already in the pipeline doc).
- I found no verified Indian street asset pack on Fab. Search Fab directly: "India", "Indian", "bazaar", "market", "slum", "rickshaw", "tuk-tuk", "street vendor", "Mumbai", "Delhi", "sari", "temple". Expect a few; most will be mid-quality. Plan on building your own Old Delhi kit from the generator.

---

## 4. Buy vs build vs outsource

**Price quality warning.** Search results here were mostly SEO pages, salary aggregators and marketplace listings. All price ranges below marked [E] are planning estimates; get 2-3 written quotes before spending.

### 4.1 Principle

| Category | Decision | Why |
|---|---|---|
| Procedural buildings, facades, signage, decals, PCG graphs, weather subsystem, boundary wall | **Build** (Claude + your scripts) | Core of the project; already underway; cheap |
| Generic filler: rocks, trash, tools, plants, pavement, kerb, generic props | **Buy** (Megascans, Fab) | Cheap and high quality |
| Hero landmarks (Jama Masjid, Red Fort Lahori Gate, Sis Ganj, Town Hall, Jain temple) | **Outsource or scan** | Accuracy and artistry matter; $1,500-$5,000+ each [E] |
| Vehicles (auto, e-rickshaw, cycle-rickshaw, DTC bus, scooter) | **Buy base + outsource modification** | Rigged and physics-ready is the expensive part |
| Characters | **MetaHuman + bought clothing + custom Indian garments** | MetaHuman is free; garments are the gap |
| Animation | **Buy / mocap (Rokoko Vision, Move One, Mixamo)** | Phase 1 only needs a locomotion set + ~20 interactions |
| Dialogue voice | **AI placeholder now; hire ~10 hero voices later** | Authenticity of Delhi Hindi/Urdu/Punjabi matters |
| Ambient sound | **Record yourself + licensed library** | Field recording in the real place is the most differentiating asset |
| QA | **Playtesters + CI**, pay for QA in Phase 2 | |
| Legal | **Lawyer review before release** (already in your doc) | Trademark/publicity risk |

### 4.2 Price references

| Item | Reference | Source |
|---|---|---|
| Ultra Dynamic Sky & Weather | about ₹4,700 (about $55) | `implementation.md` |
| Studio art outsourcing, India man-day | $135 low / $175 mid / $195 high | one undated Polycount comment [S, weak] |
| Indian 3D artist salary | average ₹22,945/month (Feb 2026, 27 reports); freelancer range ₹1.5-15.5 lakh/year | Indeed, AmbitionBox [S] |
| Freelance 3D artist hourly | about $15-40/hour for solid game artists in India | [E] |
| Hero building asset (photo-reference, Nanite-ready, 2-3 LODs/trims) | $400-1,500 for a standard shop-front/haveli; $2,000-6,000 for a monument | [E] |
| Vehicle, game-ready, rigged for Chaos | $500-2,000 each outsourced; $30-300 purchased as a static model | [E] |
| Photogrammetry / 3D scanning in Delhi | listings from ₹750/hour (handheld) to ₹10,000/hour (laser/heritage); project-based ₹2,499 listing | marketplace listings, IndiaMART-type [S] |
| Hindi voiceover, India | beginner ₹100-300/finished minute; intermediate ₹300-800; expert ₹800-2,500+; studio ₹2,000-8,000+/hour | AimIndia guide, promotional [S] |
| Hindi voice, international rate | about $50-150 per finished hour via training-data gigs; Fiverr $50-100/hour asking | Mercor listing and Fiverr [S] |
| Rokoko | Vision free; Starter free; Plus about $20/month, Pro about $50/month (2025 comparison); Smartsuit about $2,500 [S, unconfirmed] | |
| Move AI (Move One) | free tier with 30 credits; paid from about $16/month [S] | |
| ElevenLabs | see 3.2 | |
| GitHub LFS overage | $0.07/GiB storage; $0.0875/GiB bandwidth | [V] |

### 4.3 Prioritised shopping list, Phase 1 (vertical slice, 500 x 500 m)

Currency USD, rounded. "Lean" assumes you do almost everything yourself; "Recommended" buys the items that save the most calendar time.

| Priority | Item | Buy / build / outsource | Lean | Recommended |
|---|---|---|---|---|
| P0 | Unreal Engine 5.8.4, Visual Studio Community, Blender, RealityScan, Megascans | Free | $0 | $0 |
| P0 | Fast NVMe 2 TB (if the machine lacks a second fast drive) and external 4 TB backup | Buy | $200 | $350 |
| P0 | Git LFS storage (or move to Perforce free tier) | Buy | $5/mo | $25/mo |
| P0 | Field audio recorder (Zoom H5-class) + windshield; you record Chandni Chowk yourself | Buy | $250 | $350 |
| P0 | Phone gimbal + spare phone storage for the reference walk (step 12) | Buy | $100 | $150 |
| P1 | Ultra Dynamic Sky & Weather (behind your MPC) | Buy | $0 (skip) | $55 |
| P1 | Vehicle base models: auto-rickshaw, e-rickshaw, cycle-rickshaw, scooter, DTC bus | Buy | $300 | $800 |
| P1 | Vehicle rigging/mod to ChaosVehicles + Indian detailing (stickers, dents, bells, tarps) | Outsource | $0 (do it yourself) | $2,500 |
| P1 | Hero facades x 5 (modelled from your photos) | Outsource | $0 (use scans) | $4,000 |
| P1 | Hero scans of 3-5 facades with RealityScan from public vantage | Build (you, with phone) | $0 | $0 |
| P1 | MetaHuman set: 20 customised faces + Indian clothing pack (kurta, sari, topi, cap, vest, uniform) | Build + outsource clothing | $0 | $2,500 |
| P1 | Animation: Mixamo/Rokoko Vision + 1 month Move One + 20 market interactions | Buy | $50 | $400 |
| P2 | Hero-character voice: 5 voices x about 150 lines (Hindi/Urdu/Punjabi/English) | Outsource | $300 (Fiverr-grade) | $1,500 |
| P2 | AI voice for ambient dialogue (ElevenLabs plan) | Buy | $22/mo | $99/mo |
| P2 | Sound library: Sonniss GDC bundles (free), Freesound (CC0 only), one purchased Indian-ambience library | Buy | $0 | $300 |
| P2 | Signage art: Devanagari/Urdu/Gurmukhi calligraphy review (native reader) | Outsource | $100 | $400 |
| P2 | Playtesters (5 people x 2 sessions) | Pay in snacks or gift cards | $50 | $300 |
| P3 | Trademark clearance for the fictional brand kit, and a first legal read of the fiction policy | Outsource | $500 | $1,500 |
| P3 | Claude API + TTS during development | Usage | $100 | $300 |
| | **Phase 1 total, first six months** | | **about $1,800-2,500** | **about $13,000-16,000** |

The "Lean" column leaves out most outsourced art. Realistically, the single best-value outsourced item is the vehicle rig + modification pass and the hero facades, because they are visible all the time and hard to automate. Delhi photogrammetry services are a poor value for Phase 1: your own phone scans of public-vantage facades plus RealityScan cover it, and the ASI permissions for monument interiors are slow (SAHMAT portal).

### 4.4 Where to find talent (names are directions, not vetted)

- **3D/vehicle freelancers:** ArtStation job board, Polycount, Upwork/Contra filtering for India; ask for UE5 Nanite + Chaos vehicle samples. Ask for a paid 2-day test before a contract. Check outsourcing studios in Pune, Bengaluru and Hyderabad with AAA co-dev experience; their day rates are in the $135-195 range [S, weak].
- **Voice:** Hindi/Urdu/Punjabi talent via Mumbai/Delhi studios, Fiverr/Voices.com for low-stakes lines. Ask for a dialect sample (Old Delhi Urdu-inflected Hindi, Punjabi Chandni Chowk traders). Buyout contract for game use in all media; for any voice you plan to clone with AI, add an explicit AI-training clause.
- **Photogrammetry:** only if you need monument interiors; request itemised quotes and ASI permits.
- **QA:** friends and a Discord playtest group for Phase 1; professional QA vendors in Phase 2+.

---

## 5. Architecture for a small team plus AI

### 5.1 Repo and folder ownership

```
delhi6/
  OldDelhi/                  UE project
    Source/OldDelhi/         C++ (owned by Claude + you; code review required)
    Content/
      Core/                  Subsystems, GameModes, MPC_Weather, DataTables     (C++-backed; Claude)
      World/                 Maps, WP data, Data Layers, PCG graphs             (Claude + you)
      Environment/           Generated buildings (Gen/), hero buildings (Hero/) (Gen = machine-owned, never hand-edit)
      Materials/             Masters, instances, MPCs, physical materials       (you, with Claude)
      Characters/ Vehicles/ Audio/ VFX/ UI/
      ThirdParty/            Fab/Megascans; read-only; license log
    Config/                  DefaultEngine.ini, DefaultGame.ini
    Plugins/                 Project plugins
  Tools/
    ue/                      Python scripts run in the editor or headless
    blender/                 Generator scripts (from world/karachi/scripts/)
    ci/                      Workflow + build scripts
  world/                     Source data and manifests (large files not in Git)
  docs/
```

Rules:
- **Gen vs Hero:** `Content/Environment/Gen/**` is overwritten by scripts; `Hero/**` is human-owned. Never let both write to the same folder.
- One owner per file type: Claude owns `Source/` and `Tools/`; the human owns `Materials/` look-dev decisions and level dressing. Treat `.uasset` and `.umap` as non-mergeable; use LFS locks or OFPA.
- **Naming conventions** (Epic's style guide): `SM_`, `SK_`, `M_`, `MI_`, `T_`, `BP_`, `NS_`, `MS_` for MetaSounds, `DT_`, `DA_`. Enforce with an editor validator script and a CI check.
- **Asset policy file:** every external asset has a row in `docs/asset-licences.csv` (source, licence, URL, date, price).
- **CODEOWNERS** on GitHub for `Source/` and `Config/`.

### 5.2 C++ vs Blueprint

| Layer | Language |
|---|---|
| Subsystems (weather, boundary, time of day, save/load), data structures, ballistics, vehicles' physics hooks, anything ticking per-frame or per-agent, anything Claude must edit via text and test | **C++** |
| Mass/StateTree tasks and conditions | **C++** (StateTree tasks in C++ perform better and diff cleanly) |
| Level-specific scripting, UI layout (UMG), VFX hooks, one-off interactions, designer tuning of exposed properties | **Blueprint** (thin; derive from C++ base classes) |
| Data | DataTables/Data Assets, with CSV/JSON import by script |

Reason: Claude Code can read, diff, test and refactor C++ and Python; Blueprint graphs are binary assets and merge badly. Expose C++ with `UFUNCTION(BlueprintCallable)` and `UPROPERTY(EditAnywhere)` and keep Blueprint at the leaves. Avoid Blueprint nativization (removed). Use Live Coding for iteration and always do a full UBT build before commit.

### 5.3 Automated testing

- **Unit/integration (Automation framework):** `IMPLEMENT_SIMPLE_AUTOMATION_TEST` and `UE_AUTOMATION_TEST` in C++ for subsystems and data (weather presets within bounds, ballistics table complete, every Surface Type has a Physical Material). Run headless:
  `UnrealEditor-Cmd.exe OldDelhi.uproject -ExecCmds="Automation RunTests OldDelhi; Quit" -unattended -nullrhi -nosplash -log` [E, verify flags].
- **Functional tests (map-based `AFunctionalTest`)** for gameplay: boundary push-back, vehicle tip-over, pigeon scatter, rain sets wetness. Needs rendering off, or `-nullrhi` for logic-only tests.
- **Asset validation:** an Editor Validator (Data Validation plugin) + Python script that fails when a mesh lacks collision/UCX, a material lacks a Physical Material, texture > 2K (unless whitelisted), or a name breaks convention. Run in the headless step.
- **Performance regression:** a Gauntlet or `-ExecCmds` run of a flythrough with `stat unit`/CSV Profiler capture, failing if GPU ms or the game thread exceeds budget (section 8 of `implementation.md`). Gauntlet is Epic's framework for this; I found no verified CI recipe for it, so keep a simple CSV-profiler script first [E].
- **Python tests** (`pytest` with `-I`) for the generator: manifest schema, one FBX per building, UCX naming, lat/lon to UE-coordinate round trip.

### 5.4 CI with GitHub Actions on a self-hosted runner

- One Windows self-hosted runner (the dev PC or a spare tower with the same toolchain). Run it **as a console process, not a service**, to avoid PowerShell permission problems [S]. Cooking can fail on a self-hosted runner with Zen "already exists" errors [S, forum, no confirmed fix]; use a clean `-ddc=NoZenLocalFallback`/clean checkout if it appears, and keep the runner on a different Zen data directory than your interactive editor.
- Jobs: (1) lint + Python tests (fast, runs on GitHub-hosted); (2) compile Editor + Game (UBT, Development); (3) headless Automation tests; (4) asset validation; (5) nightly: `RunUAT.bat BuildCookRun -project=... -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive` and upload the artifact (only 90-day retention; consider B2/S3).
- Cache: the Zen DDC, and the `Intermediate/` of the engine plugin build; GitHub-hosted caches are too small for UE.
- Secrets: GitHub Actions secrets for API keys. Never print them.
- Do not run PR builds from forks on a self-hosted runner (arbitrary code execution risk).

### 5.5 Packaging

- Development builds for playtests; Shipping for releases. Use **Zen/IoStore + Oodle compression**, packaging with `-iostore -pak`; collect the PSO cache with a recorded playthrough; test on a fresh machine and a 6 GB GPU to find the floor.
- Distribution: itch.io for demos; Steam ($100 per app) for release (as in the pipeline doc). Put the credits screen (OSM/Overture/Mapillary/Cesium/Fab/Megascans attribution) in the build from the beginning.

---

## 6. Top actions this week

1. Install VS 2022 17.14+ (Game dev with C++, MSVC 14.44, SDK 22621 + 26100), the Epic Launcher and UE 5.8.4 (Windows only) on the biggest NVMe; put Zen DDC on that drive. Install `uv`. (Needs the user to sign in to Epic.)
2. Create the `OldDelhi` Blank C++ project, commit `.gitignore` + `.gitattributes` (with the extended LFS list) **before** any content; decide Perforce free tier vs Git LFS and set a 50 GiB trigger.
3. Enable Python Editor Script Plugin + Epic's Unreal MCP, run `ModelContextProtocol.GenerateClientConfig ClaudeCode`, and add Blender MCP and `unreal-py` (runreal); do a 30-minute smoke test (spawn a cube, import one FBX with the Interchange preset, screenshot). Then write the first `Tools/ue/import_slice.py` headless script.
4. Update the generator to one FBX per building + `manifest.json` (step 3 of `implementation.md`), and add the first Automation test and Python test to CI on the self-hosted runner.
5. Buy the field recorder and do the reference walk and sound capture in Chandni Chowk; start the asset-licence CSV; request 2-3 quotes for the vehicle rig pass and 5 hero facades.

## 7. Open items I could not verify

- Epic's own MCP plugin docs (name of every toolset, built-in Python tool, config file contents, auth).
- UE 5.8.4 install size and the headless command-line flags (`-run=pythonscript`, automation flags).
- Whether blender-mcp's add-on works unchanged on Blender 5.2.
- Hindi/Urdu/Punjabi support and 2026 prices on ElevenLabs; Cesium ion 2026 pricing; Mapillary coverage in Old Delhi.
- Real freelancer and studio rates in India for game assets, and any Fab India street packs.
- Whether VS 2026 alone builds 5.8.4 cleanly (one forum report says it does not).

## Sources

- [UE hardware and software specifications](https://dev.epicgames.com/documentation/unreal-engine/hardware-and-software-specifications-for-unreal-engine?lang=en-US)
- [Setting up Visual Studio for UE C++ (version table, workloads)](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine)
- [Forum: UE 5.8.0 Windows SDK (Epic reply)](https://forums.unrealengine.com/t/ue5-8-0-windowssdk/2739689)
- [Forum: 5.8.1 fails to build with Visual Studio 2026](https://forums.unrealengine.com/t/5-8-1-project-fails-to-build-with-visual-studio-2026/2743331)
- [Epic official MCP plugin vs third-party (StraySpark, vendor)](https://www.strayspark.studio/blog/epic-official-mcp-plugin-ue5-8-vs-third-party)
- [VP Land: UE 5.8 embeds an MCP server](https://www.vp-land.com/stories/unreal-engine-5-8-embeds-an-mcp-server-so-ai-agents-can-drive-the-editor)
- [Pixels Design: UE MCP and Claude Code explained (vendor)](https://www.pixelsdesign.it/blog/unreal-engine-mcp-claude-code-explained.html)
- [SEELES: UE 5.8 MCP official plugin setup](https://www.seeles.ai/resources/blogs/unreal-engine-5-8-mcp-official-plugin-setup-guide)
- [Forum: UE 5.8 MCP setup assistant plugin](https://forums.unrealengine.com/t/free-ue-5-8-plugin-guided-setup-for-epic-s-native-mcp-workflow/2832596)
- [chongdashu/unreal-mcp](https://github.com/chongdashu/unreal-mcp)
- [runreal/unreal-mcp](https://github.com/runreal/unreal-mcp)
- [ChiR24/Unreal_mcp](https://github.com/ChiR24/Unreal_mcp)
- [PyPI unreal-engine-mcp (Remote Control + Python)](https://pypi.org/project/unreal-engine-mcp/)
- [Remote Control for Unreal Engine](https://dev.epicgames.com/documentation/unreal-engine/remote-control-for-unreal-engine?lang=en-US)
- [blender-mcp on PyPI](https://pypi.org/project/blender-mcp)
- [Anthropic pricing](https://platform.claude.com/docs/en/about-claude/pricing)
- [MetaHuman licence](https://www.metahuman.com/license) and [MetaHuman EULA](https://www.unrealengine.com/en-US/eula/mhc)
- [Epic: devs can choose to train AI on projects](https://unrealengine.com/ue-devs-can-now-choose-to-train-ai-models-on-their-projects)
- [Forum: MetaHuman Animator runtime licensing question](https://forums.unrealengine.com/t/licensing-question-shipping-metahuman-animator-markerless-motion-capture-runtime-models-in-a-steam-game/2839692)
- [CG Channel: Megascans free again](https://www.cgchannel.com/?p=179945)
- [Cesium releases, July 2026 (UE 5.8 support)](https://cesium.com/blog/2026/07/01/cesium-releases-in-july-2026/)
- [Cesium: Google tiles pricing thread](https://community.cesium.com/t/google-tiles-api-no-longer-required-how-does-pricing-work/28017)
- [Mapillary terms](https://mapillary.com/legal) and [Mapillary API v4 intro](https://blog.mapillary.com/update/2021/06/23/getting-started-with-the-new-mapillary-api-v4.html)
- [ElevenLabs API pricing summary (ModelsLab)](https://modelslab.com/elevenlabs-api-pricing) and [TTS price comparison Aug 2026](https://invideo.io/blog/ai-voice-model-pricing/)
- [GitHub Git LFS billing](https://docs.github.com/en/billing/concepts/product-billing/git-lfs)
- [Perforce Helix Core free](https://perforce.com/products/helix-core/free-version-control)
- [Diversion indie pricing](https://www.diversion.dev/indie-pricing)
- [UE .gitignore guide (Anchorpoint)](https://anchorpoint.app/blog/how-to-set-up-a-gitignore-file-for-unreal-engine)
- [Using DDC in Unreal Engine](https://dev.epicgames.com/documentation/unreal-engine/using-derived-data-cache-in-unreal-engine?lang=en-US)
- [Build operations, cooking, packaging](https://dev.epicgames.com/documentation/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine?lang=en-US)
- [Forum: GitHub Actions self-hosted UE cook failure](https://forums.unrealengine.com/t/trying-to-build-with-github-actions-on-self-hosted-fails/2703167)
- [Polycount: India outsourcing man-day rates](https://polycount.com/discussion/comment/1904343)
- [Indeed India: 3D artist salaries](https://in.indeed.com/salaries/3d-artist-Salaries)
- [Rokoko pricing](https://www.rokoko.com/pricing) and [Move AI listing](https://aitoolsatlas.ai/tools/move-ai/pricing)
- [Delhi 3D scanning listings](https://3d.irpr.agency/locations/india/delhi/3d-scanning/)
- [Voice-over cost India (2026)](https://indianvoiceartist-com.pages.dev/2026/voice-over-cost-india/) and [Mercor Hindi voice listing](https://work.mercor.com/jobs/list_AAABnMCrtsSIatLMMLND8KTc/hindi-professional-voice-actor)
