param([string]$Toolset, [string]$WindowsSdk)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'build-tools.ps1')
$tools = Get-MMB4WBuildTools -Toolset $Toolset -WindowsSdk $WindowsSdk
$output = Join-Path $PSScriptRoot 'emulator'
$intermediate = Join-Path $PSScriptRoot 'build\emulator'
New-Item -ItemType Directory -Force -Path $output, $intermediate | Out-Null
$arguments = @(
    (Join-Path $PSScriptRoot 'Project3.vcxproj'), '/m', '/t:Build', '/nologo', '/v:minimal',
    '/p:Configuration=Release', '/p:Platform=Win32', '/p:TargetName=MMB4W-Emulator',
    "/p:PlatformToolset=$($tools.Toolset)", "/p:WindowsTargetPlatformVersion=$($tools.WindowsSdk)",
    "/p:OutDir=$output\", "/p:IntDir=$intermediate\", "/p:ImportLibrary=$intermediate\MMB4W-Emulator.lib"
)
& $tools.MSBuild @arguments
if ($LASTEXITCODE -ne 0) { throw "Emulator compilation failed with exit code $LASTEXITCODE." }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Readme') -Destination (Join-Path $output 'MMBasic-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'THIRD-PARTY-NOTICES.txt') -Destination $output
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'README.md') -Destination $output
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'tests\smoke-emulator.bas') -Destination (Join-Path $output 'Input Monitor.bas')
Write-Output (Join-Path $output 'MMB4W-Emulator.exe')
