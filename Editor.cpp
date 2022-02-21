/***********************************************************************************************************************
MMBasic for Windows

Editor.cpp

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
#include "MMBasic_Includes.h"



#define CTRLKEY(a) (a & 0x1f)


#define DISPLAY_CLS             1
#define REVERSE_VIDEO           3
#define CLEAR_TO_EOL            4
#define CLEAR_TO_EOS            5
#define SCROLL_DOWN             6
#define DRAW_LINE               7

//#if defined(MX470)
//#define GUI_C_NORMAL            Option.DefaultFC
//#else
#define GUI_C_NORMAL            M_WHITE
#define GUI_C_BCOLOUR           M_BLACK
#define GUI_C_COMMENT           M_YELLOW
#define GUI_C_KEYWORD           M_CYAN
#define GUI_C_QUOTE             M_GREEN
#define GUI_C_NUMBER            M_RUST
#define GUI_C_LINE              M_MAGENTA
#define GUI_C_STATUS            M_WHITE
#define MAXCLIPLEN              2048
//======================================================================================
//      Attribute               VT100 Code      VT 100 Colour       LCD Screen Colour
//======================================================================================
/*#define VT100_C_NORMAL          "\033[37m"      // White            Foreground Colour
#define VT100_C_COMMENT         "\033[33m"      // Yellow               Yellow
#define VT100_C_KEYWORD         "\033[36m"      // Cyan                 Cyan
#define VT100_C_QUOTE           "\033[35m"      // Green                Green
#define VT100_C_NUMBER          "\033[32m"      // Red                  Red
#define VT100_C_LINE            "\033[37m"      // White                Grey
#define VT100_C_STATUS          "\033[37m"      // Black                Brown
#define VT100_C_ERROR           "\033[31m"      // Red                  Red*/

// these two are the only global variables, the default place for the cursor when the editor opens
#if !defined(LITE)


int OriginalFC, OriginalBC;                                     // the original fore/background colours used by MMBasic

#define MX470PutC(c)        DisplayPutC(c)
#define MX470Scroll(n)      ScrollLCD(n)

//    #define dx(...) {char s[140];sprintf(s,  __VA_ARGS__); SerUSBPutS(s); SerUSBPutS("\r\n");}

void MX470PutS(unsigned char* s, int64_t fc, int64_t bc) {
    int64_t tfc, tbc;
    tfc = gui_fcolour; tbc = gui_bcolour;
    gui_fcolour = fc; gui_bcolour = bc;
    DisplayPutS(s);
    gui_fcolour = tfc; gui_bcolour = tbc;
}


void MX470Cursor(int x, int y) {
        CurrentX = x;
        CurrentY = y;
}

void MX470Display(int fn) {
    int64_t t;
    switch (fn) {
    case DISPLAY_CLS:       ClearScreen(gui_bcolour);
        break;
    case REVERSE_VIDEO:     t = gui_fcolour;
        gui_fcolour = gui_bcolour;
        gui_bcolour = t;
        break;
    case CLEAR_TO_EOL:      DrawBox(CurrentX, CurrentY, HRes, CurrentY + gui_font_height, 0, 0, gui_bcolour);
        break;
    case CLEAR_TO_EOS:      DrawBox(CurrentX, CurrentY, HRes, CurrentY + gui_font_height, 0, 0, gui_bcolour);
        DrawRectangle(0, CurrentY + gui_font_height, HRes - 1, VRes - 1, gui_bcolour);
        break;
    case SCROLL_DOWN:
        break;
    case DRAW_LINE:         DrawBox(0, gui_font_height * (Option.Height - 2), HRes - 1, VRes - 1, 0, 0, gui_bcolour);
        DrawLine(0, VRes - gui_font_height - 6, HRes - 1, VRes - gui_font_height - 6, 1, GUI_C_LINE);
        CurrentX = 0; CurrentY = VRes - gui_font_height;
        break;
    }
}



/********************************************************************************************************************************************
 THE EDIT COMMAND
********************************************************************************************************************************************/

unsigned char* EdBuff;                     // the buffer used for editing the text
int nbrlines;                     // size of the text held in the buffer (in lines)
int VWidth, VHeight;              // editing screen width and height in characters
int edx, edy;                     // column and row at the top left hand corner of the editing screen (in characters)
int curx, cury;                   // column and row of the current cursor (in characters) relative to the top left hand corner of the editing screen
unsigned char* txtp;                       // position of the current cursor in the text being edited
int drawstatusline;               // true if the status line needs to be redrawn on the next keystroke
int insert;                       // true if the editor is in insert text mode
int tempx;                        // used to track the prefered x position when up/down arrowing
int TextChanged;                  // true if the program has been modified and therefor a save might be required

#define EDIT  1                   // used to select the status line string
#define MARK  2

void FullScreenEditor(void);
char* findLine(int ln);
void printLine(int ln);
void printScreen(void);
void SCursor(int x, int y);
int editInsertChar(unsigned char c);
void PrintFunctKeys(int);
void PrintStatus(void);
void SaveToProgMemory(void);
void editDisplayMsg(unsigned char* msg);
void GetInputString(unsigned char* prompt);
void Scroll(void);
void ScrollDown(void);
void MarkMode(unsigned char* cb, unsigned char* buf);
void PositionCursor(unsigned char* curp);


// edit command:
//  EDIT              Will run the full screen editor on the current program memory, if run after an error will place the cursor on the error line
void cmd_edit(void) {
    unsigned char* fromp=NULL, * p;
    int y, x;
    checkend(cmdline);
    if (CurrentLinePtr) error((char *)"Invalid in a program");
    if (Option.ColourCode) {
        gui_fcolour = M_WHITE;
        gui_bcolour = M_BLACK;
    }
    if (gui_font_width > 16) error((char *)"Font is too large");
    ClearRuntime();
    EdBuff = (unsigned char *)GetTempMemory(EDIT_BUFFER_SIZE);
    *EdBuff = 0;

    VHeight = Option.Height - 2;
    VWidth = Option.Width;
    edx = edy = curx = cury = y = x = tempx = 0;
    txtp = EdBuff;
    *tknbuf = 0;

    fromp = ProgMemory;
    p = EdBuff;
    nbrlines = 0;
    while (*fromp != 0xff) {
        if (*fromp == T_NEWLINE) {
            if (StartEditPoint >= ProgMemory) {
                if (StartEditPoint == fromp) {
                    y = nbrlines;                                   // we will start editing at this line
                    tempx = x = StartEditChar;
                    txtp = p + StartEditChar;
                }
            }
            else {
                if (StartEditPoint == (unsigned char*)nbrlines) {
                    y = nbrlines;
                    tempx = x = StartEditChar;
                    txtp = p + StartEditChar;
                }
            }
            nbrlines++;
            fromp = llist(p, fromp);                                // otherwise expand the line
            p += strlen((const char *)p);
            *p++ = '\n'; *p = 0;
        }
        // finally, is it the end of the program?
        if (fromp[0] == 0 || fromp[0] == 0xff) break;
    }
    if (nbrlines == 0) nbrlines++;
    if (p > EdBuff) --p;
    *p = 0;                                                         // erase the last line terminator

    MX470Display(DISPLAY_CLS);                                      // clear screen on the MX470 display only
    SCursor(0, 0);
    PrintFunctKeys(EDIT);

    if (nbrlines > VHeight) {
        edy = y - VHeight / 2;                                        // edy is the line displayed at the top
        if (edy < 0) edy = 0;                                        // compensate if we are near the start
        y = y - edy;                                                // y is the line on the screen
    }
    printScreen();                                                  // draw the screen
    SCursor(x, y);
    drawstatusline = true;
    FullScreenEditor();
    memset(tknbuf, 0, STRINGSIZE);                                  // zero this so that nextstmt is pointing to the end of program
    MMCharPos = 0;
}



void FullScreenEditor(void) {
    int c, i;
    unsigned char buf[STRINGSIZE + 2], clipboard[MAXCLIPLEN];
    unsigned char* p, * tp, BreakKeySave;
    unsigned char lastkey = 0;
    int y, statuscount;

    clipboard[0] = 0;
    insert = true;
    TextChanged = false;
    BreakKeySave = BreakKey;
    BreakKey = 0;
    while (1) {
        statuscount = 0;
        do {
            ShowMMBasicCursor(true);
            c = getConsole(0);
            PrintStatus();
        } while (c == -1);
        ShowMMBasicCursor(false);

        if (drawstatusline) PrintFunctKeys(EDIT);
        drawstatusline = false;
        if (c == TAB) {
            strcpy((char *)buf, "        ");
            buf[Option.Tab - ((edx + curx) % Option.Tab)] = 0;
        }
        else {
            buf[0] = c;
            buf[1] = 0;
        }
        do {
            if (buf[0] == BreakKeySave) buf[0] = ESC;                                // if the user tried to break turn it into an escape
            switch (buf[0]) {

            case '\r':
            case '\n':  // first count the spaces at the beginning of the line
                if (txtp != EdBuff && (*txtp == '\n' || *txtp == 0)) {   // we only do this if we are at the end of the line
                    for (tp = txtp - 1, i = 0; *tp != '\n' && tp >= EdBuff; tp--)
                        if (*tp != ' ')
                            i = 0;                                      // not a space
                        else
                            i++;                                        // potential space at the start
                    if (tp == EdBuff && *tp == ' ') i++;                 // correct for a counting error at the start of the buffer
                    if (buf[1] != 0)
                        i = 0;                                          // do not insert spaces if buffer too small or has something in it
                    else
                        buf[i + 1] = 0;                                 // make sure that the end of the buffer is zeroed
                    while (i) buf[i--] = ' ';                            // now, place our spaces in the typeahead buffer
                }
                if (!editInsertChar('\n')) break;                        // insert the newline
                TextChanged = true;
                nbrlines++;
                if (!(cury < VHeight - 1))                               // if we are NOT at the bottom
                    edy++;                                              // otherwise scroll
                printScreen();                                          // redraw everything
                PositionCursor(txtp);
                break;

            case CTRLKEY('E'):
            case UP:    if (cury == 0 && edy == 0) break;
                if (*txtp == '\n') txtp--;                               // step back over the terminator if we are right at the end of the line
                while (txtp != EdBuff && *txtp != '\n') txtp--;          // move to the beginning of the line
                if (txtp != EdBuff) {
                    txtp--;                                             // step over the terminator to the end of the previous line
                    while (txtp != EdBuff && *txtp != '\n') txtp--;      // move to the beginning of that line
                    if (*txtp == '\n') txtp++;                           // and position at the start
                }
                for (i = 0; i < edx + tempx && *txtp != 0 && *txtp != '\n'; i++, txtp++);  // move the cursor to the column

                if (cury > 2 || edy == 0) {                              // if we are more that two lines from the top
                    if (cury > 0) SCursor(i, cury - 1);                  // just move the cursor up
                }
                else if (edy > 0) {                                      // if we are two lines or less from the top
                    curx = i;
                    ScrollDown();
                }
                PositionCursor(txtp);
                break;

            case CTRLKEY('X'):
            case DOWN:  p = txtp;
                while (*p != 0 && *p != '\n') p++;                       // move to the end of this line
                if (*p == 0) break;                                      // skip if it is at the end of the file
                p++;                                                    // step over the line terminator to the start of the next line
                for (i = 0; i < edx + tempx && *p != 0 && *p != '\n'; i++, p++);  // move the cursor to the column
                txtp = p;

                if (cury < VHeight - 3 || edy + VHeight == nbrlines) {
                    if (cury < VHeight - 1) SCursor(i, cury + 1);
                }
                else if (edy + VHeight < nbrlines) {
                    curx = i;
                    Scroll();
                }
                PositionCursor(txtp);
                break;

            case CTRLKEY('S'):
            case LEFT:  if (txtp == EdBuff) break;
                if (*(txtp - 1) == '\n') {                               // if at the beginning of the line wrap around
                    buf[1] = UP;
                    buf[2] = END;
                    buf[3] = 1;
                    buf[4] = 0;
                }
                else {
                    txtp--;
                    PositionCursor(txtp);
                }
                break;

            case CTRLKEY('D'):
            case RIGHT: if (*txtp == '\n') {                                   // if at the end of the line wrap around
                buf[1] = HOME;
                buf[2] = DOWN;
                buf[3] = 0;
                break;
            }
                      if (curx >= VWidth) {
                          editDisplayMsg((unsigned char *)" LINE IS TOO LONG ");
                          break;
                      }

                      if (*txtp != 0) txtp++;                                  // now we can move the cursor
                      PositionCursor(txtp);
                      break;

                      // backspace
            case BKSP:  if (txtp == EdBuff) break;
                if (*(txtp - 1) == '\n') {                               // if at the beginning of the line wrap around
                    buf[1] = UP;
                    buf[2] = END;
                    buf[3] = DEL;
                    buf[4] = 0;
                    break;
                }
                // find how many spaces are between the cursor and the start of the line
                for (p = txtp - 1; *p == ' ' && p != EdBuff; p--);
                if ((p == EdBuff || *p == '\n') && txtp - p > 1) {
                    i = txtp - p - 1;
                    // we have have the number of continuous spaces between the cursor and the start of the line
                    // now figure out the number of backspaces to the nearest tab stop

                    i = (i % Option.Tab); if (i == 0) i = Option.Tab;
                    // load the corresponding number of deletes in the type ahead buffer
                    buf[i + 1] = 0;
                    while (i--) {
                        buf[i + 1] = DEL;
                        txtp--;
                    }
                    // and let the delete case take care of deleting the characters
                    PositionCursor(txtp);
                    break;
                }
                // this is just a normal backspace (not a tabbed backspace)
                txtp--;
                PositionCursor(txtp);
                // fall through to delete the char

            case CTRLKEY(']'):
            case DEL: if (*txtp == 0) break;
                p = txtp;
                c = *p;
                while (*p) {
                    p[0] = p[1];
                    p++;
                }
                if (c == '\n') {
                    printScreen();
                    nbrlines--;
                }
                else
                    printLine(edy + cury);
                TextChanged = true;
                PositionCursor(txtp);
                break;

            case CTRLKEY('N'):
            case INSERT:insert = !insert;
                break;

            case CTRLKEY('U'):
            case HOME:  if (txtp == EdBuff) break;
                if (lastkey == HOME || lastkey == CTRLKEY('U')) {
                    edx = edy = curx = cury = 0;
                    txtp = EdBuff;
                    MX470Display(DISPLAY_CLS);                          // clear screen on the MX470 display only
                    printScreen();
                    PrintFunctKeys(EDIT);
                    PositionCursor(txtp);
                    break;
                }
                if (*txtp == '\n') txtp--;                               // step back over the terminator if we are right at the end of the line
                while (txtp != EdBuff && *txtp != '\n') txtp--;          // move to the beginning of the line
                if (*txtp == '\n') txtp++;                               // skip if no more lines above this one
                PositionCursor(txtp);
                break;

            case CTRLKEY('K'):
            case END: if (*txtp == 0) break;                                     // already at the end
                if (lastkey == END || lastkey == CTRLKEY('K')) {         // jump to the end of the file
                    i = 0; p = txtp = EdBuff;
                    while (*txtp != 0) {
                        if (*txtp == '\n') { p = txtp + 1; i++; }
                        txtp++;
                    }

                    if (i >= VHeight) {
                        edy = i - VHeight + 1;
                        printScreen();
                        cury = VHeight - 1;
                    }
                    else {
                        cury = i;
                    }
                    txtp = p;
                    curx = 0;
                }

                while (curx < VWidth && *txtp != 0 && *txtp != '\n') {
                    txtp++;
                    PositionCursor(txtp);
                }
                if (curx > VWidth) editDisplayMsg((unsigned char*)" LINE IS TOO LONG ");
                break;

            case CTRLKEY('P'):
            case PUP: if (edy == 0) {                                            // if we are already showing the top of the text
                buf[1] = HOME;                                      // force the editing point to the start of the text
                buf[2] = HOME;
                buf[3] = 0;
                break;
            }
                    else if (edy >= VHeight - 1) {                         // if we can scroll a full screenfull
                i = VHeight + 1;
                edy -= VHeight;
            }
                    else {                                                // if it is less than a full screenfull
                i = edy + 1;
                edy = 0;
            }
                    while (i--) {
                        if (*txtp == '\n') txtp--;                           // step back over the terminator if we are right at the end of the line
                        while (txtp != EdBuff && *txtp != '\n') txtp--;      // move to the beginning of the line
                        if (txtp == EdBuff) break;                           // skip if no more lines above this one
                    }
                    if (txtp != EdBuff) txtp++;                              // and position at the start of the line
                    for (i = 0; i < edx + curx && *txtp != 0 && *txtp != '\n'; i++, txtp++);  // move the cursor to the column
                    printScreen();
                    PositionCursor(txtp);
                    break;

            case CTRLKEY('L'):
            case PDOWN: if (nbrlines <= edy + VHeight + 1) {                     // if we are already showing the end of the text
                buf[1] = END;                                       // force the editing point to the end of the text
                buf[2] = END;
                buf[3] = 0;
                break;                                              // cursor to the top line
            }
                      else if (nbrlines - edy - VHeight >= VHeight) {        // if we can scroll a full screenfull
                edy += VHeight;
                i = VHeight;
            }
                      else {                                                // if it is less than a full screenfull
                i = nbrlines - VHeight - edy;
                edy = nbrlines - VHeight;
            }
                      if (*txtp == '\n') i--;                                  // compensate if we are right at the end of a line
                      while (i--) {
                          if (*txtp == '\n') txtp++;                           // step over the terminator if we are right at the start of the line
                          while (*txtp != 0 && *txtp != '\n') txtp++;          // move to the end of the line
                          if (*txtp == 0) break;                               // skip if no more lines after this one
                      }
                      if (txtp != EdBuff) txtp++;                              // and position at the start of the line
                      for (i = 0; i < edx + curx && *txtp != 0 && *txtp != '\n'; i++, txtp++);  // move the cursor to the column
                      //y = cury;
                      printScreen();
                      PositionCursor(txtp);
                      break;

                      // Abort without saving
            case ESC:                                                               // wait 50ms to see if anything more is coming
                // this must be an ordinary escape (not part of an escape code)
                 if (TextChanged) {
                    GetInputString((unsigned char*)"Exit and discard all changes (Y/N): ");
                    if (mytoupper(*inpbuf) != 'Y') break;
                }
                // fall through to the normal exit

            case CTRLKEY('Q'):   // Save and exit
            case F1:             // Save and exit
            case CTRLKEY('W'):   // Save, exit and run
            case F2:             // Save, exit and run
                gui_fcolour = PromptFC;
                gui_bcolour = PromptBC;
                MX470Display(DISPLAY_CLS);                        // clear screen on the MX470 display only
                MX470Cursor(0, 0);                                // home the cursor
                BreakKey = BreakKeySave;
                if (buf[0] != ESC && TextChanged) SaveToProgMemory();
                if (buf[0] == ESC || buf[0] == CTRLKEY('Q') || buf[0] == F1) return;
                // this must be save, exit and run.  We have done the first two, now do the run part.
                ClearRuntime();
                //                            WatchdogSet = false;
                PrepareProgram(true);
                nextstmt = ProgMemory;
                return;

                // Search
            case CTRLKEY('R'):
            case F3:    GetInputString((unsigned char*)"Find (Use SHIFT-F3 to repeat): ");
                if (*inpbuf == 0 || *inpbuf == ESC) break;
                if (!(*inpbuf == 0xD3 || *inpbuf == F3)) strcpy((char *)tknbuf, (const char*)inpbuf);
                // fall through

            case CTRLKEY('G'):
            case 0xD3:  // SHIFT-F3
                p = txtp;
                if (*p == 0) p = EdBuff - 1;
                i = strlen((const char *)tknbuf);
                while (1) {
                    p++;
                    if (p == txtp) break;
                    if (*p == 0) p = EdBuff;
                    if (p == txtp) break;
                    if (mem_equal(p, tknbuf, i)) break;
                }
                if (p == txtp) {
                    editDisplayMsg((unsigned char*)" NOT FOUND ");
                    break;
                }
                for (y = 0, txtp = EdBuff; txtp != p; txtp++) {          // find the line and column of the string
                    if (*txtp == '\n') {
                        y++;                                            // y is the line
                    }
                }
                edy = y - VHeight / 2;                                    // edy is the line displayed at the top
                if (edy < 0) edy = 0;                                    // compensate if we are near the start
                printScreen();
                PositionCursor(txtp);
                // SCursor(x, y);
                break;

                // Mark
            case CTRLKEY('T'):
            case F4:    MarkMode(clipboard, &buf[1]);
                printScreen();
                PrintFunctKeys(EDIT);
                PositionCursor(txtp);
                break;

            case CTRLKEY('Y'):
            case CTRLKEY('V'):
            case F5:    if (*clipboard == 0) {
                editDisplayMsg((unsigned char*)" CLIPBOARD IS EMPTY ");
                break;
            }
                   for (i = 0; clipboard[i]; i++) buf[i + 1] = clipboard[i];
                   buf[i + 1] = 0;
                   break;

                   // F6 to F12 - Normal function keys
            case F6:
            case F7:
            case F8:
            case F9:
            case F10:
            case F11:
            case F12:
                break;

                // a normal character
            default:    c = buf[0];
                if (c < ' ' || c > '~') break;                           // make sure that this is valid
                if (curx >= VWidth) {
                    editDisplayMsg((unsigned char*)" LINE IS TOO LONG ");
                    break;
                }
                TextChanged = true;
                if (insert || *txtp == '\n' || *txtp == 0) {
                    if (!editInsertChar(c)) break;                       // insert it
                }
                else
                    *txtp++ = c;                                        // or just overtype
                printLine(edy + cury);                            // redraw the whole line so that colour coding will occur
                PositionCursor(txtp);
                // SCursor(x, cury);
                tempx = cury;                                           // used to track the preferred cursor position
                break;

            }
            lastkey = buf[0];
            if (buf[0] != UP && buf[0] != DOWN && buf[0] != CTRLKEY('E') && buf[0] != CTRLKEY('X')) tempx = curx;
            buf[STRINGSIZE + 1] = 0;
            for (i = 0; i < STRINGSIZE + 1; i++) buf[i] = buf[i + 1];                // suffle down the buffer to get the next char
        } while (*buf);
    }
}


/*******************************************************************************************************************
  UTILITY FUNCTIONS USED BY THE FULL SCREEN EDITOR
*******************************************************************************************************************/


void PositionCursor(unsigned char* curp) {
    int ln, col;
    unsigned char* p;

    for (p = EdBuff, ln = col = 0; p < curp; p++) {
        if (*p == '\n') {
            ln++;
            col = 0;
        }
        else
            col++;
    }
    if (ln < edy || ln >= edy + VHeight) return;
    SCursor(col, ln - edy);
}



// mark mode
// implement the mark mode (when the user presses F4)
void MarkMode(unsigned char* cb, unsigned char* buf) {
    unsigned char *p, *mark, *oldmark;
    int c, x, y, i, oldx, oldy, txtpx, txtpy, errmsg = false;

    PrintFunctKeys(MARK);
    oldmark = mark = txtp;
    txtpx = oldx = curx; txtpy = oldy = cury;
    while (1) {
        c = getConsole(0);
        if (c != -1 && errmsg) {
            PrintFunctKeys(MARK);
            errmsg = false;
        }
        switch (c) {
        case ESC:                                         // wait 50ms to see if anything more is coming
            curx = txtpx; cury = txtpy;                             // just an escape key
            return;

        case CTRLKEY('E'):
        case UP:  if (cury <= 0) continue;
            p = mark;
            if (*p == '\n') p--;                                     // step back over the terminator if we are right at the end of the line
            while (p != EdBuff && *p != '\n') p--;                   // move to the beginning of the line
            if (p != EdBuff) {
                p--;                                                // step over the terminator to the end of the previous line
                for (i = 0; p != EdBuff && *p != '\n'; p--, i++);    // move to the beginning of that line
                if (*p == '\n') p++;                                 // and position at the start
                if (i >= VWidth) {
                    editDisplayMsg((unsigned char*)" LINE IS TOO LONG ");
                    errmsg = true;
                    continue;
                }
            }
            mark = p;
            for (i = 0; i < edx + curx && *mark != 0 && *mark != '\n'; i++, mark++);  // move the cursor to the column
            curx = i; cury--;
            break;

        case CTRLKEY('X'):
        case DOWN:if (cury == VHeight - 1) continue;
            for (p = mark, i = curx; *p != 0 && *p != '\n'; p++, i++);// move to the end of this line
            if (*p == 0) continue;                                    // skip if it is at the end of the file
            if (i >= VWidth) {
                editDisplayMsg((unsigned char*)" LINE IS TOO LONG ");
                errmsg = true;
                continue;
            }
            mark = p + 1;                                 // step over the line terminator to the start of the next line
            for (i = 0; i < edx + curx && *mark != 0 && *mark != '\n'; i++, mark++);  // move the cursor to the column
            curx = i; cury++;
            break;

        case CTRLKEY('S'):
        case LEFT:if (curx == edx) continue;
            mark--;
            curx--;
            break;

        case CTRLKEY('D'):
        case RIGHT:if (curx >= VWidth || *mark == 0 || *mark == '\n') continue;
            mark++;
            curx++;
            break;

        case CTRLKEY('U'):
        case HOME:if (mark == EdBuff) break;
            if (*mark == '\n') mark--;                     // step back over the terminator if we are right at the end of the line
            while (mark != EdBuff && *mark != '\n') mark--;// move to the beginning of the line
            if (*mark == '\n') mark++;                     // skip if no more lines above this one
            break;

        case CTRLKEY('K'):
        case END: if (*mark == 0) break;
            for (p = mark, i = curx; *p != 0 && *p != '\n'; p++, i++);// move to the end of this line
            if (i >= VWidth) {
                editDisplayMsg((unsigned char*)" LINE IS TOO LONG ");
                errmsg = true;
                continue;
            }
            mark = p;
            break;

        case CTRLKEY('Y'):
        case CTRLKEY('T'):
        case F5:
        case F4:  if (txtp - mark > MAXCLIPLEN || mark - txtp > MAXCLIPLEN) {
            editDisplayMsg((unsigned char*)" MARKED TEXT EXCEEDS BUFFER CHARACTERS ");
            errmsg = true;
            break;
        }
               if (mark <= txtp) {
                   p = mark;
                   while (p < txtp) *cb++ = *p++;
               }
               else {
                   p = txtp;
                   while (p <= mark - 1) *cb++ = *p++;
               }
               *cb = 0;
               if (c == F5 || c == CTRLKEY('Y')) {
                   PositionCursor(txtp);
                   return;
               }
               // fall through

        case CTRLKEY(']'):
        case DEL: if (mark < txtp) {
            p = txtp;  txtp = mark; mark = p;         // swap txtp and mark
        }
                for (p = txtp; p < mark; p++) if (*p == '\n') nbrlines--;
                for (p = txtp; *mark; ) *p++ = *mark++;
                *p++ = 0; *p++ = 0;
                TextChanged = true;
                PositionCursor(txtp);
                return;

        default:    continue;
        }

        x = curx; y = cury;

        // first unmark the area not marked as a result of the keystroke
        if (oldmark < mark) {
            PositionCursor(oldmark);
            p = oldmark;
            while (p < mark) {
                if (*p == '\n') {  MX470PutC('\r'); }// also print on the MX470 display
                MX470PutC(*p++);                                      // print on the MX470 display
            }
        }
        else if (oldmark > mark) {
            PositionCursor(mark);
            p = mark;
            while (oldmark > p) {
                if (*p == '\n') { MX470PutC('\r'); }// also print on the MX470 display
                MX470PutC(*p++);                                      // print on the MX470 display
            }
        }

        oldmark = mark; oldx = x; oldy = y;

        // now draw the marked area
        if (mark < txtp) {
            PositionCursor(mark);
            MX470Display(REVERSE_VIDEO);                            // reverse video on the MX470 display only
            p = mark;
            while (p < txtp) {
                if (*p == '\n') {
                    MX470PutC('\r');               // also print on the MX470 display
                }
                MX470PutC(*p++);                                      // print on the MX470 display
            }
            MX470Display(REVERSE_VIDEO);                            // reverse video back to normal on the MX470 display only
        }
        else if (mark > txtp) {
            PositionCursor(txtp);
            MX470Display(REVERSE_VIDEO);                            // reverse video on the MX470 display only
            p = txtp;
            while (p < mark) {
                if (*p == '\n') {
                    MX470PutC('\r');               // also print on the MX470 display
                }
                MX470PutC(*p++);                                      // print on the MX470 display
            }
            MX470Display(REVERSE_VIDEO);                            // reverse video back to normal on the MX470 display only
        }

        oldx = x; oldy = y; oldmark = mark;
        PositionCursor(mark);
    }
}




// search through the text in the editing buffer looking for a specific line
// enters with ln = the line required
// exits pointing to the start of the line or pointing to a zero char if not that many lines in the buffer
char* findLine(int ln) {
    unsigned char* p;
    p = EdBuff;
    while (ln && *p) {
        if (*p == '\n') ln--;
        p++;
    }
    return (char *)p;
}


int EditCompStr(char* p, char* tkn) {
    while (*tkn && (toupper(*tkn) == toupper(*p))) {
        if (*tkn == '(' && *p == '(') return true;
        if (*tkn == '$' && *p == '$') return true;
        tkn++; p++;
    }
    if (*tkn == 0 && !isnamechar(*p)) return true;                   // return the string if successful

    return false;                                                   // or NULL if not
}


// this function does the syntax colour coding
// p = pointer to the current character to be printed
//     or NULL if the colour coding is to be reset to normal
//
// it keeps track of where it is in the line using static variables
// so it must be fed all chars from the start of the line
void SetColour(unsigned char* p, int DoVT100) {
    int i;
    unsigned char** pp;
    static int intext = false;
    static int incomment = false;
    static int inkeyword = false;
    static unsigned char* twokeyword = NULL;
    static int inquote = false;
    static int innumber = false;

    if (!Option.ColourCode) return;

    // this is a list of keywords that can come after the OPTION and GUI commands
    // the list must be terminated with a NULL
    unsigned char* twokeywordtbl[] = {
        (unsigned char*)"BASE", (unsigned char*)"EXPLICIT", (unsigned char*)"DEFAULT", (unsigned char*)"BREAK", (unsigned char*)"AUTORUN", (unsigned char*)"BAUDRATE", (unsigned char*)"DISPLAY",
        (unsigned char*)"BUTTON", (unsigned char*)"SWITCH", (unsigned char*)"CHECKBOX", (unsigned char*)"RADIO", (unsigned char*)"LED", (unsigned char*)"FRAME", 
        (unsigned char*)"NUMBERBOX", (unsigned char*)"SPINBOX", (unsigned char*)"TEXTBOX", (unsigned char*)"DISPLAYBOX", (unsigned char*)"CAPTION", (unsigned char*)"DELETE",
        (unsigned char*)"DISABLE", (unsigned char*)"HIDE", (unsigned char*)"ENABLE", (unsigned char*)"SHOW", (unsigned char*)"FCOLOUR", (unsigned char*)"BCOLOUR",
        (unsigned char*)"REDRAW", (unsigned char*)"BEEP", (unsigned char*)"INTERRUPT", NULL
    };

    // this is a list of common keywords that should be highlighted as such
    // the list must be terminated with a NULL
    unsigned char* specialkeywords[] = {
        (unsigned char*)"SELECT", (unsigned char*)"INTEGER", (unsigned char*)"FLOAT", (unsigned char*)"STRING", (unsigned char*)"DISPLAY", (unsigned char*)"SDCARD",
        (unsigned char*)"OUTPUT", (unsigned char*)"APPEND", (unsigned char*)"WRITE", (unsigned char*)"SLAVE", NULL
    };


    // reset everything back to normal
    if (p == NULL) {
        innumber = inquote = inkeyword = incomment = intext = false;
        twokeyword = NULL;
        gui_fcolour = GUI_C_NORMAL;
        return;
    }

    // check for a comment char
    if (*p == '\'' && !inquote) {
        gui_fcolour = GUI_C_COMMENT;
        incomment = true;
        return;
    }

    // once in a comment all following chars must be comments also
    if (incomment) return;

    // check for a quoted string
    if (*p == '\"') {
        if (!inquote) {
            inquote = true;
            gui_fcolour = GUI_C_QUOTE;
            return;
        }
        else {
            inquote = false;
            return;
        }
    }

    if (inquote) return;

    // if we are displaying a keyword check that it is still actually in the keyword and reset if not
    if (inkeyword) {
        if (isnamechar(*p) || *p == '$') return;
        gui_fcolour = GUI_C_NORMAL;
        inkeyword = false;
        return;
    }

    // if we are displaying a number check that we are still actually in it and reset if not
    // this is complicated because numbers can be in hex or scientific notation
    if (innumber) {
        if (!isdigit(*p) && !(toupper(*p) >= 'A' && toupper(*p) <= 'F') && toupper(*p) != 'O' && toupper(*p) != 'H' && *p != '.') {
            gui_fcolour = GUI_C_NORMAL;
            innumber = false;
            return;
        }
        else {
            return;
        }
        // check if we are starting a number
    }
    else if (!intext) {
        if (isdigit(*p) || *p == '&' || ((*p == '-' || *p == '+' || *p == '.') && isdigit(p[1]))) {
            gui_fcolour = GUI_C_NUMBER;
            innumber = true;
            return;
        }
        // check if this is an 8 digit hex number as used in CFunctions
        for (i = 0; i < 8; i++) if (!isxdigit(p[i])) break;
        if (i == 8 && (p[8] == ' ' || p[8] == '\'' || p[8] == 0)) {
            gui_fcolour = GUI_C_NUMBER;
            innumber = true;
            return;
        }
    }

    // check if this is the start of a keyword
    if (isnamestart(*p) && !intext) {
        for (i = 0; i < CommandTableSize - 1; i++) {                 // check the command table for a match
            if (EditCompStr((char *)p, (char*)commandtbl[i].name) != 0) {
                if (EditCompStr((char *)p, (char*)"REM") != 0) {                 // special case, REM is a comment
                    gui_fcolour = GUI_C_COMMENT;
                    incomment = true;
                }
                else {
                    gui_fcolour = GUI_C_KEYWORD;
                    inkeyword = true;
                    if (EditCompStr((char *)p, (char *)"GUI") || EditCompStr((char *)p, (char*)"OPTION")) {
                        twokeyword = p;
                        while (isalnum(*twokeyword)) twokeyword++;
                        while (*twokeyword == ' ') twokeyword++;
                    }
                    return;
                }
            }
        }
        for (i = 0; i < TokenTableSize - 1; i++) {                   // check the token table for a match
            if (EditCompStr((char *)p, (char*)tokentbl[i].name) != 0) {
                gui_fcolour = GUI_C_KEYWORD;
                inkeyword = true;
                return;
            }
        }

        // check for the second keyword in two keyword commands
        if (p == twokeyword) {
            for (pp = twokeywordtbl; *pp; pp++)
                if (EditCompStr((char *)p, (char*)*pp)) break;
            if (*pp) {
                gui_fcolour = GUI_C_KEYWORD;
                inkeyword = true;
                return;
            }
        }
        if (p >= twokeyword) twokeyword = NULL;

        // check for a range of common keywords
        for (pp = specialkeywords; *pp; pp++)
            if (EditCompStr((char *)p, (char*)*pp)) break;
        if (*pp) {
            gui_fcolour = GUI_C_KEYWORD;
            inkeyword = true;
            return;
        }
    }

    // try to keep track of if we are in general text or not
    // this is to avoid recognising keywords or numbers inside variables
    if (isnamechar(*p)) {
        intext = true;
    }
    else {
        intext = false;
        gui_fcolour = GUI_C_NORMAL;
    }

}


// print a line starting at the current column (edx) at the current cursor.
// if the line is beyond the end of the text then just clear to the end of line
// enters with the line number to be printed
void printLine(int ln) {
    char* p;
    int i;

    // we always colour code the output to the LCD panel on the MX470 (when used as the console)
        MX470PutC('\r');                                            // print on the MX470 display
        p = findLine(ln);
        i = VWidth - 1;
        while (i && *p && *p != '\n') {
            SetColour((unsigned char*)p, false);                                    // set the colour for the LCD display only
            MX470PutC(*p++);                                        // print on the MX470 display
            i--;
        }
        MX470Display(CLEAR_TO_EOL);                                 // clear to the end of line on the MX470 display only
    SetColour(NULL, false);

    p = findLine(ln);
    if (Option.ColourCode) {
        // if we are colour coding we need to redraw the whole line
        i = VWidth - 1;
    }
    else {
        // if we are NOT colour coding we can start drawing at the current cursor position
        i = curx;
        while (i-- && *p && *p != '\n') p++;                         // find the editing point in the buffer
        i = VWidth - curx;
    }

    while (i && *p && *p != '\n') {
        if (Option.ColourCode) SetColour((unsigned char*)p, true);                   // if colour coding is used set the colour for the VT100 emulator
        p++;
        i--;
    }

    if (Option.ColourCode) SetColour(NULL, true);
    curx = VWidth - 1;
}



// print a full screen starting with the top left corner specified by edx, edy
// this draws the full screen including blank areas so there is no need to clear the screen first
// it then returns the cursor to its original position
void printScreen(void) {
    int i;

    SCursor(0, 0);
    for (i = 0; i < VHeight; i++) {
        printLine(i + edy);
        MX470PutS((unsigned char *)"\r\n", gui_fcolour, gui_bcolour);
        curx = 0;
        cury = i + 1;
    }
    while (getConsole(0) != -1);                                           // consume any keystrokes accumulated while redrawing the screen
}



// position the cursor on the screen
void SCursor(int x, int y) {
    char s[12];

    IntToStr(s, ((long long int)y + 1), 10);
    IntToStr(s, ((long long int)x + 1), 10);
    MX470Cursor(x * gui_font_width, y * gui_font_height);           // position the cursor on the MX470 display only
    curx = x; cury = y;
}



// move the text down by one char starting at the current position in the text
// and insert a character
int editInsertChar(unsigned char c) {
    unsigned char* p;

    for (p = EdBuff; *p; p++);                                         // find the end of the text in memory
    if (p >= EdBuff + EDIT_BUFFER_SIZE - 10) {                         // and check that we have the space (allow 10 bytes for slack)
        editDisplayMsg((unsigned char *)" OUT OF MEMORY ");
        return false;
    }
    for (; p >= txtp; p--) *(p + 1) = *p;                              // shift everything down
    *txtp++ = c;                                                      // and insert our char
    return true;
}



// print the function keys at the bottom of the screen
void PrintFunctKeys(int typ) {
    int x, y;
    unsigned char* p;

    if (typ == EDIT) {
        if (VWidth >= 78)
            p = (unsigned char *)"ESC:Exit  F1:Save  F2:Run  F3:Find  F4:Mark  F5:Paste";
        else if (VWidth >= 62)
            p = (unsigned char*)"F1:Save F2:Run F3:Find F4:Mark F5:Paste";
        else
            p = (unsigned char*)"EDIT MODE";
    }
    else {
        if (VWidth >= 49)
            p = (unsigned char*)"MARK MODE   ESC=Exit  DEL:Delete  F4:Cut  F5:Copy";
        else
            p = (unsigned char*)"MARK MODE";
    }

    MX470Display(DRAW_LINE);                                        // on the MX470 display draw the line
    MX470PutS(p, GUI_C_STATUS, gui_bcolour);                        // display the string on the display attached to the MX470
    MX470Display(CLEAR_TO_EOL);                                     // clear to the end of line on the MX470 display only

    x = curx; y = cury;
    SCursor(0, VHeight);
    SCursor(x, y);
}



// print the current status
void PrintStatus(void) {
    int tx;
    unsigned char s[MAXSTRLEN];

    tx = edx + curx + 1;
    strcpy((char *)s, "Ln: ");
    IntToStr((char*)s + strlen((const char *)s), edy + cury + 1, 10);
    strcat((char*)s + strlen((const char *)s), "  Col: ");
    IntToStr((char*)s + strlen((const char *)s), tx, 10);
    strcat((char*)s, "       ");
    strcpy((char*)s + 19, insert ? "INS" : "OVR");

    MX470Cursor((VWidth - strlen((const char *)s)) * gui_font_width, VRes - gui_font_height);
    MX470PutS(s, GUI_C_STATUS, gui_bcolour);                        // display the string on the display attached to the MX470

    SCursor(VWidth - 25, VHeight + 1);

    PositionCursor(txtp);
}



// display a message in the status line
void editDisplayMsg(unsigned char* msg) {
    SCursor(0, VHeight + 1);
    MX470Cursor(0, VRes - gui_font_height);
    MX470PutS(msg, M_BLACK, M_RED);
    MX470Display(CLEAR_TO_EOL);                                     // clear to the end of line on the MX470 display only
    PositionCursor(txtp);
    drawstatusline = true;
}



// save the program in the editing buffer into the program memory
void SaveToProgMemory(void) {
    SaveProgramToMemory(EdBuff, true);
    StartEditPoint = (unsigned char*)(edy + cury);                            // record out position in case the editor is invoked again
    StartEditChar = edx + curx;
    // bugfix for when the edit point is a space
    // the space could be at the end of a line which will be trimmed in SaveProgramToFlash() leaving StartEditChar referring to something not there
    // this is not a serious issue so fix the bug in the MX470 only because it has plenty of flash
    while (StartEditChar > 0 && txtp > EdBuff && *(--txtp) == ' ') {
        StartEditChar--;
    }
}



// get an input string from the user and save into inpbuf
void GetInputString(unsigned char* prompt) {
    int i;
    unsigned char* p;

    SCursor(0, VHeight + 1);
    MX470Cursor(0, VRes - gui_font_height);
    MX470PutS(prompt, gui_fcolour, gui_bcolour);
    for (i = 0; i < VWidth - (int)strlen((const char *)prompt); i++) {
        MX470PutC(' ');
    }
    SCursor(strlen((const char *)prompt), VHeight + 1);
    MX470Cursor(strlen((const char *)prompt) * gui_font_width, VRes - gui_font_height);
    for (p = inpbuf; (*p = MMgetchar()) != '\n'; p++) {              // get the input
        if (*p == 0xd3 || *p == F3 || *p == ESC) { p++; break; }     // return if it is SHIFT-F3, F3 or ESC
        if (isprint(*p)) {
            MX470PutC(*p);                                          // echo the char on the MX470 display
        }
        if (*p == '\b') {
            p--;                                                    // backspace over a backspace
            if (p >= inpbuf) {
                p--;                                                // and the char before
                MX470PutS((unsigned char *)"\b \b", gui_fcolour, gui_bcolour);       // erase on the MX470 display
            }
        }
    }
    *p = 0;                                                         // terminate the input string
    PrintFunctKeys(EDIT);
    PositionCursor(txtp);
}


// scroll up the video screen
void Scroll(void) {
    edy++;
    SCursor(0, VHeight);
    MX470Cursor(0, VHeight * gui_font_height);
    MX470Scroll(gui_font_height);
    SCursor(0, VHeight);
    curx = 0;
    cury = VHeight - 1;
    PrintFunctKeys(EDIT);
    printLine(VHeight - 1 + edy);
    PositionCursor(txtp);
    while (getConsole(0) != -1);                                         // consume any keystrokes accumulated while redrawing the screen
}


// scroll down the video screen
void ScrollDown(void) {
    SCursor(0, VHeight);                                            // go to the end of the editing area
    edy--;
    SCursor(0, 0);
    MX470Scroll(-gui_font_height);
    printLine(edy);
    PrintFunctKeys(EDIT);
    PositionCursor(txtp);
    while (getConsole(0) != -1);                                         // consume any keystrokes accumulated while redrawing the screen
}

#endif
