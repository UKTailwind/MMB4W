param([string]$Toolset, [string]$WindowsSdk)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'build-tools.ps1')
$tools = Get-MMB4WBuildTools -Toolset $Toolset -WindowsSdk $WindowsSdk
& (Join-Path $PSScriptRoot 'input\run.ps1') -Toolset $tools.Toolset -WindowsSdk $tools.WindowsSdk
if ($LASTEXITCODE -ne 0) { throw 'Input regression checks failed.' }
& $tools.MSBuild (Join-Path $PSScriptRoot 'shell\shell-tests.vcxproj') /m /nologo /v:minimal /p:Configuration=Release /p:Platform=Win32 "/p:PlatformToolset=$($tools.Toolset)" "/p:WindowsTargetPlatformVersion=$($tools.WindowsSdk)"
if ($LASTEXITCODE -ne 0) { throw 'Shell regression checks did not compile.' }
& (Join-Path $PSScriptRoot 'shell\build\shell-tests.exe')
if ($LASTEXITCODE -ne 0) { throw 'Shell regression checks failed.' }
