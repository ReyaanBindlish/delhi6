<#
.SYNOPSIS  Report UE install state, tool availability and MCP readiness.
#>
. "$PSScriptRoot\_common.ps1"
$dat = "C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat"
Write-Host "Engine root      : $EngineRoot"
Write-Host "UnrealEditor.exe : $(Test-Path $EditorExe)"
Write-Host "UnrealEditor-Cmd : $(Test-Path $EditorCmd)"
Write-Host "Build.bat        : $(Test-Path $BuildBat)"
if (Test-Path $dat) {
  $j = Get-Content $dat -Raw | ConvertFrom-Json
  if ($j.InstallationList.Count -eq 0) { Write-Host "Launcher list    : empty (source/manual install or download not registered yet)" }
  $j.InstallationList | %{ Write-Host "Launcher entry   : $($_.AppName) $($_.InstallLocation)" }
} else { Write-Host "Launcher list    : LauncherInstalled.dat missing" }
$ver = Join-Path $EngineRoot "Engine\Build\Build.version"
if (Test-Path $ver) { $v = Get-Content $ver -Raw | ConvertFrom-Json; Write-Host "Engine version   : $($v.MajorVersion).$($v.MinorVersion).$($v.PatchVersion)" }
foreach ($pl in "Experimental\ModelContextProtocol","Experimental\Toolsets\AllToolsets","Experimental\Toolsets\MCPClientToolset","Experimental\AIAssistant") {
  Write-Host ("Plugin {0,-45}: {1}" -f $pl, (Test-Path (Join-Path $EngineRoot "Engine\Plugins\$pl")))
}
Write-Host "uv / uvx         : $([bool](Get-Command uvx -ErrorAction SilentlyContinue))"
Write-Host "Blender 5.2      : $(Test-Path 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe')"
try { $r = Invoke-WebRequest -Uri "http://127.0.0.1:8000/mcp" -Method Get -TimeoutSec 2 -UseBasicParsing; Write-Host "UE MCP server    : reachable (HTTP $($r.StatusCode))" }
catch { if ($_.Exception.Response) { Write-Host "UE MCP server    : listening (HTTP $([int]$_.Exception.Response.StatusCode))" } else { Write-Host "UE MCP server    : not running" } }
try { $c = New-Object Net.Sockets.TcpClient; $c.Connect("127.0.0.1",9876); $c.Close(); Write-Host "Blender MCP addon: listening on 9876" } catch { Write-Host "Blender MCP addon: not listening (start Blender > N panel > BlenderMCP > Connect)" }
