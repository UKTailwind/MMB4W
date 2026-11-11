# MMB4W Emulator desktop changes

## 5.07.03b20-desktop.1 — 11 November 2026

Standalone desktop edition based on UKTailwind/MMB4W commit `b1a0586a6559e7b29b5f71ee411798f03d55fbae` (MMBasic for Windows 5.07.03b20).

This product includes software developed by Geoff Graham and Peter Mather.

### Desktop application

- Native program-open and recent-file menus, program reset, pause/resume and orderly interpreter/clock/graphics shutdown.
- Configurable keyboard, mouse and XInput input profiles, including custom host-key assignment and shared action mappings.
- Fullscreen/windowed display, integer scaling, volume/mute, pointer capture/release and timestamped BMP screenshots.
- Portable INI settings, per-program working directories and local interpreter options/error logs.
- Properly named `emulator/MMB4W-Emulator.exe` and a clean emulator-only portable archive, with input monitor and licensing.
- Removed the game launch shortcut and game-specific launch/help text. The optional Gauntlet input preset remains available without bundling a game.

### Runtime fixes

- Correct full framebuffer backup/restore copy direction and 32-bit pixel byte counts.
- Preserve full integer page indices, including reserved framebuffer pages above 255, and retain active read/write pages during page copies.
- Notify the renderer after page/bitmap copies.
- Load command-line BASIC programs through the normal tokenizer and preserve interpreter file/line errors in a local log.
- Store interpreter options beside the executable and recover from missing, truncated or incompatible options.
- Retain brief keyboard/mouse pulses for polling programs, coordinate shared input ownership, bound key storage and reject a zero mouse-wheel scale.
- Coordinate audio restart with shutdown.

### Distribution components

- Replaced the upstream Microchip BMP decoder in the build with a Windows-native decoder. The original source is retained but excluded from compilation.
- Excluded the separately licensed GPL turtle engine from the preview binary; `TURTLE` returns a recoverable unsupported-command error.
- Include third-party notices with portable and separate binary downloads.

### Build and scope

- Release Win32 with static C++ runtime, C++17 and no extra console window; unused CppWinRT imports removed.
- Reproducible local build/package scripts and native/BASIC regression runners.
- This remains an MMBasic interpreter application. CPU-level CMM2 emulation, complete hardware peripherals, save states, rewind and arbitrary ROM formats are outside this release. Physical XInput hardware has not been tested.
