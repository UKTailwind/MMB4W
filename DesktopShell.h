#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <array>
#include <cstdint>
#include <string>

// Mapping sources are canonical BASIC key codes (A-Z are uppercase). An
// identity entry passes the original character through; target 0 disables it.
constexpr int DesktopGamepadActions = 10;
struct DesktopSettings {
    std::array<int, 256> keyboard{};
    std::array<int, 3> mouseButtonMap{{0, 0, 0}}; // left, right, middle
    int mouseWheelUp = 0;
    int mouseWheelDown = 0;
    bool mouseEnabled = true;
    bool mouseMotion = false;
    bool captureMouse = false;
    int mouseDeadZone = 24;
    bool gamepadEnabled = true;
    // Up, Down, Left, Right, A, B, X, Y, Start, Back.
    std::array<int, DesktopGamepadActions> gamepadMap{{128,129,130,131,32,77,0,0,13,27}};
    int gamepadDeadZone = 10000; // XInput stick units, 0..30000
    int scale = 3;
    bool fullscreen = false;
    int volume = 80;
    bool muted = false;
    std::wstring profile = L"Default";
};

void DesktopInitialize();
void DesktopAttachWindow(HWND window);
HWND DesktopWindowHandle();
// Called before the engine's WndProc switch. True means result is handled.
bool DesktopHandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result);
DesktopSettings DesktopSettingsSnapshot();
uint64_t DesktopSettingsRevision();
void DesktopSetProgram(const char* path);
void DesktopSetProgram(const wchar_t* path);
std::wstring DesktopCurrentProgram();
bool DesktopIsPaused();
float DesktopVolume(); // includes mute, range 0..1, safe on the audio thread
bool DesktopQuitRequested();
bool DesktopConsumeScreenshot();
bool DesktopConsumeDisplayRequest(int& scale, bool& fullscreen);
void DesktopSaveScreenshot(const uint32_t* pixels, int width, int height);
// Useful to release a captured pointer when focus changes or settings opens.
void DesktopUpdateMouseCapture(HWND window, bool focused);
