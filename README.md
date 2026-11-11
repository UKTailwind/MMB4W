# MMB4W Emulator for Windows

A portable desktop application for running **MMBasic for Windows** programs, based on [UKTailwind/MMB4W](https://github.com/UKTailwind/MMB4W) version **5.07.03b20**. This edition adds native menus, configurable keyboard/mouse/controller input, fullscreen and scaling controls, audio controls and screenshots, together with interpreter runtime fixes.

This product includes software developed by **Geoff Graham and Peter Mather**.

## Download and start

Download **MMB4W-Emulator.zip** from the [releases page](https://github.com/GremlinMo/MMB4W/releases), extract the complete folder to a writable location, and double-click **MMB4W-Emulator.exe**. The executable is also available separately on the release page; keep [the original MMBasic redistribution terms](https://github.com/GremlinMo/MMB4W/blob/emulator-desktop/Readme) with any copy you distribute.

Use **File > Open program** to choose a `.BAS` file. The emulator can start by itself and does not need a game launcher. The portable ZIP contains the executable, this guide, `MMBasic-LICENSE.txt`, `THIRD-PARTY-NOTICES.txt` and `Input Monitor.bas`. Games, game shortcuts and game assets are not bundled.

The published build is an x86 Windows application with the Visual C++ runtime linked statically, so a separate Visual C++ runtime installation is not needed. It has been tested on the development Windows PC; other Windows versions and hardware combinations have not been comprehensively verified.

## Desktop controls

| Shortcut | Action |
| --- | --- |
| Ctrl+O | Open an MMBasic program |
| Ctrl+R | Reset the current program |
| F1 | Show controls and shortcuts |
| F9 | Pause / resume |
| F10 | Open keyboard, mouse, controller, display and audio settings |
| F11 or Alt+Enter | Fullscreen / windowed |
| F12 | Save a BMP screenshot |
| Ctrl+M | Mute / unmute |
| Ctrl+Alt | Release a captured mouse pointer |

Menus also provide recent programs, integer display scaling, volume and input profile selection. Opening or resetting a program starts a fresh emulator process. Display changes preserve interpreter graphics pages; a BASIC `MODE` change recreates the graphics engine.

Settings and recent-file history live in `emulator.ini` beside the executable. Interpreter options use a separate binary `.options` file in the same folder. Screenshots are timestamped BMPs in its `Screenshots` subfolder, and interpreter errors are recorded in `MMB4W-error.log`. Use a writable portable folder so these files can be saved. A fresh download contains no personal settings, history, logs or screenshots.

## Keyboard, mouse and controller input

In **Input > Settings**, select a guest key/action, choose **Assign a host key...**, then press the physical key you want to use. **Apply** keeps the change; **Save as Custom** stores a custom profile. **Remove selected override** restores a host key's usual behavior. Several physical inputs may share an action. F1 and F9–F12 remain emulator shortcuts.

The Default profile keeps normal keyboard and mouse input available to programs. Native mouse position, buttons and wheel are exposed through `MOUSE(X)`, `MOUSE(Y)`, `MOUSE(L)`, `MOUSE(R)`, `MOUSE(M)` and `MOUSE(W)`. Mapping a mouse button to a BASIC key does not remove its native mouse event. Mouse direction mapping, capture and dead zone are optional settings.

The optional Gauntlet input profile maps the left mouse button to Space (32) and the right button to M (77), while retaining normal keyboard layouts. It is an input preset only; there is no Gauntlet launch shortcut or bundled game.

An XInput controller defaults to arrows on its directional pad/left stick, A for Space, B for M, Start for Enter and Back for Escape. Settings allow controller enable/disable, dead zone and action mappings. XInput is loaded from Windows system libraries; no driver is bundled. Physical controller hardware was not available for verification.

Open **Input Monitor.bas** from the portable folder to inspect held keys, the last detected key, pointer position, buttons and accumulated wheel movement. Escape exits the monitor. Quick keyboard and mouse pulses are retained briefly so polling BASIC programs can detect them.

## Compatibility and limits

This application runs MMBasic programs through the MMB4W interpreter. It is not a CPU-level CMM2 hardware emulator. Programs that require unsupported hardware peripherals or different firmware behavior may need adaptation. Save states, rewind and arbitrary ROM formats are not implemented. The upstream `TURTLE` engine is excluded from this standalone preview; `TURTLE` reports a recoverable unsupported-command error. The original engine has separate GPL terms and remains in the source archive. BMP loading uses a new Windows-native decoder; the upstream Microchip decoder is excluded from the build.

The file-open/reset path must be **shorter than 236 characters and contain no commas**. Parts of the interpreter use the Windows active code page for filenames; use paths representable in that code page. Keep all relative program assets with the program; File Open starts it with its containing folder as the working directory.

## Source, build and verification

The standalone work is maintained on the [`emulator-desktop` branch](https://github.com/GremlinMo/MMB4W/tree/emulator-desktop), based on upstream commit `b1a0586a6559e7b29b5f71ee411798f03d55fbae`. Upstream sources and their original notices remain included. [BUILD-EMULATOR.md](https://github.com/GremlinMo/MMB4W/blob/emulator-desktop/BUILD-EMULATOR.md) explains the build, packaging and regression commands. [CHANGELOG-EMULATOR.md](https://github.com/GremlinMo/MMB4W/blob/emulator-desktop/CHANGELOG-EMULATOR.md) lists the desktop additions and runtime fixes.

```powershell
powershell -ExecutionPolicy Bypass -File .\build-emulator.ps1
powershell -ExecutionPolicy Bypass -File .\tests\run-native.ps1
powershell -ExecutionPolicy Bypass -File .\tests\run-probes.ps1
powershell -ExecutionPolicy Bypass -File .\package-emulator.ps1
```

The executable is built in `emulator/MMB4W-Emulator.exe`; the clean portable archive is written to `dist/MMB4W-Emulator.zip`.

## Attribution and redistribution

MMBasic for Windows is developed by Geoff Graham and Peter Mather, and the upstream Windows port uses olcPixelGameEngine for screen and sound handling. This product includes software developed by Geoff Graham and Peter Mather. The original startup copyright display and source notices are retained. Read [Readme](https://github.com/GremlinMo/MMB4W/blob/emulator-desktop/Readme) for the complete MMBasic redistribution conditions and warranty disclaimer; binary packages reproduce that text as `MMBasic-LICENSE.txt`. Third-party source headers retain their own notices. This fork does not imply endorsement by the original authors.
