/***********************************************************************************************************************
MMBasic for Windows

Commands.h

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

void cmd_clear(void);
void cmd_continue(void);
void cmd_delete(void);
void cmd_dim(void);
void cmd_do(void);
void cmd_else(void);
void cmd_end(void);
void cmd_endfun(void);
void cmd_endsub(void);
void cmd_erase(void);
void cmd_error(void);
void cmd_exit(void);
void cmd_exitfor(void);
void cmd_for(void);
void cmd_subfun(void);
void cmd_gosub(void);
void cmd_goto(void);
void cmd_if(void);
void cmd_inc(void);
void cmd_input(void);
void cmd_let(void);
void cmd_lineinput(void);
void cmd_list(void);
void cmd_load(void);
void cmd_loop(void);
void cmd_merge(void);
void cmd_chain(void);
void cmd_new(void);
void cmd_next(void);
void cmd_null(void);
void cmd_on(void);
void cmd_print(void);
void cmd_randomize(void);
void cmd_read(void);
void cmd_restore(void);
void cmd_return(void);
void cmd_run(void);
void cmd_save(void);
void cmd_troff(void);
void cmd_tron(void);
void cmd_trace(void);
void cmd_const(void);
void cmd_select(void);
void cmd_case(void);
void cmd_option(void);
void cmd_dump(void);
void cmd_call(void);
void cmd_execute(void);
void cmd_mid(void);
void cmd_quit(void);
void cmd_debug(void);
#endif


/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE

{ (unsigned char*)"?", T_CMD, 0, cmd_print	},
{ (unsigned char*)"Call",		T_CMD,				0, cmd_call },
{ (unsigned char*)"Clear",		T_CMD,				0, cmd_clear },
{ (unsigned char*)"Continue",           T_CMD,                          0, cmd_continue },
{ (unsigned char*)"Data",		T_CMD,				0, cmd_null },
{ (unsigned char*)"Dim",		T_CMD,				0, cmd_dim },
{ (unsigned char*)"Do",			T_CMD,				0, cmd_do },
{ (unsigned char*)"ElseIf",		T_CMD,				0, cmd_else },
//	{ (unsigned char *)"Else If",		T_CMD,				0, cmd_else	},
{ (unsigned char*)"Case Else",		T_CMD,				0, cmd_case },
{ (unsigned char*)"Else",		T_CMD,				0, cmd_else },
{ (unsigned char*)"Select Case",	T_CMD,				0, cmd_select },
{ (unsigned char*)"End Select",		T_CMD,				0, cmd_null },
{ (unsigned char*)"Case",		T_CMD,				0, cmd_case },
{ (unsigned char*)"EndIf",		T_CMD,				0, cmd_null },
//	{ (unsigned char *)"End If",		T_CMD,				0, cmd_null	},
{ (unsigned char*)"End Function",       T_CMD,                          0, cmd_endfun },      // this entry must come before END and FUNCTION
{ (unsigned char*)"End Sub",            T_CMD,                          0, cmd_return },      // this entry must come before END and SUB
{ (unsigned char*)"End",		T_CMD,				0, cmd_end },
{ (unsigned char*)"Erase",		T_CMD,				0, cmd_erase },
{ (unsigned char*)"Error",		T_CMD,				0, cmd_error },
{ (unsigned char*)"Exit For",           T_CMD,				0, cmd_exitfor },      // this entry must come before EXIT and FOR
{ (unsigned char*)"Exit Sub",           T_CMD,				0, cmd_return },      // this entry must come before EXIT and SUB
{ (unsigned char*)"Exit Function",      T_CMD,                          0, cmd_endfun },      // this entry must come before EXIT and FUNCTION
//	{ (unsigned char *)"Exit Do",            T_CMD,				0, cmd_exit	},
{ (unsigned char*)"Exit",		T_CMD,				0, cmd_exit },
{ (unsigned char*)"For",		T_CMD,				0, cmd_for },
{ (unsigned char*)"Function",           T_CMD,				0, cmd_subfun },
{ (unsigned char*)"GoSub",		T_CMD,				0, cmd_gosub },
{ (unsigned char*)"GoTo",		T_CMD,				0, cmd_goto },
{ (unsigned char*)"Inc",			T_CMD,				0, cmd_inc },
{ (unsigned char*)"If",			T_CMD,				0, cmd_if },
{ (unsigned char*)"Line Input",         T_CMD,				0, cmd_lineinput },      // this entry must come before INPUT
{ (unsigned char*)"Input",		T_CMD,				0, cmd_input },
{ (unsigned char*)"Let",		T_CMD,				0, cmd_let },
{ (unsigned char*)"List",		T_CMD,				0, cmd_list },
{ (unsigned char*)"Local",		T_CMD,				0, cmd_dim },
{ (unsigned char*)"Loop",		T_CMD,				0, cmd_loop },
{ (unsigned char*)"New",		T_CMD,				0, cmd_new },
{ (unsigned char*)"Next",		T_CMD,				0, cmd_next },
{ (unsigned char*)"On",			T_CMD,				0, cmd_on },
{ (unsigned char*)"Print",		T_CMD,				0, cmd_print },
{ (unsigned char*)"Randomize",          T_CMD,				0, cmd_randomize },
{ (unsigned char*)"Read",		T_CMD,				0, cmd_read },
{ (unsigned char*)"Rem",		T_CMD,				0, cmd_null, },
{ (unsigned char*)"Restore",            T_CMD,				0, cmd_restore },
{ (unsigned char*)"Return",		T_CMD,				0, cmd_return, },
{ (unsigned char*)"Run",		T_CMD,				0, cmd_run },
{ (unsigned char*)"Static",		T_CMD,				0, cmd_dim },
{ (unsigned char*)"Sub",		T_CMD,				0, cmd_subfun },
//	{ (unsigned char *)"TROFF",		T_CMD,				0, cmd_troff	},
//	{ (unsigned char *)"TRON",		T_CMD,				0, cmd_tron	},
{ (unsigned char*)"Trace",		T_CMD,				0, cmd_trace },
//	{ (unsigned char *)"Wend",		T_CMD,				0, cmd_loop	},
{ (unsigned char*)"While",		T_CMD,				0, cmd_do },
{ (unsigned char*)"Const",		T_CMD,				0, cmd_const },
//	{ (unsigned char *)"Execute",	T_CMD,				0, cmd_execute	},
{ (unsigned char*)"MID$(",		T_CMD | T_FUN,		0, cmd_mid },
{ (unsigned char*)"Execute",	T_CMD,				0, cmd_execute },
{ (unsigned char*)"Quit",	    T_CMD,				0, cmd_quit },
{ (unsigned char*)"Console",	    T_CMD,				0, cmd_debug },

#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE

{ (unsigned char*)"For",		T_NA,				0, op_invalid },
{ (unsigned char*)"Else",		T_NA,				0, op_invalid },
{ (unsigned char*)"GoSub",		T_NA,				0, op_invalid },
{ (unsigned char*)"GoTo",		T_NA,				0, op_invalid },
{ (unsigned char*)"Step",		T_NA,				0, op_invalid },
{ (unsigned char*)"Then",		T_NA,				0, op_invalid },
{ (unsigned char*)"To",			T_NA,				0, op_invalid },
{ (unsigned char*)"Until",		T_NA,				0, op_invalid },
{ (unsigned char*)"While",		T_NA,				0, op_invalid },

#endif

#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)

struct s_forstack {
    unsigned char* forptr;                           // pointer to the FOR command in program memory
    unsigned char* nextptr;                          // pointer to the NEXT command in program memory
    void* var;                              // value of the FOR variable
    unsigned char vartype;                           // type of the variable
    unsigned char level;                             // the sub/function level that the loop was created
    union u_totype {
        MMFLOAT f;                            // the TO value if it is a float
        long long int  i;                    // the TO value if it is an integer
    } tovalue;
    union u_steptype {
        MMFLOAT f;                            // the STEP value if it is a float
        long long int  i;                    // the STEP value if it is an integer
    } stepvalue;
};

extern struct s_forstack forstack[MAXFORLOOPS + 1];
extern int forindex;

struct s_dostack {
    unsigned char* evalptr;                          // pointer to the expression to be evaluated
    unsigned char* loopptr;                          // pointer to the loop statement
    unsigned char* doptr;                            // pointer to the DO statement
    unsigned char level;                             // the sub/function level that the loop was created
};

extern struct s_dostack dostack[MAXDOLOOPS];
extern int doindex;

extern unsigned char* gosubstack[MAXGOSUB];
extern unsigned char* errorstack[MAXGOSUB];
extern int gosubindex;
extern unsigned char DimUsed;

//extern unsigned char *GetFileName(char* CmdLinePtr, unsigned char *LastFilePtr);
//extern void mergefile(unsigned char *fname, unsigned char *MemPtr);
extern void ListProgram(unsigned char* p, int all);
extern unsigned char* llist(unsigned char* b, unsigned char* p);
extern  "C" unsigned char* CheckIfTypeSpecified(unsigned char* p, int* type, int AllowDefaultType);
extern int MMerrno;
extern char *MMErrMsg;
// definitions related to setting video off and on
extern const unsigned int CaseOption;
extern volatile int Keycomplete;
extern unsigned char* KeyInterrupt;
extern int keyselect;

#define TRACE_BUFF_SIZE  128

extern unsigned int BusSpeed;
extern unsigned char* OnKeyGOSUB;
extern unsigned char EchoOption;
extern int TraceOn;
extern unsigned char* TraceBuff[TRACE_BUFF_SIZE];
extern int TraceBuffIndex;                                          // used for listing the contents of the trace buffer
extern int OptionErrorSkip;
extern char *MMErrMsg;
extern char runcmd[STRINGSIZE];
extern unsigned char* SaveNextDataLine;
extern int SaveNextData;

#endif
