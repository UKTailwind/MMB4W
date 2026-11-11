$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath($PSScriptRoot)
$output = Join-Path $root 'emulator'
$dist = Join-Path $root 'dist'
$stage = Join-Path $dist 'MMB4W-Emulator'
$expectedStage = [IO.Path]::GetFullPath((Join-Path $root 'dist\MMB4W-Emulator'))
$inputs = @{
    'MMB4W-Emulator.exe' = Join-Path $output 'MMB4W-Emulator.exe'
    'MMBasic-LICENSE.txt' = Join-Path $root 'Readme'
    'THIRD-PARTY-NOTICES.txt' = Join-Path $root 'THIRD-PARTY-NOTICES.txt'
    'README.md' = Join-Path $root 'README.md'
    'Input Monitor.bas' = Join-Path $root 'tests\smoke-emulator.bas'
}
foreach ($source in $inputs.Values) {
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Package input missing: $source. Run build-emulator.ps1 first." }
}
foreach ($directory in @($dist, $stage)) {
    if ((Test-Path -LiteralPath $directory) -and ((Get-Item -LiteralPath $directory -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Package output must not be a directory link: $directory"
    }
}
if (Test-Path -LiteralPath $stage) {
    $resolvedStage = (Resolve-Path -LiteralPath $stage).Path
    if (-not $resolvedStage.Equals($expectedStage, [StringComparison]::OrdinalIgnoreCase) -or
        -not $resolvedStage.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Package staging resolves outside the intended repository folder.'
    }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $stage | Out-Null
foreach ($name in $inputs.Keys) { Copy-Item -LiteralPath $inputs[$name] -Destination (Join-Path $stage $name) }
$archive = Join-Path $dist 'MMB4W-Emulator.zip'
Compress-Archive -LiteralPath $stage -DestinationPath $archive -Force
$checksum = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $dist 'MMB4W-Emulator.zip.sha256'), "$checksum  MMB4W-Emulator.zip`n", [Text.Encoding]::ASCII)
Write-Output $archive
Write-Output "SHA256 $checksum"
