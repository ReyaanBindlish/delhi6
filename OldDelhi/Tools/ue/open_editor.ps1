<#
.SYNOPSIS  Open the UE editor GUI on the project, with the MCP server auto-started.
           (Run by the human; agents should not launch the GUI.)
#>
param([string]$Map = "", [int]$McpPort = 8000, [switch]$NoMcp)
. "$PSScriptRoot\_common.ps1"
Assert-Path $EditorExe "UnrealEditor.exe"
$a = @("`"$Uproject`"")
if ($Map) { $a += $Map }
if (-not $NoMcp) { $a += "-ModelContextProtocolStartServer"; $a += "-ModelContextProtocolPort=$McpPort" }
Start-Process -FilePath $EditorExe -ArgumentList $a
Write-Host "Editor starting. MCP: http://127.0.0.1:$McpPort/mcp (once plugins are enabled)"
