#pragma once
#include "olcPixelGameEngine.h"
#include "olcPGEX_Sound.h"
#include "Configuration.h"
#include <csetjmp>
#pragma warning(disable : 4996)
#pragma warning(disable : 6011)
class MMBasic : public olc::PixelGameEngine
{
private:
	uint8_t PAGE0;
	uint8_t BACKGROUND;
public:
	MMBasic();
public:
	bool OnUserDestroy() override;
	bool OnUserCreate() override;
	bool OnUserUpdate(float fElapsedTime) override;
	void CheckKeyBoard(float fElapsedTime);
	float MyCustomSynthFunction(int nChannel, float fGlobalTime, float fTimeStep);
};
enum SystemMode { MODE_RUN, MODE_QUIT, MODE_STOPTHREADS, MODE_RESIZE, MODE_SOFTRESET };
extern volatile unsigned char ConsoleRxBuf[CONSOLE_RX_BUF_SIZE];
extern volatile int ConsoleRxBufHead;
extern volatile int ConsoleRxBufTail;
extern char BreakKey ; 
extern int MMAbort;
extern int GPSchannel;
extern unsigned char WatchdogSet;
extern unsigned char IgnorePIN;
extern jmp_buf mark;
extern unsigned char* InterruptReturn;
extern int InterruptUsed;
extern unsigned char* CFunctionFlash;
extern unsigned char* CFunctionLibrary;
extern int MMCharPos;
extern int PromptFont;
extern int64_t PromptFC, PromptBC;
extern int  _excep_code;
extern volatile int HRes, VRes, PixelSize;
extern volatile unsigned int PauseTimer;                            // this is used in the PAUSE command
extern volatile unsigned int IntPauseTimer;
extern volatile long long int mSecTimer;
extern volatile unsigned int MouseTimer;
extern volatile unsigned int MouseProcTimer;
extern volatile int keytimer;
extern volatile int TOUCH_DOWN;
extern volatile int SystemMode;
extern uint32_t* PageWrite;
extern uint32_t* PageRead;
extern uint32_t* PageWrite0;
extern uint32_t* PageWrite1;
extern uint32_t* WritePageAddress;
extern uint32_t* ReadPageAddress;
extern uint32_t* PageBackground;
extern int ReadPage, WritePage;
extern int ARGBenabled;
extern unsigned char KEYLIFO[10];
extern int KEYLIFOpointer;
extern int numlock, scrolllock, shiftlock;
extern DWORD WINAPI Basic(LPVOID lpParameter);
extern DWORD WINAPI mClock(LPVOID lpParameter);
extern uint32_t FrameBuffer[];
extern int64_t frequency;
extern volatile int64_t fasttime;
extern volatile int64_t startfasttime;

extern "C" char MMputchar(char c);
extern "C" int MMgetchar(void);
extern "C" int getConsole(void);
extern "C" void CheckAbort(void);
extern "C" void uSec(int t);
extern "C" void MMgetline(int filenbr, char* p);
extern "C" void SaveProgramToMemory(unsigned char* pm, int msg);
extern "C" int kbhitConsole(void);
extern "C" void clearrepeat(void);
extern "C" void DisplayPutC(char c);
extern volatile int mouse_xpos, mouse_ypos, mouse_wheel, mouse_left, mouse_right, mouse_middle;
