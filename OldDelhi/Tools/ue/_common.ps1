# Common helpers dot-sourced by the other scripts.
$script:EngineRoot = if ($env:UE_ROOT) { $env:UE_ROOT } else { "C:\Program Files\Epic Games\UE_5.8" }
$script:ProjectDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$script:Uproject   = Join-Path $script:ProjectDir "OldDelhi.uproject"
$script:EditorExe  = Join-Path $script:EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$script:EditorCmd  = Join-Path $script:EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$script:BuildBat   = Join-Path $script:EngineRoot "Engine\Build\BatchFiles\Build.bat"
$script:LogDir     = Join-Path $script:ProjectDir "Saved\Logs\tools"
New-Item -ItemType Directory -Force $script:LogDir | Out-Null
function Assert-Path($p, $what) { if (-not (Test-Path $p)) { Write-Error "$what not found: $p"; exit 2 } }
