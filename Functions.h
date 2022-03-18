/***********************************************************************************************************************
MMBasic for Windows

Functions.h

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

void fun_abs(void);
void fun_asc(void);
void fun_atn(void);
void fun_atan2(void);
void fun_base(void); 
void fun_bin(void);
void fun_chr(void);
void fun_cint(void);
void fun_cos(void);
void fun_deg(void);
void fun_exp(void);
void fun_fix(void);
void fun_hex(void);
void fun_inkey(void);
void fun_instr(void);
void fun_int(void);
void fun_lcase(void);
void fun_left(void);
void fun_len(void);
void fun_log(void);
void fun_errno(void);
void fun_errmsg(void);
void fun_mid(void);
void fun_oct(void);
void fun_peek(void);
void fun_pi(void);
void fun_pos(void);
void fun_rad(void);
void fun_right(void);
void fun_rnd(void);
void fun_sgn(void);
void fun_sin(void);
void fun_space(void);
void fun_sqr(void);
void fun_str(void);
void fun_string(void);
void fun_tab(void);
void fun_tan(void);
void fun_ucase(void);
void fun_val(void);
void fun_eval(void);
void fun_version(void);
void fun_asin(void);
void fun_acos(void);
void fun_field(void);
void fun_max(void);
void fun_min(void);
void fun_bin2str(void);
void fun_str2bin(void);
void fun_test(void);
void fun_bound(void);
void fun_ternary(void);
void fun_call(void);
void fun_cmdline(void);

#endif




/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE
// the format is:
//    TEXT      	TYPE                P  FUNCTION TO CALL
// where type is always T_CMD
// and P is the precedence (which is only used for operators and not commands)

#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE
// the format is:
//    TEXT      	TYPE                    P  FUNCTION TO CALL
// where type is T_NA, T_FUN, T_FNA or T_OPER argumented by the types T_STR and/or T_NBR and/or T_INT
// note that the variable types (T_STR, etc) are not used as it is the responsibility of the function
// to set its return type in targ.
// and P is the precedence (which is only used for operators)
{ (unsigned char*)"ACos(", T_FUN | T_NBR, 0, fun_acos		},
{ (unsigned char*)"Abs(",		T_FUN | T_NBR | T_INT, 	0, fun_abs },
{ (unsigned char*)"Asc(",		T_FUN | T_INT,			0, fun_asc },
{ (unsigned char*)"ASin(",		T_FUN | T_NBR,			0, fun_asin },
{ (unsigned char*)"Atn(",		T_FUN | T_NBR,			0, fun_atn },
{ (unsigned char*)"Atan2(",		T_FUN | T_NBR,			0, fun_atan2 },
{ (unsigned char*)"Base$(",		T_FUN | T_STR,			0, fun_base },
//{ (unsigned char*)"Bin$(",		T_FUN | T_STR,			0, fun_bin },
{ (unsigned char*)"Bound(",		T_FUN | T_INT,			0, fun_bound },
{ (unsigned char*)"Choice(",	T_FUN | T_STR | T_INT | T_NBR,		0, fun_ternary },
{ (unsigned char*)"Chr$(",		T_FUN | T_STR,			0, fun_chr, },
{ (unsigned char*)"Cint(",		T_FUN | T_INT,			0, fun_cint },
{ (unsigned char*)"Cos(",		T_FUN | T_NBR,			0, fun_cos },
{ (unsigned char*)"Deg(",		T_FUN | T_NBR,			0, fun_deg },
{ (unsigned char*)"Exp(",		T_FUN | T_NBR,			0, fun_exp },
{ (unsigned char*)"Field$(",    T_FUN | T_STR,			0, fun_field },
{ (unsigned char*)"Fix(",		T_FUN | T_INT,			0, fun_fix },
//{ (unsigned char*)"Hex$(",		T_FUN | T_STR,			0, fun_hex },
{ (unsigned char*)"Inkey$",	T_FNA | T_STR,         0, fun_inkey },
{ (unsigned char*)"Instr(",		T_FUN | T_INT,			0, fun_instr },
{ (unsigned char*)"Int(",		T_FUN | T_INT,			0, fun_int },
{ (unsigned char*)"LCase$(",            T_FUN | T_STR,			0, fun_lcase },
{ (unsigned char*)"Left$(",		T_FUN | T_STR,			0, fun_left },
{ (unsigned char*)"Len(",		T_FUN | T_INT,			0, fun_len },
{ (unsigned char*)"Log(",		T_FUN | T_NBR,			0, fun_log },
{ (unsigned char*)"Mid$(",		T_FUN | T_STR,			0, fun_mid },
{ (unsigned char*)"MM.Errno",	T_FNA | T_INT,		0, fun_errno },
{ (unsigned char*)"MM.ErrMsg$", T_FNA | T_STR,         0, fun_errmsg },
{ (unsigned char*)"MM.Ver",		T_FNA | T_NBR,			0, fun_version },
//{ (unsigned char*)"Oct$(",		T_FUN | T_STR,			0, fun_oct },
//	{ (unsigned char *)"Peek(",		T_FUN  | T_INT,			0, fun_peek		},
{ (unsigned char*)"Pi",			T_FNA | T_NBR,			0, fun_pi },
{ (unsigned char*)"Pos",		T_FNA | T_INT,                 0, fun_pos },
{ (unsigned char*)"Rad(",		T_FUN | T_NBR,			0, fun_rad },
{ (unsigned char*)"Right$(",            T_FUN | T_STR,			0, fun_right },
{ (unsigned char*)"Rnd(",		T_FUN | T_NBR,			0, fun_rnd },        // this must come before Rnd - without bracket
{ (unsigned char*)"Rnd",		T_FNA | T_NBR,			0, fun_rnd },        // this must come after Rnd(
{ (unsigned char*)"Sgn(",		T_FUN | T_INT,			0, fun_sgn },
{ (unsigned char*)"Sin(",		T_FUN | T_NBR,			0, fun_sin },
{ (unsigned char*)"Space$(",            T_FUN | T_STR,			0, fun_space },
{ (unsigned char*)"Sqr(",		T_FUN | T_NBR,			0, fun_sqr },
{ (unsigned char*)"Str$(",		T_FUN | T_STR,			0, fun_str },
{ (unsigned char*)"String$(",           T_FUN | T_STR,			0, fun_string },
{ (unsigned char*)"Tab(",		T_FUN | T_STR,                 0, fun_tab, },
{ (unsigned char*)"Tan(",		T_FUN | T_NBR,			0, fun_tan },
{ (unsigned char*)"UCase$(",            T_FUN | T_STR,			0, fun_ucase },
{ (unsigned char*)"Val(",		T_FUN | T_NBR | T_INT,		0, fun_val },
{ (unsigned char*)"Eval(",		T_FUN | T_NBR | T_INT | T_STR,	0, fun_eval },
{ (unsigned char*)"Max(",		T_FUN | T_NBR,			0, fun_max },
{ (unsigned char*)"Min(",		T_FUN | T_NBR,			0, fun_min },
{ (unsigned char*)"Bin2str$(",  T_FUN | T_STR,			0, fun_bin2str },
{ (unsigned char*)"Str2bin(",	T_FUN | T_NBR | T_INT,	0, fun_str2bin },
{ (unsigned char*)"Call(",		T_FUN | T_STR | T_INT | T_NBR,		0, fun_call },
{ (unsigned char*)"MM.CmdLine$",T_FNA | T_STR,			0, fun_cmdline },

#endif
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
#define RADCONV   (MMFLOAT)57.2957795130823229	  // Used when converting degrees -> radians and vice versa
#define PI_VALUE  (MMFLOAT)3.14159265358979323
#define Rad(a)  (((MMFLOAT)a) / RADCONV)
#endif
