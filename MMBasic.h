/***********************************************************************************************************************
MMBasic for Windows

MMBasic.h

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
#pragma once
#include "Configuration.h"
#define IntToStrBufSize 65
// Types used to define an item of data.  Often they are ORed together.
// Used in tokens, variables and arguments to functions
#define T_NOTYPE       0                            // type not set or discovered
#define T_NBR       0x01                            // number (or float) type
#define T_STR       0x02                            // string type
#define T_INT       0x04                            // 64 bit integer type
#define T_PTR       0x08                            // the variable points to another variable's data
#define T_IMPLIED   0x10                            // the variables type does not have to be specified with a suffix
#define T_CONST     0x20                            // the contents of this variable cannot be changed
#define T_BLOCKED   0x40                            // Hash table entry blocked after ERASE

#define TypeMask(a) ((a) & (T_NBR | T_INT | T_STR)) // macro to isolate the variable type bits

// types of tokens.  These are or'ed with the data types above to fully define a token
#define T_INV       0                               // an invalid token
#define T_NA        0                               // an invalid token
#define T_CMD       0x10                            // a command
#define T_OPER      0x20                            // an operator
#define T_FUN       0x40                            // a function (also used for a function that can operate as a command)
#define T_FNA       0x80                            // a function that has no arguments

#define C_BASETOKEN 0x80                            // the base of the token numbers

// flags used in the program lines
#define T_CMDEND    0                               // end of a command
#define T_NEWLINE   1                               // Single byte indicating the start of a new line
#define T_LINENBR   2                               // three bytes for a line number
#define T_LABEL     3                               // variable length indicating a label

#define E_END       255                             // dummy last operator in an expression

// these constants are used in the second argument of the findvar() function, they should be or'd together
#define V_FIND              0x0000                    // a straight forward find, if the variable is not found it is created and set to zero
#define V_NOFIND_ERR        0x0200                    // throw an error if not found
#define V_NOFIND_NULL       0x0400                    // return a null pointer if not found
#define V_DIM_VAR           0x0800                    // dimension an array
#define V_LOCAL             0x1000                    // create a local variable
#define V_EMPTY_OK          0x2000                    // allow an empty array variable.  ie, var()
#define V_FUNCT             0x4000                    // we are defining the name of a function

// these flags are used in the last argument in expression()
#define E_NOERROR           true
#define E_ERROR             0
#define E_DONE_GETVAL       0b10
#define getargs(x, y, s) unsigned char argbuf[STRINGSIZE + STRINGSIZE/2]; unsigned char *argv[y]; int argc; makeargs(x, y, argbuf, argv, &argc, s)
#define tokentype(i)    ((i >= C_BASETOKEN && i < TokenTableSize - 1 + C_BASETOKEN) ? (tokentbl[i - C_BASETOKEN].type) : 0)             // get the type of a token
#define tokenfunction(i)((i >= C_BASETOKEN && i < TokenTableSize - 1 + C_BASETOKEN) ? (tokentbl[i - C_BASETOKEN].fptr) : (tokentbl[0].fptr))    // get the function pointer  of a token
#define tokenname(i)    ((i >= C_BASETOKEN && i < TokenTableSize - 1 + C_BASETOKEN) ? (tokentbl[i - C_BASETOKEN].name) : (unsigned char *)"")            // get the name of a token

#define commandtype(i)  ((i >= C_BASETOKEN && i < CommandTableSize - 1 + C_BASETOKEN) ? (commandtbl[i - C_BASETOKEN].type) : 0)             // get the type of a token
#define commandfunction(i)((i >= C_BASETOKEN && i < CommandTableSize - 1 + C_BASETOKEN) ? (commandtbl[i - C_BASETOKEN].fptr) : (commandtbl[0].fptr))    // get the function pointer  of a token
#define commandname(i)  ((i >= C_BASETOKEN && i < CommandTableSize - 1 + C_BASETOKEN) ? (commandtbl[i - C_BASETOKEN].name) : (unsigned char *)"")        // get the name of a command
#define MAXLINENBR          65001                                   // maximim acceptable line number
#define NOLINENBR           MAXLINENBR+1
// finishes with x pointing to the next non space char
#define skipspace(x)    while(*x == ' ') x++
#define IsDigitinline(a)	( a >= '0' && a <= '9' )
extern const char namestart[256];
extern const char namein[256];
extern const char nameend[256];
extern const unsigned char myupper[256];
#define mytoupper(a) myupper[(unsigned int)a]
#define isnamestart(c)  (namestart[(uint8_t)c])                    // true if valid start of a variable name
#define isnamechar(c)   (namein[(uint8_t)c])        // true if valid part of a variable name
#define isnameend(c)    (nameend[(uint8_t)c])        // true if valid at the end of a variable name
#define TypeMask(a) ((a) & (T_NBR | T_INT | T_STR)) // macro to isolate the variable type bits

// skip to the next element
// finishes pointing to the zero unsigned char that preceeds an element
#define skipelement(x)  while(*x) x++

// skip to the next line
// skips text and and element separators until it is pointing to the zero char marking the start of a new line.
// the next byte will be either the newline token or zero char if end of program
#define skipline(x)     while(!(x[-1] == 0 && (x[0] == T_NEWLINE || x[0] == 0)))x++

extern "C" void IntToStr(char* strr, long long int nbr, unsigned int base);
extern "C" void IntToStrPad(char* p, long long int nbr, signed char padch, int maxch, int radix);
extern "C" void MMPrintString(char* s);
extern "C" void  error(char* msg, ...);
extern "C" void Mstrcpy(unsigned char* dest, unsigned char* src);
extern "C" void Mstrcat(unsigned char* dest, unsigned char* src);
extern "C" int Mstrcmp(unsigned char* s1, unsigned char* s2);
extern "C" void makeargs(unsigned char** tp, int maxargs, unsigned char* argbuf, unsigned char* argv[], int* argc, unsigned char* delim);
extern "C" MMFLOAT getnumber(unsigned char* p);
extern "C" long long int  getinteger(unsigned char* p);
extern "C" long long int getint(unsigned char* p, long long int min, long long int max);
extern "C" unsigned char* getstring(unsigned char* p);
extern "C" unsigned char* CtoM(unsigned char* p);
extern "C" void FloatToStr(char* p, MMFLOAT f, int m, int n, unsigned char ch);
extern "C" unsigned char* evaluate(unsigned char* p, MMFLOAT * fa, long long int* ia, unsigned char** sa, int* ta, int flags);
extern "C" unsigned char* checkstring(unsigned char* p, unsigned char* tkn);
extern "C" unsigned char* getCstring(unsigned char* p);
extern "C" unsigned char* MtoC(unsigned char* p);
extern "C" int FloatToInt32(MMFLOAT x);
extern "C" void* findvar(unsigned char* p, int action);
extern "C" int FindSubFun(unsigned char* p, int type);
extern "C" void ClearVars(int level);
extern "C" void DefinedSubFun(int isfun, unsigned char* cmd, int index, MMFLOAT * fa, long long int* i64a, unsigned char** sa, int* typ);
extern "C" void* DoExpression(unsigned char* p, int* t);
extern "C" void  tokenise(int console);
extern "C" void ExecuteProgram(unsigned char* p);
extern "C" void STR_REPLACE(char* target, const char* needle, const char* replacement);
extern "C" unsigned char* skipvar(unsigned char* p, int noerror);
extern "C" long long int  FloatToInt64(MMFLOAT x);
extern "C" void checkend(unsigned char* p);
extern "C" int GetCommandValue(unsigned char* n);
extern "C" int GetTokenValue(unsigned char* n);
extern "C" void makeupper(unsigned char* p);
extern "C" unsigned char* GetNextCommand(unsigned char* p, unsigned char** CLine, unsigned char* EOFMsg);
extern "C" unsigned char* doexpr(unsigned char* p, MMFLOAT * fa, long long int* ia, unsigned char** sa, int* oo, int* ta);
extern "C" unsigned char* findlabel(unsigned char* labelptr);
extern "C" void ClearRuntime(void);
extern "C" void ClearProgram(void);
extern "C" unsigned char* findline(int nbr, int mustfind);
extern "C" int mystrncasecmp(const unsigned char* s1, const unsigned char* s2, size_t  length);
extern "C" int CountLines(unsigned char* target);
extern "C" void  PrepareProgram(int ErrAbort);
extern "C" void InitBasic(void);
extern "C" int mem_equal(unsigned char* s1, unsigned char* s2, int i);
extern "C" unsigned char* getclosebracket(unsigned char* p);
extern "C" int strncasecmp(const char* s1, const char* s2, size_t n);
extern "C" int str_equal(const unsigned char* s1, const unsigned char* s2);
extern unsigned char inpbuf[];                                            // used to store user keystrokes until we have a line
extern unsigned char tknbuf[];                                            // used to store the tokenised representation of the users input line
extern MMFLOAT farg1, farg2, fret;                // Global floating point variables used by operators
extern long long int  iarg1, iarg2, iret;        // Global integer variables used by operators
extern unsigned char* sarg1, * sarg2, * sret;              // Global string pointers used by operators
extern int targ;                                // Global type of argument (string or float) returned by an operator
extern int varcnt;                              // number of variables defined (eg, largest index into the variable table)
extern int Localvarcnt;                              // number of LOCAL variables defined (eg, largest index into the variable table)
extern int Globalvarcnt;                              // number of GLOBAL variables defined (eg, largest index into the variable table)
extern int VarIndex;                            // index of the current variable.  set after the findvar() function has found/created a variable
extern int LocalIndex;                          // used to track the level of local variables
struct s_funtbl {
    char name[MAXVARLEN];                       // variable's name
    uint32_t index;
};
struct s_tokentbl {                             // structure of the token table
    unsigned char* name;                                 // the string (eg, PRINT, FOR, ASC(, etc)
    unsigned char type;                                  // the type returned (T_NBR, T_STR, T_INT)
    unsigned char precedence;                            // precedence used by operators only.  operators with equal precedence are processed left to right.
    void (*fptr)(void);                         // pointer to the function that will interpret that token
};
extern int TempStringClearStart;                                           // used to prevent clearing of space in an expression that called a FUNCTION
extern unsigned char tokenTHEN, tokenELSE, tokenGOTO, tokenEQUAL, tokenTO, tokenSTEP, tokenWHILE, tokenUNTIL, tokenGOSUB, tokenAS, tokenFOR;
extern unsigned char cmdIF, cmdENDIF, cmdEND_IF, cmdELSEIF, cmdELSE_IF, cmdELSE, cmdSELECT_CASE, cmdFOR, cmdNEXT, cmdWHILE, cmdENDSUB, cmdENDFUNCTION, cmdLOCAL, cmdSTATIC, cmdCASE, cmdDO, cmdLOOP, cmdCASE_ELSE, cmdEND_SELECT;
extern unsigned char cmdSUB, cmdFUN, cmdCFUN, cmdCSUB, cmdIRET;
extern int cmdtoken;                            // Token number of the command
extern unsigned char* cmdline;                           // Command line terminated with a zero unsigned char and trimmed of spaces
extern unsigned char* nextstmt;                          // Pointer to the next statement to be executed.
extern unsigned char* ep;                                // Pointer to the argument to a function
extern int emptyarray;
extern int TempStringClearStart;                                           // used to prevent clearing of space in an expression that called a FUNCTION
extern int NextData;                                                       // used to track the next item to read in DATA & READ stmts
extern unsigned char* NextDataLine;                                                 // used to track the next line to read in DATA & READ stmts
extern int OptionBase;                                                     // track the state of OPTION BASE
extern int MMCharPos;
extern unsigned char DefaultType;                                                   // the default type if a variable is not specifically typed
extern int CommandTableSize, TokenTableSize;
extern const struct s_tokentbl tokentbl[];
extern const struct s_tokentbl commandtbl[];
extern unsigned char* ContinuePoint;
extern unsigned char* CurrentLinePtr;                                               // Pointer to the current line (used in error reporting)
extern unsigned char* subfun[];
extern char CurrentSubFunName[];
extern char CurrentInterruptName[];
extern unsigned char OptionExplicit;                                                // used to force the declaration of variables before their use
                            // longjump to recover from an error

struct s_vartbl {                               // structure of the variable table
    char name[MAXVARLEN];                       // variable's name
    char type;                                  // its type (T_NUM, T_INT or T_STR)
    char level;                                 // its subroutine or function level (used to track local variables)
    unsigned char size;                         // the number of chars to allocate for each element in a string array
    char dummy;
    int dims[MAXDIM];                     // the dimensions. it is an array if the first dimension is NOT zero
    union u_val {
        MMFLOAT f;                              // the value if it is a float
        long long int i;                        // the value if it is an integer
        MMFLOAT* fa;                            // pointer to the allocated memory if it is an array of floats
        long long int* ia;                      // pointer to the allocated memory if it is an array of integers
        char* s;                                // pointer to the allocated memory if it is a string
    }   val;
} ;

struct s_hash {                             // structure of the token table
    short hash;                                 // the string (eg, PRINT, FOR, ASC(, etc)
    short level;                                  // the type returned (T_NBR, T_STR, T_INT)
};
extern struct s_vartbl vartbl[MAXVARS];                                            // this table stores all variables

