# Building MMB4W Emulator

This product includes software developed by Geoff Graham and Peter Mather. The original MMBasic redistribution terms remain in [Readme](Readme).

## Requirements

Use Windows, PowerShell, and Visual Studio with **Desktop development with C++**, the x86/x64 C++ tools and a Windows 10/11 SDK. `build-tools.ps1` finds the newest suitable Visual Studio installation, chooses an installed Win32 toolset (v145, v143 or v142) and selects the newest complete Windows SDK. The published preview was built with v145 and SDK 10.0.28000.0; earlier development builds used SDK 10.0.26100.0. An installed toolset or SDK can be selected explicitly:

```powershell
.\build-emulator.ps1 -Toolset v143 -WindowsSdk 10.0.26100.0
```

The script builds **Release | Win32**, names the program **MMB4W-Emulator.exe** and writes it to the repository's `emulator` folder. Intermediate objects, import libraries and export files go under `build/emulator`. The C++ runtime is linked statically (`/MT`), with C++17 and the Windows subsystem; the app does not open an extra console window. The 32-bit target is intentional because the interpreter stores graphics pointers in 32-bit integers. Do not substitute an x64 build without addressing those assumptions.

The project no longer needs its unused CppWinRT NuGet imports. No package restore is needed for this build. Existing project configurations outside Release Win32 are retained for source history but are not the supported release path. The API target is Windows 7; this is an API baseline, not a statement that all Windows versions have been tested.

## Regression checks

After building, run:

```powershell
.\tests\run-native.ps1
.\tests\run-probes.ps1
```

The native runner compiles and runs input and desktop-shell checks. Input tests exercise mappings, shared key ownership, held keys, brief keyboard/mouse pulses, bounded key storage and audio-restart shutdown behavior. The shell suite checks configuration persistence, settings controls, hotkeys, menus, recent files and BMP screenshot encoding. The input runner regenerates small test extracts directly from the current runtime helpers.

The probe runner copies the executable to `build/probe-run` and runs six auto-exiting BASIC probes: command-line autorun/file output, repeated MODE changes, full/rectangular framebuffer restore with page-copy state preservation, the input monitor's 100-frame SELFTEST with mouse-wheel divisor validation, Windows BMP truecolor/palette/orientation/clipping/error recovery, and the preview's recoverable TURTLE exclusion. Each must exit within 30 seconds and write its expected marker. These tests use relative paths and keep settings/error logs separate from the portable emulator folder.

Interactive checks remain useful for keyboard/mouse handling, File Open, input assignment, pause, display transitions and audio on real hardware. The automated checks do not establish physical XInput controller compatibility or comprehensive program/hardware compatibility.

## Packaging

```powershell
.\package-emulator.ps1
```

The script creates an allowlisted staging folder and `dist/MMB4W-Emulator.zip`, plus a SHA-256 sidecar. It includes only the executable, README, original license text, third-party notices and input monitor. It excludes games, launchers, shortcuts, personal settings, recent files, logs, screenshots, build outputs and regression markers. Existing settings beside the local executable remain available for development runs.

Publish the ZIP, checksum sidecar, executable and patch to the same release. Recipients of the separate executable must also receive the license text. A downloadable patch can also be generated from the upstream base:

```powershell
git diff --binary b1a0586a6559e7b29b5f71ee411798f03d55fbae HEAD --output=dist/MMB4W-Emulator.patch
```

## Troubleshooting

Interpreter errors are appended to `MMB4W-error.log` beside the executable. Set `MMB4W_TRACE_STARTUP=1` in the launch environment to write startup arguments and the attempted program path to `MMB4W-startup.log`; normal releases do not enable it. Keep BASIC file paths under 236 characters, without commas, and representable in the Windows active code page. Inspect the error log when a BASIC probe times out rather than treating a missing marker as a passing test.
