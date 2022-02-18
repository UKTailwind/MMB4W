/***********************************************************************************************************************
MMBasic for Windows

Configuration.h

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
#define RoundUptoPage(a)     ((((uint64_t)a) + (uint64_t)(256 - 1)) & (uint64_t)(~(256 - 1)))// round up to the nearest whole integer
#define MagicKey 0x9CFC28E6
#define HEAP_MEMORY_SIZE	(65536*1024*2)
#define MAX_PROG_SIZE		(1024*1024)
#define CONSOLE_RX_BUF_SIZE MAX_PROG_SIZE
#define BREAK_KEY 3
#define MMFLOAT double
#define TFLOAT double
#define MAXFORLOOPS         20                      // each entry uses 17 bytes
#define MAXDOLOOPS          20                      // each entry uses 12 bytes
#define MAXGOSUB            50                     // each entry uses 4 bytes
#define MAX_MULTILINE_IF    20                      // each entry uses 8 bytes
#define MAXTEMPSTRINGS      64                      // each entry takes up 4 bytes
#define MAXSUBFUN           512               // each entry takes up 4 bytes
#define MAXSUBHASH          MAXSUBFUN
// operating characteristics
#define MAXVARLEN           32                      // maximum length of a variable name
#define MAXSTRLEN           255                     // maximum length of a string
#define STRINGSIZE          256                     // must be 1 more than MAXSTRLEN.  2 of these buffers are staticaly created
#define MAXDIM              5                       // maximum nbr of dimensions to an array
#define MAXOPENFILES		128
#define MAXCOMPORTS			64
#define MAXERRMSG           128                      // max error msg size (MM.ErrMsg$ is truncated to this)
#define MAXSOUNDS           4
#define MAXKEYLEN           64
// define the maximum number of arguments to PRINT, INPUT, WRITE, ON, DIM, ERASE, DATA and READ
// each entry uses zero bytes.  The number is limited by the length of a command line
#define MAX_ARG_COUNT       50
#define STR_AUTO_PRECISION  999
#define STR_SIG_DIGITS 9                            // number of significant digits to use when converting MMFLOAT to a string
#define RoundUptoInt(a)     (((a) + (32 - 1)) & (~(32 - 1)))// round up to the nearest whole integer
#define MAXCFUNCTION		20
#define CURSOR_OFF			350              // cursor off time in mS
#define CURSOR_ON			650                  // cursor on time in mS
#define FNV_prime           16777619
#define FNV_offset_basis    2166136261
#define MAXVARS             1024                     // 8 + MAXVARLEN + MAXDIM * 2  (ie, 56 bytes) - these do not incl array members
#define MAXVARHASH				MAXVARS/2
#define CONFIG_TITLE		0
#define CONFIG_LOWER		1
#define CONFIG_UPPER		2
#define VCHARS				25					// nbr of lines in the DOS box (used in LIST)
#define MAXMODES			18
#define MAXCTRLS			1000
#define RESET_COMMAND       9999                                // indicates that the reset was caused by the RESET command
#define WATCHDOG_TIMEOUT    9998                                // reset caused by the watchdog timer
#define PIN_RESTART         9997                                // reset caused by entering 0 at the PIN prompt
#define RESTART_NOAUTORUN   9996                                // reset required after changing the LCD or touch config
#define SCREWUP_TIMEOUT    	9994                                // reset caused by the watchdog timer
#define EDIT_BUFFER_SIZE    MAX_PROG_SIZE			// this is the maximum RAM that we can get
#define FRAMEBUFFERSIZE		(3840*2160*16)
#define SCREENSIZE			(RoundUptoPage(VRes * HRes * sizeof(uint32_t)))
#define MAXPAGES			((FRAMEBUFFERSIZE / SCREENSIZE) - 1)
#define SMALLPAGE			(240*216*4)
#define MAXTOTALPAGES		(FRAMEBUFFERSIZE/SMALLPAGE + 6)
#define NBRSETTICKS         4                       // the number of SETTICK interrupts available
#define WPN (MAXTOTALPAGES-1)   //Framebuffer page no.
#define BPN (MAXTOTALPAGES-2)   //Framebuffer backup page no.
#define TPN (MAXTOTALPAGES-3)   //temporary page no. for when duplicate lines need processing
#define TPN1 (MAXTOTALPAGES-4)   //temporary page no. for when duplicate lines need processing
#define TPN2 (MAXTOTALPAGES-5)   //temporary page no. for when duplicate lines need processing
#define MAXBLITBUF          65                      // the maximum number of BLIT buffers
#define MAXCOLLISIONS		8						// maximum number of collisions tested
#define MAXLAYER            10                      // maximum number of sprite layers
#define MAX3D				256						// Maximum number of 3D objects
#define MAXCAM				6						// Maximum number of cameras
#define FLOAT3D double
#define sqrt3d sqrtf
#define round3d roundf
#define fabs3d fabsf
#define MAX_POLYGON_VERTICES 128
#define POKERANGE(a)   ((a>=(uint32_t)FrameBuffer && a<(uint32_t)FrameBuffer+FRAMEBUFFERSIZE) || (a >= (uint32_t)vartbl && a < (uint32_t)vartbl + MAXVARS * sizeof(s_vartbl)) || (a>=(uint32_t)MMHeap && a< (uint32_t)MMHeap+HEAP_MEMORY_SIZE))

#define MES_SIGNON  "\rWindows MMBasic Version " VERSION "\r\n"\
					"Copyright " YEAR " Geoff Graham\r\n"\
					"Copyright " YEAR2 " Peter Mather\r\n\r\n"