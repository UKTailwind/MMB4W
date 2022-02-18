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

extern "C" uint8_t BMP_bDecode(int x, int y, int fnbr);
int dirflags;
union uFileTable FileTable[MAXOPENFILES + 1];
int OptionFileErrorAbort = true;
static uint32_t g_nInFileSize;
static uint32_t g_nInFileOfs;
static int jpgfnbr;
// 8*8*4 bytes * 3 = 768
int16_t* gCoeffBuf;

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
    Option.Height = Option.hres / gui_font_height;
    Option.Width = Option.vres / gui_font_width;
    Option.DefaultFC = M_WHITE;
    Option.DefaultBC = M_BLACK;
    Option.KeyboardConfig = CONFIG_UK;
    Option.RepeatStart = 600;
    Option.RepeatRate = 150;
    strcpy(Option.defaultpath, (const char *)my_documents);
}
int32_t ErrorThrow(int32_t e) {
    MMerrno = e;
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
    int fr = 0;
    FILE* fp;
    if ((fp = fopen((const char*)fname, "r")) != NULL) {
        fclose(fp);
        fr = 1;
    }
    return fr;
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
    FileTable[fnbr].fptr = (FILE *)GetMemory(sizeof(FILE));              // allocate the file descriptor
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
    if(fnbr == 0) return MMputchar(c);                                // accessing the console
    if(fnbr < 1 || fnbr > MAXOPENFILES) error((char *)"File number");
    if(FileTable[fnbr].com == 0) error((char*)"File number is not open");
    if (FileTable[fnbr].com > MAXCOMPORTS) {
        if (fwrite(&c, 1, 1, FileTable[fnbr].fptr) != 1)error((char*)"File write");
    }
    else c = SerialPutchar(FileTable[fnbr].com, c);                   // send the char to the serial port
    return c;
}
// output a string to a file
// the string must be a MMBasic string
/*extern "C" void MMfputs(unsigned char* p, int filenbr) {
    int i;
    i = *p++;
    if(FileTable[filenbr].com > MAXCOMPORTS) {
        if(fwrite(p, 1, i, FileTable[filenbr].fptr) != i)error((char*)"File write");
    }
    else {
        while (i--) MMfputc(*p++, filenbr);
    }
}*/
extern "C" void MMfputs(unsigned char* p, int filenbr) {
    int i;
    i = *p++;
    if (filenbr == 0) {
        while (i--)MMputchar(*p++);
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
    if (fnbr < 0 || fnbr > 10) error((char *)"Invalid file number");
    if (fnbr == 0) return 0;
    if (FileTable[fnbr].fptr== NULL) error((char*)"File number % is not open", fnbr);
    if ((uint32_t)FileTable[fnbr].fptr > MAXCOMPORTS) {
        errno = 0;
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

extern "C" int FileLoadProgram(unsigned char* fname, int mode) {
    int fnbr;
    char* p, * buf, buff[STRINGSIZE] = { 0 };
    int ch = 0;
    CloseAllFiles();
    ClearProgram();                                                 // clear any leftovers from the previous program
    fnbr = FindFreeFileNbr();
    if(mode)strcpy(buff, (char *)fname);
    else {
        p = (char*)getCstring(fname);
        strcpy(buff, p);
    }
    if (strchr(buff, '.') == NULL) strcat(buff, ".BAS");
    if (!BasicFileOpen(buff, fnbr, (char *)"rb")) return false;
    p = buf = (char *)GetTempMemory(EDIT_BUFFER_SIZE);              // get all the memory while leaving space for the couple of buffers defined and the file handle
    while (!MMfeof(fnbr)) {                                         // while waiting for the end of file
        if ((p - buf) >= EDIT_BUFFER_SIZE - 512) error((char *)"Not enough memory");
        if (fread(&ch, 1, 1, FileTable[fnbr].fptr) == 0) error((char*)"File Read");
        if (isprint(ch) || ch == '\r' || ch == '\n' || ch == TAB) {
            if (ch == TAB) ch = ' ';
            *p++ = ch;                                               // get the input into RAM
        }
    }
    strcpy(lastfileedited, buff);

    *p = 0;                                                         // terminate the string in RAM
    MMfclose(fnbr);
    SaveProgramToMemory((unsigned char *)buf, false);
    return true;
}
void LoadImage(unsigned char* p) {
    int fnbr;
    int xOrigin, yOrigin;

    // get the command line arguments
    getargs(&p, 5, (unsigned char*)",");                                            // this MUST be the first executable line in the function
    if (argc == 0) error((char*)"Argument count");

    p = getCstring(argv[0]);                                        // get the file name

    xOrigin = yOrigin = 0;
    if (argc >= 3) xOrigin = (int)getinteger(argv[2]);                    // get the x origin (optional) argument
    if (argc == 5) yOrigin = (int)getinteger(argv[4]);                    // get the y origin (optional) argument

    // open the file
    if (strchr((const char*)p, '.') == NULL) strcat((char*)p, ".BMP");
    fnbr = FindFreeFileNbr();
    if (!BasicFileOpen((char*)p, fnbr, (char*)"rb")) return;
    BMP_bDecode(xOrigin, yOrigin, fnbr);
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

    uint32_t decoded_width, decoded_height;
    int xOrigin, yOrigin;

    // get the command line arguments
    getargs(&p, 5, (unsigned char *)",");                                            // this MUST be the first executable line in the function
    if (argc == 0) error((char *)"Argument count");

    p = getCstring(argv[0]);                                        // get the file name

    xOrigin = yOrigin = 0;
    if (argc >= 3) xOrigin = (int)getint(argv[2], 0, HRes - 1);                    // get the x origin (optional) argument
    if (argc == 5) yOrigin = (int)getint(argv[4], 0, VRes - 1);                    // get the y origin (optional) argument

    // open the file
    if (strchr((const char *)p, '.') == NULL) strcat((char *)p, ".JPG");
    jpgfnbr = FindFreeFileNbr();
    if (!BasicFileOpen((char *)p, jpgfnbr, (char*)"rb")) return;
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
                error((char*)"pjpeg_decode_mcu() failed with status %", status);
            }
            break;
        }

        if (mcu_y >= image_info.m_MCUSPerCol)
        {
            FileClose(jpgfnbr);
            return;
        }
        /*    for(int i=0;i<image_info.m_MCUHeight*image_info.m_MCUWidth ;i++){
                  imageblock[i*3+2]=image_info.m_pMCUBufR[i];
                  imageblock[i*3+1]=image_info.m_pMCUBufG[i];
                  imageblock[i*3]=image_info.m_pMCUBufB[i];
              }*/
              //         pDst_row = pImage + (mcu_y * image_info.m_MCUHeight) * row_pitch + (mcu_x * image_info.m_MCUWidth * image_info.m_comps);

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
            return;
        }
        mcu_x++;
        if (mcu_x == image_info.m_MCUSPerRow)
        {
            mcu_x = 0;
            mcu_y++;
        }
    }
    FileClose(jpgfnbr);
}
void LoadPNG(unsigned char* p) {
    //	int fnbr;
    int xOrigin, yOrigin, w, h, transparent = 0, force = 0;
    int maxW = PageTable[WritePage].xmax;
    int maxH = PageTable[WritePage].ymax;
    upng_t* upng;
    // get the command line arguments
    getargs(&p, 7, (unsigned char *)",");                                            // this MUST be the first executable line in the function
    if (argc == 0) error((char *)"Argument count");

    p = getCstring(argv[0]);                                        // get the file name

    xOrigin = yOrigin = 0;
    if (argc >= 3 && *argv[2]) xOrigin = (int)getinteger(argv[2]);                    // get the x origin (optional) argument
    if (argc >= 5 && *argv[4]) {
        yOrigin = (int)getinteger(argv[4]);                    // get the y origin (optional) argument
//******        if (optiony) yOrigin = maxH - 1 - yOrigin;
    }
    if (argc == 7)transparent = (int)getint(argv[6], 0, 15);
    if (transparent) {
        if (transparent > 1)force = transparent << 4;
        transparent = 4;
    }
    // open the file
    if (strchr((const char*)p, '.') == NULL) strcat((char*)p, ".PNG");
    //	fnbr = FindFreeFileNbr();
    //    if(!BasicFileOpen(p, fnbr, FA_READ)) return;
    upng = upng_new_from_file((char *)p);
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
//******    int savey = optiony;
//    optiony = 0;
    if (upng_get_format(upng) == 3) {
        DrawBuffer32(xOrigin, yOrigin, xOrigin + w - 1, yOrigin + h - 1, (char*)rr, 3 | transparent | force);
    }
    else {
        DrawBuffer32(xOrigin, yOrigin, xOrigin + w - 1, yOrigin + h - 1, (char*)rr, 2 | transparent | force);
    }
//    optiony = savey;
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
        getargs(&p, 3, (unsigned char*)",");
        if (argc != 3)error((char*)"Syntax");
        pp = (char *)getCstring(argv[0]);
        if (strchr(pp, '.') == NULL) strcat(pp, ".DAT");
        uint32_t address = (GetPokeAddr(argv[2]) & 0b11111111111111111111111111111100);
        fnbr = FindFreeFileNbr();
        if (!BasicFileOpen(pp, fnbr, (char*)"rb")) return;
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
    strcat(sPath, "\\");
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
        if (++ListCnt >= Option.Height && i < fcnt) {
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


char* MMgetcwd(void) {
    char* b;
    b = (char *)GetTempMemory(STRINGSIZE);
    int i = 0;
    i=(int)GetCurrentDirectoryA(255,b);
    if (i == 0)error((char*)"Directory error %", GetLastError());
    return b;
}

void cmd_files(void) {
    int i, j;
    char* p;
    int sortorder = 0;
    char ts[STRINGSIZE] = { 0 };
    char pp[STRINGSIZE] = { 0 };
    char q[STRINGSIZE] = { 0 };
    if (*cmdline) {
        getargs(&cmdline, 3, (unsigned char *)",");
        if (!(argc == 1 || argc == 3))error((char*)"Syntax");
        p = (char*)getCstring(argv[0]);
        i = strlen(p) - 1;
        while (i > 0 && !(p[i] == 92 || p[i] == 47))i--;
        if (i > 0) {
            memcpy(q, p, i);
            for (j = 0; j < (int)strlen(q); j++)if (q[j] == (char)'\\')q[j] = '/';  //allow backslash for the DOS oldies
            i++;
        }
        strcpy(pp, &p[i]);
        if (argc == 3) {
            if (checkstring(argv[2], (unsigned char*)"NAME"))sortorder = 0;
            else if (checkstring(argv[2], (unsigned char*)"TIME"))sortorder = 1;
            else if (checkstring(argv[2], (unsigned char*)"SIZE"))sortorder = 2;
            else if (checkstring(argv[2], (unsigned char*)"TYPE"))sortorder = 3;
            else error((char*)"Syntax");
        }
    }
    if (pp[0] == 0)strcpy(pp, "*");
    if (CurrentLinePtr) error((char*)"Invalid in a program");
    if (!*q) {
        MMPrintString((char*)MMgetcwd());
        MMPrintString((char*)"\r\n");
        ListDirectoryContents(MMgetcwd(), (const char*)pp, sortorder);
    }  else {
        if (!(q[1] == ':' || q[0] == '\\' || q[0] == '/')) {
            strcpy(ts, MMgetcwd());
            strcat(ts, "\\");
            strcat(ts, q);
            strcat(ts, "\\");
            MMPrintString(ts);
            MMPrintString((char*)"\r\n");
            ListDirectoryContents((const char*)q, (const char*)pp, sortorder);
        }
        else {
            MMPrintString(ts);
            MMPrintString((char*)"\r\n");
            ListDirectoryContents((const char*)q, (const char*)pp, sortorder);
        }
    }
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
    char* p;
    int i = 0;
    DWORD j = 0;
    p = (char *)getCstring(cmdline);										// get the directory name and convert to a standard C string
    i= (int)SetCurrentDirectoryA(p);
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
        if (argc != 1 && argc != 9)error((char*)"Syntax");
        if (strchr((char *)pp, '.') == NULL) strcat((char*)pp, ".BMP");
        if (!BasicFileOpen((char*)pp, fnbr, (char *)"wb")) return;
        if (argc == 1) {
            x = 0; y = 0; h = maxH; w = maxW;
        }
        else {
            x = (int)getint(argv[2], 0, maxW - 1);
            y = (int)getint(argv[4], 0, maxH - 1);
            w = (int)getint(argv[6], 1, maxW - x);
            h = (int)getint(argv[8], 1, maxH - y);
        }
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
        return;
    }
    else {
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
    }
}
void cmd_open(void) {
    int32_t fnbr;
    char* mode = NULL, * fname;
    unsigned char ss[3];														// this will be used to split up the argument line

    ss[0] = tokenFOR;
    ss[1] = tokenAS;
    ss[2] = 0;
    // start a new block
    getargs(&cmdline, 5, ss);									// getargs macro must be the first executable stmt in a block
    fname = (char *)getCstring(argv[0]);

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
        BasicFileOpen(fname, fnbr, mode);
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
    iret = MMfeof(fnbr);
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
            sret[i] = getConsole();                                 // get the char from the console input buffer and save in our returned string
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
            iret = ftell(FileTable[fnbr].fptr)-pos;
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

    ss[0] = tokenTO;                                 // this will be used to split up the argument line
    ss[1] = 0;
    {
        getargs(&cmdline, 3, ss);                                   // getargs macro must be the first executable stmt in a block
        if (argc != 3) error((char*)"Syntax");
        oldf = getCstring(argv[0]);                                  // get the old file name and convert to a standard C string
        newf = getCstring(argv[2]);                                  // get the new file name and convert to a standard C string

        of = FindFreeFileNbr();
        if (of == 0) error((char*)"Too many files open");
        MMfopen((char*)oldf, (char*)"r", of);

        nf = FindFreeFileNbr();
        if (nf == 0) error((char*)"Too many files open");
        MMfopen((char*)newf, (char*)"w", nf); 										// We'll just overwrite any existing file
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
        if (!(p[1] == ':' || p[0] == '\\' || p[0] == '/')) {
            strcpy(ts, MMgetcwd());
            strcat(ts, "\\");
            strcat(ts, (const char*)p);
        } else strcpy(ts, (const char*)p);
        for (int j = 0; j < (int)strlen(ts); j++)if (ts[j] == (char)'\\')ts[j] = '/';  //allow backslash for the DOS oldies
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
    unsigned char* p;

    p = getCstring(cmdline);										// get the directory name and convert to a standard C string
    if (!CreateDirectoryA((LPCSTR)p, NULL )) error((char *)"Unable to create directory");
}
void cmd_rmdir(void) {
    unsigned char* p;

    p = getCstring(cmdline);										// get the directory name and convert to a standard C string
    if (!RemoveDirectoryA((LPCSTR)p)) error((char*)"Unable to delete directory");
}

void fun_port(void) {
    int a = (int)getint(ep, 0, 127);
    iret = TestPort(a);
    targ = T_INT;
}