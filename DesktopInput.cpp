#include "MainThread.h"
#include "Commands.h"
#include "DesktopInput.h"
#include "DesktopShell.h"
#include <Xinput.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <mutex>

extern "C" void LIFOadd(int key);
extern "C" void LIFOremove(int key);

namespace {
constexpr int MouseFirst = 256;
constexpr int WheelUp = 259;
constexpr int WheelDown = 260;
constexpr int MotionFirst = 261;
constexpr int GamepadFirst = 265;
constexpr ULONGLONG MinimumTapMs = 80;

DesktopInputOwners owners(LIFOadd, LIFOremove);
std::mutex inputMutex;
std::array<ULONGLONG, DesktopInputOwners::SourceCount> releaseAfter{};
DesktopSettings previousSettings;
bool haveSettings = false;
bool blockMouse = true;
bool blockGamepad = true;
std::atomic<unsigned> pendingMousePresses{0};
std::array<std::atomic<bool>, 256> pendingKeyPresses{};
std::array<std::atomic<bool>, 256> rawKeyboardDown{};
std::array<bool, 256> keyboardReleased{};

void QueueCharacter(int key) {
    if (key < 1 || key > 255) return;
    if (BreakKey && key == BreakKey) {
        MMAbort = true;
        ConsoleRxBufHead = ConsoleRxBufTail;
        return;
    }
    if (key == keyselect && KeyInterrupt != nullptr) {
        Keycomplete = 1;
        return;
    }
    ConsoleRxBuf[ConsoleRxBufHead] = static_cast<unsigned char>(key);
    ConsoleRxBufHead = (ConsoleRxBufHead + 1) % CONSOLE_RX_BUF_SIZE;
    if (ConsoleRxBufHead == ConsoleRxBufTail)
        ConsoleRxBufTail = (ConsoleRxBufTail + 1) % CONSOLE_RX_BUF_SIZE;
}

int MapKeyboard(const DesktopSettings& settings, int code) {
    // An identity entry preserves normal lowercase/Shift/Caps Lock behaviour.
    return DesktopInputMappedKey(settings.keyboard, code);
}

bool SameBindings(const DesktopSettings& left, const DesktopSettings& right) {
    return left.keyboard == right.keyboard &&
        left.mouseButtonMap == right.mouseButtonMap &&
        left.mouseWheelUp == right.mouseWheelUp && left.mouseWheelDown == right.mouseWheelDown &&
        left.mouseEnabled == right.mouseEnabled && left.mouseMotion == right.mouseMotion &&
        left.mouseDeadZone == right.mouseDeadZone &&
        left.gamepadEnabled == right.gamepadEnabled && left.gamepadMap == right.gamepadMap &&
        left.gamepadDeadZone == right.gamepadDeadZone;
}

void ClearHeldLocked() {
    owners.Clear();
    releaseAfter.fill(0);
    blockMouse = true;
    blockGamepad = true;
    pendingMousePresses.store(0);
    for (int key = 0; key < 256; ++key) {
        pendingKeyPresses[key].store(false);
        rawKeyboardDown[key].store(false);
        keyboardReleased[key] = false;
    }
    mouse_left = mouse_right = mouse_middle = mouse_wheel = 0;
}

void VirtualHeld(int source, bool held, int code, ULONGLONG now, bool latchTap) {
    if (held && code > 0 && code < 256) {
        if (owners.KeyFor(source) != code) {
            owners.Press(source, code);
            releaseAfter[source] = latchTap ? now + MinimumTapMs : 0;
            QueueCharacter(code);
        }
    } else if (now >= releaseAfter[source]) {
        owners.Release(source);
        releaseAfter[source] = 0;
    }
}

void WheelPulse(int source, int code, ULONGLONG now) {
    if (code < 1 || code > 255) return;
    owners.Press(source, code);
    releaseAfter[source] = now + MinimumTapMs;
    QueueCharacter(code);
}

void HoldKeyboardLocked(int physicalKey, int mapped, ULONGLONG now) {
    if (physicalKey < 1 || physicalKey >= 256) return;
    owners.Press(physicalKey, mapped);
    keyboardReleased[physicalKey] = false;
    releaseAfter[physicalKey] = now + MinimumTapMs;
}

void ReleaseKeyboardLocked(int physicalKey, ULONGLONG now) {
    if (physicalKey < 1 || physicalKey >= 256) return;
    if (!owners.KeyFor(physicalKey)) return;
    keyboardReleased[physicalKey] = true;
    if (now >= releaseAfter[physicalKey]) {
        owners.Release(physicalKey);
        keyboardReleased[physicalKey] = false;
        releaseAfter[physicalKey] = 0;
    }
}

void ExpireKeyboardReleases(ULONGLONG now) {
    for (int key = 1; key < 256; ++key)
        if (keyboardReleased[key]) ReleaseKeyboardLocked(key, now);
}

using XInputGetStateFunction = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
XInputGetStateFunction GamepadFunction() {
    static const auto function = []() -> XInputGetStateFunction {
        // Only Windows' own XInput DLLs are loaded; no runtime download/install.
        for (const wchar_t* library : {L"xinput1_4.dll", L"xinput9_1_0.dll", L"xinput1_3.dll"}) {
            HMODULE module = LoadLibraryExW(library, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (!module) continue;
            auto result = reinterpret_cast<XInputGetStateFunction>(GetProcAddress(module, "XInputGetState"));
            if (result) return result;
            FreeLibrary(module);
        }
        return nullptr;
    }();
    return function;
}

std::array<bool, DesktopGamepadActions> ReadGamepad(int deadZone) {
    std::array<bool, DesktopGamepadActions> held{};
    auto getState = GamepadFunction();
    if (!getState) return held;
    XINPUT_STATE state{};
    bool found = false;
    for (DWORD index = 0; index < XUSER_MAX_COUNT; ++index) {
        if (getState(index, &state) == ERROR_SUCCESS) { found = true; break; }
    }
    if (!found) return held;
    const WORD buttons = state.Gamepad.wButtons;
    deadZone = std::clamp(deadZone, 0, 30000);
    held[0] = (buttons & XINPUT_GAMEPAD_DPAD_UP) != 0 || state.Gamepad.sThumbLY > deadZone;
    held[1] = (buttons & XINPUT_GAMEPAD_DPAD_DOWN) != 0 || state.Gamepad.sThumbLY < -deadZone;
    held[2] = (buttons & XINPUT_GAMEPAD_DPAD_LEFT) != 0 || state.Gamepad.sThumbLX < -deadZone;
    held[3] = (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0 || state.Gamepad.sThumbLX > deadZone;
    held[4] = (buttons & XINPUT_GAMEPAD_A) != 0;
    held[5] = (buttons & XINPUT_GAMEPAD_B) != 0;
    held[6] = (buttons & XINPUT_GAMEPAD_X) != 0;
    held[7] = (buttons & XINPUT_GAMEPAD_Y) != 0;
    held[8] = (buttons & XINPUT_GAMEPAD_START) != 0;
    held[9] = (buttons & XINPUT_GAMEPAD_BACK) != 0;
    return held;
}
} // namespace

int DesktopInputKeyboardPress(int physicalKey, int translatedCode) {
    const auto settings = DesktopSettingsSnapshot();
    // Windows maps both the main Return and keypad Enter to OLC's ENTER key.
    // BASIC games and the console expect carriage return rather than line feed.
    if (physicalKey == olc::Key::ENTER || physicalKey == olc::Key::RETURN)
        translatedCode = 13;
    const int mapped = MapKeyboard(settings, translatedCode);
    std::lock_guard<std::mutex> lock(inputMutex);
    HoldKeyboardLocked(physicalKey, mapped, GetTickCount64());
    return mapped;
}

void DesktopInputKeyboardRelease(int physicalKey) {
    std::lock_guard<std::mutex> lock(inputMutex);
    ReleaseKeyboardLocked(physicalKey, GetTickCount64());
}

void DesktopInputClearHeld() {
    std::lock_guard<std::mutex> lock(inputMutex);
    ClearHeldLocked();
}

void DesktopInputMouseEvent(int button, bool pressed) {
    if (pressed && button >= 0 && button < 3)
        pendingMousePresses.fetch_or(1u << button);
}

void DesktopInputKeyboardEvent(int physicalKey, bool pressed) {
    if (physicalKey < 1 || physicalKey >= 256) return;
    const bool wasDown = rawKeyboardDown[physicalKey].exchange(pressed);
    if (pressed && !wasDown) pendingKeyPresses[physicalKey].store(true);
}

bool DesktopInputConsumeKeyPress(int physicalKey) {
    return physicalKey > 0 && physicalKey < 256 && pendingKeyPresses[physicalKey].exchange(false);
}

void DesktopInputProcess(MMBasic& engine) {
    const auto settings = DesktopSettingsSnapshot();
    const ULONGLONG now = GetTickCount64();
    std::lock_guard<std::mutex> lock(inputMutex);
    if (!haveSettings || !SameBindings(settings, previousSettings)) {
        ClearHeldLocked();
        previousSettings = settings;
        haveSettings = true;
    }
    if (!engine.IsFocused() || DesktopIsPaused()) {
        ClearHeldLocked();
        return;
    }
    const unsigned mousePresses = pendingMousePresses.exchange(0);
    ExpireKeyboardReleases(now);

    const std::array<bool, 3> mouseHeld{{engine.GetMouse(0).bHeld,
        engine.GetMouse(1).bHeld, engine.GetMouse(2).bHeld}};
    const bool anyMouseHeld = std::any_of(mouseHeld.begin(), mouseHeld.end(), [](bool value) {return value;});
    if (!anyMouseHeld) blockMouse = false;
    const int wheel = engine.GetMouseWheel();
    if (settings.mouseEnabled) {
        mouse_xpos = engine.GetMouseX();
        mouse_ypos = engine.GetMouseY();
        mouse_wheel += wheel;
        mouse_left = mouseHeld[0];
        mouse_right = mouseHeld[1];
        mouse_middle = mouseHeld[2];
    } else {
        mouse_left = mouse_right = mouse_middle = mouse_wheel = 0;
    }
    for (int index = 0; index < 3; ++index)
        VirtualHeld(MouseFirst + index, settings.mouseEnabled && !blockMouse &&
                    (mouseHeld[index] || engine.GetMouse(index).bPressed || (mousePresses & (1u << index))),
                    settings.mouseButtonMap[index], now, true);

    VirtualHeld(WheelUp, false, 0, now, false);
    VirtualHeld(WheelDown, false, 0, now, false);
    if (settings.mouseEnabled && wheel > 0) WheelPulse(WheelUp, settings.mouseWheelUp, now);
    if (settings.mouseEnabled && wheel < 0) WheelPulse(WheelDown, settings.mouseWheelDown, now);

    std::array<bool, 4> motion{};
    if (settings.mouseEnabled && settings.mouseMotion) {
        const int dx = engine.GetMouseX() - engine.ScreenWidth() / 2;
        const int dy = engine.GetMouseY() - engine.ScreenHeight() / 2;
        const int deadZone = (std::max)(0, settings.mouseDeadZone);
        motion = {{dy < -deadZone, dy > deadZone, dx < -deadZone, dx > deadZone}};
    }
    const int directions[4] = {128,129,130,131};
    for (int index = 0; index < 4; ++index)
        VirtualHeld(MotionFirst + index, motion[index], directions[index], now, false);

    const auto gamepadHeld = settings.gamepadEnabled ? ReadGamepad(settings.gamepadDeadZone) :
        std::array<bool, DesktopGamepadActions>{};
    if (!std::any_of(gamepadHeld.begin(), gamepadHeld.end(), [](bool value) {return value;})) blockGamepad = false;
    for (int index = 0; index < DesktopGamepadActions; ++index)
        VirtualHeld(GamepadFirst + index, !blockGamepad && gamepadHeld[index], settings.gamepadMap[index], now, true);
}
