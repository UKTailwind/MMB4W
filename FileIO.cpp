/***********************************************************************************************************************
MMBasic for Windows

FileIO.cpp

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
#include "olcPixelGameEngine.h"
#include "MainThread.h"
#include "MMBasic_Includes.h"
#include <stdlib.h>									// standard library functions
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <shlobj.h>
#include <tchar.h>
#include <strsafe.h>
#include "picojpeg.h"
#include "upng.h"
#include "Shlwapi.h"

extern "C" uint8_t BMP_bDecode(int x, int y, int fnbr);
void tidypath(char* p, char* qq);
int dirflags;
union uFileTable FileTable[MAXOPENFILES + 1]={0};
int OptionFileErrorAbort = true;
static uint32_t g_nInFileSize;
static uint32_t g_nInFileOfs;
static int jpgfnbr;
// 8*8*4 bytes * 3 = 768
int16_t* gCoeffBuf;
int system_active = 0;
FILE* system_file;

// 8*8*4 bytes * 3 = 768
uint8_t* gMCUBufR;
uint8_t* gMCUBufG;
uint8_t* gMCUBufB;

// 256 bytes
int16_t* gQuant0;
int16_t* gQuant1;
uint8_t* gHuffVal2;
uint8_t* gHuffVal3;
uint8_t* gInBuf;
#define MAXFILES 2000
typedef struct ss_flist {
    char fn[STRINGSIZE];
    int fs; //file size
    uint64_t fd; //file date
} s_flist;
typedef struct sa_dlist {
    char from[STRINGSIZE];
    char to[STRINGSIZE];
} a_dlist;
a_dlist* dlist;

int nDefines;
int LineCount = 0;
extern int opensocket;
extern int client_fd;
int udpopen = 0,tcpopen = 0;
SOCKET udpsockfd, tcpsockfd;
;
struct sockaddr_in si_me, si_other, tcp_other;

struct option_s Option;
extern "C" void ResetOptions(void) {
    CHAR my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
    strcat(my_documents, "\\");
    memset((void*)&Option.magic, 0, sizeof(struct option_s));
    Option.Tab = 2;
    Option.magic = MagicKey;
    Option.DefaultFont = (2<<4) | 1;
    gui_font_width = FontTable[Option.DefaultFont >> 4][0] * (Option.DefaultFont & 0b1111);
    gui_font_height = FontTable[Option.DefaultFont >> 4][1] * (Option.DefaultFont & 0b1111);
    Option.mode = 9;
    Option.hres = xres[Option.mode];
    Option.vres = yres[Option.mode];
    Option.pixelnum = pixeldensity[Option.mode];
    Option.Height = Option.vres / gui_font_height;
    Option.Width = Option.hres / gui_font_width;
    Option.DefaultFC = M_WHITE;
    Option.DefaultBC = M_BLACK;
    Option.KeyboardConfig = CONFIG_UK;
    Option.RepeatStart = 600;
    Option.RepeatRate = 150;
    Option.fullscreen = false;
    Option.ColourCode = true;
    strcpy(Option.defaultpath, (const char *)my_documents);
}
int32_t ErrorThrow(int32_t e) {
    MMerrno = e;
    MMErrMsg = (char*)strerror(e);
    if (e > 0 && e < 41 && OptionFileErrorAbort) error((char*)strerror(e));
    errno = 0;
    return e;
}


int32_t ErrorCheck(void) {
    int32_t e;
    e = errno;
    errno = 0;
    if (e < 1 || e > 41) return e;
    return ErrorThrow(e);
}
extern "C" char FileGetChar(int fnbr) {
    char ch;
    if (fread(&ch, 1, 1, FileTable[fnbr].fptr) != 1) ch = -1;
    return ch;
}
extern "C" int existsfile(char* fname) {
    DWORD ftyp = GetFileAttributesA(fname);
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;  //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return false;   // this is a directory!

    return true;    // this is not a directory!
}
extern "C" int64_t filesize(char* fname) {
    char filename[STRINGSIZE] = { 0 };
    char path[STRINGSIZE] = { 0 };
    char q[STRINGSIZE] = { 0 };
    strcpy(q, fname);
    tidyfilename(q, path, filename,0);
    strcpy(q, path);
    strcat(q, filename);
    WIN32_FIND_DATAA fdFile;
    HANDLE hFind = NULL;
     if ((hFind = FindFirstFileA((LPCSTR)q, &fdFile)) == INVALID_HANDLE_VALUE)return  -1;
    FindClose(hFind);
    return  ((int64_t)fdFile.nFileSizeHigh << 32) | (int64_t)fdFile.nFileSizeLow;
}
// fname must be a standard C style string (not the MMBasic style)
void MMfopen(char* fname, char* mode, int32_t fnbr) {
    if (fnbr < 1 || fnbr > MAXOPENFILES) error((char *)"Invalid file number");
//    if ((uint32_t) FileTable[fnbr].fptr != 0) error((char*)"File number is already open");
    MMerrno = errno = 0;

    // random writing is not allowed when a file is opened for append so open it first for read+update
    // and if that does not work open it for writing+update.  This has the same effect as opening for 
    // append+update but will allow writing
    if (*mode == 'x') {
        FileTable[fnbr].fptr = fopen(fname, "rb+");
        if (FileTable[fnbr].fptr == NULL) {
            errno = 0;
            FileTable[fnbr].fptr = fopen(fname, "wb+");
        }
        fseek(FileTable[fnbr].fptr, 0, SEEK_END);
    }
    else
        FileTable[fnbr].fptr = fopen(fname, mode);


    if (FileTable[fnbr].fptr == NULL) {
        FileTable[fnbr].fptr = 0;
        error((char *)"Failed to open file $", fname);
    }
}
// this performs the basic duties of opening a file, all file opens in MMBasic should use this
// it will open the file, set the FileTable[] entry and populate the file descriptor
// it returns with true if successful or false if an error
extern "C" int BasicFileOpen(char* fname, int fnbr, char *mode) {
    if (fnbr < 1 || fnbr > MAXOPENFILES) error((char *)"File number");
    if (FileTable[fnbr].com != 0) error((char *)"File number already open");
    // if we are writing check the write protect pin (negative pin number means that low = write protect)
//    FileTable[fnbr].fptr = (FILE *)GetMemory(sizeof(FILE));              // allocate the file descriptor
    MMfopen(fname, mode, fnbr);
    return true;
}
extern "C" void FilePutStr(int count, char* c, int fnbr) {
    if (fwrite(c, 1, count, FileTable[fnbr].fptr) != count)error((char*)"File write");
}

extern "C" int MMfgetc(int fnbr) {
    int ch=0;
    if(fnbr == 0) return MMgetchar();                                 // accessing the console
    if(fnbr < 1 || fnbr > MAXOPENFILES) error((char *)"File number");
    if(FileTable[fnbr].com == 0) error((char*)"File number is not open");
    if (FileTable[fnbr].com > MAXCOMPORTS) {
        if (fread(&ch, 1, 1, FileTable[fnbr].fptr) != 1) ch = -1;
    }  else ch = SerialGetchar(FileTable[fnbr].com);                        // get the char from the serial port
    return ch;
}
// send a character to a file or the console
// if fnbr == 0 then send the char to the console
// otherwise the COM port or file opened as #fnbr
extern "C" unsigned char MMfputc(unsigned char c, int fnbr) {
    if (fnbr == 99999 || OptionConsoleSerial) {
        char b[2] = { 0 };
        b[0] = c;
        std::cout << b;
        return c;
    }
    if (fnbr == 0) {
        return MMputchar(c);
    }// accessing the console
    if(fnbr < 1 || fnbr > MAXOPENFILES) error((char *)"File number");
    if(FileTable[fnbr].com == 0) error((char*)"File number is not open");
    if (FileTable[fnbr].com > MAXCOMPORTS) {
        if (fwrite(&c, 1, 1, FileTable[fnbr].fptr) != 1)error((char*)"File write");
    }
    else c = SerialPutchar(FileTable[fnbr].com, c);                   // send the char to the serial port
    return c;
}

extern "C" void MMfputs(unsigned char* p, int filenbr) {
    int i;
    i = *p++;
    if (filenbr == 99999 || OptionConsoleSerial) {
        while (i--)MMfputc(*p++,99999);
        return;
    }
    if (filenbr == 0) {
        while (i--) {
            MMputchar(*p++);
        }
        return;
    }
    if (filenbr < 1 || filenbr > MAXOPENFILES) error((char*)"File number");
    if (FileTable[filenbr].com == 0) error((char*)"File number is not open");
    if (FileTable[filenbr].com > MAXCOMPORTS) {
        if (fwrite(p, 1, i, FileTable[filenbr].fptr) != i)error((char*)"File write");
    }
    else {
        DWORD dNoOfBytesWritten = 0;
        //    MMputchar(c);
        WriteFile(hComm[FileTable[filenbr].com],        // Handle to the Serial port
            (char*)p,     // Data to be written to the port
            i,  //No of bytes to write
            &dNoOfBytesWritten, //Bytes written
            NULL);
        if (dNoOfBytesWritten != i)error((char*)"Comm write");
    }
    
}

void MMfclose(int32_t fnbr) {
    if (fnbr < 1 || fnbr > MAXOPENFILES) error((char *)"Invalid file number");
    if (FileTable[fnbr].fptr == 0) error((char*)"File number is not open");
    errno = 0;
    if (FileTable[fnbr].fptr != NULL)fclose(FileTable[fnbr].fptr);
    FileTable[fnbr].fptr = NULL;
    FreeMemory((unsigned char*)FileTable[fnbr].fptr);
    ErrorCheck();
}

extern "C" void FileClose(int fnbr) {
    if (fnbr < 1 || fnbr > MAXOPENFILES) return;
    if (FileTable[fnbr].fptr == NULL) return;
    errno = 0;
    fclose(FileTable[fnbr].fptr);
    FileTable[fnbr].fptr = NULL;
    FreeMemory((unsigned char*)FileTable[fnbr].fptr);
    ErrorCheck();
}
// load a file into program memory

// finds the first available free file number.  Throws an error if no free file numbers
extern "C" int FindFreeFileNbr(void) {
    int i;
    for (i = 1; i <= MAXOPENFILES; i++)
        if (FileTable[i].com == 0) return i;
    error((char *)"Too many files open");
    return 0;
}
extern "C" int32_t MMfeof(int32_t fnbr) {
    int32_t i=0, c;
    if (fnbr < 0 || fnbr > MAXOPENFILES) error((char *)"Invalid file number");
    if (fnbr == 0) return 0;
    if (FileTable[fnbr].fptr== NULL) error((char*)"File number % is not open", fnbr);
    if ((uint32_t)FileTable[fnbr].fptr > MAXCOMPORTS) {
        errno = 0;
        fflush(FileTable[fnbr].fptr);
        c = fgetc(FileTable[fnbr].fptr);										// the Watcom compiler will only set eof after it has tried to read beyond the end of file
        i = (feof(FileTable[fnbr].fptr) != 0) ? -1 : 0;
        ungetc(c, FileTable[fnbr].fptr);										// undo the Watcom bug fix
        ErrorCheck();
    }
/*    else if (OpenFileTable[fnbr] == -1) {
        j = SocketBufHead - SocketBufTail;
        if (j < 0) j += SOCKETBUFSIZE;
        i = (j != 0) ? 0 : 1;
    }*/
    else return SerialRxStatus(FileTable[fnbr].com) == 0;
    return i;
}
int cmpstr(char* s1, char* s2)
{
    unsigned char* p1 = (unsigned char*)s1;
    unsigned char* p2 = (unsigned char*)s2;
    unsigned char c1, c2;

    if (p1 == p2)
        return 0;

    do
    {
        c1 = tolower(*p1++);
        c2 = tolower(*p2++);
        if (c1 == '\0') return 0;
    } while (c1 == c2);

    return c1 - c2;
}
int massage(char* buff) {
    int i = nDefines;
    while (i--) {
        char* p = dlist[i].from;
        while (*p) {
            *p = toupper(*p);
            p++;
        }
        p = dlist[i].to;
        while (*p) {
            *p = toupper(*p);
            p++;
        }
        STR_REPLACE(buff, dlist[i].from, dlist[i].to);
    }
    STR_REPLACE(buff, "=<", "<=");
    STR_REPLACE(buff, "=>", ">=");
    STR_REPLACE(buff, " ,", ",");
    STR_REPLACE(buff, ", ", ",");
    STR_REPLACE(buff, " *", "*");
    STR_REPLACE(buff, "* ", "*");
    STR_REPLACE(buff, "- ", "-");
    STR_REPLACE(buff, " /", "/");
    STR_REPLACE(buff, "/ ", "/");
    STR_REPLACE(buff, "= ", "=");
    STR_REPLACE(buff, "+ ", "+");
    STR_REPLACE(buff, " )", ")");
    STR_REPLACE(buff, ") ", ")");
    STR_REPLACE(buff, "( ", "(");
    STR_REPLACE(buff, "> ", ">");
    STR_REPLACE(buff, "< ", "<");
    STR_REPLACE(buff, " '", "'");
    return strlen(buff);
}
void importfile(unsigned char* path, unsigned char* filename, unsigned char** p, uint32_t buf, int convertdebug) {
    int fnbr;
    char buff[256];
    char qq[STRINGSIZE] = { 0 };
    char num[10];
    int importlines = 0;
    int ignore = 0;
    char* fname, * sbuff, * op, * ip;
    int c, f, slen, data;
    fnbr = FindFreeFileNbr();
    char* q;
//    MMPrintString((char*)"path = "); MMPrintString((char*)path); PRet();
//    MMPrintString((char*)"file = "); MMPrintString((char*)filename); PRet();

    if ((q = strchr((char *)filename, 34)) == 0) error((char *)"Syntax");
    q++;
    if ((q = strchr(q, 34)) == 0) error((char *)"Syntax");
    fname = (char *)getCstring(filename);
    strcpy(qq, fname);
    for (int i = 0; i < (int)strlen(qq); i++) if(qq[i] == '/')qq[i] = '\\';
    if (!(qq[1] == ':' || qq[0] == '\\')) { //filename is relative path to "path"
        char q[STRINGSIZE] = { 0 };
        strcpy(q, (char *)path);
        if (path[strlen(q) - 1] != '\\')strcat(q, "\\");
        strcat(q, qq);
        PathCanonicalizeA(qq, q);
    }
    f = strlen(qq);
    if (strchr((char*)qq, '.') == NULL) strcat((char*)qq, (char *)"INC");
    q = &qq[strlen(qq) - 4];
    if (strcasecmp(q, ".inc") != 0)error((char *)"must be a .inc file");
    BasicFileOpen(qq, fnbr, (char*)"rb");
    while (!MMfeof(fnbr)) {
        int toggle = 0, len = 0;// while waiting for the end of file
        sbuff = buff;
        if (((uint32_t)*p - buf) >= EDIT_BUFFER_SIZE - 256 * 6) error((char *)"Not enough memory");
        memset(buff, 0, 256);
        MMgetline(fnbr, (char*)buff);									    // get the input line
        data = 0;
        importlines++;
        LineCount++;
        len = strlen(buff);
        toggle = 0;
        for (c = 0; c < (int)strlen(buff); c++) {
            if (buff[c] == TAB) buff[c] = ' ';
        }
        while (*sbuff == ' ') {
            sbuff++;
            len--;
        }
        if (ignore && sbuff[0] != '#')*sbuff = '\'';
        if (strncasecmp(sbuff, "rem ", 4) == 0 || (len == 3 && strncasecmp(sbuff, "rem", 3) == 0)) {
            sbuff += 2;
            *sbuff = '\'';
            continue;
        }
        if (strncasecmp(sbuff, "data ", 5) == 0)data = 1;
        slen = len;
        op = sbuff;
        ip = sbuff;
        while (*ip) {
            if (*ip == 34) {
                if (toggle == 0)toggle = 1;
                else toggle = 0;
            }
            if (!toggle && (*ip == ' ' || *ip == ':')) {
                *op++ = *ip++; //copy the first space
                while (*ip == ' ') {
                    ip++;
                    len--;
                }
            }
            else *op++ = *ip++;
        }
        slen = len;
        if (!(toupper(sbuff[0]) == 'R' && toupper(sbuff[1]) == 'U' && toupper(sbuff[2]) == 'N' && (strlen(sbuff) == 3 || sbuff[3] == ' '))) {
            toggle = 0;
            for (c = 0; c < slen; c++) {
                if (!(toggle || data))sbuff[c] = toupper(sbuff[c]);
                if (sbuff[c] == 34) {
                    if (toggle == 0)toggle = 1;
                    else toggle = 0;
                }
            }
        }
        toggle = 0;
        for (c = 0; c < slen; c++) {
            if (sbuff[c] == 34) {
                if (toggle == 0)toggle = 1;
                else toggle = 0;
            }
            if (!toggle && sbuff[c] == 39 && len == slen) {
                len = c;//get rid of comments
                break;
            }
        }
        if (sbuff[0] == '#') {
            unsigned char* mp = checkstring((unsigned char*)&sbuff[1], (unsigned char *)"DEFINE");
            if (mp) {
                getargs(&mp, 3, (unsigned char*)",");
                if (nDefines >= MAXDEFINES) {
                    FreeMemorySafe((void**)&buf);
                    FreeMemorySafe((void**)&dlist);
                    error((char *)"Too many #DEFINE statements");
                }
                strcpy(dlist[nDefines].from, (char *)getCstring(argv[0]));
                strcpy(dlist[nDefines].to, (char*)getCstring(argv[2]));
                nDefines++;
            }
            else {
                if (cmpstr((char*)"COMMENT END", &sbuff[1]) == 0)ignore = 0;
                if (cmpstr((char*)"COMMENT START", &sbuff[1]) == 0)ignore = 1;
                if (cmpstr((char*)"MMDEBUG ON", &sbuff[1]) == 0)convertdebug = 0;
                if (cmpstr((char*)"MMDEBUG OFF", &sbuff[1]) == 0)convertdebug = 1;
                if (cmpstr((char*)"INCLUDE ", &sbuff[1]) == 0) {
                    error((char *)"Can't import from an import");
                }
            }
        }
        else {
            if (toggle)sbuff[len++] = 34;
            sbuff[len++] = 39;
            sbuff[len++] = '|';
            memcpy(&sbuff[len], fname, f);
            len += strlen(fname);
            sbuff[len++] = ',';
            IntToStr(num, importlines, 10);
            strcpy(&sbuff[len], num);
            len += strlen(num);
            if (len > 254) {
                FreeMemorySafe((void**)&buf);
                FreeMemorySafe((void**)&dlist);
                error((char *)"Line too long");
            }
            sbuff[len] = 0;
            len = massage(sbuff); //can't risk crushing lines with a quote in them
            if ((sbuff[0] != 39) || (sbuff[0] == 39 && sbuff[1] == 39)) {
/*******                if (Option.profile) {
                    while (strlen(sbuff) < 9) {
                        strcat(sbuff, " ");
                        len++;
                    }
                }*/
                memcpy(*p, sbuff, len);
                *p += len;
                **p = '\n';
                *p += 1;
            }
        }
    }
    FileClose(fnbr);
    return;
}
// load a file into program memory
int FileLoadProgram(unsigned char* fn, int mode) {
    int fnbr, size = 0;
    char* p, * op, * ip, * buf, * sbuff, name[STRINGSIZE] = { 0 }, buff[STRINGSIZE];
    char path[STRINGSIZE] = { 0 };
    char pp[STRINGSIZE] = { 0 };
    char num[10];
    int c;
    int convertdebug = 1;
    int ignore = 0;
    nDefines = 0;
    LineCount = 0;
    int importlines = 0, data;
    CloseAllFiles();
    ClearProgram();                                                 // clear any leftovers from the previous program
    fnbr = FindFreeFileNbr();
    if (mode) {
        strcpy(buff, (char*)fn);
        strcpy(path, buff);
        char* e = &path[strlen(path) - 1];
        while (*e != '\\') {
            *e = 0;
            e--;
        }
    }
    else {
        p = (char*)getCstring(fn);
        strcpy(buff, p);
        tidyfilename(buff, path, name,0);
        strcpy(buff, path);
        strcat(buff, name);
        if (strchr(buff, '.') == NULL) strcat(buff, ".BAS");
        if (!existsfile(buff)) {
            strcpy(buff, p);
            tidyfilename(buff, path, name, 1);
            strcpy(buff, path);
            strcat(buff, name);
            if (strchr(buff, '.') == NULL) strcat(buff, ".BAS");
            if (!existsfile(buff))error((char*)"File not found");
        }
    }
//    tidyfilename(buff, path, name,0);
//    strcpy(buff, path);
//    strcat(buff, name);
//    if (strchr(buff, '.') == NULL) strcat(buff, ".BAS");
    if (!BasicFileOpen(buff, fnbr, (char*)"rb")) return false;
    strcpy(lastfileedited, buff);
    strcpy(Option.lastfilename, buff);
    SaveOptions();
    strcpy(pp, name);
    p = buf = (char *)GetMemory(EDIT_BUFFER_SIZE);
    dlist = (a_dlist *)GetMemory(sizeof(a_dlist) * MAXDEFINES);

    while (!MMfeof(fnbr)) {                                     // while waiting for the end of file
        int toggle = 0, len = 0, slen;// while waiting for the end of file
        sbuff = buff;
        if ((p - buf) >= EDIT_BUFFER_SIZE - 256 * 6) error((char *)"Not enough memory");
        memset(buff, 0, 256);
        MMgetline(fnbr, (char*)buff);									    // get the input line
        data = 0;
        importlines++;
        LineCount++;
        len = strlen(buff);
        toggle = 0;
        for (c = 0; c < (int)strlen(buff); c++) {
            if (buff[c] == TAB) buff[c] = ' ';
        }
        while (sbuff[0] == ' ') { //strip leading spaces
            sbuff++;
            len--;
        }
        if (ignore && sbuff[0] != '#')*sbuff = '\'';
        if (strncasecmp(sbuff, "rem ", 4) == 0 || (len == 3 && strncasecmp(sbuff, "rem", 3) == 0)) {
            sbuff += 2;
            *sbuff = '\'';
            continue;
        }
        if (strncasecmp(sbuff, "mmdebug ", 7) == 0 && convertdebug == 1) {
            sbuff += 6;
            *sbuff = '\'';
            continue;
        }
        if (strncasecmp(sbuff, "data ", 5) == 0)data = 1;
        slen = len;
        op = sbuff;
        ip = sbuff;
        while (*ip) {
            if (*ip == 34) {
                if (toggle == 0)toggle = 1;
                else toggle = 0;
            }
            if (!toggle && (*ip == ' ' || *ip == ':')) {
                *op++ = *ip++; //copy the first space
                while (*ip == ' ') {
                    ip++;
                    len--;
                }
            }
            else *op++ = *ip++;
        }
        slen = len;
        if (sbuff[0] == '#') {
            unsigned char* mp = checkstring((unsigned char*)&sbuff[1], (unsigned char*)"DEFINE");
            if (mp) {
                getargs(&mp, 3, (unsigned char*)",");
                if (nDefines >= MAXDEFINES) {
                    FreeMemorySafe((void**)&buf);
                    FreeMemorySafe((void**)&dlist);
                    error((char *)"Too many #DEFINE statements");
                }
                strcpy(dlist[nDefines].from, (char*)getCstring(argv[0]));
                strcpy(dlist[nDefines].to, (char*)getCstring(argv[2]));
                nDefines++;
            }
            else {
                if (cmpstr((char*)"COMMENT END", &sbuff[1]) == 0)ignore = 0;
                if (cmpstr((char*)"COMMENT START", &sbuff[1]) == 0)ignore = 1;
                if (cmpstr((char*)"MMDEBUG ON", &sbuff[1]) == 0)convertdebug = 0;
                if (cmpstr((char*)"MMDEBUG OFF", &sbuff[1]) == 0)convertdebug = 1;
                if (cmpstr((char*)"INCLUDE", &sbuff[1]) == 0) {
                    importfile((unsigned char*)path, (unsigned char*)&sbuff[8], (unsigned char**)&p, (uint32_t)buf, convertdebug);
                }
            }
        }
        else {
            if (!(toupper(sbuff[0]) == 'R' && toupper(sbuff[1]) == 'U' && toupper(sbuff[2]) == 'N' && (strlen(sbuff) == 3 || sbuff[3] == ' '))) {
                toggle = 0;
                for (c = 0; c < slen; c++) {
                    if (!(toggle || data))sbuff[c] = toupper(sbuff[c]);
                    if (sbuff[c] == 34) {
                        if (toggle == 0)toggle = 1;
                        else toggle = 0;
                    }
                }
            }
            toggle = 0;
            for (c = 0; c < slen; c++) {
                if (sbuff[c] == 34) {
                    if (toggle == 0)toggle = 1;
                    else toggle = 0;
                }
                if (!toggle && sbuff[c] == 39 && len == slen) {
                    len = c;//get rid of comments
                    break;
                }
            }
            if (toggle)sbuff[len++] = 34;
            sbuff[len++] = 39;
            sbuff[len++] = '|';
            IntToStr(num, importlines, 10);
            strcpy(&sbuff[len], num);
            len += strlen(num);
            if (len > 254) {
                FreeMemorySafe((void**)&buf);
                FreeMemorySafe((void**)&dlist);
                error((char *)"Line too long");
            }
            sbuff[len] = 0;
            len = massage(sbuff); //can't risk crushing lines with a quote in them
            if ((sbuff[0] != 39) || (sbuff[0] == 39 && sbuff[1] == 39)) {
/*******                if (Option.profile) {
                    while (strlen(sbuff) < 9) {
                        strcat(sbuff, " ");
                        len++;
                    }
                }*/
                memcpy(p, sbuff, len);
                p += len;
                *p++ = '\n';
            }
        }

    }
    *p = 0;                                                         // terminate the string in RAM
    FileClose(fnbr);
    int load = 0;
    SaveProgramToMemory((unsigned char*)buf, false);
    FreeMemorySafe((void**)&buf);
    FreeMemorySafe((void**)&dlist);
    return true;
}
void LoadImage(unsigned char* p) {
    int fnbr;
    int xOrigin, yOrigin;
    char fname[STRINGSIZE] = { 0 };

    // get the command line arguments
    getargs(&p, 5, (unsigned char*)",");                                            // this MUST be the first executable line in the function
    if (argc == 0) error((char*)"Argument count");

    p = getCstring(argv[0]);                                        // get the file name
    int maxH = PageTable[WritePage].ymax;
    fullfilename((char*)p, fname, ".BMP");
    xOrigin = yOrigin = 0;
    if (argc >= 3) xOrigin = (int)getinteger(argv[2]);                    // get the x origin (optional) argument
    if (argc == 5) {
        yOrigin = (int)getinteger(argv[4]);                    // get the y origin (optional) argument
        if (optiony) yOrigin = maxH - 1 - yOrigin;
    }

    // open the file
    fnbr = FindFreeFileNbr();
    if (!BasicFileOpen(fname, fnbr, (char*)"rb")) return;
    int savey = optiony;
    optiony = 0;
    BMP_bDecode(xOrigin, yOrigin, fnbr);
    optiony = savey;
    FileClose(fnbr);
}

unsigned char pjpeg_need_bytes_callback(unsigned char* pBuf, unsigned char buf_size, unsigned char* pBytes_actually_read, void* pCallback_data)
{
    uint32_t n, n_read;
    pCallback_data;

    n = std::min(g_nInFileSize - g_nInFileOfs, (unsigned int)buf_size);
    n_read=fread(pBuf, 1, n, FileTable[jpgfnbr].fptr);
    if (n != n_read)
        return PJPG_STREAM_READ_ERROR;
    *pBytes_actually_read = (unsigned char)(n);
    g_nInFileOfs += n;
    return 0;
}

void LoadJPGImage(unsigned char* p) {
    pjpeg_image_info_t image_info;
    int mcu_x = 0;
    int mcu_y = 0;
    uint32_t row_pitch;
    uint8_t status;
    int maxH = PageTable[WritePage].ymax;
    int maxW = PageTable[WritePage].xmax;
    gCoeffBuf = (int16_t*)GetTempMemory(8 * 8 * sizeof(int16_t));
    gMCUBufR = (uint8_t*)GetTempMemory(256);
    gMCUBufG = (uint8_t*)GetTempMemory(256);
    gMCUBufB = (uint8_t*)GetTempMemory(256);
    gQuant0 = (int16_t*)GetTempMemory(8 * 8 * sizeof(int16_t));
    gQuant1 = (int16_t*)GetTempMemory(8 * 8 * sizeof(int16_t));
    gHuffVal2 = (uint8_t*)GetTempMemory(256);
    gHuffVal3 = (uint8_t*)GetTempMemory(256);
    gInBuf = (uint8_t*)GetTempMemory(PJPG_MAX_IN_BUF_SIZE);
    g_nInFileSize = g_nInFileOfs = 0;
    char fname[STRINGSIZE] = { 0 };

    uint32_t decoded_width, decoded_height;
    int xOrigin, yOrigin;

    // get the command line arguments
    getargs(&p, 5, (unsigned char *)",");                                            // this MUST be the first executable line in the function
    if (argc == 0) error((char *)"Argument count");

    p = getCstring(argv[0]);                                        // get the file name
    fullfilename((char*)p, fname, ".JPG");
    xOrigin = yOrigin = 0;
    if (argc >= 3) xOrigin = (int)getint(argv[2], 0, HRes - 1);                    // get the x origin (optional) argument
    if (argc == 5) {
        yOrigin = (int)getint(argv[4], 0, VRes - 1);
        if (optiony) yOrigin = maxH - 1 - yOrigin;
    }
    // get the y origin (optional) argument

    // open the file
    jpgfnbr = FindFreeFileNbr();
    if (!BasicFileOpen(fname, jpgfnbr, (char*)"rb")) return;
    fseek(FileTable[jpgfnbr].fptr, 0L, SEEK_END);
    g_nInFileSize = ftell(FileTable[jpgfnbr].fptr);
    fseek(FileTable[jpgfnbr].fptr, 0L, SEEK_SET);
    status = pjpeg_decode_init(&image_info, pjpeg_need_bytes_callback, NULL, 0);
    if (status)
    {
        if (status == PJPG_UNSUPPORTED_MODE)
        {
            FileClose(jpgfnbr);
            error((char*)"Progressive JPEG files are not supported");
        }
        FileClose(jpgfnbr);
        error((char*)"pjpeg_decode_init() failed with status %", status);
    }
    decoded_width = image_info.m_width;
    decoded_height = image_info.m_height;

    row_pitch = image_info.m_MCUWidth * image_info.m_comps;
    int savey = optiony;
    optiony = 0;

    unsigned char* imageblock = (uint8_t *)GetTempMemory(image_info.m_MCUHeight * image_info.m_MCUWidth * image_info.m_comps);
    unsigned char* newblock = (uint8_t*)GetTempMemory(image_info.m_MCUHeight * image_info.m_MCUWidth * sizeof(uint32_t));
    for (; ; )
    {
        uint8_t* pDst_row = imageblock;
        int y, x;

        status = pjpeg_decode_mcu();

        if (status)
        {
            if (status != PJPG_NO_MORE_BLOCKS)
            {
                FileClose(jpgfnbr);
                optiony = savey;
                error((char*)"pjpeg_decode_mcu() failed with status %", status);
            }
            break;
        }

        if (mcu_y >= image_info.m_MCUSPerCol)
        {
            FileClose(jpgfnbr);
            optiony = savey;
            return;
        }
 
        for (y = 0; y < image_info.m_MCUHeight; y += 8)
        {
            const int by_limit = std::min(8, image_info.m_height - (mcu_y * image_info.m_MCUHeight + y));
            for (x = 0; x < image_info.m_MCUWidth; x += 8)
            {
                uint8_t* pDst_block = pDst_row + x * image_info.m_comps;
                // Compute source byte offset of the block in the decoder's MCU buffer.
                uint32_t src_ofs = (x * 8U) + (y * 16U);
                const uint8_t* pSrcR = image_info.m_pMCUBufR + src_ofs;
                const uint8_t* pSrcG = image_info.m_pMCUBufG + src_ofs;
                const uint8_t* pSrcB = image_info.m_pMCUBufB + src_ofs;

                const int bx_limit = std::min(8, image_info.m_width - (mcu_x * image_info.m_MCUWidth + x));

                {
                    int bx, by;
                    for (by = 0; by < by_limit; by++)
                    {
                        uint8_t* pDst = pDst_block;

                        for (bx = 0; bx < bx_limit; bx++)
                        {
                            pDst[2] = *pSrcR++;
                            pDst[1] = *pSrcG++;
                            pDst[0] = *pSrcB++;
                            pDst += 3;
                        }

                        pSrcR += (8 - bx_limit);
                        pSrcG += (8 - bx_limit);
                        pSrcB += (8 - bx_limit);

                        pDst_block += row_pitch;
                    }
                }
            }
            pDst_row += (row_pitch * 8);
        }
        unsigned char* s = imageblock;
        unsigned char* d = newblock;
        for (int yp = 0; yp < image_info.m_MCUHeight; yp++) {
            for (int xp = 0; xp < image_info.m_MCUWidth; xp++) {
                    *d++ = s[2];
                    *d++ = s[1];
                    *d++ = s[0];
                    *d++ = 0xFF;
                    s += 3;
             }
        }

        x = mcu_x * image_info.m_MCUWidth + xOrigin;
        y = mcu_y * image_info.m_MCUHeight + yOrigin;
        if (y < VRes && x < HRes) {
            int yend = std::min(VRes - 1, y + image_info.m_MCUHeight - 1);
            int xend = std::min(HRes - 1, x + image_info.m_MCUWidth - 1);
            if (xend < x + image_info.m_MCUWidth - 1) {
                // need to get rid of some pixels to remove artifacts
                xend = HRes - 1;
                unsigned char* s = newblock;
                unsigned char* d = newblock;
                for (int yp = 0; yp < image_info.m_MCUHeight; yp++) {
                    for (int xp = 0; xp < image_info.m_MCUWidth; xp++) {
                        if (xp < xend - x + 1) {
                            *d++ = *s++;
                            *d++ = *s++;
                            *d++ = *s++;
                            *d++ = *s++;
                        }
                        else {
                            s += 4;
                        }
                    }
                }
            }
            DrawBuffer(x, y, xend, yend, (uint32_t*)newblock);
        }
        if (y >= VRes) { //nothing useful left to process
            FileClose(jpgfnbr);
            optiony = savey;
            return;
        }
        mcu_x++;
        if (mcu_x == image_info.m_MCUSPerRow)
        {
            mcu_x = 0;
            mcu_y++;
        }
    }
    optiony = savey;
    FileClose(jpgfnbr);
}
void LoadPNG(unsigned char* p) {
    //	int fnbr;
    int xOrigin, yOrigin, w, h, transparent = 0, force = 0;
    int maxW = PageTable[WritePage].xmax;
    int maxH = PageTable[WritePage].ymax;
    upng_t* upng;
    char fname[STRINGSIZE] = { 0 };
    // get the command line arguments
    getargs(&p, 7, (unsigned char *)",");                                            // this MUST be the first executable line in the function
    if (argc == 0) error((char *)"Argument count");

    p = getCstring(argv[0]);                                        // get the file name
    fullfilename((char*)p, fname, ".PNG");

    xOrigin = yOrigin = 0;
    if (argc >= 3 && *argv[2]) xOrigin = (int)getinteger(argv[2]);                    // get the x origin (optional) argument
    if (argc >= 5 && *argv[4]) {
        yOrigin = (int)getinteger(argv[4]);                    // get the y origin (optional) argument
        if (optiony) yOrigin = maxH - 1 - yOrigin;
    }
    if (argc == 7)transparent = (int)getint(argv[6], 0, 15);
    if (transparent) {
        if (transparent > 1)force = transparent << 4;
        transparent = 4;
    }
    // open the file
    //	fnbr = FindFreeFileNbr();
    //    if(!BasicFileOpen(p, fnbr, FA_READ)) return;
    upng = upng_new_from_file(fname);
    upng_header(upng);
    w = upng_get_width(upng);
    h = upng_get_height(upng);
    if (w + xOrigin > maxW || h + yOrigin > maxH) {
        upng_free(upng);
        error((char*)"Image too large");
    }
    if (!(upng_get_format(upng) == 1 || upng_get_format(upng) == 3)) {
        upng_free(upng);
        error((char*)"Invalid format");
    }
    upng_decode(upng);
    unsigned char* rr;
    rr = (unsigned char*)upng_get_buffer(upng);
    int savey = optiony;
    optiony = 0;
    if (upng_get_format(upng) == 3) {
        DrawBuffer32(xOrigin, yOrigin, xOrigin + w - 1, yOrigin + h - 1, (char*)rr, 3 | transparent | force);
    }
    else {
        DrawBuffer32(xOrigin, yOrigin, xOrigin + w - 1, yOrigin + h - 1, (char*)rr, 2 | transparent | force);
    }
    optiony = savey;
    upng_free(upng);
    //    FileClose(fnbr);
    clearrepeat();
}

void cmd_load(void) {
    int autorun = false;
    unsigned char* p;

    p = checkstring(cmdline, (unsigned char *)"BMP");
    if (p) {
        LoadImage(p);
        return;
    }
    p = checkstring(cmdline, (unsigned char*)"JPG");
    if (p) {
        LoadJPGImage(p);
        return;
    }
    p = checkstring(cmdline, (unsigned char*)"PNG");
    if (p) {
        LoadPNG(p);
        return;
    }
    p = checkstring(cmdline, (unsigned char*)"DATA");
    if (p) {
        int fnbr;
        unsigned int nbr;
        char* pp;
        char fname[STRINGSIZE] = { 0 };
        getargs(&p, 3, (unsigned char*)",");
        if (argc != 3)error((char*)"Syntax");
        pp = (char *)getCstring(argv[0]);
        fullfilename((char*)pp, fname, ".DAT");
        uint32_t address = (GetPokeAddr(argv[2]) & 0b11111111111111111111111111111100);
        fnbr = FindFreeFileNbr();
        if (!BasicFileOpen(fname, fnbr, (char*)"rb")) return;
        fseek(FileTable[fnbr].fptr, 0L, SEEK_END);
        uint32_t size = ftell(FileTable[fnbr].fptr);
        fseek(FileTable[fnbr].fptr, 0L, SEEK_SET);
        for (uint32_t i = address; i < address + size; i++)if (!POKERANGE(i)) error((char*)"Address");
        nbr = fread((char*)address, 1, size, FileTable[fnbr].fptr);
        FileClose(fnbr);
        if (nbr != size)error((char *)"File read error");
        return;
    }
    getargs(&cmdline, 3, (unsigned char *)",");
    if (!(argc & 1) || argc == 0) error((char *)"Syntax");
    if (argc == 3) {
        if (toupper(*argv[2]) == 'R')
            autorun = true;
        else
            error((char*)"Syntax");
    }
    else if (CurrentLinePtr != NULL)
        error((char*)"Invalid in a program");

    if (!FileLoadProgram(argv[0],0)) return;
    if (autorun) {
        if (*ProgMemory != 0x01) return;                              // no program to run
        ClearRuntime();
        WatchdogSet = false;
        PrepareProgram(true);
        IgnorePIN = false;
        nextstmt = ProgMemory;
    }
}
int strcicmp(char const* a, char const* b)
{
    for (;; a++, b++) {
        int d = tolower(*a) - tolower(*b);
        if (d != 0 || !*a)
            return d;
    }
}

bool ListDirectoryContents(const char* sDir, const char *mask, int sortorder)
{
    WIN32_FIND_DATAA fdFile;
    HANDLE hFind = NULL;
    static s_flist *flist = NULL;
    int i,fcnt = 0;
    if (flist)FreeMemorySafe((void**)&flist);
    flist = (s_flist*)GetMemory(sizeof(s_flist) * MAXFILES);
    char ts[STRINGSIZE] = { 0 }, extension[8] = { 0 };
    char sPath[2048] = { 0 };
    uint64_t currentdate, currentsize;

    //Specify a file mask. *.* = We want everything!
    strcpy(sPath, sDir);
//    strcat(sPath, "\\");
    strcat(sPath, mask);
//    sprintf(sPath, "%s\\*.*", sDir);

    if ((hFind = FindFirstFileA(sPath, &fdFile)) == INVALID_HANDLE_VALUE)
    {
        error((char *)"Not found: $", (char *)sPath);
        return false;
    }

    do
    {
        //Find first file will always return "."
        //    and ".." as the first two directories.
        if (strcmp((const char *)fdFile.cFileName, ".") != 0
            && strcmp((const char*)fdFile.cFileName, "..") != 0)
        {
             if (fcnt >= MAXFILES) {
                FreeMemorySafe((void**)&flist);
                error((char *)"Too many files to list");
            }
            // add a prefix to each line so that directories will sort ahead of files
            if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                ts[0] = 'D';
                currentdate = 0xFFFFFFFF;
                fdFile.ftCreationTime.dwHighDateTime= 0xFFFFFFFF;
                fdFile.ftCreationTime.dwLowDateTime = 0xFFFFFFFF;
                memset(extension, '+', sizeof(extension));
                extension[sizeof(extension) - 1] = 0;
            }
            else {
                ts[0] = 'F';
                currentdate = ((uint64_t)fdFile.ftCreationTime.dwHighDateTime << 32) | fdFile.ftCreationTime.dwLowDateTime;
                if (fdFile.cFileName[strlen(fdFile.cFileName) - 1] == '.') strcpy(extension, &fdFile.cFileName[strlen(fdFile.cFileName) - 1]);
                else if (fdFile.cFileName[strlen(fdFile.cFileName) - 2] == '.') strcpy(extension, &fdFile.cFileName[strlen(fdFile.cFileName) - 2]);
                else if (fdFile.cFileName[strlen(fdFile.cFileName) - 3] == '.') strcpy(extension, &fdFile.cFileName[strlen(fdFile.cFileName) - 3]);
                else if (fdFile.cFileName[strlen(fdFile.cFileName) - 4] == '.') strcpy(extension, &fdFile.cFileName[strlen(fdFile.cFileName) - 4]);
                else if (fdFile.cFileName[strlen(fdFile.cFileName) - 5] == '.') strcpy(extension, &fdFile.cFileName[strlen(fdFile.cFileName) - 5]);
                else {
                    memset(extension, '.', sizeof(extension));
                    extension[sizeof(extension) - 1] = 0;
                }
            }
            currentsize = fdFile.nFileSizeLow;
            // and concatenate the filename found
            strcpy(&ts[1], fdFile.cFileName);
            // sort the file name into place in the array
            if (sortorder == 0) {
                for (i = fcnt; i > 0; i--) {
                    if (strcicmp((flist[i - 1].fn), (ts)) > 0)
                        flist[i] = flist[i - 1];
                    else
                        break;
                }
            }
            else if (sortorder == 2) {
                for (i = fcnt; i > 0; i--) {
                    if ((flist[i - 1].fs) > currentsize)
                        flist[i] = flist[i - 1];
                    else
                        break;
                }
            }
            else if (sortorder == 3) {
                for (i = fcnt; i > 0; i--) {
                    char e2[8];
                    if (flist[i - 1].fn[strlen(flist[i - 1].fn) - 1] == '.') strcpy(e2, &flist[i - 1].fn[strlen(flist[i - 1].fn) - 1]);
                    else if (flist[i - 1].fn[strlen(flist[i - 1].fn) - 2] == '.') strcpy(e2, &flist[i - 1].fn[strlen(flist[i - 1].fn) - 2]);
                    else if (flist[i - 1].fn[strlen(flist[i - 1].fn) - 3] == '.') strcpy(e2, &flist[i - 1].fn[strlen(flist[i - 1].fn) - 3]);
                    else if (flist[i - 1].fn[strlen(flist[i - 1].fn) - 4] == '.') strcpy(e2, &flist[i - 1].fn[strlen(flist[i - 1].fn) - 4]);
                    else if (flist[i - 1].fn[strlen(flist[i - 1].fn) - 5] == '.') strcpy(e2, &flist[i - 1].fn[strlen(flist[i - 1].fn) - 5]);
                    else {
                        if (flist[i - 1].fn[0] == 'D') {
                            memset(e2, '+', sizeof(e2));
                            e2[sizeof(e2) - 1] = 0;
                        }
                        else {
                            memset(e2, '.', sizeof(e2));
                            e2[sizeof(e2) - 1] = 0;
                        }
                    }
                    if (strcicmp((e2), (extension)) > 0)
                        flist[i] = flist[i - 1];
                    else
                        break;
                }
            }
            else {
                for (i = fcnt; i > 0; i--) {
                    if (flist[i - 1].fd  < currentdate)
                        flist[i] = flist[i - 1];
                    else
                        break;
                }

            }
            strcpy(flist[i].fn, ts);
            flist[i].fs = fdFile.nFileSizeLow;
            flist[i].fd = ((uint64_t)fdFile.ftCreationTime.dwHighDateTime << 32) | fdFile.ftCreationTime.dwLowDateTime;
            fcnt++;
        }
    } while (FindNextFileA(hFind, &fdFile)); //Find the next file.

    FindClose(hFind); //Always, Always, clean things up!
    // list the files with a pause every screen full
    int ListCnt, dirs, c;
    ListCnt = 2;
    SYSTEMTIME stUTC, stLocal;
    FILETIME ftCreate;
    for (i = dirs = 0; i < fcnt; i++) {
         if (MMAbort) {
            FreeMemorySafe((void**)&flist);
            memset(inpbuf, 0, STRINGSIZE);
            longjmp(mark, 1);
        }
        if (flist[i].fn[0] == 'D') {
            dirs++;
            MMPrintString((char *)"   <DIR>  ");
        }
        else {
            ftCreate.dwHighDateTime = (DWORD)(flist[i].fd >> 32);
            ftCreate.dwLowDateTime = (DWORD)(flist[i].fd & 0xFFFFFFFF);
            FileTimeToSystemTime(&ftCreate, &stUTC);
            SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
            StringCchPrintfA((LPSTR)ts, 255,
                ("%02d/%02d/%d  %02d:%02d"),
                stLocal.wMonth, stLocal.wDay, stLocal.wYear,
                stLocal.wHour, stLocal.wMinute);
            IntToStrPad(ts + 17, flist[i].fs, ' ', 10, 10); MMPrintString(ts);
            MMPrintString((char*)"  ");
        }
        MMPrintString(flist[i].fn + 1);
        MMPrintString((char*)"\r\n");
        // check if it is more than a screen full
        if (++ListCnt >= OptionHeight && i < fcnt) {
            MMPrintString((char*)"PRESS ANY KEY ...");
            do {
                ShowCursor(1);
                if (MMAbort) {
                    FreeMemorySafe((void**)&flist);
                    memset(inpbuf, 0, STRINGSIZE);
                    ShowCursor(false);
                    longjmp(mark, 1);
                }
                c = -1;
                if (ConsoleRxBufHead != ConsoleRxBufTail) {                            // if the queue has something in it
                    c = ConsoleRxBuf[ConsoleRxBufTail];
                    ConsoleRxBufTail = (ConsoleRxBufTail + 1) % CONSOLE_RX_BUF_SIZE;   // advance the head of the queue
                }
            } while (c == -1);
            ShowCursor(0);
            MMPrintString((char*)"\r                 \r");
            ListCnt = 1;
        }
    }
    // display the summary
    IntToStr(ts, dirs, 10); MMPrintString(ts);
    MMPrintString((char*)" director"); MMPrintString(dirs == 1 ? (char*)"y, " : (char*)"ies, ");
    IntToStr(ts, fcnt - dirs, 10); MMPrintString(ts);
    MMPrintString((char*)" file"); MMPrintString((fcnt - dirs) == 1 ? (char*)"" : (char*)"s");
    MMPrintString((char*)"\r\n");
    FreeMemorySafe((void**)&flist);
    memset(inpbuf, 0, STRINGSIZE);
    longjmp(mark, 1);

    return true;
}


extern "C" char* MMgetcwd(void) {
    char* b;
    b = (char *)GetTempMemory(STRINGSIZE);
    int i = 0;
    i=(int)GetCurrentDirectoryA(255,b);
    if (i == 0)error((char*)"Directory error %", GetLastError());
    return b;
}
extern "C" bool dirExists(const char * dirName_in)
{
    DWORD ftyp = GetFileAttributesA(dirName_in);
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;  //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;   // this is a directory!

    return false;    // this is not a directory!
}
extern "C" void tidypath(char* p, char* qq) {
    strcpy(qq, p);
    for (int i = 0; i < (int)strlen(qq); i++)if (qq[i] == '/')qq[i] = '\\';
    if (!(qq[1] == ':' || qq[0] == '\\')) {
        char q[STRINGSIZE] = { 0 };
        strcpy(q, MMgetcwd());
        strcat(q, "\\");
        strcat(q, qq);
        PathCanonicalizeA(qq, q);
//        if (qq[strlen(qq) - 1] != '\\')strcat(qq, "\\");
    }
}
extern "C" void tidyfilename(char* p, char* path, char* filename, int search) {
    char q[STRINGSIZE] = { 0 };
    char r[STRINGSIZE] = { 0 };
    strcpy(q, p);
    for (int i = 0; i < (int)strlen(q); i++)if (q[i] == '/')q[i] = '\\';
    int i = strlen(q) - 1;
    while (i > 0 && q[i] != '\\')i--;
    if (i > 0) {
        memcpy(r, q, i);
        tidypath(r, path);
        i++;
    }
    else {
        if (search == 0)strcpy(path, MMgetcwd());
        else strcpy(path, Option.searchpath);
    }
    if (!(path[strlen(path) - 1] == '/' || path[strlen(path) - 1] == '\\'))strcat(path, "\\");
    strcpy(filename, &p[i]);
}
extern "C" void fullfilename(char* infile, char* outfile, const char * extension) {
    char q[STRINGSIZE] = { 0 };
    char filename[STRINGSIZE] = { 0 };
    char path[STRINGSIZE] = { 0 };
    strcpy(q, infile);
    tidyfilename(q, path, filename,0);
    strcpy(q, path);
    strcat(q, filename);
    if (strchr((char*)q, '.') == NULL && extension != NULL) strcat((char*)q, extension);
    strcpy(outfile, q);
}
void cmd_files(void) {
    char* p;
    int sortorder = 0;
    char filename[STRINGSIZE] = { 0 };
    char path[STRINGSIZE] = { 0 };
    char q[STRINGSIZE] = { 0 };
    if (CurrentLinePtr) error((char*)"Invalid in a program");
    if (*cmdline) {
        getargs(&cmdline, 3, (unsigned char *)",");
        if (!(argc == 1 || argc == 3))error((char*)"Syntax");
        p = (char*)getCstring(argv[0]);
        strcpy(q, p);
        if (q[strlen(q) - 1] == '/' || q[strlen(q) - 1] == '\\')strcat(q, "*");
        if (argc == 3) {
            if (checkstring(argv[2], (unsigned char*)"NAME"))sortorder = 0;
            else if (checkstring(argv[2], (unsigned char*)"TIME"))sortorder = 1;
            else if (checkstring(argv[2], (unsigned char*)"SIZE"))sortorder = 2;
            else if (checkstring(argv[2], (unsigned char*)"TYPE"))sortorder = 3;
            else error((char*)"Syntax");
        }
    }
    else strcpy(q, ".\\*");
    tidyfilename(q, path, filename,0);
    ListDirectoryContents((const char*)path, (const char*)filename, sortorder);
}
void fun_cwd(void) {
    MMerrno = 0;
    targ = T_STR;
    sret = CtoM((unsigned char *)MMgetcwd());
}
void cmd_chdir(void) {
    if (checkstring(cmdline, (unsigned char*)"DEFAULT")) {
        SetCurrentDirectoryA(Option.defaultpath);
        return;
    }
    if (checkstring(cmdline, (unsigned char*)"SEARCH")) {
        SetCurrentDirectoryA(Option.searchpath);
        return;
    }
    char* qq = (char*)GetTempMemory(STRINGSIZE);
    char* p;
    int i = 0;
    DWORD j = 0;
    p = (char *)getCstring(cmdline);
    // get the directory name and convert to a standard C string
    tidypath(p, qq);
    if (!dirExists(p))error((char*)"Directory does not exist");
    i= (int)SetCurrentDirectoryA(qq);
    if (i==0) {
        j = GetLastError();
        error((char*)"Directory error %", j);
    }


}
extern "C" void SaveOptions(void) {
    FILE* fp;
    char buf[sizeof(struct option_s)];
    CHAR my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
    strcat(my_documents, "\\.options");
    fp = fopen(my_documents, "w");
    memcpy(buf, &Option, sizeof(struct option_s));                  // insert our new data into the RAM copy
    fwrite(buf, 1, sizeof(struct option_s), fp);
    fclose(fp);
}


extern "C" void LoadOptions(void) {
    FILE* fp;
    char buf[sizeof(struct option_s)];
    CHAR my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
    strcat(my_documents, "\\.options");
    if ((fp = fopen(my_documents, "r")) != NULL) {
        fread(buf, 1, 4, fp);
        uint32_t* p = (uint32_t*)buf;
        if (*p != MagicKey) { //options have changed so reset everything
            fclose(fp);
            ResetOptions();
            fp = fopen(my_documents, "w");
            memcpy(buf, &Option, sizeof(struct option_s));                  // insert our new data into the RAM copy
            fwrite(buf, 1, sizeof(struct option_s), fp);
        }  else {
            fread(&buf[4], 1, sizeof(struct option_s) - 4, fp);
            memcpy(&Option, buf, sizeof(struct option_s));
        }
    }
    else {
        ResetOptions();
        fp = fopen(my_documents, "w");
        memcpy(buf, &Option, sizeof(struct option_s));                  // insert our new data into the RAM copy
        fwrite(buf, 1, sizeof(struct option_s), fp);
    }
    fclose(fp);
}

void cmd_save(void) {
    int fnbr;
    unsigned char* pp, * p;
    uint32_t* flinebuf;                                    // get the file name and change to the directory
    int maxH = VRes;
    int maxW = HRes;
    fnbr = FindFreeFileNbr();
    if ((p = checkstring(cmdline, (unsigned char *)"IMAGE")) != NULL) {
        char filename[STRINGSIZE] = { 0 };
        union colourmap
        {
            char rgbbytes[4];
            unsigned int rgb;
        } c;
        int i, x, y, w, h, filesize;
        unsigned char bmpfileheader[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
        unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0 };
        char bmppad[3] = { 0,0,0 };
        getargs(&p, 9, (unsigned char*)",");
        pp = getCstring(argv[0]);
        fullfilename((char *)pp, filename, ".BMP");
        if (argc != 1 && argc != 9)error((char*)"Syntax");
        if (!BasicFileOpen(filename, fnbr, (char *)"wb")) return;
        if (argc == 1) {
            x = 0; y = 0; h = maxH; w = maxW;
        }
        else {
            x = (int)getint(argv[2], 0, maxW - 1);
            if (optiony) x = maxW - 1 - x;
            y = (int)getint(argv[4], 0, maxH - 1);
            if (optiony) y = maxH - 1 - y;
            w = (int)getint(argv[6], 1, maxW - x);
            h = (int)getint(argv[8], 1, maxH - y);
        }
        int savey = optiony;
        optiony = 0;
        filesize = 54 + 3 * w * h;
        bmpfileheader[2] = (unsigned char)(filesize);
        bmpfileheader[3] = (unsigned char)(filesize >> 8);
        bmpfileheader[4] = (unsigned char)(filesize >> 16);
        bmpfileheader[5] = (unsigned char)(filesize >> 24);

        bmpinfoheader[4] = (unsigned char)(w);
        bmpinfoheader[5] = (unsigned char)(w >> 8);
        bmpinfoheader[6] = (unsigned char)(w >> 16);
        bmpinfoheader[7] = (unsigned char)(w >> 24);
        bmpinfoheader[8] = (unsigned char)(h);
        bmpinfoheader[9] = (unsigned char)(h >> 8);
        bmpinfoheader[10] = (unsigned char)(h >> 16);
        bmpinfoheader[11] = (unsigned char)(h >> 24);
        FilePutStr(14, (char *)bmpfileheader, fnbr);
        FilePutStr(40, (char *)bmpinfoheader, fnbr);
        flinebuf = (uint32_t *)GetTempMemory(maxW * 4);
        for (i = y + h - 1; i >= y; i--) {
            ReadBuffer(x, i, x + w - 1, i, (uint32_t *)flinebuf);
            for (int j = 0; j < w; j++) {
                c.rgb = flinebuf[j];
                MMfputc(c.rgbbytes[2], fnbr);
                MMfputc(c.rgbbytes[1], fnbr);
                MMfputc(c.rgbbytes[0], fnbr);
            }
            if ((w * 3) % 4 != 0) FilePutStr(4 - ((w * 3) % 4), bmppad, fnbr);
        }
        MMfclose(fnbr);
        optiony = savey;
        return;
    }
    error((char *)"Syntax");
/*    else {
        unsigned char b[STRINGSIZE];
        p = getCstring(cmdline);                           // get the file name and change to the directory
        if (strchr((char *)p, '.') == NULL) strcat((char*)p, ".BAS");
        if (!BasicFileOpen((char *)p, fnbr, (char*)"w")) return;
        p = ProgMemory;
        while (!(*p == 0 || *p == 0xff)) {                           // this is a safety precaution
            p = llist(b, p);                                        // expand the line
            pp = b;
            while (*pp) MMfputc(*pp++, fnbr);                    // write the line to the SD card
            MMfputc('\n', fnbr);       // terminate the line
            if (p[0] == 0 && p[1] == 0) break;                       // end of the listing ?
        }
        MMfclose(fnbr);
    }*/
}
void cmd_open(void) {
    int32_t fnbr;
    char* mode = NULL, * fname;
    unsigned char ss[3];														// this will be used to split up the argument line
    char filename[STRINGSIZE] = { 0 };

    ss[0] = tokenFOR;
    ss[1] = tokenAS;
    ss[2] = 0;
    // start a new block
    getargs(&cmdline, 5, ss);									// getargs macro must be the first executable stmt in a block
    fname = (char *)getCstring(argv[0]);
    fullfilename(fname, filename, NULL);
    if (!(argc == 3 || argc == 5)) error((char*)"Syntax");
    if (argc == 5) {
        if (checkstring(argv[2], (unsigned char*)"OUTPUT"))
            mode = (char*)"wb";											// binary mode so that we do not have lf to cr/lf translation
        else if (checkstring(argv[2], (unsigned char*)"APPEND"))
            mode = (char*)"ab";											// binary mode is used in MMfopen()
        else if (checkstring(argv[2], (unsigned char*)"INPUT"))
            mode = (char*)"rb";											// note binary mode
        else if (checkstring(argv[2], (unsigned char*)"RANDOM"))
            mode = (char*)"x";												// a special mode for MMfopen()
        else
            error((char*)"Invalid file access mode");
        if (*argv[4] == '#') argv[4]++;
        fnbr = (int)getinteger(argv[4]);
        BasicFileOpen(filename, fnbr, mode);
    }
    else {
        if (*argv[2] == '#') argv[2]++;
        fnbr = (int)getint(argv[2], 1, MAXOPENFILES);
        if (FileTable[fnbr].com != 0) error((char *)"Already open");
        FileTable[fnbr].com = SerialOpen(fname);
    }
}
void cmd_close(void) {
    int i, fnbr;
    getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, (unsigned char*)",");				// getargs macro must be the first executable stmt in a block
    if ((argc & 0x01) == 0) error((char*)"Syntax");
    for (i = 0; i < argc; i += 2) {
        if (*argv[i] == '#') argv[i]++;
        fnbr = (int)getint(argv[i], 1, MAXOPENFILES);
        if (FileTable[fnbr].com == 0) error((char*)"File number is not open");
        while (SerialTxStatus(FileTable[fnbr].com) && !MMAbort);     // wait for anything in the buffer to be transmitted
        if (FileTable[fnbr].com > MAXCOMPORTS) {
            FileClose(fnbr);
        } else SerialClose(FileTable[fnbr].com);
        FileTable[fnbr].com = 0;
    }
}
void cmd_seek(void) {
    int fnbr, idx;
    getargs(&cmdline, 3, (unsigned char*)",");
    if (argc != 3) error((char*)"Syntax");
    if (*argv[0] == '#') argv[0]++;
    fnbr = (int)getinteger(argv[0]);
    if (fnbr < 1 || fnbr > MAXOPENFILES || FileTable[fnbr].com <= MAXCOMPORTS) error((char*)"File number");
    if (FileTable[fnbr].com == 0) error((char*)"File number #% is not open", fnbr);
    idx = (int)getinteger(argv[2]) - 1;
    if (idx < 0) idx = 0;
    errno = 0;
    fseek(FileTable[fnbr].fptr, idx, SEEK_SET);
    ErrorCheck();
}
void cmd_kill(void) {
    unsigned char* p;
    p = getCstring(cmdline);										// get the file name and convert to a standard C string
    if (remove((char *)p)) error((char *)"Failed to delete: check case");
}

void fun_eof(void) {
    int fnbr;
    getargs(&ep, 1, (unsigned char*)",");
    if (argc == 0) error((char*)"Syntax");
    if (*argv[0] == '#') argv[0]++;
    fnbr = (int)getinteger(argv[0]);
    iret = (MMfeof(fnbr) ? 1:0);
    targ = T_INT;
}
void fun_inputstr(void) {
    int i, nbr, fnbr;
    getargs(&ep, 3, (unsigned char*)",");
    if (argc != 3) error((char *)"Syntax");
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);                                      // this will last for the life of the command
    nbr = (int)getint(argv[0], 1, MAXSTRLEN);
    if (*argv[2] == '#') argv[2]++;
    fnbr = (int)getinteger(argv[2]);
    if (fnbr == 0) {                                                 // accessing the console
        for (i = 1; i <= nbr && kbhitConsole(); i++)
            sret[i] = getConsole(0);                                 // get the char from the console input buffer and save in our returned string
    }
    else {
        if (fnbr < 1 || fnbr > MAXOPENFILES) error((char*)"File number");
        if (FileTable[fnbr].com == 0) error((char*)"File number is not open");
        if (MMfeof(fnbr)) {
            targ = T_STR;
            return;
        }
        targ = T_STR;
        if (FileTable[fnbr].com > MAXCOMPORTS) {
            i = 0;
            do {
                i++;
                sret[i] = FileGetChar(fnbr);
            } while (i < nbr && !MMfeof(fnbr));
            *sret = i;                                              // update the length of the string
            return;                                                     // all done so skip the rest
        }
        for (i = 1; i <= nbr && SerialRxStatus(FileTable[fnbr].com); i++)
            sret[i] = SerialGetchar(FileTable[fnbr].com);				// get the char from the serial input buffer and save in our returned string
    }
    *sret = i - 1;
}
void fun_loc(void) {
    int fnbr;
    getargs(&ep, 1, (unsigned char*)",");
    if (argc == 0) error((char*)"Syntax");
    if (*argv[0] == '#') argv[0]++;
    fnbr = (int)getinteger(argv[0]);
    if (fnbr == 0)                                                   // accessing the console
        iret = kbhitConsole();
    else {
        if (fnbr < 1 || fnbr > MAXOPENFILES) error((char*)"File number");
        if (FileTable[fnbr].com == NULL) error((char*)"File number is not open");
        if (FileTable[fnbr].com > MAXCOMPORTS) {
            iret = ftell(FileTable[fnbr].fptr) + 1;
        }
        else iret = SerialRxStatus(FileTable[fnbr].com);
    }
    targ = T_INT;
}


void fun_lof(void) {
    int fnbr,pos;
    getargs(&ep, 1, (unsigned char*)",");
    if (argc == 0) error((char*)"Syntax");
    if (*argv[0] == '#') argv[0]++;
    fnbr = (int)getinteger(argv[0]);
    if (fnbr == 0)                                                   // accessing the console
        iret = 0;
    else {
        if (fnbr < 1 || fnbr > MAXOPENFILES) error((char*)"File number");
        if (FileTable[fnbr].com == NULL) error((char*)"File number is not open");
        if (FileTable[fnbr].com > MAXCOMPORTS) {
            pos = ftell(FileTable[fnbr].fptr);
            fseek(FileTable[fnbr].fptr, 0L, SEEK_END);
            iret = ftell(FileTable[fnbr].fptr);
            fseek(FileTable[fnbr].fptr, pos, SEEK_SET);
        }
        else iret = (TX_BUFFER_SIZE - SerialTxStatus(FileTable[fnbr].com));
    }
    targ = T_INT;
}
extern "C" void ForceFileClose(int fnbr) {
    if (fnbr && FileTable[fnbr].fptr != NULL) {
        FileClose(fnbr);
        FreeMemory((unsigned char*)FileTable[fnbr].fptr);
        FileTable[fnbr].fptr = NULL;
    }
}

extern "C" void CloseAllFiles(void) {
    int i;
    closeall3d();
    closeallsprites();
    for (i = 1; i <= MAXOPENFILES; i++) {
        if (FileTable[i].com != 0) {
            if (FileTable[i].com > MAXCOMPORTS) {
                ForceFileClose(i);
            }
            else
                SerialClose(FileTable[i].com);
            FileTable[i].com = 0;
        }
    }
}
void cmd_name(void) {
    unsigned char* oldf, * newf, ss[2] = { 0 };
    ss[0] = tokenAS;										// this will be used to split up the argument line
    ss[1] = 0;
    {																// start a new block
        getargs(&cmdline, 3, ss);									// getargs macro must be the first executable stmt in a block
        if (argc != 3) error((char*)"Syntax");
        oldf = getCstring(argv[0]);									// get the old file name and convert to a standard C string
        newf = getCstring(argv[2]);									// get the new file name and convert to a standard C string
        errno = 0;
        if (rename((char*)oldf, (char*)newf))error((char*)"Failed to rename: check case");
    }
}




void cmd_copy(void) { 
    unsigned char* oldf, * newf, ss[2] = { 0 };
    char c;
    int32_t of, nf;
    char oldfilename[STRINGSIZE] = { 0 };
    char newfilename[STRINGSIZE] = { 0 };

    ss[0] = tokenTO;                                 // this will be used to split up the argument line
    ss[1] = 0;
    {
        getargs(&cmdline, 3, ss);                                   // getargs macro must be the first executable stmt in a block
        if (argc != 3) error((char*)"Syntax");
        oldf = getCstring(argv[0]);                                  // get the old file name and convert to a standard C string
        newf = getCstring(argv[2]);                                  // get the new file name and convert to a standard C string
        fullfilename((char *)oldf, oldfilename, NULL);
        fullfilename((char*)newf, newfilename, NULL);
        of = FindFreeFileNbr();
        if (of == 0) error((char*)"Too many files open");
        BasicFileOpen((char*)oldfilename, of, (char*)"rb");
        nf = FindFreeFileNbr();
        if (nf == 0) error((char*)"Too many files open");
        BasicFileOpen((char*)newfilename, nf, (char*)"wb");
    }
    while (1) {
        if (MMfeof(of)) break;
        c = MMfgetc(of);
        MMfputc(c, nf);
    }
    MMfclose(of);
    MMfclose(nf);
}
// search for a volume label, directory or file
// s$ = DIR$(fspec, DIR|FILE|ALL)       will return the first entry
// s$ = DIR$()                          will return the next
// If s$ is empty then no (more) files found
void fun_dir(void) {
    static WIN32_FIND_DATAA fdFile;
    static HANDLE hFind = NULL;
    unsigned char* p;
    static char pp[STRINGSIZE];
    int found = 0;
    static int found_OK = 0;
    getargs(&ep, 3, (unsigned char *)",");
    sret = (unsigned char*)GetTempMemory(STRINGSIZE);                                      // this will last for the life of the command
    if (argc != 0) dirflags = -1;
    if (!(argc <= 3)) error((char *)"Syntax");

    if (argc == 3) {
        if (checkstring(argv[2], (unsigned char*)"DIR"))
            dirflags = 1;
        else if (checkstring(argv[2], (unsigned char*)"FILE"))
            dirflags = -1;
        else if (checkstring(argv[2], (unsigned char*)"ALL"))
            dirflags = 0;
        else
            error((char*)"Invalid flag specification");
    }


    if (argc != 0) {
        char ts[STRINGSIZE];
        found_OK = 0;
        p=getCstring(argv[0]);
        // get the directory name and convert to a standard C string
        tidypath((char *)p, ts);
        if (hFind) {
            FindClose(hFind);
            hFind = NULL;
        }
        if ((hFind = FindFirstFileA((LPCSTR)ts, &fdFile)) == INVALID_HANDLE_VALUE)
        {
            goto found_it;
        }
        else {
            found_OK = 1;
            if (dirflags == 1) {
                if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (!(checkstring((unsigned char*)fdFile.cFileName, (unsigned char*)".") || checkstring((unsigned char*)fdFile.cFileName, (unsigned char*)".."))) {
                        found = 1;
                        goto found_it;     // Test for the file name
                    }
                }
            }
            else if (dirflags == -1) {
                    if (!(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        found = 1;
                        goto found_it;
                    }
            } else {
                if (!(checkstring((unsigned char*)fdFile.cFileName, (unsigned char*)".") || checkstring((unsigned char*)fdFile.cFileName, (unsigned char*)".."))) {
                    found = 1;
                    goto found_it;     // Test for the file name
                }
            }
        }
    }
    if (!found_OK) {
        CtoM(sret);                                                     // convert to a MMBasic style string
        targ = T_STR;
        return;
    }
    if (dirflags == 1) {
        for (;;) {
            if(!(found_OK =FindNextFileA(hFind, &fdFile)))break;      // Terminate if any error or end of directory
                if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (!(checkstring((unsigned char*)fdFile.cFileName, (unsigned char *)".") || checkstring((unsigned char*)fdFile.cFileName, (unsigned char*)".."))){
                        found = 1;
                        break;     // Test for the file name
                    }
                }
        }
    }
    else if (dirflags == -1) {
        for (;;) {
            if (!(found_OK =FindNextFileA(hFind, &fdFile)))break;      // Terminate if any error or end of directory
            if (!(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                found = 1;
                break;
            }
        }
    }
    else {
        for (;;) {
            if (!(found_OK =FindNextFileA(hFind, &fdFile)))break;      // Terminate if any error or end of directory
            if (!(checkstring((unsigned char*)fdFile.cFileName, (unsigned char*)".") || checkstring((unsigned char*)fdFile.cFileName, (unsigned char*)".."))) {
                found = 1;
                break;     // Test for the file name
            }
        }
    }

    if (!found_OK) {
        FindClose(hFind); //Always, Always, clean things up!
        hFind = NULL;
    }
    found_it:
    if (found_OK)strcpy((char *)sret, (const char *)fdFile.cFileName);
    CtoM(sret);                                                     // convert to a MMBasic style string
    targ = T_STR;
}

void cmd_mkdir(void) {
    char* qq = (char*)GetTempMemory(STRINGSIZE);
    char* p;
    p = (char*)getCstring(cmdline);
    // get the directory name and convert to a standard C string
    tidypath(p, qq);
    if (!CreateDirectoryA((LPCSTR)qq, NULL )) error((char *)"Unable to create directory");
}
void cmd_rmdir(void) {
    char* qq = (char*)GetTempMemory(STRINGSIZE);
    char* p;
    p = (char*)getCstring(cmdline);
    // get the directory name and convert to a standard C string
    tidypath(p, qq);
    if(!dirExists((const char*) qq)) error((char *)"Directory does not exist");
    if (!RemoveDirectoryA((LPCSTR)qq)) error((char*)"Unable to delete directory");
}

void fun_port(void) {
    int a = (int)getint(ep, 0, 127);
    iret = TestPort(a);
    targ = T_INT;
}
void cmd_tcp(void) {

    unsigned char* tp;
    char *tcp_SERVER;
    static int tcp_server_PORT;//, tcp_client_PORT;
//    const socklen_t slen = sizeof(struct sockaddr_in);
    tp = checkstring(cmdline, (unsigned char *)"CLIENT");
    if (tp) {
        getargs(&tp, 3, (unsigned char*)",");
        if (argc != 3)error((char *)"Syntax");
        if (tcpopen)error((char *)"Already open");
        char *ip;
        struct hostent* he;
        tcp_SERVER = (char*)getCstring(argv[0]);

        if ((he = gethostbyname(tcp_SERVER)) == NULL)
        {
            // get the host info
            error((char*)"Not Found");
        }
        ip = inet_ntoa(*(struct in_addr*)*he->h_addr_list);
        tcp_server_PORT = (int)getint(argv[2], 1, 65535);
        tcpsockfd = INVALID_SOCKET;
        memset((char*)&tcp_other, 0, sizeof(tcp_other));
        tcp_other.sin_addr.s_addr = inet_addr(ip);
        tcp_other.sin_port = htons(tcp_server_PORT);
        tcp_other.sin_family = AF_INET;
        tcpopen = 1;
        if ((tcpsockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
            error((char *)"Socket not opened");
        if (connect(tcpsockfd, (const struct sockaddr*)&tcp_other, sizeof(struct sockaddr_in)) == -1) {
            MMPrintString(inet_ntoa(tcp_other.sin_addr)); PRet();
            PInt(ntohs(tcp_other.sin_port)); PRet();
            error((char *)"connect failed %", WSAGetLastError());
        }
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"SEND");
    if (tp) {
        char* tcp_message;
        getargs((unsigned char**)&tp, 1, (unsigned char*)",");
        if (argc != 1)error((char *)"Syntax");
        if (tcpopen == 0)error((char *)"Not open");
        tcp_message = (char *)getCstring(argv[0]);
        if (send(tcpsockfd, tcp_message, strlen(tcp_message), 0) < 0) {
            error((char *)"send() failed");
            return;
        }
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"RECEIVE");
    if (tp) {
        unsigned char* retstring;
        int str1len, bytesRcvd;
        getargs((unsigned char**)&tp, 1, (unsigned char*)",");
        if (argc != 1)error((char *)"Syntax");
        if (tcpopen == 0)error((char *)"Not open");
        retstring = (unsigned char *)findvar(argv[0], V_FIND);
        if (!(vartbl[VarIndex].type & T_STR)) error((char *)"Invalid variable");
        str1len = (vartbl[VarIndex].size);
        memset(retstring, 0, str1len);
        if ((bytesRcvd = recv(tcpsockfd, (char *)retstring, str1len - 1, 0)) <= 0)
            retstring[0] = 0;
        retstring = CtoM(retstring);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"CLOSE");
    if (tp) {
        if (tcpopen == 0)error((char *)"Not open");
        tcpopen = 0;
        closesocket(tcpsockfd);
        return;

    }
    error((char *)"Syntax");

}
void cmd_udp(void) {
    /*
    unsigned char* tp;
    char *udp_SERVER;
    static int udp_server_PORT, udp_client_PORT;
    unsigned long mode = 1;
    const socklen_t slen = sizeof(struct sockaddr_in);
    tp = checkstring(cmdline, (unsigned char*)"CLIENT");
    if (tp) {
        getargs((unsigned char**)&tp, 5, (unsigned char*)",");
        if (argc != 5)error((char *)"Syntax");
        if (udpopen)error((char *)"Already open");
        udp_SERVER = (char *)getCstring(argv[0]);
        udp_server_PORT = (int)getint(argv[2], 1, 65535);
        udp_client_PORT = (int)getint(argv[4], 1, 65535);
        memset((char*)&si_me, 0, sizeof(si_me));
        memset((char*)&si_other, 0, sizeof(si_other));
        udpopen = 1;
        udpsockfd = 0;
        if ((udpsockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
            error((char *)"Socket not opened");
        ioctlsocket(udpsockfd, FIONBIO, &mode);
        if (udpsockfd < 0) error((char *)"ERROR opening socket");
        si_other.sin_family = AF_INET;
        si_other.sin_port = htons(udp_server_PORT);
        if (InetPtonA(AF_INET, udp_SERVER, &si_other.sin_addr) == 0) {
            error((char *)"inet_aton() failed");
            return;
        }
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(udp_client_PORT);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(udpsockfd, (const struct sockaddr*)&si_me, sizeof(si_me)) == -1)
            error((char *)"Bind failed");
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"SERVER");
    if (tp) {
        int udp_server_PORT;
        getargs((unsigned char**)&tp, 1, (unsigned char*)",");
        if (argc != 1)error((char *)"Syntax");
        if (udpopen)error((char *)"Already open");
        udpopen = 2;
        udp_server_PORT = (int)getint(argv[0], 1, 65535);
        if ((udpsockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
            error((char *)"Socket not opened");

        memset((char*)&si_me, 0, sizeof(si_me));
        memset((char*)&si_other, 0, sizeof(si_other));
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(udp_server_PORT);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(udpsockfd, (const struct sockaddr*)&si_me, sizeof(si_me)) == -1)
            error((char *)"Bind failed");
        ioctlsocket(udpsockfd, FIONBIO, &mode) ;
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"CLOSE");
    if (tp) {
        if (udpopen == 0)error((char *)"Not open");
        udpopen = 0;
        if (udpopen == 1) {
            closesocket(udp_server_PORT);
            closesocket(udp_client_PORT);
        }
        else closesocket(udpsockfd);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"SEND");
    if (tp) {
        char* udp_message;
        getargs((unsigned char**)&tp, 1, (unsigned char*)",");
        if (argc != 1)error((char *)"Syntax");
        if (udpopen == 0)error((char *)"Not open");
        if (si_other.sin_port == 0)error((char *)"No Client");
        udp_message = (char *)getCstring(argv[0]);
        if (sendto(udpsockfd, udp_message, strlen(udp_message), 0, (struct sockaddr*)&si_other, slen) < 0) {
            error((char *)"sendto() failed");
            return;
        }
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"RECEIVE");
    if (tp) {
        char* retstring, * otheripadd = NULL;
        int str1len, str2len;
        uint64_t* otherport = NULL;
        getargs((unsigned char**)&tp, 5, (unsigned char*)",");
        if (argc > 5 || argc < 1)error((char *)"Syntax");
        if (udpopen == 0)error((char *)"Not open");
        if (argc > 5) error((char *)"Incorrect number of arguments");

        // get the two variables
        retstring = (char *)findvar(argv[0], V_FIND);
        if (!(vartbl[VarIndex].type & T_STR)) error((char *)"Invalid variable");
        str1len = (vartbl[VarIndex].size);
        memset(retstring, 0, str1len);
        if (argc >= 3) {
            otheripadd = (char*)findvar(argv[2], V_FIND);
            if (!(vartbl[VarIndex].type & T_STR)) error((char *)"Invalid variable");
            str2len = (vartbl[VarIndex].size);
            memset(otheripadd, 0, str2len);
        }
        if (argc == 5) {
            otherport = (uint64_t *)findvar(argv[4], V_FIND);
            if (!((vartbl[VarIndex].type & T_NBR) || (vartbl[VarIndex].type & T_INT))) error((char *)"Invalid variable");
        }
        if (udpopen == 0)error((char *)"Not open");
        int slen = sizeof(si_other);
        if (recvfrom(udpsockfd, retstring, 255, 0, (struct sockaddr*)&si_other, (socklen_t*)&slen) == -1) {
            retstring[0] = 0;
            if (argc >= 3)otheripadd[0] = 0;
            if (argc == 5)*otherport = 0;
            return;
        }
        retstring = (char *)CtoM((unsigned char *)retstring);
        if (argc >= 3) {
            strcpy(otheripadd, inet_ntoa(si_other.sin_addr));
            otheripadd = (char *)CtoM((unsigned char*)otheripadd);
        }
        if (argc == 5)*otherport = ntohs(si_other.sin_port);
        return;
    }
    error((char *)"Syntax");
    */
}
void cmd_system(void) {
    char* p;
    char* buff;
    getargs(&cmdline, 3, (unsigned char *)",");
    if (system_active)error((char*)"Background SYSTEM command already active");
    p = (char *)getCstring(argv[0]);
    if (argc == 1) {
        if (p[strlen(p) - 1] == '&')system_active = 1;
        else system_active = 0;
        buff = (char *)GetTempMemory(2048 + 256);
        system_file = _popen(p, "r");
        if (system_active == 0) {
            while (!feof(system_file)) {
                if (fgets(buff, 2048, system_file) != NULL) MMPrintString(buff); MMPrintString((char *)"\r");
            }
            _pclose(system_file);
        }
    }
    else if (argc == 3) {
        void* ptr2 = NULL;
        int size, i = 0;
        int64_t* src;
        char* q;
        char ic;
        ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK);
        if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char*)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
                error((char*)"Invalid variable");
            }
            src = (int64_t*)ptr2;
            q = (char*)&src[1];
            size = (vartbl[VarIndex].dims[0] - OptionBase) * 8;
            system_file = _popen(p, "r");
            while (!feof(system_file) && i < size) {
                ic = (char)fgetc(system_file);
                if (!feof(system_file)) {
                    q[i] = ic;
                    i++;
                    if (ic == '\n') {
                        q[i] = '\r';
                        i++;
                    }
                }
            }
            src[0] = i;
            _pclose(system_file);
        } else if (vartbl[VarIndex].type & T_STR) {
            if (vartbl[VarIndex].dims[0] > 0) {		// Not an array
                error((char*)"Invalid variable");
            }
            q = (char*)ptr2;
            i = 1;
            size = 254;
            system_file = _popen(p, "r");
            while (!feof(system_file) && i < size) {
                ic = (char)fgetc(system_file);
                if (!feof(system_file)) {
                    q[i] = ic;
                    i++;
                    if (ic == '\n') {
                        q[i] = '\r';
                        i++;
                    }
                }
            }
            q[0] = i;
            _pclose(system_file);
        } else error((char*)"Syntax");
    }
    else error((char*)"Syntax");
}
void fun_getip(void) {
    char* q;
    char ip[255] = { 0 };
    struct hostent* he;
    struct in_addr** addr_list;
    int i;
    getargs(&ep, 3, (unsigned char *)",");
    q = (char *)getCstring(argv[0]);

    if ((he = gethostbyname(q)) == NULL)
    {
        // get the host info
        error((char *)"Not Found");
    }

    addr_list = (struct in_addr**)he->h_addr_list;

    for (i = 0; addr_list[i] != NULL; i++)
    {
        //Return the first one;
        strcpy(ip, inet_ntoa(*addr_list[i]));
        break;
    }

    sret = CtoM((unsigned char *)ip);
    targ = T_STR;
}
