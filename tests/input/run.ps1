param([string]$Toolset, [string]$WindowsSdk)
$ErrorActionPreference = 'Stop'
$sourcePath = Join-Path $PSScriptRoot '..\..\Keyboard.cpp'
$source = [System.IO.File]::ReadAllText($sourcePath, [System.Text.Encoding]::GetEncoding(1252))
$functions = foreach ($name in @('LIFOadd', 'LIFOremove')) {
    $start = $source.IndexOf('extern "C" void ' + $name + '(')
    if ($start -lt 0) { throw "Native input function missing: $name" }
    $brace = $source.IndexOf('{', $start)
    $depth = 1
    $end = $brace + 1
    while ($depth -gt 0) {
        if ($source[$end] -eq '{') { $depth++ }
        if ($source[$end] -eq '}') { $depth-- }
        $end++
    }
    $source.Substring($start, $end - $start)
}
[System.IO.File]::WriteAllText((Join-Path $PSScriptRoot 'NativeLifo.inc'), ($functions -join "`r`n"), [System.Text.Encoding]::ASCII)
$audioPath = Join-Path $PSScriptRoot '..\..\audio.cpp'
$audio = [System.IO.File]::ReadAllText($audioPath, [System.Text.Encoding]::GetEncoding(1252))
$audioStart = $audio.IndexOf('static bool RestartAudioDevice()')
if ($audioStart -lt 0) { throw 'Native audio restart helper missing.' }
$audioBrace = $audio.IndexOf('{', $audioStart)
$audioDepth = 1
$audioEnd = $audioBrace + 1
while ($audioDepth -gt 0) {
    if ($audio[$audioEnd] -eq '{') { $audioDepth++ }
    if ($audio[$audioEnd] -eq '}') { $audioDepth-- }
    $audioEnd++
}
[System.IO.File]::WriteAllText((Join-Path $PSScriptRoot 'NativeAudioWait.inc'), $audio.Substring($audioStart, $audioEnd - $audioStart), [System.Text.Encoding]::ASCII)
$inputPath = Join-Path $PSScriptRoot '..\..\DesktopInput.cpp'
$inputSource = [System.IO.File]::ReadAllText($inputPath)
$inputFunctions = foreach ($signature in @('void VirtualHeld(', 'void WheelPulse(', 'void DesktopInputMouseEvent(', 'void HoldKeyboardLocked(', 'void ReleaseKeyboardLocked(', 'void ExpireKeyboardReleases(', 'void DesktopInputKeyboardEvent(', 'bool DesktopInputConsumeKeyPress(')) {
    $inputStart = $inputSource.IndexOf($signature)
    if ($inputStart -lt 0) { throw "Native input function missing: $signature" }
    $inputBrace = $inputSource.IndexOf('{', $inputStart)
    $inputDepth = 1
    $inputEnd = $inputBrace + 1
    while ($inputDepth -gt 0) {
        if ($inputSource[$inputEnd] -eq '{') { $inputDepth++ }
        if ($inputSource[$inputEnd] -eq '}') { $inputDepth-- }
        $inputEnd++
    }
    $inputSource.Substring($inputStart, $inputEnd - $inputStart)
}
[System.IO.File]::WriteAllText((Join-Path $PSScriptRoot 'NativeInputPulses.inc'), ($inputFunctions -join "`r`n"), [System.Text.Encoding]::ASCII)
. (Join-Path $PSScriptRoot '..\..\build-tools.ps1')
$tools = Get-MMB4WBuildTools -Toolset $Toolset -WindowsSdk $WindowsSdk
& $tools.MSBuild (Join-Path $PSScriptRoot 'input-tests.vcxproj') /m /nologo /v:minimal /p:Configuration=Release /p:Platform=Win32 "/p:PlatformToolset=$($tools.Toolset)" "/p:WindowsTargetPlatformVersion=$($tools.WindowsSdk)"
if ($LASTEXITCODE -ne 0) { throw 'Input regression tests did not compile.' }
& (Join-Path $PSScriptRoot 'build\input-tests.exe')
if ($LASTEXITCODE -ne 0) { throw 'Input regression tests failed.' }
