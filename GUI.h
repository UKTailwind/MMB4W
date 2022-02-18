/***********************************************************************************************************************
MMBasic for Windows

GUI.h

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


/**********************************************************************************
 the C language function associated with commands, functions or operators should be
 declared here
**********************************************************************************/
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)

void cmd_backlight(void);
void cmd_ctrlval(void);
void cmd_GUIpage(unsigned char * page);
void cmd_gui(void);

void fun_msgbox(void);
void fun_ctrlval(void);
void fun_mmhpos(void);
void fun_mmvpos(void);

#endif




/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE

//  { "BackLight",      T_CMD,                      0, cmd_backlight  },
{ (unsigned char *)"CtrlVal(", T_CMD | T_FUN, 0, cmd_ctrlval    },
{ (unsigned char*)"GUI",           T_CMD,                      0, cmd_gui },

#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE

{ (unsigned char*)"MsgBox(",        T_FUN | T_INT,              0, fun_msgbox },
{ (unsigned char*)"CtrlVal(",       T_FUN | T_NBR | T_STR,      0, fun_ctrlval },
//  { "MM.HPos",        T_FNA | T_INT,              0, fun_mmhpos     },
//  { "MM.VPos",        T_FNA | T_INT,              0, fun_mmvpos     },
//{ (unsigned char*)"Click(",       T_FUN | T_INT,        0, fun_touch },

#endif

#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
#ifndef GUI_H_INCL
#define GUI_H_INCL

extern void ProcessTouch(void);

extern void ResetGUI(void);
extern void DrawKeyboard(int);
extern void DrawFmtBox(int);

// define the blink rate for the cursor
#define CURSOR_OFF        350              // cursor off time in mS
#define CURSOR_ON     650                  // cursor on time in mS

#define MAX_CAPTION_LINES   10             // maximum number of lines in a caption

extern void HideAllControls(void);

extern volatile int TouchDown;
extern volatile int TouchUp;
extern volatile int TouchState;
extern int64_t gui_fcolour, gui_bcolour;
extern int64_t last_fcolour, last_bcolour;

extern int gui_click_pin;                  // the sound pin
extern int display_backlight;              // the brightness of the backlight (1 to 100)


extern int gui_int_down;                   // true if the touch down has triggered an interrupt
extern unsigned char* GuiIntDownVector;             // address of the interrupt routine or NULL if no interrupt
extern int gui_int_up;                     // true if the release of the touch has triggered an interrupt
extern unsigned char* GuiIntUpVector;               // address of the interrupt routine or NULL if no interrupt
extern volatile int DelayedDrawKeyboard;            // a flag to indicate that the pop-up keyboard should be drawn AFTER the pen down interrupt
extern volatile int DelayedDrawFmtBox;              // a flag to indicate that the pop-up formatted keyboard should be drawn AFTER the pen down interrupt

extern int CurrentRef;                     // if the pen is down this is the control (or zero if not on a control)
extern int LastRef;                        // this is the last control touched
extern int LastX;                          // this is the x coord when the pen was lifted
extern int LastY;                          // ditto for y

extern MMFLOAT CtrlSavedVal;               // a temporary place to save a control's value
extern volatile int TOUCH_DOWN;
extern int CheckGuiFlag;                   // used by Timer.c to tell if it has to call CheckGuiTimeouts()
extern void CheckGui(void);
extern void CheckGuiTimeouts(void);
extern volatile int ClickTimer;            // used to time the click when touch occurs
extern volatile int TouchTimer;                // used to time the response to touch


#endif
#endif
