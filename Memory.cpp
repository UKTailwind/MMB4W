/***********************************************************************************************************************
MMBasic for Windows

Memory.cpp

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
#include "Memory.h"
#include "MMBasic.h"
#include "FileIO.h"
#include "MM_Misc.h"
__declspec(align(256)) unsigned char ProgMemory[MAX_PROG_SIZE];
__declspec(align(256)) unsigned char MMHeap[HEAP_MEMORY_SIZE];
__declspec(align(256)) struct s_ctrl Ctrl[MAXCTRLS];
// Get a temporary buffer of any size
// The space only lasts for the length of the command.
// A pointer to the space is saved in an array so that it can be returned at the end of the command
volatile char* StrTmp[MAXTEMPSTRINGS];                                       // used to track temporary string space on the heap
volatile char StrTmpLocalIndex[MAXTEMPSTRINGS];                              // used to track the LocalIndex for each temporary string space on the heap
int TempMemoryIsChanged = false;						            // used to prevent unnecessary scanning of strtmp[]
int StrTmpIndex = 0;                                                // index to the next unallocated slot in strtmp[]
unsigned int mmap[HEAP_MEMORY_SIZE / PAGESIZE / PAGESPERWORD];
void cmd_memory(void) {
    unsigned char* p, * tp;
    tp = checkstring(cmdline, (unsigned char *)"COPY");
    if (tp) {
        if ((p = checkstring(tp, (unsigned char*)"INTEGER"))) {
            int stepin = 1, stepout = 1;
            getargs(&p, 9, (unsigned char*)",");
            if (argc < 5)error((char *)"Syntax");
            int n = (int)getinteger(argv[4]);
            if (n <= 0)return;
            uint64_t* from = (uint64_t*)GetPokeAddr(argv[0]);
            uint64_t* to = (uint64_t*)GetPokeAddr(argv[2]);
            if ((uint32_t)from % 8)error((char *)"Address not divisible by 8");
            if ((uint32_t)to % 8)error((char *)"Address not divisible by 8");
            if (argc >= 7 && *argv[6])stepin = (int)getint(argv[6], 0, 0xFFFF);
            if (argc == 9)stepout = (int)getint(argv[8], 0, 0xFFFF);
            if (stepin == 1 && stepout == 1)memcpy(to, from, n * 8);
            else {
                while (n--) {
                    *to = *from;
                    to += stepout;
                    from += stepin;
                }
            }
            return;
        }
        if ((p = checkstring(tp, (unsigned char*)"FLOAT"))) {
            int stepin = 1, stepout = 1;
            getargs(&p, 9, (unsigned char*)","); //assume byte
            if (argc < 5)error((char *)"Syntax");
            int n = (int)getinteger(argv[4]);
            if (n <= 0)return;
            MMFLOAT* from = (MMFLOAT*)GetPokeAddr(argv[0]);
            MMFLOAT* to = (MMFLOAT*)GetPokeAddr(argv[2]);
            if ((uint32_t)from % 8)error((char *)"Address not divisible by 8");
            if ((uint32_t)to % 8)error((char *)"Address not divisible by 8");
            if (argc >= 7 && *argv[6])stepin = (int)getint(argv[6], 0, 0xFFFF);
            if (argc == 9)stepout = (int)getint(argv[8], 0, 0xFFFF);
            if (n <= 0)return;
            if (stepin == 1 && stepout == 1)memcpy(to, from, n * 8);
            else {
                while (n--) {
                    *to = *from;
                    to += stepout;
                    from += stepin;
                }
            }
            return;
        }
        getargs(&tp, 5, (unsigned char*)",");
        if (argc != 5)error((char *)"Syntax");
        char* from = (char*)GetPeekAddr(argv[0]);
        char* to = (char*)GetPokeAddr(argv[2]);
        int n = (int)getinteger(argv[4]);
        memcpy(to, from, n);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"SET");
    if (tp) {
        unsigned char* p;
        if ((p = checkstring(tp, (unsigned char*)"BYTE"))) {
            getargs(&p, 5, (unsigned char*)","); //assume byte
            if (argc != 5)error((char *)"Syntax");
            char* to = (char*)GetPokeAddr(argv[0]);
            int val = (int)getint(argv[2], 0, 255);
            int n = (int)getinteger(argv[4]);
            if (n <= 0)return;
            memset(to, val, n);
            return;
        }
        if ((p = checkstring(tp, (unsigned char*)"SHORT"))) {
            getargs(&p, 5, (unsigned char*)","); //assume byte
            if (argc != 5)error((char *)"Syntax");
            short* to = (short*)GetPokeAddr(argv[0]);
            if ((uint32_t)to % 2)error((char *)"Address not divisible by 2");
            short* q = to;
            short data = (int)getint(argv[2], 0, 65535);
            int n = (int)getinteger(argv[4]);
            if (n <= 0)return;
            while (n > 0) {
                *q++ = data;
                n--;
            }
            return;
        }
        if ((p = checkstring(tp, (unsigned char*)"WORD"))) {
            getargs(&p, 5, (unsigned char*)","); //assume byte
            if (argc != 5)error((char *)"Syntax");
            unsigned int* to = (unsigned int*)GetPokeAddr(argv[0]);
            if ((uint32_t)to % 4)error((char *)"Address not divisible by 4");
            unsigned int* q = to;
            unsigned int data = (int)getint(argv[2], 0, 0xFFFFFFFF);
            int n = (int)getinteger(argv[4]);
            if (n <= 0)return;
            while (n > 0) {
                *q++ = data;
                n--;
            }
            return;
        }
        if ((p = checkstring(tp, (unsigned char*)"INTEGER"))) {
            int stepin = 1;
            getargs(&p, 7, (unsigned char*)",");
            if (argc < 5)error((char *)"Syntax");
            uint64_t* to = (uint64_t*)GetPokeAddr(argv[0]);
            if ((uint32_t)to % 8)error((char *)"Address not divisible by 8");
            int64_t data;
            data = getinteger(argv[2]);
            int n = (int)getinteger(argv[4]);
            if (argc == 7)stepin = (int)getint(argv[6], 0, 0xFFFF);
            if (n <= 0)return;
            if (stepin == 1)while (n--)*to++ = data;
            else {
                while (n--) {
                    *to = data;
                    to += stepin;
                }
            }
            return;
        }
        if ((p = checkstring(tp, (unsigned char*)"FLOAT"))) {
            int stepin = 1;
            getargs(&p, 7, (unsigned char*)","); //assume byte
            if (argc < 5)error((char *)"Syntax");
            MMFLOAT* to = (MMFLOAT*)GetPokeAddr(argv[0]);
            if ((uint32_t)to % 8)error((char *)"Address not divisible by 8");
            MMFLOAT data;
            data = getnumber(argv[2]);
            int n = (int)getinteger(argv[4]);
            if (argc == 7)stepin = (int)getint(argv[6], 0, 0xFFFF);
            if (n <= 0)return;
            if (stepin == 1)while (n--)*to++ = data;
            else {
                while (n--) {
                    *to = data;
                    to += stepin;
                }
            }
            return;
        }
        getargs(&tp, 5, (unsigned char*)","); //assume byte
        if (argc != 5)error((char *)"Syntax");
        char* to = (char*)GetPokeAddr(argv[0]);
        int val = (int)getint(argv[2], 0, 255);
        int n = (int)getinteger(argv[4]);
        if (n <= 0)return;
        memset(to, val, n);
        return;
    }
    int i, j, var, nbr, vsize, VarCnt;
    int ProgramSize=0, ProgramPercent=0, VarSize=0, VarPercent=0, GeneralSize=0, GeneralPercent=0, SavedVarSize=0, SavedVarSizeK=0, SavedVarPercent=0, SavedVarCnt=0;
    int CFunctSizeK=0, CFunctNbr=0, CFunctPercent=0, FontSizeK=0, FontNbr=0, FontPercent=0, LibrarySizeK=0, LibraryPercent=0;
    unsigned int CurrentRAM;

    CurrentRAM = HEAP_MEMORY_SIZE + MAXVARS * sizeof(struct s_vartbl);

    // calculate the space allocated to variables on the heap
    for (i = VarCnt = vsize = var = 0; var < MAXVARS; var++) {
        if (vartbl[var].type == T_NOTYPE) continue;
        VarCnt++;  vsize += sizeof(struct s_vartbl);
        if (vartbl[var].val.s == NULL) continue;
        if (vartbl[var].type & T_PTR) continue;
        nbr = vartbl[var].dims[0] + 1 - OptionBase;
        if (vartbl[var].dims[0]) {
            for (j = 1; j < MAXDIM && vartbl[var].dims[j]; j++)
                nbr *= (vartbl[var].dims[j] + 1 - OptionBase);
            if (vartbl[var].type & T_NBR)
                i += MRoundUp(nbr * sizeof(MMFLOAT));
            else if (vartbl[var].type & T_INT)
                i += MRoundUp(nbr * sizeof(long long int));
            else
                i += MRoundUp(nbr * (vartbl[var].size + 1));
        }
        else
            if (vartbl[var].type & T_STR)
                i += STRINGSIZE;
    }
    VarSize = (vsize + i + 512) / 1024;                               // this is the memory allocated to variables
    VarPercent = ((vsize + i) * 100) / CurrentRAM;
    if (VarCnt && VarSize == 0) VarPercent = VarSize = 1;            // adjust if it is zero and we have some variables
    i = UsedHeap() - i;
    if (i < 0) i = 0;
    GeneralSize = (i + 512) / 1024; GeneralPercent = (i * 100) / CurrentRAM;

    // count the space used by saved variables (in flash)
    SavedVarCnt = 0;
/*    while (!(*p == 0 || *p == 0xff)) {
        unsigned char type, array;
        SavedVarCnt++;
        type = *p++;
        array = type & 0x80;  type &= 0x7f;                         // set array to true if it is an array
        p += strlen((const char *)p) + 1;
        if (array)
            p += (p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24) + 4;
        else {
            if (type & T_NBR)
                p += sizeof(MMFLOAT);
            else if (type & T_INT)
                p += sizeof(long long int);
            else
                p += *p + 1;
        }
    }
    SavedVarSize = p - (SavedVarsFlash);*/
    SavedVarSizeK = (SavedVarSize + 512) / 1024;
    SavedVarPercent = (SavedVarSize * 100) / (HEAP_MEMORY_SIZE /* + SAVEDVARS_FLASH_SIZE*/);
    if (SavedVarCnt && SavedVarSizeK == 0) SavedVarPercent = SavedVarSizeK = 1;        // adjust if it is zero and we have some variables

    // count the space used by CFunctions, CSubs and fonts
    /*CFunctSize = CFunctNbr = FontSize = FontNbr = 0;
    pint = (unsigned int *)CFunctionFlash;
    while(*pint != 0xffffffff) {
        if(*pint < FONT_TABLE_SIZE) {
            pint++;
            FontNbr++;
            FontSize += *pint + 8;
        } else {
            pint++;
            CFunctNbr++;
            CFunctSize += *pint + 8;
        }
        pint += (*pint + 4) / sizeof(unsigned int);
    }
    CFunctPercent = (CFunctSize * 100) /  (Option.PROG_FLASH_SIZE + SAVEDVARS_FLASH_SIZE);
    CFunctSizeK = (CFunctSize + 512) / 1024;
    if(CFunctNbr && CFunctSizeK == 0) CFunctPercent = CFunctSizeK = 1;              // adjust if it is zero and we have some functions
    FontPercent = (FontSize * 100) /  (Option.PROG_FLASH_SIZE + SAVEDVARS_FLASH_SIZE);
    FontSizeK = (FontSize + 512) / 1024;
    if(FontNbr && FontSizeK == 0) FontPercent = FontSizeK = 1;                      // adjust if it is zero and we have some functions
*/
// count the number of lines in the program
    p = ProgMemory;
    i = 0;
    while (*p != 0xff) {                                             // skip if program memory is erased
        if (*p == 0) p++;                                            // if it is at the end of an element skip the zero marker
        if (*p == 0) break;                                          // end of the program or module
        if (*p == T_NEWLINE) {
            i++;                                                    // count the line
            p++;                                                    // skip over the newline token
        }
        if (*p == T_LINENBR) p += 3;                                 // skip over the line number
        skipspace(p);
        if (p[0] == T_LABEL) p += p[1] + 2;							// skip over the label
        while (*p) p++;												// look for the zero marking the start of an element
    }
    ProgramSize = ((p - ProgMemory) + 512) / 1024;
    ProgramPercent = ((p - ProgMemory) * 100) / (MAX_PROG_SIZE /* + SAVEDVARS_FLASH_SIZE*/);
    if (ProgramPercent > 100) ProgramPercent = 100;
    if (i && ProgramSize == 0) ProgramPercent = ProgramSize = 1;                                        // adjust if it is zero and we have some lines

    MMPrintString((char*)"Program:\r\n");
    IntToStrPad((char *)inpbuf, ProgramSize, ' ', 4, 10); strcat((char *)inpbuf, "K (");
    IntToStrPad((char *)inpbuf + strlen((const char *)inpbuf), ProgramPercent, ' ', 2, 10); strcat((char *)inpbuf, "%) Program (");
    IntToStr((char*)inpbuf + strlen((const char *)inpbuf), i, 10); strcat((char *)inpbuf, " lines)\r\n");
    MMPrintString((char *)inpbuf);

    if (CFunctNbr) {
        IntToStrPad((char *)inpbuf, CFunctSizeK, ' ', 4, 10); strcat((char *)inpbuf, "K (");
        IntToStrPad((char *)inpbuf + strlen((const char *)inpbuf), CFunctPercent, ' ', 2, 10); strcat((char *)inpbuf, "%) "); MMPrintString((char *)inpbuf);
        IntToStr((char*)inpbuf, CFunctNbr, 10); strcat((char *)inpbuf, " Embedded C Routine"); strcat((char *)inpbuf, CFunctNbr == 1 ? "\r\n" : "s\r\n");
        MMPrintString((char *)inpbuf);
    }

    if (FontNbr) {
        IntToStrPad((char *)inpbuf, FontSizeK, ' ', 4, 10); strcat((char *)inpbuf, "K (");
        IntToStrPad((char *)inpbuf + strlen((const char *)inpbuf), FontPercent, ' ', 2, 10); strcat((char *)inpbuf, "%) "); MMPrintString((char *)inpbuf);
        IntToStr((char*)inpbuf, FontNbr, 10); strcat((char *)inpbuf, " Embedded Fonts"); strcat((char *)inpbuf, FontNbr == 1 ? "\r\n" : "s\r\n");
        MMPrintString((char *)inpbuf);
    }

    if (SavedVarCnt) {
        IntToStrPad((char *)inpbuf, SavedVarSizeK, ' ', 4, 10); strcat((char *)inpbuf, "K (");
        IntToStrPad((char *)inpbuf + strlen((const char *)inpbuf), SavedVarPercent, ' ', 2, 10); strcat((char *)inpbuf, "%)");
        IntToStrPad((char *)inpbuf + strlen((const char *)inpbuf), SavedVarCnt, ' ', 2, 10); strcat((char *)inpbuf, " Saved Variable"); strcat((char *)inpbuf, SavedVarCnt == 1 ? " (" : "s (");
        IntToStr((char*)inpbuf + strlen((const char *)inpbuf), SavedVarSize, 10); strcat((char *)inpbuf, " bytes)\r\n");
        MMPrintString((char *)inpbuf);
    }

    LibrarySizeK = LibraryPercent = 0;

    IntToStrPad((char *)inpbuf, ((MAX_PROG_SIZE/* + SAVEDVARS_FLASH_SIZE*/) + 512) / 1024 - ProgramSize - CFunctSizeK - FontSizeK - SavedVarSizeK - LibrarySizeK, ' ', 4, 10); strcat((char *)inpbuf, "K (");
    IntToStrPad((char *)inpbuf + strlen((const char *)inpbuf), 100 - ProgramPercent - CFunctPercent - FontPercent - SavedVarPercent - LibraryPercent, ' ', 2, 10); strcat((char *)inpbuf, "%) Free\r\n");
    MMPrintString((char *)inpbuf);

    MMPrintString((char*)"\r\nRAM:\r\n");
    IntToStrPad((char *)inpbuf, VarSize, ' ', 4, 10); strcat((char *)inpbuf, "K (");
    IntToStrPad((char *)inpbuf + strlen((const char *)inpbuf), VarPercent, ' ', 2, 10); strcat((char *)inpbuf, "%) ");
    IntToStr((char*)inpbuf + strlen((const char *)inpbuf), VarCnt, 10); strcat((char *)inpbuf, " Variable"); strcat((char *)inpbuf, VarCnt == 1 ? "\r\n" : "s\r\n");
    MMPrintString((char *)inpbuf);

    IntToStrPad((char *)inpbuf, GeneralSize, ' ', 4, 10); strcat((char *)inpbuf, "K (");
    IntToStrPad((char *)inpbuf + strlen((const char *)inpbuf), GeneralPercent, ' ', 2, 10); strcat((char *)inpbuf, "%) General\r\n");
    MMPrintString((char *)inpbuf);

    IntToStrPad((char *)inpbuf, (CurrentRAM + 512) / 1024 - VarSize - GeneralSize, ' ', 4, 10); strcat((char *)inpbuf, "K (");
    IntToStrPad((char *)inpbuf + strlen((const char *)inpbuf), 100 - VarPercent - GeneralPercent, ' ', 2, 10); strcat((char *)inpbuf, "%) Free\r\n");
    MMPrintString((char *)inpbuf);
}

unsigned int MBitsGet(unsigned char* addr) {
    unsigned int i, * p;
    addr -= (unsigned int)&MMHeap[0];
    p = &mmap[((unsigned int)addr / PAGESIZE) / PAGESPERWORD];        // point to the word in the memory map
    i = ((((unsigned int)addr / PAGESIZE)) & (PAGESPERWORD - 1)) * PAGEBITS; // get the position of the bits in the word
    return (*p >> i) & ((1 << PAGEBITS) - 1);
}



void MBitsSet(unsigned char* addr, int bits) {
    unsigned int i, * p;
    addr -= (unsigned int)&MMHeap[0];
    p = &mmap[((unsigned int)addr / PAGESIZE) / PAGESPERWORD];        // point to the word in the memory map
    i = ((((unsigned int)addr / PAGESIZE)) & (PAGESPERWORD - 1)) * PAGEBITS; // get the position of the bits in the word
    *p = (bits << i) | (*p & (~(((1 << PAGEBITS) - 1) << i)));
}

extern "C" void FreeMemory(unsigned char* addr) {
    int bits;
    if(addr == NULL) return;
    //   dp(" free = %p", addr);
    do {
        bits = MBitsGet(addr);
        MBitsSet(addr, 0);
        addr += PAGESIZE;
    } while (bits != (PUSED | PLAST));
}
int MemSize(void* addr) { //returns the amount of heap memory allocated to an address
    int i = 0;
    int bits;
    if (addr >= (void*)MMHeap && addr < (void*)(MMHeap + HEAP_MEMORY_SIZE)) {
        do {
            bits = MBitsGet((unsigned char*)addr);
            addr = (unsigned char*)addr + PAGESIZE;
            i += PAGESIZE;
        } while (bits != (PUSED | PLAST));
    }
    return i;
}

void* ReAllocMemory(void* addr, size_t msize) {
    int size = MemSize(addr);
    if (msize <= (size_t)size)return addr;
    void* newaddr = GetMemory(msize);
    if (addr != NULL && size != 0) {
        memcpy(newaddr, addr, MemSize(addr));
        FreeMemory((unsigned char *)addr);
        addr = NULL;

    }
    return newaddr;
}


extern "C" void InitHeap(void) {
    int i;
    for (i = 0; i < (HEAP_MEMORY_SIZE / PAGESIZE) / PAGESPERWORD; i++) mmap[i] = 0;
    for (i = 0; i < MAXTEMPSTRINGS; i++) StrTmp[i] = NULL;
}



extern "C" void* GetTempMemory(int NbrBytes) {
    if(StrTmpIndex >= MAXTEMPSTRINGS) error((char *)"Not enough memory");
    StrTmpLocalIndex[StrTmpIndex] = LocalIndex;
    StrTmp[StrTmpIndex] = (volatile char *)GetMemory(NbrBytes);
    TempMemoryIsChanged = true;
    return (void*)StrTmp[StrTmpIndex++];
}



extern "C" void *GetMemory(int size) {
    unsigned int j, n;
    unsigned char* addr;
    j = n = (size + PAGESIZE - 1) / PAGESIZE;                         // nbr of pages rounded up
    for (addr = MMHeap + HEAP_MEMORY_SIZE - PAGESIZE; addr >= MMHeap; addr -= PAGESIZE) {
        if(!(MBitsGet(addr) & PUSED)) {
            if(--n == 0) {                                          // found a free slot
                j--;
                MBitsSet(addr + (j * PAGESIZE), PUSED | PLAST);     // show that this is used and the last in the chain of pages
                while (j--) MBitsSet(addr + (j * PAGESIZE), PUSED);  // set the other pages to show that they are used
                memset(addr, 0, size);                              // zero the memory
 //               dp("alloc = %p (%d)", addr, size);
                return (void*)addr;
            }
        }
        else
            n = j;                                                  // not enough space here so reset our count
    }
    // out of memory
    TempStringClearStart = 0;
    ClearTempMemory();                                               // hopefully this will give us enough to print the prompt
    error((char *)"Not enough memory");
    return NULL;                                                    // keep the compiler happy
}
// clear any temporary string spaces (these last for just the life of a command) and return the memory to the heap
// this will not clear memory allocated with a local index less than LocalIndex, sub/funs will increment LocalIndex
// and this prevents the automatic use of ClearTempMemory from clearing memory allocated before calling the sub/fun
extern "C" void ClearTempMemory(void) {
    while (StrTmpIndex > 0) {
        if(StrTmpLocalIndex[StrTmpIndex - 1] >= LocalIndex) {
            StrTmpIndex--;
            FreeMemory((unsigned char*)StrTmp[StrTmpIndex]);
            StrTmp[StrTmpIndex] = NULL;
            TempMemoryIsChanged = false;
        }
        else
            break;
    }
}



void ClearSpecificTempMemory(void* addr) {
    int i;
    for (i = 0; i < StrTmpIndex; i++) {
        if(StrTmp[i] == addr) {
            FreeMemory((unsigned char *)addr);
            StrTmp[i] = NULL;
            StrTmpIndex--;
            while (i < StrTmpIndex) {
                StrTmp[i] = StrTmp[i + 1];
                StrTmpLocalIndex[i] = StrTmpLocalIndex[i + 1];
                i++;
            }
            return;
        }
    }
}
extern "C" void FreeMemorySafe(void** addr) {
    if(*addr != NULL) {
        if(*addr >= (void*)MMHeap && *addr < (void*)(MMHeap + HEAP_MEMORY_SIZE)) { FreeMemory((unsigned char *)(*addr)); *addr = NULL; }
    }
}
void m_alloc(int type) {
    switch (type) {
    case M_PROG:    // this is called initially in InitBasic() to set the base pointer for program memory
                    // everytime the program size is adjusted up or down this must be called to check for memory overflow
        memset(MMHeap, 0, HEAP_MEMORY_SIZE);
        break;

    case M_VAR:     // this must be called to initialises the variable memory pointer
                    // everytime the variable table is increased this must be called to verify that enough memory is free
        memset(vartbl, 0, MAXVARS * sizeof(struct s_vartbl));
        break;
    }
}
extern "C" int FreeSpaceOnHeap(void) {
    unsigned int nbr;
    unsigned char* addr;
    nbr = 0;
    for (addr = MMHeap + HEAP_MEMORY_SIZE - PAGESIZE; addr >= MMHeap; addr -= PAGESIZE)
        if (!(MBitsGet(addr) & PUSED)) nbr++;
    return nbr * PAGESIZE;
}

extern "C" unsigned int UsedHeap(void) {
    unsigned int nbr;
    unsigned char* addr;
    nbr = 0;
    for (addr = MMHeap + HEAP_MEMORY_SIZE - PAGESIZE; addr >= MMHeap; addr -= PAGESIZE)
        if (MBitsGet(addr) & PUSED) nbr++;
    return nbr * PAGESIZE;
}
