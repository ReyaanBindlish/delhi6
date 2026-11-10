<#
.SYNOPSIS  Take a HighResShot of a map by launching the game renderer windowed, then quitting.
           Needs a GPU session (not a headless service). Output: Saved\Screenshots\WindowsEditor (or WindowsNoEditor).
.EXAMPLE   .\screenshot.ps1 -Map /Game/Maps/Lyari -Width 1920 -Height 1080 -DelaySec 8
#>
param([Parameter(Mandatory)][string]$Map, [int]$Width = 1920, [int]$Height = 1080, [int]$DelaySec = 10, [string]$Extra = "")
. "$PSScriptRoot\_common.ps1"
Assert-Path $EditorExe "UnrealEditor.exe"
$shots = Join-Path $ProjectDir "Saved\Screenshots"
$before = Get-Date
# -game runs the map as a game inside the editor binary; ExecCmds executes after load. 'Delay' not a console cmd, so use a stat-free wait via t.MaxFPS-less sleep.
$exec = "HighResShot ${Width}x${Height}"
$a = @("`"$Uproject`"", $Map, "-game", "-windowed", "-ResX=$Width", "-ResY=$Height", "-nosplash", "-NoSound", "-ForceRes", "-ExecCmds=`"Sleep $DelaySec; $exec; Sleep 3; Quit`"")
if ($Extra) { $a += $Extra }
$p = Start-Process -FilePath $EditorExe -ArgumentList $a -PassThru
if (-not $p.WaitForExit(300000)) { $p.Kill(); Write-Warning "Killed after 5 min" }
$new = Get-ChildItem $shots -Recurse -Include *.png,*.jpg -ErrorAction SilentlyContinue | ?{ $_.LastWriteTime -ge $before } | sort LastWriteTime
if ($new) { $new | %{ Write-Host "Screenshot: $($_.FullName)" } } else { Write-Warning "No screenshot produced. If 'Sleep' is unsupported in ExecCmds on this build, use the MCP automation/screenshot tool instead (see docs\MCP_SETUP.md)." }
