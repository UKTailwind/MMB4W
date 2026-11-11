param([string]$Executable = (Join-Path (Split-Path -Parent $PSScriptRoot) 'emulator\MMB4W-Emulator.exe'))

$ErrorActionPreference = 'Stop'
$exe = (Resolve-Path -LiteralPath $Executable).Path
$root = Split-Path -Parent $PSScriptRoot
$work = Join-Path $root 'build\probe-run'
New-Item -ItemType Directory -Force -Path $work | Out-Null
# Keep probe settings and error logs separate from the user's portable folder.
$probeExe = Join-Path $work 'MMB4W-Emulator.exe'
Copy-Item -LiteralPath $exe -Destination $probeExe -Force
Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'probes') -Filter 'bmp-*.bmp' -File | Copy-Item -Destination $work
$probes = @(
    @{ Name = 'WINDOWS BMP'; Source = 'probes\bmp-smoke.bas'; Marker = 'bmp-marker.txt'; Expected = 'WINDOWS BMP TRUECOLOUR PALETTE CLIP ERROR PASS'; Arguments = '' },
    @{ Name = 'TURTLE SCOPE'; Source = 'probes\turtle-scope.bas'; Marker = 'turtle-marker.txt'; Expected = 'TURTLE PREVIEW SCOPE PASS'; Arguments = '' },
    @{ Name = 'CLI'; Source = 'probes\cli-smoke.bas'; Marker = 'cli-marker.txt'; Expected = 'CLI AUTORUN PASS'; Arguments = '' },
    @{ Name = 'MODE'; Source = 'probes\mode-smoke.bas'; Marker = 'mode-marker.txt'; Expected = 'MODE TRANSITIONS PASS'; Arguments = '' },
    @{ Name = 'FRAMEBUFFER'; Source = 'probes\framebuffer-smoke.bas'; Marker = 'framebuffer-marker.txt'; Expected = 'FRAMEBUFFER FULL AND RECT RESTORE PASS'; Arguments = '' },
    @{ Name = 'INPUT MONITOR'; Source = 'smoke-emulator.bas'; Marker = 'input-monitor-marker.txt'; Expected = 'INPUT MONITOR STARTUP PASS'; Arguments = ',\"SELFTEST\"' }
)
foreach ($probe in $probes) {
    $source = Join-Path $PSScriptRoot $probe.Source
    $program = Join-Path $work ([IO.Path]::GetFileName($source))
    $marker = Join-Path $work $probe.Marker
    Copy-Item -LiteralPath $source -Destination $program -Force
    if (Test-Path -LiteralPath $marker) { Remove-Item -LiteralPath $marker -Force }
    # One CRT argument holds the filename and optional BASIC string expression.
    $arguments = '0 "' + $program + $probe.Arguments + '"'
    $process = Start-Process -FilePath $probeExe -ArgumentList $arguments -WorkingDirectory $work -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit(30000)) {
        $process.Kill()
        throw "$($probe.Name) timed out. Inspect $work\MMB4W-error.log."
    }
    if ($process.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $marker) -or -not ((Get-Content -LiteralPath $marker -Raw).Contains($probe.Expected))) {
        throw "$($probe.Name) failed. Inspect $work\MMB4W-error.log."
    }
    Write-Output "$($probe.Name) PASS"
}
Write-Output "Runtime SHA256 $((Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash)"
