/* Kilo -- A very simple editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * Copyright (C) 2016 Salvatore Sanfilippo <antirez at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitteoption default mode 1
 d provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 /* Massively modified and enhanced by Peter Mather. Copyright 2019-2022
  *
  */#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "olcPixelGameEngine.h"
#include "MainThread.h"
#include "MMBasic_Includes.h"
#define DISPLAY_CLS             1
#define REVERSE_VIDEO           3
#define CLEAR_TO_EOL            4
#define CLEAR_TO_EOS            5
#define SCROLL_DOWN             6
#define DRAW_LINE               7

extern volatile struct s_nunstruct mousestruct[4];
extern volatile unsigned int MouseTimer;
extern char* firststmt;
extern unsigned char* nextstmt;
void cutpasteselectCallback(char* cutbuf, int key);
#define GUI_C_NORMAL            M_WHITE
#define GUI_C_BCOLOUR           M_BLACK
#define GUI_C_COMMENT           (RGB(192, 192, 0) | 0xFF000000)
#define GUI_C_KEYWORD           M_CYAN
#define GUI_C_QUOTE             (RGB(0, 200, 0) | 0xFF000000)
#define GUI_C_NUMBER            (RGB(255, 128, 128) | 0xFF000000)
#define GUI_C_LINE              M_GRAY
#define GUI_C_STATUS            M_WHITE
#define MX470PutC(c)        {DisplayPutC(c,0);}
#define MX470Scroll(n)      ScrollLCD(n, 1)
#define PaintOn 189
#define PaintOff 188
#define CLIPBOARDSIZE		8192					// maximum size of clipboard for edit
char lastfileedited[STRINGSIZE] = { 0 };
//    #define dx(...) {char s[140];sprintf(s,  __VA_ARGS__); SerUSBPutS(s); SerUSBPutS("\r\n");}
# define TOLOWER(Ch) tolower (Ch)
extern "C" int strcasecmp(const char* s1, const char* s2)
{
	const unsigned char* p1 = (const unsigned char*)s1;
	const unsigned char* p2 = (const unsigned char*)s2;
	int result;
	if (p1 == p2)
		return 0;
	while ((result = TOLOWER(*p1) - TOLOWER(*p2++)) == 0)
		if (*p1++ == '\0')
			break;
	return result;
}
static void MX470PutS(unsigned char* s, int fc, int bc) {
	int64_t tfc, tbc;
	tfc = gui_fcolour; tbc = gui_bcolour;
	gui_fcolour = fc; gui_bcolour = bc;
	DisplayPutS(s);
	gui_fcolour = tfc; gui_bcolour = tbc;
}


static void MX470Cursor(int x, int y) {
		CurrentX = x;
		CurrentY = y;
}

static void MX470Display(int fn) {
	int maxW = PageTable[WritePage].xmax;
	int maxH = PageTable[WritePage].ymax;
	switch (fn) {
	case DISPLAY_CLS:       ClearScreen(gui_bcolour);
		break;
	case REVERSE_VIDEO:     
		std::swap(gui_bcolour, gui_fcolour);
		break;
	case CLEAR_TO_EOL:      DrawBox(CurrentX, CurrentY, maxW, CurrentY + gui_font_height, 0, 0, gui_bcolour);
		break;
	case CLEAR_TO_EOS:      DrawBox(CurrentX, CurrentY, maxW, CurrentY + gui_font_height, 0, 0, gui_bcolour);
		DrawRectangle(0, CurrentY + gui_font_height, maxW - 1, maxH - 1, gui_bcolour);
		break;
	case SCROLL_DOWN:
		break;
	case DRAW_LINE:         DrawBox(0, gui_font_height * (OptionHeight - 2), maxW - 1, maxH - 1, 0, 0, gui_bcolour);
		DrawLine(0, maxH - gui_font_height - 6, maxW - 1, maxH - gui_font_height - 6, 1, GUI_C_LINE);
		CurrentX = 0; CurrentY = maxH - gui_font_height;
		break;
	}
}
char* fstrstr(const char* s1, const char* s2);
static char* strcasechr(const char* p, int ch)
{
	char c;

	c = toupper(ch);
	for (;; ++p) {
		if (toupper(*p) == c)
			return ((char*)p);
		if (*p == '\0')
			return (NULL);
	}
	/* NOTREACHED */
}


void (*callback)(char*, int);

/** defines **/
#define KILO_VERSION "0.1b1"
#define KILO_TAB_STOP Option.Tab
#define KILO_QUIT_TIMES 3

#define CTRLKEY(a) (a & 0x1f)
#define BACKSPACE CTRLKEY('H')
extern volatile int ConsoleRxBufHead;
extern volatile int ConsoleRxBufTail;
void setterminal(void);
int StartEditLine = 0, StartEditCharacter = 0;
int newmarkmode = 0;
int BreakKeySave;
char* editCbuff = NULL;
int showdefault = 0;
int bufferpasted = 0;
extern volatile int ConsoleRxBufHead;
extern volatile int ConsoleRxBufTail;
extern volatile unsigned char ConsoleRxBuf[];

const char defaultpromptsmall[] = "ESC=Quit, F1=Save current, F2=Run, F6=Save, F7=Insert file, ^F=Find, ^S=Select, ^V=Paste, ^W=Backup";
const char defaultpromptbig[] = "ESC=Quit, F1=Save current, F2=Run, ^F=Find, ^S=Select";
const char defaultpromptverybig[] = "ESC=Quit, F1=Save current, F2=Run";
enum editorHighlight {
	HL_NORMAL = 0,
	HL_COMMENT,
	HL_MLCOMMENT,
	HL_KEYWORD1,
	HL_KEYWORD2,
	HL_STRING,
	HL_NUMBER,
	HL_MATCH,
	HL_MARK
};

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)
#define defaultprompt ((OptionWidth < 52) ? defaultpromptverybig : (OptionWidth < 100) ? defaultpromptbig : defaultpromptsmall)

/*** data ***/

struct editorSyntax {
	char* filetype;
	char** filematch;
	char** keywords;
	char* singleline_comment_start;
	char* rem_comment_start;
	int flags;
};

typedef struct erow {
	int idx;
	short size;
	short modified;
	short markstart;
	short markend;
	char* chars;
	char* selectbuff;
	unsigned char* hl;
} erow;

struct editorConfig {
	int cx, cy;
	int rowoff;
	int coloff;
	int screenrows;
	int screencols;
	int numrows;
	int insert;
	int cutpastebuffersize;
	erow* row;
	short dirty;
	short firsttime;
	int selstartx;
	int selstarty;
	char filename[STRINGSIZE];
	char statusmsg[106];
	struct editorSyntax* syntax;
	char* helpbuff;
	unsigned char lastkey;
};

struct editorConfig E;

/** filetypes **/

const char* C_HL_extensions[] = { ".bas", ".BAS", ".inc", ".INC",NULL };

const char* C_HL_keywords[] = { "Arc","AutoSave","Blit",
		"Box","Call","Case","Case Else","cat","Chdir","Circle",
		"Clear","Close","CLS","Colour","Console", "Const",
		"Continue","Copy","CSUB","CPU","Data",
		"Date","DefineFont","Dim","Do",
		"Edit","Else","Else If","ElseIf","End",
		"End DefineFont End Function","End If","End Select","End Sub",
		"EndIf","Erase","Error","Execute","Exit","Exit Do",
		"Exit For","Exit Function","Exit Sub","FFT","Files",
		"Font","For","Framebuffer","Function","GoSub","GoTo","gui",
		"If","Image","Inc",
		"Input","IReturn","Kill",
		"Let","Line","Line Input","List",
		"Load","Local","LongString","Loop","Math",
		"Memory","Mkdir","MMDEBUG","Mode","Rename",
		"New","NewEdit","Next","On",
		"Open","Option","Page","Pause",
		"Pixel","Play","Poke","Polygon",
		"Print","RBox",
		"Read","Rem","Restore","Return","Rmdir",
		"Run","Save","Seek","Select Case",
		"SetTick","Sort",
		"SPRITE","Static","Sub","TCP","TEMPR START","Text",
		"Time","Timer","Trace","Triangle",
		"TURTLE","VAR","WatchDog","While",
		"Abs","ACos","And",
		"As","Asc","ASin","Atan2","Atn",
		"Baudrate","Bin$","Bin2str$","Choice","Chr$","Cint",
		"Cos","Cwd$","Date$","DateTime$","Day$",
		"Deg","Dir$","Else","Eof",
		"Epoch","Eval","Exp","Field$","Fix",
		"For","Format$","GetIP$","GoSub","GoTo",
		"GPS","Hex$","Inkey$","Input$","Instr",
		"Int","inv","JSON$","KeyDown","LCase$","LCompare","Left$",
		"Len","LGetByte","LGetStr$","LInStr","LLen","Loc",
		"Lof","Log","math","Max","Mid$",
		"Min","mmdebug","MM.Device$","MM.ErrMsg$","MM.Errno",
		"MM.HRes","MM.CMDLINE$",
		"MM.Info","MM.Info$","MM.VRes",
		"MM.Watchdog","Mod","Mouse","Not","Oct$",
		"Or","Peek","Pi","Pixel",
		"Port","Pos","Rad","RGB",
		"Right$","Rnd","Rnd","Sgn","Sin",
		"Space$","sprite","Sqr",
		"Step","Str$","Str2bin","String$","Tab",
		"Tan","Then","Time$","Timer",
		"To","UCase$","Until","Val","While",
		"Xor",  "BASE", "EXPLICIT", "DEFAULT",
		"NONE", "AUTORUN", "BAUDRATE", "SELECT",
		"INTEGER", "FLOAT", "STRING", "OUTPUT", "APPEND", "WRITE",
		NULL };

struct editorSyntax HLDB[] = {
	{(char *)"bas", (char**)C_HL_extensions, (char**)C_HL_keywords, (char*)"'", (char*)"rem ",
	 HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS},
};
struct abuf {
	char* b;
	int len;
};

#define ABUF_INIT                                                              \
  { NULL, 0 }

struct abuf ab = ABUF_INIT;

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/** prototypes **/
void editorUpdateRow(erow* row);

void editorSetStatusMessage(const char* fmt, ...);
void editorRefreshScreen();
static char* editorPrompt(char* prompt, int offset);
void editorMoveCursor(int key);
/*** terminal ***/
int copytoclipboard(void)
{
//	LPWSTR cwdBuffer;

//	// Get the current working directory:
//	if ((cwdBuffer = _wgetcwd(NULL, 0)) == NULL)
//		return 1;

	DWORD len = E.cutpastebuffersize;
	HGLOBAL hdst;
	LPSTR dst;

	// Allocate string for cwd
	hdst = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (len + 1) * sizeof(CHAR));
	dst = (LPSTR)GlobalLock(hdst);
	memcpy(dst, editCbuff, len );
	dst[len] = 0;
	GlobalUnlock(hdst);

	// Set clipboard data
	if (!OpenClipboard(NULL)) return GetLastError();
	EmptyClipboard();
	if (!SetClipboardData(CF_TEXT, hdst)) return GetLastError();
	CloseClipboard();
	return 0;
}
void parseanddisplay(char* p, int len) {
	int i = 0, j;
	char seq[12];
	while (i < len) {
		while (p[i] != '\033') {
			DisplayPutC(p[i]);
			i++;
		}
		i++;
		if (p[i] == '[') { //escape sequence found
			memset(seq, 0, 12);
			j = 0;
			seq[j++] = p[i];
			do {
				i++;
				seq[j++] = p[i];
			} while (!(isalpha(p[i])));
			if (seq[strlen(seq) - 1] == 'H') {
				if (strlen(seq) == 2) {
					MX470Cursor(0, 0);
				}
				else {
					int rows, cols;
					sscanf(&seq[1], "%d;%d", &cols, &rows);
					MX470Cursor((rows - 1) * gui_font_width, (cols - 1) * gui_font_height);
				}
			}
			else if (strncmp(seq, "[K", 2) == 0) { // erase to end of line
				int i = E.screencols - (CurrentX / gui_font_width);
				while (i--)DisplayPutC(' ');
			}
			else if (strncmp(seq, "[?25l", 5) == 0)ShowMMBasicCursor(false);
			else if (strncmp(seq, "[?25h", 5) == 0)ShowMMBasicCursor(true);
			else if (strncmp(seq, "[36m", 4) == 0) { gui_fcolour = M_CYAN; gui_bcolour = M_BLACK; }
			else if (strncmp(seq, "[37m", 4) == 0) { gui_fcolour = M_WHITE; gui_bcolour = M_BLACK; }
			else if (strncmp(seq, "[32m", 4) == 0) { gui_fcolour = M_GREEN; gui_bcolour = M_BLACK; }
			else if (strncmp(seq, "[39m", 4) == 0) { gui_fcolour = M_WHITE; gui_bcolour = M_BLACK; }
			else if (strncmp(seq, "[35m", 4) == 0) { gui_fcolour = M_MAGENTA; gui_bcolour = M_BLACK; }
			else if (strncmp(seq, "[31m", 4) == 0) { gui_fcolour = M_RED; gui_bcolour = M_BLACK; }
			else if (strncmp(seq, "[34m", 4) == 0)gui_bcolour = M_BLUE;
			else if (strncmp(seq, "[33m", 4) == 0) { gui_fcolour = M_YELLOW; gui_bcolour = M_BLACK; }
			else if (strncmp(seq, "[m", 2) == 0) {
				gui_fcolour = M_WHITE;
				gui_bcolour = M_BLACK;
			}
			else if (strncmp(seq, "[7m", 3) == 0) {
//					int i = gui_fcolour;
//					gui_fcolour = gui_bcolour;
//					gui_bcolour = i;
					std::swap(gui_fcolour, gui_bcolour);
			}
			i++;
		}
	}
}

static int editorReadKey(int cursor, int prompt, int buflen) {
	int c;
	static int lastZ = 0, lastC = 0, lefthold = 0, lastCX, lastCY, lastR = 0;
	int maxW = PageTable[WritePage].xmax;
	do {
		if (callback == NULL && prompt)MX470Cursor(buflen * gui_font_width, (E.screenrows + 1) * gui_font_height - 2);
		if (cursor)ShowMMBasicCursor(true);
		c = getConsole(0);
			if (mouse_right && lastR == 0) {
				lastR = 1;
				if (mouse_xpos < gui_font_width) {
					E.lastkey = 0;
					return HOME;
				}
				else if (mouse_xpos > E.row[E.cy].size * gui_font_width || mouse_xpos > maxW - gui_font_width) {
					E.lastkey = 0;
					return END;
				}
			}
			if (!mouse_right)lastR = 0;
			if (mouse_wheel > 0) {
				mouse_wheel = 0;
				return UP;
			}
			if (mouse_wheel < 0) {
				mouse_wheel = 0;
				return DOWN;
			}
			
			if (mouse_left != lastZ) { //left button has changed
				lastZ = mouse_left;
				MouseTimer = 0;
				if (lastZ) { //new left press
					int line = mouse_ypos / gui_font_height;
					int charpos = mouse_xpos / gui_font_width;
					if (callback == cutpasteselectCallback) {
					int newline = E.rowoff + line;
						int newpos = E.coloff + charpos;
						if (newpos >= E.row[newline].size)newpos = E.row[newline].size;
						if (!(newline < E.selstarty || (newline == E.selstarty && newpos <= E.selstartx))) {
							if (newline > E.cy || (newline == E.cy && newpos > E.cx)) {
								while (!(E.cy == newline && E.cx == newpos)) {
									int lastcy = E.cy;
									editorMoveCursor(RIGHT);
									if (E.cy == lastcy) { //no change of line
										if (E.cx >= E.row[E.cy].markend)E.row[E.cy].markend = E.cx;
										else E.row[E.cy].markstart = E.cx;
									}
									else {
										lastCX = 0;
										lastCY++;
									}
									editorUpdateRow(&E.row[E.cy]);
								}
							}
							else if (newline < E.cy || (newline == E.cy && newpos < E.cx)) {
								while (!(E.cy == newline && E.cx == newpos)) {
									int lastcy = E.cy;
									editorMoveCursor(LEFT);
									if (E.cy == lastcy) { //no change of line
										if (E.cx < E.row[E.cy].markstart)E.row[E.cy].markstart = E.cx;
										else E.row[E.cy].markend = E.cx;
									}
									else { // is a new row
										if (E.row[E.cy].markend == -1)E.row[E.cy].markstart = E.cx;
										E.row[E.cy].markend = E.cx;
									}
									editorUpdateRow(&E.row[E.cy]);
								}
							}
						}
						ShowMMBasicCursor(false);
						editorRefreshScreen();
						ShowMMBasicCursor(true);
					}
					else {
						if (line <= E.screenrows) {
							if (mouse_xpos == 0 && E.coloff) {
								charpos--;
							}
							if (E.cy == E.rowoff + line && mouse_xpos == maxW - 1 && E.row[E.cy].size > E.coloff + E.screencols) {
								charpos++;
							}
							E.cy = E.rowoff + line;
							if (charpos <= E.row[E.cy].size)E.cx = E.coloff + charpos;
							else E.cx = E.row[E.cy].size;
							ShowMMBasicCursor(false);
							editorRefreshScreen();
							ShowMMBasicCursor(true);
						}
					}
				}
				else { //new release
					if (callback == cutpasteselectCallback && lefthold) {
						int lastcy;
						lefthold = 0;
						int line = mouse_ypos / gui_font_height;
						int charpos = mouse_xpos / gui_font_width;
						if (charpos > E.row[line + E.rowoff].size)charpos = E.row[line + E.rowoff].size - 1;
						if (line + E.rowoff < E.cy)return 0x1b;
						if (line + E.rowoff == E.cy && E.coloff + charpos <= E.cx)return 0x1b;
						while (lastCY < line + E.rowoff) {
							lastcy = E.cy;
							editorMoveCursor(RIGHT);
							if (E.cy == lastcy) { //no change of line
								if (E.cx >= E.row[E.cy].markend)E.row[E.cy].markend = E.cx;
								else E.row[E.cy].markstart = E.cx;
							}
							else {
								lastCX = 0;
								lastCY++;
							}
							editorUpdateRow(&E.row[E.cy]);
						}
						while (lastCX <= charpos + E.coloff && lastCX < E.row[E.cy].size) {
							editorMoveCursor(RIGHT);
							if (E.cx >= E.row[E.cy].markend)E.row[E.cy].markend = E.cx;
							else E.row[E.cy].markstart = E.cx;
							lastCX++;
						}
						editorUpdateRow(&E.row[E.cy]);
						ShowMMBasicCursor(false);
						editorRefreshScreen();
						ShowMMBasicCursor(true);
					}
				}
			}
			else if (mouse_left == 0)MouseTimer = 0;
			else if (MouseTimer > 500 && callback == NULL) {
				lefthold = 1;
				lastCX = E.cx;
				lastCY = E.cy;
				return F4;
			}
			if (mouse_middle != lastC) {
				lastC = mouse_middle;
				if (lastC) {
					int line = mouse_ypos / gui_font_height;
					if (callback == NULL) {
						if (line >= E.screenrows / 2) return PDOWN;
						else return PUP;
					}
					else return 0x1b;
				}
			}
	} while (c == -1);

	return c;
}
/** syntax highlighting **/

int is_separator(int c) {
	return isspace(c) || c == '\0' || strchr(":,.()+-/*=~%<>[];", c) != NULL;
}

void editorUpdateSyntax(erow* row) {
	row->hl = (unsigned char *)ReAllocMemory(row->hl, row->size + 8);
	memset(row->hl, HL_NORMAL, row->size + 8);
	if (E.syntax == NULL) {
		int i;
		if (row->markend > row->markstart) { //show mark if end >= start
			i = row->markstart;
			while (i < row->markend) {
				row->hl[i++] = HL_MATCH;
			}
		}
		return;
	}

	char** keywords = E.syntax->keywords;

	char* scs = E.syntax->singleline_comment_start;
	char* mcs = E.syntax->rem_comment_start;

	int scs_len = scs ? strlen(scs) : 0;
	int mcs_len = mcs ? strlen(mcs) : 0;

	int prev_sep = 1;
	int in_string = 0;
	int in_command = 0;
	int i = 0;
	while (i < row->size) {
		char c = row->chars[i];
		unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;
		if (scs_len && !in_string) {
			if (!strncmp(&row->chars[i], scs, scs_len)) {
				memset(&row->hl[i], HL_COMMENT, row->size - i);
				break;
			}
		}
		if (mcs_len && !in_string && !in_command) {
			if (!strncasecmp(&row->chars[i], mcs, mcs_len)) {
				memset(&row->hl[i], HL_COMMENT, row->size - i);
				break;
			}
			if (!strncasecmp(&row->chars[i], mcs, mcs_len - 1) && (row->size - i == 3)) {
				memset(&row->hl[i], HL_COMMENT, row->size - i);
				break;
			}
		}
		if (c != ' ')in_command = 1;
		if (c == ':' && !in_string)in_command = 0;

		if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
			if (in_string) {
				row->hl[i] = HL_STRING;
				if (c == in_string)
					in_string = 0;
				i++;
				prev_sep = 1;
				continue;
			}
			else {
				if (c == '"') {
					in_string = c;
					row->hl[i] = HL_STRING;
					i++;
					continue;
				}
			}
		}

		if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
			if ((IsDigitinline(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
				(c == '.' && prev_hl == HL_NUMBER)) {
				row->hl[i] = HL_NUMBER;
				i++;
				prev_sep = 0;
				continue;
			}
		}

		if (prev_sep) {
			int j;
			for (j = 0; keywords[j]; j++) {
				int klen = strlen(keywords[j]);
				int kw2 = (keywords[j][klen - 1] == '|');
				if (kw2)
					klen--;

				if (!strncasecmp(&row->chars[i], keywords[j], klen) &&
					is_separator(row->chars[i + klen])) {
					memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
					i += klen;
					break;
				}
			}
			if (keywords[j] != NULL) {
				prev_sep = 0;
				continue;
			}
		}

		prev_sep = is_separator(c);
		i++;
	}
	if (row->markend > row->markstart) { //show mark if end >= start
		i = row->markstart;
		while (i < row->markend) {
			row->hl[i++] = HL_MATCH;
		}
	}
}

int editorSyntaxToColor(int hl) {
	switch (hl) {
	case HL_COMMENT:
	case HL_MLCOMMENT:
		return 33;
	case HL_KEYWORD1:
		return 36;
	case HL_KEYWORD2:
		return 32;
	case HL_STRING:
		return 35;
	case HL_NUMBER:
		return 32;
	case HL_MATCH:
		return 34;
	default:
		return 37;
	}
}

void editorSelectSyntaxHighlight() {
	E.syntax = NULL;
	if (E.filename == NULL || Option.ColourCode ==0)
		return;

	char* ext = strrchr(E.filename, '.');

	for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
		struct editorSyntax* s = &HLDB[j];
		unsigned int i = 0;
		while (s->filematch[i]) {
			int is_ext = (s->filematch[i][0] == '.');
			if ((is_ext && ext && !strcasecmp(ext, s->filematch[i])) ||
				(!is_ext && fstrstr(E.filename, s->filematch[i]))) {
				E.syntax = s;

				int filerow;
				for (filerow = 0; filerow < E.numrows; filerow++) {
					editorUpdateSyntax(&E.row[filerow]);
				}
				return;
			}
			i++;
		}
	}
}

void editorUpdateRow(erow* row) {
	row->modified = 1;
	editorUpdateSyntax(row);
}

void editorInsertRow(int at, char* s, size_t len, int update) {
	int i;
	if (at < 0 || at > E.numrows)
		return;
	E.row = (erow *)ReAllocMemory(E.row, sizeof(erow) * (E.numrows + 1));

	memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
	for (int j = at + 1; j <= E.numrows; j++)
		E.row[j].idx++;

	E.row[at].idx = at;

	E.row[at].size = (short)len;
	E.row[at].chars = (char *)GetMemory(len + 1);
	memcpy(E.row[at].chars, s, len);
	E.row[at].chars[len] = '\0';

	E.row[at].hl = NULL;
	//  E.row[at].hl_open_comment = 0;
	E.row[at].markstart = 0;
	E.row[at].markend = -1;
	E.row[at].modified = 1;
	if (update)editorUpdateRow(&E.row[at]);
	for (i = at - 1; i <= E.numrows; i++)E.row[i].modified = 1;
	E.numrows++;
	E.dirty++;
}

void editorFreeRow(erow* row) {
	FreeMemorySafe((void**)&row->chars);
	FreeMemorySafe((void**)&row->hl);
}

void editorDelRow(int at) {
	int i;
	if (at < 0 || at >= E.numrows)
		return;
	editorFreeRow(&E.row[at]);
	memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
	for (int j = at; j < E.numrows - 1; j++)
		E.row[j].idx--;

	E.numrows--;
	E.dirty++;
	for (i = at - 1; i < E.numrows; i++)E.row[i].modified = 1;
}

void editorRowInsertChar(erow* row, int at, int c, int update) {
	if (row->size >= 240) {
		editorSetStatusMessage("Line too long\033[K");
		showdefault = 2;
		E.cx--;
		return; //maximum size
	}
	if (at < 0 || at > row->size)
		at = row->size;
	row->chars = (char *)ReAllocMemory(row->chars, row->size + 2);
	memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
	row->size++;
	row->chars[at] = c;
	row->modified = 1;
	if (update)editorUpdateRow(row);
	E.dirty++;
}

void editorRowAppendString(erow* row, char* s, size_t len) {
	row->chars = (char *)ReAllocMemory(row->chars, row->size + len + 1);
	memcpy(&row->chars[row->size], s, len);
	row->size += (short)len;
	row->chars[row->size] = '\0';
	editorUpdateRow(row);
	E.dirty++;
}

void editorRowDelChar(erow* row, int at) {
	if (at < 0 || at >= row->size)
		return;
	memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
	row->size--;
	editorUpdateRow(row);
	E.dirty++;
}

/** editor operations **/

void editorInsertChar(int c, int update) {
	if (E.cy == E.numrows) {
		editorInsertRow(E.numrows, (char *)"", 0, update);
	}
	if (E.insert || E.row[E.cy].size == E.cx)editorRowInsertChar(&E.row[E.cy], E.cx, c, update);
	else {
		E.row[E.cy].chars[E.cx] = c;
		editorUpdateRow(&E.row[E.cy]);
	}
	E.cx++;
}

void editorInsertNewLine(int update) {
	if (E.cx == 0) {
		editorInsertRow(E.cy, (char*)"", 0, update);
	}
	else {
		erow* row = &E.row[E.cy];
		editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx, update);
		row = &E.row[E.cy];
		row->size = E.cx;
		row->modified = 1;
		row->chars[row->size] = '\0';
		if (update)editorUpdateRow(row);
	}
	E.cy++;
	E.cx = 0;
}

void editorDelChar() {
	if (E.cy == E.numrows)
		return;

	if (E.cx == 0 && E.cy == 0)
		return;

	erow* row = &E.row[E.cy];
	if (E.cx > 0) {
		editorRowDelChar(row, E.cx - 1);
		E.cx--;
	}
	else {
		if (E.row[E.cy - 1].size + row->size > 240) {
			editorSetStatusMessage("Line too long\033[K");
			showdefault = 2;
			return; //maximum size
		}

		E.cx = E.row[E.cy - 1].size;
		editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
		editorDelRow(E.cy);
		E.cy--;
	}
}

void editorOpen(char* filename) {
	int fnbr;
	//	if(strchr(filename, '.') == NULL) strcat(filename, ".BAS");
	FreeMemorySafe((void**)&E.filename);
	strcpy(E.filename, filename);

	editorSelectSyntaxHighlight();

	fnbr = FindFreeFileNbr();
	if (!BasicFileOpen(E.filename, fnbr, (char*)"rb")) return;

	char line[256];
	size_t linelen;
	while (!MMfeof(fnbr)) {
		memset(line, 0, 256);
		MMgetline(fnbr, (char*)line);
		linelen = strlen(line);
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
		editorInsertRow(E.numrows, line, linelen, 1);
		editorUpdateRow(&E.row[E.numrows - 1]);
	}
	FileClose(fnbr);
	E.dirty = 0;
}
void BufferSave(int c) {
	int fnbr;
	fnbr = FindFreeFileNbr();
	if (!BasicFileOpen(E.filename, fnbr, (char*)"wb")) return;
	FilePutStr(E.cutpastebuffersize, editCbuff, fnbr);
	FileClose(fnbr);
	editorSetStatusMessage("%d bytes written to disk as %s\033[K", E.cutpastebuffersize, E.filename);
}

void editorSave(int c) {
	int fnbr;
	StartEditCharacter = E.cx;
	StartEditLine = E.cy;
	char tempfile[STRINGSIZE];
	char crlf[3] = "\r\n";
	int j, len = 0;
	fnbr = FindFreeFileNbr();
	strcpy(tempfile, E.filename);
	strcat(tempfile, ".bak");
	remove(tempfile);
	rename(E.filename, tempfile);
	if(!BasicFileOpen(E.filename, fnbr, (char*)"wb")) return;
	for (j = 0; j < E.numrows; j++) {
		FilePutStr(E.row[j].size, E.row[j].chars, fnbr);
		FilePutStr(2, crlf, fnbr);
		len += (E.row[j].size + 2);
	}
	FileClose(fnbr);
	if (c)E.dirty = 0;
	editorSetStatusMessage("%d bytes written to disk as %s\033[K", len, E.filename);

	if (c) {
		char* q = &E.filename[strlen(E.filename) - 4];
		if (strcasecmp(q, ".bas") != 0)return;
		FileLoadProgram((unsigned char *)E.filename,1);
	}
}
void editorSaveAs(int c) {
	char p[STRINGSIZE];
	strcpy(p, E.filename);
	strcpy(E.filename, editorPrompt((char *)"Backup as: %s  (ESC to cancel)\033[K", 11));
	if (strlen(E.filename) == 0) {
		editorSetStatusMessage("Backup aborted\033[K");
		strcpy(E.filename, p);
		showdefault = 2;
		return;
	}
	editorSave(0);
	strcpy(E.filename, p);
	showdefault = 2;
	clearrepeat();
}
void BufferCheck(void) {
	editorSetStatusMessage("Warning unpasted buffer\033[K");
	bufferpasted = 1;
	showdefault = 2;
}
void BufferSaveAs(int c) {
	char p[STRINGSIZE];
	char q[STRINGSIZE];
	strcpy(p, E.filename);
	strcpy(E.filename, editorPrompt((char*)"Backup as: %s  (ESC to cancel)\033[K", 11));
	if (strlen(E.filename) == 0) {
		editorSetStatusMessage("Backup aborted\033[K");
		strcpy(E.filename, p);
		showdefault = 2;
		return;
	}
	fullfilename(E.filename, q, NULL);
	strcpy(E.filename, q);
	BufferSave(0);
	strcpy(E.filename, p);
	showdefault = 2;
	clearrepeat();
}
void editorInsert(int q) {
	char p[STRINGSIZE];
	char qq[STRINGSIZE];
	int fnbr;
	int fr = 0;
	strcpy(p, E.filename);
	strcpy(E.filename, editorPrompt((char*)"Insert file as: %s  (ESC to cancel)\033[K", 16));
	if (strlen(E.filename) == 0) {
		editorSetStatusMessage("Insert aborted\033[K");
		strcpy(E.filename, p);
		showdefault = 2;
		return;
	}
	fullfilename(E.filename, qq, NULL);
	strcpy(E.filename, qq);
	fr = existsfile(E.filename);
	if (!fr) { //file doesn't exist
		editorSetStatusMessage("File not found\033[K");
		strcpy(E.filename, p);
		showdefault = 2;
		return;
	}
	fnbr = FindFreeFileNbr();
	if (!BasicFileOpen(E.filename, fnbr, (char *)"rb")) {
		strcpy(E.filename, p);
		editorSetStatusMessage("File read error\033[K");
		showdefault = 2;
		return;
	}

	int c, scy = E.cy;
	while (!MMfeof(fnbr)) {
		c = FileGetChar(fnbr);
		if (c == '\n') {
			editorInsertNewLine(0);
		}
		else if (c == '\r') {}
		else editorInsertChar(c, 0);
	}
	int i;
	for (i = scy; i <= E.cy; i++)editorUpdateRow(&E.row[i]);
	FileClose(fnbr);
	E.dirty++;
	strcpy(E.filename, p);
	showdefault = 1;
}

void cutpasteselectCallback(char* cutbuf, int key) {
	int lastcy = E.cy, lastcx = E.cx, buffsize = 0,cerror=0;
	switch (key) {
	case CTRLKEY('X'): //cut command
	case CTRLKEY('C'): //copy command
	case F4:
	case F5:
	{
		int i = 0, j;
		while (E.row[i].markend == -1 && E.row[i].markstart == 0 && i < E.numrows)i++; //get to the first row with something to delete
		if (i == E.numrows || (E.cx == E.selstartx && E.cy == E.selstarty)) {
			E.cx = E.selstartx;
			E.cy = E.selstarty;
			callback = NULL;
			editorSetStatusMessage((key == F5 || key == CTRLKEY('C')) ? "Nothing to copy\033[K" : "Nothing to delete\033[K");
			showdefault = 2;
			return;
		}
		editCbuff = (char*)ReAllocMemory(editCbuff, E.row[i].markend - E.row[i].markstart + 2);

		//Deal with the first line which may be incomplete
		for (j = E.row[i].markstart; j < E.row[i].markend; j++)editCbuff[buffsize++] = E.row[i].chars[j];
		if (E.row[i].markend == E.row[i].size) {
			editCbuff[buffsize++] = '\r';
			editCbuff[buffsize++] = '\n';
		}
		i++; //In this case we can step to the next line

		while (i < E.numrows && E.row[i].markend == E.row[i].size && E.row[i].markstart == 0) {
			editCbuff = (char*)ReAllocMemory(editCbuff, E.row[i].markend - E.row[i].markstart + 2 + buffsize);
			for (j = E.row[i].markstart; j < E.row[i].markend; j++)editCbuff[buffsize++] = E.row[i].chars[j];
			editCbuff[buffsize++] = '\r';
			editCbuff[buffsize++] = '\n';
			i++;
		}

		if ((E.row[i].markend != E.row[i].size || E.row[i].markstart != 0) && E.row[i].markend != -1 && i < E.numrows) { //Deal with the last line which may be incomplete
			editCbuff = (char*)ReAllocMemory(editCbuff, E.row[i].markend - E.row[i].markstart + buffsize);
			for (j = E.row[i].markstart; j < E.row[i].markend; j++)editCbuff[buffsize++] = E.row[i].chars[j];
		}
	}
	editCbuff = (char*)ReAllocMemory(editCbuff, 10 + buffsize);
	E.cutpastebuffersize = buffsize;
	if ((cerror = copytoclipboard()))error((char*)"Clipboard %", cerror);
	if (E.cutpastebuffersize != buffsize)E.cutpastebuffersize = 999999;
	FreeMemory((unsigned char*)editCbuff);
	editCbuff = NULL;
	case DEL:
		if (!(key == CTRLKEY('C') || key == F5)) {
			if (key == DEL) {
				FreeMemorySafe((void**)&editCbuff); //Don't need the buffer
				E.cutpastebuffersize = 0;
			}
			int i = 0, j, firstline = -1;
			while (E.row[i].markend == -1 && E.row[i].markstart == 0 && i < E.numrows)i++; //get to the first row with something to delete
			if (i == E.numrows || (E.cx == E.selstartx && E.cy == E.selstarty)) {
				E.cx = E.selstartx;
				E.cy = E.selstarty;
				callback = NULL;
				editorSetStatusMessage("Nothing to delete\033[K");
				showdefault = 2;
				return;
			}
			lastcx = E.row[i].markstart;
			lastcy = i;
			for (j = E.row[i].markstart; j < E.row[i].markend; j++)editorRowDelChar(&E.row[i], E.row[i].markstart);
			editorUpdateRow(&E.row[i]);
			firstline = i;
			i++; //In this case we can step to the next line
			while (i < E.numrows && E.row[i].markend == E.row[i].size && E.row[i].markstart == 0) {
				editorDelRow(i); //delete all complete lines
			}

			if ((E.row[i].markend != E.row[i].size || E.row[i].markstart != 0) && E.row[i].markend != -1 && i < E.numrows) { //Deal with the last line which may be incomplete
				for (j = E.row[i].markstart; j < E.row[i].markend; j++)editorRowDelChar(&E.row[i], E.row[i].markstart);
				editorUpdateRow(&E.row[i]);
				if (firstline != -1) { //if the firstline wasn't complete then we need to concatenate this on with it
					E.cx = 0;
					E.cy = firstline + 1;
					editorDelChar();
				}
			}
		}
		int i;
		for (i = 0; i < E.numrows; i++) { //clear the select markers on all (remaining) lines
			if (E.row[i].markend != -1 || E.row[i].markstart != 0) {
				E.row[i].markend = -1;
				E.row[i].markstart = 0;
				editorUpdateRow(&E.row[i]);
			}
		}
		E.cx = lastcx;
		E.cy = lastcy;
		callback = NULL;
		editorSetStatusMessage(defaultprompt);
		editorRefreshScreen();
		return;

	case 0x1b: //exit condition
	{
		int i;
		for (i = 0; i < E.numrows; i++) { //clear the select markers on all (remaining) lines
			if (E.row[i].markend != -1 || E.row[i].markstart != 0) {
				E.row[i].markend = -1;
				E.row[i].markstart = 0;
				editorUpdateRow(&E.row[i]);
			}
		}
		callback = NULL;
		editorSetStatusMessage(defaultprompt);
		return;
	}
	case RIGHTSEL:
	case RIGHT: //the easy one
		editorMoveCursor(RIGHT);
		if (E.cy == lastcy) { //no change of line
			if (E.cx >= E.row[E.cy].markend)E.row[E.cy].markend = E.cx;
			else E.row[E.cy].markstart = E.cx;
		}
		editorUpdateRow(&E.row[E.cy]);
		break;
	case LEFT:
		if (E.cy == E.selstarty && E.cx == E.selstartx)break;
		editorMoveCursor(key);
		if (E.cy == lastcy) { //no change of line
			if (E.cx == lastcx)return; //special case beginning of files
			if (E.cx < E.row[E.cy].markstart)E.row[E.cy].markstart = E.cx;
			else E.row[E.cy].markend = E.cx;
		}
		else { // is a new row
			if (E.row[E.cy].markend == -1)E.row[E.cy].markstart = E.cx;
			E.row[E.cy].markend = E.cx;
		}
		editorUpdateRow(&E.row[E.cy]);
		break;
	case END:
		while (E.cx < E.row[E.cy].size) {
			editorMoveCursor(RIGHT);
			if (E.cx > E.row[E.cy].markend)E.row[E.cy].markend = E.cx;
			else E.row[E.cy].markstart = E.cx;
		}
		editorUpdateRow(&E.row[lastcy]);
		break;
	case HOME:
		if (E.cy == E.selstarty && E.cx == E.selstartx)break;
		while (E.cx >= 1 && (E.cy != E.selstarty || E.cx > E.selstartx)) {
			int tempcx = E.cx;
			editorMoveCursor(LEFT);
			if (E.cy == lastcy) {
				if (E.cx == tempcx)return; //special case beginning of files
				if (E.cx < E.row[E.cy].markstart)E.row[E.cy].markstart = E.cx;
				else E.row[E.cy].markend = E.cx;
			}
		}
		editorUpdateRow(&E.row[lastcy]);
		break;
	case UP:
		if (E.cy == E.selstarty || (E.cy == E.selstarty + 1 && E.cx < E.selstartx))break;
		while (E.cy == lastcy) {
			int tempcx = E.cx;
			editorMoveCursor(LEFT);
			if (E.cy == lastcy) {
				if (E.cx == tempcx)return; //special case beginning of files
				if (E.cx < E.row[E.cy].markstart)E.row[E.cy].markstart = E.cx;
				else E.row[E.cy].markend = E.cx;
			}
		}
		if (E.row[E.cy].markend == -1)E.row[E.cy].markstart = E.cx;
		E.row[E.cy].markend = E.cx;
		while (E.cx > lastcx && E.cx >= 1) {
			int tempcx = E.cx;
			editorMoveCursor(LEFT);
			if (E.cx == tempcx)return; //special case beginning of files
			if (E.cx < E.row[E.cy].markstart)E.row[E.cy].markstart = E.cx;
			else E.row[E.cy].markend = E.cx;
		}
		editorUpdateRow(&E.row[lastcy]);
		editorUpdateRow(&E.row[E.cy]);
		break;
	case DOWNSEL:
	case DOWN:
		while (E.cy == lastcy) {
			editorMoveCursor(RIGHT);
			if (E.cx > E.row[E.cy].markend)E.row[E.cy].markend = E.cx;
			else if (E.cy == lastcy && E.cx == E.row[E.cy].markend)break;
			else E.row[E.cy].markstart = E.cx;
		}
		if (E.cy != lastcy) {
			while (E.cx < lastcx && E.cx < E.row[E.cy].size) {
				editorMoveCursor(RIGHT);
				if (E.cx > E.row[E.cy].markend)E.row[E.cy].markend = E.cx;
				else E.row[E.cy].markstart = E.cx;
			}
		}
		editorUpdateRow(&E.row[lastcy]);
		editorUpdateRow(&E.row[E.cy]);
		break;
		return;
	}
}
/** find **/
static char* fstrstr(const char* s1, const char* s2)
{
	const char* p = s1;
	const size_t len = strlen(s2);

	for (; (p = strcasechr(p, *s2)) != 0; p++)
	{
		if (strncasecmp(p, s2, len) == 0)
			return (char*)p;
	}
	return (0);
}
char* bstrstr(const char* s1, const char* s3, const char* s2)
{
	const char* p = s1 - strlen(s2);
	const size_t len = strlen(s2);
	while (p-- >= s3)
	{
		if (strncasecmp(p, s2, len) == 0)
			return (char*)p;
	}
	return (0);
}
void editorFindCallback(char* query, int key) {
	static int last_match = -1;
	static int direction = 1;
	static char* last_match_pos = NULL;
	static char* last_match_actual = NULL;
	static int saved_hl_line = -1;
	//  static char *saved_hl = NULL;
	char* match;
	int current = 0;
	if (saved_hl_line != -1) {
		E.row[saved_hl_line].markstart = 0;
		E.row[saved_hl_line].markend = -1;
		editorUpdateRow(&E.row[saved_hl_line]);
	}
	if (key == '\n' || key == 0x1b) {
		last_match = -1;
		direction = 1;
		callback = NULL;
		editorSetStatusMessage(defaultprompt);
		int i;
		for (i = 0; i < E.numrows; i++)E.row[i].modified = 1;
		return;
	}
	else if (key == RIGHT || key == DOWN) {
		if (direction == -1)last_match_pos = last_match_pos + strlen(query) - 1;
		direction = 1;
	}
	else if (key == CTRLKEY('V') || key == F5) {
		if (editCbuff == NULL)return;
		char* p;
		p = editCbuff;
		if (last_match != -1) {
			int i, k = (int)((uint32_t)last_match_actual - (uint32_t)E.row[last_match].chars);
			for (i = 0; i < (int)strlen(query); i++) {
				editorRowDelChar(&E.row[last_match], k);
			}
			while (*p) {
				if (*p == '\n') {
					editorInsertNewLine(1);
					p++;
				}
				else if (*p == '\r')p++;
				else editorInsertChar(*p++, 1);
			}
		}
		return;
	}
	else if (key == LEFT || key == UP) {
		direction = -1;
	}
	else {
		last_match = -1;
		direction = 1;
	}

	if (last_match == -1) {
		direction = 1;
		current = E.cy;
		last_match_pos = E.row[current].chars + E.cx;
	}
	else {
		current = last_match;
	}
	int i;
	for (i = 0; i < E.numrows; i++) {
		erow* row = &E.row[current];
		if (direction == 1)match = fstrstr(last_match_pos, query);
		else match = bstrstr(last_match_pos, E.row[current].chars, query);
		if (match) {
			last_match_actual = match;
			int k = (int)((uint32_t)last_match_actual - (uint32_t)E.row[current].chars);
			E.row[current].markstart = k;
			E.row[current].markend = k + (short)strlen(query);
			editorUpdateRow(&E.row[current]);
			if (direction == 1)last_match_pos = match + strlen(query);
			else last_match_pos = match + 1;
			last_match = current;
			saved_hl_line = current;
			E.cy = current;
			E.cx = match - row->chars;
			break;
		}
		else {
			last_match_actual = NULL;
			current += direction;
			if (current == -1)
				current = E.numrows - 1;
			else if (current == E.numrows)
				current = 0;
			if (direction == 1)last_match_pos = E.row[current].chars;
			else last_match_pos = E.row[current].chars + E.row[current].size + 1;
		}
		clearrepeat();
	}
}
void cutpasteselect() {
	int saved_cx = E.cx;
	int saved_cy = E.cy;
	int i;
	for (i = 0; i < E.numrows; i++) {
		E.row[i].markend = -1;
		E.row[i].markstart = 0;
	}
	FreeMemorySafe((void**)&editCbuff);
	editCbuff = (char*)GetMemory(CLIPBOARDSIZE);
	E.row[saved_cy].markend = E.row[saved_cy].markstart = saved_cx;
	callback = cutpasteselectCallback;
	char* cutbuf = editorPrompt((char*)"Ctrl-X to Cut, Ctrl-C to Copy, DEL to delete, ESC to exit\033[K", 0);
	if (cutbuf) {
		FreeMemorySafe((void**)&cutbuf);
	}
	else {
		E.cx = saved_cx;
		E.cy = saved_cy;
	}
}
void editorFind(void) {
	int saved_cx = E.cx;
	int saved_cy = E.cy;
	int saved_coloff = E.coloff;
	int saved_rowoff = E.rowoff;
	callback = editorFindCallback;
	char* query = editorPrompt((char*)"Search: %s (Use ESC/Arrows/Enter/Ctrl-V)\033[K", 0);
	if (query) {
		FreeMemorySafe((void**)&query);
	}
	else {
		E.cx = saved_cx;
		E.cy = saved_cy;
		E.coloff = saved_coloff;
		E.rowoff = saved_rowoff;
	}
}

/*** append buffer ***/


void abAppend(struct abuf* ab, const char* s, int len) {
	//  char *new = ReAllocMemory(ab->b, ab->len + len);
	/*
	  if (new == NULL)
		return;*/
	memcpy(&ab->b[ab->len], s, len);
	//  ab->b = new;
	ab->len += len;
}

/*void abfree(struct abuf *ab) {
//	FreeMemorySafe((void *)&ab->b);
}*/

/*** output ***/

void editorScroll() {
	int i = 0, oldrow = E.rowoff, oldcol = E.coloff;
	//  E.rx = 0;
	if (E.cy >= E.numrows) {
		E.cx = 0;
	}

	if (E.cy < E.rowoff) {
		E.rowoff = E.cy;
	}
	if (E.cy >= E.rowoff + E.screenrows) {
		E.rowoff = E.cy - E.screenrows + 1;
	}
	if (E.cx < E.coloff) {
		E.coloff = E.cx;
	}
	if (E.cx >= E.coloff + E.screencols) {
		E.coloff = E.cx - E.screencols + 1;
	}
	if (oldrow != E.rowoff || oldcol != E.coloff) {
		for (i = 0; i < E.numrows; i++)E.row[i].modified = 1;
		while (getConsole(0) != -1);
	}
}

void editorDrawRows(struct abuf* ab) {
	int y;
	for (y = 0; y < E.screenrows; y++) {
		if (y > E.numrows)abAppend(ab, "\033[K\r\n", 5);
		else {
			int filerow = y + E.rowoff;
			if (E.row[filerow].modified == 0 && filerow < E.numrows) {
				abAppend(ab, "\r\n", 2);
				continue;
			}
			E.row[filerow].modified = 0;
			if (filerow >= E.numrows) {
				abAppend(ab, "\033[K\r\n", 5);
			}
			else {
				int len = E.row[filerow].size - E.coloff;
				if (len < 0)
					len = 0;
				if (len > E.screencols)
					len = E.screencols;

				char* c = &E.row[filerow].chars[E.coloff];
				unsigned char* hl = &E.row[filerow].hl[E.coloff];
				int current_color = -1;
				int j;
				for (j = 0; j < len; j++) {
					if (iscntrl(c[j])) {
						char sym = (c[j] <= 26) ? '@' + c[j] : '?';
						abAppend(ab, "\033[7m", 4);
						abAppend(ab, &sym, 1);
						abAppend(ab, "\033[m", 3);
						if (current_color != -1) {
							char buf[16];
							int clen = snprintf(buf, sizeof(buf), "\033[%dm", current_color);
							abAppend(ab, buf, clen);
						}
					}
					else if (hl[j] == HL_NORMAL) {
						if (current_color != -1) {
							abAppend(ab, "\033[39m", 5);
							current_color = -1;
						}
						abAppend(ab, &c[j], 1);
					}
					else {
						int color = editorSyntaxToColor(hl[j]);
						if (color != current_color) {
							current_color = color;
							char buf[16];
							int clen = snprintf(buf, sizeof(buf), "\033[%dm", color);
							abAppend(ab, buf, clen);
						}
						abAppend(ab, &c[j], 1);
					}
				}
				abAppend(ab, "\033[39m", 5);
				abAppend(ab, "\033[K", 3);
				abAppend(ab, "\r\n", 2);
			}
		}
	}
}

void editorDrawStatusBar(struct abuf* ab) {
	char buf[32];
	char pb[10];
	IntToStr(pb, E.cutpastebuffersize, 10);
	snprintf(buf, sizeof(buf), "\033[%d;%dH", E.screenrows + 1, 1);
	abAppend(ab, buf, strlen(buf));
	abAppend(ab, "\033[7m", 4);
	char status[80], rstatus[80];
	int len = snprintf(status, sizeof(status), "%.60s - %d lines %s %s %s %s",
		E.filename ? E.filename : "[No Name", E.numrows,
		E.dirty ? "(modified)" : "", (E.cutpastebuffersize ? ", Paste Buffer" : ""), (E.cutpastebuffersize ? pb : ""), (E.cutpastebuffersize ? "chars" : ""));
	int rlen =
		snprintf(rstatus, sizeof(rstatus), "%s,%s | %d/%d/%d",
			E.insert ? "INS" : "OVR", E.syntax ? E.syntax->filetype : "no ft", E.cx + 1, E.cy + 1, E.numrows);

	if (len > E.screencols)
		len = E.screencols;
	if (status[0] == '0' && status[1] == ':')status[0] = 'A';
	abAppend(ab, status, len);
	while (len < E.screencols) {
		if (E.screencols - len == rlen) {
			abAppend(ab, rstatus, rlen);
			break;
		}
		else {
			abAppend(ab, " ", 1);
			len++;
		}
	}
	abAppend(ab, "\033[m", 3);
}

void editorDrawMessageBar(struct abuf* ab) {
	abAppend(ab, "\033[K", 3);
	int msglen = strlen(E.statusmsg);
	//  if (msglen > E.screencols) msglen = E.screencols;
	char buf[32] = { 0 };
	snprintf(buf, sizeof(buf), "\033[%d;%dH", E.screenrows + 2, 1);
	abAppend(ab, buf, strlen(buf));
	abAppend(ab, E.statusmsg, msglen);
}

void editorRefreshScreen(void) {
	//	  abfree(&ab);
	memset(ab.b, 0, OptionHeight * OptionWidth * 8);
	editorScroll();
	ab.len = 0;
	abAppend(&ab, "\033[?25l", 6);
	abAppend(&ab, "\033[H", 3);

	editorDrawRows(&ab);
	if (!E.firsttime) {
		editorDrawStatusBar(&ab);
		editorDrawMessageBar(&ab);
	}
	char buf[32];
	snprintf(buf, sizeof(buf), "\033[%d;%dH", (E.cy - E.rowoff) + 1,
		(E.cx - E.coloff) + 1);
	abAppend(&ab, buf, strlen(buf));

	abAppend(&ab, "\033[?25h", 6);
	parseanddisplay(ab.b, ab.len);
}

void editorSetStatusMessage(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
	va_end(ap);
}

/** input **/

static char* editorPrompt(char* prompt, int offset) {
	size_t bufsize = 128;
	char* buf = (char*)GetMemory(bufsize);

	size_t buflen = 0;
	buf[0] = '\0';

	while (1) {
		editorSetStatusMessage(prompt, buf);
		editorRefreshScreen();
		if (callback == NULL) {
			ShowMMBasicCursor(false);
		}
		int c = editorReadKey((callback == NULL ? 1 : 1), (callback == NULL ? 1 : 0), buflen + offset);
		if (callback == editorFindCallback || callback == NULL) {
			if (c == DEL || c == CTRLKEY('H') || c == BACKSPACE) {
				if (buflen != 0)
					buf[--buflen] = '\0';
			}
			else if (c == 0x1b) {
				editorSetStatusMessage("");
				if (callback)
					callback(buf, c);
				FreeMemorySafe((void**)&buf);
				return NULL;
			}
			else if (c == '\n') {
				if (buflen != 0) {
					editorSetStatusMessage("");
					if (callback)
						callback(buf, c);
					return buf;
				}
			}
			else if (!iscntrl(c) && c < 128) {
				if (buflen == bufsize - 1) {
					bufsize *= 2;
					buf = (char*)ReAllocMemory(buf, bufsize);
				}
				buf[buflen++] = c;
				buf[buflen] = '\0';
			}

			if (callback)
				callback(buf, c);
		}
		else { //cutpasteselect callback
			if (c == 0x1b || c == DEL || c == CTRLKEY('C') || c == CTRLKEY('X') || c == F4 || c == F5) {
				editorSetStatusMessage("");
				if (callback)
					callback(buf, c);
				FreeMemorySafe((void**)&buf);
				return NULL;
			}
			if (callback)
				callback(buf, c);
		}
	}
}

void editorMoveCursor(int key) {
	erow* row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	switch (key) {
	case LEFT:
		if (E.cx != 0) {
			E.cx--;
		}
		else if (E.cy > 0) {
			E.cy--;
			E.cx = E.row[E.cy].size;
		}
		break;
	case RIGHT:
		if (row && E.cx < row->size) {
			E.cx++;
		}
		else if (row && E.cx == row->size && E.cy != E.numrows - 1) {
			E.cy++;
			E.cx = 0;
		}
		break;
	case UP:
		if (E.cy > 0) {
			E.cy--;
		}
		break;
	case DOWN:
		if (E.cy < E.numrows - 1) {
			E.cy++;
		}
		break;
	}

	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	int rowlen = row ? row->size : 0;
	if (E.cx > rowlen) {
		E.cx = rowlen;
	}
}
extern "C"  int WINAPI EditPaste(VOID)
{
	HGLOBAL   hglb;
	LPTSTR    lptstr;
	HWND hwnd=NULL;
	int size;
	{
		if (editCbuff) {
			FreeMemory((unsigned char *)editCbuff);
			editCbuff = NULL;
		}
		if (!IsClipboardFormatAvailable(CF_TEXT))
			return 0;
		if (!OpenClipboard(hwnd))
			return 0;

		hglb = GetClipboardData(CF_TEXT);
		if (hglb != NULL)
		{
			lptstr = (LPTSTR)GlobalLock(hglb);
			if (lptstr != NULL)
			{
				char* p = (char*)hglb;
				int lastp = 0;
				size = strlen(p);
				editCbuff = (char *)GetMemory(size);
				char* q = editCbuff;
				while (*p) {
					*q++ = *p++;
				}
				GlobalUnlock(hglb);
			}
		}
		CloseClipboard();

		return size;
	}
}

int editorProcessKeypress() {
	static int quit_times = KILO_QUIT_TIMES;
	int c = editorReadKey(1, 0, 0);

	switch (c) {
	case '\r':
		break;
	case '\n':
		editorInsertNewLine(1);
		break;
	case F1:	         // Save and exit
	case F2:		     // Save, exit and run
	{
/*******			if (Option.profile) {
				if (G1Hardware)memset((char*)0xD0300000, 0, 512 * 1024);
				else memset((char*)0xD0700000, 0, 512 * 1024);
			}*/
			editorSave(c);
			if (c == F2) {
/*				for (int i = 0; i <= E.numrows; i++)editorFreeRow(&E.row[i]);
				FreeMemorySafe((void**)&E.row);
				FreeMemorySafe((void**)&E.helpbuff);
				FreeMemorySafe((void**)&ab.b);
				FreeMemorySafe((void**)&editCbuff);*/
				gui_fcolour = PromptFC;
				gui_bcolour = PromptBC;
//				MMPrintString("\033[H");
				MX470Display(DISPLAY_CLS);                          // clear screen on the MX470 display only
				MX470Cursor(0, 0);
				BreakKey = BreakKeySave;
//				ClearVars(0);
				SetFont(Option.DefaultFont);
				setterminal();
//				reset_CLUT();
				memset(tknbuf, 0, STRINGSIZE);
				tknbuf[0] = GetCommandValue((unsigned char*)"RUN");
				strcat((char*)tknbuf, "\"");
				strcat((char*)tknbuf, lastfileedited);
				strcat((char*)tknbuf, "\"");
				longjmp(jmprun, 1);
				//******				cleanend();
			}

			E.lastkey = c;
			return 0;

			break;
		}
	case F6:		     // Save but don't load
		editorSave(0);
		return 0;
		break;
	case F5:
	case CTRLKEY('V'):
		bufferpasted = 1;
		E.cutpastebuffersize=EditPaste();
		if (editCbuff == NULL || E.cutpastebuffersize == 0)break;
		{
			int i = E.cutpastebuffersize;
			char* p;
			int scy = E.cy;
			p = editCbuff;
			while (i--) {
				if (*p == '\n') {
					editorInsertNewLine(0);
					p++;
				}
				else if (*p == '\r')p++;
				else editorInsertChar(*p++, 0);
			}
			for (i = scy; i <= E.cy; i++)editorUpdateRow(&E.row[i]);
		}
		break;
	case F7:
		editorInsert(c);
		break;
	case CTRLKEY('S'):
	case F4:
		if (editCbuff != NULL && bufferpasted == 0 && E.cutpastebuffersize != 0) {
			BufferCheck();
			break;
		}
		bufferpasted = 0;
		E.cutpastebuffersize = 0;
		E.selstartx = E.cx;
		E.selstarty = E.cy;
		cutpasteselect();
		break;
	case CTRLKEY('W'):
		editorSaveAs(c);
		break;
	case F11:
	{
		int i;
		for (i = 0; i < (int)strlen(E.helpbuff); i++)editorInsertChar(E.helpbuff[i], 0);
		editorUpdateRow(&E.row[E.cy]);
	}
	break;
	case F12:
	{
		/*		int i, j = 0;;
		strcpy((char*)inpbuf, "PAGE COPY 0 TO 1:BOX 0,0,MM.HRES,MM.INFO(FONTHEIGHT)*7,1,&HFF0000,0:PRINT @(0,2)");
		SerUSBPutS("\033[s");
		SerUSBPutS("\033[H");
		SerUSBPutS("\r\033[K");
		SerUSBPutS("\r\n\033[K");
		SerUSBPutS("\r\n\033[K");
		SerUSBPutS("\r\n\033[K");
		SerUSBPutS("\r\n\033[K");
		SerUSBPutS("\r\n\033[K");
		SerUSBPutS("\r\n\033[K");
		SerUSBPutS("\033[H");
		tokenise(true);                                             // turn into executable code
		ExecuteProgram(tknbuf);  // execute the line straight away
		memset(E.helpbuff, 0, STRINGSIZE);
		if (namestart[(int)E.row[E.cy].chars[E.cx]] || namein[(int)E.row[E.cy].chars[E.cx]]) {
			i = E.cx;
			while ((namein[(int)E.row[E.cy].chars[i]] || namestart[(int)E.row[E.cy].chars[i]]) && i) {
				i--;
			}
			if (!(namestart[(int)E.row[E.cy].chars[i]]))i++;
			while (namein[(int)E.row[E.cy].chars[i]] || namestart[(int)E.row[E.cy].chars[i]]) {
				E.helpbuff[j++] = E.row[E.cy].chars[i++];
			}
		}
		do_help(E.helpbuff);
		memset(inpbuf, 0, STRINGSIZE);
		strcpy((char*)inpbuf, "PAGE COPY 1 TO 0\r\n");
		tokenise(true);                                             // turn into executable code
		ExecuteProgram(tknbuf);                                     // execute the line straight away
		memset(inpbuf, 0, STRINGSIZE);
		for (i = E.rowoff; i < E.rowoff + 8; i++) if (i < E.numrows)E.row[i].modified = 1;
//		SerUSBPutS("\033[u");*/
	}

	break;
	case F8:
	case CTRLKEY('B'):
		if (editCbuff == NULL || E.cutpastebuffersize == 0)break;
		BufferSaveAs(c);
		break;
	case CTRLKEY('K'): //Delete to end of line
		if (E.cx == 0) editorDelRow(E.cy);
		else {
			int i = E.row[E.cy].size - E.cx;
			while (i--)editorRowDelChar(&E.row[E.cy], E.cx);
			E.cy++;
		}
		E.lastkey = CTRLKEY('K');
	case HOME:
		E.cx = 0;
		if (E.lastkey == HOME) {
			E.lastkey = 0;
			int times = E.rowoff + E.cy;
			while (times--) editorMoveCursor(UP);
		}
		break;

	case END:
		if (E.cy < E.numrows) E.cx = E.row[E.cy].size;
		if (E.lastkey == END) {
			E.lastkey = 0;
			int times = E.rowoff + E.cy;
			while (times--) editorMoveCursor(UP);
			times = E.numrows;
			while (times-- >= 1) editorMoveCursor(DOWN);
			E.cx = E.row[E.cy].size;
		}
		break;
	case INSERT:E.insert = !E.insert;
		break;
	case TAB:
	{
		char buf[9];
		int i = 0;
		strcpy(buf, "        ");
		switch (Option.Tab) {
		case 2:
			i = (2 - E.cx % 2); break;
		case 3:
			i = (3 - E.cx % 3); break;
		case 4:
			i = (4 - E.cx % 4); break;
		case 8:
			i = (8 - E.cx % 8); break;
		}
		while (i--) editorInsertChar(' ', 1);
	}
	break;

	case CTRLKEY('F'):
	case F3:
		editorFind();
		break;

	case SHIFT_TAB:
	{
		int i;
		for (i = 0; i < Option.Tab; i++) {
			if (E.row[E.cy].chars[E.cx] != ' ')break;
			editorMoveCursor(RIGHT);
			editorDelChar();
		}
	}
	break;
	case SHIFT_DEL:
	{
		if (E.cx == 0 && E.row[E.cy].chars[E.cx] == ' ') {
			while (E.row[E.cy].chars[E.cx] == ' ') {
				editorMoveCursor(RIGHT);
				editorDelChar();
			}
		}
		else {
			if (E.cy + 1 == E.numrows && E.cx == E.row[E.cy].size)break;
			editorMoveCursor(RIGHT);
			editorDelChar();
		}
	}
	break;

	case CTRLKEY('H'):
	case DEL:
		if (c == DEL) {
			if (E.cy + 1 == E.numrows && E.cx == E.row[E.cy].size)break;
			editorMoveCursor(RIGHT);
		}
		editorDelChar();
		break;

	case PUP:
	case PDOWN: {
		if (c == PUP) {
			if (E.cy != E.rowoff) {
				E.cy = E.rowoff;
				E.cx = 0;
			}
			else {
				int times = E.screenrows;
				while (times--) editorMoveCursor((c == PUP || c == CTRLKEY('P')) ? UP : DOWN);
				E.cx = 0;
			}
		}
		else if (c == PDOWN) {
			if (E.cy != E.rowoff + E.screenrows - 1) {
				E.cy = E.rowoff + E.screenrows - 1;
				if (E.cy > E.numrows)E.cy = E.numrows - 1;
				E.cx = 0;
			}
			else {
				E.cy = E.rowoff + E.screenrows - 1;
				if (E.cy > E.numrows)
					E.cy = E.numrows;

				int times = E.screenrows;
				while (times--) editorMoveCursor((c == PUP || c == CTRLKEY('P')) ? UP : DOWN);
				E.cx = 0;
			}
		}
	} break;
	case DOWNSEL:
	case RIGHTSEL:
		ConsoleRxBuf[ConsoleRxBufHead] = c & 0xDF;   // store the byte in the ring buffer
		ConsoleRxBufHead = (ConsoleRxBufHead + 1) % CONSOLE_RX_BUF_SIZE;     // advance the head of the queue
		E.cutpastebuffersize = 0;
		E.selstartx = E.cx;
		E.selstarty = E.cy;
		cutpasteselect();
		break;
	case UP:
	case DOWN:
	case LEFT:
	case RIGHT:
		editorMoveCursor(c);
		break;

	case 0x1b:
	{
		int c = getConsole(0);
		if (c == -1) {
			if (E.dirty && quit_times > 0) {
				editorSetStatusMessage("WARNING!!! Unsaved changes. "
					"Press ESC %d more times to quit.\033[K",
					quit_times - 1);
				quit_times--;
				return quit_times;
			}
//			MMPrintString("\0337\033[2J\033[H");									// vt100 clear screen and home cursor
			return 0;
		}
		else {
			while ((c = getConsole(0)) != -1) {}
		}
	} break;
	default:
		if (c >= 32 && c < 0x80)editorInsertChar(c, 1);
		break;
	}
	if (showdefault) {
		showdefault--;
		if (!showdefault)editorSetStatusMessage(defaultprompt);
	}
	E.lastkey = c;
	if (quit_times != KILO_QUIT_TIMES)	editorSetStatusMessage(defaultprompt);
	quit_times = KILO_QUIT_TIMES;
	return quit_times;
}

/*** init ***/
void setterminal(void) {
	char sp[50] = { 0 };
	strcpy(sp, "\033[8;");
	IntToStr(&sp[strlen(sp)], (long long int)OptionHeight, 10);
	strcat(sp, ";");
	IntToStr(&sp[strlen(sp)], (long long int)OptionWidth + 1, 10);
	strcat(sp, "t");
//	SerUSBPutS(sp);						//
}
void initEditor() {
	E.cx = 0;
	E.cy = 0;
	E.rowoff = 0;
	E.coloff = 0;
	E.numrows = 0;
	E.row = (erow *)GetMemory(1024 * 128);
	E.dirty = 0;
	memset(E.filename, 0, sizeof(E.filename));
	E.statusmsg[0] = '\0';
	E.syntax = NULL;
	E.firsttime = 1;
	E.insert = 1;
	E.cutpastebuffersize = 0;
	setterminal();
	E.screenrows = OptionHeight;
	E.screencols = OptionWidth;
	E.screenrows -= 2;
	E.lastkey = 0;
	ab.b = (char *)GetMemory(OptionHeight * OptionWidth * 8);
	E.helpbuff = (char*)GetMemory(STRINGSIZE);
}

void cmd_newedit(void) {
	char* c=NULL;
	char q[STRINGSIZE] = { 0 };
	BreakKeySave = BreakKey;
	BreakKey = 0;
//******	sendCRLF = 3;
	int fr = 0;
	if (CurrentLinePtr) error((char*)"Invalid in a program");
	MX470Display(DISPLAY_CLS);                          // clear screen on the MX470 display only
	MX470Cursor(0, 0);                                  // home the cursor
	SerialCloseAll();                             // same for serial ports
	ClearVars(0);
	closeallsprites();
	closeall3d();
	FreeMemorySafe((void**)&main_turtle_polyX);
	FreeMemorySafe((void**)&main_turtle_polyY);
	if (!(*cmdline == 0 || *cmdline == '\'')) {
		fullfilename((char*)getCstring(cmdline), q, NULL);
		if (strchr(q, '.') == NULL) {
			fr = existsfile(q);
			if (!fr)strcat(q, ".BAS");
		}
		StartEditLine = StartEditCharacter = 0;
	}
	else {
		if (!*lastfileedited)error((char*)"Nothing to edit");
		c = lastfileedited;
		strcpy(q, c);
	}
	while (getConsole(0) != -1) {}
	initEditor();
	fr = existsfile(q);
	if (!fr ) { //file doesn't exist
		int fnbr;
		char b = 0;
		FreeMemorySafe((void**)&E.filename);
		strcpy(E.filename, q);
		// Try and create the file to check the pathname/filename works then delete it
		fnbr = FindFreeFileNbr();
		BasicFileOpen(E.filename, fnbr, (char*)"wb");
		FileClose(fnbr);
		remove((char*)E.filename);
		memset(inpbuf, 0, STRINGSIZE);
		editorSelectSyntaxHighlight();
		StartEditLine = 0;
		StartEditCharacter = 0;
		editorInsertRow(E.numrows, &b, 0, 1);
		editorUpdateRow(&E.row[E.numrows - 1]);
		E.dirty = 1;
	}
	else editorOpen(q);
	
	editorSetStatusMessage(defaultprompt);
	if (E.numrows < StartEditLine) { //Shouldn't happen but.....
		StartEditLine = 0;
		StartEditCharacter = 0;
	}
	if (E.numrows > E.screenrows) {
		if (StartEditLine != 0) {
			int back = E.screenrows / 2;
			int n = StartEditLine + back;
			if (n > E.numrows) {
				n = E.numrows;
				back = E.numrows - StartEditLine - 1;
			}
			while (n--) {
				editorMoveCursor(DOWN);
			}
			editorRefreshScreen();
			while (back--) {
				editorMoveCursor(UP);
			}
		}
		if (StartEditCharacter != 0) {
			int n;
			n = StartEditCharacter;
			while (n--) {
				editorMoveCursor(RIGHT);
			}
		}
	}
	else {
		E.cy = StartEditLine;
		E.cx = StartEditCharacter;
	}
	E.firsttime = 0;
	editorRefreshScreen();
	clearrepeat();
	while (1) {
		if (editorProcessKeypress() == 0) {
			int i;
			if (E.lastkey != F1) {
				for (i = 0; i <= E.numrows; i++)editorFreeRow(&E.row[i]);
				FreeMemorySafe((void**)&E.row);
				FreeMemorySafe((void**)&E.helpbuff);
				FreeMemorySafe((void**)&ab.b);
				FreeMemorySafe((void**)&editCbuff);
			}
			gui_fcolour = PromptFC;
			gui_bcolour = PromptBC;
			MX470Display(DISPLAY_CLS);                          // clear screen on the MX470 display only
			MX470Cursor(0, 0);                                  // home the cursor
			BreakKey = BreakKeySave;
			memset(inpbuf, 0, STRINGSIZE);
			return;
		}
		editorRefreshScreen();
	}
}
