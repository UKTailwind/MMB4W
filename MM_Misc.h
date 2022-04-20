/***********************************************************************************************************************
MMBasic for Windows

MM_Misc.h

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
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
// format:
//      void cmd_???(void)
//      void fun_???(void)
//      void op_???(void)

void cmd_autosave(void);
void cmd_option(void);
void cmd_pause(void);
void cmd_timer(void);
void cmd_date(void);
void cmd_time(void);
void cmd_ireturn(void);
void cmd_poke(void);
void cmd_settick(void);
void cmd_watchdog(void);
void cmd_cpu(void);
void cmd_cfunction(void);
void cmd_longString(void);
void cmd_sort(void);
void cmd_test(void);
void fun_timer(void);
void fun_date(void);
void fun_time(void);
void fun_device(void);
void fun_keydown(void);
void fun_peek(void);
void fun_restart(void);
void fun_day(void);
void fun_info(void);
void fun_LLen(void);
void fun_LGetByte(void);
void fun_LGetStr(void);
void fun_LCompare(void);
void fun_keydown(void);
void fun_LInstr(void);
void fun_epoch(void);
void fun_datetime(void);
void fun_json(void);
void cmd_update(void);
void fun_format(void);
void cmd_font(void);
void fun_mouse(void);
void cmd_mouse(void);
void cmd_restart(void);
void fun_json(void);
#endif


/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE

{ (unsigned char*)"AutoSave", T_CMD, 0, cmd_autosave	},
 
{ (unsigned char*)"WatchDog",		T_CMD,				0, cmd_watchdog },
/*{ (unsigned char*)"Interrupt", 		T_CMD,              0, cmd_csubinterrupt },
*/
{ (unsigned char*)"DefineFont",     T_CMD,				0, cmd_cfunction },
{ (unsigned char*)"End DefineFont", T_CMD,				0, cmd_null },
{ (unsigned char*)"Font",			 T_CMD,				0, cmd_font },
{ (unsigned char*)"Pause",			T_CMD,				0, cmd_pause },
{ (unsigned char*)"Timer",			T_CMD | T_FUN,      0, cmd_timer },
{ (unsigned char*)"Option",			T_CMD,				0, cmd_option },
{ (unsigned char*)"Test",			T_CMD,				0, cmd_test },
{ (unsigned char*)"Sort",			T_CMD,				0, cmd_sort },
{ (unsigned char*)"LongString",		T_CMD,				0, cmd_longString },
{ (unsigned char*)"IReturn",		T_CMD,				0, cmd_ireturn },
{ (unsigned char*)"SetTick",		T_CMD,				0, cmd_settick },
{ (unsigned char*)"Mouse",			T_CMD,				0, cmd_mouse },
{ (unsigned char*)"Poke",			T_CMD,				0, cmd_poke },
{ (unsigned char*)"Restart",        T_CMD,				0, cmd_restart },
{ (unsigned char*)"CSub",           T_CMD,              0, cmd_cfunction },
{ (unsigned char*)"End CSub",       T_CMD,              0, cmd_null },
#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE

/*
{ (unsigned char*)"MM.Watchdog",T_FNA | T_INT,		0, fun_restart },
*/
{ (unsigned char*)"Day$(",	T_FUN | T_STR,		0, fun_day },
{ (unsigned char*)"MM.Device$",	T_FNA | T_STR,		0, fun_device },
{ (unsigned char*)"Timer", T_FNA | T_NBR, 0, fun_timer},
{ (unsigned char*)"MM.Info(",		T_FUN | T_INT | T_NBR | T_STR,		0, fun_info },
{ (unsigned char*)"Date$",	T_FNA | T_STR,		0, fun_date },
{ (unsigned char*)"Time$",	T_FNA | T_STR,		0, fun_time },
{ (unsigned char*)"Epoch(",		T_FUN | T_INT,			0, fun_epoch },
{ (unsigned char*)"DateTime$(",		T_FUN | T_STR,		0, fun_datetime },
{ (unsigned char*)"LInStr(",		T_FUN | T_INT,		0, fun_LInstr },
{ (unsigned char*)"LCompare(",		T_FUN | T_INT,		0, fun_LCompare },
{ (unsigned char*)"LLen(",		T_FUN | T_INT,		0, fun_LLen },
{ (unsigned char*)"LGetStr$(",		T_FUN | T_STR,		0, fun_LGetStr },
{ (unsigned char*)"LGetByte(",		T_FUN | T_INT,		0, fun_LGetByte },
{ (unsigned char*)"Mouse(",		T_FUN | T_INT,		0, fun_mouse },
{ (unsigned char*)"Format$(",	T_FUN | T_STR,			0, fun_format },
{ (unsigned char*)"Keydown(",		T_FUN | T_INT,		0, fun_keydown },
{ (unsigned char*)"Peek(",		T_FUN | T_INT | T_STR | T_NBR,			0, fun_peek },
{ (unsigned char*)"JSON$(",		T_FUN | T_STR,          0, fun_json },
#endif
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
extern "C" int check_interrupt(void);
extern "C" unsigned char* GetIntAddress(unsigned char* p);
extern "C" void SoftReset(void);
extern "C" void PRet(void);
extern "C" void PInt(int64_t n);
extern "C" void PIntComma(int64_t n);
extern "C" void PIntH(unsigned long long int n);
extern "C" void PIntHC(unsigned long long int n);
extern "C" void PFlt(MMFLOAT flt);
extern "C" void PFltComma(MMFLOAT n);
extern "C" unsigned int GetPokeAddr(unsigned char* p);
extern "C" unsigned int GetPeekAddr(unsigned char* p);
extern "C" void PO(const char* s, int m);
extern int TickPeriod[NBRSETTICKS];
extern volatile int TickTimer[NBRSETTICKS];
extern unsigned char* TickInt[NBRSETTICKS];
extern volatile unsigned char TickActive[NBRSETTICKS];
extern unsigned char* MouseInterrupLeftDown;
extern volatile int MouseFoundLeftDown, MouseLeftDown;
extern unsigned char* MouseInterrupLeftUp;
extern volatile int MouseFoundLeftUp, MouseLeftUp;
extern unsigned char* MouseInterrupRightDown;
extern volatile int MouseFoundRightDown, MouseRightDown;
extern volatile int MouseDouble;
extern int IntJustDone;
extern int64_t fasttimerat0;
extern int VideoMode;
extern int ConsoleRepeat;
extern MMFLOAT optionangle;
extern int optiony;
extern int64_t lasttimer;
extern bool OptionConsoleSerial;
#endif
