<#
.SYNOPSIS  Run an Unreal Python script headless via UnrealEditor-Cmd (pythonscript commandlet).
.EXAMPLE   .\run_python.ps1 -Script ..\ue\build_vertical_slice.py -ScriptArgs "--map Lyari"
#>
param([Parameter(Mandatory)][string]$Script, [string]$ScriptArgs = "", [int]$TimeoutMin = 60)
. "$PSScriptRoot\_common.ps1"
Assert-Path $EditorCmd "UnrealEditor-Cmd.exe"; Assert-Path $Uproject "uproject"
$Script = (Resolve-Path $Script).Path
$log = Join-Path $LogDir ("python_{0}_{1:yyyyMMdd_HHmmss}.log" -f [IO.Path]::GetFileNameWithoutExtension($Script), (Get-Date))
$scriptArg = "`"$Script`""; if ($ScriptArgs) { $scriptArg = "`"$Script $ScriptArgs`"" }
$a = @("`"$Uproject`"", "-run=pythonscript", "-script=$scriptArg", "-unattended", "-nop4", "-nosplash", "-NoSound", "-stdout", "-FullStdOutLogOutput", "-abslog=`"$log`"")
Write-Host "$EditorCmd $($a -join ' ')"
$p = Start-Process -FilePath $EditorCmd -ArgumentList $a -NoNewWindow -PassThru
if (-not $p.WaitForExit($TimeoutMin * 60000)) { $p.Kill(); Write-Error "Timed out after $TimeoutMin min"; exit 3 }
Write-Host "Exit code $($p.ExitCode). Log: $log"
Select-String -Path $log -Pattern "Error:|Traceback|LogPython: Error" -ErrorAction SilentlyContinue | select -First 20 | %{ $_.Line }
exit $p.ExitCode
