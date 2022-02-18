/***********************************************************************************************************************
MMBasic for Windows

Keyboard.cpp

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
#include "olcPixelGameEngine.h"
#include "MainThread.h"
#include "FileIO.h"
#include "Commands.h"
#include "MM_Misc.h"
#include "Serial.h"
using namespace olc;
const unsigned char asciiUK[256] = {
	0,
	'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
	'0','1','2','3','4','5','6','7','8','9',//K0, K1, K2, K3, K4, K5, K6, K7, K8, K9 //36
	145,146,147,148,149,150,151,152,153,154,155,156,//F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
	128,129,130,131,//UP, DOWN, LEFT, RIGHT //52
	' ',9,0,0,132,127,134,135,136,137,//SPACE, TAB, SHIFT, CTRL, INS, DEL, HOME, END, PGUP, PGDN,
	8,27,13,10,158,0,//BACK, ESCAPE, RETURN, ENTER, PAUSE, SCROLL
	'0','1','2','3','4','5','6','7','8','9',//NP0, NP1, NP2, NP3, NP4, NP5, NP6, NP7, NP8, NP9
	'*','/','+','-','.',//NP_MUL, NP_DIV, NP_ADD, NP_SUB, NP_DECIMAL, 
	'.','=',',','-',//PERIOD, EQUALS, COMMA, MINUS //87
	';','/','\'','[','\\',']','#','`',//OEM_1, OEM_2, OEM_3, OEM_4, OEM_5, OEM_6, OEM_7, OEM_8//95
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//111
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//127
	0,
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	')','!','"','#','$','%','^','&','*','(',//36
	209,210,211,212,213,214,215,216,217,218,219,220, //175
	0,161,0,163,
	0,159,0,0,0,160,0,0,0,0,
	0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0, 
	'>','+','<','_',
	':','?','@','{','|','}','~',0, //223
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //239
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  //255
};
const unsigned char asciiUS[512] = {
	0,
	'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',//26
	'0','1','2','3','4','5','6','7','8','9',//K0, K1, K2, K3, K4, K5, K6, K7, K8, K9 //36
	145,146,147,148,149,150,151,152,153,154,155,156,//F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12 //48
	128,129,130,131,//UP, DOWN, LEFT, RIGHT //52
	' ',9,0,0,132,127,134,135,136,137,//SPACE, TAB, SHIFT, CTRL, INS, DEL, HOME, END, PGUP, PGDN, //62
	8,27,13,10,158,0,//BACK, ESCAPE, RETURN, ENTER, PAUSE, SCROLL //68
	'0','1','2','3','4','5','6','7','8','9',//NP0, NP1, NP2, NP3, NP4, NP5, NP6, NP7, NP8, NP9 //78
	'*','/','+','-','.',//NP_MUL, NP_DIV, NP_ADD, NP_SUB, NP_DECIMAL,  //83
	'.','=',',','-',//PERIOD, EQUALS, COMMA, MINUS //87
	';','/','`','[','\\',']','\'','`',//OEM_1, OEM_2, OEM_3, OEM_4, OEM_5, OEM_6, OEM_7, OEM_8//95
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//111
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//127
	0, //128
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',  //26
	')','!','@','#','$','%','^','&','*','(',//36
	209,210,211,212,213,214,215,216,217,218,219,220, //48
	0,161,0,163,  //52
	0,159,0,0,0,160,0,0,0,0, //62
	0,0,0,0,0,0, //68
	0,0,0,0,0,0,0,0,0,0,//78
	0,0,0,0,0, //83
	'>','+','<','_', //87
	':','?','~','{','|','}','"',0, //95
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //111
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  //127
};
const unsigned char asciiDE[256] = {
	0,
	'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',//28
	'0','1','2','3','4','5','6','7','8','9',//K0, K1, K2, K3, K4, K5, K6, K7, K8, K9 //36
	145,146,147,148,149,150,151,152,153,154,155,156,//F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12 //48
	128,129,130,131,//UP, DOWN, LEFT, RIGHT //52
	' ',9,0,0,132,127,134,135,136,137,//SPACE, TAB, SHIFT, CTRL, INS, DEL, HOME, END, PGUP, PGDN, //62
	8,27,13,10,158,0,//BACK, ESCAPE, RETURN, ENTER, PAUSE, SCROLL //68
	'0','1','2','3','4','5','6','7','8','9',//NP0, NP1, NP2, NP3, NP4, NP5, NP6, NP7, NP8, NP9 //78
	'*','/','+','-','.',//NP_MUL, NP_DIV, NP_ADD, NP_SUB, NP_DECIMAL,  //83
	'.','+',',','-',//PERIOD, EQUALS, COMMA, MINUS //87
	0,'#',0,0,'^','\'',0,'`',//OEM_1, OEM_2, OEM_3, OEM_4, OEM_5, OEM_6, OEM_7, OEM_8//95
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//111
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//127
	0, //128
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',  //28
	'=','!','"','^','$','%','&','/','(',')',//36
	209,210,211,212,213,214,215,216,217,218,219,220, //48
	0,161,0,163,  //52
	0,159,0,0,0,160,0,0,0,0, //62
	0,0,0,0,0,0, //68
	0,0,0,0,0,0,0,0,0,0,//78
	0,0,0,0,0, //83
	':','*',';','_', //87
	0,'\'',0,'?','`','`',0,0, //95
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //111
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  //127
};
const unsigned char asciiFR[256] = {
	0,
	'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',//26
	0,'&',0,'"','\'','(','-',0,'_',0,//36
	145,146,147,148,149,150,151,152,153,154,155,156,//F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12 //48
	128,129,130,131,//UP, DOWN, LEFT, RIGHT //52
	' ',9,0,0,132,127,134,135,136,137,//SPACE, TAB, SHIFT, CTRL, INS, DEL, HOME, END, PGUP, PGDN, //62
	8,27,13,10,158,0,//BACK, ESCAPE, RETURN, ENTER, PAUSE, SCROLL //68
	'0','1','2','3','4','5','6','7','8','9',//NP0, NP1, NP2, NP3, NP4, NP5, NP6, NP7, NP8, NP9 //78
	'*','/','+','-','.',//NP_MUL, NP_DIV, NP_ADD, NP_SUB, NP_DECIMAL,  //83
	';','=',',','-',//PERIOD, EQUALS, COMMA, MINUS //87
	'$',':',0,')','*','^',0,'!',//OEM_1, OEM_2, OEM_3, OEM_4, OEM_5, OEM_6, OEM_7, OEM_8//95
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//111
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//127
	0, //128
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',  //26
	'0','1','2','3','4','5','6','7','8','9',//K0, K1, K2, K3, K4, K5, K6, K7, K8, K9 //36
	209,210,211,212,213,214,215,216,217,218,219,220, //48
	0,161,0,163,  //52
	0,159,0,0,0,160,0,0,0,0, //62
	0,0,0,0,0,0, //68
	0,0,0,0,0,0,0,0,0,0,//78
	0,0,0,0,0, //83
	'.','+','?','_', //87
	'^','/','%',96,0,0,'"',0, //95
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //111
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  //127
};
const unsigned char asciiSW[512] = {
	0,
	'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',//26
	'0','1','2','3','4','5','6','7','8','9',//K0, K1, K2, K3, K4, K5, K6, K7, K8, K9 //36
	145,146,147,148,149,150,151,152,153,154,155,156,//F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12 //48
	128,129,130,131,//UP, DOWN, LEFT, RIGHT //52
	' ',9,0,0,132,127,134,135,136,137,//SPACE, TAB, SHIFT, CTRL, INS, DEL, HOME, END, PGUP, PGDN, //62
	8,27,13,10,158,0,//BACK, ESCAPE, RETURN, ENTER, PAUSE, SCROLL //68
	'0','1','2','3','4','5','6','7','8','9',//NP0, NP1, NP2, NP3, NP4, NP5, NP6, NP7, NP8, NP9 //78
	'*','/','+','-','.',//NP_MUL, NP_DIV, NP_ADD, NP_SUB, NP_DECIMAL,  //83
	'.','+',',','-',//PERIOD, EQUALS, COMMA, MINUS //87
	0,'\'',0,'\'',0,0,0,'`',//OEM_1, OEM_2, OEM_3, OEM_4, OEM_5, OEM_6, OEM_7, OEM_8//95
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//111
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//127
	0, //128
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',  //26
	'=','!','"','#',0,'%','&','/','(',')',//36
	209,210,211,212,213,214,215,216,217,218,219,220, //48
	0,161,0,163,  //52
	0,159,0,0,0,160,0,0,0,0, //62
	0,0,0,0,0,0, //68
	0,0,0,0,0,0,0,0,0,0,//78
	0,0,0,0,0, //83
	':','?',';','_', //87
	'^','*',0,'`',0,0,0,0, //95
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //111
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  //127
};

unsigned char KEYLIFO[10] = { 0 };
int KEYLIFOpointer = 0;
static int ctrl = 0, shift = 0, alt = 0, altgr = 0, KE2 = 0, KDC = 0, K81 = 0;
int numlock=0, scrolllock=0, shiftlock = 0, lastchar=0, repeattime=0;
extern "C" void MMPrintString(char* s);
extern "C" void LIFOadd(int n) {
	int i, j = 0;
	for (i = 0; i < KEYLIFOpointer; i++) {
		if (KEYLIFO[i] != n) {
			KEYLIFO[j] = KEYLIFO[i];
			j++;
		}
	}
	KEYLIFO[j] = n;
	KEYLIFOpointer = j + 1;
	keytimer=0; //New character added so start the key repeat timer
}
extern "C" void LIFOremove(int n) {
	int i, j = 0;
	for (i = 0; i < KEYLIFOpointer; i++) {
		if (KEYLIFO[i] != n) {
			KEYLIFO[j] = KEYLIFO[i];
			j++;
		}
	}
	KEYLIFOpointer = j;
}
extern "C" void clearrepeat(void) {
	memset(KEYLIFO, 0, sizeof(KEYLIFO));
	KEYLIFOpointer = 0;
	repeattime = Option.RepeatStart;
}
/*extern "C"  void WINAPI EditPaste(VOID)
{
	HGLOBAL   hglb;
	LPTSTR    lptstr;
	HWND hwnd=NULL;
	{
		if (!IsClipboardFormatAvailable(CF_TEXT))
			return;
		if (!OpenClipboard(hwnd))
			return;

		hglb = GetClipboardData(CF_TEXT);
		if (hglb != NULL)
		{
			lptstr = (LPTSTR)GlobalLock(hglb);
			if (lptstr != NULL)
			{
				char* p = (char*)hglb;
				int lastp = 0;
				while (*p) {
					ConsoleRxBuf[ConsoleRxBufHead] = *p++;
					if (lastp == 13 && ConsoleRxBuf[ConsoleRxBufHead] != 10) {
						unsigned char c = ConsoleRxBuf[ConsoleRxBufHead];
						ConsoleRxBuf[ConsoleRxBufHead] = 10;
						ConsoleRxBufHead = (ConsoleRxBufHead + 1) % CONSOLE_RX_BUF_SIZE;     // advance the head of the queue
						if (ConsoleRxBufHead == ConsoleRxBufTail) {                           // if the buffer has overflowed
							ConsoleRxBufTail = (ConsoleRxBufTail + 1) % CONSOLE_RX_BUF_SIZE; // throw away the oldest char
						}
						ConsoleRxBuf[ConsoleRxBufHead] = c;
						lastp = 0;
					}
					else lastp = 0;
					if (ConsoleRxBuf[ConsoleRxBufHead] == 13)lastp = 13;
					ConsoleRxBufHead = (ConsoleRxBufHead + 1) % CONSOLE_RX_BUF_SIZE;     // advance the head of the queue
					if (ConsoleRxBufHead == ConsoleRxBufTail) {                           // if the buffer has overflowed
						ConsoleRxBufTail = (ConsoleRxBufTail + 1) % CONSOLE_RX_BUF_SIZE; // throw away the oldest char
					}
				}
				GlobalUnlock(hglb);
			}
		}
		CloseClipboard();

		return;
	}
}*/
extern "C" void processkey(int i, int mode) {
	unsigned char* ascii = (unsigned char*)asciiUK;
	if(Option.KeyboardConfig==CONFIG_US)ascii = (unsigned char*)asciiUS;
	if(Option.KeyboardConfig == CONFIG_DE)ascii = (unsigned char*)asciiDE;
	if (Option.KeyboardConfig == CONFIG_FR)ascii = (unsigned char*)asciiFR;
	if (Option.KeyboardConfig == CONFIG_SW)ascii = (unsigned char*)asciiSW;
	unsigned char c;
	if (i == 0xE2) {
		if (Option.KeyboardConfig == CONFIG_DE || Option.KeyboardConfig == CONFIG_SW) {
			if (GetKeyState(VK_RMENU) & 0x8000)c = '|';
			else if (GetKeyState(VK_SHIFT) & 0x8000)c = '>';
			else c = '<';
		}
		if (Option.KeyboardConfig == CONFIG_FR) {
			if (GetKeyState(VK_SHIFT) & 0x8000)c = '>';
			else c = '<';
		}
	}
	else if (GetKeyState(VK_RMENU) & 0x8000 &&
			(Option.KeyboardConfig == CONFIG_DE ||
			Option.KeyboardConfig == CONFIG_FR || 
			Option.KeyboardConfig == CONFIG_ES || 
			Option.KeyboardConfig == CONFIG_SW ||
			Option.KeyboardConfig == CONFIG_BE))
	{
		if (Option.KeyboardConfig == CONFIG_DE) {
			if (i == 34)c= '{';
			else if (i == 35)c = '[';
			else if (i == 36)c = ']';
			else if (i == 27)c = '}';
			else if (i == 91)c = '\\';
			else if (i == 17)c = '@';
			else if (i == 85)c = '~';
			else if (i == 93)c = '^';
		}
		if (Option.KeyboardConfig == CONFIG_SW) {
			if (i == 34)c = '{';
			else if (i == 35)c = '[';
			else if (i == 36)c = ']';
			else if (i == 27)c = '}';
			else if (i == 85)c = '\\';
			else if (i == 29)c = '@';
			else if (i == 88)c = '~';
			else if (i == 31)c = '$';
		}
		if (Option.KeyboardConfig == CONFIG_FR) {
			if (i == 29)     c = 126;// ~
			else if (i == 30)c = 35; // #
			else if (i == 31)c = 123;// {
			else if (i == 32)c = 91; // [
			else if (i == 33)c = 124;// |
			else if (i == 56)c = 96;// `
			else if (i == 35)c = 92; // '\'
			else if (i == 36)c = 94; // ^
			else if (i == 27)c = 64; // @
			else if (i == 91)c = 93; // ]
			else if (i == 85)c = 125;// }
		}
		if (Option.KeyboardConfig == CONFIG_ES) {
			if (i == 0x35)     c = 92;   // backslash
			else if (i == 0x1E)c = 124;  // |
			else if (i == 0x08)c = 0;    // €
			else if (i == 0x1F)c = 64;   // @
			else if (i == 0x20)c = 35;   // #
			else if (i == 0x21)c = 0;    // ~
			else if (i == 0x2F)c = 91;   // [
			else if (i == 0x30)c = 93;   // ]
			else if (i == 0x31)c = 125;  // }
			else if (i == 0x34)c = 123;  // {
		}
		if (Option.KeyboardConfig == CONFIG_BE) {
			if (i == 0x64)     c = 92;   // backslash
			else if (i == 0x20)c = 35;  // |
			else if (i == 0x2F)c = 91;    // €
			else if (i == 0x30)c = 93;   // @<><>
			else if (i == 0x31)c = 96;   // #
			else if (i == 0x34)c = 39;    // ~
			else if (i == 0x38)c = 126;   // [
		}
	} else if(mode!=-1) {
		if (i == olc::Key::X && shift && ctrl)SystemMode = MODE_QUIT;
		else if (ctrl && i >= olc::Key::A && i <= olc::Key::Z) c = i;
		else if (shift && !shiftlock) c = ascii[i + 128];
		else if (shift && shiftlock && !(i >= olc::Key::A && i <= olc::Key::Z)) c = ascii[i + 128];
		else if (shiftlock && !shift && i >= olc::Key::A && i <= olc::Key::Z)c = ascii[i + 128];
		else c = ascii[i];   // store the byte in the ring buffer
	}
/*	if (mode == 1 && c == 22) {
		EditPaste();
	}
	else */ {
		if (mode == 1) {
			repeattime = Option.RepeatStart;
			ConsoleRxBuf[ConsoleRxBufHead] = c;
			LIFOadd(c);
			if (BreakKey && ConsoleRxBuf[ConsoleRxBufHead] == BreakKey) {// if the user wants to stop the progran
				MMAbort = true;                                        // set the flag for the interpreter to see
				ConsoleRxBufHead = ConsoleRxBufTail;                    // empty the buffer
			}
			else if (ConsoleRxBuf[ConsoleRxBufHead] == keyselect && KeyInterrupt != NULL) {
				Keycomplete = 1;
			}
			else {
				ConsoleRxBufHead = (ConsoleRxBufHead + 1) % CONSOLE_RX_BUF_SIZE;     // advance the head of the queue
				if (ConsoleRxBufHead == ConsoleRxBufTail) {                           // if the buffer has overflowed
					ConsoleRxBufTail = (ConsoleRxBufTail + 1) % CONSOLE_RX_BUF_SIZE; // throw away the oldest char
				}
			}
		}
		else if (mode == 0) {
			repeattime = Option.RepeatStart;
			LIFOremove(c);
		}
		else {
			if (keytimer > repeattime && KEYLIFOpointer) {
				repeattime = Option.RepeatRate;
				keytimer = 0;
				if (KEYLIFO[KEYLIFOpointer - 1] >= ' ' || KEYLIFO[KEYLIFOpointer - 1] == '\b') {
					ConsoleRxBuf[ConsoleRxBufHead] = KEYLIFO[KEYLIFOpointer - 1];
					ConsoleRxBufHead = (ConsoleRxBufHead + 1) % CONSOLE_RX_BUF_SIZE;     // advance the head of the queue
					if (ConsoleRxBufHead == ConsoleRxBufTail) {                           // if the buffer has overflowed
						ConsoleRxBufTail = (ConsoleRxBufTail + 1) % CONSOLE_RX_BUF_SIZE; // throw away the oldest char
					}
				}
				else repeattime = Option.RepeatStart;
			}
		}
	}
}
void MMBasic::CheckKeyBoard(float fElapsedTime) {
	int state = 0;
	shiftlock = (GetKeyState(VK_CAPITAL) & 0x0001);
	numlock = (GetKeyState(VK_NUMLOCK) & 0x0001);
	scrolllock = (GetKeyState(VK_SCROLL) & 0x0001);
//	for (int i = 0; i < 255; i++) {\
//		if (j = GetKeyState(1)) {
//			PInt(1);	 PIntHC(j); PRet();
//		}
//	}
	if(GetKeyState(0xE2) & 0x8000) {
		KE2++;
		if(KE2==1)processkey(0xE2, 1);
	} else {
		if (KE2)processkey(0xE2, 0);
		KE2 = 0;
	}
	for (uint32_t i = 1; i < olc::Key::ENUM_END; i++) {
		int j=GetKeyState(i) & 0x8000;
		if (GetKey((olc::Key)i).bReleased) {
			if (i == olc::Key::SHIFT) {
				clearrepeat();
				state = 1;
				shift = 0;
				continue;
			}
			if (i == olc::Key::CTRL) {
				clearrepeat();
				state = 1;
				ctrl = 0;
				continue;
			}
			processkey(i, 0);
			state = 1;
		}
		if(GetKey((olc::Key)i).bPressed) {
			if (i == olc::Key::SHIFT) {
				clearrepeat();
				state = 1;
				shift = 1;
				continue;
			} 
			if (i == olc::Key::CTRL) {
				clearrepeat();
				state = 1;
				ctrl = 1;
				continue;
			}
			state = 1;
			processkey(i,1);
		}
	}
	if (state == 0)processkey(0, -1);
	mouse_xpos = GetMouseX();
	mouse_ypos = GetMouseY();
	mouse_wheel += GetMouseWheel();
	mouse_left = GetMouse(olc::Mouse::LEFT).bHeld;
	mouse_right = GetMouse(olc::Mouse::RIGHT).bHeld;
	mouse_middle = GetMouse(olc::Mouse::MIDDLE).bHeld;
	if (comused) {
		DWORD dNoOfBytesWritten = 0;
		unsigned char c;
		for (int i = 1; i < MAXCOMPORTS; i++) {
			if (comn[i]) {
				while (1) {
					ReadFile(hComm[i],        // Handle to the Serial port
						(char*)&c,     // Data to be written to the port
						1,  //No of bytes to write
						&dNoOfBytesWritten, //Bytes written
						NULL);
					if (!dNoOfBytesWritten)break;
					comRx_buf[i][comRx_head[i]] = c;   // store the byte in the ring buffer
					comRx_head[i] = (comRx_head[i] + 1) % com_buf_size[i];     // advance the head of the queue
					if (comRx_head[i] == comRx_tail[i]) {                           // if the buffer has overflowed
						comRx_tail[i] = (comRx_tail[i] + 1) % com_buf_size[i]; // throw away the oldest char
					}
				}
			}
		}
	}
}
