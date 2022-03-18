/***********************************************************************************************************************
MMBasic for Windows

MainThread.h

<COPYRIGHT HOLDERS>  Geoff Graham, Peter Mather
Copyright (c) 2021, <COPYRIGHT HOLDERS> All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1.	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2.	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
	in the documentation and/or other materials provided with the distribution.
3.	The name MMBasic be used when referring to the interpreter in any documentation and promotional material and the original copyright message be displayed
	on the console at startup (additional copyright messages may be added).
4.	All advertising materials mentioning features or use of this software must display the following acknowledgement: This product includes software developed
	by the <copyright holder>.
5.	Neither the name of the <copyright holder> nor the names of its contributors may be used to endorse or promote products derived from this software
	without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDERS> AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDERS> BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

************************************************************************************************************************/
#pragma once
#include "olcPixelGameEngine.h"
#include "olcPGEX_Sound.h"
#include "Configuration.h"
#include <csetjmp>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma warning(disable : 4996)
#pragma warning(disable : 6011)
class MMBasic : public olc::PixelGameEngine
{
private:
	uint8_t PAGE1;
	uint8_t PAGE0;
	uint8_t BACKGROUND;
public:
	MMBasic();
public:
	bool OnUserDestroy() override;
	bool OnUserCreate() override;
	bool OnUserUpdate(float fElapsedTime) override;
	void CheckKeyBoard(void);
	float MyCustomSynthFunction(int nChannel, float fGlobalTime, float fTimeStep);
};
enum SystemMode { MODE_RUN, MODE_QUIT, MODE_STOPTHREADS, MODE_RESIZE, MODE_SOFTRESET, MODE_SAMPLERATE };
extern volatile unsigned char ConsoleRxBuf[CONSOLE_RX_BUF_SIZE];
extern volatile int ConsoleRxBufHead;
extern volatile int ConsoleRxBufTail;
extern char BreakKey ; 
extern int MMAbort;
extern int GPSchannel;
extern unsigned char WatchdogSet;
extern unsigned char IgnorePIN;
extern jmp_buf mark;
extern jmp_buf jmprun; 
extern unsigned char* InterruptReturn;
extern int InterruptUsed;
extern unsigned char* CFunctionFlash;
extern int MMCharPos;
extern int PromptFont;
extern int64_t PromptFC, PromptBC;
extern volatile int  _excep_code;
extern volatile int HRes, VRes, PixelSize;
extern volatile unsigned int PauseTimer;                            // this is used in the PAUSE command
extern volatile unsigned int IntPauseTimer;
extern volatile long long int mSecTimer;
extern volatile unsigned int MouseTimer;
extern volatile unsigned int MouseProcTimer;
extern volatile unsigned int ScrewUpTimer;
extern volatile unsigned int WDTimer;
extern volatile int keytimer;
extern volatile int TOUCH_DOWN;
extern volatile int SystemMode;
extern uint32_t* PageWrite;
extern uint32_t* PageRead;
extern uint32_t* PageWrite0;
extern uint32_t* PageWrite1;
extern uint32_t* PageBackground;
extern uint64_t BackGroundColour;
extern int ReadPage, WritePage;
extern int ARGBenabled;
extern volatile unsigned char KEYLIFO[10];
extern volatile int KEYLIFOpointer;
extern int numlock, scrolllock, shiftlock, modifiers;
extern DWORD WINAPI Basic(LPVOID lpParameter);
extern DWORD WINAPI mClock(LPVOID lpParameter);
extern uint32_t FrameBuffer[];
extern int64_t frequency;
extern volatile int64_t fasttime;
extern volatile int64_t startfasttime;
extern volatile int DisplayActive;
extern int64_t doupdate;
extern volatile float fSynthFrequencyL;
extern volatile float fSynthFrequencyR;
extern volatile float fFilterVolumeL;
extern volatile float fFilterVolumeR;
extern int SampleRate;
extern int NChannels;
extern int OptionHeight;
extern int OptionWidth;
extern int noaudio;

extern bool FullScreen;
extern "C" unsigned char MMputchar(unsigned char c);
extern "C" int MMgetchar(void);
extern "C" int getConsole(int speed);
extern "C" void CheckAbort(void);
extern "C" void uSec(int t);
extern "C" void MMgetline(int filenbr, char* p);
extern "C" void SaveProgramToMemory(unsigned char* pm, int msg);
extern "C" int kbhitConsole(void);
extern "C" void clearrepeat(void);
extern "C" void DisplayPutC(unsigned char c);
extern "C"  int WINAPI EditPaste(VOID);
extern volatile int mouse_xpos, mouse_ypos, mouse_wheel, mouse_left, mouse_right, mouse_middle;
