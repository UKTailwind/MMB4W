/***********************************************************************************************************************
MMBasic for Windows

FileIO.h

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
void cmd_chdir(void);
void cmd_files(void);
void cmd_load(void);
void cmd_save(void);
void fun_cwd(void);
void fun_edit(void);
void cmd_open(void);
void cmd_close(void);
void cmd_seek(void);
void fun_eof(void);
void fun_inputstr(void);
void fun_loc(void);
void fun_lof(void);
void fun_port(void);
void cmd_copy(void);
void cmd_name(void);
void fun_dir(void);
void cmd_mkdir(void);
void cmd_rmdir(void);
void cmd_kill(void);
void cmd_newedit(void);
void cmd_system(void);
void cmd_telnet(void);
void cmd_tcp(void);
void fun_getip(void);
#endif
/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE

{ (unsigned char*)"Open", T_CMD, 0, cmd_open		},
{ (unsigned char*)"Close",		T_CMD,				0, cmd_close },
{ (unsigned char*)"Load", T_CMD, 0, cmd_load	},
{ (unsigned char*)"Save", T_CMD, 0, cmd_save },
{ (unsigned char*)"Files", T_CMD, 0, cmd_files },
{ (unsigned char*)"OldEdit",   T_CMD,              0, cmd_edit },
{ (unsigned char*)"Chdir",		T_CMD,				0, cmd_chdir },
{ (unsigned char*)"Seek",		T_CMD,				0, cmd_seek },
{ (unsigned char*)"Copy",		T_CMD,				0, cmd_copy },
{ (unsigned char*)"Rename",	T_CMD,				0, cmd_name },
{ (unsigned char*)"Mkdir",		T_CMD,				0, cmd_mkdir },
{ (unsigned char*)"Rmdir",		T_CMD,				0, cmd_rmdir },
{ (unsigned char*)"Kill",		T_CMD,				0, cmd_kill },
{ (unsigned char*)"Edit",   T_CMD,              0, cmd_newedit },
{ (unsigned char*)"System",   T_CMD,              0, cmd_system },
{ (unsigned char*)"TCP",   T_CMD,              0, cmd_tcp },

#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE
{ (unsigned char*)"Cwd$",		T_FNA | T_STR,		0, fun_cwd },
{ (unsigned char*)"Eof(",   T_FUN | T_INT,      0, fun_eof },
{ (unsigned char*)"Input$(",	T_FUN | T_STR,		0, fun_inputstr },
{ (unsigned char*)"Loc(",   T_FUN | T_INT,      0, fun_loc },
{ (unsigned char*)"Lof(",   T_FUN | T_INT,      0, fun_lof },
{ (unsigned char*)"Dir$(",		T_FUN | T_STR,		0, fun_dir },
{ (unsigned char*)"ComPort(",   T_FUN | T_INT,      0, fun_port },
{ (unsigned char*)"GetIP$(",		T_FUN | T_STR,		0, fun_getip },
#endif
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
#ifndef FILEIO_H
#define FILEIO_H
#include <stdio.h>
#define MAXKEYLEN 64
#define DISPLAY_LANDSCAPE   (Option.DISPLAY_ORIENTATION & 1)
#define NO_KEYBOARD             0
#define CONFIG_US		1
#define CONFIG_FR		2
#define CONFIG_DE		3
#define CONFIG_IT		4
#define CONFIG_BE		5
#define CONFIG_UK		6
#define CONFIG_ES		7
#define CONFIG_SW		8
struct option_s {
    unsigned int magic;
    char Autorun;
    char Tab;
    char Invert;
    char Listcase; //4
  //
    int Height;
    int Width;
    int vres;
    int hres;
    int pixelnum;
    int mode;
    int  PIN;
    int  ColourCode;
    int DefaultFC, DefaultBC;      // the default colours
    char defaultpath[STRINGSIZE];
    short RepeatStart;
    short RepeatRate;
    unsigned char F5key[MAXKEYLEN];
    unsigned char F6key[MAXKEYLEN];
    unsigned char F7key[MAXKEYLEN];
    unsigned char F8key[MAXKEYLEN];
    unsigned char F9key[MAXKEYLEN];
    unsigned char F10key[MAXKEYLEN];
    unsigned char F11key[MAXKEYLEN];
    unsigned char F12key[MAXKEYLEN];
    unsigned char DefaultFont;
    unsigned char KeyboardConfig;
    bool fullscreen;
    char lastfilename[STRINGSIZE];
    char searchpath[STRINGSIZE];
} ;
extern "C" void ResetOptions(void);
extern struct option_s Option;
union uFileTable {
    unsigned int com;
    FILE* fptr;
};
extern "C" int MMfgetc(int fnbr);
extern "C" unsigned char MMfputc(unsigned char c, int fnbr);
extern "C" void MMfputs(unsigned char* p, int filenbr);
extern "C" void CloseAllFiles(void);
extern "C" void FileClose(int fnbr);
extern "C" int FileLoadProgram(unsigned char* fname, int mode);
extern "C" int FindFreeFileNbr(void);
extern "C" int BasicFileOpen(char* fname, int fnbr, char* mode);
extern "C" void SaveOptions(void);
extern "C" void LoadOptions(void);
extern "C" int32_t MMfeof(int32_t fnbr);
extern "C" void FilePutStr(int count, char* c, int fnbr);
extern "C" char FileGetChar(int fnbr);
extern "C" int existsfile(char* fname);
extern "C" void ForceFileClose(int fnbr);
extern "C" char* MMgetcwd(void);
extern "C" void tidyfilename(char* p, char* path, char* filename, int search);
extern "C" int64_t filesize(char* fname);
extern "C" int existsfile(char* fname);
extern "C" void fullfilename(char* infile, char* outfile, const char * extension);
extern "C" bool dirExists(const char* dirName_in);
extern "C" void tidypath(char* p, char* qq);
extern char lastfileedited[STRINGSIZE];
extern union uFileTable FileTable[MAXOPENFILES + 1];
extern int telnetopen , tcpopen ;

#endif
#endif
