/***********************************************************************************************************************
MMBasic for Windows

Maths.h

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

//void cmd_FFT(void);
void cmd_math(void);
void fun_math(void);

#endif




/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE

{ (unsigned char *)"Math", T_CMD, 0, cmd_math		},

#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE
{ (unsigned char*)"Math(",	    T_FUN | T_NBR,		0, fun_math },

#endif




#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)


// General definitions used by other modules
#define MMFLOAT double
#define M_PI		3.14159265358979323846

extern void Q_Mult(MMFLOAT* q1, MMFLOAT* q2, MMFLOAT* n);
extern void Q_Invert(MMFLOAT* q, MMFLOAT* n);
extern void cmd_SensorFusion(char* passcmdline);
void MahonyQuaternionUpdate(MMFLOAT ax, MMFLOAT ay, MMFLOAT az, MMFLOAT gx, MMFLOAT gy, MMFLOAT gz, MMFLOAT mx, MMFLOAT my, MMFLOAT mz, MMFLOAT Ki, MMFLOAT Kp, MMFLOAT deltat, MMFLOAT* yaw, MMFLOAT* pitch, MMFLOAT* roll);
void MadgwickQuaternionUpdate(MMFLOAT ax, MMFLOAT ay, MMFLOAT az, MMFLOAT gx, MMFLOAT gy, MMFLOAT gz, MMFLOAT mx, MMFLOAT my, MMFLOAT mz, MMFLOAT beta, MMFLOAT deltat, MMFLOAT* pitch, MMFLOAT* yaw, MMFLOAT* roll);
extern volatile unsigned int AHRSTimer;
extern "C" unsigned char*** alloc3df(int l, int m, int n);
extern "C" void dealloc3df(uint8_t * **array, int l, int m, int n);
#endif

