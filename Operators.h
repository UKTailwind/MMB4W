/***********************************************************************************************************************
MMBasic for Windows

Operators.h

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
// format:
//      void cmd_???(void)
//      void fun_???(void)
//      void op_???(void)

void op_invalid(void);
void op_exp(void);
void op_mul(void);
void op_div(void);
void op_divint(void);
void op_add(void);
void op_subtract(void);
void op_mod(void);
void op_ne(void);
void op_gte(void);
void op_lte(void);
void op_lt(void);
void op_gt(void);
void op_equal(void);
void op_and(void);
void op_or(void);
void op_xor(void);
void op_not(void);
void op_shiftleft(void);
void op_shiftright(void);
void op_inv(void);

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
// and P is the precedence (which is only used for operators)
	{ (unsigned char *)"^",			T_OPER | T_NBR | T_INT,		0, op_exp		},
	{ (unsigned char *)"*",			T_OPER | T_NBR | T_INT,		1, op_mul		},
	{ (unsigned char *)"/",			T_OPER | T_NBR,                 1, op_div		},
	{ (unsigned char *)"\\",			T_OPER | T_INT,			1, op_divint            },
	{ (unsigned char *)"Mod",		T_OPER | T_INT,			1, op_mod		},
	{ (unsigned char *)"+",			T_OPER | T_NBR | T_INT | T_STR, 2, op_add		},
	{ (unsigned char *)"-",			T_OPER | T_NBR | T_INT,		2, op_subtract          },
	{ (unsigned char *)"Not",		T_OPER | T_NBR | T_INT,			3, op_not		},
	{ (unsigned char *)"INV",			T_OPER | T_NBR | T_INT,			3, op_inv		},
	{ (unsigned char *)"<<",			T_OPER | T_INT,                 4, op_shiftleft		},
	{ (unsigned char *)">>",			T_OPER | T_INT,                 4, op_shiftright	},
	{ (unsigned char *)"<>",			T_OPER | T_NBR | T_INT | T_STR, 5, op_ne		},
	{ (unsigned char *)">=",			T_OPER | T_NBR | T_INT | T_STR, 5, op_gte		},
	{ (unsigned char *)"<=",			T_OPER | T_NBR | T_INT | T_STR, 5, op_lte		},
	{ (unsigned char *)"<",			T_OPER | T_NBR | T_INT | T_STR, 5, op_lt		},
	{ (unsigned char *)">",			T_OPER | T_NBR | T_INT | T_STR, 5, op_gt		},
	{ (unsigned char *)"=",			T_OPER | T_NBR | T_INT | T_STR, 6, op_equal		},
	{ (unsigned char *)"And",		T_OPER | T_INT,			7, op_and		},
	{ (unsigned char *)"Or",			T_OPER | T_INT,			7, op_or		},
	{ (unsigned char *)"Xor",		T_OPER | T_INT,			7, op_xor		},
	{ (unsigned char *)"As",			T_NA,			0, op_invalid },

#endif

