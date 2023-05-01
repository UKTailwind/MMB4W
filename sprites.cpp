/***********************************************************************************************************************
MMBasic for Windows

sprites.cpp

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

************************************************************************************************************************/#include "MMBasic_Includes.h"
#include "MainThread.h"
#include "MMBasic_Includes.h"
int layer_in_use[MAXLAYER + 1];
unsigned char LIFO[MAXBLITBUF];
unsigned char zeroLIFO[MAXBLITBUF];
#define min(a, b) ((a < b) ? a : b)
#define max(a, b) ((a < b) ? b : a)
int LIFOpointer = 0;
int zeroLIFOpointer = 0;
int sprites_in_use = 0;
char* COLLISIONInterrupt = NULL;
int CollisionFound = false;
int sprite_which_collided = -1;
static int hideall = 0;
struct blitbuffer blitbuff[MAXBLITBUF] = { 0 };		
extern "C" void fullfilename(char* infile, char* outfile, const char* extension);
//Buffer pointers for the BLIT command
void LIFOadd(int n) {
    int i, j = 0;
    for (i = 0; i < LIFOpointer; i++) {
        if (LIFO[i] != n) {
            LIFO[j] = LIFO[i];
            j++;
        }
    }
    LIFO[j] = n;
    LIFOpointer = j + 1;
}
void LIFOremove(int n) {
    int i, j = 0;
    for (i = 0; i < LIFOpointer; i++) {
        if (LIFO[i] != n) {
            LIFO[j] = LIFO[i];
            j++;
        }
    }
    LIFOpointer = j;
}
void LIFOswap(int n, int m) {
    int i;
    for (i = 0; i < LIFOpointer; i++) {
        if (LIFO[i] == n)LIFO[i] = m;
    }
}
void zeroLIFOadd(int n) {
    int i, j = 0;
    for (i = 0; i < zeroLIFOpointer; i++) {
        if (zeroLIFO[i] != n) {
            zeroLIFO[j] = zeroLIFO[i];
            j++;
        }
    }
    zeroLIFO[j] = n;
    zeroLIFOpointer = j + 1;
}
void zeroLIFOremove(int n) {
    int i, j = 0;
    for (i = 0; i < zeroLIFOpointer; i++) {
        if (zeroLIFO[i] != n) {
            zeroLIFO[j] = zeroLIFO[i];
            j++;
        }
    }
    zeroLIFOpointer = j;
}
void zeroLIFOswap(int n, int m) {
    int i;
    for (i = 0; i < zeroLIFOpointer; i++) {
        if (zeroLIFO[i] == n)zeroLIFO[i] = m;
    }
}

extern "C" void closeallsprites(void) {
    int i;
    for (i = 0; i < MAXBLITBUF; i++) {
        if (i <= MAXLAYER)layer_in_use[i] = 0;
        if (i) {
            if (blitbuff[i].mymaster == -1) {
                if (blitbuff[i].blitbuffptr != NULL) {
                    FreeMemory((unsigned char *)blitbuff[i].blitbuffptr);
                    blitbuff[i].blitbuffptr = NULL;
                }
            }
            if (blitbuff[i].blitstoreptr != NULL) {
                FreeMemory((unsigned char*)blitbuff[i].blitstoreptr);
                blitbuff[i].blitstoreptr = NULL;
            }
        }
        blitbuff[i].blitbuffptr = NULL;
        blitbuff[i].blitstoreptr = NULL;
        blitbuff[i].master = -1;
        blitbuff[i].mymaster = -1;
        blitbuff[i].x = 10000;
        blitbuff[i].y = 10000;
        blitbuff[i].w = 0;
        blitbuff[i].h = 0;
        blitbuff[i].next_x = 10000;
        blitbuff[i].next_y = 10000;
        blitbuff[i].bc = 0;
        blitbuff[i].layer = -1;
        blitbuff[i].active = false;
        blitbuff[i].edges = 0;
    }
    LIFOpointer = 0;
    zeroLIFOpointer = 0;
    sprites_in_use = 0;
    hideall = 0;
}
void fun_sprite(void) {
    int bnbr = 0, w = -1, h = -1, t = 0, x = 10000, y = 10000, l = 0, n, c = 0;
    getargs(&ep, 5, (unsigned char *)",");
    if (checkstring(argv[0], (unsigned char*)"W")) t = 1;
    else if (checkstring(argv[0], (unsigned char*)"H")) t = 2;
    else if (checkstring(argv[0], (unsigned char*)"X")) t = 3;
    else if (checkstring(argv[0], (unsigned char*)"Y")) t = 4;
    else if (checkstring(argv[0], (unsigned char*)"L")) t = 5;
    else if (checkstring(argv[0], (unsigned char*)"C")) t = 6;
    else if (checkstring(argv[0], (unsigned char*)"V")) t = 7;
    else if (checkstring(argv[0], (unsigned char*)"T")) t = 8;
    else if (checkstring(argv[0], (unsigned char*)"E")) t = 9;
    else if (checkstring(argv[0], (unsigned char*)"D")) t = 10;
    else if (checkstring(argv[0], (unsigned char*)"A")) t = 11;
    else if (checkstring(argv[0], (unsigned char*)"N")) t = 12;
    else if (checkstring(argv[0], (unsigned char*)"S")) t = 13;
    else error((char *)"Syntax");
    if (t < 12) {
        if (argc < 3)error((char *)"Syntax");
        if (*argv[2] == '#') argv[2]++;
        bnbr = (int)getint(argv[2], 0, MAXBLITBUF - 1);
        if (bnbr == 0) {
            if (argc == 5 && !(t == 7 || t == 10)) {
                n = (int)getint(argv[4], 1, blitbuff[0].collisions[0]);
                c = blitbuff[0].collisions[n];
            }
            else c = blitbuff[0].collisions[0];
        }
        if (blitbuff[bnbr].blitbuffptr != NULL) {
            w = blitbuff[bnbr].w;
            h = blitbuff[bnbr].h;
        }
        if (blitbuff[bnbr].active) {
            x = blitbuff[bnbr].x;
            y = blitbuff[bnbr].y;
            l = blitbuff[bnbr].layer;
            if (argc == 5 && !(t == 7 || t == 10)) {
                n = (int)getint(argv[4], 1, blitbuff[bnbr].collisions[0]);
                c = blitbuff[bnbr].collisions[n];
            }
            else c = blitbuff[bnbr].collisions[0];
        }
    }
    if (t == 1)iret = w;
    else if (t == 2)iret = h;
    else if (t == 3) { if (blitbuff[bnbr].active)iret = x; else iret = 10000; }
    else if (t == 4) { if (blitbuff[bnbr].active)iret = y; else iret = 10000; }
    else if (t == 5) { if (blitbuff[bnbr].active)iret = l; else iret = -1; }
    else if (t == 8) { if (blitbuff[bnbr].active)iret = blitbuff[bnbr].lastcollisions; else iret = 0; }
    else if (t == 9) { if (blitbuff[bnbr].active)iret = blitbuff[bnbr].edges; else iret = 0; }
    else if (t == 6) { if (blitbuff[bnbr].collisions[0])iret = c; else iret = -1; }
    else if (t == 11) iret = (int64_t)((uint32_t)blitbuff[bnbr].blitbuffptr);
    else if (t == 7) {
        int rbnbr = 0;
        int x1 = 0, y1 = 0, h1 = 0, w1 = 0;
        double vector;
        if (argc < 5)error((char *)"Syntax");
        if (*argv[4] == '#') argv[4]++;
        rbnbr = (int)getint(argv[4], 1, MAXBLITBUF - 1);
        if (blitbuff[rbnbr].blitbuffptr != NULL) {
            w1 = blitbuff[rbnbr].w;
            h1 = blitbuff[rbnbr].h;
        }
        if (blitbuff[rbnbr].active) {
            x1 = blitbuff[rbnbr].x;
            y1 = blitbuff[rbnbr].y;
        }
        if (!(blitbuff[bnbr].active && blitbuff[rbnbr].active))fret = -1.0;
        else {
            x += w / 2;
            y += h / 2;
            x1 += w1 / 2;
            y1 += h1 / 2;
            y1 -= y;
            x1 -= x;
            vector = atan2(y1, x1);
            vector += PI_VALUE / 2.0;
            if (vector < 0)vector += PI_VALUE * 2.0;
            fret = vector;
        }
        targ = T_NBR;
        return;
    }
    else if (t == 10) {
        int rbnbr = 0;
        int x1 = 0, y1 = 0, h1 = 0, w1 = 0;
        if (argc < 5)error((char *)"Syntax");
        if (*argv[4] == '#') argv[4]++;
        rbnbr = (int)getint(argv[4], 1, MAXBLITBUF - 1);
        if (blitbuff[rbnbr].blitbuffptr != NULL) {
            w1 = blitbuff[rbnbr].w;
            h1 = blitbuff[rbnbr].h;
        }
        if (blitbuff[rbnbr].active) {
            x1 = blitbuff[rbnbr].x;
            y1 = blitbuff[rbnbr].y;
        }
        if (!(blitbuff[bnbr].active && blitbuff[rbnbr].active))fret = -1.0;
        else {
            x += w / 2;
            y += h / 2;
            x1 += w1 / 2;
            y1 += h1 / 2;
            fret = sqrt((x1 - x) * (x1 - x) + (y1 - y) * (y1 - y));
        }
        targ = T_NBR;
        return;
    }
    else if (t == 12) {
        if (argc == 3) {
            n = (int)getint(argv[2], 0, MAXLAYER);
            iret = layer_in_use[n];
        }
        else iret = sprites_in_use;
    }
    else if (t == 13) iret = sprite_which_collided;
    else {
    }
    targ = T_INT;
}
void checklimits(int bnbr, int* n) {
    int maxW = PageTable[WritePage].xmax;
    int maxH = PageTable[WritePage].ymax;
    blitbuff[bnbr].collisions[*n] = 0;
    if (blitbuff[bnbr].x < 0) {
        if (!(blitbuff[bnbr].edges & 1)) {
            blitbuff[bnbr].edges |= 1;
            blitbuff[bnbr].collisions[*n] = (char)0xF1;
            (*n)++;
        }
    }
    else blitbuff[bnbr].edges &= ~1;

    if (blitbuff[bnbr].y < 0) {
        if (!(blitbuff[bnbr].edges & 2)) {
            blitbuff[bnbr].edges |= 2;
            if (blitbuff[bnbr].collisions[*n] & 0xF0)blitbuff[bnbr].collisions[*n] |= 0xF2;
            else {
                blitbuff[bnbr].collisions[*n] = (char)0xF2;
                (*n)++;
            }
        }
    }
    else blitbuff[bnbr].edges &= ~2;

    if (blitbuff[bnbr].x + blitbuff[bnbr].w > maxW) {
        if (!(blitbuff[bnbr].edges & 4)) {
            blitbuff[bnbr].edges |= 4;
            if (blitbuff[bnbr].collisions[*n] & 0xF0)blitbuff[bnbr].collisions[*n] |= 0xF4;
            else {
                blitbuff[bnbr].collisions[*n] = (char)0xF4;
                (*n)++;
            }
        }
    }
    else blitbuff[bnbr].edges &= ~4;

    if (blitbuff[bnbr].y + blitbuff[bnbr].h > maxH) {
        if (!(blitbuff[bnbr].edges & 8)) {
            blitbuff[bnbr].edges |= 8;
            if (blitbuff[bnbr].collisions[*n] & 0xF0)blitbuff[bnbr].collisions[*n] |= 0xF8;
            else {
                blitbuff[bnbr].collisions[*n] = (char)0xF8;
                (*n)++;
            }
        }
    }
    else blitbuff[bnbr].edges &= ~8;
}

void ProcessCollisions(int bnbr) {
    int k, j = 1, n = 1, bcol = 1;
    //We know that any collision is caused by movement of sprite bnbr
    // a value of zero indicates that we are processing movement of layer 0 and any
    // sprites on that layer
    CollisionFound = false;
    sprite_which_collided = -1;
    uint64_t mask, mymask = (uint64_t)1 << ((uint64_t)bnbr - (uint64_t)1);
    memset(blitbuff[0].collisions, 0, MAXCOLLISIONS);
    if (bnbr != 0) { // a specific sprite has moved
        memset(blitbuff[bnbr].collisions, 0, MAXCOLLISIONS); //clear our previous collisions
        if (blitbuff[bnbr].layer != 0) {
            if (layer_in_use[blitbuff[bnbr].layer] + layer_in_use[0] > 1) { //other sprites in this layer
                for (k = 1; k < MAXBLITBUF; k++) {
                    mask = (uint64_t)1 << ((uint64_t)k - (uint64_t)1);
                    if (!(blitbuff[k].active)) {
                        blitbuff[bnbr].lastcollisions &= ~mask;
                        continue;
                    }
                    if (k == bnbr) continue;
                    if (j == layer_in_use[blitbuff[bnbr].layer] + layer_in_use[0]) break; //nothing left to process
                    if ((blitbuff[k].layer == blitbuff[bnbr].layer || blitbuff[k].layer == 0)) {
                        j++;
                        if (!(blitbuff[k].x + blitbuff[k].w < blitbuff[bnbr].x ||
                            blitbuff[k].x > blitbuff[bnbr].x + blitbuff[bnbr].w ||
                            blitbuff[k].y + blitbuff[k].h < blitbuff[bnbr].y ||
                            blitbuff[k].y > blitbuff[bnbr].y + blitbuff[bnbr].h)) {
                            if (n < MAXCOLLISIONS && !(blitbuff[bnbr].lastcollisions & mask))blitbuff[bnbr].collisions[n++] = k;
                            blitbuff[bnbr].lastcollisions |= mask;
                            blitbuff[k].lastcollisions |= mymask;
                        }
                        else {
                            blitbuff[bnbr].lastcollisions &= ~mask;
                            blitbuff[k].lastcollisions &= ~mymask;
                        }
                    }
                }
            }
        }
        else {
            for (k = 1; k < MAXBLITBUF; k++) {
                if (j == sprites_in_use) break; //nothing left to process
                if (k == bnbr) continue;
                mask = (uint64_t)1 << ((uint64_t)k - (uint64_t)1);
                if (!(blitbuff[k].active)) {
                    blitbuff[bnbr].lastcollisions &= ~mask;
                    continue;
                }
                else j++;
                if (!(blitbuff[k].x + blitbuff[k].w < blitbuff[bnbr].x ||
                    blitbuff[k].x > blitbuff[bnbr].x + blitbuff[bnbr].w ||
                    blitbuff[k].y + blitbuff[k].h < blitbuff[bnbr].y ||
                    blitbuff[k].y > blitbuff[bnbr].y + blitbuff[bnbr].h)) {
                    if (n < MAXCOLLISIONS && !(blitbuff[bnbr].lastcollisions & mask))blitbuff[bnbr].collisions[n++] = k;
                    blitbuff[bnbr].lastcollisions |= mask;
                    blitbuff[k].lastcollisions |= mymask;
                }
                else {
                    blitbuff[bnbr].lastcollisions &= ~mask;
                    blitbuff[k].lastcollisions &= ~mymask;
                }
            }

        }
        // now look for collisions with the edge of the screen
        checklimits(bnbr, &n);
        if (n > 1) {
            CollisionFound = true;
            sprite_which_collided = bnbr;
            blitbuff[bnbr].collisions[0] = n - 1;
        }
    }
    else { //the background layer has moved
        j = 0;
        for (k = 1; k < MAXBLITBUF; k++) { //loop through all sprites
            mask = (uint64_t)1 << ((uint64_t)k - (uint64_t)1);
            n = 1;
            int kk, jj = 1;
            if (j == sprites_in_use) break; //nothing left to process
            if (blitbuff[k].active) { //sprite found
                memset(blitbuff[k].collisions, 0, MAXCOLLISIONS);
                j++;
                if (layer_in_use[blitbuff[k].layer] + layer_in_use[0] > 1) { //other sprites in this layer
                    for (kk = 1; kk < MAXBLITBUF; kk++) {
                        if (kk == k) continue;
                        if (jj == layer_in_use[blitbuff[k].layer] + layer_in_use[0]) break; //nothing left to process
                        if ((blitbuff[kk].layer == blitbuff[k].layer || blitbuff[kk].layer == 0)) {
                            jj++;
                            if (!(blitbuff[kk].x + blitbuff[kk].w < blitbuff[k].x ||
                                blitbuff[kk].x > blitbuff[k].x + blitbuff[k].w ||
                                blitbuff[kk].y + blitbuff[kk].h < blitbuff[k].y ||
                                blitbuff[kk].y > blitbuff[k].y + blitbuff[k].h)) {
                                if (n < MAXCOLLISIONS && !(blitbuff[k].lastcollisions & mask))blitbuff[k].collisions[n++] = kk;
                                blitbuff[k].lastcollisions |= mask;
                            }
                            else {
                                blitbuff[k].lastcollisions &= ~mask;
                            }
                        }
                    }
                }
                checklimits(k, &n);
                if (n > 1 && n < MAXCOLLISIONS && bcol < MAXCOLLISIONS) {
                    blitbuff[0].collisions[bcol] = k;
                    bcol++;
                    blitbuff[k].collisions[0] = n - 1;
                }
            }
        }
        if (bcol > 1) {
            CollisionFound = true;
            sprite_which_collided = 0;
            blitbuff[0].collisions[0] = bcol - 1;
        }
    }
}

void blithide(int bnbr, int free) {
    int w, h, x1, y1;
    w = blitbuff[bnbr].w;
    h = blitbuff[bnbr].h;
    x1 = blitbuff[bnbr].x;
    y1 = blitbuff[bnbr].y;
    blitbuff[bnbr].active = 0;
    DrawBuffer(x1, y1, x1 + w - 1, y1 + h - 1, (uint32_t *)blitbuff[bnbr].blitstoreptr);
}

void BlitShowBuffer(int bnbr, int x1, int y1, int mode) {
    char* current, * r, * rr, * q, * qq;
    int x, y, rotation, linelength;
    rotation = blitbuff[bnbr].rotation;
    ReadPage = WritePage;
    union colourmap
    {
        char rgbbytes[4];
        short rgb[2];
        int r;
    } a, b ;
    int w, h;
    if (blitbuff[bnbr].blitbuffptr != NULL) {
        qq = q = blitbuff[bnbr].blitbuffptr;
        w = blitbuff[bnbr].w;
        h = blitbuff[bnbr].h;
        linelength = w * 4;
        current = blitbuff[bnbr].blitstoreptr;
        if (!(mode == 0 || mode & 4) && blitbuff[bnbr].active) {
            DrawBuffer(blitbuff[bnbr].x, blitbuff[bnbr].y, blitbuff[bnbr].x + w - 1, blitbuff[bnbr].y + h - 1, (uint32_t*)current);
        }
        blitbuff[bnbr].x = x1;
        blitbuff[bnbr].y = y1;
        if (!(mode == 2))ReadBuffer(x1, y1, x1 + w - 1, y1 + h - 1, (uint32_t*)current);
        // we now have the old screen image stored together with the coordinates
        rr = r = (char *)GetMemory(w * h * 4);
        a.r = 0;
        b.r = 0;
        for (y = 0; y < h; y++) {
            if (rotation < 2)qq = q + linelength * y;
            else qq = q + (h - y - 1) * linelength;
            if (rotation == 1 || rotation == 3)qq += linelength;
            for (x = 0; x < w; x++) {
                if (rotation == 1 || rotation == 3) {
                    a.rgbbytes[3] = *--qq;
                    a.rgbbytes[2] = *--qq;
                    a.rgbbytes[1] = *--qq;
                    a.rgbbytes[0] = *--qq;
                }
                else {
                    a.rgbbytes[0] = *qq++;
                    a.rgbbytes[1] = *qq++;
                    a.rgbbytes[2] = *qq++;
                    a.rgbbytes[3] = *qq++;
                }
                b.rgbbytes[0] = *current++;
                b.rgbbytes[1] = *current++;
                b.rgbbytes[2] = *current++;
                b.rgbbytes[3] = *current++;
                if (a.r != blitbuff[bnbr].bc || mode & 8) {
                    *r++ = a.rgbbytes[0];
                    *r++ = a.rgbbytes[1];
                    *r++ = a.rgbbytes[2];
                    *r++ = a.rgbbytes[3];
                }
                else {
                    *r++ = b.rgbbytes[0];
                    *r++ = b.rgbbytes[1];
                    *r++ = b.rgbbytes[2];
                    *r++ = b.rgbbytes[3];
                }
            }
        }
        DrawBuffer(x1, y1, x1 + w - 1, y1 + h - 1, (uint32_t*)rr);
        if (!(mode & 4))blitbuff[bnbr].active = 1;
        FreeMemory((unsigned char *)rr);
    }
}

int sumlayer(void) {
    int i, j = 0;
    for (i = 0; i <= MAXLAYER; i++)j += layer_in_use[i];
    return j;
}
void loadarray(unsigned char* p) {
    int bnbr, w, h, size, i;
    int maxH = PageTable[WritePage].ymax;
    int maxW = PageTable[WritePage].xmax;
    void* ptr1 = NULL;
    MMFLOAT* a3float = NULL;
    int64_t* a3int = NULL;
    char* q;
    uint16_t* qq;
    uint32_t* qqq;
    getargs(&p, 7, (unsigned char *)",");
    if (*argv[0] == '#') argv[0]++;
    bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);
    if (blitbuff[bnbr].blitbuffptr == NULL) {
        w = (int)getint(argv[2], 1, maxW);
        h = (int)getint(argv[4], 1, maxH);
        ptr1 = findvar(argv[6], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
        if (vartbl[VarIndex].type & T_NBR) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
                error((char *)"Argument 1 must be array");
            }
            a3float = (MMFLOAT*)ptr1;
        }
        else if (vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
                error((char *)"Argument 1 must be array");
            }
            a3int = (int64_t*)ptr1;
        }
        else error((char *)"Argument 1 must be array");
        size = (vartbl[VarIndex].dims[0] - OptionBase);
        if (size < w * h - 1)error((char *)"Array Dimensions");
        blitbuff[bnbr].blitbuffptr = (char *)GetMemory(w * h * sizeof(uint32_t));
        blitbuff[bnbr].blitstoreptr = (char *)GetMemory(w * h * sizeof(uint32_t));
        blitbuff[bnbr].bc = 0;
        blitbuff[bnbr].w = w;
        blitbuff[bnbr].h = h;
        blitbuff[bnbr].master = 0;
        blitbuff[bnbr].mymaster = -1;
        blitbuff[bnbr].x = 10000;
        blitbuff[bnbr].y = 10000;
        blitbuff[bnbr].layer = -1;
        blitbuff[bnbr].next_x = 10000;
        blitbuff[bnbr].next_y = 10000;
        blitbuff[bnbr].active = false;
        blitbuff[bnbr].lastcollisions = 0;
        blitbuff[bnbr].edges = 0;
        q = blitbuff[bnbr].blitbuffptr;
        qq = (uint16_t*)q;
        qqq = (uint32_t*)q;
            int c;
            for (i = 0; i < w * h; i++) {
                if (a3float)c = (int)a3float[i];
                else c = (int)a3int[i];
                *qqq++ = c;
            }
    }
    else error((char *)"Buffer already in use");
}
void loadpng(unsigned char* p) {
    upng_t* upng;
    union colourmap
    {
        unsigned char rgbbytes[4];
        unsigned short rgb[2];
        unsigned int r;
    } c;
    int maxW = PageTable[WritePage].xmax;
    int maxH = PageTable[WritePage].ymax;
    int bnbr, i, w, h, deftrans = 8;
    char* q;
    uint16_t* qq;
    uint32_t* qqq;
    getargs(&p, 5, (unsigned char *)",");
    if (*argv[0] == '#') argv[0]++;
    bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);
    if (blitbuff[bnbr].blitbuffptr == NULL) {
        unsigned char* p = getFstring(argv[2]);
        if (argc == 5)deftrans = (int)getint(argv[4], 1, 15);
        upng = upng_new_from_file((char *)p);
        upng_header(upng);
        if ((int)upng_get_width(upng) >= maxW || (int)upng_get_height(upng) >= maxH) {
            upng_free(upng);
            error((char *)"Image too large");
        }
        if (!(upng_get_format(upng) == 1 || upng_get_format(upng) == 3)) {
            upng_free(upng);
            error((char *)"Invalid format");
        }
        upng_decode(upng);
        w = upng_get_width(upng);
        h = upng_get_height(upng);
        //        PInt(w*h*sizeof(uint32_t));PRet();MM_Delay(500);
        blitbuff[bnbr].blitbuffptr = (char *)GetMemory(w * h * sizeof(uint32_t));
        blitbuff[bnbr].blitstoreptr = (char*)GetMemory(w * h * sizeof(uint32_t));
        blitbuff[bnbr].bc = 0;
        blitbuff[bnbr].w = w;
        blitbuff[bnbr].h = h;
        blitbuff[bnbr].master = 0;
        blitbuff[bnbr].mymaster = -1;
        blitbuff[bnbr].x = 10000;
        blitbuff[bnbr].y = 10000;
        blitbuff[bnbr].layer = -1;
        blitbuff[bnbr].next_x = 10000;
        blitbuff[bnbr].next_y = 10000;
        blitbuff[bnbr].active = false;
        blitbuff[bnbr].lastcollisions = 0;
        blitbuff[bnbr].edges = 0;
        q = blitbuff[bnbr].blitbuffptr;
        qq = (uint16_t*)q;
        qqq = (uint32_t*)q;
            const unsigned char* rr;
            rr = upng_get_buffer(upng);
            i = upng_get_width(upng) * upng_get_height(upng) * 4;
            c.rgbbytes[3] = 0;
            while (i) {
                c.rgbbytes[3] = 0;
                c.rgbbytes[0] = *rr++;
                c.rgbbytes[1] = *rr++;
                c.rgbbytes[2] = *rr++;
                if (upng_get_format(upng) == 3)c.rgbbytes[3] = *rr++;
                else c.rgbbytes[3] = 0xFF;
                *qqq++ = c.r;
                i -= 4;
            }
        upng_free(upng);
    }
    else error((char *)"Buffer already in use");
}
void hidesafe(int bnbr) {
    int found = 0;
    int i;
    for (i = LIFOpointer - 1; i >= 0; i--) {
        if (LIFO[i] == bnbr) {
            blithide(LIFO[i], 0);
            found = i;
            break;
        }
        blithide(LIFO[i], 0);
    }
    if (!found) {
        for (i = zeroLIFOpointer - 1; i >= 0; i--) {
            if (zeroLIFO[i] == bnbr) {
                blithide(zeroLIFO[i], 0);
                found = -i;
                break;
            }
            blithide(zeroLIFO[i], 0);
        }
    }
    sprites_in_use--;
    layer_in_use[blitbuff[bnbr].layer]--;
    blitbuff[bnbr].x = 10000;
    blitbuff[bnbr].y = 10000;
    if (blitbuff[bnbr].layer == 0)zeroLIFOremove(bnbr);
    else LIFOremove(bnbr);
    blitbuff[bnbr].layer = -1;
    blitbuff[bnbr].next_x = 10000;
    blitbuff[bnbr].next_y = 10000;
    blitbuff[bnbr].lastcollisions = 0;
    blitbuff[bnbr].edges = 0;
    if (found < 0) {
        found = -found;
        for (i = found; i < zeroLIFOpointer; i++) {
            BlitShowBuffer(zeroLIFO[i], blitbuff[zeroLIFO[i]].x, blitbuff[zeroLIFO[i]].y, 0);
        }
        for (i = 0; i < LIFOpointer; i++) {
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
        }
    }
    else {
        for (i = found; i < LIFOpointer; i++) {
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
        }
    }
}

void showsafe(int bnbr, int x, int y) {
    int found = 0;
    int i;
    for (i = LIFOpointer - 1; i >= 0; i--) {
        if (LIFO[i] == bnbr) {
            blithide(LIFO[i], 0);
            found = i;
            break;
        }
        blithide(LIFO[i], 0);
    }
    if (!found) {
        for (i = zeroLIFOpointer - 1; i >= 0; i--) {
            if (zeroLIFO[i] == bnbr) {
                blithide(zeroLIFO[i], 0);
                found = -i;
                break;
            }
            blithide(zeroLIFO[i], 0);
        }
    }
    BlitShowBuffer(bnbr, x, y, 1);
    if (found < 0) {
        found = -found;
        for (i = found + 1; i < zeroLIFOpointer; i++) {
            BlitShowBuffer(zeroLIFO[i], blitbuff[zeroLIFO[i]].x, blitbuff[zeroLIFO[i]].y, 0);
        }
        for (i = 0; i < LIFOpointer; i++) {
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
        }
    }
    else {
        for (i = found + 1; i < LIFOpointer; i++) {
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
        }
    }

}
void loadsprite(unsigned char* p) {
    int fnbr, width, number, height = 0, newsprite = 1, startsprite = 1, bnbr, lc, i;
    char filename[STRINGSIZE] = { 0 };
    char* q, * fname;
    short* qq;
    uint32_t* qqq=NULL;
    char buff[256];
    getargs(&p, 3, (unsigned char *)",");
    fnbr = FindFreeFileNbr();
    fname = (char*)getFstring(argv[0]);
    if (argc == 3)startsprite = (int)getint(argv[2], 1, 64);
    fullfilename(fname, filename, ".SPR");
    if (!BasicFileOpen(filename, fnbr, (char *)"rb")) error((char *)"File not found");
    MMgetline(fnbr, (char*)buff);							    // get the input line
    while (buff[0] == 39)MMgetline(fnbr, (char*)buff);
    sscanf((char*)buff, "%d,%d, %d", &width, &number, &height);
    if (height == 0)height = width;
    bnbr = startsprite;
    if (number + startsprite > MAXBLITBUF) {
        FileClose(fnbr);
        error((char *)"Maximum of 64 sprites");
    }
    while (!MMfeof(fnbr) && bnbr <= number + startsprite) {                                     // while waiting for the end of file
        if (newsprite) {
            newsprite = 0;
            if (blitbuff[bnbr].blitbuffptr == NULL)blitbuff[bnbr].blitbuffptr = (char *)GetMemory(width * height *  4);
            if (blitbuff[bnbr].blitstoreptr == NULL)blitbuff[bnbr].blitstoreptr = (char*)GetMemory(width * height * 4);
            blitbuff[bnbr].bc = 0;
            blitbuff[bnbr].w = width;
            blitbuff[bnbr].h = height;
            blitbuff[bnbr].master = 0;
            blitbuff[bnbr].mymaster = -1;
            blitbuff[bnbr].x = 10000;
            blitbuff[bnbr].y = 10000;
            blitbuff[bnbr].layer = -1;
            blitbuff[bnbr].next_x = 10000;
            blitbuff[bnbr].next_y = 10000;
            blitbuff[bnbr].active = false;
            blitbuff[bnbr].lastcollisions = 0;
            blitbuff[bnbr].edges = 0;
            q = blitbuff[bnbr].blitbuffptr;
            qq = (short*)blitbuff[bnbr].blitbuffptr;
            qqq = (uint32_t*)blitbuff[bnbr].blitbuffptr;
            lc = height;
        }
        while (lc--) {
            MMgetline(fnbr, (char*)buff);									    // get the input line
            while (buff[0] == 39)MMgetline(fnbr, (char*)buff);
            if ((int)strlen(buff) < width)memset(&buff[strlen(buff)], 32, width - strlen(buff));
                 for (i = 0; i < width; i++) {
                    if (buff[i] == ' ')*qqq++ = 0;
                    else if (buff[i] == '0')*qqq++ = 0xFF000000;
                    else if (buff[i] == '1')*qqq++ = M_BLUE;
                    else if (buff[i] == '2')*qqq++ = M_GREEN;
                    else if (buff[i] == '3')*qqq++ = M_CYAN;
                    else if (buff[i] == '4')*qqq++ = M_RED;
                    else if (buff[i] == '5')*qqq++ = M_MAGENTA;
                    else if (buff[i] == '6')*qqq++ = M_YELLOW;
                    else if (buff[i] == '7')*qqq++ = M_WHITE;
                    else if (buff[i] == '8')*qqq++ = M_LITEGRAY;
                    else if (buff[i] == '9')*qqq++ = M_GRAY;
                    else if (buff[i] == 'A' || buff[i] == 'a')*qqq++ = M_ORANGE;
                    else if (buff[i] == 'B' || buff[i] == 'b')*qqq++ = M_PINK;
                    else if (buff[i] == 'C' || buff[i] == 'c')*qqq++ = M_GOLD;
                    else if (buff[i] == 'D' || buff[i] == 'd')*qqq++ = M_SALMON;
                    else if (buff[i] == 'E' || buff[i] == 'e')*qqq++ = M_BEIGE;
                    else if (buff[i] == 'F' || buff[i] == 'f')*qqq++ = M_BROWN;
                    else *qqq++ = 0;
                }
            }
        bnbr++;
        newsprite = 1;
    }
    FileClose(fnbr);
}
void cmd_blit(void) {
    int x1, y1, x2, y2, w, h, bnbr;
    int newb = 0;
    int maxW = PageTable[WritePage].xmax;
    int maxH = PageTable[WritePage].ymax;
    unsigned char* p;
    uint32_t *q;
    if ((p = checkstring(cmdline, (unsigned char *)"SHOW SAFE"))) {
        int layer;
        getargs(&p, 11, (unsigned char*)",");
        if (!(argc == 7 || argc == 9 || argc == 11)) error((char *)"Syntax");
        if (hideall)error((char *)"Sprites are hidden");
        if (*argv[0] == '#') argv[0]++;
        bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);									// get the number
        if (blitbuff[bnbr].blitbuffptr != NULL) {
            x1 = (int)getint(argv[2], -blitbuff[bnbr].w + 1, maxW - 1);
            y1 = (int)getint(argv[4], -blitbuff[bnbr].h + 1, maxH - 1);
            layer = (int)getint(argv[6], 0, MAXLAYER);
            if (argc >= 9 && *argv[8])blitbuff[bnbr].rotation = (char)getint(argv[8], 0, 3);
            else blitbuff[bnbr].rotation = 0;
            if (argc == 11 && *argv[10]) {
                newb = (int)getint(argv[10], 0, 1);
            }
            q = (uint32_t*)blitbuff[bnbr].blitbuffptr;
            w = blitbuff[bnbr].w;
            h = blitbuff[bnbr].h;
            if (blitbuff[bnbr].active) {
                if (newb) {
                    hidesafe(bnbr);
                    blitbuff[bnbr].layer = layer;
                    layer_in_use[blitbuff[bnbr].layer]++;
                    if (blitbuff[bnbr].layer == 0) zeroLIFOadd(bnbr);
                    else LIFOadd(bnbr);
                    sprites_in_use++;
                    BlitShowBuffer(bnbr, x1, y1, 1);
                }
                else {
                    showsafe(bnbr, x1, y1);
                }
            }
            else {
                blitbuff[bnbr].layer = layer;
                layer_in_use[blitbuff[bnbr].layer]++;
                if (blitbuff[bnbr].layer == 0) zeroLIFOadd(bnbr);
                else LIFOadd(bnbr);
                sprites_in_use++;
                BlitShowBuffer(bnbr, x1, y1, 1);
            }
            ProcessCollisions(bnbr);
            if (sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error((char *)"sprite internal error");
        }
        else error((char *)"Buffer not in use");
    }
    else if ((p = checkstring(cmdline, (unsigned char*)"SHOW"))) {
        int layer;
        getargs(&p, 9, (unsigned char*)",");
        if (!(argc == 7 || argc == 9)) error((char *)"Syntax");
        if (hideall)error((char *)"Sprites are hidden");
        if (*argv[0] == '#') argv[0]++;
        bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);									// get the number
        if (blitbuff[bnbr].blitbuffptr != NULL) {
            x1 = (int)getint(argv[2], -blitbuff[bnbr].w + 1, maxW - 1);
            y1 = (int)getint(argv[4], -blitbuff[bnbr].h + 1, maxH - 1);
            layer = (int)getint(argv[6], 0, MAXLAYER);
            if (argc == 9)blitbuff[bnbr].rotation = (int)getint(argv[8], 0, 3);
            else blitbuff[bnbr].rotation = 0;
            q = (uint32_t*)blitbuff[bnbr].blitbuffptr;
            w = blitbuff[bnbr].w;
            h = blitbuff[bnbr].h;
            if (blitbuff[bnbr].active) {
                layer_in_use[blitbuff[bnbr].layer]--;
                if (blitbuff[bnbr].layer == 0)zeroLIFOremove(bnbr);
                else LIFOremove(bnbr);
                sprites_in_use--;
            }
            blitbuff[bnbr].layer = layer;
            layer_in_use[blitbuff[bnbr].layer]++;
            if (blitbuff[bnbr].layer == 0) zeroLIFOadd(bnbr);
            else LIFOadd(bnbr);
            sprites_in_use++;
            int cursorhidden = 0;
            BlitShowBuffer(bnbr, x1, y1, 1);
            ProcessCollisions(bnbr);
            if (sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error((char *)"sprite internal error");
        }
        else error((char *)"Buffer not in use");
    }
    else if ((p = checkstring(cmdline, (unsigned char*)"HIDE ALL"))) {
        if (hideall)error((char *)"Sprites are hidden");
        int i;
        int cursorhidden = 0;
        for (i = LIFOpointer - 1; i >= 0; i--) {
            blithide(LIFO[i], 0);
        }
        for (i = zeroLIFOpointer - 1; i >= 0; i--) {
            blithide(zeroLIFO[i], 0);
        }
        hideall = 1;
    }
    else if ((p = checkstring(cmdline, (unsigned char*)"RESTORE"))) {
        if (!hideall)error((char *)"Sprites are not hidden");
        int i;
        int cursorhidden = 0;
        for (i = 0; i < zeroLIFOpointer; i++) {
            BlitShowBuffer(zeroLIFO[i], blitbuff[zeroLIFO[i]].x, blitbuff[zeroLIFO[i]].y, 0);
        }
        for (i = 0; i < LIFOpointer; i++) {
            if (blitbuff[LIFO[i]].next_x != 10000) {
                blitbuff[LIFO[i]].x = blitbuff[LIFO[i]].next_x;
                blitbuff[LIFO[i]].next_x = 10000;
            }
            if (blitbuff[LIFO[i]].next_y != 10000) {
                blitbuff[LIFO[i]].y = blitbuff[LIFO[i]].next_y;
                blitbuff[LIFO[i]].next_y = 10000;
            }
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
        }
        hideall = 0;
        ProcessCollisions(0);
    }
    else if ((p = checkstring(cmdline, (unsigned char*)"HIDE SAFE"))) {
        getargs(&p, 1, (unsigned char*)",");
        if (sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error((char *)"sprite internal error");
        if (argc != 1) error((char *)"Syntax");
        if (*argv[0] == '#') argv[0]++;
        bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);									// get the number
        if (hideall)error((char *)"Sprites are hidden");
        if (blitbuff[bnbr].blitbuffptr != NULL) {
            if (blitbuff[bnbr].active) {
                int cursorhidden = 0;
                hidesafe(bnbr);
                if (sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error((char *)"sprite internal error");
            }
            else error((char *)"Not Showing");
        }
        else error((char *)"Buffer not in use");
    }
    else if ((p = checkstring(cmdline, (unsigned char*)"HIDE"))) {
        getargs(&p, 1, (unsigned char*)",");
        if (argc != 1) error((char *)"Syntax");
        if (*argv[0] == '#') argv[0]++;
        bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);									// get the number
        if (blitbuff[bnbr].blitbuffptr != NULL) {
            if (blitbuff[bnbr].active) {
                sprites_in_use--;
                int cursorhidden = 0;
                blithide(bnbr, 0);
                layer_in_use[blitbuff[bnbr].layer]--;
                blitbuff[bnbr].x = 10000;
                blitbuff[bnbr].y = 10000;
                if (blitbuff[bnbr].layer == 0)zeroLIFOremove(bnbr);
                else LIFOremove(bnbr);
                blitbuff[bnbr].layer = -1;
                blitbuff[bnbr].next_x = 10000;
                blitbuff[bnbr].next_y = 10000;
                blitbuff[bnbr].lastcollisions = 0;
                blitbuff[bnbr].edges = 0;
            }
            else error((char *)"Not Showing");
        }
        else error((char *)"Buffer not in use");
        if (sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error((char *)"sprite internal error");
        //
    }
    else if ((p = checkstring(cmdline, (unsigned char*)"SWAP"))) {
        int rbnbr=0;
        getargs(&p, 5, (unsigned char*)",");
        if (argc < 3) error((char *)"Syntax");
        if (hideall)error((char *)"Sprites are hidden");
        if (*argv[0] == '#') argv[0]++;
        bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);									// get the number
        if (*argv[2] == '#') argv[0]++;
        rbnbr = (int)getint(argv[2], 1, MAXBLITBUF - 1);									// get the number
        if (blitbuff[bnbr].blitbuffptr == NULL || blitbuff[bnbr].active == false) error((char *)"Original buffer not displayed");
        if (!blitbuff[bnbr].active)error((char *)"Original buffer not displayed");
        if (blitbuff[rbnbr].active) error((char *)"New buffer already displayed");
        if (!(blitbuff[rbnbr].w == blitbuff[bnbr].w && blitbuff[rbnbr].h == blitbuff[bnbr].h)) error((char *)"Size mismatch");
        // copy the relevant data
        blitbuff[rbnbr].blitstoreptr = blitbuff[bnbr].blitstoreptr;
        blitbuff[rbnbr].x = blitbuff[bnbr].x;
        blitbuff[rbnbr].y = blitbuff[bnbr].y;
        blitbuff[rbnbr].layer = blitbuff[bnbr].layer;
        blitbuff[rbnbr].lastcollisions = blitbuff[bnbr].lastcollisions;
        if (blitbuff[rbnbr].layer == 0)zeroLIFOswap(bnbr, rbnbr);
        else LIFOswap(bnbr, rbnbr);
        // "Hide" the old sprite
        blitbuff[bnbr].x = 10000;
        blitbuff[bnbr].y = 10000;
        blitbuff[bnbr].layer = -1;
        blitbuff[bnbr].next_x = 10000;
        blitbuff[bnbr].next_y = 10000;
        blitbuff[bnbr].active = 0;
        blitbuff[bnbr].lastcollisions = 0;
        if (argc == 5)blitbuff[rbnbr].rotation = (char)getint(argv[4], 0, 3);
        else blitbuff[rbnbr].rotation = 0;
        BlitShowBuffer(rbnbr, blitbuff[rbnbr].x, blitbuff[rbnbr].y, 2);
        if (sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error((char *)"sprite internal error");

    }
    else if ((p = checkstring(cmdline, (unsigned char*)"READ"))) {
        getargs(&p, 11, (unsigned char*)",");
        if (!(argc == 9 || argc == 11)) error((char *)"Syntax");
        if (*argv[0] == '#') argv[0]++;
        bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);									// get the number
        x1 = (int)getinteger(argv[2]);
        y1 = (int)getinteger(argv[4]);
        w = (int)getinteger(argv[6]);
        h = (int)getinteger(argv[8]);
        if (w < 1 || h < 1) return;
        blitbuff[bnbr].bc = 0;
        if (blitbuff[bnbr].blitbuffptr == NULL) {
            blitbuff[bnbr].blitbuffptr = (char *)GetMemory(w * h * sizeof(uint32_t));
            blitbuff[bnbr].blitstoreptr = (char*)GetMemory(w * h * sizeof(uint32_t));
            blitbuff[bnbr].bc = 0;
            blitbuff[bnbr].w = w;
            blitbuff[bnbr].h = h;
            blitbuff[bnbr].master = 0;
            blitbuff[bnbr].mymaster = -1;
            blitbuff[bnbr].x = 10000;
            blitbuff[bnbr].y = 10000;
            blitbuff[bnbr].layer = -1;
            blitbuff[bnbr].next_x = 10000;
            blitbuff[bnbr].next_y = 10000;
            blitbuff[bnbr].active = false;
            blitbuff[bnbr].lastcollisions = 0;
            blitbuff[bnbr].edges = 0;
            q = (uint32_t*)blitbuff[bnbr].blitbuffptr;
        }
        else {
            if (blitbuff[bnbr].mymaster != -1) error((char *)"Can't read into a copy", bnbr);
            if (blitbuff[bnbr].master > 0) error((char *)"Copies exist", bnbr);
            if (!(blitbuff[bnbr].w == w && blitbuff[bnbr].h == h))error((char *)"Existing buffer is incorrect size");
            q = (uint32_t*)blitbuff[bnbr].blitbuffptr;
        }
        if (argc == 11) {
            if (checkstring(argv[10], (unsigned char*)"FRAMEBUFFER")) {
                ReadPage = WPN;
            }
            else {
                ReadPage = (int)getint(argv[10], 0, MAXPAGES);
            }
        }
        ReadBuffer(x1, y1, x1 + w - 1, y1 + h - 1, (uint32_t *)q);
        ReadPage = WritePage;
    }
    else if ((p = checkstring(cmdline, (unsigned char*)"COPY"))) {
        int cpy, nbr, c1, n1;
        getargs(&p, 5, (unsigned char*)",");
        if (argc != 5) error((char *)"Syntax");
        if (*argv[0] == '#') argv[0]++;
        bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);									// get the number
        if (blitbuff[bnbr].blitbuffptr != NULL) {
            if (*argv[2] == '#') argv[2]++;
            c1 = cpy = (int)getint(argv[2], 1, MAXBLITBUF - 1);
            n1 = nbr = (int)getint(argv[4], 1, MAXBLITBUF - 2);

            while (n1) {
                if (blitbuff[c1].blitbuffptr != NULL)error((char *)"Buffer already in use %", c1);
                if (blitbuff[bnbr].master == -1)error((char *)"Can't copy a copy");;
                n1--;
                c1++;
            }
            while (nbr) {
                blitbuff[cpy].blitbuffptr = blitbuff[bnbr].blitbuffptr;
                blitbuff[cpy].w = blitbuff[bnbr].w;
                blitbuff[cpy].h = blitbuff[bnbr].h;
                blitbuff[cpy].blitstoreptr = (char *)GetMemory(blitbuff[cpy].w * blitbuff[cpy].h * sizeof(uint32_t));
                blitbuff[cpy].bc = blitbuff[bnbr].bc;
                blitbuff[cpy].x = 10000;
                blitbuff[cpy].y = 10000;
                blitbuff[cpy].next_x = 10000;
                blitbuff[cpy].next_y = 10000;
                blitbuff[cpy].layer = -1;
                blitbuff[cpy].mymaster = bnbr;
                blitbuff[cpy].master = -1;
                blitbuff[cpy].edges = 0;
                blitbuff[bnbr].master |= ((int64_t)1 << (int64_t)cpy);
                blitbuff[bnbr].lastcollisions = 0;
                blitbuff[cpy].active = false;
                nbr--;
                cpy++;
            }
        }
        else error((char *)"Buffer not in use");

    }
    else if ((p = checkstring(cmdline, (unsigned char*)"LOADARRAY"))) {
        loadarray(p);

    }
    else if ((p = checkstring(cmdline, (unsigned char*)"LOADPNG"))) {
        loadpng(p);

    }
    else if ((p = checkstring(cmdline, (unsigned char*)"LOAD"))) {
        loadsprite(p);

    }
    else if ((p = checkstring(cmdline, (unsigned char*)"INTERRUPT"))) {
        getargs(&p, 1, (unsigned char*)",");
        COLLISIONInterrupt = (char*)GetIntAddress(argv[0]);					// get the interrupt location
        InterruptUsed = true;
        return;

    }
    else if ((p = checkstring(cmdline, (unsigned char*)"NOINTERRUPT"))) {
        COLLISIONInterrupt = NULL;					// get the interrupt location
        return;

    }
    else if ((p = checkstring(cmdline, (unsigned char*)"CLOSE ALL"))) {
        closeallsprites();

    }
    else if ((p = checkstring(cmdline, (unsigned char*)"CLOSE"))) {
        getargs(&p, 1, (unsigned char*)",");
        if (hideall)error((char *)"Sprites are hidden");
        if (*argv[0] == '#') argv[0]++;
        bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);
        if (blitbuff[bnbr].master > 0) error((char *)"Copies still open");
        if (blitbuff[bnbr].blitbuffptr != NULL) {
            if (blitbuff[bnbr].active) {
               blithide(bnbr, 1);
                if (blitbuff[bnbr].layer == 0)zeroLIFOremove(bnbr);
                else LIFOremove(bnbr);
                layer_in_use[blitbuff[bnbr].layer]--;
                sprites_in_use--;
            }
            if (blitbuff[bnbr].mymaster == -1)FreeMemorySafe((void**)&blitbuff[bnbr].blitbuffptr);
            else blitbuff[blitbuff[bnbr].mymaster].master &= ~(1 << bnbr);
            FreeMemorySafe((void**)&blitbuff[bnbr].blitstoreptr);
            blitbuff[bnbr].blitbuffptr = NULL;
            blitbuff[bnbr].blitstoreptr = NULL;
            blitbuff[bnbr].master = -1;
            blitbuff[bnbr].mymaster = -1;
            blitbuff[bnbr].x = 10000;
            blitbuff[bnbr].y = 10000;
            blitbuff[bnbr].w = 0;
            blitbuff[bnbr].h = 0;
            blitbuff[bnbr].next_x = 10000;
            blitbuff[bnbr].next_y = 10000;
            blitbuff[bnbr].bc = 0;
            blitbuff[bnbr].layer = -1;
            blitbuff[bnbr].active = false;
            blitbuff[bnbr].edges = 0;
        }
        else error((char *)"Buffer not in use");
        if (sprites_in_use != LIFOpointer + zeroLIFOpointer || sprites_in_use != sumlayer())error((char *)"sprite internal error");
    }
    else if ((p = checkstring(cmdline, (unsigned char*)"TRANSPARENCY"))) {
        int x, y, trans;
        getargs(&p, 3, (unsigned char*)",");
        if (!(ARGBenabled))error((char *)"Invalid for this display mode");
        if (argc != 3) error((char *)"Syntax");
        if (*argv[0] == '#') argv[0]++;
        bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);									// get the number
        trans = (int)getint(argv[2], 1, 255);
        if (blitbuff[bnbr].blitbuffptr != NULL) {
            w = blitbuff[bnbr].w;
            h = blitbuff[bnbr].h;
                uint32_t* q = (uint32_t*)blitbuff[bnbr].blitbuffptr;
                for (y = 0; y < h; y++) {
                    for (x = 0; x < w; x++) {
                        if (*q & 0xFF000000) { //only process if not transparent
                            *q &=  0xFFFFFF;
                            *q |= (trans << 24);
                        }
                        q++;
                    }
                }
        }
        else error((char *)"Buffer not in use");

    }
    else if ((p = checkstring(cmdline, (unsigned char*)"NEXT"))) {
        getargs(&p, 5, (unsigned char*)",");
        if (!(argc == 5)) error((char *)"Syntax");
        if (*argv[0] == '#') argv[0]++;
        bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);									// get the number
        blitbuff[bnbr].next_x = (short)getint(argv[2], -blitbuff[bnbr].w + 1, maxW - 1);
        blitbuff[bnbr].next_y = (short)getint(argv[4], -blitbuff[bnbr].h + 1, maxH - 1);
        //
    }
    else if ((p = checkstring(cmdline, (unsigned char*)"WRITE"))) {
        int mode = 4;
        getargs(&p, 7, (unsigned char*)",");
        if (!(argc == 5 || argc == 7)) error((char *)"Syntax");
        if (*argv[0] == '#') argv[0]++;
        bnbr = (int)getint(argv[0], 1, MAXBLITBUF - 1);									// get the number
        if (blitbuff[bnbr].blitbuffptr != NULL) {
            x1 = (int)getint(argv[2], -blitbuff[bnbr].w + 1, maxW);
            y1 = (int)getint(argv[4], -blitbuff[bnbr].h + 1, maxH);
            if (argc == 7)blitbuff[bnbr].rotation = (char)getint(argv[6], 0, 7);
            else blitbuff[bnbr].rotation = 4;
            if ((blitbuff[bnbr].rotation & 4) == 0)mode |= 8;
            blitbuff[bnbr].rotation &= 3;
            q = (uint32_t*)blitbuff[bnbr].blitbuffptr;
            w = blitbuff[bnbr].w;
            h = blitbuff[bnbr].h;
            int cursorhidden = 0;
            BlitShowBuffer(bnbr, x1, y1, mode);
        }
        else error((char *)"Buffer not in use");
    }
    else if ((p = checkstring(cmdline, (unsigned char*)"MOVE"))) {
        if (hideall)error((char *)"Sprites are hidden");
        int i;
        int cursorhidden = 0;
        for (i = LIFOpointer - 1; i >= 0; i--) blithide(LIFO[i], 0);
        for (i = zeroLIFOpointer - 1; i >= 0; i--)blithide(zeroLIFO[i], 0);
        //
        for (i = 0; i < zeroLIFOpointer; i++) {
            if (blitbuff[zeroLIFO[i]].next_x != 10000) {
                blitbuff[zeroLIFO[i]].x = blitbuff[zeroLIFO[i]].next_x;
                blitbuff[zeroLIFO[i]].next_x = 10000;
            }
            if (blitbuff[zeroLIFO[i]].next_y != 10000) {
                blitbuff[zeroLIFO[i]].y = blitbuff[zeroLIFO[i]].next_y;
                blitbuff[zeroLIFO[i]].next_y = 10000;
            }
            BlitShowBuffer(zeroLIFO[i], blitbuff[zeroLIFO[i]].x, blitbuff[zeroLIFO[i]].y, 0);
        }
        for (i = 0; i < LIFOpointer; i++) {
            if (blitbuff[LIFO[i]].next_x != 10000) {
                blitbuff[LIFO[i]].x = blitbuff[LIFO[i]].next_x;
                blitbuff[LIFO[i]].next_x = 10000;
            }
            if (blitbuff[LIFO[i]].next_y != 10000) {
                blitbuff[LIFO[i]].y = blitbuff[LIFO[i]].next_y;
                blitbuff[LIFO[i]].next_y = 10000;
            }
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);

        }
        ProcessCollisions(0);
    }
    else if ((p = checkstring(cmdline, (unsigned char*)"SCROLLR"))) {
        int i, xmin, xmax, ymin, ymax, sx = 0, sy = 0, b1, b2, xs = 10000, ys = 10000;
        int64_t blank = -1;
        int flip = 0;
        char* current3 = NULL, * current4 = NULL;
        getargs(&p, 13, (unsigned char*)",");
        if (hideall)error((char *)"Sprites are hidden");
        if (!(argc == 11 || (argc == 13))) error((char *)"Syntax");
        x1 = (int)getint(argv[0], 0, maxW - 2);
        y1 = (int)getint(argv[2], 0, maxH - 2);
        w = (int)getint(argv[4], 1, maxW - x1);
        h = (int)getint(argv[6], 1, maxH - y1);
        sx = (int)getint(argv[8], 1 - w, w - 1);
        sy = (int)getint(argv[10], 1 - h, h - 1);
        if (sx >= 0) { //convert coordinates for use with BLIT
            x2 = x1 + sx;
            w -= sx;
        }
        else {
            x2 = x1;
            x1 -= sx; //sx is negative so this subtracts it
            w += sx;
        }
        if (sy < 0) { //convert coordinates for use with BLIT
            y2 = y1 - sy;
            h += sy;
        }
        else {
            y2 = y1;
            y1 += sy;
            h -= sy;
        }
        if (argc == 13)blank = getColour(argv[12], 1);
        xmin = min(x1, x2);
        xmax = max(x1, x2) + w - 1;
        ymin = min(y1, y2);
        ymax = max(y1, y2) + h - 1;
        if (w < 1 || h < 1) return;
        if (x1 < 0) { x2 -= x1; w += x1; x1 = 0; }
        if (x2 < 0) { x1 -= x2; w += x2; x2 = 0; }
        if (y1 < 0) { y2 -= y1; h += y1; y1 = 0; }
        if (y2 < 0) { y1 -= y2; h += y2; y2 = 0; }
        if (x1 + w > maxW) w = maxW - x1;
        if (x2 + w > maxW) w = maxW - x2;
        if (y1 + h > maxH) h = maxH - y1;
        if (y2 + h > maxH) h = maxH - y2;
        if (w < 1 || h < 1 || x1 < 0 || x1 + w > maxW || x2 < 0 || x2 + w > maxW || y1 < 0 || y1 + h > maxH || y2 < 0 || y2 + h > maxH) return;

        b1 = RoundUptoInt(abs(sx)) * ymax;
        b2 = RoundUptoInt(xmax) * abs(sy);
        if (blank < 0) {
            if (x1 != x2)current3 = (char *)GetMemory(b1 * sizeof(uint32_t));
            if (y1 != y2)current4 = (char *)GetMemory(b2 * sizeof(uint32_t));
        }

        for (i = LIFOpointer - 1; i >= 0; i--) {
            blithide(LIFO[i], 0);

        }

        for (i = zeroLIFOpointer - 1; i >= 0; i--) {
            xs = blitbuff[zeroLIFO[i]].x + (blitbuff[zeroLIFO[i]].w >> 1);
            ys = blitbuff[zeroLIFO[i]].y + (blitbuff[zeroLIFO[i]].h >> 1);
            blithide(zeroLIFO[i], 0);
            if ((xs <= xmax) && (xs >= xmin) && (ys <= ymax) && (ys >= ymin)) {
                xs += (x2 - x1);
                if (xs >= xmax)xs -= (xmax - xmin + 1);
                if (xs < xmin)xs += (xmax - xmin + 1);
                blitbuff[zeroLIFO[i]].x = xs - (blitbuff[zeroLIFO[i]].w >> 1);
                ys += (y2 - y1);
                if (ys >= ymax)ys -= (ymax - ymin + 1);
                if (ys < ymin)ys += (ymax - ymin + 1);
                blitbuff[zeroLIFO[i]].y = ys - (blitbuff[zeroLIFO[i]].h >> 1);
            }
        }
        if (blank == -1) {
            if (x1 != x2) {
                if (sx > 0) {
                    ReadBuffer(xmax - sx + 1, ymin, xmax, ymax, (uint32_t *)current3);
                }
                else {
                    ReadBuffer(xmin, ymin, xmin - sx - 1, ymax, (uint32_t*)current3);
                }
            }
            if (y1 != y2) {
                if (sy > 0) {
                    ReadBuffer(xmin, ymin, xmax, ymin - 1 + sy, (uint32_t*)current4);
                }
                else {
                    ReadBuffer(xmin, ymax + sy + 1, xmax, ymax, (uint32_t*)current4);
                }
            }
        }
        MoveBufferNormal(x1, y1, x2, y2, w, h, flip);
        if (x1 != x2) {
            if (blank != -1) {
                if (sx > 0) {
                    DrawRectangle(xmin, ymin, xmin + sx - 1, ymax, blank);
                }
                else {
                    DrawRectangle(xmax + sx + 1, ymin, xmax, ymax, blank);
                }
            }
            else {
                if (sx > 0) {
                    DrawBuffer(xmin, ymin, xmin + sx - 1, ymax, (uint32_t*)current3);
                }
                else {
                    DrawBuffer(xmax + sx + 1, ymin, xmax, ymax, (uint32_t*)current3);
                }
            }
        }
        if (y1 != y2) {
            if (blank != -1) {
                if (sy > 0) {
                    DrawRectangle(xmin, ymax - sy + 1, xmax, ymax, blank);
                }
                else {
                    DrawRectangle(xmin, ymin, xmax, ymin - 1 - sy, blank);
                }
            }
            else {
                if (sy > 0) {
                    DrawBuffer(xmin, ymax - sy + 1, xmax, ymax, (uint32_t*)current4);
                }
                else {
                    DrawBuffer(xmin, ymin, xmax, ymin - 1 - sy, (uint32_t*)current4);
                }
            }
        }
        FreeMemorySafe((void**)&current3);
        FreeMemorySafe((void**)&current4);
        for (i = 0; i < zeroLIFOpointer; i++) {
            BlitShowBuffer(zeroLIFO[i], blitbuff[zeroLIFO[i]].x, blitbuff[zeroLIFO[i]].y, 0);
        }
        for (i = 0; i < LIFOpointer; i++) {
            if (blitbuff[LIFO[i]].next_x != 10000) {
                blitbuff[LIFO[i]].x = blitbuff[LIFO[i]].next_x;
                blitbuff[LIFO[i]].next_x = 10000;
            }
            if (blitbuff[LIFO[i]].next_y != 10000) {
                blitbuff[LIFO[i]].y = blitbuff[LIFO[i]].next_y;
                blitbuff[LIFO[i]].next_y = 10000;
            }
            BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
        }
        ProcessCollisions(0);
    }
    else if ((p = checkstring(cmdline, (unsigned char*)"SCROLL"))) {
        int i, n, m = 0, blank = -2, x, y;
        char* current = NULL;
        getargs(&p, 5, (unsigned char*)",");
        if (hideall)error((char *)"Sprites are hidden");
        x = (int)getint(argv[0], -maxW / 2 - 1, maxW);
        y = (int)getint(argv[2], -maxH / 2 - 1, maxH);
        if (argc == 5)blank = (int)getColour(argv[2], 1);
        if (!(x == 0 && y == 0)) {
           m = ((maxW * (y > 0 ? y : -y)) * sizeof(uint32_t));
            n = ((maxH * (x > 0 ? x : -x)) * sizeof(uint32_t));
            if (n > m)m = n;
            if (blank == -2)current = (char *)GetMemory(m);
            for (i = LIFOpointer - 1; i >= 0; i--) blithide(LIFO[i], 0);
            for (i = zeroLIFOpointer - 1; i >= 0; i--) {
                int xs = blitbuff[zeroLIFO[i]].x + (blitbuff[zeroLIFO[i]].w >> 1);
                int ys = blitbuff[zeroLIFO[i]].y + (blitbuff[zeroLIFO[i]].h >> 1);
                blithide(zeroLIFO[i], 0);
                xs += x;
                if (xs >= maxW)xs -= maxW;
                if (xs < 0)xs += maxW;
                blitbuff[zeroLIFO[i]].x = xs - (blitbuff[zeroLIFO[i]].w >> 1);
                ys -= y;
                if (ys >= maxH)ys -= maxH;
                if (ys < 0)ys += maxH;
                blitbuff[zeroLIFO[i]].y = ys - (blitbuff[zeroLIFO[i]].h >> 1);
            }
            if (x > 0) {
                if (blank == -2)ReadBuffer(maxW - x, 0, maxW - 1, maxH - 1, (uint32_t*)current);
                ScrollBufferH(x);
                if (blank == -2)DrawBuffer(0, 0, x - 1, maxH - 1, (uint32_t*)current);
                else if (blank != -1)DrawRectangle(0, 0, x - 1, maxH - 1, blank);
            }
            else if (x < 0) {
                x = -x;
                if (blank == -2)ReadBuffer(0, 0, x - 1, maxH - 1, (uint32_t*)current);
                ScrollBufferH(-x);
                if (blank == -2)DrawBuffer(maxW - x, 0, maxW - 1, maxH - 1, (uint32_t*)current);
                else if (blank != -1)DrawRectangle(maxW - x, 0, maxW - 1, maxH - 1, blank);
            }
            if (y > 0) {
                if (blank == -2)ReadBuffer(0, 0, maxW - 1, y - 1, (uint32_t*)current);
                ScrollBufferV(y, 0);
                if (blank == -2)DrawBuffer(0, maxH - y, maxW - 1, maxH - 1, (uint32_t*)current);
                else if (blank != -1)DrawRectangle(0, maxH - y, maxW - 1, maxH - 1, blank);
            }
            else if (y < 0) {
                y = -y;
                if (blank == -2)ReadBuffer(0, maxH - y, maxW - 1, maxH - 1, (uint32_t*)current);
                ScrollBufferV(-y, 0);
                if (blank == -2)DrawBuffer(0, 0, maxW - 1, y - 1, (uint32_t*)current);
                else if (blank != -1)DrawRectangle(0, 0, maxW - 1, y - 1, blank);
            }
            for (i = 0; i < zeroLIFOpointer; i++) {
                BlitShowBuffer(zeroLIFO[i], blitbuff[zeroLIFO[i]].x, blitbuff[zeroLIFO[i]].y, 0);
            }
            for (i = 0; i < LIFOpointer; i++) {
                if (blitbuff[LIFO[i]].next_x != 10000) {
                    blitbuff[LIFO[i]].x = blitbuff[LIFO[i]].next_x;
                    blitbuff[LIFO[i]].next_x = 10000;
                }
                if (blitbuff[LIFO[i]].next_y != 10000) {
                    blitbuff[LIFO[i]].y = blitbuff[LIFO[i]].next_y;
                    blitbuff[LIFO[i]].next_y = 10000;
                }

                BlitShowBuffer(LIFO[i], blitbuff[LIFO[i]].x, blitbuff[LIFO[i]].y, 0);
            }
            ProcessCollisions(0);
            if (current)FreeMemory((unsigned char *)current);
        }
    }
    else {
        int flip = 0;
        getargs(&cmdline, 15, (unsigned char*)",");
        if (argc < 11) error((char *)"Syntax");
        x1 = (int)getinteger(argv[0]);
        y1 = (int)getinteger(argv[2]);
        x2 = (int)getinteger(argv[4]);
        y2 = (int)getinteger(argv[6]);
        w = (int)getinteger(argv[8]);
        h = (int)getinteger(argv[10]);
        if (argc >= 13 && *argv[12]) {
            if (checkstring(argv[12], (unsigned char*)"FRAMEBUFFER")) {
                ReadPage = WPN;
            }
            else {
                ReadPage = (int)getint(argv[12], 0, MAXPAGES);
            }
        }
        int readlimx = PageTable[ReadPage].xmax;
        int readlimy = PageTable[ReadPage].ymax;
        if (argc == 15)flip = (int)getint(argv[14], 0, 7);
        if (w < 1 || h < 1) {
            ReadPage = WritePage;
            return;
        }
        if (x1 < 0) {
            x2 -= x1;
            w += x1;
            x1 = 0;
        }
        if (x2 < 0) {
            if (!(flip & 1))x1 -= x2;
            w += x2;
            x2 = 0;
        }
        if (y1 < 0) {
            y2 -= y1;
            h += y1;
            y1 = 0;
        }
        if (y2 < 0) {
            if (!(flip & 2))y1 -= y2;
            h += y2;
            y2 = 0;
        }
        if (x1 + w > readlimx) {
            w = readlimx - x1;
        }
        if (x2 + w > maxW) {
            if (flip & 1) x1 += x2 + w - maxW;
            w = maxW - x2;
        }
        if (y1 + h > readlimy) {
            h = readlimy - y1;
        }
        if (y2 + h > maxH) {
            if (flip & 2) y1 += y2 + h - maxH;
            h = maxH - y2;
        }
        if (w < 1 || h < 1 || x1 < 0 || x1 + w > readlimx || x2 < 0 || x2 + w > maxW || y1 < 0 || y1 + h > readlimy || y2 < 0 || y2 + h > maxH) {
            ReadPage = WritePage;
            return;
        }
        MoveBufferNormal(x1, y1, x2, y2, w, h, flip);
        ReadPage = WritePage;
    }
}

