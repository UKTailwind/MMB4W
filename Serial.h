/***********************************************************************************************************************
MMBasic for Windows

Serial.h

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
#include "olcPixelGameEngine.h"
extern "C" unsigned char SerialPutchar(int comnbr, unsigned char c);
extern "C" int SerialOpen(char* fname);
extern "C" int SerialGetchar(int comnbr);
extern "C" int SerialTxStatus(int comnbr);
extern "C" int SerialRxStatus(int comnbr);
extern "C" void SerialClose(int comnbr);
extern "C" int TestPort(int portno);
extern "C" void SerialCloseAll(void);
extern "C" void SendBreak(int fnbr);
#define	COM_DEFAULT_BAUD_RATE       9600
#define	COM_DEFAULT_BUF_SIZE	    1024
#define TX_BUFFER_SIZE              256
extern unsigned char* comRx_buf[MAXCOMPORTS];											// pointer to the buffer for received characters
extern volatile int comRx_head[MAXCOMPORTS];
extern volatile int comRx_tail[MAXCOMPORTS];								// head and tail of the ring buffer for com1
extern volatile int comcomplete[MAXCOMPORTS];
extern HANDLE hComm[MAXCOMPORTS];
extern int comn[MAXCOMPORTS];														// true if COM1 is enabled
extern int com_buf_size[MAXCOMPORTS];													// size of the buffer used to receive chars
extern int com_baud[MAXCOMPORTS];												// determines the baud rate
extern char* com_interrupt[MAXCOMPORTS];											// pointer to the interrupt routine
extern int com_ilevel[MAXCOMPORTS];
extern int comused;