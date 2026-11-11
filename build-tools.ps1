function Get-MMB4WBuildTools {
    param([string]$Toolset, [string]$WindowsSdk)

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw 'Install Visual Studio C++ Build Tools with the Desktop development with C++ workload and a Windows SDK.'
    }
    $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
    if (-not $installation) { throw 'No Visual Studio installation with x86 C++ build tools was found.' }
    $msbuild = Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
    if (-not (Test-Path -LiteralPath $msbuild)) { throw 'MSBuild was not found in the Visual Studio installation.' }

    if (-not $Toolset) {
        $toolsetRoot = Join-Path $installation 'MSBuild\Microsoft\VC'
        $available = foreach ($vcVersion in Get-ChildItem -LiteralPath $toolsetRoot -Directory) {
            foreach ($candidate in @('v145', 'v143', 'v142')) {
                if (Test-Path -LiteralPath (Join-Path $vcVersion.FullName "Platforms\Win32\PlatformToolsets\$candidate")) { $candidate }
            }
        }
        $Toolset = $available | Sort-Object -Descending -Unique | Select-Object -First 1
        if (-not $Toolset) { throw 'No supported Win32 toolset was found. Pass -Toolset for an installed toolset.' }
    }
    if (-not $WindowsSdk) {
        $sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Include'
        $WindowsSdk = Get-ChildItem -LiteralPath $sdkRoot -Directory |
            Where-Object { (Test-Path -LiteralPath (Join-Path $_.FullName 'um\Windows.h')) -and (Test-Path -LiteralPath (Join-Path $_.FullName 'ucrt\stdlib.h')) } |
            Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1 -ExpandProperty Name
        if (-not $WindowsSdk) { throw 'No complete Windows SDK was found. Install a Windows 10/11 SDK or pass -WindowsSdk.' }
    }
    [pscustomobject]@{ MSBuild = $msbuild; Toolset = $Toolset; WindowsSdk = $WindowsSdk }
}
