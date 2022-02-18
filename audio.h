/***********************************************************************************************************************
MMBasic for Windows

audio.h

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

#include <stdint.h>

/**********************************************************************************
 the C language function associated with commands, functions or operators should be
 declared here
**********************************************************************************/
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
void cmd_play(void);
//    void cmd_tts(void);
void StopAudio(void);
void audioInterrupt(void);
void CheckAudio(void);
extern volatile int vol_left, vol_right;
#endif




/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE
{ (unsigned char *)"Play", T_CMD, 0, cmd_play	    },
//	{ "TTS",        	T_CMD,				0, cmd_tts	    },
#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE
// the format is:
//    TEXT      	TYPE                P  FUNCTION TO CALL
// where type is T_NA, T_FUN, T_FNA or T_OPER augmented by the types T_STR and/or T_NBR
// and P is the precedence (which is only used for operators)

#endif
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
// General definitions used by other modules

#ifndef AUDIO_HEADER
#define AUDIO_HEADER
#define MAXSOUNDS 4
typedef enum { P_NOTHING, P_PAUSE_TONE, P_TONE, P_PAUSE_SOUND, P_SOUND, P_WAV, P_PAUSE_WAV, P_FLAC, P_MP3, P_MOD, P_PAUSE_MOD, P_PAUSE_FLAC, P_TTS, P_DAC, P_PAUSE_MP3 } e_CurrentlyPlaying;
extern volatile e_CurrentlyPlaying CurrentlyPlaying;
extern unsigned char* WAVInterrupt;
extern volatile int WAVcomplete;
extern int WAV_fnbr;
extern int PWM_FREQ;
extern char* sbuff1, * sbuff2;
extern volatile uint64_t bcount[3];
extern volatile int wav_filesize;                                    // head and tail of the ring buffer for com1
extern int trackplaying, trackstoplay;
extern "C" void checkWAVinput(void);
extern volatile uint64_t SoundPlay;
#define WAV_BUFFER_SIZE 65536
extern const unsigned short SineTable[4096];
extern unsigned short nulltable[4096];
extern volatile uint64_t bcount[3];
extern volatile float PhaseM_left, PhaseM_right;
extern volatile unsigned char PWM_count;
extern uint16_t* playbuff;
extern volatile uint64_t bcount[3];
extern volatile int sound_v_left[MAXSOUNDS];
extern volatile int sound_v_right[MAXSOUNDS];
extern volatile float sound_PhaseAC_left[MAXSOUNDS], sound_PhaseAC_right[MAXSOUNDS];
extern volatile float sound_PhaseM_left[MAXSOUNDS], sound_PhaseM_right[MAXSOUNDS];
extern volatile unsigned short* sound_mode_left[MAXSOUNDS];
extern volatile unsigned short* sound_mode_right[MAXSOUNDS];
extern const unsigned short whitenoise[2];
extern const int mapping[101];
extern volatile int ppos;                                                       // playing position for PLAY WAV
extern volatile float PhaseAC_left, PhaseAC_right;
extern volatile int swingbuf, nextbuf, playreadcomplete;
extern volatile int mono;
extern volatile int audiorepeat;
extern "C" void CloseAudio(void);
typedef struct sa_flist {
    char fn[255];
} a_flist;

#endif
#endif

