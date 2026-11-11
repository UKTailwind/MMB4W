#pragma once
#include <array>

class MMBasic;

// Called by the graphics thread; the BASIC interpreter keeps using KEYDOWN/MOUSE.
void DesktopInputProcess(MMBasic& engine);
int DesktopInputKeyboardPress(int physicalKey, int translatedCode);
void DesktopInputKeyboardRelease(int physicalKey);
void DesktopInputClearHeld();
// Called by native mouse messages so a complete click between graphics frames
// still produces its mapped guest action. Button order is left, right, middle.
void DesktopInputMouseEvent(int button, bool pressed);
void DesktopInputKeyboardEvent(int physicalKey, bool pressed);
bool DesktopInputConsumeKeyPress(int physicalKey);

inline int DesktopInputMappedKey(const std::array<int, 256>& mapping, int code) {
    if (code < 1 || code > 255) return 0;
    const int canonical = code >= 'a' && code <= 'z' ? code - 'a' + 'A' : code;
    const int mapped = mapping[canonical];
    if (mapped == canonical) return code;
    return mapped >= 0 && mapped <= 255 ? mapped : 0;
}

// Pure ownership table used by the runtime and native regression checks. Each
// physical source owns its pressed guest key until release, even if a profile
// changes while it is held. Two sources may hold the same guest key safely.
class DesktopInputOwners {
public:
    static constexpr int SourceCount = 320;
    static constexpr int KeyCount = 256;
    using KeyCallback = void (*)(int);

    DesktopInputOwners(KeyCallback down, KeyCallback up) : down_(down), up_(up) {}

    void Press(int source, int key) {
        if (source < 0 || source >= SourceCount) return;
        if (key < 1 || key >= KeyCount) key = 0;
        if (owners_[source] == key) return;
        Release(source);
        owners_[source] = key;
        if (key && references_[key]++ == 0 && down_) down_(key);
    }

    void Release(int source) {
        if (source < 0 || source >= SourceCount) return;
        const int key = owners_[source];
        owners_[source] = 0;
        if (key && --references_[key] == 0 && up_) up_(key);
    }

    void Clear() {
        for (int source = 0; source < SourceCount; ++source) Release(source);
    }

    int KeyFor(int source) const {
        return source >= 0 && source < SourceCount ? owners_[source] : 0;
    }
    int References(int key) const {
        return key >= 0 && key < KeyCount ? references_[key] : 0;
    }

private:
    int owners_[SourceCount]{};
    int references_[KeyCount]{};
    KeyCallback down_;
    KeyCallback up_;
};
