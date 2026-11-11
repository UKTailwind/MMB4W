#include "../../DesktopInput.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>

static std::vector<int> heldKeys;
static int downs = 0, ups = 0;
static void Down(int key) { heldKeys.push_back(key); ++downs; }
static void Up(int key) { heldKeys.erase(std::remove(heldKeys.begin(), heldKeys.end(), key), heldKeys.end()); ++ups; }
static void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }

struct GuardedLifo {
    unsigned char before[4] = {0xcd,0xcd,0xcd,0xcd};
    volatile unsigned char bytes[10]{};
    unsigned char after[4] = {0xcd,0xcd,0xcd,0xcd};
} lifo;
#define KEYLIFO lifo.bytes
volatile int KEYLIFOpointer = 0;
int keytimer = 0;
// Extracted from Keyboard.cpp by run.ps1 so the overflow check tests real code.
#include "NativeLifo.inc"

static DesktopInputOwners owners(LIFOadd,LIFOremove);
static std::array<ULONGLONG,DesktopInputOwners::SourceCount> releaseAfter{};
static std::atomic<unsigned> pendingMousePresses{0};
static std::array<std::atomic<bool>,256> pendingKeyPresses{};
static std::array<std::atomic<bool>,256> rawKeyboardDown{};
static std::array<bool,256> keyboardReleased{};
static constexpr ULONGLONG MinimumTapMs=80;
static std::vector<int> queued;
static void QueueCharacter(int key) { queued.push_back(key); }
#include "NativeInputPulses.inc"

enum { MODE_RUN, MODE_QUIT, MODE_STOPTHREADS, MODE_RESIZE, MODE_SOFTRESET, MODE_SAMPLERATE };
volatile int SystemMode = MODE_RUN;
static std::atomic<bool> quit{false};
static bool DesktopQuitRequested() { return quit.load(); }
#include "NativeAudioWait.inc"

static void SetMode(int mode) {
    InterlockedExchange(reinterpret_cast<volatile LONG*>(&SystemMode), mode);
}
static void CompleteAudioRestart(int finalMode) {
    while (SystemMode != MODE_SAMPLERATE) Sleep(1);
    SetMode(finalMode);
}

int main() {
    try {
        std::array<int,256> mapping{};
        for (int key=0; key<256; ++key) mapping[key]=key;
        Check(DesktopInputMappedKey(mapping,'w')=='w',"Identity preserves lowercase typing");
        Check(DesktopInputMappedKey(mapping,'W')=='W',"Identity preserves uppercase typing");
        mapping['W']=128;
        Check(DesktopInputMappedKey(mapping,'w')==128 && DesktopInputMappedKey(mapping,'W')==128,
              "Mappings match source letters independently of Shift");
        mapping['F']=0;
        Check(DesktopInputMappedKey(mapping,'f')==0,"Disabled keys emit no guest key");
        Check(DesktopInputMappedKey(mapping,13)==13 && DesktopInputMappedKey(mapping,131)==131,
              "Special guest keys pass through");
        Check(DesktopInputMappedKey(mapping,-1)==0 && DesktopInputMappedKey(mapping,256)==0,
              "Out of range mappings cannot index the table");

        DesktopInputOwners unitOwners(Down,Up);
        unitOwners.Press(1,32); unitOwners.Press(256,32);
        Check(downs==1 && unitOwners.References(32)==2,"Keyboard and mouse share one guest key");
        unitOwners.Release(1);
        Check(ups==0 && heldKeys.size()==1,"One source release must not cancel another source");
        unitOwners.Release(256);
        Check(ups==1 && heldKeys.empty(),"Last source releases guest key");
        unitOwners.Press(7,128); unitOwners.Press(7,131);
        Check(unitOwners.References(128)==0 && unitOwners.KeyFor(7)==131 && heldKeys.size()==1,
              "Changing a heldKeys mapping releases the old target");
        unitOwners.Release(7);
        Check(heldKeys.empty(),"Key release uses target stored at press time");
        unitOwners.Press(8,77); unitOwners.Press(8,77);
        Check(unitOwners.References(77)==1,"Repeated press does not duplicate ownership");
        unitOwners.Press(8,0);
        Check(unitOwners.References(77)==0 && heldKeys.empty(),"Disabling a source releases its old key");
        unitOwners.Press(9,128); unitOwners.Press(265,32); unitOwners.Clear();
        Check(heldKeys.empty() && unitOwners.References(128)==0 && unitOwners.References(32)==0,
              "Focus loss and pause release all keyboard/controller unitOwners");
        unitOwners.Press(-1,32); unitOwners.Press(999,32);
        Check(heldKeys.empty(),"Invalid physical sources are ignored");

        DesktopInputOwners nativeOwners(LIFOadd,LIFOremove);
        for (int source=0; source<20; ++source) nativeOwners.Press(source,source+32);
        Check(KEYLIFOpointer<=10,"Native KEYDOWN backing array stays bounded");
        Check(std::all_of(std::begin(lifo.before),std::end(lifo.before),[](unsigned char c){return c==0xcd;}) &&
              std::all_of(std::begin(lifo.after),std::end(lifo.after),[](unsigned char c){return c==0xcd;}),
              "More than ten sources do not overwrite adjacent native memory");
        nativeOwners.Clear();
        Check(KEYLIFOpointer==0,"All overflowed and representable sources clear cleanly");

        DesktopInputMouseEvent(0,true);DesktopInputMouseEvent(0,false);
        unsigned press=pendingMousePresses.exchange(0);
        Check(press==1,"A complete click between frames retains its native press event");
        VirtualHeld(256,(press&1)!=0,32,100,true);
        Check(KEYLIFOpointer==1 && KEYLIFO[0]==32 && queued.size()==1,"Latched mouse click reaches guest keyboard and console");
        VirtualHeld(256,false,32,179,true);
        Check(KEYLIFOpointer==1,"Short click stays heldKeys long enough for a 20Hz game");
        VirtualHeld(256,false,32,180,true);
        Check(KEYLIFOpointer==0,"Mouse pulse releases at its 80ms deadline");
        WheelPulse(259,77,200);
        VirtualHeld(259,false,0,279,false);
        Check(KEYLIFOpointer==1 && KEYLIFO[0]==77,"Mapped wheel action also has an 80ms guest pulse");
        VirtualHeld(259,false,0,280,false);
        Check(KEYLIFOpointer==0,"Wheel pulse releases without a physical key-up event");
        DesktopInputMouseEvent(-1,true);DesktopInputMouseEvent(3,true);DesktopInputMouseEvent(2,false);
        Check(pendingMousePresses.exchange(0)==0,"Invalid buttons and releases do not create phantom presses");

        DesktopInputKeyboardEvent(26,true);DesktopInputKeyboardEvent(26,true);DesktopInputKeyboardEvent(26,false);
        Check(DesktopInputConsumeKeyPress(26),"Completed native keyboard taps survive until the graphics frame");
        Check(!DesktopInputConsumeKeyPress(26),"OS keyboard repeat does not duplicate the native press latch");
        Check(!DesktopInputConsumeKeyPress(-1) && !DesktopInputConsumeKeyPress(256),"Invalid native key events are ignored");
        HoldKeyboardLocked(26,128,300);ReleaseKeyboardLocked(26,301);
        Check(KEYLIFOpointer==1 && KEYLIFO[0]==128,"A mapped quick key tap reaches guest KEYDOWN");
        ExpireKeyboardReleases(379);
        Check(KEYLIFOpointer==1,"Keyboard tap remains held for at least80ms");
        ExpireKeyboardReleases(380);
        Check(KEYLIFOpointer==0,"Completed keyboard tap releases after its deadline");
        HoldKeyboardLocked(26,128,400);ExpireKeyboardReleases(600);
        Check(KEYLIFOpointer==1,"Physical keyboard holds are not expired as taps");
        ReleaseKeyboardLocked(26,601);
        Check(KEYLIFOpointer==0,"Long keyboard holds release immediately on key-up");

        SetMode(MODE_RUN);
        std::thread normalRestart(CompleteAudioRestart,MODE_RUN);
        bool restarted=RestartAudioDevice();
        normalRestart.join();
        Check(restarted,"Audio restart resumes after the desktop acknowledges it");
        SetMode(MODE_QUIT);
        Check(!RestartAudioDevice() && SystemMode==MODE_QUIT,"Audio cannot overwrite an existing close request");
        SetMode(MODE_STOPTHREADS);
        Check(!RestartAudioDevice() && SystemMode==MODE_STOPTHREADS,"Audio cannot overwrite interpreter shutdown");
        SetMode(MODE_RUN);
        std::thread closing(CompleteAudioRestart,MODE_QUIT);
        restarted=RestartAudioDevice();
        closing.join();
        Check(!restarted,"Window close releases an in-flight audio restart wait");
        SetMode(MODE_RUN);
        std::thread stopping(CompleteAudioRestart,MODE_STOPTHREADS);
        restarted=RestartAudioDevice();
        stopping.join();
        Check(!restarted,"Interpreter shutdown releases an in-flight audio restart wait");
        SetMode(MODE_RUN);quit.store(true);
        Check(!RestartAudioDevice() && SystemMode==MODE_RUN,"Shell quit does not start a new sound device");
        std::puts("DESKTOP INPUT MAPPING / OWNERSHIP / NATIVE LIFO / MOUSE+KEYBOARD PULSES / AUDIO SHUTDOWN PASS");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr,"INPUT TEST FAILED: %s\n",error.what());
        return 1;
    }
}
