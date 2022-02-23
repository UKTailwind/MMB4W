/***********************************************************************************************************************
MMBasic for Windows

Basic.cpp

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
#include "MainThread.h"
#include <chrono>
#include "Draw.h"
#include "FileIO.h"
#include "MMBasic.h"
#include "Editor.h"
#include "Memory.h"
#include "Commands.h"
#include "MM_Misc.h"
#include "GUI.h"
#include "audio.h"
#include "Version.h"
#include <csetjmp>
int MMCharPos;
volatile int second = 15;                                            // date/time counters
volatile int minute = 50;
volatile int hour = 19;
volatile int day = 21;
volatile int month = 1;
volatile int year = 2022;
int GPSchannel = 0;
unsigned char WatchdogSet = false;
unsigned char IgnorePIN = false;
unsigned char* InterruptReturn=NULL;
int InterruptUsed=0;
volatile int64_t startfasttime; 
volatile int64_t fasttime;
volatile unsigned int WDTimer = 0;
volatile unsigned int SecondsTimer = 0;
volatile unsigned int PauseTimer = 0; 
volatile unsigned int IntPauseTimer = 0;
volatile long long int mSecTimer =0; 
volatile unsigned int AHRSTimer = 0;
volatile unsigned int MouseTimer = 0;
volatile unsigned int MouseProcTimer = 0;
volatile unsigned int ScrewUpTimer = 0;
volatile int keytimer;
volatile int TOUCH_DOWN = 0;
int64_t frequency;
extern volatile int CursorTimer;
volatile int mouse_xpos, mouse_ypos, mouse_wheel, mouse_left, mouse_right, mouse_middle;
unsigned char lastcmd[STRINGSIZE * 16];                                           // used to store the last command in case it is needed by the EDIT command
unsigned char EchoOption = true;
const char DaysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
int PromptFont = 1;
int64_t PromptFC = M_WHITE, PromptBC = M_BLACK;                             // the font and colours selected at the prompt
jmp_buf mark;                                                       // longjump to recover from an error and abort
char errstring[256] = { 0 };
int errpos = 0;
int OptionHeight;
int OptionWidth;

using clock_type = std::chrono::high_resolution_clock;
using namespace std::literals; 
extern "C" void CheckAbort(void) {
    if (CurrentlyPlaying == P_WAV || CurrentlyPlaying == P_FLAC || CurrentlyPlaying == P_MP3 || CurrentlyPlaying == P_MOD)checkWAVinput();
    if (_excep_code) {
        SoftReset();
    }
    if(MMAbort) {
        WDTimer = 0;                                                // turn off the watchdog timer
        memset(inpbuf, 0, STRINGSIZE);
        ShowCursor(false);
        longjmp(mark, 1);                                           // jump back to the input prompt
    }
}
extern "C" int getConsole(int speed) {
    int c = -1;
    CheckAbort();
    if(ConsoleRxBufHead != ConsoleRxBufTail) {                            // if the queue has something in it
        c = ConsoleRxBuf[ConsoleRxBufTail];
        ConsoleRxBufTail = (ConsoleRxBufTail + 1) % CONSOLE_RX_BUF_SIZE;   // advance the head of the queue
    }
    else if(!speed){
        auto when_started = clock_type::now();
        auto target_time = when_started + 1ms;
        std::this_thread::sleep_until(target_time);
    }
    return c;
}

extern "C" unsigned char MMputchar(unsigned char c) {
    if (ConsoleRepeat) {
        char b[2] = { 0 };
        b[0] = c;
        std::cout << b;
    }
    DisplayPutC(c);
    if(isprint((unsigned char)c)) MMCharPos++;
    if(c == '\r') {
        MMCharPos = 1;
    }
    return c;
}
// get a keystroke.  Will wait forever for input
// if the unsigned char is a cr then replace it with a newline (lf)
extern "C" int MMgetchar(void) {
    int c;
    do {
        ProcessTouch();                                             // check GUI touch
        ShowMMBasicCursor(1);
        c = getConsole(0);
    } while (c == -1);
    ShowMMBasicCursor(0);
    return c;
}
// returns the number of character waiting in the console input queue
extern "C" int kbhitConsole(void) {
    int i;
    i = ConsoleRxBufHead - ConsoleRxBufTail;
    if (i < 0) i += CONSOLE_RX_BUF_SIZE;
    return i;
}

// get a line from the keyboard or a serial file handle
// filenbr == 0 means the console input
extern "C" void MMgetline(int filenbr, char* p) {
    int c, nbrchars = 0;
    char* tp;
    while (1) {
        CheckAbort();												// jump right out if CTRL-C
        if(FileTable[filenbr].com > MAXCOMPORTS && feof(FileTable[filenbr].fptr)) break;
        c = MMfgetc(filenbr);
        if(c <= 0) continue;                                       // keep looping if there are no chars

        // if this is the console, check for a programmed function key and insert the text
        if(filenbr == 0) {
            tp = NULL;
            if(c == F1)  tp = (char*)"FILES";
            if(c == F2)  tp = (char*)"RUN";
            if(c == F3)  tp = (char*)"LIST";
            if(c == F4)  tp = (char*)"EDIT";
            if(c == F10) tp = (char*)"AUTOSAVE";
            if(c == F11) tp = (char*)"XMODEM RECEIVE";
            if(c == F12) tp = (char*)"XMODEM SEND";
            if(c == F5) tp = (char*)Option.F5key;
            if(c == F6) tp = (char*)Option.F6key;
            if(c == F7) tp = (char*)Option.F7key;
            if(c == F8) tp = (char*)Option.F8key;
            if(c == F9) tp = (char*)Option.F9key;
            if(tp) {
                strcpy(p, tp);
                if(EchoOption) { MMPrintString(tp); MMPrintString((char*)"\r\n"); }
                return;
            }
        }

        if(c == '\t') {												// expand tabs to spaces
            do {
                if(++nbrchars > MAXSTRLEN) error((char *)"Line is too long");
                *p++ = ' ';
                if(filenbr == 0 && EchoOption) MMputchar(' ');
            } while (nbrchars % 4);
            continue;
        }

        if(c == '\b') {												// handle the backspace
            if(nbrchars) {
                if(filenbr == 0 && EchoOption) MMPrintString((char*)"\b \b");
                nbrchars--;
                p--;
            }
            continue;
        }

        if(c == '\n') {                                             // what to do with a newline
            break;                                              // a newline terminates a line (for a file)
        }

        if(c == '\r') {
            if(filenbr == 0 && EchoOption) {
                MMPrintString((char*)"\r\n");
                break;                                              // on the console this means the end of the line - stop collecting
            }
            else
                continue;                                          // for files loop around looking for the following newline
        }

        if(isprint(c)) {
            if(filenbr == 0 && EchoOption) MMputchar(c);           // The console requires that chars be echoed
        }
        if(++nbrchars > MAXSTRLEN) error((char*)"Line is too long");		// stop collecting if maximum length
        *p++ = c;													// save our char
    }
    *p = 0;
}

void InsertLastcmd(unsigned char* s) {
    int i, slen;
    if(strcmp((const char*)lastcmd, (const char*)s) == 0) return;                             // don't duplicate
    slen = strlen((const char*)s);
    if(slen < 1 || slen > STRINGSIZE * 4 - 1) return;
    slen++;
    for (i = STRINGSIZE * 4 - 1; i >= slen; i--)
        lastcmd[i] = lastcmd[i - slen];                             // shift the contents of the buffer up
    strcpy((char *)lastcmd, (const char *)s);                                             // and insert the new string in the beginning
    for (i = STRINGSIZE * 4 - 1; lastcmd[i]; i--) lastcmd[i] = 0;             // zero the end of the buffer
}
void EditInputLine(void) {
    char* p = NULL;
    char buf[MAXKEYLEN + 3] = { 0 };
    int lastcmd_idx, lastcmd_edit;
    int insert, startline, maxchars;
    int CharIndex, BufEdited;
    int c, i, j;
    maxchars = OptionWidth;
    if((int)strlen((const char *)inpbuf) >= maxchars) {
        MMPrintString((char*)inpbuf);
        error((char *)"Line is too long to edit");
    }
    startline = MMCharPos - 1;                                                          // save the current cursor position
    MMPrintString((char*)inpbuf);                                                              // display the contents of the input buffer (if any)
    CharIndex = (int)strlen((const char*)inpbuf);                                                         // get the current cursor position in the line
    insert = false;
    //    Cursor = C_STANDARD;
    lastcmd_edit = lastcmd_idx = 0;
    BufEdited = false; //(CharIndex != 0);
    while (1) {
        c = MMgetchar();
        if(c == TAB) {
            strcpy(buf, "        ");
            switch (Option.Tab) {
            case 2:
                buf[2 - (CharIndex % 2)] = 0; break;
            case 3:
                buf[3 - (CharIndex % 3)] = 0; break;
            case 4:
                buf[4 - (CharIndex % 4)] = 0; break;
            case 8:
                buf[8 - (CharIndex % 8)] = 0; break;
            }
        }
        else {
            buf[0] = c;
            buf[1] = 0;
        }
        do {
            switch (buf[0]) {
            case '\r':
            case '\n':  //if(autoOn && atoi(inpbuf) > 0) autoNext = atoi(inpbuf) + autoIncr;
                        //if(autoOn && !BufEdited) *inpbuf = 0;
                goto saveline;
                break;

            case '\b':  
                if(CharIndex > 0) {
                    BufEdited = true;
                    i = CharIndex - 1;
                    for (p = (char*)inpbuf + i; *p; p++) *p = *(p + 1);                 // remove the char from inpbuf
                    while (CharIndex) { MMputchar('\b'); CharIndex--; }         // go to the beginning of the line
                    MMPrintString((char*)inpbuf); MMputchar(' '); MMputchar('\b');     // display the line and erase the last char
                    for (CharIndex = strlen((const char*)inpbuf); CharIndex > i; CharIndex--)
                        MMputchar('\b');
                }
                     break;

            case CTRLKEY('S'):
            case (char)LEFT:  
                if(CharIndex > 0) {
                if(CharIndex == strlen((const char*)inpbuf)) {
                    insert = true;
                    //                              Cursor = C_INSERT;
                }
                MMputchar('\b');
                CharIndex--;
            }
                     break;

            case CTRLKEY('D'):
            case (char)RIGHT: if(CharIndex < (int)strlen((const char*)inpbuf)) {
                MMputchar(inpbuf[CharIndex]);
                CharIndex++;
            }
                      break;

            case CTRLKEY(']'):
            case (char)DEL:   if(CharIndex < (int)strlen((const char*)inpbuf)) {
                BufEdited = true;
                i = CharIndex;
                for (p = (char *)inpbuf + i; *p; p++) *p = *(p + 1);                 // remove the char from inpbuf
                while (CharIndex) { MMputchar('\b'); CharIndex--; }         // go to the beginning of the line
                MMPrintString((char*)inpbuf); MMputchar(' '); MMputchar('\b');     // display the line and erase the last char
                for (CharIndex = strlen((const char*)inpbuf); CharIndex > i; CharIndex--)
                    MMputchar('\b');
            }
                    break;

            case CTRLKEY('N'):
            case (char)INSERT:insert = !insert;
                //                            Cursor = C_STANDARD + insert;
                break;

            case CTRLKEY('U'):
            case (char)HOME:  if(CharIndex > 0) {
                if(CharIndex == strlen((const char*)inpbuf)) {
                    insert = true;
                    //                                    Cursor = C_INSERT;
                }
                while (CharIndex) { MMputchar('\b'); CharIndex--; }
            }
                     break;

            case CTRLKEY('K'):
            case (char)END:   while (CharIndex < (int)strlen((const char*)inpbuf))
                MMputchar(inpbuf[CharIndex++]);
                break;

             case (char)0x91:
                strcpy(&buf[1], (char*)"FILES\r\n");
                break;
            case (char)0x92:
                strcpy(&buf[1], (char*)"RUN\r\n");
                break;
            case (char)0x93:
                strcpy(&buf[1], (char*)"LIST\r\n");
                break;
            case (char)0x94:
                strcpy(&buf[1], (char*)"EDIT\r\n");
                break;
            case (char)0x95:
                if(*Option.F5key)strcpy(&buf[1], (char*)Option.F5key);
                break;
            case (char)0x96:
                if(*Option.F6key)strcpy(&buf[1], (char*)Option.F6key);
                break;
            case (char)0x97:
                if(*Option.F7key)strcpy(&buf[1], (char*)Option.F7key);
                break;
            case (char)0x98:
                if(*Option.F8key)strcpy(&buf[1], (char*)Option.F8key);
                break;
            case (char)0x99:
                if(*Option.F9key)strcpy(&buf[1], (char*)Option.F9key);
                break;
            case (char)0x9a:
                strcpy(&buf[1], (char*)"AUTOSAVE\r\n");
                break;
            case (char)0x9b:
                strcpy(&buf[1], (char*)"XMODEM RECEIVE\r\n");
                break;
            case (char)0x9c:
                strcpy(&buf[1], (char*)"XMODEM SEND\r\n");
                break;
            case CTRLKEY('E'):
            case (char)UP:    if(!(BufEdited /*|| autoOn || CurrentLineNbr */)) {
                while (CharIndex) { MMputchar('\b'); CharIndex--; }
                if(lastcmd_edit) {
                    i = lastcmd_idx + strlen((const char*)&lastcmd[lastcmd_idx]) + 1;    // find the next command
                    if(lastcmd[i] != 0 && i < STRINGSIZE * 4 - 1) lastcmd_idx = i;  // and point to it for the next time around
                }
                else
                    lastcmd_edit = true;
                strcpy((char*)inpbuf, (const char*)&lastcmd[lastcmd_idx]);                      // get the command into the buffer for editing
                goto insert_lastcmd;
            }
                   break;


            case CTRLKEY('X'):
            case (char)DOWN:  if(!(BufEdited /*|| autoOn || CurrentLineNbr */)) {
                while (CharIndex) { MMputchar('\b'); CharIndex--; }
                 if(lastcmd_idx == 0)
                    *inpbuf = lastcmd_edit = 0;
                else {
                    for (i = lastcmd_idx - 2; i > 0 && lastcmd[i - 1] != 0; i--);// find the start of the previous command
                    lastcmd_idx = i;                                        // and point to it for the next time around
                    strcpy((char*)inpbuf, (const char*)&lastcmd[i]);                            // get the command into the buffer for editing
                }
                goto insert_lastcmd;                                        // gotos are bad, I know, I know
            }
                     break;

                 insert_lastcmd:                                                             // goto here if we are just editing a command
                     if((int)strlen((const char*)inpbuf) + startline >= maxchars) {                    // if the line is too long
                         while (CharIndex) { MMputchar('\b'); CharIndex--; }         // go to the start of the line
                         MMPrintString((char*)inpbuf);                                      // display the offending line
                         error((char*)"Line is too long to edit");
                     }
                     MMPrintString((char*)inpbuf);                                          // display the line
                     CharIndex = strlen((const char*)inpbuf);                                     // get the current cursor position in the line
                           for(i = 1; i <= maxchars - (int)strlen((const char*)inpbuf) - startline; i++) {
                                MMputchar(' ');                                             // erase the rest of the line
                                CharIndex++;
                            }
                            while(CharIndex > (int)strlen((const char*)inpbuf)) { MMputchar('\b'); CharIndex--; } // return the cursor to the right position

                     break;

            default:    if((unsigned char)buf[0] >= (unsigned char)' ' && (unsigned char)buf[0] < (unsigned char)0x7f) {
                BufEdited = true;                                           // this means that something was typed
                i = CharIndex;
                j = strlen((const char*)inpbuf);
                if(insert) {
                    if((int)strlen((const char*)inpbuf) >= maxchars - 1) break;               // sorry, line full
                    for (p = (char*)inpbuf + strlen((const char*)inpbuf); j >= CharIndex; p--, j--) *(p + 1) = *p;
                    inpbuf[CharIndex] = buf[0];                             // insert the char
                    MMPrintString((char*)&inpbuf[CharIndex]);                      // display new part of the line
                    CharIndex++;
                    for (j = strlen((const char*)inpbuf); j > CharIndex; j--)
                        MMputchar('\b');
                }
                else {
                    inpbuf[strlen((const char*)inpbuf) + 1] = 0;                         // incase we are adding to the end of the string
                    inpbuf[CharIndex++] = buf[0];                           // overwrite the char
                    MMputchar(buf[0]);                                      // display it
                    if(CharIndex + startline >= maxchars) {                 // has the input gone beyond the end of the line?
                        MMgetline(0, (char*)inpbuf);                               // use the old fashioned way of getting the line
                        //if(autoOn && atoi(inpbuf) > 0) autoNext = atoi(inpbuf) + autoIncr;
                        goto saveline;
                    }
                }
            }
                   break;
            }
            for (i = 0; i < MAXKEYLEN + 1; i++) buf[i] = buf[i + 1];                             // shuffle down the buffer to get the next char
            if ((int)strlen((char *)inpbuf) >= xres[VideoMode] / gui_font_width - 3) { beep(500, 650); }
        } while (*buf);
        if(CharIndex == strlen((const char*)inpbuf)) {
            insert = false;
            //        Cursor = C_STANDARD;
        }
    }

saveline:
    //    Cursor = C_STANDARD;
    MMPrintString((char*)"\r\n");
}

extern "C" void uSec(int t) {
    auto when_started = clock_type::now();
    auto target_time = when_started + std::chrono::microseconds(t); 
    std::this_thread::sleep_until(target_time);
}
void MouseProc(void) {
    static int lastl = 0, lastr=0, lastxclick = 0, lastyclick = 0;
    TOUCH_DOWN = mouse_left;
    if (MouseTimer > 1000) MouseDouble = 0;
    if (lastl == 0 && mouse_left) {
        int xmove = abs(lastxclick - mouse_xpos);
        int ymove = abs(lastyclick - mouse_ypos);
        lastl = 1;
        if (MouseTimer >= 500 || MouseTimer < 100 || xmove>10 || ymove > 10) MouseTimer=0;
        else {
            MouseDouble = 1;
            MouseTimer = 500;
        }
        MouseFoundLeftDown = 1;
        lastxclick = mouse_xpos;
        lastyclick = mouse_ypos;
    }
    if (lastr == 0 && mouse_right) {
        lastr = 1;
        MouseFoundRightDown = 1;
    }
    if (lastl == 1 && mouse_left==0) {
        lastl = 0;
        MouseFoundLeftUp = 1;
    }
    if (mouse_left == 0) lastl = 0;
    if (mouse_right == 0) lastr = 0;
}
DWORD WINAPI Basic(LPVOID lpParameter)
{
    static int ErrorInPrompt;
    clearrepeat();
    LoadOptions();
    HRes = Option.hres;
    VRes = Option.vres;
    OptionHeight = Option.Height;
    OptionWidth = Option.Width;
    strcpy(lastfileedited, Option.lastfilename);
    WDTimer = 0;
    SecondsTimer = 0;
    PauseTimer = 0;
    IntPauseTimer = 0;
    mSecTimer = 0;
    AHRSTimer = 0;
    MouseTimer = 0;
    MouseProcTimer = 0;
    ScrewUpTimer = 0;
    TOUCH_DOWN = 0;
    EchoOption = true;
    PromptFont = 1;
    PromptFC = M_WHITE, PromptBC = M_BLACK;                             // the font and colours selected at the prompt
    memset(errstring,0,256);
    memset(lastcmd, 0, sizeof(lastcmd));
    memset(FrameBuffer, 0, FRAMEBUFFERSIZE);
    errpos = 0;
    FullScreen = Option.fullscreen;
    memset(inpbuf, 0, STRINGSIZE);
    memset(tknbuf, 0, STRINGSIZE);
    PixelSize = Option.pixelnum;
    for (int i = 0; i <= MAXPAGES; i++) {
        PageTable[i].address = getpageaddress(i);
        PageTable[i].xmax = HRes;
        PageTable[i].ymax = VRes;
        PageTable[i].size = SCREENSIZE;
    }
    PageWrite1 = PageTable[1].address;
    m_alloc(M_PROG);                                           // init the variables for program memory
    m_alloc(M_VAR);                                           // init the variables for program memory
    ConsoleRxBufHead = 0;
    ConsoleRxBufTail = 0;
    mSecTimer = 0;
    InitHeap();              										// initilise memory allocation
    OptionErrorSkip = false;
    InitBasic();
    *tknbuf = 0;
    ContinuePoint = nextstmt;                               // in case the user wants to use the continue command
    ErrorInPrompt = false;
    CurrentX = CurrentY = 0;
    PromptFont = Option.DefaultFont;
    SetFont(PromptFont);
    gui_fcolour = PromptFC;
    gui_bcolour = PromptBC;
    VideoMode = Option.mode;
    QueryPerformanceFrequency((LARGE_INTEGER*) &frequency);
    QueryPerformanceCounter((LARGE_INTEGER*) &startfasttime);
    while (mSecTimer < 1000) {}
    while (!DisplayActive) {}
    if (!(_excep_code == RESTART_NOAUTORUN || _excep_code == WATCHDOG_TIMEOUT || _excep_code == SCREWUP_TIMEOUT)) {
        if (Option.Autorun == 0) {
            if (!(_excep_code == RESET_COMMAND))MMPrintString((char *)MES_SIGNON);                           // print sign on message
        }
    }
    if (_excep_code == WATCHDOG_TIMEOUT) {
        WatchdogSet = true;                                 // remember if it was a watchdog timeout
        MMPrintString((char*)"\rWatchdog timeout");
    }
    if (_excep_code == SCREWUP_TIMEOUT) {
        MMPrintString((char*)"\rCommand timeout");
    }
    _excep_code = 0;
    SetCurrentDirectoryA(Option.defaultpath);
    if(setjmp(mark) != 0) {
        // we got here via a long jump which means an error or CTRL-C or the program wants to exit to the command prompt
        if (CurrentlyPlaying != P_NOTHING)CloseAudio();
        optionangle = 1.0;
        optiony = 0;
        ScrewUpTimer = 0;
        MMAbort = 0;
        clearrepeat();
        WritePage=0;
        ReadPage=0;
        if(VideoMode==Option.mode)SetFont(Option.DefaultFont);
        else SetFont(defaultfont[VideoMode]);
        PromptFC = M_WHITE; PromptBC = M_BLACK;                             // the font and colours selected at the prompt
        gui_fcolour = PromptFC;
        gui_bcolour = PromptBC;
        //        setmode(Option.mode, 0, Option.fullscreen);
        CurrentX = 0;
        if (errpos) {
            CloseAllFiles();
            MMPrintString(errstring);
            memset(inpbuf, 0, STRINGSIZE);
        }
        errpos = 0;
        errstring[0] = 0;
    }
    else {
        if (lpParameter != NULL) {
            strcpy((char*)tknbuf, (char*)lpParameter);
            if (strchr((char*)tknbuf, '.') == NULL) strcat((char*)tknbuf, ".BAS");
            if (existsfile((char *)tknbuf)) {
                strcpy((char*)inpbuf, "RUN \"");
                strcat((char*)inpbuf, (char*)tknbuf);
                strcat((char*)inpbuf, "\"");
                memset(tknbuf, 0, STRINGSIZE);
                tokenise(true);
                ExecuteProgram(tknbuf);                                     // execute the line straight away
                memset(inpbuf, 0, STRINGSIZE);
            }
        }
        if (*lastfileedited && Option.Autorun) {
            strcpy((char*)tknbuf, lastfileedited);
            if (strchr((char*)tknbuf, '.') == NULL) strcat((char*)tknbuf, ".BAS");
            if (existsfile((char*)tknbuf)) {
                strcpy((char*)inpbuf, "RUN \"");
                strcat((char*)inpbuf, (char*)tknbuf);
                strcat((char*)inpbuf, "\"");
                memset(tknbuf, 0, STRINGSIZE);
                tokenise(true);
                ExecuteProgram(tknbuf);                                     // execute the line straight away
                memset(inpbuf, 0, STRINGSIZE);
            }
        }
    }
    while (!(SystemMode == MODE_STOPTHREADS || SystemMode == MODE_SOFTRESET)) {
        MMAbort = false;
        BreakKey = BREAK_KEY;
        EchoOption = true;
        LocalIndex = 0;                                             // this should not be needed but it ensures that all space will be cleared
        ClearTempMemory();                                          // clear temp string space (might have been used by the prompt)
        CurrentLinePtr = NULL;                                      // do not use the line number in error reporting
        if (MMCharPos > 1) MMPrintString((char *)"\r\n");                    // prompt should be on a new line
        PrepareProgram(false);
        if (!ErrorInPrompt && FindSubFun((unsigned char *)"MM.PROMPT", 0) >= 0) {
            ErrorInPrompt = true;
            ExecuteProgram((unsigned char *)"MM.PROMPT\0");
        }
        else
            MMPrintString((char *)"> ");                                    // print the prompt
        ErrorInPrompt = false;
        EditInputLine();
        InsertLastcmd(inpbuf);                                  // save in case we want to edit it later
//        MMgetline(0, inpbuf);                                       // get the input
        if (!*inpbuf) continue;                                      // ignore an empty line
        char* p = (char *)inpbuf;
        skipspace(p);
        if (*p == '*') { //shortform RUN command so convert to a normal version
            memmove(&p[4], &p[0], strlen(p) + 1);
            p[0] = 'R'; p[1] = 'U'; p[2] = 'N'; p[3] = '$'; p[4] = 34;
            char* q;
            if ((q = strchr(p, ' ')) != 0) { //command line after the filename
                *q = ','; //chop the command at the first space character
                memmove(&q[1], &q[0], strlen(q) + 1);
                q[0] = 34;
            }
            else strcat(p, "\"");
            p[3] = ' ';
            //		  PRet();MMPrintString(inpbuf);PRet();
        }
        tokenise(true);                                             // turn into executable code
        if (setjmp(jmprun) != 0) {
            PrepareProgram(false);
            CurrentLinePtr = 0;
        }
        ExecuteProgram(tknbuf);                                     // execute the line straight away
        memset(inpbuf, 0, STRINGSIZE);
    }
	return 0;
}
DWORD WINAPI mClock(LPVOID lpParameter)
{
	auto when_started = clock_type::now();
	auto target_time = when_started + 500ms;
    while (SystemMode != MODE_STOPTHREADS) {
		std::this_thread::sleep_until(target_time);
		target_time += 1ms;
        if(++CursorTimer > CURSOR_OFF + CURSOR_ON) CursorTimer = 0;		// used to control cursor blink rate
        PauseTimer++;                                                     // used by the PAUSE command
        mSecTimer++; 
        AHRSTimer++;
        MouseTimer++;
        MouseProcTimer++;
        TouchTimer++;
        keytimer++;
        if (ScrewUpTimer) {
            if (--ScrewUpTimer == 0) {
                _excep_code = SCREWUP_TIMEOUT;
            }
        }
        if (WDTimer) {
            if (--WDTimer == 0) {
                _excep_code = WATCHDOG_TIMEOUT;
            }
        }

        if (MouseProcTimer % 20 == 0)MouseProc();
        if (InterruptUsed) {
            int i;
            for (i = 0; i < NBRSETTICKS; i++) if (TickActive[i])TickTimer[i]++;			// used in the interrupt tick
        }
        if (TOUCH_DOWN) {                                            // is the pen down
            if (!TouchState) {                                       // yes, it is.  If we have not reported this before
                TouchState = TouchDown = true;                      // set the flags
                TouchUp = false;
            }
        }
        else {
            if (TouchState) {                                        // the pen is not down.  If we have not reported this before
                TouchState = TouchDown = false;                     // set the flags
                TouchUp = true;
            }
        }
        if (ClickTimer) {
            ClickTimer--;
            if(!ClickTimer)bSynthPlaying = false;
        }

    }
	return 0;
}
volatile union u_flash {
    uint64_t i64[4];
    uint8_t  i8[32];
    uint32_t  i32[8];
} MemWord;
volatile int mi8p = 0;
// globals used when writing bytes to flash
volatile uint32_t realmempointer;
void MemWriteBlock(void) {
    int i;
    uint32_t address = realmempointer - 32;
    //    if(address % 32)error((char *)"Memory write address");
    memcpy((char*)address, (char*)&MemWord.i64[0], 32);
    for (i = 0; i < 8; i++)MemWord.i32[i] = 0xFFFFFFFF;
}
void MemWriteByte(unsigned char b) {
    realmempointer++;
    MemWord.i8[mi8p] = b;
    mi8p++;
    mi8p %= 32;
    if (mi8p == 0) {
        MemWriteBlock();
    }
}
void MemWriteWord(unsigned int i) {
    MemWriteByte(i & 0xFF);
    MemWriteByte((i >> 8) & 0xFF);
    MemWriteByte((i >> 16) & 0xFF);
    MemWriteByte((i >> 24) & 0xFF);
}

void MemWriteAlign(void) {
    while (mi8p != 0) {
        MemWriteByte(0x0);
    }
    MemWriteWord(0xFFFFFFFF);
}



void MemWriteClose(void) {
    while (mi8p != 0) {
        MemWriteByte(0xff);
    }

}

// takes a pointer to RAM containing a program (in clear text) and writes it to memory in tokenised format
extern "C" void SaveProgramToMemory(unsigned char* pm, int msg) {
    unsigned char* p, endtoken, fontnbr, prevchar = 0, buf[STRINGSIZE];
    int nbr, i, n, SaveSizeAddr;
    uint32_t storedupdates[MAXCFUNCTION] = { 0 }, updatecount = 0, realmemsave;
    int firsthex = 1;
    memcpy(buf, tknbuf, STRINGSIZE);                                // save the token buffer because we are going to use it
    realmempointer = (uint32_t)ProgMemory;
    memset(ProgMemory, 0xFF, MAX_PROG_SIZE);
    clearrepeat();
    nbr = 0;
    // this is used to count the number of bytes written to flash
    while (*pm) {
        p = inpbuf;
        while (!(*pm == 0 || *pm == '\r' || (*pm == '\n' && prevchar != '\r'))) {
            if (*pm == TAB) {
                do {
                    *p++ = ' ';
                    if ((p - inpbuf) >= MAXSTRLEN) goto exiterror1;
                } while ((p - inpbuf) % 2);
            }
            else {
                if (isprint((uint8_t)*pm)) {
                    *p++ = *pm;
                    if ((p - inpbuf) >= MAXSTRLEN) goto exiterror1;
                }
            }
            prevchar = *pm++;
        }
        if (*pm) prevchar = *pm++;                                   // step over the end of line char but not the terminating zero
        *p = 0;                                                     // terminate the string in inpbuf

        if (*inpbuf == 0 && (*pm == 0 || (!isprint((uint8_t)*pm) && pm[1] == 0))) break; // don't save a trailing newline

        tokenise(false);                                            // turn into executable code
        p = tknbuf;
        while (!(p[0] == 0 && p[1] == 0)) {
            MemWriteByte(*p++); nbr++;

            if ((int)((char*)realmempointer - (uint32_t)ProgMemory) >= MAX_PROG_SIZE - 5) error((char *)"Not enough memory");
            //                goto exiterror1;
        }
        MemWriteByte(0); nbr++;                              // terminate that line in flash
    }
    MemWriteByte(0);
    MemWriteAlign();                                            // this will flush the buffer and step the flash write pointer to the next word boundary
    // now we must scan the program looking for CFUNCTION/CSUB/DEFINEFONT statements, extract their data and program it into the flash used by  CFUNCTIONs
     // programs are terminated with two zero bytes and one or more bytes of 0xff.  The CFunction area starts immediately after that.
     // the format of a CFunction/CSub/Font in flash is:
     //   Unsigned Int - Address of the CFunction/CSub in program memory (points to the token representing the "CFunction" keyword) or NULL if it is a font
     //   Unsigned Int - The length of the CFunction/CSub/Font in bytes including the Offset (see below)
     //   Unsigned Int - The Offset (in words) to the main() function (ie, the entry point to the CFunction/CSub).  Omitted in a font.
     //   word1..wordN - The CFunction/CSub/Font code
     // The next CFunction/CSub/Font starts immediately following the last word of the previous CFunction/CSub/Font
    realmemsave = realmempointer;
    p = (unsigned char*)ProgMemory;                                              // start scanning program memory
    while (*p != 0xff) {
        nbr++;
        if (*p == 0) p++;                                            // if it is at the end of an element skip the zero marker
        if (*p == 0) break;                                          // end of the program
        if (*p == T_NEWLINE) {
            CurrentLinePtr = p;
            p++;                                                    // skip the newline token
        }
        if (*p == T_LINENBR) p += 3;                                 // step over the line number

        skipspace(p);
        if (*p == T_LABEL) {
            p += p[1] + 2;                                          // skip over the label
            skipspace(p);                                           // and any following spaces
        }
        if (/**p == cmdCSUB ||*/ *p == GetCommandValue((unsigned char*)"DefineFont")) {      // found a CFUNCTION, CSUB or DEFINEFONT token
            if (*p == GetCommandValue((unsigned char*)"DefineFont")) {
                endtoken = GetCommandValue((unsigned char*)"End DefineFont");
                p++;                                                // step over the token
                skipspace(p);
                if (*p == '#') p++;
                fontnbr = (unsigned char)getint(p, 1, FONT_TABLE_SIZE);
                // font 6 has some special characters, some of which depend on font 1
                if (fontnbr == 1 || fontnbr == 6 || fontnbr == 7) error((char *)"Cannot redefine fonts 1, 6 or 7");
                realmempointer += 4;
                skipelement(p);                                     // go to the end of the command
                p--;
            }
//            else {
//                endtoken = GetCommandValue((unsigned char*)"End CSub");
//                realmempointer += 4;
//                fontnbr = 0;
//                firsthex = 0;
//            }
            SaveSizeAddr = realmempointer;                                // save where we are so that we can write the CFun size in here
            realmempointer += 4;
            p++;
            skipspace(p);
            if (!fontnbr) {
                if (!isnamestart((uint8_t)*p))  error((char *)"Function name");
                do { p++; } while (isnamechar((uint8_t)*p));
                skipspace(p);
                if (!(isxdigit((uint8_t)p[0]) && isxdigit((uint8_t)p[1]) && isxdigit((uint8_t)p[2]))) {
                    skipelement(p);
                    p++;
                    if (*p == T_NEWLINE) {
                        CurrentLinePtr = p;
                        p++;                                        // skip the newline token
                    }
                    if (*p == T_LINENBR) p += 3;                     // skip over a line number
                }
            }
            do {
                while (*p && *p != '\'') {
                    skipspace(p);
                    n = 0;
                    for (i = 0; i < 8; i++) {
                        if (!isxdigit((uint8_t)*p)) error((char *)"Invalid hex word");
                        if ((int)((char*)realmempointer - (uint32_t)ProgMemory) >= MAX_PROG_SIZE - 5) error((char *)"Not enough memory");
                        n = n << 4;
                        if (*p <= '9')
                            n |= (*p - '0');
                        else
                            n |= (toupper(*p) - 'A' + 10);
                        p++;
                    }
                    realmempointer += 4;
                    skipspace(p);
                    if (firsthex) {
                        firsthex = 0;
                        if (((n >> 16) & 0xff) < 0x20)error((char *)"Can't define non-printing characters");
                    }
                }
                // we are at the end of a embedded code line
                while (*p) p++;                                      // make sure that we move to the end of the line
                p++;                                                // step to the start of the next line
                if (*p == 0) error((char *)"Missing END declaration");
                if (*p == T_NEWLINE) {
                    CurrentLinePtr = p;
                    p++;                                            // skip the newline token
                }
                if (*p == T_LINENBR) p += 3;                         // skip over the line number
                skipspace(p);
            } while (*p != endtoken);
            storedupdates[updatecount++] = realmempointer - SaveSizeAddr - 4;
        }
        while (*p) p++;                                              // look for the zero marking the start of the next element
    }
    realmempointer = realmemsave;
    updatecount = 0;
    p = (unsigned char*)ProgMemory;                                              // start scanning program memory
    while (*p != 0xff) {
        nbr++;
        if (*p == 0) p++;                                            // if it is at the end of an element skip the zero marker
        if (*p == 0) break;                                          // end of the program
        if (*p == T_NEWLINE) {
            CurrentLinePtr = p;
            p++;                                                    // skip the newline token
        }
        if (*p == T_LINENBR) p += 3;                                 // step over the line number

        skipspace(p);
        if (*p == T_LABEL) {
            p += p[1] + 2;                                          // skip over the label
            skipspace(p);                                           // and any following spaces
        }
        if (/**p == cmdCSUB ||*/ *p == GetCommandValue((unsigned char*)"DefineFont")) {      // found a CFUNCTION, CSUB or DEFINEFONT token
            if (*p == GetCommandValue((unsigned char*)"DefineFont")) {      // found a CFUNCTION, CSUB or DEFINEFONT token
                endtoken = GetCommandValue((unsigned char*)"End DefineFont");
                p++;                                                // step over the token
                skipspace(p);
                if (*p == '#') p++;
                fontnbr = (unsigned char)getint(p, 1, FONT_TABLE_SIZE);
                // font 6 has some special characters, some of which depend on font 1
                if (fontnbr == 1 || fontnbr == 6 || fontnbr == 7) error((char *)"Cannot redefine fonts 1, 6, or 7");

                MemWriteWord(fontnbr - 1);             // a low number (< FONT_TABLE_SIZE) marks the entry as a font
                skipelement(p);                                     // go to the end of the command
                p--;
            }
 //           else {
 //               endtoken = GetCommandValue((unsigned char *)"End CSub");
 //               MemWriteWord((unsigned int)p);               // if a CFunction/CSub save a pointer to the declaration
 //               fontnbr = 0;
 //           }
            SaveSizeAddr = realmempointer;                                // save where we are so that we can write the CFun size in here
            MemWriteWord(storedupdates[updatecount++]);                        // leave this blank so that we can later do the write
            p++;
            skipspace(p);
            if (!fontnbr) {
                if (!isnamestart((uint8_t)*p))  error((char *)"Function name");
                do { p++; } while (isnamechar(*p));
                skipspace(p);
                if (!(isxdigit(p[0]) && isxdigit(p[1]) && isxdigit(p[2]))) {
                    skipelement(p);
                    p++;
                    if (*p == T_NEWLINE) {
                        CurrentLinePtr = p;
                        p++;                                        // skip the newline token
                    }
                    if (*p == T_LINENBR) p += 3;                     // skip over a line number
                }
            }
            do {
                while (*p && *p != '\'') {
                    skipspace(p);
                    n = 0;
                    for (i = 0; i < 8; i++) {
                        if (!isxdigit(*p)) error((char *)"Invalid hex word");
                        if ((int)((char*)realmempointer - (uint32_t)ProgMemory) >= MAX_PROG_SIZE - 5) error((char *)"Not enough memory");
                        n = n << 4;
                        if (*p <= '9')
                            n |= (*p - '0');
                        else
                            n |= (toupper(*p) - 'A' + 10);
                        p++;
                    }

                    MemWriteWord(n);
                    skipspace(p);
                }
                // we are at the end of a embedded code line
                while (*p) p++;                                      // make sure that we move to the end of the line
                p++;                                                // step to the start of the next line
                if (*p == 0) error((char *)"Missing END declaration");
                if (*p == T_NEWLINE) {
                    CurrentLinePtr = p;
                    p++;                                        // skip the newline token
                }
                if (*p == T_LINENBR) p += 3;                     // skip over a line number
                skipspace(p);
            } while (*p != endtoken);
        }
        while (*p) p++;                                              // look for the zero marking the start of the next element
    }
    MemWriteWord((unsigned int)0xffffffff);                                // make sure that the end of the CFunctions is terminated with an erased word
    MemWriteClose();                                              // this will flush the buffer and step the flash write pointer to the next word boundary
    if (msg) {                                                       // if requested by the caller, print an informative message
        if (MMCharPos > 1) MMPrintString((char *)"\r\n");                    // message should be on a new line
        MMPrintString((char *)"Saved ");
        IntToStr((char *)tknbuf, nbr + 3, 10);
        MMPrintString((char*)tknbuf);
        MMPrintString((char *)" bytes\r\n");
    }
    memcpy(tknbuf, buf, STRINGSIZE);                                // restore the token buffer in case there are other commands in it
//    initConsole();
    return;

    // we only get here in an error situation while writing the program to flash
exiterror1:
    MemWriteByte(0); MemWriteByte(0); MemWriteByte(0);    // terminate the program in flash
    MemWriteClose();
    error((char *)"Not enough memory");
}
