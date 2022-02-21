/***********************************************************************************************************************
MMBasic for Windows

MM_Misc.cpp

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
#include "Editor.h"
#include "MMBasic_Includes.h"
#include <shlobj.h>
#include "hxcmod.h"

extern char* editCbuff;
extern modcontext mcontext;

int TickPeriod[NBRSETTICKS];
volatile int TickTimer[NBRSETTICKS];
unsigned char* TickInt[NBRSETTICKS];
volatile unsigned char TickActive[NBRSETTICKS];
unsigned char* MouseInterrupLeftDown; 
volatile int MouseFoundLeftDown = 0;
unsigned char* MouseInterrupLeftUp;
volatile int MouseFoundLeftUp = 0;
unsigned char* MouseInterrupRightDown;
volatile int MouseFoundRightDown = 0;
volatile int MouseDouble = 0;
const char* daystrings[] = { "dummy","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday" };
const char* CaseList[] = { "", "LOWER", "UPPER" };
const char* KBrdList[] = { "", "US", "FR", "DE", "IT", "BE", "UK", "ES" };
unsigned char* OnKeyGOSUB = NULL;
int64_t fasttimerat0;
#define EPOCH_ADJUSTMENT_DAYS	719468L
/* year to which the adjustment was made */
#define ADJUSTED_EPOCH_YEAR	0
/* 1st March of year 0 is Wednesday */
#define ADJUSTED_EPOCH_WDAY	3
/* there are 97 leap years in 400-year periods. ((400 - 97) * 365 + 97 * 366) */
#define DAYS_PER_ERA		146097L
/* there are 24 leap years in 100-year periods. ((100 - 24) * 365 + 24 * 366) */
#define DAYS_PER_CENTURY	36524L
/* there is one leap year every 4 years */
#define DAYS_PER_4_YEARS	(3 * 365 + 366)
/* number of days in a non-leap year */
#define DAYS_PER_YEAR		365
/* number of days in January */
#define DAYS_IN_JANUARY		31
/* number of days in non-leap February */
#define DAYS_IN_FEBRUARY	28
/* number of years per era */
#define YEARS_PER_ERA		400
#define SECSPERDAY 86400
#define SECSPERHOUR 3600
#define SECSPERMIN 60
#define DAYSPERWEEK 7
#define YEAR_BASE 1900
/* Number of days per month (except for February in leap years). */
static const int monoff[] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};
int Optionfulltime = 0;
static int
is_leap_year(int year)
{
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static int
leap_days(int y1, int y2)
{
    --y1;
    --y2;
    return (y2 / 4 - y1 / 4) - (y2 / 100 - y1 / 100) + (y2 / 400 - y1 / 400);
}
time_t timegm(const struct tm* tm)
{
    int year;
    time_t days;
    time_t hours;
    time_t minutes;
    time_t seconds;

    year = 1900 + tm->tm_year;
    days = 365 * ((time_t)year - 1970) + leap_days(1970, year);
    days += monoff[tm->tm_mon];

    if (tm->tm_mon > 1 && is_leap_year(year))
        ++days;
    days += tm->tm_mday - 1;

    hours = days * 24 + tm->tm_hour;
    minutes = hours * 60 + tm->tm_min;
    seconds = minutes * 60 + tm->tm_sec;

    return seconds;
}
extern "C" void PRet(void) {
    MMPrintString((char *)"\r\n");
}

extern "C" void PInt(int64_t n) {
    char s[20];
    IntToStr(s, (int64_t)n, 10);
    MMPrintString(s);
}

extern "C" void PIntComma(int64_t n) {
    MMPrintString((char*)", "); PInt(n);
}

extern "C" void PIntH(unsigned long long int n) {
    char s[20];
    IntToStr(s, (int64_t)n, 16);
    MMPrintString(s);
}
extern "C" void PIntHC(unsigned long long int n) {
    MMPrintString((char*)", "); PIntH(n);
}

extern "C" void PFlt(MMFLOAT flt) {
    char s[20];
    FloatToStr(s, flt, 4, 4, ' ');
    MMPrintString(s);
}
extern "C" void PFltComma(MMFLOAT n) {
    MMPrintString((char*)", "); PFlt(n);
}
extern "C" int check_interrupt(void) {
    unsigned char* intaddr;
    static unsigned char rti[2];
    ProcessTouch();                                             // check GUI touch
    if (CheckGuiFlag) CheckGui();                                // This implements a LED flash
    if (!InterruptUsed) return 0;                                    // quick exit if there are no interrupts set
    if (InterruptReturn != NULL || CurrentLinePtr == NULL) return 0; // skip if we are in an interrupt or in immediate mode
    // check if one of the tick interrupts is enabled and if it has occured
    // check for an  ON KEY loc  interrupt
    if (KeyInterrupt != NULL && Keycomplete) {
        Keycomplete = false;
        intaddr = KeyInterrupt;									    // set the next stmt to the interrupt location
        goto GotAnInterrupt;
    }

    if (OnKeyGOSUB && kbhitConsole()) {
        intaddr = OnKeyGOSUB;                                       // set the next stmt to the interrupt location
        goto GotAnInterrupt;
    }
    for (int i = 0; i < NBRSETTICKS; i++) {
        if (TickInt[i] != NULL && TickTimer[i] > TickPeriod[i]) {
            // reset for the next tick but skip any ticks completely missed
            while (TickTimer[i] > TickPeriod[i]) TickTimer[i] -= TickPeriod[i];
            intaddr = TickInt[i];
            goto GotAnInterrupt;
        }
    }
    if (MouseInterrupLeftDown != NULL && MouseFoundLeftDown) {
        MouseFoundLeftDown = 0;
        intaddr = MouseInterrupLeftDown;
        goto GotAnInterrupt;
    }

    if (MouseInterrupLeftUp != NULL && MouseFoundLeftUp) {
        MouseFoundLeftUp = 0;
        intaddr = MouseInterrupLeftUp;
        goto GotAnInterrupt;
    }

    if (MouseInterrupRightDown != NULL && MouseFoundRightDown) {
        MouseFoundRightDown = 0;
        intaddr = MouseInterrupRightDown;
        goto GotAnInterrupt;
    }
    if (gui_int_down && GuiIntDownVector) {                          // interrupt on pen down
        intaddr = GuiIntDownVector;                                   // get a pointer to the interrupt routine
        gui_int_down = false;
        goto GotAnInterrupt;
    }

    if (gui_int_up && GuiIntUpVector) {
        intaddr = GuiIntUpVector;                                     // get a pointer to the interrupt routine
        gui_int_up = false;
        goto GotAnInterrupt;
    }

    if (WAVInterrupt != NULL && WAVcomplete) {
        WAVcomplete = false;
        intaddr = WAVInterrupt;									    // set the next stmt to the interrupt location
        goto GotAnInterrupt;
    }

    if (COLLISIONInterrupt != NULL && CollisionFound) {
        CollisionFound = false;
        intaddr = (unsigned char *)COLLISIONInterrupt;									    // set the next stmt to the interrupt location
        goto GotAnInterrupt;
    }
    if (comused) {
        for (int i = 1; i < MAXCOMPORTS; i++) {
            if (comn[i]) {
                if(com_interrupt[i] != NULL && SerialRxStatus(i) >= com_ilevel[i]) {// do we need to interrupt?
                    intaddr = (unsigned char *)com_interrupt[i];									// set the next stmt to the interrupt location
                    goto GotAnInterrupt;
                }
            }
        }
    }
    return 0;
GotAnInterrupt:
    LocalIndex++;                                                   // IRETURN will decrement this
    InterruptReturn = nextstmt;                                     // for when IRETURN is executed
    // if the interrupt is pointing to a SUB token we need to call a subroutine
    if (*intaddr == cmdSUB) {
        strncpy(CurrentInterruptName, (char *)(intaddr + 1), MAXVARLEN);
        rti[0] = cmdIRET;                                           // setup a dummy IRETURN command
        rti[1] = 0;
        if (gosubindex >= MAXGOSUB) error((char *)"Too many SUBs for interrupt");
        errorstack[gosubindex] = CurrentLinePtr;
        gosubstack[gosubindex++] = rti;                             // return from the subroutine to the dummy IRETURN command
        LocalIndex++;                                               // return from the subroutine will decrement LocalIndex
        skipelement(intaddr);                                       // point to the body of the subroutine
    }

    nextstmt = intaddr;                                             // the next command will be in the interrupt routine
    return 1;
}

// get the address for a MMBasic interrupt
// this will handle a line number, a label or a subroutine
// all areas of MMBasic that can generate an interrupt use this function
extern "C" unsigned char* GetIntAddress(unsigned char* p) {
    int i;
    if (isnamestart((uint8_t)*p)) {                                           // if it starts with a valid name char
        i = FindSubFun(p, 0);                                       // try to find a matching subroutine
        if (i == -1)
            return findlabel(p);                                    // if a subroutine was NOT found it must be a label
        else
            return subfun[i];                                       // if a subroutine was found, return the address of the sub
    }

    return findline((int)getinteger(p), true);                           // otherwise try for a line number
}


void cmd_ireturn(void) {
    if (InterruptReturn == NULL) error((char *)"Not in interrupt");
    checkend(cmdline);
    nextstmt = InterruptReturn;
    InterruptReturn = NULL;
    if (LocalIndex)    ClearVars(LocalIndex--);                        // delete any local variables
    TempMemoryIsChanged = true;                                     // signal that temporary memory should be checked
    *CurrentInterruptName = 0;                                        // for static vars we are not in an interrupt
    if (DelayedDrawKeyboard) {
        DrawKeyboard(1);                                            // the pop-up GUI keyboard should be drawn AFTER the pen down interrupt
        DelayedDrawKeyboard = false;
    }
    if (DelayedDrawFmtBox) {
        DrawFmtBox(1);                                              // the pop-up GUI keyboard should be drawn AFTER the pen down interrupt
        DelayedDrawFmtBox = false;
    }
}
// remove unnecessary text
void CrunchData(unsigned char** p, int c) {
    static unsigned char inquotes, lastch, incomment;

    if (c == '\n') c = '\r';                                         // CR is the end of line terminator
    if (c == 0 || c == '\r') {
        inquotes = false; incomment = false;                        // newline so reset our flags
        if (c) {
            if (lastch == '\r') return;                              // remove two newlines in a row (ie, empty lines)
            *((*p)++) = '\r';
        }
        lastch = '\r';
        return;
    }

    if (incomment) return;                                           // discard comments
    if (c == ' ' && lastch == '\r') return;                          // trim all spaces at the start of the line
    if (c == '"') inquotes = !inquotes;
    if (inquotes) {
        *((*p)++) = c;                                              // copy everything within quotes
        return;
    }
    if (c == '\'') {                                                 // skip everything following a comment
        incomment = true;
        return;
    }
    if (c == ' ' && (lastch == ' ' || lastch == ',')) {
        lastch = ' ';
        return;                                                     // remove more than one space or a space after a comma
    }
    *((*p)++) = lastch = c;
}

void cmd_autosave(void) {
    unsigned char* buf, * p, *r=NULL;
    int c, prevc = 0, crunch = false;
    int count = 0;
    char fname[STRINGSIZE];
    if (CurrentLinePtr) error((char *)"Invalid in a program");
    if ((*cmdline == 0 || *cmdline == '\''))error((char *)"Syntax");
    r = getCstring(cmdline);
    if (strchr((char *)cmdline, '.') == NULL) strcat((char*)r, ".BAS");
    strcpy(fname, (const char *)r);
    ClearProgram();                                                 // clear any leftovers from the previous program
    p = buf = (unsigned char *)GetMemory(EDIT_BUFFER_SIZE);
    CrunchData(&p, 0);                                              // initialise the crunch data subroutine
    while ((c = getConsole(0)) != 0x1a && c != F1 && c != F2) {                    // while waiting for the end of text char
        if (c == CTRLKEY('V')) {
            int count = EditPaste();
            if (!(editCbuff == NULL || count == 0)) {
                char* p = (char*)editCbuff;
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
            }
        }

        ShowMMBasicCursor(1);
        if (c == -1)continue;
        if (p == buf && c == '\n') continue;                         // throw away an initial line feed which can follow the command
        if ((p - buf) >= EDIT_BUFFER_SIZE) error((char*)"Not enough memory");
        if (isprint(c) || c == '\r' || c == '\n' || c == TAB) {
            ShowMMBasicCursor(0);
            if (c == TAB) c = ' ';
            if (crunch)
                CrunchData(&p, c);                                  // insert into RAM after throwing away comments. etc
            else
              *p++ = c;                                           // insert the input into RAM
 //           {
 //           if (!(c == '\n' && prevc == '\r'))  DisplayPutC(c);    // and echo it
            if (c != '\r' && prevc == '\n') {
                DisplayPutC('\r');
                CurrentX = 0;
            }
 //           }
              DisplayPutC(c);
              if(c=='\n')CurrentX = 0;
              prevc = c;
        }
    }

    *p = 0;                                                         // terminate the string in RAM
    ShowMMBasicCursor(0);
    while (getConsole(0) != -1);                                      // clear any rubbish in the input
//    ClearSavedVars();                                               // clear any saved variables
    int fnbr;
    char* pm = (char *)buf;
    fnbr = FindFreeFileNbr();
    if (!BasicFileOpen(fname, fnbr, (char *)"wb")) return;
    FilePutStr(strlen((const char *)buf), (char*)buf, fnbr);
    FileClose(fnbr);
    SaveProgramToMemory(buf, true);
    FreeMemory(buf);
    strcpy(lastfileedited, fname);
    if (c == F2) {
        ClearVars(0);
        strcpy((char *)inpbuf, (char *)"RUN\r\n");
        tokenise(true);                                             // turn into executable code
        ExecuteProgram(tknbuf);                                     // execute the line straight away
    }
}
void cmd_cfunction(void) {
    unsigned char* p, EndToken;
    EndToken = GetCommandValue((unsigned char*)"End DefineFont");           // this terminates a DefineFont
//    if (cmdtoken == cmdCSUB) EndToken = GetCommandValue("End CSub");                 // this terminates a CSUB
    p = cmdline;
    while (*p != 0xff) {
        if (*p == 0) p++;                                            // if it is at the end of an element skip the zero marker
        if (*p == 0) error((char*)"Missing END declaration");               // end of the program
        if (*p == T_NEWLINE) p++;                                    // skip over the newline token
        if (*p == T_LINENBR) p += 3;                                 // skip over the line number
        skipspace(p);
        if (*p == T_LABEL) {
            p += p[1] + 2;                                          // skip over the label
            skipspace(p);                                           // and any following spaces
        }
        if (*p == EndToken) {                                        // found an END token
            nextstmt = p;
            skipelement(nextstmt);
            return;
        }
        p++;
    }
}
void cmd_pause(void) {
    static int interrupted = false;
    MMFLOAT f;

    f = getnumber(cmdline);                                         // get the pulse width
    if (f < 0) error((char *)"Number out of bounds");
    if (f < 0.05) return;

    if (f < 1.5) {
        uSec((int)f * 1000);                                             // if less than 1.5mS do the pause right now
        return;                                                     // and exit straight away
    }

    //    #if defined(MX470)
    //      InPause = true;
    //    #endif

    if (InterruptReturn == NULL) {
        // we are running pause in a normal program
        // first check if we have reentered (from an interrupt) and only zero the timer if we have NOT been interrupted.
        // This means an interrupted pause will resume from where it was when interrupted
        if (!interrupted) PauseTimer = 0;
        interrupted = false;

        while (PauseTimer < (unsigned int)FloatToInt32(f)) {
            CheckAbort();
            if (check_interrupt()) {
                // if there is an interrupt fake the return point to the start of this stmt
                // and return immediately to the program processor so that it can send us off
                // to the interrupt routine.  When the interrupt routine finishes we should reexecute
                // this stmt and because the variable interrupted is static we can see that we need to
                // resume pausing rather than start a new pause time.
                while (*cmdline && *cmdline != cmdtoken) cmdline--;  // step back to find the command token
                InterruptReturn = cmdline;                          // point to it
                interrupted = true;                                 // show that this stmt was interrupted
                return;                                             // and let the interrupt run
            }
        }
        interrupted = false;
    }
    else {
        // we are running pause in an interrupt, this is much simpler but note that
        // we use a different timer from the main pause code (above)
        IntPauseTimer = 0;
        while (IntPauseTimer < (unsigned int)FloatToInt32(f)) CheckAbort();
    }
    //    #if defined(MX470)
    //      InPause = false;
    //    #endif
}

void cmd_font(void) {
    getargs(&cmdline, 3, (unsigned char*)",");
    if (argc < 1) error((char *)"Argument count");
    if (*argv[0] == '#') ++argv[0];
    if (argc == 3)
        SetFont((((int)getint(argv[0], 1, FONT_TABLE_SIZE) - 1) << 4) | (int)getint(argv[2], 1, 15));
    else
        SetFont((((int)getint(argv[0], 1, FONT_TABLE_SIZE) - 1) << 4) | 1);
    if (!CurrentLinePtr) {                 // if we are at the command prompt on the LCD
        PromptFont = gui_font;
        if (CurrentY + gui_font_height >= VRes) {
            ScrollLCD(CurrentY + gui_font_height - VRes);           // scroll up if the font change split the line over the bottom
            CurrentY -= (CurrentY + gui_font_height - VRes);
        }
    }
}
// this is invoked as a command (ie, TIMER = 0)
// search through the line looking for the equals sign and step over it,
// evaluate the rest of the command and save in the timer
void cmd_timer(void) {
    while (*cmdline && tokenfunction(*cmdline) != op_equal) cmdline++;
    if (!*cmdline) error((char *)"Syntax");
    QueryPerformanceCounter((LARGE_INTEGER*)&fasttimerat0);
    fasttimerat0 -= getinteger(++cmdline)/1000 * frequency;
}

void cmd_test(void) {
    getargs(&cmdline, 5, (unsigned char*)",");
//    float a = getnumber(argv[0]);
//    float b= getnumber(argv[2]);
}

// this is invoked as a function
void fun_timer(void) {
    int64_t i;
    QueryPerformanceCounter((LARGE_INTEGER*)&i);
    i -= fasttimerat0;
    fret = (MMFLOAT)i / (MMFLOAT)(frequency / 1000.0);
    targ = T_NBR;
}
extern "C" void PO(const char* s, int m) {
    if (m == 1)MMPrintString((char*)"OPTION ");
    else if (m == 2)MMPrintString((char*)"DEFAULT ");
    else if (m == 3)MMPrintString((char*)"CURRENT ");
    MMPrintString((char*)s); MMPrintString((char*)" ");
}
void PO2Str(const char* s1, const char* s2, int m) {
    PO(s1, m); MMPrintString((char*)s2); MMPrintString((char*)"\r\n");
}


void PO2Int(const char* s1, int64_t n) {
    PO(s1, 1); PInt(n); MMPrintString((char*)"\r\n");
}

void PO3Int(const char* s1, int64_t n1, int64_t n2) {
    PO(s1, 1); PInt(n1); MMPrintString((char*)","); PInt(n2); MMPrintString((char*)"\r\n");
}

void printoptions(void) {
    LoadOptions();
    if (Option.mode == 8) 	PO2Str("Default mode 8", "640x480", 1);
    else if (Option.mode == 9)	PO2Str("Default mode 9", "1024x768", 1);
    else if (Option.mode == 10)	PO2Str("Default mode 10", "848x480", 1);
    else if (Option.mode == 11)	PO2Str("Default mode 11", "1280x720", 1);
    else if (Option.mode == 12)	PO2Str("Default mode 12", "960x540", 1);
    else if (Option.mode == 14)	PO2Str("Default mode 14", "960x540", 1);
    else if (Option.mode == 15)	PO2Str("Default mode 15", "1280x1024", 1);
    else if (Option.mode == 16)	PO2Str("Default mode 16", "1920x1080", 1);
    else if (Option.mode == 18)	PO2Str("Default mode 18", "1024x600", 1);
    else if (Option.mode == 19)	PO2Str("Default mode 18", "3840x2160", 1);
    PO3Int("Default Font", (Option.DefaultFont >> 4) + 1, Option.DefaultFont & 0xf);
    if (strlen((char*)Option.defaultpath))PO2Str("Default path", (char*)Option.defaultpath, 1);
    if (Option.Autorun) PO2Str("Autorun", "ON", 1);
    if (!Option.ColourCode) PO2Str("Colourcode", "OFF", 1);
    if (Option.Listcase != CONFIG_TITLE) PO2Str("Case", CaseList[(int)Option.Listcase], 1);
    if (Option.Tab != 2) {
        char buff[20] = { 0 };
        sprintf(buff, "OPTION TAB %d\r\n", Option.Tab);
        MMPrintString(buff);
    }
    PO("Keyboard",1); MMPrintString((char*)KBrdList[(int)Option.KeyboardConfig]);
    if (!(Option.RepeatStart == 600 && Option.RepeatRate == 150)) {
        char buff[40] = { 0 };
        sprintf(buff, ",%d,%d\r\n", Option.RepeatStart, Option.RepeatRate);
        MMPrintString(buff);
    } else PRet();
    if (strlen((char*)Option.F5key)) {
        char cc[MAXKEYLEN + 4];
        strcpy(cc, (char*)Option.F5key);
        if (cc[strlen(cc) - 2] == '\r' && cc[strlen(cc) - 2] == '\r') {
            cc[strlen(cc) - 2] = 0;
            strcat(cc, "<crlf>");
        }

        PO2Str("F5", (char*)cc, 1);
    }
    if (strlen((char*)Option.F6key)) {
        char cc[MAXKEYLEN + 4];
        strcpy(cc, (char*)Option.F6key);
        if (cc[strlen(cc) - 2] == '\r' && cc[strlen(cc) - 2] == '\r') {
            cc[strlen(cc) - 2] = 0;
            strcat(cc, "<crlf>");
        }

        PO2Str("F6", (char*)cc, 1);
    }
    if (strlen((char*)Option.F6key)) {
        char cc[MAXKEYLEN + 4];
        strcpy(cc, (char*)Option.F6key);
        if (cc[strlen(cc) - 2] == '\r' && cc[strlen(cc) - 2] == '\r') {
            cc[strlen(cc) - 2] = 0;
            strcat(cc, "<crlf>");
        }

        PO2Str("F7", (char*)cc, 1);
    }
    if (strlen((char*)Option.F8key)) {
        char cc[MAXKEYLEN + 4];
        strcpy(cc, (char*)Option.F8key);
        if (cc[strlen(cc) - 2] == '\r' && cc[strlen(cc) - 2] == '\r') {
            cc[strlen(cc) - 2] = 0;
            strcat(cc, "<crlf>");
        }

        PO2Str("F8", (char*)cc, 1);
    }
    if (strlen((char*)Option.F9key)) {
        char cc[MAXKEYLEN + 4];
        strcpy(cc, (char*)Option.F9key);
        if (cc[strlen(cc) - 2] == '\r' && cc[strlen(cc) - 2] == '\r') {
            cc[strlen(cc) - 2] = 0;
            strcat(cc, "<crlf>");
        }

        PO2Str("F9", (char*)cc, 1);
    }
    char buff[30] = { 0 };
    sprintf(buff, "Current display %d,%d\r\n", Option.Height, Option.Width);
    MMPrintString(buff);
    return;

}

void cmd_option(void) {
    unsigned char* tp;
    tp = checkstring(cmdline, (unsigned char*)"DEFAULT PATH");
    if (tp) {
        char* p;
        int i = 0;
        DWORD j = 0;
        p = (char*)getCstring(tp);										// get the directory name and convert to a standard C string
        i = (int)SetCurrentDirectoryA(p);
        if (i == 0) {
            j = GetLastError();
            error((char*)"Directory error %", j);
        }
        strcpy(Option.defaultpath, (const char*)p);
        SaveOptions();
        return;
    }
        tp = checkstring(cmdline, (unsigned char*)"DEFAULT FONT");
    if (tp) {
        if (CurrentLinePtr) error((char *)"Invalid in a program");
        getargs(&tp, 3, (unsigned char*)",");
        if (argc < 1) error((char*)"Argument count");
        if (*argv[0] == '#') ++argv[0];
        if (argc == 3)
        {
            Option.DefaultFont=(((int)(getint(argv[0], 1, FONT_TABLE_SIZE) - 1) << 4) | (int)getint(argv[2], 1, 15));
        }  else {
            Option.DefaultFont = (((int)(getint(argv[0], 1, FONT_TABLE_SIZE) - 1) << 4) | 1);
        }
        SetFont(Option.DefaultFont);
        if (!CurrentLinePtr) {                 // if we are at the command prompt on the LCD
            PromptFont = gui_font;
            if (CurrentY + gui_font_height >= VRes) {
                ScrollLCD(CurrentY + gui_font_height - VRes);           // scroll up if the font change split the line over the bottom
                CurrentY -= (CurrentY + gui_font_height - VRes);
            }
        }
        SaveOptions();
        return;
    }

    tp = checkstring(cmdline, (unsigned char *)"DEFAULT MODE");
    if (tp) {
        int mode = 9, DefaultFont = 1;
        bool fullscreen = 0;
        getargs(&tp, 3, (unsigned char*)",");
        if (CurrentLinePtr) error((char *)"Invalid in a program");
        if (checkstring(argv[0], (unsigned char*)"8")) {
            mode = 8;
            Option.DefaultFont = 1;
        }
        else if (checkstring(argv[0], (unsigned char*)"1")) {
            mode = 1;
            DefaultFont = 1;
        }
         else if (checkstring(argv[0], (unsigned char*)"9")) {
            mode = 9;
            DefaultFont = (2<<4) | 1;
        }
        else if (checkstring(argv[0], (unsigned char*)"10")) {
            mode = 10;
            DefaultFont = 1;
        }
        else if (checkstring(argv[0], (unsigned char*)"11")) {
            mode = 11;
            DefaultFont = (2 << 4) | 1;
        }
        else if (checkstring(argv[0], (unsigned char*)"12")) {
            mode = 12;
            DefaultFont = (3 << 4) | 1;
        }
        else if (checkstring(argv[0], (unsigned char*)"14")) {
            mode = 14;
            DefaultFont = 1;
        }
        else if (checkstring(argv[0], (unsigned char*)"15")) {
            mode = 15;
            DefaultFont = (2 << 4) | 1;
        }
        else if (checkstring(argv[0], (unsigned char*)"16")) {
            mode = 16;
            DefaultFont = (2 << 4) | 1;
        }
        else if (checkstring(argv[0], (unsigned char*)"18")) {
            mode = 18;
            DefaultFont = (3 << 4) | 1;
        }
       else error((char *)"Invalid mode");
        if (argc == 3)fullscreen = (bool)getint(argv[2], 0, 1);
        Option.mode = mode;
        Option.DefaultFont = DefaultFont;
        Option.fullscreen = fullscreen;
        Option.hres = xres[Option.mode];
        Option.vres = yres[Option.mode];
        Option.pixelnum = pixeldensity[Option.mode];
        SaveOptions();
        setmode(Option.mode,0, Option.fullscreen);
        SetFont(Option.DefaultFont);
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"BASE");
    if (tp) {
        if (DimUsed) error((char*)"Must be before DIM or LOCAL");
        OptionBase = (int)getint(tp, 0, 1);
        return;
    }

    if (tp) {
        if (DimUsed) error((char *)"Must be before DIM or LOCAL");
        OptionBase = (int)getint(tp, 0, 1);
        return;
    }

    tp = checkstring(cmdline, (unsigned char*)"EXPLICIT");
    if (tp) {
        //        if(varcnt != 0) error((char *)"Variables already defined");
        OptionExplicit = true;
        return;
    }

    tp = checkstring(cmdline, (unsigned char*)"DEFAULT");
    if (tp) {
        if (checkstring(tp, (unsigned char*)"INTEGER")) { DefaultType = T_INT;  return; }
        if (checkstring(tp, (unsigned char*)"FLOAT")) { DefaultType = T_NBR;  return; }
        if (checkstring(tp, (unsigned char*)"STRING")) { DefaultType = T_STR;  return; }
        if (checkstring(tp, (unsigned char*)"NONE")) { DefaultType = T_NOTYPE;   return; }
    }

    tp = checkstring(cmdline, (unsigned char*)"BREAK");
    if (tp) {
        BreakKey = (int)getint(tp,0,255);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"F5");
    if (tp) {
        char p[STRINGSIZE];
        strcpy(p, (const char*)getCstring(tp));
        if (strlen(p) >= sizeof(Option.F5key))error((char *)"Maximum % characters", MAXKEYLEN - 1);
        else strcpy((char*)Option.F5key, p);
        SaveOptions();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"F6");
    if (tp) {
        char p[STRINGSIZE];
        strcpy(p, (const char*)getCstring(tp));
        if (strlen(p) >= sizeof(Option.F6key))error((char *)"Maximum % characters", MAXKEYLEN - 1);
        else strcpy((char*)Option.F6key, p);
        SaveOptions();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"F7");
    if (tp) {
        char p[STRINGSIZE];
        strcpy(p, (const char*)getCstring(tp));
        if (strlen(p) >= sizeof(Option.F7key))error((char *)"Maximum % characters", MAXKEYLEN - 1);
        else strcpy((char*)Option.F7key, p);
        SaveOptions();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"F8");
    if (tp) {
        char p[STRINGSIZE];
        strcpy(p, (const char*)getCstring(tp));
        if (strlen(p) >= sizeof(Option.F8key))error((char *)"Maximum % characters", MAXKEYLEN - 1);
        else strcpy((char*)Option.F8key, p);
        SaveOptions();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"F9");
    if (tp) {
        char p[STRINGSIZE];
        strcpy(p, (const char*)getCstring(tp));
        if (strlen(p) >= sizeof(Option.F9key))error((char *)"Maximum % characters", MAXKEYLEN - 1);
        else strcpy((char*)Option.F9key, p);
        SaveOptions();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"AUTORUN");
    if (tp) {
        if (checkstring(tp, (unsigned char*)"OFF")) { Option.Autorun = 0; SaveOptions(); return; }
        if (checkstring(tp, (unsigned char*)"ON")) { Option.Autorun = 1; SaveOptions(); return; }
        SaveOptions(); return;
    }
    tp = checkstring(cmdline, (unsigned char*)"CASE");
    if (tp) {
        if (checkstring(tp, (unsigned char*)"LOWER")) { Option.Listcase = CONFIG_LOWER; SaveOptions(); return; }
        if (checkstring(tp, (unsigned char*)"UPPER")) { Option.Listcase = CONFIG_UPPER; SaveOptions(); return; }
        if (checkstring(tp, (unsigned char*)"TITLE")) { Option.Listcase = CONFIG_TITLE; SaveOptions(); return; }
    }

    tp = checkstring(cmdline, (unsigned char*)"TAB");
    if (tp) {
        if (checkstring(tp, (unsigned char*)"2")) { Option.Tab = 2; SaveOptions(); return; }
        if (checkstring(tp, (unsigned char*)"3")) { Option.Tab = 3; SaveOptions(); return; }
        if (checkstring(tp, (unsigned char*)"4")) { Option.Tab = 4; SaveOptions(); return; }
        if (checkstring(tp, (unsigned char*)"8")) { Option.Tab = 8; SaveOptions(); return; }
    }
    tp = checkstring(cmdline, (unsigned char*)"PIN");
    if (tp) {
        int i;
        i = (int)getint(tp, 0, 99999999);
        Option.PIN = i;
        SaveOptions();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"COLOURCODE");
    if (tp == NULL) tp = checkstring(cmdline, (unsigned char*)"COLORCODE");
    if (tp) {
        if (checkstring(tp, (unsigned char*)"ON")) { Option.ColourCode = true; SaveOptions(); return; }
        else if (checkstring(tp, (unsigned char*)"OFF")) { Option.ColourCode = false; SaveOptions(); return; }
        else error((char *)"Syntax");
    }
    tp = checkstring(cmdline, (unsigned char*)"RESET");
    if (tp) {
        ResetOptions();
        SaveOptions();
        HRes = Option.hres;
        VRes = Option.vres;
        PixelSize = Option.pixelnum;
        SystemMode = MODE_RESIZE;
        uSec(500000);
        _excep_code = RESET_COMMAND;
        SoftReset();
    }
    tp = checkstring(cmdline, (unsigned char*)"MILLISECONDS");
    if (tp) {
        if (checkstring(tp, (unsigned char*)"ON")) { Optionfulltime = true; return; }
        if (checkstring(tp, (unsigned char*)"OFF")) { Optionfulltime = false; return; }
    }
    tp = checkstring(cmdline, (unsigned char*)"KEYBOARD");
    if (tp) {
        getargs(&tp, 5, (unsigned char *)",");
        if (checkstring(argv[0], (unsigned char*)"US"))	Option.KeyboardConfig = CONFIG_US;
        else if (checkstring(argv[0], (unsigned char*)"FR"))	Option.KeyboardConfig = CONFIG_FR;
        else if (checkstring(argv[0], (unsigned char*)"GR"))	Option.KeyboardConfig = CONFIG_DE;
        else if (checkstring(argv[0], (unsigned char*)"DE"))	Option.KeyboardConfig = CONFIG_DE;
        else if (checkstring(argv[0], (unsigned char*)"IT"))	Option.KeyboardConfig = CONFIG_IT;
        else if (checkstring(argv[0], (unsigned char*)"BE"))	Option.KeyboardConfig = CONFIG_BE;
        else if (checkstring(argv[0], (unsigned char*)"UK"))	Option.KeyboardConfig = CONFIG_UK;
        else if (checkstring(argv[0], (unsigned char*)"ES"))	Option.KeyboardConfig = CONFIG_ES;
        else if (checkstring(argv[0], (unsigned char*)"SW"))	Option.KeyboardConfig = CONFIG_SW;
        else error((char*)"Syntax");
        if (argc >= 3 && *argv[2])Option.RepeatStart = (short)getint(argv[2], 10, 2000);
        if (argc >= 5 && *argv[4])Option.RepeatRate = (short)getint(argv[4], 10,2000);
        SaveOptions();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"LIST");
    if (tp) {
        LoadOptions();
        printoptions();
        return;
    }

    error((char *)"Invalid Option");
}
void fun_device(void) {
    sret = (unsigned char*)GetTempMemory(STRINGSIZE);                                  // this will last for the life of the command
    strcpy((char *)sret, "MMBasic for Windows");
    CtoM(sret);
    targ = T_STR;
}

void fun_info(void) {
    unsigned char* tp;
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);                                  // this will last for the life of the command
    tp = checkstring(ep, (unsigned char *)"FONT POINTER");
    if (tp) {
        iret = (int64_t)((uint32_t)&FontTable[getint(tp, 1, FONT_TABLE_SIZE) - 1]);
        targ = T_INT;
        return;
    }
    tp = checkstring(ep, (unsigned char*)"SOUND");
    if (tp) {
        switch (CurrentlyPlaying) {
        case P_NOTHING:strcpy((char*)sret, "OFF"); break;
        case P_PAUSE_TONE:
        case P_PAUSE_MP3:
        case P_PAUSE_WAV:
        case P_PAUSE_MOD:
        case P_PAUSE_SOUND:
        case P_PAUSE_FLAC:strcpy((char*)sret, "PAUSED"); break;
        case P_TONE:strcpy((char*)sret, "TONE"); break;
        case P_WAV:strcpy((char*)sret, "WAV"); break;
        case P_MP3:strcpy((char*)sret, "MP3"); break;
        case P_MOD:strcpy((char*)sret, "MODFILE"); break;
        case P_TTS:strcpy((char*)sret, "TTS"); break;
        case P_FLAC:strcpy((char*)sret, "FLAC"); break;
        case P_DAC:strcpy((char*)sret, "DAC"); break;
        case P_SOUND:strcpy((char*)sret, "SOUND"); break;
        }
        CtoM(sret);
        targ = T_STR;
        return;
    }
    tp = checkstring(ep, (unsigned char*)"SAMPLE PLAYING");
    if (tp) {
        iret = (int64_t)((uint32_t)hxcmod_effectplaying(&mcontext, (unsigned short)(getint(tp, 1, NUMMAXSEFFECTS) - 1)));
        targ = T_INT;
        return;
    }
    tp = checkstring(ep, (unsigned char *)"FONT ADDRESS");
    if (tp) {
        iret = (int64_t)((uint32_t)FontTable[getint(tp, 1, FONT_TABLE_SIZE) - 1]);
        targ = T_INT;
        return;
    }
    tp = checkstring(ep, (unsigned char*)"MAX PAGES");
    if (tp) {
        iret = MAXPAGES;
        targ = T_INT;
        return;
    }
    tp = checkstring(ep, (unsigned char*)"PAGE ADDRESS");
    if (tp) {
        iret = (int64_t)((uint32_t)getpageaddress((int)getint(tp, 0, MAXPAGES)));
        targ = T_INT;
        return;
    }
    tp = checkstring(ep, (unsigned char*)"FRAMEBUFFER");
    if (tp) {
        iret = (int64_t)((uint32_t)PageTable[WPN].address);
        targ = T_INT;
        return;
    }
    tp = checkstring(ep, (unsigned char *)"OPTION");
    if (tp) {
        if (checkstring(tp, (unsigned char *)"AUTORUN")) {
            if (Option.Autorun == false)strcpy((char *)sret, "Off");
            else strcpy((char *)sret, "On");
        }
        else if (checkstring(tp, (unsigned char *)"EXPLICIT")) {
            if (OptionExplicit == false)strcpy((char *)sret, "Off");
            else strcpy((char *)sret, "On");
        }
        else if (checkstring(tp, (unsigned char *)"DEFAULT")) {
            if (DefaultType == T_INT)strcpy((char *)sret, "Integer");
            else if (DefaultType == T_NBR)strcpy((char *)sret, "Float");
            else if (DefaultType == T_STR)strcpy((char *)sret, "String");
            else strcpy((char *)sret, "None");
        }
         else if (checkstring(tp, (unsigned char *)"BASE")) {
            if (OptionBase == 1)iret = 1;
            else iret = 0;
            targ = T_INT;
            return;
        }
        else if (checkstring(tp, (unsigned char *)"BREAK")) {
            iret = BreakKey;
            targ = T_INT;
            return;
        }
        else error((char *)"Syntax");
        CtoM(sret);
        targ = T_STR;
        return;
    }
    /*tp = checkstring(ep, (unsigned char*)"CALLTABLE");
    if (tp) {
        iret = (int64_t)(uint32_t)CallTable;
        targ = T_INT;
        return;
    }*/
    tp = checkstring(ep, (unsigned char *)"PROGRAM");
    if (tp) {
        iret = (int64_t)(uint32_t)ProgMemory;
        targ = T_INT;
        return;
    }
    /*
    tp = checkstring(ep, (unsigned char *)"FILESIZE");
    if (tp) {
        int i, j;
        DIR djd;
        FILINFO fnod;
        memset(&djd, 0, sizeof(DIR));
        memset(&fnod, 0, sizeof(FILINFO));
        char* p = getCstring(tp);

        ErrorCheck(0);
        FSerror = f_stat(p, &fnod);
        if (FSerror != FR_OK) { iret = -1; targ = T_INT; strcpy(MMErrMsg, FErrorMsg[4]); return; }
        if ((fnod.fattrib & AM_DIR)) { iret = -2; targ = T_INT; strcpy(MMErrMsg, FErrorMsg[4]); return; }
        iret = fnod.fsize;
        targ = T_INT;
        return;
    }
    tp = checkstring(ep, (unsigned char *)"MODIFIED");
    if (tp) {
        int i, j;
        DIR djd;
        FILINFO fnod;
        memset(&djd, 0, sizeof(DIR));
        memset(&fnod, 0, sizeof(FILINFO));
        char* p = getCstring(tp);
        ErrorCheck(0);
        FSerror = f_stat(p, &fnod);
        if (FSerror != FR_OK) { iret = -1; targ = T_STR; strcpy(MMErrMsg, FErrorMsg[4]); return; }
        //		if((fnod.fattrib & AM_DIR)){ iret=-2; targ=T_INT; strcpy(MMErrMsg,FErrorMsg[4]); return;}
        IntToStr(sret, ((fnod.fdate >> 9) & 0x7F) + 1980, 10);
        sret[4] = '-'; IntToStrPad(sret + 5, (fnod.fdate >> 5) & 0xF, '0', 2, 10);
        sret[7] = '-'; IntToStrPad(sret + 8, fnod.fdate & 0x1F, '0', 2, 10);
        sret[10] = ' ';
        IntToStrPad(sret + 11, (fnod.ftime >> 11) & 0x1F, '0', 2, 10);
        sret[13] = ':'; IntToStrPad(sret + 14, (fnod.ftime >> 5) & 0x3F, '0', 2, 10);
        sret[16] = ':'; IntToStrPad(sret + 17, (fnod.ftime & 0x1F) * 2, '0', 2, 10);
        CtoM(sret);
        targ = T_STR;
        return;
    }
*/
    else {
        if (checkstring(ep, (unsigned char*)"AUTORUN")) {
            if (Option.Autorun == false)strcpy((char*)sret, "Off");
            else strcpy((char*)sret, "On");
        } 
        else if (checkstring(ep, (unsigned char*)"DEVICE")) {
            fun_device();
            return;
        }
        else if (checkstring(ep, (unsigned char*)"DEFAULT PATH")) {
            strcpy((char*)sret, (const char*)Option.defaultpath);
        }
        else if (checkstring(ep, (unsigned char *)"VERSION")) {
            char* p;
            fret = (MMFLOAT)strtol(VERSION, &p, 10);
            fret += (MMFLOAT)strtol(p + 1, &p, 10) / (MMFLOAT)100.0;
            fret += (MMFLOAT)strtol(p + 1, &p, 10) / (MMFLOAT)10000.0;
            fret += (MMFLOAT)strtol(p + 1, &p, 10) / (MMFLOAT)1000000.0;
            targ = T_NBR;
            return;
        }
        else if (checkstring(ep, (unsigned char *)"VARCNT")) {
            iret = (int64_t)((uint32_t)varcnt);
            targ = T_INT;
            return;
        }
        else if (checkstring(ep, (unsigned char*)"CURRENT")) {
            strcpy((char *)sret, MMgetcwd());
            strcat((char*)sret, "\\");
            strcat((char*)sret, lastfileedited);
        }
        else if (checkstring(ep, (unsigned char *)"FONTWIDTH")) {
            iret = FontTable[gui_font >> 4][0] * (gui_font & 0b1111);
            targ = T_INT;
            return;
        }
        else if (tp=checkstring(ep, (unsigned char*)"FILESIZE")) {
            WIN32_FIND_DATAA fdFile;
            HANDLE hFind = NULL;
            char* p = (char *)getCstring(tp);
            if ((hFind = FindFirstFileA((LPCSTR)p, &fdFile)) == INVALID_HANDLE_VALUE)error((char *)"File not found");
            iret = ((int64_t)fdFile.nFileSizeHigh<<32) | (int64_t)fdFile.nFileSizeLow;
            FindClose(hFind);
            targ = T_INT;
            return;
        }
        else if (checkstring(ep, (unsigned char*)"CPU CLOCK")) {
            iret = frequency;
            targ = T_INT;
            return;
        }

        else if (checkstring(ep, (unsigned char*)"FAST TIME")) {
            LARGE_INTEGER t;
            QueryPerformanceCounter((LARGE_INTEGER*)&t);
            iret = t.QuadPart;
            targ = T_INT;
            return;
        }
        else if (checkstring(ep, (unsigned char *)"FONTHEIGHT")) {
            iret = FontTable[gui_font >> 4][1] * (gui_font & 0b1111);
            targ = T_INT;
            return;
        }
        else if (checkstring(ep, (unsigned char*)"FONTCOUNT")) {
            iret = FontTable[gui_font >> 4][3];
            targ = T_INT;
            return;
        }
        else if (checkstring(ep, (unsigned char*)"WIDTH")) {
            iret = Option.Width;
            targ = T_INT;
            return;
        }
        else if (checkstring(ep, (unsigned char*)"HEIGHT")) {
            iret = Option.Height;
            targ = T_INT;
            return;
        }
        else if (checkstring(ep, (unsigned char *)"HPOS")) {
            iret = CurrentX;
            targ = T_INT;
            return;
        }
        else if (checkstring(ep, (unsigned char *)"VPOS")) {
            iret = CurrentY;
            targ = T_INT;
            return;
        }
       else if (checkstring(ep, (unsigned char *)"ERRNO")) {
            iret = MMerrno;
            targ = T_INT;
            return;
        }
        else if (checkstring(ep, (unsigned char *)"ERRMSG")) {
            strcpy((char *)sret, MMErrMsg);
        }
        else if (checkstring(ep, (unsigned char *)"FCOLOUR") || checkstring(ep, (unsigned char *)"FCOLOR")) {
            iret = gui_fcolour;
            targ = T_INT;
            return;
        }
        else if (checkstring(ep, (unsigned char *)"BCOLOUR") || checkstring(ep, (unsigned char *)"BCOLOR")) {
            iret = gui_bcolour;
            targ = T_INT;
            return;
        } else if (checkstring(ep, (unsigned char*)"FRAMEH")) {
            iret = PageTable[WPN].xmax;
            targ = T_INT;
            return;
        } else if (checkstring(ep, (unsigned char*)"FRAMEV")) {
            iret = PageTable[WPN].ymax;
            targ = T_INT;
            return;
        } else if (checkstring(ep, (unsigned char*)"WRITE PAGE")) {
            iret = (int64_t)((uint32_t)PageTable[WritePage].address);
            targ = T_INT;
            return;
        } else if (checkstring(ep, (unsigned char*)"FONT")) {
            iret = (gui_font >> 4) + 1;
            targ = T_INT;
            return;
        }
        else error((char *)"Syntax");
    }
    CtoM(sret);
    targ = T_STR;
}
extern "C" void SoftReset(void) {
    SystemMode = MODE_SOFTRESET;
    longjmp(mark, 1);
}

void fun_date(void) {
    SYSTEMTIME lt;
    GetLocalTime(&lt);
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);                                    // this will last for the life of the command
    IntToStrPad((char *)sret, lt.wDay, '0', 2, 10);
    sret[2] = '-'; IntToStrPad((char*)sret + 3, lt.wMonth, '0', 2, 10);
    sret[5] = '-'; IntToStr((char*)sret + 6, lt.wYear, 10);
    CtoM(sret);
    targ = T_STR;
}

// this is invoked as a function
void fun_time(void) {
    SYSTEMTIME lt;
    GetLocalTime(&lt);
    sret = (unsigned char*)GetTempMemory(STRINGSIZE);                                  // this will last for the life of the command
    IntToStrPad((char*)sret, lt.wHour, '0', 2, 10);
    sret[2] = ':'; IntToStrPad((char*)sret + 3, lt.wMinute, '0', 2, 10);
    sret[5] = ':'; IntToStrPad((char*)sret + 6, lt.wSecond, '0', 2, 10);
    if (Optionfulltime) {
        sret[8] = '.'; IntToStrPad((char*)sret + 9, lt.wMilliseconds, '0', 3, 10);
    }
    CtoM(sret);
    targ = T_STR;
}
// this is invoked as a function
void fun_day(void) {
    unsigned char* arg;
    struct tm* tm;
    struct tm tma;
    tm = &tma;
    time_t time_of_day;
    int i;
    sret = (unsigned char*)GetTempMemory(STRINGSIZE);                                    // this will last for the life of the command
    int d, m, y;
    if (!checkstring(ep, (unsigned char *)"NOW"))
    {
        arg = getCstring(ep);
        getargs(&arg, 5, (unsigned char*)"-/");										// this is a macro and must be the first executable stmt in a block
        if (!(argc == 5))error((char *)"Syntax");
        d = atoi((const char *)(const char*)argv[0]);
        m = atoi((const char *)(const char*)argv[2]);
        y = atoi((const char *)(const char*)argv[4]);
        if (d > 1000) {
            int tmp = d;
            d = y;
            y = tmp;
        }
        if (y >= 0 && y < 100) y += 2000;
        if (d < 1 || d > 31 || m < 1 || m > 12 || y < 1902 || y > 2999) error((char *)"Invalid date");
        tm->tm_year = y - 1900;
        tm->tm_mon = m - 1;
        tm->tm_mday = d;
        tm->tm_hour = 0;
        tm->tm_min = 0;
        tm->tm_sec = 0;
        time_of_day = timegm(tm);
        tm = gmtime(&time_of_day);
        i = tm->tm_wday;
        if (i == 0)i = 7;
        strcpy((char *)sret, daystrings[i]);
    }
    else {
        SYSTEMTIME lt;
        GetLocalTime(&lt);
        strcpy((char *)sret, daystrings[lt.wDayOfWeek]);
    }
    CtoM(sret);
    targ = T_STR;
}
void fun_epoch(void) {
    unsigned char* arg;
    struct tm* tm;
    struct tm tma;
    tm = &tma;
    int d, m, y, h, min, s;
    if (!checkstring(ep, (unsigned char*)"NOW"))
    {
        arg = getCstring(ep);
        getargs(&arg, 11, (unsigned char *)"-/ :");                                      // this is a macro and must be the first executable stmt in a block
        if (!(argc == 11)) error((char *)"Syntax");
        d = atoi((const char *)argv[0]);
        m = atoi((const char *)argv[2]);
        y = atoi((const char *)argv[4]);
        if (d > 1000) {
            int tmp = d;
            d = y;
            y = tmp;
        }
        if (y >= 0 && y < 100) y += 2000;
        if (d < 1 || d > 31 || m < 1 || m > 12 || y < 1902 || y > 2999) error((char *)"Invalid date");
        h = atoi((const char *)argv[6]);
        min = atoi((const char *)argv[8]);
        s = atoi((const char *)argv[10]);
        if (h < 0 || h > 23 || min < 0 || m > 59 || s < 0 || s > 59) error((char *)"Invalid time");
        //            day = d;
        //            month = m;
        //            year = y;
        tm->tm_year = y - 1900;
        tm->tm_mon = m - 1;
        tm->tm_mday = d;
        tm->tm_hour = h;
        tm->tm_min = min;
        tm->tm_sec = s;
    }
    else {
        SYSTEMTIME lt;
        GetLocalTime(&lt);
        tm->tm_year = lt.wYear - 1900;
        tm->tm_mon = lt.wMonth - 1;
        tm->tm_mday = lt.wDay;
        tm->tm_hour = lt.wHour;
        tm->tm_min = lt.wMinute;
        tm->tm_sec = lt.wSecond;
    }
    time_t timestamp = timegm(tm); /* See README.md if your system lacks timegm(). */
    iret = timestamp;
    targ = T_INT;
}
void fun_datetime(void) {
    sret = (unsigned char*)GetTempMemory(STRINGSIZE);                                    // this will last for the life of the command
    if (checkstring(ep, (unsigned char*)"NOW")) {
        SYSTEMTIME lt;
        GetLocalTime(&lt);
        IntToStrPad((char*)sret, lt.wDay, '0', 2, 10);
        sret[2] = '-'; IntToStrPad((char*)sret + 3, lt.wMonth, '0', 2, 10);
        sret[5] = '-'; IntToStr((char*)sret + 6, lt.wYear, 10);
        sret[10] = ' ';
        IntToStrPad((char*)sret + 11, lt.wHour, '0', 2, 10);
        sret[13] = ':'; IntToStrPad((char*)sret + 14, lt.wMinute, '0', 2, 10);
        sret[16] = ':'; IntToStrPad((char*)sret + 17, lt.wSecond, '0', 2, 10);
    }
    else {
        struct tm* tm;
        struct tm tma;
        tm = &tma;
        time_t timestamp = getinteger(ep); /* See README.md if your system lacks timegm(). */
        if (timestamp < 0)error((char*)"Epoch<0");
        tm = gmtime(&timestamp);
        IntToStrPad((char*)sret, tm->tm_mday, '0', 2, 10);
        sret[2] = '-'; IntToStrPad((char*)sret + 3, tm->tm_mon + 1, '0', 2, 10);
        sret[5] = '-'; IntToStr((char*)sret + 6, tm->tm_year + 1900, 10);
        sret[10] = ' ';
        IntToStrPad((char*)sret + 11, tm->tm_hour, '0', 2, 10);
        sret[13] = ':'; IntToStrPad((char*)sret + 14, tm->tm_min, '0', 2, 10);
        sret[16] = ':'; IntToStrPad((char*)sret + 17, tm->tm_sec, '0', 2, 10);
    }
    CtoM(sret);
    targ = T_STR;
}
void integersort(int64_t* iarray, int n, long long* index, int flags, int startpoint) {
    int i, j = n, s = 1;
    int64_t t;
    if ((flags & 1) == 0) {
        while (s) {
            s = 0;
            for (i = 1; i < j; i++) {
                if (iarray[i] < iarray[i - 1]) {
                    t = iarray[i];
                    iarray[i] = iarray[i - 1];
                    iarray[i - 1] = t;
                    s = 1;
                    if (index != NULL) {
                        t = index[i - 1 + startpoint];
                        index[i - 1 + startpoint] = index[i + startpoint];
                        index[i + startpoint] = t;
                    }
                }
            }
            j--;
        }
    }
    else {
        while (s) {
            s = 0;
            for (i = 1; i < j; i++) {
                if (iarray[i] > iarray[i - 1]) {
                    t = iarray[i];
                    iarray[i] = iarray[i - 1];
                    iarray[i - 1] = t;
                    s = 1;
                    if (index != NULL) {
                        t = index[i - 1 + startpoint];
                        index[i - 1 + startpoint] = index[i + startpoint];
                        index[i + startpoint] = t;
                    }
                }
            }
            j--;
        }
    }
}
void floatsort(MMFLOAT* farray, int n, long long* index, int flags, int startpoint) {
    int i, j = n, s = 1;
    int64_t t;
    MMFLOAT f;
    if ((flags & 1) == 0) {
        while (s) {
            s = 0;
            for (i = 1; i < j; i++) {
                if (farray[i] < farray[i - 1]) {
                    f = farray[i];
                    farray[i] = farray[i - 1];
                    farray[i - 1] = f;
                    s = 1;
                    if (index != NULL) {
                        t = index[i - 1 + startpoint];
                        index[i - 1 + startpoint] = index[i + startpoint];
                        index[i + startpoint] = t;
                    }
                }
            }
            j--;
        }
    }
    else {
        while (s) {
            s = 0;
            for (i = 1; i < j; i++) {
                if (farray[i] > farray[i - 1]) {
                    f = farray[i];
                    farray[i] = farray[i - 1];
                    farray[i - 1] = f;
                    s = 1;
                    if (index != NULL) {
                        t = index[i - 1 + startpoint];
                        index[i - 1 + startpoint] = index[i + startpoint];
                        index[i + startpoint] = t;
                    }
                }
            }
            j--;
        }
    }
}

void stringsort(unsigned char* sarray, int n, int offset, long long* index, int flags, int startpoint) {
    int ii, i, s = 1;
    long long int isave;
    int k;
    unsigned char* s1, * s2, * p1, * p2;
    unsigned char temp;
    int reverse = 1 - ((flags & 1) << 1);
    while (s) {
        s = 0;
        for (i = 1; i < n; i++) {
            s2 = i * offset + sarray;
            s1 = (i - 1) * offset + sarray;
            ii = *s1 < *s2 ? *s1 : *s2; //get the smaller  length
            p1 = s1 + 1; p2 = s2 + 1;
            k = 0; //assume the strings match
            while ((ii--) && (k == 0)) {
                if (flags & 2) {
                    if (toupper(*p1) > toupper(*p2)) {
                        k = reverse; //earlier in the array is bigger
                    }
                    if (toupper(*p1) < toupper(*p2)) {
                        k = -reverse; //later in the array is bigger
                    }
                }
                else {
                    if (*p1 > *p2) {
                        k = reverse; //earlier in the array is bigger
                    }
                    if (*p1 < *p2) {
                        k = -reverse; //later in the array is bigger
                    }
                }
                p1++; p2++;
            }
            // if up to this point the strings match
            // make the decision based on which one is shorter
            if (k == 0) {
                if (*s1 > *s2) k = reverse;
                if (*s1 < *s2) k = -reverse;
            }
            if (k == 1) { // if earlier is bigger swap them round
                ii = *s1 > *s2 ? *s1 : *s2; //get the bigger length
                ii++;
                p1 = s1; p2 = s2;
                while (ii--) {
                    temp = *p1;
                    *p1 = *p2;
                    *p2 = temp;
                    p1++; p2++;
                }
                s = 1;
                if (index != NULL) {
                    isave = index[i - 1 + startpoint];
                    index[i - 1 + startpoint] = index[i + startpoint];
                    index[i + startpoint] = isave;
                }
            }
        }
    }
}
void cmd_sort(void) {
    void* ptr1 = NULL;
    void* ptr2 = NULL;
    MMFLOAT* a3float = NULL;
    int64_t* a3int = NULL, * a4int = NULL;
    unsigned char* a3str = NULL;
    int i, size, truesize, flags = 0, maxsize = 0, startpoint = 0;
    getargs(&cmdline, 9, (unsigned char *)(unsigned char *)(unsigned char *)",");
    ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
    if (vartbl[VarIndex].type & T_NBR) {
        if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
        if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
            error((char *)"Argument 1 must be array");
        }
        a3float = (MMFLOAT*)ptr1;
    }
    else if (vartbl[VarIndex].type & T_INT) {
        if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
        if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
            error((char *)"Argument 1 must be array");
        }
        a3int = (int64_t*)ptr1;
    }
    else if (vartbl[VarIndex].type & T_STR) {
        if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
        if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
            error((char *)"Argument 1 must be array");
        }
        a3str = (unsigned char*)ptr1;
        maxsize = vartbl[VarIndex].size;
    }
    else error((char *)"Argument 1 must be array");
    if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Argument 1 must be array");
    truesize = size = (vartbl[VarIndex].dims[0] - OptionBase);
    if (argc >= 3 && *argv[2]) {
        ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
                error((char *)"Argument 2 must be integer array");
            }
            a4int = (int64_t*)ptr2;
        }
        else error((char *)"Argument 2 must be integer array");
        if ((vartbl[VarIndex].dims[0] - OptionBase) != size)error((char *)"Arrays should be the same size");
        if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Argument 2 must be array");
    }
    if (argc >= 5 && *argv[4])flags = (int)getint(argv[4], 0, 3);
    if (argc >= 7 && *argv[6])startpoint = (int)getint(argv[6], OptionBase, size + OptionBase);
    size -= startpoint;
    if (argc == 9)size = (int)getint(argv[8], 1, size + 1 + OptionBase) - 1;
    if (startpoint)startpoint -= OptionBase;
    if (a3float != NULL) {
        a3float += startpoint;
        if (a4int != NULL)for (i = 0; i < truesize + 1; i++)a4int[i] = i + OptionBase;
        floatsort(a3float, size + 1, a4int, flags, startpoint);
    }
    else if (a3int != NULL) {
        a3int += startpoint;
        if (a4int != NULL)for (i = 0; i < truesize + 1; i++)a4int[i] = i + OptionBase;
        integersort(a3int, size + 1, a4int, flags, startpoint);
    }
    else if (a3str != NULL) {
        a3str += ((startpoint) * (maxsize + 1));
        if (a4int != NULL)for (i = 0; i < truesize + 1; i++)a4int[i] = i + OptionBase;
        stringsort(a3str, size + 1, maxsize + 1, a4int, flags, startpoint);
    }
}

void cmd_longString(void) {
    unsigned char* tp;
    tp = checkstring(cmdline, (unsigned char *)"SETBYTE");
    if (tp) {
        void* ptr1 = NULL;
        int64_t* dest = NULL;
        int p = 0;
        uint8_t* q = NULL;
        int nbr;
        int j = 0;
        getargs(&tp, 5, (unsigned char *)(unsigned char *)",");
        if (argc != 5)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
                error((char *)"Argument 1 must be integer array");
            }
            j = (vartbl[VarIndex].dims[0] - OptionBase) * 8 - 1;
            dest = (long long int*)ptr1;
            q = (uint8_t*)&dest[1];
        }
        else error((char *)"Argument 1 must be integer array");
        p = (int)getint(argv[2], OptionBase, j - OptionBase);
        nbr = (int)getint(argv[4], 0, 255);
        q[p - OptionBase] = nbr;
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"APPEND");
    if (tp) {
        void* ptr1 = NULL;
        int64_t* dest = NULL;
        unsigned char* p = NULL;
        unsigned char* q = NULL;
        int i, j, nbr;
        getargs(&tp, 3, (unsigned char *)(unsigned char *)",");
        if (argc != 3)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 1 must be integer array");
            }
            dest = (long long int*)ptr1;
            q = (unsigned char*)&dest[1];
            q += dest[0];
        }
        else error((char *)"Argument 1 must be integer array");
        j = (vartbl[VarIndex].dims[0] - OptionBase);
        p = getstring(argv[2]);
        nbr = i = *p++;
        if (j * 8 < dest[0] + i)error((char *)"Integer array too small");
        while (i--)*q++ = *p++;
        dest[0] += nbr;
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"TRIM");
    if (tp) {
        void* ptr1 = NULL;
        int64_t* dest = NULL;
        uint32_t trim;
        unsigned char* p = NULL;
        unsigned char* q = NULL;
        int i;
        getargs(&tp, 3, (unsigned char *)(unsigned char *)",");
        if (argc != 3)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 1 must be integer array");
            }
            dest = (long long int*)ptr1;
            q = (unsigned char*)&dest[1];
        }
        else error((char *)"Argument 1 must be integer array");
        trim = (unsigned int)getint(argv[2], 1, dest[0] - 1);
        i = (int)dest[0] - trim;
        p = q + trim;
        while (i--)*q++ = *p++;
        dest[0] -= trim;
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"REPLACE");
    if (tp) {
        void* ptr1 = NULL;
        int64_t* dest = NULL;
        unsigned char* p = NULL;
        unsigned char* q = NULL;
        int i, nbr;
        getargs(&tp, 5, (unsigned char *)(unsigned char *)",");
        if (argc != 5)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 1 must be integer array");
            }
            dest = (long long int*)ptr1;
            q = (unsigned char*)&dest[1];
        }
        else error((char *)"Argument 1 must be integer array");
        p = getstring(argv[2]);
        nbr = (int)getint(argv[4], 1, dest[0] - *p + 1);
        q += nbr - 1;
        i = *p++;
        while (i--)*q++ = *p++;
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"LOAD");
    if (tp) {
        void* ptr1 = NULL;
        int64_t* dest = NULL;
        unsigned char* p = NULL;
        unsigned char* q = NULL;
        int i, j;
        getargs(&tp, 5, (unsigned char *)(unsigned char *)",");
        if (argc != 5)error((char *)"Argument count");
        int nbr = (int)getinteger(argv[2]);
        i = nbr;
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 1 must be integer array");
            }
            dest = (long long int*)ptr1;
            dest[0] = 0;
            q = (unsigned char*)&dest[1];
        }
        else error((char *)"Argument 1 must be integer array");
        j = (vartbl[VarIndex].dims[0] - OptionBase);
        p = getstring(argv[4]);
        if (nbr > *p)nbr = *p;
        p++;
        if (j * 8 < dest[0] + nbr)error((char *)"Integer array too small");
        while (i--)*q++ = *p++;
        dest[0] += nbr;
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"LEFT");
    if (tp) {
        void* ptr1 = NULL;
        void* ptr2 = NULL;
        int64_t* dest = NULL, * src = NULL;
        unsigned char* p = NULL;
        unsigned char* q = NULL;
        int i, j, nbr;
        getargs(&tp, 5, (unsigned char *)(unsigned char *)",");
        if (argc != 5)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 1 must be integer array");
            }
            dest = (int64_t*)ptr1;
            q = (unsigned char*)&dest[1];
        }
        else error((char *)"Argument 1 must be integer array");
        j = (vartbl[VarIndex].dims[0] - OptionBase);
        ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 2 must be integer array");
            }
            src = (int64_t*)ptr2;
            p = (unsigned char*)&src[1];
        }
        else error((char *)"Argument 2 must be integer array");
        nbr = i = (int)getinteger(argv[4]);
        if (nbr > (int)src[0])nbr = i = (int)src[0];
        if (j * 8 < i)error((char *)"Destination array too small");
        while (i--)*q++ = *p++;
        dest[0] = nbr;
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"RIGHT");
    if (tp) {
        void* ptr1 = NULL;
        void* ptr2 = NULL;
        int64_t* dest = NULL, * src = NULL;
        unsigned char* p = NULL;
        unsigned char* q = NULL;
        int i, j, nbr;
        getargs(&tp, 5, (unsigned char *)(unsigned char *)",");
        if (argc != 5)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 1 must be integer array");
            }
            dest = (int64_t*)ptr1;
            q = (unsigned char*)&dest[1];
        }
        else error((char *)"Argument 1 must be integer array");
        j = (vartbl[VarIndex].dims[0] - OptionBase);
        ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 2 must be integer array");
            }
            src = (int64_t*)ptr2;
            p = (unsigned char*)&src[1];
        }
        else error((char *)"Argument 2 must be integer array");
        nbr = i = (int)getinteger(argv[4]);
        if (nbr > src[0]) {
            nbr = i = (int)src[0];
        }
        else p += (src[0] - nbr);
        if (j * 8 < i)error((char *)"Destination array too small");
        while (i--)*q++ = *p++;
        dest[0] = nbr;
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"MID");
    if (tp) {
        void* ptr1 = NULL;
        void* ptr2 = NULL;
        int64_t* dest = NULL, * src = NULL;
        unsigned char* p = NULL;
        unsigned char* q = NULL;
        int i, j, nbr, start;
        getargs(&tp, 7, (unsigned char *)(unsigned char *)",");
        if (argc != 7)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 1 must be integer array");
            }
            dest = (int64_t*)ptr1;
            q = (unsigned char*)&dest[1];
        }
        else error((char *)"Argument 1 must be integer array");
        j = (vartbl[VarIndex].dims[0] - OptionBase);
        ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 2 must be integer array");
            }
            src = (int64_t*)ptr2;
            p = (unsigned char*)&src[1];
        }
        else error((char *)"Argument 2 must be integer array");
        start = (int)getint(argv[4], 1, src[0]);
        nbr = (int)getinteger(argv[6]);
        p += start - 1;
        if (nbr + start > src[0]) {
            nbr = (int)src[0] - start + 1;
        }
        i = nbr;
        if (j * 8 < nbr)error((char *)"Destination array too small");
        while (i--)*q++ = *p++;
        dest[0] = nbr;
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"CLEAR");
    if (tp) {
        void* ptr1 = NULL;
        int64_t* dest = NULL;
        getargs(&tp, 1, (unsigned char *)(unsigned char *)",");
        if (argc != 1)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 1 must be integer array");
            }
            dest = (long long int*)ptr1;
        }
        else error((char *)"Argument 1 must be integer array");
        dest[0] = 0;
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"RESIZE");
    if (tp) {
        void* ptr1 = NULL;
        int64_t* dest = NULL;
        int j = 0;
        getargs(&tp, 3, (unsigned char *)(unsigned char *)",");
        if (argc != 3)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
                error((char *)"Argument 1 must be integer array");
            }
            j = (vartbl[VarIndex].dims[0] - OptionBase) * 8;
            dest = (long long int*)ptr1;
        }
        else error((char *)"Argument 1 must be integer array");
        dest[0] = getint(argv[2], 0, j);
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"UCASE");
    if (tp) {
        void* ptr1 = NULL;
        int64_t* dest = NULL;
        char* q = NULL;
        int i;
        getargs(&tp, 1, (unsigned char *)(unsigned char *)",");
        if (argc != 1)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 1 must be integer array");
            }
            dest = (long long int*)ptr1;
            q = (char*)&dest[1];
        }
        else error((char *)"Argument 1 must be integer array");
        i = (int)dest[0];
        while (i--) {
            if (*q >= 'a' && *q <= 'z')
                *q -= 0x20;
            q++;
        }
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"PRINT");
    if (tp) {
        void* ptr1 = NULL;
        int64_t* dest = NULL;
        char* q = NULL;
        int i, j, fnbr;
        getargs(&tp, 5, (unsigned char*)",;");
        if (argc < 1 || argc > 4)error((char *)"Argument count");
        if (argc > 0 && *argv[0] == '#') {                                // check if the first arg is a file number
            argv[0]++;
            fnbr = (int)getinteger(argv[0]);                                 // get the number
            i = 1;
            if (argc >= 2 && *argv[1] == ',') i = 2;                      // and set the next argument to be looked at
        }
        else {
            fnbr = 0;                                                   // no file number so default to the standard output
            i = 0;
        }
        if (argc >= 1) {
            ptr1 = findvar(argv[i], V_FIND | V_EMPTY_OK);
            if (vartbl[VarIndex].type & T_INT) {
                if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
                if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                    error((char *)"Argument must be integer array");
                }
                dest = (long long int*)ptr1;
                q = (char*)&dest[1];
            }
            else error((char *)"Argument must be integer array");
            j = (int)dest[0];
            while (j--) {
                MMfputc(*q++, fnbr);
            }
            i++;
        }
        if (argc > i) {
            if (*argv[i] == ';') return;
        }
        MMfputs((unsigned char*)"\2\r\n", fnbr);
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"LCASE");
    if (tp) {
        void* ptr1 = NULL;
        int64_t* dest = NULL;
        char* q = NULL;
        int i;
        getargs(&tp, 1, (unsigned char *)(unsigned char *)",");
        if (argc != 1)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 1 must be integer array");
            }
            dest = (long long int*)ptr1;
            q = (char*)&dest[1];
        }
        else error((char *)"Argument 1 must be integer array");
        i = (int)dest[0];
        while (i--) {
            if (*q >= 'A' && *q <= 'Z')
                *q += 0x20;
            q++;
        }
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"COPY");
    if (tp) {
        void* ptr1 = NULL;
        void* ptr2 = NULL;
        int64_t* dest = NULL, * src = NULL;
        char* p = NULL;
        char* q = NULL;
        int i = 0, j;
        getargs(&tp, 3, (unsigned char *)(unsigned char *)",");
        if (argc != 3)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 1 must be integer array");
            }
            dest = (int64_t*)ptr1;
            dest[0] = 0;
            q = (char*)&dest[1];
        }
        else error((char *)"Argument 1 must be integer array");
        j = (vartbl[VarIndex].dims[0] - OptionBase);
        ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 2 must be integer array");
            }
            src = (int64_t*)ptr2;
            p = (char*)&src[1];
            i = (int)src[0];
        }
        else error((char *)"Argument 2 must be integer array");
        if (j * 8 < i)error((char *)"Destination array too small");
        while (i--)*q++ = *p++;
        dest[0] = src[0];
        return;
    }
    tp = checkstring(cmdline, (unsigned char *)"CONCAT");
    if (tp) {
        void* ptr1 = NULL;
        void* ptr2 = NULL;
        int64_t* dest = NULL, * src = NULL;
        char* p = NULL;
        char* q = NULL;
        int i = 0, j, d = 0, s = 0;
        getargs(&tp, 3, (unsigned char *)(unsigned char *)",");
        if (argc != 3)error((char *)"Argument count");
        ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 1 must be integer array");
            }
            dest = (int64_t*)ptr1;
            d = (int)dest[0];
            q = (char*)&dest[1];
        }
        else error((char *)"Argument 1 must be integer array");
        j = (vartbl[VarIndex].dims[0] - OptionBase);
        ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
                error((char *)"Argument 2 must be integer array");
            }
            src = (int64_t*)ptr2;
            p = (char*)&src[1];
            i = s = (int)src[0];
        }
        else error((char *)"Argument 2 must be integer array");
        if (j * 8 < (d + s))error((char *)"Destination array too small");
        q += d;
        while (i--)*q++ = *p++;
        dest[0] += src[0];
        return;
    }
    error((char *)"Invalid option");
}
void fun_LGetStr(void) {
    void* ptr1 = NULL;
    unsigned char* p;
    unsigned char* s = NULL;
    int64_t* src = NULL;
    int start, nbr, j;
    getargs(&ep, 5, (unsigned char *)",");
    if (argc != 5)error((char *)"Argument count");
    ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
    if (vartbl[VarIndex].type & T_INT) {
        if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
        if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
            error((char *)"Argument 1 must be integer array");
        }
        src = (int64_t*)ptr1;
        s = (unsigned char*)&src[1];
    }
    else error((char *)"Argument 1 must be integer array");
    j = (vartbl[VarIndex].dims[0] - OptionBase) * 8;
    start = (int)getint(argv[2], 1, j);
    nbr = (int)getinteger(argv[4]);
    if (nbr < 1 || nbr > MAXSTRLEN) error((char *)"Number out of bounds");
    if (start + nbr > src[0])nbr = (int)src[0] - start + 1;
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);                                       // this will last for the life of the command
    s += (start - 1);
    p = sret + 1;
    *sret = nbr;
    while (nbr--)*p++ = *s++;
    *p = 0;
    targ = T_STR;
}

void fun_LGetByte(void) {
    void* ptr1 = NULL;
    uint8_t* s = NULL;
    int64_t* src = NULL;
    int start, j;
    getargs(&ep, 3, (unsigned char *)",");
    if (argc != 3)error((char *)"Argument count");
    ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
    if (vartbl[VarIndex].type & T_INT) {
        if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
        if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
            error((char *)"Argument 1 must be integer array");
        }
        src = (int64_t*)ptr1;
        s = (uint8_t*)&src[1];
    }
    else error((char *)"Argument 1 must be integer array");
    j = (vartbl[VarIndex].dims[0] - OptionBase) * 8 - 1;
    start = (int)getint(argv[2], OptionBase, j - OptionBase);
    iret = s[start - OptionBase];
    targ = T_INT;
}


void fun_LInstr(void) {
    void* ptr1 = NULL;
    int64_t* dest = NULL;
    unsigned char* srch;
    char* str = NULL;
    int slen, found = 0, i, j, n;
    getargs(&ep, 5, (unsigned char *)",");
    if (argc < 3 || argc > 5)error((char *)"Argument count");
    int64_t start;
    if (argc == 5)start = getinteger(argv[4]) - 1;
    else start = 0;
    ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
    if (vartbl[VarIndex].type & T_INT) {
        if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
        if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
            error((char *)"Argument 1 must be integer array");
        }
        dest = (long long int*)ptr1;
        str = (char*)&dest[0];
    }
    else error((char *)"Argument 1 must be integer array");
    j = (vartbl[VarIndex].dims[0] - OptionBase);
    srch = getstring(argv[2]);
    slen = *srch;
    iret = 0;
    if (start > dest[0] || start<0 || slen == 0 || dest[0] == 0 || slen>dest[0] - start)found = 1;
    if (!found) {
        n = (int)dest[0] - slen - (int)start;

        for (i = (int)start; i <= n + (int)start; i++) {
            if (str[i + 8] == srch[1]) {
                for (j = 0; j < slen; j++)
                    if (str[j + i + 8] != srch[j + 1])
                        break;
                if (j == slen) { iret = i + 1; break; }
            }
        }
    }
    targ = T_INT;
}

void fun_LCompare(void) {
    void* ptr1 = NULL;
    void* ptr2 = NULL;
    int64_t* dest, * src;
    char* p = NULL;
    char* q = NULL;
    int d = 0, s = 0, found = 0;
    getargs(&ep, 3, (unsigned char *)",");
    if (argc != 3)error((char *)"Argument count");
    ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
    if (vartbl[VarIndex].type & T_INT) {
        if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
        if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
            error((char *)"Argument 1 must be integer array");
        }
        dest = (int64_t*)ptr1;
        q = (char*)&dest[1];
        d = (int)dest[0];
    }
    else error((char *)"Argument 1 must be integer array");
    ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK);
    if (vartbl[VarIndex].type & T_INT) {
        if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
        if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
            error((char *)"Argument 2 must be integer array");
        }
        src = (int64_t*)ptr2;
        p = (char*)&src[1];
        s = (int)src[0];
    }
    else error((char *)"Argument 2 must be integer array");
    while (!found) {
        if (d == 0 && s == 0) { found = 1; iret = 0; }
        if (d == 0 && !found) { found = 1; iret = -1; }
        if (s == 0 && !found) { found = 1; iret = 1; }
        if (*q < *p && !found) { found = 1; iret = -1; }
        if (*q > *p && !found) { found = 1; iret = 1; }
        q++;  p++;  d--; s--;
    }
    targ = T_INT;
}

void fun_LLen(void) {
    void* ptr1 = NULL;
    int64_t* dest = NULL;
    getargs(&ep, 1, (unsigned char *)",");
    if (argc != 1)error((char *)"Argument count");
    ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK);
    if (vartbl[VarIndex].type & T_INT) {
        if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
        if (vartbl[VarIndex].dims[0] <= 0) {      // Not an array
            error((char *)"Argument 1 must be integer array");
        }
        dest = (long long int*)ptr1;
    }
    else error((char *)"Argument 1 must be integer array");
    iret = dest[0];
    targ = T_INT;
}
void fun_format(void) {
    unsigned char* p, * fmt;
    int inspec;
    getargs(&ep, 3, (unsigned char *)",");
    if (argc % 2 == 0) error((char *)"Invalid syntax");
    if (argc == 3)
        fmt = getCstring(argv[2]);
    else
        fmt = (unsigned char*)"%g";

    // check the format string for errors that might crash the CPU
    for (inspec = 0, p = fmt; *p; p++) {
        if (*p == '%') {
            inspec++;
            if (inspec > 1) error((char *)"Only one format specifier (%) allowed");
            continue;
        }

        if (inspec == 1 && (*p == 'g' || *p == 'G' || *p == 'f' || *p == 'e' || *p == 'E' || *p == 'l'))
            inspec++;


        if (inspec == 1 && !(IsDigitinline(*p) || *p == '+' || *p == '-' || *p == '.' || *p == ' '))
            error((char*)"Illegal character in format specification");
    }
    if (inspec != 2) error((char*)"Format specification not found");
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);									// this will last for the life of the command
    sprintf((char *)sret, (const char *)fmt, getnumber(argv[0]));
    CtoM(sret);
    targ = T_STR;
}


// set up the tick interrupt
void cmd_settick(void) {
    int period;
    int irq = 0;;
    int pause = 0;
    unsigned char s[STRINGSIZE];
    getargs(&cmdline, 5, (unsigned char*)",");
    strcpy((char *)s, (const char *)argv[0]);
    if (!(argc == 3 || argc == 5)) error((char *)"Argument count");
    if (argc == 5) irq = (int)getint(argv[4], 1, NBRSETTICKS) - 1;
    if (strncasecmp((const char *)argv[0], (const char*)"PAUSE", 6) == 0) {
        TickActive[irq] = 0;
        return;
    }
    else if (strncasecmp((const char*)argv[0], (const char*)"RESUME",6) == 0) {
        TickActive[irq] = 1;
        return;
    }
    else period = (int)getint(argv[0], -1, INT_MAX);
    if (period == 0) {
        TickInt[irq] = NULL;                                        // turn off the interrupt
    }
    else {
        TickPeriod[irq] = period;
        TickInt[irq] = GetIntAddress(argv[2]);                      // get a pointer to the interrupt routine
        TickTimer[irq] = 0;                                         // set the timer running
        InterruptUsed = true;
        TickActive[irq] = 1;

    }
}

void fun_mouse(void) {
    iret = -1;
    int chan = 2;
    unsigned char* p;
    getargs(&ep, 3, (unsigned char *)",");
    p = (unsigned char *)argv[0];
    if (toupper(*p) == 'X')iret = mouse_xpos;
    else if (checkstring(p, (unsigned char*)"REF"))
        iret = CurrentRef;
    else if (checkstring(p, (unsigned char*)"LASTREF"))
        iret = LastRef;
    else if (checkstring(p, (unsigned char*)"LASTX"))
        iret = LastX;
    else if (checkstring(p, (unsigned char*)"LASTY"))
        iret = LastY;
    else if (checkstring(p, (unsigned char*)"DOWN"))
        iret = TOUCH_DOWN;
    else if (checkstring(p, (unsigned char*)"UP"))
        iret = !TOUCH_DOWN;
    else if (toupper(*p) == 'Y')iret = mouse_ypos;
    else if (toupper(*p) == 'L')iret = mouse_left;
    else if (toupper(*p) == 'R')iret = mouse_right;
    else if (toupper(*p) == 'M')iret = mouse_middle;
    else if (toupper(*p) == 'D') { 
        iret = MouseDouble; 
        MouseDouble = 0; 
    }
    else if (toupper(*p) == 'W') {
        int scale=1;
        if (argc == 3)scale= (int)getint(argv[2], 0, 100);
        iret = mouse_wheel/scale;
        mouse_wheel = 0;
    }
    else error((char *)"Syntax");
    targ = T_INT;
}
void cmd_mouse(void) {
    getargs(&cmdline, 5, (unsigned char*)",");
    if (argc == 0)error((char*)"Syntax");
    if (*argv[0]) {
        MouseFoundLeftDown = 0;
        InterruptUsed = true;
    }
    if (argc >= 3 && *argv[2]) {
        MouseInterrupRightDown = GetIntAddress(argv[2]);					// get the interrupt location
        MouseFoundRightDown = 0;
        InterruptUsed = true;
    }
    if (argc ==5 && *argv[4]) {
        MouseInterrupLeftUp = GetIntAddress(argv[4]);					// get the interrupt location
        MouseFoundLeftUp = 0;
        InterruptUsed = true;
    }

}
void fun_keydown(void) {
    int n = (int)getint(ep, 0, 8);
    iret = 0;
    while (getConsole(1) != -1); // clear anything in the input buffer
    if (n == 8) {
        iret = (shiftlock ? 1 : 0) |
            (numlock ? 2 : 0) |
            (scrolllock ? 4 : 0);
    }
    else if (n == 7)iret = modifiers;
    else if (n) {
        iret = (n <= KEYLIFOpointer ? KEYLIFO[n - 1] : 0);											        // this is the character
    }
    else {
        iret = KEYLIFOpointer;
    }
    targ = T_INT;
}
// utility function used by fun_peek() to validate an address
extern "C" unsigned int GetPeekAddr(unsigned char* p) {
    unsigned int i;
    i = (unsigned int)getinteger(p);
    if(!POKERANGE(i)) error((char *)"Address");
    return i;
}

// Will return a byte within the PIC32 virtual memory space.
void fun_peek(void) {
    unsigned char* p, *pp;
    getargs(&ep, 3, (unsigned char *)",");

    if ((p = checkstring(argv[0], (unsigned char*)"BYTE"))) {
        if (argc != 1) error((char *)"Syntax");
        iret = *(unsigned char*)GetPeekAddr(p);
        targ = T_INT;
        return;
    }

    if ((p = checkstring(argv[0], (unsigned char*)"VAR"))) {
        pp = (unsigned char *)findvar(p, V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
        iret = *((char*)pp + (int)getinteger(argv[2]));
        targ = T_INT;
        return;
    }

    if ((p = checkstring(argv[0], (unsigned char*)"VARADDR"))) {
        if (argc != 1) error((char *)"Syntax");
        pp = (unsigned char*)findvar(p, V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
        iret = (unsigned int)pp;
        targ = T_INT;
        return;
    }


    if ((p = checkstring(argv[0], (unsigned char*)"WORD"))) {
        if (argc != 1) error((char *)"Syntax");
        iret = *(unsigned int*)(GetPeekAddr(p) & 0b11111111111111111111111111111100);
        targ = T_INT;
        return;
    }
    if ((p = checkstring(argv[0], (unsigned char*)"SHORT"))) {
        if (argc != 1) error((char *)"Syntax");
        iret = (unsigned long long int) (*(unsigned short*)(GetPeekAddr(p) & 0b11111111111111111111111111111110));
        targ = T_INT;
        return;
    }
    if ((p = checkstring(argv[0], (unsigned char*)"INTEGER"))) {
        if (argc != 1) error((char *)"Syntax");
        iret = *(uint64_t*)(GetPeekAddr(p) & 0xFFFFFFF8);
        targ = T_INT;
        return;
    }

    if ((p = checkstring(argv[0], (unsigned char*)"FLOAT"))) {
        if (argc != 1) error((char *)"Syntax");
        fret = *(MMFLOAT*)(GetPeekAddr(p) & 0xFFFFFFF8);
        targ = T_NBR;
        return;
    }

    if (argc != 3) error((char *)"Syntax");

    if ((checkstring(argv[0], (unsigned char*)"PROGMEM"))) {
        iret = *((char*)ProgMemory + (int)getinteger(argv[2]));
        targ = T_INT;
        return;
    }

    if ((checkstring(argv[0], (unsigned char*)"VARTBL"))) {
        iret = *((char*)vartbl + (int)getinteger(argv[2]));
        targ = T_INT;
        return;
    }


    // default action is the old syntax of  b = PEEK(hiaddr, loaddr)
    iret = *(char*)(((int)getinteger(argv[0]) << 16) + (int)getinteger(argv[2]));
    targ = T_INT;
}
extern "C" unsigned int GetPokeAddr(unsigned char* p) {
    unsigned int i;
    i = (unsigned int)getinteger(p);
    if (!POKERANGE(i)) error((char*)"Address");
    return i;
}

void cmd_poke(void) {
    unsigned char* p, *pp;
        getargs(&cmdline, 5, (unsigned char*)",");
        if ((p = checkstring(argv[0], (unsigned char*)"BYTE"))) {
            if (argc != 3) error((char *)"Argument count");
            uint32_t a = GetPokeAddr(p);
            uint8_t* padd = (uint8_t*)(a);
            *padd = (uint8_t)getinteger(argv[2]);
            return;
        }
        if ((p = checkstring(argv[0], (unsigned char*)"SHORT"))) {
            if (argc != 3) error((char *)"Argument count");
            uint32_t a = GetPokeAddr(p);
            if (a % 2)error((char *)"Address not divisible by 2");
            uint16_t* padd = (uint16_t*)(a);
            *padd = (uint16_t)getinteger(argv[2]);
            return;
        }

        if ((p = checkstring(argv[0], (unsigned char*)"WORD"))) {
            if (argc != 3) error((char *)"Argument count");
            uint32_t a = GetPokeAddr(p);
            if (a % 4)error((char *)"Address not divisible by 4");
            uint32_t* padd = (uint32_t*)(a);
            *padd = (uint32_t)getinteger(argv[2]);
            return;
        }

        if ((p = checkstring(argv[0], (unsigned char*)"INTEGER"))) {
            if (argc != 3) error((char *)"Argument count");
            uint32_t a = GetPokeAddr(p);
            if (a % 8)error((char *)"Address not divisible by 8");
            uint64_t* padd = (uint64_t*)(a);
            *padd = getinteger(argv[2]);
            return;
        }
        if ((p = checkstring(argv[0], (unsigned char*)"FLOAT"))) {
            if (argc != 3) error((char *)"Argument count");
            uint32_t a = GetPokeAddr(p);
            if (a % 8)error((char *)"Address not divisible by 8");
            MMFLOAT* padd = (MMFLOAT*)(a);
            *padd = getnumber(argv[2]);
            return;
        }

        if (argc != 5) error((char *)"Argument count");

        if (checkstring(argv[0], (unsigned char*)"VARTBL")) {
            *((char*)vartbl + (unsigned int)getinteger(argv[2])) = (uint8_t)getinteger(argv[4]);
            return;
        }
        if ((p = checkstring(argv[0], (unsigned char*)"VAR"))) {
            pp = (unsigned char*)findvar(p, V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
            if (vartbl[VarIndex].type & T_CONST) error((char *)"Cannot change a constant");
            *((char*)pp + (unsigned int)getinteger(argv[2])) = (uint8_t)getinteger(argv[4]);
            return;
        }
        // the default is the old syntax of:   POKE hiaddr, loaddr, byte
        *(char*)(((int)getinteger(argv[0]) << 16) + (int)getinteger(argv[2])) = (uint8_t)getinteger(argv[4]);
}