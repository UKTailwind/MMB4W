#define OLC_PGE_APPLICATION
#include "MainThread.h"
#include "FileIO.h"
#include "Draw.h"
#include <chrono>
extern "C" void ResetOptions(void);
unsigned volatile char ConsoleRxBuf[CONSOLE_RX_BUF_SIZE];
volatile int ConsoleRxBufHead = 0;
volatile int ConsoleRxBufTail = 0;
char BreakKey = BREAK_KEY;                                          // defaults to CTRL-C.  Set to zero to disable the break function
uint32_t __declspec(align(256)) FrameBuffer[FRAMEBUFFERSIZE];
#define PAGE1 nullptr
uint32_t low_x=0, high_x=HRes-1, low_y=0, high_y=VRes-1;
uint32_t* PageWrite = FrameBuffer;//Page address used by DrawBuffer, DrawRectangle & DrawBiMap
uint32_t* PageRead = FrameBuffer; //Page address used by ReadBuffer
uint32_t* PageBackground = FrameBuffer; //Page address to write to Background
uint32_t* PageWrite0 = FrameBuffer; //Page address to write to layer 0
uint32_t* PageWrite1 = FrameBuffer; //Page address to write to layer 1
uint32_t* WritePageAddress = FrameBuffer; //Page address to write to layer 1
uint32_t* ReadPageAddress = FrameBuffer; //Page address to write to layer 1
int ReadPage=0, WritePage=0; 
int ARGBenabled = 0;
// Default layer is used as Page 0
// Next layer
int MMAbort = false;
volatile int HRes, VRes, PixelSize, PlayBeep;
volatile int SystemMode = MODE_RUN;
MMBasic* demo;
bool bSynthPlaying = false;
float fSynthFrequency = 0.0f;
float fFilterVolume = 1.0f;
int nSamplePos = 0;
float fPreviousSamples[128];
using clock_type = std::chrono::high_resolution_clock;
using namespace std::literals;
float MMBasic::MyCustomSynthFunction(int nChannel, float fGlobalTime, float fTimeStep)
{
	// Just generate a sine wave of the appropriate frequency
	if (bSynthPlaying)
		return sin(fSynthFrequency * 2.0f * 3.14159f * fGlobalTime);
	else
		return 0.0f;
}

bool MMBasic::OnUserUpdate(float fElapsedTime)
{
	auto when_started = clock_type::now();
	auto target_time = when_started + 10ms;
	CheckKeyBoard(fElapsedTime);
	SetDrawTarget(PAGE0);
	SetPixelMode(olc::Pixel::NORMAL);
	for (int y = 0; y < VRes; y++) {
		for (int x = 0; x < HRes; x++) {
			Draw(x, y, PageWrite0[y * HRes + x]);
		}
	}
	if(ARGBenabled){
		SetDrawTarget(PAGE1);
		SetPixelMode(olc::Pixel::ALPHA);
		for (int y = 0; y < VRes; y++) {
			for (int x = 0; x < HRes; x++) {
				Draw(x, y, PageWrite1[y * HRes + x]);
			}
		}
	}
	std::this_thread::sleep_until(target_time);
	return true;
}
MMBasic::MMBasic()
{
	// Name your application
	sAppName = "MMBasic V5.07.03";
}
bool MMBasic::OnUserCreate()
{
	using namespace std;
	SetDrawTarget(PAGE1); //Top layer
	SetPixelMode(olc::Pixel::ALPHA);
	Clear(olc::BLANK);
 
	PAGE0 = CreateLayer(); //Normal write layer
	EnableLayer(PAGE0, 1);
	SetDrawTarget(PAGE0); 
	SetPixelMode(olc::Pixel::NORMAL);
	Clear(0xFF000000);

	BACKGROUND = CreateLayer(); //solid backgound layer
	EnableLayer(BACKGROUND, 1);
	SetDrawTarget(BACKGROUND);
	SetPixelMode(olc::Pixel::NORMAL);
	Clear(0xFF000000);
	// This gives a compilation error and I can't find the correct syntax
	olc::SOUND::SetUserSynthFunction(MMBasic::MyCustomSynthFunction);
	return true;
}
bool MMBasic::OnUserDestroy()
{
	using namespace std;
	memset((char*)FrameBuffer, 0, FRAMEBUFFERSIZE);
	CurrentX = CurrentY = 0;
	// Called once at the start, so create things here
	return true;
}
DWORD WINAPI Screen(LPVOID lpParameter)
{
	if(demo->Construct(HRes, VRes, PixelSize, PixelSize))
		demo->Start();
	return 0;
}
int main(int argc, char* argv[]) {
	DWORD threadexit;
	demo = new MMBasic;
	unsigned int myCounter = 0;
	DWORD TimerID;
	HANDLE myTHandle = CreateThread(0, 0, mClock, NULL, 0, &TimerID);
	while (mSecTimer < 200) {}
	DWORD BasicID;
	HANDLE myHandle = CreateThread(0, 0, Basic, argv[1], 0, &BasicID);
	while (mSecTimer < 400) {}
	DWORD ScreenID;
	HANDLE mySHandle = CreateThread(0, 0, Screen, NULL, 0, &ScreenID);
	auto when_started = clock_type::now();
	auto target_time = when_started + 10ms;
		while (1) {
			std::this_thread::sleep_until(target_time);
			target_time += 10ms;
			switch (SystemMode) {
		case MODE_RUN:
			break;
		case MODE_QUIT:
			demo->olc_Terminate();
			uSec(100000);
			delete demo;
			SystemMode = MODE_STOPTHREADS;
			while (GetExitCodeThread(myHandle,&threadexit)==STILL_ACTIVE){}
			while (GetExitCodeThread(myTHandle, &threadexit) == STILL_ACTIVE) {}
			return 0;
			break;
		case MODE_RESIZE:
			demo->olc_Terminate();
			uSec(100000);
			delete demo;
			if(mySHandle)CloseHandle(mySHandle);
			demo = new MMBasic;
			mySHandle = CreateThread(0, 0, Screen, &myCounter, 0, &ScreenID);
			KEYLIFOpointer = 0;
			SystemMode = MODE_RUN;
			break;
		case MODE_SOFTRESET:
			while (GetExitCodeThread(myHandle, &threadexit) == STILL_ACTIVE) {}
			uSec(100000);
			CloseHandle(myHandle);
			SystemMode = MODE_RUN;
			uSec(100000);
			KEYLIFOpointer = 0;
			myHandle = CreateThread(0, 0, Basic, &myCounter, 0, &BasicID);
			break;
		}
	}
	return 0;
}