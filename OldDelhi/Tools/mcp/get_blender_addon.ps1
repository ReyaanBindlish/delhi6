<#
.SYNOPSIS  Download the blender-mcp add-on (addon.py) into Tools\mcp for manual install in Blender.
#>
$dst = Join-Path $PSScriptRoot "blender_mcp_addon.py"
Invoke-WebRequest "https://raw.githubusercontent.com/ahujasid/blender-mcp/main/addon.py" -OutFile $dst -UseBasicParsing
Write-Host "Saved $dst. In Blender 5.2: Edit > Preferences > Add-ons > Install from Disk... > select it, enable 'Interface: Blender MCP'."
