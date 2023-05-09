/***********************************************************************************************************************
MMBasic for Windows

main.cpp

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
#define OLC_PGE_APPLICATION
#define OLC_PGEX_SOUND
#include "MainThread.h"
#include "FileIO.h"
#include "Draw.h"
#include "audio.h"
#include <chrono>
extern "C" void ResetOptions(void);
unsigned volatile char ConsoleRxBuf[CONSOLE_RX_BUF_SIZE] = { 0 };
volatile int ConsoleRxBufHead = 0;
volatile int ConsoleRxBufTail = 0;
volatile int DisplayActive = 0;
char BreakKey = BREAK_KEY;                                          // defaults to CTRL-C.  Set to zero to disable the break function
uint32_t __declspec(align(256)) FrameBuffer[FRAMEBUFFERSIZE] = { 0 };
#define PAGE1 nullptr
uint32_t low_x=0, high_x=HRes-1, low_y=0, high_y=VRes-1;
uint32_t* PageWrite = FrameBuffer;//Page address used by DrawBuffer, DrawRectangle & DrawBiMap
uint32_t* PageRead = FrameBuffer; //Page address used by ReadBuffer
uint32_t* PageBackground = FrameBuffer; //Page address to write to Background
uint32_t* PageWrite0 = FrameBuffer; //Page address to write to layer 0
uint32_t* PageWrite1 = FrameBuffer; //Page address to write to layer 1
uint64_t BackGroundColour = 0xFF000000;
int SampleRate = 44100;
int NChannels = 2;
bool FullScreen = false;
int ReadPage=0, WritePage=0; 
int ARGBenabled = 0;
// Default layer is used as Page 0
// Next layer
int MMAbort = false;
int noaudio = 0;
volatile int HRes, VRes, PixelSize, PlayBeep;
volatile int SystemMode = MODE_RUN;
MMBasic* demo;
int64_t doupdate = 10;
volatile float fSynthFrequencyL = 261.63f;
volatile float fSynthFrequencyR = 261.63f;
volatile float fFilterVolumeL = 1.0f;
volatile float fFilterVolumeR = 1.0f;
int nSamplePos = 0;
volatile int  _excep_code = 0;
float fPreviousSamples[4096];
using clock_type = std::chrono::high_resolution_clock;
using namespace std::literals;
extern "C" void MMPrintString(char* x);
float MMBasic::MyCustomSynthFunction(int nChannel, float fGlobalTime, float fTimeStep)
{
	if (CurrentlyPlaying == P_TONE) {
		if (!SoundPlay) {
			CloseAudio(1);
			WAVcomplete = true;
		}
		else {
			int v;
			SoundPlay--;
			if (nChannel == 0) {
				v= ((((SineTable[(int)PhaseAC_left] - 2000) * mapping[vol_left]) / 2000) + 2000);
				PhaseAC_left = PhaseAC_left + PhaseM_left;
				if (PhaseAC_left >= 4096.0)PhaseAC_left -= 4096.0;
			}
			else {
				v= ((((SineTable[(int)PhaseAC_right] - 2000) * mapping[vol_right]) / 2000) + 2000);
				PhaseAC_right = PhaseAC_right + PhaseM_right;
				if (PhaseAC_right >= 4096.0)PhaseAC_right -= 4096.0;
			}
			return ((float)v - 2000.0f) / 2000.0f;
		}
	}
	else if (CurrentlyPlaying == P_MOD) {
		static int toggle = 0;
		int32_t c1 = 0, c2 = 0, c3 = 0, c4 = 0;
		float value = 0, valuee=0;
		if (swingbuf) { //buffer is primed
			int16_t* flacbuff;
			if (swingbuf == 1)flacbuff = (int16_t*)sbuff1;
			else flacbuff = (int16_t*)sbuff2;
			if (ppos < bcount[swingbuf]) {
				value = (float)flacbuff[ppos++] / 32768.0f * (nChannel == 0 ? fFilterVolumeL : fFilterVolumeR);
			}
			if (ppos == bcount[swingbuf]) {
				int psave = ppos;
				bcount[swingbuf] = 0;
				ppos = 0;
				if (swingbuf == 1)swingbuf = 2;
				else swingbuf = 1;
				if (bcount[swingbuf] == 0) { //nothing ready yet so flip back
					if (swingbuf == 1) {
						swingbuf = 2;
						nextbuf = 1;
					}
					else {
						swingbuf = 1;
						nextbuf = 2;
					}
					bcount[swingbuf] = psave;
					ppos = 0;
				}
			}
		}
		if (CurrentlyPlayinge == P_WAV) {
			static int toggle = 0;
			float* flacbuff;
			if (swingbufe == 1)flacbuff = (float*)sbuff1e;
			else flacbuff = (float*)sbuff2e;
			if (ppose < bcounte[swingbufe]) {
				if (mono) {
					if(toggle)
						valuee = (float)flacbuff[ppose++] * (nChannel == 0 ? fFilterVolumeL : fFilterVolumeR);
					else 
						valuee = (float)flacbuff[ppose] * (nChannel == 0 ? fFilterVolumeL : fFilterVolumeR);
					toggle = !toggle;
				}
				else {
					valuee = (float)flacbuff[ppose++] * (nChannel == 0 ? fFilterVolumeL : fFilterVolumeR);

				}
			}
			if (ppose == bcounte[swingbufe]) {
				int psave = ppose;
				bcounte[swingbufe] = 0;
				ppose = 0;
				if (swingbufe == 1)swingbufe = 2;
				else swingbufe = 1;
				if (bcounte[swingbufe] == 0 && !playreadcompletee) { //nothing ready yet so flip back
					if (swingbufe == 1) {
						swingbufe = 2;
						nextbufe = 1;
					}
					else {
						swingbufe = 1;
						nextbufe = 2;
					}
					bcounte[swingbufe] = psave;
					ppose = 0;
				}
			}
		}
		return value+valuee;
	}
	else if (CurrentlyPlaying == P_MP3 || CurrentlyPlaying == P_WAV || CurrentlyPlaying == P_FLAC) {
		float* flacbuff;
		float value = 0;;
		if (bcount[1] == 0 && bcount[2] == 0 && playreadcomplete == 1) {
//				HAL_TIM_Base_Stop_IT(&htim4);
		}
		if (swingbuf) { //buffer is primed
			if (swingbuf == 1)flacbuff = (float*)sbuff1;
			else flacbuff = (float*)sbuff2;
			if (ppos < bcount[swingbuf]) {
				value = (float)flacbuff[ppos++] * (nChannel == 0 ? fFilterVolumeL : fFilterVolumeR);
			}
			if (ppos == bcount[swingbuf]) {
				int psave = ppos;
				bcount[swingbuf] = 0;
				ppos = 0;
				if (swingbuf == 1)swingbuf = 2;
				else swingbuf = 1;
				if (bcount[swingbuf] == 0 && !playreadcomplete) { //nothing ready yet so flip back
					if (swingbuf == 1) {
						swingbuf = 2;
						nextbuf = 1;
					}
					else {
						swingbuf = 1;
						nextbuf = 2;
					}
					bcount[swingbuf] = psave;
					ppos = 0;
				}
			}
		} return value;
	}
	else if (CurrentlyPlaying == P_SOUND) {
		static int noisedwellleft[MAXSOUNDS] = { 0 }, noisedwellright[MAXSOUNDS] = { 0 };
		static uint32_t noiseleft[MAXSOUNDS] = { 0 }, noiseright[MAXSOUNDS] = { 0 };
		int i, j;
		int leftv = 0, rightv = 0;
		for (i = 0; i < MAXSOUNDS; i++) { //first update the 8 sound pointers
			if (nChannel == 0) {
				if (sound_mode_left[i] != nulltable) {
					if (sound_mode_left[i] != whitenoise) {
						sound_PhaseAC_left[i] = sound_PhaseAC_left[i] + sound_PhaseM_left[i];
						if (sound_PhaseAC_left[i] >= 4096.0)sound_PhaseAC_left[i] -= 4096.0;
						j = (int)sound_mode_left[i][(int)sound_PhaseAC_left[i]];
						j = (j - 2000) * mapping[sound_v_left[i]] / 2000;
						leftv += j;
					}
					else {
						if (noisedwellleft[i] <= 0) {
							noisedwellleft[i] = (int)sound_PhaseM_left[i];
							noiseleft[i] = rand() % 3700 + 100;
						}
						if (noisedwellleft[i])noisedwellleft[i]--;
						j = (int)noiseleft[i];
						j = (j - 2000) * mapping[sound_v_left[i]] / 2000;
						leftv += j;
					}
				}
			}
			else {
				if (sound_mode_right[i] != nulltable) {
					if (sound_mode_right[i] != whitenoise) {
						sound_PhaseAC_right[i] = sound_PhaseAC_right[i] + sound_PhaseM_right[i];
						if (sound_PhaseAC_right[i] >= 4096.0)sound_PhaseAC_right[i] -= 4096.0;
						j = (int)sound_mode_right[i][(int)sound_PhaseAC_right[i]];
						j = (j - 2000) * mapping[sound_v_right[i]] / 2000;
						rightv += j;
					}
					else {
						if (noisedwellright[i] <= 0) {
							noisedwellright[i] = (int)sound_PhaseM_right[i];
							noiseright[i] = rand() % 3700 + 100;
						}
						if (noisedwellright[i])noisedwellright[i]--;
						j = (int)noiseright[i];
						j = (j - 2000) * mapping[sound_v_right[i]] / 2000;
						rightv += j;
					}
				}
			}
		}
		leftv += 2000;
		rightv += 2000;
		if (nChannel == 0)return ((float)leftv - 2000.0f) / 2000.0f;
		else return ((float)rightv - 2000.0f) / 2000.0f;
	}
	return 0.0f;
}

bool MMBasic::OnUserUpdate(float fElapsedTime)
{
	auto when_started = clock_type::now();
	auto target_time = when_started + 10ms;
	CheckKeyBoard();
	if (doupdate) {
		SetPixelMode(olc::Pixel::NORMAL);
		SetDrawTarget(PAGE0);
		for (int y = 0; y < VRes; y++) {
			for (int x = 0; x < HRes; x++) {
				Draw(x, y, PageWrite0[y * HRes + x]);
			}
		}
		doupdate--;
	}
	if(ARGBenabled){
		SetPixelMode(olc::Pixel::NORMAL);
		SetDrawTarget(PAGE1);
		for (int y = 0; y < VRes; y++) {
			for (int x = 0; x < HRes; x++) {
				Draw(x, y, PageWrite1[y * HRes + x]);
			}
		}
	}
	std::this_thread::sleep_until(target_time);
	if (SystemMode == MODE_QUIT || SystemMode == MODE_RESIZE) {
		DisplayActive = 0;
		return false;
	}
	else {
		DisplayActive = 1;
		return true;
	}
}
MMBasic::MMBasic()
{
	// Name your application
	sAppName = "MMBasic V5.07.03";
}
bool MMBasic::OnUserCreate()
{
	using namespace std;
	for (int i = 0; i < FRAMEBUFFERSIZE; i++)FrameBuffer[i] = 0;
	SetDrawTarget(PAGE1); //Top layer
	SetPixelMode(olc::Pixel::NORMAL);

	Clear(olc::BLANK);
 
	PAGE0 = CreateLayer(); //Normal write layer
	EnableLayer(PAGE0, 1);
	SetDrawTarget(PAGE0); 
	SetPixelMode(olc::Pixel::NORMAL);
	Clear(olc::BLANK);

	BACKGROUND = CreateLayer(); //solid backgound layer
	EnableLayer(BACKGROUND, 1);
	SetDrawTarget(BACKGROUND);
	SetPixelMode(olc::Pixel::NORMAL);
	Clear((uint32_t)BackGroundColour);
	int bufferneeded = RoundUptoPage(SampleRate * NChannels /96) ;
	using namespace std::placeholders;
	if (!noaudio) {
		olc::SOUND::InitialiseAudio(SampleRate, NChannels, 8, bufferneeded);
		olc::SOUND::SetUserSynthFunction(std::bind(&MMBasic::MyCustomSynthFunction, demo, _1, _2, _3));
	}
	return true;
}
bool MMBasic::OnUserDestroy()
{
	using namespace std;
	memset((char*)FrameBuffer, 0, FRAMEBUFFERSIZE);
	CurrentX = CurrentY = 0;
	if (!noaudio)olc::SOUND::DestroyAudio();
	return true;
}
DWORD WINAPI Screen(LPVOID lpParameter)
{
	if(demo->Construct(HRes, VRes, PixelSize, PixelSize, FullScreen, false, true))
//	if (demo->Construct(HRes, VRes, PixelSize, PixelSize))
		demo->Start();
	return 0;
}
int main(int argc, char* argv[]) {
	DWORD threadexit;
	demo = new MMBasic;
	unsigned int myCounter = 0;
	DWORD TimerID;
	if (argc>1) {
		if (*argv[1] == '0') {
			noaudio = 1;
		}
	}
	HANDLE myTHandle = CreateThread(0, 0, mClock, NULL, 0, &TimerID);
	while (mSecTimer < 200) {}
	DWORD BasicID;
	HANDLE myHandle=CreateThread(0, 0, Basic, (noaudio==1? argv[2]:argv[1]), 0, &BasicID);
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
//			demo->olc_Terminate();
			uSec(100000);
			delete demo;
			SystemMode = MODE_STOPTHREADS;
			while (GetExitCodeThread(myHandle,&threadexit)==STILL_ACTIVE){}
			while (GetExitCodeThread(myTHandle, &threadexit) == STILL_ACTIVE) {}
			while (GetExitCodeThread(mySHandle, &threadexit) == STILL_ACTIVE) {}
			return 0;
			break;
		case MODE_RESIZE:
//			demo->olc_Terminate();
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
			myHandle = CreateThread(0, 0, Basic, NULL, 0, &BasicID);
			break;
		case MODE_SAMPLERATE:
			if (!noaudio)olc::SOUND::DestroyAudio();
			int bufferneeded = RoundUptoPage(SampleRate * NChannels / 96);
			if (!noaudio)olc::SOUND::InitialiseAudio(SampleRate, NChannels, 8, bufferneeded);
			SystemMode = MODE_RUN;
			break;
		}
	}
	return 0;
}