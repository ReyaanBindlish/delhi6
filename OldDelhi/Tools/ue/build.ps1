<#
.SYNOPSIS  Build OldDelhiEditor (Win64 Development) with UnrealBuildTool.
.PARAMETER Target  Default OldDelhiEditor. Use OldDelhi for the game target.
.PARAMETER Configuration Default Development.
#>
param([string]$Target = "OldDelhiEditor", [string]$Configuration = "Development", [switch]$NoWaitMutex)
. "$PSScriptRoot\_common.ps1"
Assert-Path $BuildBat "Build.bat"; Assert-Path $Uproject "uproject"
$log = Join-Path $LogDir ("build_{0:yyyyMMdd_HHmmss}.log" -f (Get-Date))
$args = @($Target, "Win64", $Configuration, "-Project=`"$Uproject`"", "-NoHotReloadFromIDE")
if (-not $NoWaitMutex) { $args += "-WaitMutex" }
Write-Host "UBT: $BuildBat $($args -join ' ')"
& $BuildBat @args 2>&1 | Tee-Object -FilePath $log
$code = $LASTEXITCODE
Write-Host "Exit code $code. Log: $log"
exit $code
