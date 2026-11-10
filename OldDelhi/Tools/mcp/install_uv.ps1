<#
.SYNOPSIS  Install uv (provides uvx for the Blender MCP server) if missing.
#>
if (Get-Command uvx -ErrorAction SilentlyContinue) { Write-Host "uvx already installed"; exit 0 }
winget install --id astral-sh.uv -e --silent --accept-package-agreements --accept-source-agreements
Write-Host "Restart the terminal (PATH refresh), then run: uvx blender-mcp --help"
