/***********************************************************************************************************************
MMBasic for Windows

Draw.cpp

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
#include "font1.h"
#include "FileIO.h"
#include "Misc_12x20_LE.h"
#include "Hom_16x24_LE.h"
#include "Fnt_10x16.h"
#include "Inconsola.h"
#include "ArialNumFontPlus.h"
#include "Font_8x6.h"
int gui_font;
int64_t gui_fcolour = M_WHITE;
int64_t gui_bcolour = M_BLACK;
int low_y = VRes, high_y = 0, low_x = HRes, high_x = 0;
int lastx, lasty;
int PrintPixelMode = 0;
int gui_font_width, gui_font_height;
int64_t last_bcolour, last_fcolour;
int CurrentX = 0, CurrentY = 0;                                             // the current default position for the next char to be written
int DisplayHRes, DisplayVRes;                                       // the physical characteristics of the display
volatile int CursorTimer = 0;               // used to time the flashing cursor
unsigned char* CFunctionFlash = NULL;
unsigned char* CFunctionLibrary = NULL;
uint32_t * blitbuffptr[MAXBLITBUF];                                  //Buffer pointers for the BLIT command
struct s_pagetable PageTable[MAXTOTALPAGES];
uint32_t linebuff[3840 * 2 * sizeof(uint32_t)];
struct D3D* struct3d[MAX3D + 1] = { NULL };
s_camera camera[MAXCAM + 1];
int VideoMode;
int CMM1 = 0;
const int64_t colourmap[16] = { M_BLACK,M_BLUE,M_GREEN,M_CYAN,M_RED,M_MAGENTA,M_YELLOW,M_WHITE,M_MYRTLE,M_COBALT,M_MIDGREEN,M_CERULEAN,M_RUST,M_FUCHSIA,M_BROWN,M_LILAC };
const int xres[MAXMODES + 1] =         { 0,800,640,320,480,240,256,320,640,1024,848,1280,960,400,960,1280,1920,384,1024};
const int yres[MAXMODES + 1] =         { 0,600,400,200,432,216,240,240,480,768,480,720,540,300,540,1024,1080,240,600};
const int pixeldensity[MAXMODES + 1] = { 0,  1,  1,  3,  2,  4,  4,  3,  1,  1,  1,  1,  1,  2,  2,  1 ,   1,  3,  1};
const int defaultfont[MAXMODES + 1] = { 0,1,1,1,1,1,1,1,1,(2 << 4) | 1 ,1 ,(2 << 4) | 1 , (3 << 4) | 1 ,1,1,(2 << 4) | 1 ,(2 << 4) | 1 ,1,(3 << 4) | 1 };
typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char transparency;
} rgb_t;
typedef struct {
    TFLOAT  xpos;       // current position and heading
    TFLOAT  ypos;       // (uses floating-point numbers for
    TFLOAT  heading;    //  increased accuracy)

    rgb_t  pen_color;   // current pen color
    rgb_t  fill_color;  // current fill color
    bool   pendown;     // currently drawing?
    bool   filled;      // currently filling?
} fill_t;
fill_t main_fill;
fill_t backup_fill;
int    main_fill_poly_vertex_count = 0;       // polygon vertex count
TFLOAT* main_fill_polyX = NULL; // polygon vertex x-coords
TFLOAT* main_fill_polyY = NULL; // polygon vertex y-coords

    unsigned char* FontTable[FONT_TABLE_SIZE] = {   (unsigned char*)font1,
                                                    (unsigned char*)Misc_12x20_LE,
                                                    (unsigned char*)Hom_16x24_LE,
                                                    (unsigned char*)Fnt_10x16,
                                                    (unsigned char*)Inconsola,
                                                    (unsigned char*)ArialNumFontPlus,
                                                    (unsigned char*)F_6x8_LE,
                                                    (unsigned char*)NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,

    };
    extern "C" void SetFont(int fnt) {
        if(FontTable[fnt >> 4] == NULL) error((char *)"Invalid font number #%", (fnt >> 4) + 1);
        gui_font_width = FontTable[fnt >> 4][0] * (fnt & 0b1111);
        gui_font_height = FontTable[fnt >> 4][1] * (fnt & 0b1111);
        OptionHeight = VRes / gui_font_height;
        OptionWidth = HRes / gui_font_width;
        gui_font = fnt;
    }

    extern "C" void hline(int x0, int x1, int y, int f, int ints_per_line, uint32_t* br) { //draw a horizontal line
        uint32_t w1, xx1, w0, xx0, x, xn, i;
        const uint32_t a[] = { 0xFFFFFFFF,0x7FFFFFFF,0x3FFFFFFF,0x1FFFFFFF,0xFFFFFFF,0x7FFFFFF,0x3FFFFFF,0x1FFFFFF,
                            0xFFFFFF,0x7FFFFF,0x3FFFFF,0x1FFFFF,0xFFFFF,0x7FFFF,0x3FFFF,0x1FFFF,
                            0xFFFF,0x7FFF,0x3FFF,0x1FFF,0xFFF,0x7FF,0x3FF,0x1FF,
                            0xFF,0x7F,0x3F,0x1F,0x0F,0x07,0x03,0x01 };
        const uint32_t b[] = { 0x80000000,0xC0000000,0xe0000000,0xf0000000,0xf8000000,0xfc000000,0xfe000000,0xff000000,
                            0xff800000,0xffC00000,0xffe00000,0xfff00000,0xfff80000,0xfffc0000,0xfffe0000,0xffff0000,
                            0xffff8000,0xffffC000,0xffffe000,0xfffff000,0xfffff800,0xfffffc00,0xfffffe00,0xffffff00,
                            0xffffff80,0xffffffC0,0xffffffe0,0xfffffff0,0xfffffff8,0xfffffffc,0xfffffffe,0xffffffff };
        w0 = y * (ints_per_line);
        xx0 = 0;
        w1 = y * (ints_per_line)+x1 / 32;
        xx1 = (x1 & 0x1F);
        w0 = y * (ints_per_line)+x0 / 32;
        xx0 = (x0 & 0x1F);
        w1 = y * (ints_per_line)+x1 / 32;
        xx1 = (x1 & 0x1F);
        if(w1 == w0) { //special case both inside same word
            x = (a[xx0] & b[xx1]);
            xn = ~x;
            if(f)br[w0] |= x; else  br[w0] &= xn;                   // turn on the pixel
        }
        else {
            if(w1 - w0 > 1) { //first deal with full words
                for (i = w0 + 1; i < w1; i++) {
                    // draw the pixel
                    br[i] = 0;
                    if(f)br[i] = M_WHITE;          // turn on the pixels
                }
            }
            x = ~a[xx0];
            br[w0] &= x;
            x = ~x;
            if(f)br[w0] |= x;                         // turn on the pixel
            x = ~b[xx1];
            br[w1] &= x;
            x = ~x;
            if(f)br[w1] |= x;                         // turn on the pixel
        }
    }

    extern "C" void DrawFilledCircle(int x, int y, int radius, int r, int fill, int ints_per_line, uint32_t* br, MMFLOAT aspect, MMFLOAT aspect2) {
        int a, b, P;
        int A, B, asp;
        x = (int)((MMFLOAT)r * aspect) + radius;
        y = r + radius;
        a = 0;
        b = radius;
        P = 1 - radius;
        asp = (int)(aspect2 * (MMFLOAT)(1 << 10));
        do {
            A = (a * asp) >> 10;
            B = (b * asp) >> 10;
            hline(x - A - radius, x + A - radius, y + b - radius, fill, ints_per_line, br);
            hline(x - A - radius, x + A - radius, y - b - radius, fill, ints_per_line, br);
            hline(x - B - radius, x + B - radius, y + a - radius, fill, ints_per_line, br);
            hline(x - B - radius, x + B - radius, y - a - radius, fill, ints_per_line, br);
            if(P < 0)
                P += 3 + 2 * a++;
            else
                P += 5 + 2 * (a++ - b--);

        } while (a <= b);
    }
    /***********************************************************************************************
    Draw a circle on the video output
        x, y - the center of the circle
        radius - the radius of the circle
        w - width of the line drawing the circle
        c - the colour to use for the circle
        fill - the colour to use for the fill or -1 if no fill
        aspect - the ration of the x and y axis (a MMFLOAT).  1.0 gives a prefect circle
    ***********************************************************************************************/
    extern "C" void DrawCircle(int x, int y, int radius, int w, int64_t c, int64_t fill, MMFLOAT aspect) {
        int a, b, P;
        int A, B;
        int asp;
        MMFLOAT aspect2;
        if(w > 1) {
            if(fill >= 0) { // thick border with filled centre
                DrawCircle(x, y, radius, 0, c, c, aspect);
                aspect2 = ((aspect * (MMFLOAT)radius) - (MMFLOAT)w) / ((MMFLOAT)(radius - w));
                DrawCircle(x, y, radius - w, 0, fill, fill, aspect2);
            }
            else { //thick border with empty centre
                int r1 = radius - w, r2 = radius, xs = -1, xi = 0, i, j, k, m, ll = radius;
                if(aspect > 1.0)ll = (int)((MMFLOAT)radius * aspect);
                int ints_per_line = RoundUptoInt((ll * 2) + 1) / 32;
                uint32_t* br = (uint32_t*)GetTempMemory(((ints_per_line + 1) * ((r2 * 2) + 1)) * 4);
                DrawFilledCircle(x, y, r2, r2, 1, ints_per_line, br, aspect, aspect);
                aspect2 = ((aspect * (MMFLOAT)r2) - (MMFLOAT)w) / ((MMFLOAT)r1);
                DrawFilledCircle(x, y, r1, r2, 0, ints_per_line, br, aspect, aspect2);
                x = (int)((MMFLOAT)x + (MMFLOAT)r2 * (1.0 - aspect));
                for (j = 0; j < r2 * 2 + 1; j++) {
                    for (i = 0; i < ints_per_line; i++) {
                        k = br[i + j * ints_per_line];
                        for (m = 0; m < 32; m++) {
                            if(xs == -1 && (k & 0x80000000)) {
                                xs = m;
                                xi = i;
                            }
                            if(xs != -1 && !(k & 0x80000000)) {
                                DrawRectangle(x - r2 + xs + xi * 32, y - r2 + j, x - r2 + m + i * 32, y - r2 + j, c);
                                xs = -1;
                            }
                            k <<= 1;
                        }
                    }
                    if(xs != -1) {
                        DrawRectangle(x - r2 + xs + xi * 32, y - r2 + j, x - r2 + m + i * 32, y - r2 + j, c);
                        xs = -1;
                    }
                }
            }

        }
        else { //single thickness outline
            int w1 = w, r1 = radius;
            if(fill >= 0) {
                while (w >= 0 && radius > 0) {
                    a = 0;
                    b = radius;
                    P = 1 - radius;
                    asp = (int)(aspect * (MMFLOAT)(1 << 10));

                    do {
                        A = (a * asp) >> 10;
                        B = (b * asp) >> 10;
                        if(fill >= 0 && w >= 0) {
                            DrawRectangle(x - A, y + b, x + A, y + b, fill);
                            DrawRectangle(x - A, y - b, x + A, y - b, fill);
                            DrawRectangle(x - B, y + a, x + B, y + a, fill);
                            DrawRectangle(x - B, y - a, x + B, y - a, fill);
                        }
                        if(P < 0)
                            P += 3 + 2 * a++;
                        else
                            P += 5 + 2 * (a++ - b--);

                    } while (a <= b);
                    w--;
                    radius--;
                }
            }
            if(c != fill) {
                w = w1; radius = r1;
                while (w >= 0 && radius > 0) {
                    a = 0;
                    b = radius;
                    P = 1 - radius;
                    asp = (int)(aspect * (MMFLOAT)(1 << 10));
                    do {
                        A = (a * asp) >> 10;
                        B = (b * asp) >> 10;
                        if(w) {
                            DrawPixel(A + x, b + y, c);
                            DrawPixel(B + x, a + y, c);
                            DrawPixel(x - A, b + y, c);
                            DrawPixel(x - B, a + y, c);
                            DrawPixel(B + x, y - a, c);
                            DrawPixel(A + x, y - b, c);
                            DrawPixel(x - A, y - b, c);
                            DrawPixel(x - B, y - a, c);
                        }
                        if(P < 0)
                            P += 3 + 2 * a++;
                        else
                            P += 5 + 2 * (a++ - b--);

                    } while (a <= b);
                    w--;
                    radius--;
                }
            }
        }

    }
    extern "C" void DrawBitmap(int x1, int y1, int width, int height, int scale, int64_t fc, int64_t bc, unsigned char* bitmap) {
        int i, j, k, m, x, y;
        uint32_t *WritePageAddress = PageTable[WritePage].address;
        int HRes = PageTable[WritePage].xmax;
        int VRes = PageTable[WritePage].ymax;
        if (optiony)y1 = VRes - 1 - y1;
        if(x1 >= HRes || y1 >= VRes || x1 + width * scale < 0 || y1 + height * scale < 0)return;
        for (i = 0; i < height; i++) {                                   // step thru the font scan line by line
            for (j = 0; j < scale; j++) {                                // repeat lines to scale the font
                for (k = 0; k < width; k++) {                            // step through each bit in a scan line
                    for (m = 0; m < scale; m++) {                        // repeat pixels to scale in the x axis
                        x = x1 + k * scale + m;
                        y = y1 + i * scale + j;
                        if(x >= 0 && x < HRes && y >= 0 && y < VRes) {  // if the coordinates are valid
                            if((bitmap[((i * width) + k) / 8] >> (((height * width) - ((i * width) + k) - 1) % 8)) & 1) {
                                WritePageAddress[y * HRes + x] = (uint32_t)fc | (ARGBenabled ? 0 : 0xFF000000);
                            } else {
                                if(bc !=-1) {
                                    WritePageAddress[y * HRes + x] = (uint32_t)bc | (ARGBenabled ? 0 : 0xFF000000);
                                }
                            }
                        }
                    }
                }
            }
        }

    }
    extern "C" void DrawRectangle(int x1, int y1, int x2, int y2, int64_t c) {
        int x, y;
        uint32_t* WritePageAddress = PageTable[WritePage].address;
        int HRes = PageTable[WritePage].xmax;
        int VRes = PageTable[WritePage].ymax;
        if (optiony)y1 = VRes - 1 - y1;
        if (optiony)y2 = VRes - 1 - y2;
        if(x1 < 0) x1 = 0;
        if(x1 >= HRes) x1 = HRes - 1;
        if(x2 < 0) x2 = 0;
        if(x2 >= HRes) x2 = HRes - 1;
        if(y1 < 0) y1 = 0;
        if(y1 >= VRes) y1 = VRes - 1;
        if(y2 < 0) y2 = 0;
        if(y2 >= VRes) y2 = VRes - 1;
        if(x2 <= x1) std::swap(x1, x2);
        if(y2 <= y1) std::swap(y1, y2);
        for (y = y1; y <= y2; y++) {
            for (x = x1; x <= x2; x++) {
                WritePageAddress[y * HRes + x] = (uint32_t)c | (ARGBenabled ? 0 : 0xFF000000);
            }
        }
    }
    extern "C" void ScrollLCD(int lines) {
        if(lines == 0)return;
        uint32_t* WritePageAddress = PageTable[WritePage].address;
        int HRes = PageTable[WritePage].xmax;
        int VRes = PageTable[WritePage].ymax;
        if(lines >= 0) {
            for (int i = 0; i < VRes - lines; i++) {
                int d = i * HRes , s = (i + lines) * HRes ;
                for (int c = 0; c < HRes ; c++)WritePageAddress[d + c] = WritePageAddress[s + c];
            }
            DrawRectangle(0, VRes - lines, HRes - 1, VRes - 1, gui_bcolour); // erase the lines to be scrolled off
        }
        else {
            lines = -lines;
            for (int i = VRes - 1; i >= lines; i--) {
                int d = i * HRes, s = (i - lines) * HRes;
                for (int c = 0; c < HRes; c++)WritePageAddress[d + c] = WritePageAddress[s + c];
            }
            DrawRectangle(0, 0, HRes - 1, lines - 1, gui_bcolour); // erase the lines introduced at the top
        }
    }
    extern "C" int GetFontWidth(int fnt) {
        return FontTable[fnt >> 4][0] * (fnt & 0b1111);
    }


    extern "C" int GetFontHeight(int fnt) {
        return FontTable[fnt >> 4][1] * (fnt & 0b1111);
    }
    extern "C" void ReadBuffer(int x1, int y1, int x2, int y2, uint32_t* c) {
        int x, y, t;
        uint32_t* ReadPageAddress = PageTable[ReadPage].address;
        int HRes = PageTable[ReadPage].xmax;
        int VRes = PageTable[ReadPage].ymax;
        if (optiony)y1 = VRes - 1 - y1;
        if (optiony)y2 = VRes - 1 - y2;
        uint32_t *d=(uint32_t *)c;
        if (x2 <= x1) { t = x1; x1 = x2; x2 = t; }
        if (y2 <= y1) { t = y1; y1 = y2; y2 = t; }
        int xx1 = x1, yy1 = y1, xx2 = x2, yy2 = y2;
        if (x1 < 0) xx1 = 0;
        if (x1 >= HRes) xx1 = HRes - 1;
        if (x2 < 0) xx2 = 0;
        if (x2 >= HRes) xx2 = HRes - 1;
        if (y1 < 0) yy1 = 0;
        if (y1 >= VRes) yy1 = VRes - 1;
        if (y2 < 0) yy2 = 0;
        if (y2 >= VRes) yy2 = VRes - 1;
        for (y = y1; y <= y2; y++) {
            for (x = x1; x <= x2; x++) {
                *d++ = ReadPageAddress[y*HRes+x];
            }
        }
    }
    extern "C" void DrawBuffer(int x1, int y1, int x2, int y2, uint32_t *p) {
        int x, y, t;
        // make sure the coordinates are kept within the display area
        int maxW = PageTable[WritePage].xmax;
        int maxH = PageTable[WritePage].ymax;
        uint32_t wpa = (uint32_t)PageTable[WritePage].address;
        if (optiony)y1 = maxH - 1 - y1;
        if (optiony)y2 = maxH - 1 - y2;
        if (x2 <= x1) { t = x1; x1 = x2; x2 = t; }
        if (y2 <= y1) { t = y1; y1 = y2; y2 = t; }
        uint32_t* s, * pp = (uint32_t*)p;
        for (y = y1; y <= y2; y++) {
            s = (uint32_t*)((y * maxW + x1) * 4 + wpa);
            for (x = x1; x <= x2; x++) {
                if (x >= 0 && x < maxW && y >= 0 && y < maxH) {
                    *s++ = *pp++; //this order swaps the bytes to match the .BMP file
                }
                else {
                    s++;
                    pp++;
                }
            }
        }
        }
    extern "C" void DrawBuffer32(int x1, int y1, int x2, int y2, char* p, int skip) {
        int x, y, t;
        uint32_t* sc;
        union colourmap
        {
            char rgbbytes[4];
            uint32_t rgb;
        } c;
        int maxW = PageTable[WritePage].xmax;
        int maxH = PageTable[WritePage].ymax;
        uint32_t wpa = (uint32_t)PageTable[WritePage].address;
        if (optiony)y1 = maxH - 1 - y1;
        if (optiony)y2 = maxH - 1 - y2;
        // make sure the coordinates are kept within the display area
        if (x2 <= x1) { t = x1; x1 = x2; x2 = t; }
        if (y2 <= y1) { t = y1; y1 = y2; y2 = t; }
        t = 0;
        int cursorhidden = 0;
            for (y = y1; y <= y2; y++) {
                sc = (uint32_t*)((y * maxW + x1) * 4 + wpa);
                for (x = x1; x <= x2; x++) {
                    if (x >= 0 && x < maxW && y >= 0 && y < maxH) {
                        if (skip & 2) {
                            c.rgbbytes[3] = (char)0xFF; //assume solid colour
                            c.rgbbytes[0] = *p++; //this order swaps the bytes to match the .BMP file
                            c.rgbbytes[1] = *p++;
                            c.rgbbytes[2] = *p++;
                            if (skip & 1)c.rgbbytes[3] = *p++; //ARGB8888 so set transparency
                        }
                        else {
                            c.rgbbytes[3] = (char)0xFF;
                            c.rgbbytes[0] = *p++; //this order swaps the bytes to match the .BMP file
                            c.rgbbytes[1] = *p++;
                            c.rgbbytes[2] = *p++;
                            if (skip & 1)p++;
                        }
                        *sc = c.rgb;
                    }
                    else {
                        p += (skip & 1) ? 4 : 3;
                    }
                    sc++;
                }
            }
    }

    /******************************************************************************************
     Print a char on the LCD display
     Any characters not in the font will print as a space.
     The char is printed at the current location defined by CurrentX and CurrentY
    *****************************************************************************************/
    extern "C" void GUIPrintChar(int fnt, int64_t fc, int64_t bc, unsigned char c, int orientation) {
        unsigned char* p, * fp, * np = NULL, * AllocatedMemory = NULL;
        int BitNumber, BitPos, x, y, newx, newy, modx, mody, scale = fnt & 0b1111;
        int height, width;
        if(PrintPixelMode == 1)bc = -1;
        if(PrintPixelMode == 2) {
            std::swap(bc, fc);
        }
        if(PrintPixelMode == 5) {
            fc = bc;
            bc = -1;
        }

        // to get the +, - and = chars for font 6 we fudge them by scaling up font 1
        if((fnt & 0xf0) == 0x50 && (c == '-' || c == '+' || c == '=')) {
            fp = (unsigned char*)FontTable[0];
            scale = scale * 4;
        }
        else
            fp = (unsigned char*)FontTable[fnt >> 4];

        height = fp[1];
        width = fp[0];
        modx = mody = 0;
        if(orientation > ORIENT_VERT) {
            AllocatedMemory = np = (unsigned char *)GetTempMemory(width * height);
            if(orientation == ORIENT_INVERTED) {
                modx -= width * scale - 1;
                mody -= height * scale - 1;
            }
            else if(orientation == ORIENT_CCW90DEG) {
                mody -= width * scale;
            }
            else if(orientation == ORIENT_CW90DEG) {
                modx -= height * scale - 1;
            }
        }

        if(c >= fp[2] && c < fp[2] + fp[3]) {
            p = fp + 4 + (int)(((c - fp[2]) * height * width) / 8);

            if(orientation > ORIENT_VERT) {                             // non-standard orientation
                if(orientation == ORIENT_INVERTED) {
                    for (y = 0; y < height; y++) {
                        newy = height - y - 1;
                        for (x = 0; x < width; x++) {
                            newx = width - x - 1;
                            if((p[((y * width) + x) / 8] >> (((height * width) - ((y * width) + x) - 1) % 8)) & 1) {
                                BitNumber = ((newy * width) + newx);
                                BitPos = 128 >> (BitNumber % 8);
                                np[BitNumber / 8] |= BitPos;
                            }
                        }
                    }
                }
                else if(orientation == ORIENT_CCW90DEG) {
                    for (y = 0; y < height; y++) {
                        newx = y;
                        for (x = 0; x < width; x++) {
                            newy = width - x - 1;
                            if((p[((y * width) + x) / 8] >> (((height * width) - ((y * width) + x) - 1) % 8)) & 1) {
                                BitNumber = ((newy * height) + newx);
                                BitPos = 128 >> (BitNumber % 8);
                                np[BitNumber / 8] |= BitPos;
                            }
                        }
                    }
                }
                else if(orientation == ORIENT_CW90DEG) {
                    for (y = 0; y < height; y++) {
                        newx = height - y - 1;
                        for (x = 0; x < width; x++) {
                            newy = x;
                            if((p[((y * width) + x) / 8] >> (((height * width) - ((y * width) + x) - 1) % 8)) & 1) {
                                BitNumber = ((newy * height) + newx);
                                BitPos = 128 >> (BitNumber % 8);
                                np[BitNumber / 8] |= BitPos;
                            }
                        }
                    }
                }
            }
            else np = p;

            if(orientation < ORIENT_CCW90DEG) DrawBitmap(CurrentX + modx, CurrentY + mody, width, height, scale, fc, bc, np);
            else DrawBitmap(CurrentX + modx, CurrentY + mody, height, width, scale, fc, bc, np);
        }
        else {
            if(orientation < ORIENT_CCW90DEG) DrawRectangle(CurrentX + modx, CurrentY + mody, CurrentX + modx + (width * scale), CurrentY + mody + (height * scale), bc);
            else DrawRectangle(CurrentX + modx, CurrentY + mody, CurrentX + modx + (height * scale), CurrentY + mody + (width * scale), bc);
        }

        // to get the . and degree symbols for font 6 we draw a small circle
        if((fnt & 0xf0) == 0x50) {
            if(orientation > ORIENT_VERT) {
                if(orientation == ORIENT_INVERTED) {
                    if(c == '.') DrawCircle(CurrentX + modx + (width * scale) / 2, CurrentY + mody + 7 * scale, 4 * scale, 0, fc, fc, 1.0);
                    if(c == 0x60) DrawCircle(CurrentX + modx + (width * scale) / 2, CurrentY + mody + (height * scale) - 9 * scale, 6 * scale, 2 * scale, fc, -1, 1.0);
                }
                else if(orientation == ORIENT_CCW90DEG) {
                    if(c == '.') DrawCircle(CurrentX + modx + (height * scale) - 7 * scale, CurrentY + mody + (width * scale) / 2, 4 * scale, 0, fc, fc, 1.0);
                    if(c == 0x60) DrawCircle(CurrentX + modx + 9 * scale, CurrentY + mody + (width * scale) / 2, 6 * scale, 2 * scale, fc, -1, 1.0);
                }
                else if(orientation == ORIENT_CW90DEG) {
                    if(c == '.') DrawCircle(CurrentX + modx + 7 * scale, CurrentY + mody + (width * scale) / 2, 4 * scale, 0, fc, fc, 1.0);
                    if(c == 0x60) DrawCircle(CurrentX + modx + (height * scale) - 9 * scale, CurrentY + mody + (width * scale) / 2, 6 * scale, 2 * scale, fc, -1, 1.0);
                }

            }
            else {
                if(c == '.') DrawCircle(CurrentX + modx + (width * scale) / 2, CurrentY + mody + (height * scale) - 7 * scale, 4 * scale, 0, fc, fc, 1.0);
                if(c == 0x60) DrawCircle(CurrentX + modx + (width * scale) / 2, CurrentY + mody + 9 * scale, 6 * scale, 2 * scale, fc, -1, 1.0);
            }
        }

        if(orientation == ORIENT_NORMAL) CurrentX += width * scale;
        else if(orientation == ORIENT_VERT) CurrentY += height * scale;
        else if(orientation == ORIENT_INVERTED) CurrentX -= width * scale;
        else if(orientation == ORIENT_CCW90DEG) CurrentY -= width * scale;
        else if(orientation == ORIENT_CW90DEG) CurrentY += width * scale;
    }


    /******************************************************************************************
     Print a string on the LCD display
     The string must be a C string (not an MMBasic string)
     Any characters not in the font will print as a space.
    *****************************************************************************************/
    extern "C" void GUIPrintString(int x, int y, int fnt, int jh, int jv, int jo, int64_t fc, int64_t bc, unsigned char* str) {

        CurrentX = x;  CurrentY = y;
        char* newstr = NULL;
        if (optiony) {
            newstr = (char *)GetTempMemory(STRINGSIZE);
            if (jo == ORIENT_VERT || jo == ORIENT_CCW90DEG || jo == ORIENT_CW90DEG) {
                int i = strlen((const char *)str) - 1;
                int j = 0;
                while (i >= 0) {
                    newstr[i] = str[j];
                    i--;
                    j++;
                }
            }
            else strcpy(newstr, (const char*)str);
        }
        if (jo == ORIENT_NORMAL && optiony == 0) {
            if (jh == JUSTIFY_CENTER) CurrentX -= (strlen((const char *)str) * GetFontWidth(fnt)) / 2;
            if (jh == JUSTIFY_RIGHT)  CurrentX -= (strlen((const char *)str) * GetFontWidth(fnt));
            if (jv == JUSTIFY_MIDDLE) CurrentY -= GetFontHeight(fnt) / 2;
            if (jv == JUSTIFY_BOTTOM) CurrentY -= GetFontHeight(fnt);
        }
        else if (jo == ORIENT_NORMAL && optiony) {
            if (jh == JUSTIFY_CENTER) CurrentX -= (strlen((const char *)str) * GetFontWidth(fnt)) / 2;
            if (jh == JUSTIFY_RIGHT)  CurrentX -= (strlen((const char *)str) * GetFontWidth(fnt));
            if (jv == JUSTIFY_MIDDLE) CurrentY += GetFontHeight(fnt) / 2;
            if (jv == JUSTIFY_BOTTOM) CurrentY += GetFontHeight(fnt);
        }
        else if (jo == ORIENT_VERT && optiony == 0) {
            if (jh == JUSTIFY_CENTER) CurrentX -= GetFontWidth(fnt) / 2;
            if (jh == JUSTIFY_RIGHT)  CurrentX -= GetFontWidth(fnt);
            if (jv == JUSTIFY_MIDDLE) CurrentY -= (strlen((const char *)str) * GetFontHeight(fnt)) / 2;
            if (jv == JUSTIFY_BOTTOM) CurrentY -= (strlen((const char *)str) * GetFontHeight(fnt));
        }
        else if (jo == ORIENT_VERT && optiony) {
            CurrentY -= GetFontHeight(fnt) * (strlen((const char *)str) - 1);
            if (jh == JUSTIFY_CENTER) CurrentX -= GetFontWidth(fnt) / 2;
            if (jh == JUSTIFY_RIGHT)  CurrentX -= GetFontWidth(fnt);
            if (jv == JUSTIFY_MIDDLE) CurrentY += (strlen((const char *)str) * GetFontHeight(fnt)) / 2;
            if (jv == JUSTIFY_BOTTOM) CurrentY += (strlen((const char *)str) * GetFontHeight(fnt));
        }
        else if (jo == ORIENT_INVERTED && optiony == 0) {
            if (jh == JUSTIFY_CENTER) CurrentX += (strlen((const char *)str) * GetFontWidth(fnt)) / 2;
            if (jh == JUSTIFY_RIGHT)  CurrentX += (strlen((const char *)str) * GetFontWidth(fnt));
            if (jv == JUSTIFY_MIDDLE) CurrentY += GetFontHeight(fnt) / 2;
            if (jv == JUSTIFY_BOTTOM) CurrentY += GetFontHeight(fnt);
        }
        else if (jo == ORIENT_INVERTED && optiony) {
            if (jh == JUSTIFY_CENTER) CurrentX += (strlen((const char *)str) * GetFontWidth(fnt)) / 2;
            if (jh == JUSTIFY_RIGHT)  CurrentX += (strlen((const char *)str) * GetFontWidth(fnt));
            if (jv == JUSTIFY_MIDDLE) CurrentY -= GetFontHeight(fnt) / 2;
            if (jv == JUSTIFY_BOTTOM) CurrentY -= GetFontHeight(fnt);
        }
        else if (jo == ORIENT_CCW90DEG && optiony == 0) {
            if (jh == JUSTIFY_CENTER) CurrentX -= GetFontHeight(fnt) / 2;
            if (jh == JUSTIFY_RIGHT)  CurrentX -= GetFontHeight(fnt);
            if (jv == JUSTIFY_MIDDLE) CurrentY += (strlen((const char *)str) * GetFontWidth(fnt)) / 2;
            if (jv == JUSTIFY_BOTTOM) CurrentY += (strlen((const char *)str) * GetFontWidth(fnt));
        }
        else if (jo == ORIENT_CCW90DEG && optiony) {
            CurrentY += GetFontWidth(fnt) * (strlen((const char *)str) + 1);
            if (jh == JUSTIFY_CENTER) CurrentX -= GetFontHeight(fnt) / 2;
            if (jh == JUSTIFY_RIGHT)  CurrentX -= GetFontHeight(fnt);
            if (jv == JUSTIFY_MIDDLE) CurrentY -= (strlen((const char *)str) * GetFontWidth(fnt)) / 2;
            if (jv == JUSTIFY_BOTTOM) CurrentY -= (strlen((const char *)str) * GetFontWidth(fnt));
        }
        else if (jo == ORIENT_CW90DEG && optiony == 0) {
            if (jh == JUSTIFY_CENTER) CurrentX += GetFontHeight(fnt) / 2;
            if (jh == JUSTIFY_RIGHT)  CurrentX += GetFontHeight(fnt);
            if (jv == JUSTIFY_MIDDLE) CurrentY -= (strlen((const char *)str) * GetFontWidth(fnt)) / 2;
            if (jv == JUSTIFY_BOTTOM) CurrentY -= (strlen((const char *)str) * GetFontWidth(fnt));
        }
        else if (jo == ORIENT_CW90DEG && optiony) {
            CurrentY -= GetFontWidth(fnt) * (strlen((const char *)str) - 1);
            if (jh == JUSTIFY_CENTER) CurrentX += GetFontHeight(fnt) / 2;
            if (jh == JUSTIFY_RIGHT)  CurrentX += GetFontHeight(fnt);
            if (jv == JUSTIFY_MIDDLE) CurrentY += (strlen((const char *)str) * GetFontWidth(fnt)) / 2;
            if (jv == JUSTIFY_BOTTOM) CurrentY += (strlen((const char *)str) * GetFontWidth(fnt));
        }
        if (optiony) while (*newstr) GUIPrintChar(fnt, fc, bc, *newstr++, jo);
        else while (*str) GUIPrintChar(fnt, fc, bc, *str++, jo);
    }

    extern "C" void DisplayPutC(unsigned char c) {
        int maxH = PageTable[WritePage].ymax;
        int maxW = PageTable[WritePage].xmax;

        // if it is printable and it is going to take us off the right hand end of the screen do a CRLF
        if(c >= FontTable[gui_font >> 4][2] && c < FontTable[gui_font >> 4][2] + FontTable[gui_font >> 4][3]) {
            if(CurrentX + gui_font_width > HRes) {
                DisplayPutC('\r');
                DisplayPutC('\n');
            }
        }
        // handle the standard control chars
        switch (c) {
        case '\b':  CurrentX -= gui_font_width;
            if(CurrentX < 0) CurrentX = 0;
            return;
        case '\r':  CurrentX = 0;
            return;
        case '\n':  CurrentY += (optiony ? -gui_font_height : gui_font_height);
            if (optiony == 0 && (CurrentY + gui_font_height >= maxH )) {
                    ScrollLCD(CurrentY + gui_font_height - maxH);
                    CurrentY -= (CurrentY + gui_font_height - maxH);
            }
            else if (optiony == 1 && CurrentY < 0) {
                ScrollLCD(gui_font_height);
                CurrentY += gui_font_height;
                DrawRectangle(0, 0, maxW - 1, gui_font_height, gui_bcolour); // erase the line to be scrolled off
            }
            return;
        case '\t':  do {
            DisplayPutC(' ');
        } while ((CurrentX / gui_font_width) % Option.Tab);
        return;
        }
        GUIPrintChar(gui_font, gui_fcolour, gui_bcolour, c, ORIENT_NORMAL);            // print it
    }
    extern "C" void DisplayPutS(unsigned char* s) {
        while (*s) DisplayPutC(*s++);
    }
   
    void DrawBuffered(int xti, int yti, int64_t c, int complete) {
        static unsigned char pos = 0;
        static unsigned char movex, movey, movec;
        static short xtilast[8];
        static short ytilast[8];
        static int64_t clast[8];
        xtilast[pos] = xti;
        ytilast[pos] = yti;
        clast[pos] = c;
        if(complete == 1) {
            if(pos == 1) {
                DrawPixel(xtilast[0], ytilast[0], clast[0]);
            }
            else {
                DrawLine(xtilast[0], ytilast[0], xtilast[pos - 1], ytilast[pos - 1], 1, clast[0]);
            }
            pos = 0;
        }
        else {
            if(pos == 0) {
                movex = movey = movec = 1;
                pos += 1;
            }
            else {
                if(xti == xtilast[0] && abs(yti - ytilast[pos - 1]) == 1)movex = 0; else movex = 1;
                if(yti == ytilast[0] && abs(xti - xtilast[pos - 1]) == 1)movey = 0; else movey = 1;
                if(c == clast[0])movec = 0; else movec = 1;
                if(movec == 0 && (movex == 0 || movey == 0) && pos < 6) pos += 1;
                else {
                    if(pos == 1) {
                        DrawPixel(xtilast[0], ytilast[0], clast[0]);
                    }
                    else {
                        DrawLine(xtilast[0], ytilast[0], xtilast[pos - 1], ytilast[pos - 1], 1, clast[0]);
                    }
                    movex = movey = movec = 1;
                    xtilast[0] = xti;
                    ytilast[0] = yti;
                    clast[0] = c;
                    pos = 1;
                }
            }
        }
    }
    /**************************************************************************************************
    Draw a line on a the video output
      x1, y1 - the start coordinate
      x2, y2 - the end coordinate
        w - the width of the line (ignored for diagional lines)
      c - the colour to use
    ***************************************************************************************************/
#define abs( a)     (((a)> 0) ? (a) : -(a))

    extern "C" void DrawLine(int x1, int y1, int x2, int y2, int w, int64_t c) {


        if(y1 == y2) {
            DrawRectangle(x1, y1, x2, y2 + w - 1, c);                   // horiz line
            return;
        }
        if(x1 == x2) {
            DrawRectangle(x1, y1, x2 + w - 1, y2, c);                   // vert line
            return;
        }
        int  dx, dy, sx, sy, err, e2;
        dx = abs(x2 - x1); sx = x1 < x2 ? 1 : -1;
        dy = -abs(y2 - y1); sy = y1 < y2 ? 1 : -1;
        err = dx + dy;
        while (1) {
            DrawBuffered(x1, y1, c, 0);
            e2 = 2 * err;
            if(e2 >= dy) {
                if(x1 == x2) break;
                err += dy; x1 += sx;
            }
            if(e2 <= dx) {
                if(y1 == y2) break;
                err += dx; y1 += sy;
            }
        }
        DrawBuffered(0, 0, 0, 1);
    }

    extern "C" void ShowMMBasicCursor(int show) {
        static int visible = false;
        int newstate;
        newstate = ((CursorTimer <= CURSOR_ON) & show);                  // what should be the state of the cursor?
        if(visible == newstate) return;                                   // we can skip the rest if the cursor is already in the correct state
        visible = newstate;                                               // draw the cursor BELOW the font
        DrawLine(CurrentX, CurrentY + gui_font_height - 1, CurrentX + gui_font_width, CurrentY + gui_font_height - 1, (gui_font_height <= 8 ? 1 : 2), visible ? gui_fcolour : gui_bcolour);
    }
    /**********************************************************************************************
    Draw a box
         x1, y1 - the start coordinate
         x2, y2 - the end coordinate
         w      - the width of the sides of the box (can be zero)
         c      - the colour to use for sides of the box
         fill   - the colour to fill the box (-1 for no fill)
    ***********************************************************************************************/
    extern "C" void DrawBox(int x1, int y1, int x2, int y2, int w, int64_t c, int64_t fill) {
        // make sure the coordinates are in the right sequence
        if (x2 <= x1) std::swap(x1, x2);
        if (y2 <= y1) std::swap(y1, y2);
        if (w > x2 - x1) w = x2 - x1;
        if (w > y2 - y1) w = y2 - y1;

        if (w > 0) {
            w--;
            DrawRectangle(x1, y1, x2, y1 + w, c);                       // Draw the top horiz line
            DrawRectangle(x1, y2 - w, x2, y2, c);                       // Draw the bottom horiz line
            DrawRectangle(x1, y1, x1 + w, y2, c);                       // Draw the left vert line
            DrawRectangle(x2 - w, y1, x2, y2, c);                       // Draw the right vert line
            w++;
        }

        if (fill >= 0)
            DrawRectangle(x1 + w, y1 + w, x2 - w, y2 - w, fill);
    }

    extern "C" void ClearScreen(int64_t c) {
        DrawRectangle(0, 0, HRes - 1, VRes - 1, c);
    }
    /**********************************************************************************************
    Draw a box with rounded corners
         x1, y1 - the start coordinate
         x2, y2 - the end coordinate
         radius - the radius (in pixels) of the arc forming the corners
         c      - the colour to use for sides
         fill   - the colour to fill the box (-1 for no fill)
    ***********************************************************************************************/
    extern "C" void DrawRBox(int x1, int y1, int x2, int y2, int radius, int64_t c, int64_t fill) {
        int f, ddF_x, ddF_y, xx, yy;

        f = 1 - radius;
        ddF_x = 1;
        ddF_y = -2 * radius;
        xx = 0;
        yy = radius;

        while (xx < yy) {
            if (f >= 0) {
                yy -= 1;
                ddF_y += 2;
                f += ddF_y;
            }
            xx += 1;
            ddF_x += 2;
            f += ddF_x;
            DrawPixel(x2 + xx - radius, y2 + yy - radius, c);           // Bottom Right Corner
            DrawPixel(x2 + yy - radius, y2 + xx - radius, c);           // ^^^
            DrawPixel(x1 - xx + radius, y2 + yy - radius, c);           // Bottom Left Corner
            DrawPixel(x1 - yy + radius, y2 + xx - radius, c);           // ^^^

            DrawPixel(x2 + xx - radius, y1 - yy + radius, c);           // Top Right Corner
            DrawPixel(x2 + yy - radius, y1 - xx + radius, c);           // ^^^
            DrawPixel(x1 - xx + radius, y1 - yy + radius, c);           // Top Left Corner
            DrawPixel(x1 - yy + radius, y1 - xx + radius, c);           // ^^^
            if (fill >= 0) {
                DrawLine(x2 + xx - radius - 1, y2 + yy - radius, x1 - xx + radius + 1, y2 + yy - radius, 1, fill);
                DrawLine(x2 + yy - radius - 1, y2 + xx - radius, x1 - yy + radius + 1, y2 + xx - radius, 1, fill);
                DrawLine(x2 + xx - radius - 1, y1 - yy + radius, x1 - xx + radius + 1, y1 - yy + radius, 1, fill);
                DrawLine(x2 + yy - radius - 1, y1 - xx + radius, x1 - yy + radius + 1, y1 - xx + radius, 1, fill);
            }
        }
        if (fill >= 0) DrawRectangle(x1 + 1, y1 + radius, x2 - 1, y2 - radius, fill);
        DrawRectangle(x1 + radius - 1, y1, x2 - radius + 1, y1, c);     // top side
        DrawRectangle(x1 + radius - 1, y2, x2 - radius + 1, y2, c);    // botom side
        DrawRectangle(x1, y1 + radius, x1, y2 - radius, c);             // left side
        DrawRectangle(x2, y1 + radius, x2, y2 - radius, c);             // right side

    }
    /**********************************************************************************************
    Draw a triangle
        Thanks to Peter Mather (matherp on the Back Shed forum)
         x0, y0 - the first corner
         x1, y1 - the second corner
         x2, y2 - the third corner
         c      - the colour to use for sides of the triangle
         fill   - the colour to fill the triangle (-1 for no fill)
    ***********************************************************************************************/
    extern "C" void DrawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int64_t c, int64_t fill) {
        if (x0 * (y1 - y2) + x1 * (y2 - y0) + x2 * (y0 - y1) == 0) { // points are co-linear i.e zero area
            if (y0 > y1) {
                std::swap(y0, y1);
                std::swap(x0, x1);
            }
            if (y1 > y2) {
                std::swap(y2, y1);
                std::swap(x2, x1);
            }
            if (y0 > y1) {
                std::swap(y0, y1);
                std::swap(x0, x1);
            }
            DrawLine(x0, y0, x2, y2, 1, c);
        }
        else {
            if (fill == -1) {
                // draw only the outline
                DrawLine(x0, y0, x1, y1, 1, c);
                DrawLine(x1, y1, x2, y2, 1, c);
                DrawLine(x2, y2, x0, y0, 1, c);
            }
            else {
                //we are drawing a filled triangle which may also have an outline
                int a, b, y, last;

                if (y0 > y1) {
                    std::swap(y0, y1);
                    std::swap(x0, x1);
                }
                if (y1 > y2) {
                    std::swap(y2, y1);
                    std::swap(x2, x1);
                }
                if (y0 > y1) {
                    std::swap(y0, y1);
                    std::swap(x0, x1);
                }

                if (y1 == y2) {
                    last = y1;                                          //Include y1 scanline
                }
                else {
                    last = y1 - 1;                                      // Skip it
                }
                for (y = y0; y <= last; y++) {
                    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
                    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
                    if (a > b)std::swap(a, b);
                    DrawRectangle(a, y, b, y, fill);
                }
                while (y <= y2) {
                    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
                    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
                    if (a > b) std::swap(a, b);
                    DrawRectangle(a, y, b, y, fill);
                    y = y + 1;
                }
                // we also need an outline but we do this last to overwrite the edge of the fill area
                if (c != fill) {
                    DrawLine(x0, y0, x1, y1, 1, c);
                    DrawLine(x1, y1, x2, y2, 1, c);
                    DrawLine(x2, y2, x0, y0, 1, c);
                }
            }
        }
    }
    void  getargaddress(unsigned char* p, long long int** ip, MMFLOAT** fp, int* n) {
        unsigned char* ptr = NULL;
        *fp = NULL;
        *ip = NULL;
        unsigned char pp[STRINGSIZE] = { 0 };
        strcpy((char *)pp, (const char*)p);
        if (!isnamestart(pp[0])) { //found a literal
            *n = 1;
            return;
        }
        ptr = (unsigned char *)findvar(pp, V_FIND | V_EMPTY_OK | V_NOFIND_NULL);
        if (ptr && vartbl[VarIndex].type & T_NBR) {
            if (vartbl[VarIndex].dims[0] <= 0) { //simple variable
                *n = 1;
                return;
            }
            else { // array or array element
                if (*n == 0)*n = vartbl[VarIndex].dims[0] + 1 - OptionBase;
                else *n = (vartbl[VarIndex].dims[0] + 1 - OptionBase) < *n ? (vartbl[VarIndex].dims[0] + 1 - OptionBase) : *n;
                skipspace(p);
                do {
                    p++;
                } while (isnamechar(*p));
                if (*p == '!') p++;
                if (*p == '(') {
                    p++;
                    skipspace(p);
                    if (*p != ')') { //array element
                        *n = 1;
                        return;
                    }
                }
            }
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            *fp = (MMFLOAT*)ptr;
        }
        else if (ptr && vartbl[VarIndex].type & T_INT) {
            if (vartbl[VarIndex].dims[0] <= 0) {
                *n = 1;
                return;
            }
            else {
                if (*n == 0)*n = vartbl[VarIndex].dims[0] + 1 - OptionBase;
                else *n = (vartbl[VarIndex].dims[0] + 1 - OptionBase) < *n ? (vartbl[VarIndex].dims[0] + 1 - OptionBase) : *n;
                skipspace(p);
                do {
                    p++;
                } while (isnamechar(*p));
                if (*p == '%') p++;
                if (*p == '(') {
                    p++;
                    skipspace(p);
                    if (*p != ')') { //array element
                        *n = 1;
                        return;
                    }
                }
            }
            if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
            *ip = (long long int*)ptr;
        }
        else {
            *n = 1; //may be a function call
        }
    }
    void cmd_cls(void) {
        HideAllControls();
        skipspace(cmdline);
        if (!(*cmdline == 0 || *cmdline == '\''))
            ClearScreen(getint(cmdline, 0, M_WHITE));
        else
            ClearScreen(gui_bcolour);
        CurrentX = CurrentY = 0;
    }
    void fun_rgb(void) {
        getargs(&ep, 7, (unsigned char *)",");
        if (argc == 5)
            iret = RGB(getint(argv[0], 0, 255), getint(argv[2], 0, 255), getint(argv[4], 0, 255)) | 0xFF000000;
        else if (argc == 7) 
            iret = RGB(getint(argv[0], 0, 255), getint(argv[2], 0, 255), getint(argv[4], 0, 255)) | (getint(argv[6], 0, 255)<<24);
        else if (argc == 1) {
            if (checkstring(argv[0], (unsigned char*)"WHITE"))        iret = M_WHITE;
            else if (checkstring(argv[0], (unsigned char*)"YELLOW"))  iret = M_YELLOW;
            else if (checkstring(argv[0], (unsigned char*)"LILAC"))   iret = M_LILAC;
            else if (checkstring(argv[0], (unsigned char*)"BROWN"))   iret = M_BROWN;
            else if (checkstring(argv[0], (unsigned char*)"FUCHSIA")) iret = M_FUCHSIA;
            else if (checkstring(argv[0], (unsigned char*)"RUST"))    iret = M_RUST;
            else if (checkstring(argv[0], (unsigned char*)"MAGENTA")) iret = M_MAGENTA;
            else if (checkstring(argv[0], (unsigned char*)"RED"))     iret = M_RED;
            else if (checkstring(argv[0], (unsigned char*)"CYAN"))    iret = M_CYAN;
            else if (checkstring(argv[0], (unsigned char*)"GREEN"))   iret = M_GREEN;
            else if (checkstring(argv[0], (unsigned char*)"CERULEAN"))iret = M_CERULEAN;
            else if (checkstring(argv[0], (unsigned char*)"MIDGREEN"))iret = M_MIDGREEN;
            else if (checkstring(argv[0], (unsigned char*)"COBALT"))  iret = M_COBALT;
            else if (checkstring(argv[0], (unsigned char*)"MYRTLE"))  iret = M_MYRTLE;
            else if (checkstring(argv[0], (unsigned char*)"BLUE"))    iret = M_BLUE;
            else if (checkstring(argv[0], (unsigned char*)"BLACK"))   iret = M_BLACK;
            else if (checkstring(argv[0], (unsigned char*)"GRAY"))    iret = M_GRAY;
            else if (checkstring(argv[0], (unsigned char*)"GREY"))    iret = M_GRAY;
            else if (checkstring(argv[0], (unsigned char*)"LIGHTGRAY"))    iret = M_LITEGRAY;
            else if (checkstring(argv[0], (unsigned char*)"LIGHTGREY"))    iret = M_LITEGRAY;
            else if (checkstring(argv[0], (unsigned char*)"ORANGE"))    iret = M_ORANGE;
            else if (checkstring(argv[0], (unsigned char*)"PINK"))    iret = M_PINK;
            else if (checkstring(argv[0], (unsigned char*)"GOLD"))    iret = M_GOLD;
            else if (checkstring(argv[0], (unsigned char*)"SALMON"))    iret = M_SALMON;
            else if (checkstring(argv[0], (unsigned char*)"BEIGE"))    iret = M_BEIGE;
            else if (checkstring(argv[0], (unsigned char*)"BLANK"))    iret = M_BLANK;
            else error((char *)"Invalid colour: $", argv[0]);
        }
        else
            error((char *)"Syntax");
        targ = T_INT;
    }
    // these three functions were written by Peter Mather (matherp on the Back Shed forum)
// read the contents of a PIXEL out of screen memory
    void fun_pixel(void) {
        uint32_t p;
        int x, y;
        getargs(&ep, 3, (unsigned char *)",");
        if (argc != 3) error((char *)"Argument count");
        x = (int)getinteger(argv[0]);
        y = (int)getinteger(argv[2]);
        ReadBuffer(x, y, x, y, &p);
        iret = p & M_WHITE;
        targ = T_INT;
    }
    void getcoord(char* p, int* x, int* y) {
        unsigned char* tp, * ttp;
        unsigned char b[STRINGSIZE];
        char savechar;
        tp = getclosebracket((unsigned char*)p);
        savechar = *tp;
        *tp = 0;														// remove the closing brackets
        strcpy((char *)b, p);													// copy the coordinates to the temp buffer
        *tp = savechar;														// put back the closing bracket
        ttp = b + 1;
        // kludge (todo: fix this)
        {
            getargs(&ttp, 3, (unsigned char*)",");										// this is a macro and must be the first executable stmt in a block
            if (argc != 3) error((char*)"Invalid Syntax");
            *x = (int)getinteger(argv[0]);
            *y = (int)getinteger(argv[2]);
        }
    }

    extern "C" int64_t getColour(unsigned char* c, int minus) {
        int64_t colour;
        if (CMM1) {
            colour = getint(c, (minus ? -1 : 0), 15);
            if (colour >= 0)colour = colourmap[colour];
        }
        else colour = getint((unsigned char*)c, (minus ? -1 : 0), M_WHITE);
        return colour;

    }
    int64_t filloldcolour, ConvertedColour;
    int fillmaxH, fillmaxW;
#define DrawHLineFast(x1,y,x2,c)  DrawRectangle(x1,y,x2,y,c);
    uint32_t ReadPixelFast(int x, int y) {
        uint32_t p;
        ReadBuffer(x, y, x, y, &p);
        return p;
    }
    void floodFillScanline(int x, int y)
    {
        if (filloldcolour == ConvertedColour) return;
        if (ReadPixelFast(x, y) != filloldcolour) return;

        int x1, xe, xs;

        //draw current scanline from start position to the right
        x1 = x;
        xe = -1;
        while (x1 < fillmaxW && ReadPixelFast(x1, y) == filloldcolour)
        {
            xe = x1;
            x1++;
        }
        if (xe != -1) {
            DrawHLineFast(x, y, xe, ConvertedColour);
        }
        //draw current scanline from start position to the left
        x1 = x - 1;
        xs = -1;
        while (x1 >= 0 && ReadPixelFast(x1, y) == filloldcolour)
        {
            xs = x1;
            x1--;
        }
        if (xs != -1) {
            DrawHLineFast(xs, y, x - 1, ConvertedColour);
        }

        //test for new scanlines above
        x1 = x;
        if (xe != -1) {
            while (x1 <= xe)
            {
                if (y > 0 && ReadPixelFast(x1, (y - 1)) == filloldcolour)
                {
                    floodFillScanline(x1, y - 1);
                }
                x1++;
            }
        }
        x1 = x - 1;
        if (xs != -1) {
            while (x1 >= xs)
            {
                if (y > 0 && ReadPixelFast(x1, (y - 1)) == filloldcolour)
                {
                    floodFillScanline(x1, y - 1);
                }
                x1--;
            }
        }

        //test for new scanlines below
        x1 = x;
        if (xe != -1) {
            while (x1 <= xe)
            {
                if (y < fillmaxH - 1 && ReadPixelFast(x1, (y + 1)) == filloldcolour)
                {
                    floodFillScanline(x1, y + 1);
                }
                x1++;
            }
        }
        x1 = x - 1;
        if (xs != -1) {
            while (x1 >= xs)
            {
                if (y < fillmaxH - 1 && ReadPixelFast(x1, (y + 1)) == filloldcolour)
                {
                    floodFillScanline(x1, y + 1);
                }
                x1--;
            }
        }
    }
    void cmd_pixel(void) {
        unsigned char* nostackp;
        if ((nostackp = checkstring(cmdline, (unsigned char *)"FILL"))) {
            fillmaxH = VRes;
            fillmaxW = HRes;
            {
                int x1, y1;
                getargs(&nostackp, 5, (unsigned char*)",");
                x1 = (int)getint(argv[0],0, fillmaxW);
                y1 = (int)getint(argv[2],0, fillmaxH);
                ConvertedColour = getColour(argv[4], 0);
                filloldcolour = ReadPixelFast(x1, y1);
                floodFillScanline(x1, y1);
            }
            return;
        }
        if (CMM1) {
            int x, y;
            int64_t value;
            getcoord((char *)cmdline, &x, &y);
            cmdline = getclosebracket(cmdline) + 1;
            while (*cmdline && tokenfunction(*cmdline) != op_equal) cmdline++;
            if (!*cmdline) error((char*)"Invalid syntax");
            ++cmdline;
            if (!*cmdline) error((char*)"Invalid syntax");
            value = getColour(cmdline, 0);
            DrawPixel(x, y, value);
            lastx = x; lasty = y;
        }
        else {
            int x1, y1, n = 0, i, nc = 0;
            int64_t c;
            long long int* x1ptr=NULL, * y1ptr=NULL, * cptr=NULL;
            MMFLOAT* x1fptr=NULL, * y1fptr=NULL, * cfptr=NULL;
            getargs(&cmdline, 5, (unsigned char *)",");
            if (!(argc == 3 || argc == 5)) error((char*)"Argument count");
            getargaddress(argv[0], &x1ptr, &x1fptr, &n);
            if (n != 1) getargaddress(argv[2], &y1ptr, &y1fptr, &n);
            if (n == 1) { //just a single point
                c = gui_fcolour;                                    // setup the defaults
                x1 = (int)getinteger(argv[0]);
                y1 = (int)getinteger(argv[2]);
                if (argc == 5)
                    c = getint(argv[4], M_CLEAR, M_WHITE);
                else
                    c = gui_fcolour;
                DrawPixel(x1, y1, c);
            }
            else {
                c = gui_fcolour;                                        // setup the defaults
                if (argc == 5) {
                    getargaddress(argv[4], &cptr, &cfptr, &nc);
                    if (nc == 1) c = getint(argv[4], M_CLEAR, M_WHITE);
                    else if (nc > 1) {
                        if (nc < n) n = nc; //adjust the dimensionality
                        for (i = 0; i < nc; i++) {
                            c = (cfptr == NULL ? cptr[i] : (int64_t)cfptr[i]);
                            if (c < 0 || c > M_WHITE) error((char *)"% is invalid (valid is % to %)", c, M_CLEAR, M_WHITE);
                        }
                    }
                }
                for (i = 0; i < n; i++) {
                    x1 = (x1fptr == NULL ? (int)x1ptr[i] : (int)x1fptr[i]);
                    y1 = (y1fptr == NULL ? (int)y1ptr[i] : (int)y1fptr[i]);
                    if (nc > 1) c = (cfptr == NULL ? cptr[i] : (int64_t)cfptr[i]);
                    DrawPixel(x1, y1, c);
                }
            }
        }
    }
    void fun_mmhres(void) {
        iret = HRes;
        targ = T_INT;
    }



    void fun_mmvres(void) {
        iret = VRes;
        targ = T_INT;
    }
    void fun_mmcharwidth(void) {
        iret = FontTable[gui_font >> 4][0] * (gui_font & 0b1111);
        targ = T_INT;
    }


    void fun_mmcharheight(void) {
        iret = FontTable[gui_font >> 4][1] * (gui_font & 0b1111);
        targ = T_INT;
    }

    void cmd_circle(void) {
        if (CMM1) {
            int x, y, radius;
            int64_t colour, fill;
            float aspect;
            getargs(&cmdline, 9, (unsigned char *)",");
            if (argc % 2 == 0 || argc < 3) error((char *)"Invalid syntax");
            if (*argv[0] != '(') error((char *)"Expected opening bracket");
            if (toupper(*argv[argc - 1]) == 'F') {
                argc -= 2;
                fill = true;
            }
            else fill = false;
            getcoord((char *)argv[0], &x, &y);
            radius = (int)getinteger(argv[2]);
            if (radius == 0) return;                                         //nothing to draw
            if (radius < 1) error((char *)"Invalid argument");
            if (argc > 3 && *argv[4])colour = getColour(argv[4], 0);
            else colour = gui_fcolour;

            if (argc > 5 && *argv[6])
                aspect = (float)getnumber(argv[6]);
            else
                aspect = 1;

            DrawCircle(x, y, radius, (fill ? 0 : 1), colour, (fill ? colour : -1), aspect);
            lastx = x; lasty = y;
        }
        else {
            int x, y, r, w = 0, n = 0, i, nc = 0, nw = 0, nf = 0, na = 0;
            int64_t c = 0, f = 0;
            MMFLOAT a;
            long long int* xptr=NULL, * yptr = NULL, * rptr = NULL, * fptr = NULL, * wptr = NULL, * cptr = NULL, * aptr = NULL;
            MMFLOAT* xfptr = NULL, * yfptr = NULL, * rfptr = NULL, * ffptr = NULL, * wfptr = NULL, * cfptr = NULL, * afptr = NULL;
            getargs(&cmdline, 13, (unsigned char*)",");
            if (!(argc & 1) || argc < 5) error((char *)"Argument count");
            getargaddress(argv[0], &xptr, &xfptr, &n);
            if (n != 1) {
                getargaddress(argv[2], &yptr, &yfptr, &n);
                getargaddress(argv[4], &rptr, &rfptr, &n);
            }
            if (n == 1) {
                w = 1; c = gui_fcolour; f = -1; a = 1;                          // setup the defaults
                x = (int)getinteger(argv[0]);
                y = (int)getinteger(argv[2]);
                r = (int)getinteger(argv[4]);
                if (argc > 5 && *argv[6]) w = (int)getint(argv[6], 0, 100);
                if (argc > 7 && *argv[8]) a = getnumber(argv[8]);
                if (argc > 9 && *argv[10]) c = getint(argv[10], M_CLEAR, M_WHITE);
                if (argc > 11) f = getint(argv[12], -1, M_WHITE);
                DrawCircle(x, y, r, w, c, f, a);
            }
            else {
                w = 1; c = gui_fcolour; f = -1; a = 1;                          // setup the defaults
                if (argc > 5 && *argv[6]) {
                    getargaddress(argv[6], &wptr, &wfptr, &nw);
                    if (nw == 1) w = (int)getint(argv[6], 0, 100);
                    else if (nw > 1) {
                        if (nw > 1 && nw < n) n = nw; //adjust the dimensionality
                        for (i = 0; i < nw; i++) {
                            w = (wfptr == NULL ? (int)wptr[i] : (int)wfptr[i]);
                            if (w < 0 || w > 100) error((char *)"% is invalid (valid is % to %)", (int)w, 0, 100);
                        }
                    }
                }
                if (argc > 7 && *argv[8]) {
                    getargaddress(argv[8], &aptr, &afptr, &na);
                    if (na == 1) a = getnumber(argv[8]);
                    if (na > 1 && na < n) n = na; //adjust the dimensionality
                }
                if (argc > 9 && *argv[10]) {
                    getargaddress(argv[10], &cptr, &cfptr, &nc);
                    if (nc == 1) c = getint(argv[10], M_CLEAR, M_WHITE);
                    else if (nc > 1) {
                        if (nc > 1 && nc < n) n = nc; //adjust the dimensionality
                        for (i = 0; i < nc; i++) {
                            c = (cfptr == NULL ? cptr[i] : (int64_t)cfptr[i]);
                            if (c < 0 || c > M_WHITE) error((char *)"% is invalid (valid is % to %)", c, M_CLEAR, M_WHITE);
                        }
                    }
                }
                if (argc > 11) {
                    getargaddress(argv[12], &fptr, &ffptr, &nf);
                    if (nf == 1) f = getint(argv[12], -1, M_WHITE);
                    else if (nf > 1) {
                        if (nf > 1 && nf < n) n = nf; //adjust the dimensionality
                        for (i = 0; i < nf; i++) {
                            f = (ffptr == NULL ? fptr[i] : (int64_t)ffptr[i]);
                            if (f < 0 || f > M_WHITE) error((char *)"% is invalid (valid is % to %)", f, M_CLEAR, M_WHITE);
                        }
                    }
                }
                for (i = 0; i < n; i++) {
                    x = (xfptr == NULL ? (int)xptr[i] : (int)xfptr[i]);
                    y = (yfptr == NULL ? (int)yptr[i] : (int)yfptr[i]);
                    r = (rfptr == NULL ? (int)rptr[i] : (int)rfptr[i]) - 1;
                    if (nw > 1) w = (wfptr == NULL ? (int)wptr[i] : (int)wfptr[i]);
                    if (nc > 1) c = (cfptr == NULL ? (int)cptr[i] : (int)cfptr[i]);
                    if (nf > 1) f = (ffptr == NULL ? (int)fptr[i] : (int)ffptr[i]);
                    if (na > 1) a = (afptr == NULL ? (MMFLOAT)aptr[i] : afptr[i]);
                    DrawCircle(x, y, r, w, c, f, a);
                }
            }
        }
    }

    void cmd_line(void) {
        if (CMM1) {
            int box, x1, y1, x2, y2;
            int64_t colour, fill;
            char* p;
            getargs(&cmdline, 5, (unsigned char*)",");

            // check if it is actually a LINE INPUT command
            if (argc < 1) error((char *)"Invalid syntax");
            x1 = lastx; y1 = lasty; colour = gui_fcolour; box = false; fill = false;	// set the defaults for optional components
            p = (char *)argv[0];
            if (tokenfunction(*p) != op_subtract) {
                // the start point is specified - get the coordinates and step over to where the minus token should be
                if (*p != '(') error((char *)"Expected opening bracket");
                getcoord(p, &x1, &y1);
                p = (char*)getclosebracket((unsigned char*)p) + 1;
                skipspace(p);
            }
            if (tokenfunction(*p) != op_subtract) error((char *)"Invalid syntax");
            p++;
            skipspace(p);
            if (*p != '(') error((char *)"Expected opening bracket");
            getcoord(p, &x2, &y2);
            if (argc > 1 && *argv[2]) {
                colour = getColour(argv[2], 0);
            }
            if (argc == 5) {
                box = (strchr((const char *)argv[4], 'b') != NULL || strchr((const char*)argv[4], 'B') != NULL);
                fill = (strchr((const char*)argv[4], 'f') != NULL || strchr((const char*)argv[4], 'F') != NULL);
            }
            if (box)
                DrawBox(x1, y1, x2, y2, 1, colour, (fill ? colour : -1));						// draw a box
            else
                DrawLine(x1, y1, x2, y2, 1, colour);								// or just a line

            lastx = x2; lasty = y2;											// save in case the user wants the last value
        }
        else {
            int x1, y1, x2, y2, w = 0, n = 0, i, nc = 0, nw = 0;
            int64_t c = 0;
            long long int* x1ptr = NULL, * y1ptr = NULL, * x2ptr = NULL, * y2ptr = NULL, * wptr = NULL, * cptr = NULL;
            MMFLOAT* x1fptr = NULL, * y1fptr = NULL, * x2fptr = NULL, * y2fptr = NULL, * wfptr = NULL, * cfptr = NULL;
            getargs(&cmdline, 11, (unsigned char*)",");
            if (!(argc & 1) || argc < 7) error((char *)"Argument count");
            getargaddress(argv[0], &x1ptr, &x1fptr, &n);
            if (n != 1) {
                getargaddress(argv[2], &y1ptr, &y1fptr, &n);
                getargaddress(argv[4], &x2ptr, &x2fptr, &n);
                getargaddress(argv[6], &y2ptr, &y2fptr, &n);
            }
            if (n == 1) {
                c = gui_fcolour;  w = 1;                                        // setup the defaults
                x1 = (int)getinteger(argv[0]);
                y1 = (int)getinteger(argv[2]);
                x2 = (int)getinteger(argv[4]);
                y2 = (int)getinteger(argv[6]);
                if (argc > 7 && *argv[8]) {
                    w = (int)getint(argv[8], 1, 100);
                }
                if (argc == 11) c = getint(argv[10], M_CLEAR, M_WHITE);
                DrawLine(x1, y1, x2, y2, w, c);
            }
            else {
                c = gui_fcolour;  w = 1;                                        // setup the defaults
                if (argc > 7 && *argv[8]) {
                    getargaddress(argv[8], &wptr, &wfptr, &nw);
                    if (nw == 1) w = (int)getint(argv[8], 0, 100);
                    else if (nw > 1) {
                        if (nw > 1 && nw < n) n = nw; //adjust the dimensionality
                        for (i = 0; i < nw; i++) {
                            w = (wfptr == NULL ? (int)wptr[i] : (int)wfptr[i]);
                            if (w < 0 || w > 100) error((char *)"% is invalid (valid is % to %)", (int)w, 0, 100);
                        }
                    }
                }
                if (argc == 11) {
                    getargaddress(argv[10], &cptr, &cfptr, &nc);
                    if (nc == 1) c = getint(argv[10], M_CLEAR, M_WHITE);
                    else if (nc > 1) {
                        if (nc > 1 && nc < n) n = nc; //adjust the dimensionality
                        for (i = 0; i < nc; i++) {
                            c = (cfptr == NULL ? cptr[i] : (int64_t)cfptr[i]);
                            if (c < 0 || c > M_WHITE) error((char *)"% is invalid (valid is % to %)", c, M_CLEAR, M_WHITE);
                        }
                    }
                }
                for (i = 0; i < n; i++) {
                    x1 = (x1fptr == NULL ? (int)x1ptr[i] : (int)x1fptr[i]);
                    y1 = (y1fptr == NULL ? (int)y1ptr[i] : (int)y1fptr[i]);
                    x2 = (x2fptr == NULL ? (int)x2ptr[i] : (int)x2fptr[i]);
                    y2 = (y2fptr == NULL ? (int)y2ptr[i] : (int)y2fptr[i]);
                    if (nw > 1) w = (wfptr == NULL ? (int)wptr[i] : (int)wfptr[i]);
                    if (nc > 1) c = (cfptr == NULL ? (int)cptr[i] : (int)cfptr[i]);
                    DrawLine(x1, y1, x2, y2, w, c);
                }
            }
        }
    }

    void cmd_colour(void) {
        getargs(&cmdline, 3, (unsigned char *)",");
        if (argc < 1) error((char *)"Argument count");
        gui_fcolour = getColour(argv[0], 0);
        if (argc == 3)  gui_bcolour = getColour(argv[2], 0);
        last_fcolour = gui_fcolour;
        last_bcolour = gui_bcolour;
        if (!CurrentLinePtr) {
            PromptFC = gui_fcolour;
            PromptBC = gui_bcolour;
        }
    }
    void cmd_box(void) {
        int x1, y1, wi, h, w = 0, n = 0, i, nc = 0, nw = 0, nf = 0, hmod, wmod;
        int64_t c = 0, f = 0;
        long long int* x1ptr=NULL, * y1ptr = NULL, * wiptr = NULL, * hptr = NULL, * wptr = NULL, * cptr = NULL, * fptr = NULL;
        MMFLOAT* x1fptr = NULL, * y1fptr = NULL, * wifptr = NULL, * hfptr = NULL, * wfptr = NULL, * cfptr = NULL, * ffptr = NULL;
        getargs(&cmdline, 13, (unsigned char*)",");
        if (!(argc & 1) || argc < 7) error((char *)"Argument count");
        getargaddress(argv[0], &x1ptr, &x1fptr, &n);
        if (n != 1) {
            getargaddress(argv[2], &y1ptr, &y1fptr, &n);
            getargaddress(argv[4], &wiptr, &wifptr, &n);
            getargaddress(argv[6], &hptr, &hfptr, &n);
        }
        if (n == 1) {
            c = gui_fcolour; w = 1; f = -1;                                 // setup the defaults
            x1 = (int)getinteger(argv[0]);
            y1 = (int)getinteger(argv[2]);
            wi = (int)getinteger(argv[4]);
            h = (int)getinteger(argv[6]);
            wmod = (wi > 0 ? -1 : 1);
            hmod = (h > 0 ? -1 : 1);
            if (argc > 7 && *argv[8]) w = (int)getint(argv[8], 0, 100);
            if (argc > 9 && *argv[10]) c = getint(argv[10], M_CLEAR, M_WHITE);
            if (argc == 13) f = getint(argv[12], -1, M_WHITE);
            if (wi != 0 && h != 0) DrawBox(x1, y1, x1 + wi + wmod, y1 + h + hmod, w, c, f);
        }
        else {
            c = gui_fcolour;  w = 1;                                        // setup the defaults
            if (argc > 7 && *argv[8]) {
                getargaddress(argv[8], &wptr, &wfptr, &nw);
                if (nw == 1) w = (int)getint(argv[8], 0, 100);
                else if (nw > 1) {
                    if (nw > 1 && nw < n) n = nw; //adjust the dimensionality
                    for (i = 0; i < nw; i++) {
                        w = (wfptr == NULL ? (int)wptr[i] : (int)wfptr[i]);
                        if (w < 0 || w > 100) error((char *)"% is invalid (valid is % to %)", (int)w, 0, 100);
                    }
                }
            }
            if (argc > 9 && *argv[10]) {
                getargaddress(argv[10], &cptr, &cfptr, &nc);
                if (nc == 1) c = getint(argv[10], M_CLEAR, M_WHITE);
                else if (nc > 1) {
                    if (nc > 1 && nc < n) n = nc; //adjust the dimensionality
                    for (i = 0; i < nc; i++) {
                        c = (cfptr == NULL ? cptr[i] : (int64_t)cfptr[i]);
                        if (c < 0 || c > M_WHITE) error((char *)"% is invalid (valid is % to %)", (int64_t)c, M_CLEAR, M_WHITE);
                    }
                }
            }
            if (argc == 13) {
                getargaddress(argv[12], &fptr, &ffptr, &nf);
                if (nf == 1) f = getint(argv[12], M_CLEAR, M_WHITE);
                else if (nf > 1) {
                    if (nf > 1 && nf < n) n = nf; //adjust the dimensionality
                    for (i = 0; i < nf; i++) {
                        f = (ffptr == NULL ? fptr[i] : (int64_t)ffptr[i]);
                        if (f < -1 || f > M_WHITE) error((char *)"% is invalid (valid is % to %)", (int64_t)f, -1, M_WHITE);
                    }
                }
            }
            for (i = 0; i < n; i++) {
                x1 = (x1fptr == NULL ? (int)x1ptr[i] : (int)x1fptr[i]);
                y1 = (y1fptr == NULL ? (int)y1ptr[i] : (int)y1fptr[i]);
                wi = (wifptr == NULL ? (int)wiptr[i] : (int)wifptr[i]);
                h = (hfptr == NULL ? (int)hptr[i] : (int)hfptr[i]);
                wmod = (wi > 0 ? -1 : 1);
                hmod = (h > 0 ? -1 : 1);
                if (nw > 1) w = (wfptr == NULL ? (int)wptr[i] : (int)wfptr[i]);
                if (nc > 1) c = (cfptr == NULL ? (int)cptr[i] : (int)cfptr[i]);
                if (nf > 1) f = (ffptr == NULL ? (int)fptr[i] : (int)ffptr[i]);
                if (wi != 0 && h != 0) DrawBox(x1, y1, x1 + wi + wmod, y1 + h + hmod, w, c, f);

            }
        }
    }

    void cmd_rbox(void) {
        int x1, y1, wi, h, w = 0, r = 0, n = 0, i, nc = 0, nw = 0, nf = 0, hmod, wmod;
        int64_t c = 0, f = 0;
        long long int* x1ptr=NULL, * y1ptr = NULL, * wiptr = NULL, * hptr = NULL, * wptr = NULL, * cptr = NULL, * fptr = NULL;
        MMFLOAT* x1fptr = NULL, * y1fptr = NULL, * wifptr = NULL, * hfptr = NULL, * wfptr = NULL, * cfptr = NULL, * ffptr = NULL;
        getargs(&cmdline, 13, (unsigned char*)",");
        if (!(argc & 1) || argc < 7) error((char *)"Argument count");
        getargaddress(argv[0], &x1ptr, &x1fptr, &n);
        if (n != 1) {
            getargaddress(argv[2], &y1ptr, &y1fptr, &n);
            getargaddress(argv[4], &wiptr, &wifptr, &n);
            getargaddress(argv[6], &hptr, &hfptr, &n);
        }
        if (n == 1) {
            c = gui_fcolour; w = 1; f = -1; r = 10;                         // setup the defaults
            x1 = (int)getinteger(argv[0]);
            y1 = (int)getinteger(argv[2]);
            w = (int)getinteger(argv[4]);
            h = (int)getinteger(argv[6]);
            wmod = (w > 0 ? -1 : 1);
            hmod = (h > 0 ? -1 : 1);
            if (argc > 7 && *argv[8]) r = (int)getint(argv[8], 0, 100);
            if (argc > 9 && *argv[10]) c = getint(argv[10], M_CLEAR, M_WHITE);
            if (argc == 13) f = getint(argv[12], -1, M_WHITE);
            if (w != 0 && h != 0) DrawRBox(x1, y1, x1 + w + wmod, y1 + h + hmod, r, c, f);
        }
        else {
            c = gui_fcolour;  w = 1;                                        // setup the defaults
            if (argc > 7 && *argv[8]) {
                getargaddress(argv[8], &wptr, &wfptr, &nw);
                if (nw == 1) w = (int)getint(argv[8], 0, 100);
                else if (nw > 1) {
                    if (nw > 1 && nw < n) n = nw; //adjust the dimensionality
                    for (i = 0; i < nw; i++) {
                        w = (wfptr == NULL ? (int)wptr[i] : (int)wfptr[i]);
                        if (w < 0 || w > 100) error((char *)"% is invalid (valid is % to %)", (int)w, 0, 100);
                    }
                }
            }
            if (argc > 9 && *argv[10]) {
                getargaddress(argv[10], &cptr, &cfptr, &nc);
                if (nc == 1) c = getint(argv[10], M_CLEAR, M_WHITE);
                else if (nc > 1) {
                    if (nc > 1 && nc < n) n = nc; //adjust the dimensionality
                    for (i = 0; i < nc; i++) {
                        c = (cfptr == NULL ? cptr[i] : (int64_t)cfptr[i]);
                        if (c < 0 || c > M_WHITE) error((char *)"% is invalid (valid is % to %)", c, M_CLEAR, M_WHITE);
                    }
                }
            }
            if (argc == 13) {
                getargaddress(argv[12], &fptr, &ffptr, &nf);
                if (nf == 1) f = getint(argv[12], M_CLEAR, M_WHITE);
                else if (nf > 1) {
                    if (nf > 1 && nf < n) n = nf; //adjust the dimensionality
                    for (i = 0; i < nf; i++) {
                        f = (ffptr == NULL ? fptr[i] : (int64_t)ffptr[i]);
                        if (f < -1 || f > M_WHITE) error((char *)"% is invalid (valid is % to %)", f, -1, M_WHITE);
                    }
                }
            }
            for (i = 0; i < n; i++) {
                x1 = (x1fptr == NULL ? (int)x1ptr[i] : (int)x1fptr[i]);
                y1 = (y1fptr == NULL ? (int)y1ptr[i] : (int)y1fptr[i]);
                wi = (wifptr == NULL ? (int)wiptr[i] : (int)wifptr[i]);
                h = (hfptr == NULL ? (int)hptr[i] : (int)hfptr[i]);
                wmod = (wi > 0 ? -1 : 1);
                hmod = (h > 0 ? -1 : 1);
                if (nw > 1) w = (wfptr == NULL ? (int)wptr[i] : (int)wfptr[i]);
                if (nc > 1) c = (cfptr == NULL ? (int)cptr[i] : (int)cfptr[i]);
                if (nf > 1) f = (ffptr == NULL ? (int)fptr[i] : (int)ffptr[i]);
                if (wi != 0 && h != 0) DrawRBox(x1, y1, x1 + wi + wmod, y1 + h + hmod, w, c, f);
            }
        }
    }
    extern "C" uint32_t * getpageaddress(int pageno) {
        uint8_t* page = (uint8_t*)FrameBuffer;
        page += (pageno * SCREENSIZE);
        return (uint32_t*)page;
    }
    extern "C" void setmode(int mode, int argb, bool fullscreen) {
        static int currentmode = 0;
        static int currenttrans = 0;
        static bool currentfull = 0;
        if (mode == currentmode && currenttrans == argb && fullscreen == currentfull)return;
        currentmode = mode;
        currenttrans = argb;
        currentfull = fullscreen;
        ARGBenabled = 0;
        WritePage = 0;
        ReadPage = 0;
        if (PageTable[WPN].address != NULL) {
            FreeMemorySafe((void**)&PageTable[BPN].address);
            FreeMemorySafe((void**)&PageTable[WPN].address);
            PageTable[BPN].address = NULL;
            PageTable[WPN].address = NULL;
        }
        WritePage = 0;
        ReadPage = 0;
        HRes = xres[mode];
        VRes = yres[mode];
        FullScreen = fullscreen;
        PixelSize = pixeldensity[mode];
        OptionHeight = VRes / gui_font_height;
        OptionWidth = HRes / gui_font_width;
        for (int i = 0; i <= MAXPAGES; i++) {
            PageTable[i].address = getpageaddress(i);
            PageTable[i].xmax = HRes;
            PageTable[i].ymax = VRes;
            PageTable[i].size = SCREENSIZE;
        }
        SystemMode = MODE_RESIZE;
        uSec(500000);
        PageWrite1 = PageTable[1].address;
        if (argb) {
            for (int i = 0; i < HRes * VRes; i++) {
                PageWrite0[i] = M_BLACK;
                PageWrite1[i] = 0;
            }
            ARGBenabled = 1;
        }
        return;
    }
    void cmd_mode(void) {
        int mode, argb;
        bool fullscreen=false;
        unsigned char* p=NULL;
        {
            getargs(&cmdline, 5, (unsigned char*)",");
            if (!argc)error((char*)"Syntax");
            mode = (int)getint(argv[0], -MAXMODES, MAXMODES);
            if (mode == 0)error((char*)"Invalid mode");
            argb = 0;
            if (mode < 0) {
                mode = -mode;
                fullscreen = true;
            }
            BackGroundColour = M_BLACK;
            if (argc >= 3)argb = (int)getint(argv[2], 0, 32);
            if (!(argb == 1 || argb == 12))argb = 0;
            else argb = 1;
            if (argc == 5)BackGroundColour = getint(argv[4], 0, M_WHITE);
            setmode(mode,argb, fullscreen);
            gui_fcolour = PromptFC = M_WHITE;
            gui_bcolour = PromptBC = M_BLACK;
            VideoMode = mode;
            return;
        }
    }


    void cmd_triangle(void) {                                           // thanks to Peter Mather (matherp on the Back Shed forum)
        int x1, y1, x2, y2, x3, y3, n = 0, i, nc = 0, nf = 0;
        int64_t c = 0, f = 0;
        long long int* x3ptr=NULL, * y3ptr = NULL, * x1ptr = NULL, * y1ptr = NULL, * x2ptr = NULL, * y2ptr = NULL, * fptr = NULL, * cptr = NULL;
        MMFLOAT* x3fptr = NULL, * y3fptr = NULL, * x1fptr = NULL, * y1fptr = NULL, * x2fptr = NULL, * y2fptr = NULL, * ffptr = NULL, * cfptr = NULL;
        getargs(&cmdline, 15, (unsigned char *)",");
        if (!(argc & 1) || argc < 11) error((char *)"Argument count");
        getargaddress(argv[0], &x1ptr, &x1fptr, &n);
        if (n != 1) {
            int cn = n;
            getargaddress(argv[2], &y1ptr, &y1fptr, &n);
            if (n < cn)cn = n;
            getargaddress(argv[4], &x2ptr, &x2fptr, &n);
            if (n < cn)cn = n;
            getargaddress(argv[6], &y2ptr, &y2fptr, &n);
            if (n < cn)cn = n;
            getargaddress(argv[8], &x3ptr, &x3fptr, &n);
            if (n < cn)cn = n;
            getargaddress(argv[10], &y3ptr, &y3fptr, &n);
            if (n < cn)cn = n;
            n = cn;
        }
        if (n == 1) {
            c = gui_fcolour; f = -1;
            x1 = (int)getinteger(argv[0]);
            y1 = (int)getinteger(argv[2]);
            x2 = (int)getinteger(argv[4]);
            y2 = (int)getinteger(argv[6]);
            x3 = (int)getinteger(argv[8]);
            y3 = (int)getinteger(argv[10]);
            if (argc >= 13 && *argv[12]) c = getint(argv[12], M_CLEAR, M_WHITE);
            if (argc == 15) f = getint(argv[14], -1, M_WHITE);
            DrawTriangle(x1, y1, x2, y2, x3, y3, c, f);
        }
        else {
            c = gui_fcolour; f = -1;
            if (argc >= 13 && *argv[12]) {
                getargaddress(argv[12], &cptr, &cfptr, &nc);
                if (nc == 1) c = getint(argv[10], M_CLEAR, M_WHITE);
                else if (nc > 1) {
                    if (nc > 1 && nc < n) n = nc; //adjust the dimensionality
                    for (i = 0; i < nc; i++) {
                        c = (cfptr == NULL ? cptr[i] : (int64_t)cfptr[i]);
                        if (c < 0 || c > M_WHITE) error((char *)"% is invalid (valid is % to %)", c, M_CLEAR, M_WHITE);
                    }
                }
            }
            if (argc == 15) {
                getargaddress(argv[14], &fptr, &ffptr, &nf);
                if (nf == 1) f = getint(argv[14], -1, M_WHITE);
                else if (nf > 1) {
                    if (nf > 1 && nf < n) n = nf; //adjust the dimensionality
                    for (i = 0; i < nf; i++) {
                        f = (ffptr == NULL ? fptr[i] : (int64_t)ffptr[i]);
                        if (f < -1 || f > M_WHITE) error((char *)"% is invalid (valid is % to %)", f, -1, M_WHITE);
                    }
                }
            }
            for (i = 0; i < n; i++) {
                x1 = (x1fptr == NULL ? (int)x1ptr[i] : (int)x1fptr[i]);
                y1 = (y1fptr == NULL ? (int)y1ptr[i] : (int)y1fptr[i]);
                x2 = (x2fptr == NULL ? (int)x2ptr[i] : (int)x2fptr[i]);
                y2 = (y2fptr == NULL ? (int)y2ptr[i] : (int)y2fptr[i]);
                x3 = (x3fptr == NULL ? (int)x3ptr[i] : (int)x3fptr[i]);
                y3 = (y3fptr == NULL ? (int)y3ptr[i] : (int)y3fptr[i]);
                if (x1 == x2 && x1 == x3 && y1 == y2 && y1 == y3 && x1 == -1 && y1 == -1)return;
                if (nc > 1) c = (cfptr == NULL ? (int)cptr[i] : (int)cfptr[i]);
                if (nf > 1) f = (ffptr == NULL ? (int)fptr[i] : (int)ffptr[i]);
                DrawTriangle(x1, y1, x2, y2, x3, y3, c, f);
            }
        }
    }


    void pointcalc(int angle, int x, int y, int r2, int* x0, int* y0) {
        MMFLOAT c1, s1;
        int quad;
        angle %= 360;
        switch (angle) {
        case 0:
            *x0 = x;
            *y0 = y - r2;
            break;
        case 45:
            *x0 = x + r2 + 1;
            *y0 = y - r2;
            break;
        case 90:
            *x0 = x + r2 + 1;
            *y0 = y;
            break;
        case 135:
            *x0 = x + r2 + 1;
            *y0 = y + r2;
            break;
        case 180:
            *x0 = x;
            *y0 = y + r2;
            break;
        case 225:
            *x0 = x - r2;
            *y0 = y + r2;
            break;
        case 270:
            *x0 = x - r2;
            *y0 = y;
            break;
        case 315:
            *x0 = x - r2;
            *y0 = y - r2;
            break;
        default:
            c1 = cos(Rad(angle));
            s1 = sin(Rad(angle));
            quad = (angle / 45) % 8;
            switch (quad) {
            case 0:
                *y0 = y - r2;
                *x0 = (int)(x + s1 * r2 / c1);
                break;
            case 1:
                *x0 = x + r2 + 1;
                *y0 = (int)(y - c1 * r2 / s1);
                break;
            case 2:
                *x0 = x + r2 + 1;
                *y0 = (int)(y - c1 * r2 / s1);
                break;
            case 3:
                *y0 = y + r2;
                *x0 = (int)(x - s1 * r2 / c1);
                break;
            case 4:
                *y0 = y + r2;
                *x0 = (int)(x - s1 * r2 / c1);
                break;
            case 5:
                *x0 = x - r2;
                *y0 = (int)(y + c1 * r2 / s1);
                break;
            case 6:
                *x0 = x - r2;
                *y0 = (int)(y + c1 * r2 / s1);
                break;
            case 7:
                *y0 = y - r2;
                *x0 = (int)(x + s1 * r2 / c1);
                break;
            }
        }
    }


    void ClearTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int ints_per_line, uint32_t* br) {
        if (x0 * (y1 - y2) + x1 * (y2 - y0) + x2 * (y0 - y1) == 0)return;
        long a, b, y, last;
        long  dx01, dy01, dx02, dy02, dx12, dy12, sa, sb;

        if (y0 > y1) {
            std::swap(y0, y1);
            std::swap(x0, x1);
        }
        if (y1 > y2) {
            std::swap(y2, y1);
            std::swap(x2, x1);
        }
        if (y0 > y1) {
            std::swap(y0, y1);
            std::swap(x0, x1);
        }

        dx01 = x1 - x0;  dy01 = y1 - y0;  dx02 = x2 - x0;
        dy02 = y2 - y0; dx12 = x2 - x1;  dy12 = y2 - y1;
        sa = 0; sb = 0;
        if (y1 == y2) {
            last = y1;                                          //Include y1 scanline
        }
        else {
            last = y1 - 1;                                      // Skip it
        }
        for (y = y0; y <= last; y++) {
            a = x0 + sa / dy01;
            b = x0 + sb / dy02;
            sa = sa + dx01;
            sb = sb + dx02;
            a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
            if (a > b)std::swap(a, b);
            hline(a, b, y, 0, ints_per_line, br);
        }
        sa = dx12 * (y - y1);
        sb = dx02 * (y - y0);
        while (y <= y2) {
            a = x1 + sa / dy12;
            b = x0 + sb / dy02;
            sa = sa + dx12;
            sb = sb + dx02;
            a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
            b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
            if (a > b) std::swap(a, b);
            hline(a, b, y, 0, ints_per_line, br);
            y = y + 1;
        }
    }
    void cmd_arc(void) {
        // Parameters are:
        // X coordinate of centre of arc
        // Y coordinate of centre of arc
        // inner radius of arc
        // outer radius of arc - omit it 1 pixel wide
        // start radial of arc in degrees
        // end radial of arc in degrees
        // Colour of arc
        int x, y, r1, r2, i, j, k, xs = -1, xi = 0, m;
        int64_t c;
        int arcrad1, arcrad2, arcrad3, rstart, quadr;
        int x0, y0, x1, y1, x2, y2, xr, yr;
        getargs(&cmdline, 13, (unsigned char *)",");
        if (!(argc == 11 || argc == 13)) error((char *)"Argument count");
        x = (int)getinteger(argv[0]);
        y = (int)getinteger(argv[2]);
        r1 = (int)getinteger(argv[4]);
        if (*argv[6])r2 = (int)getinteger(argv[6]);
        else {
            r2 = r1;
            r1--;
        }
        if (r2 < r1)error((char *)"Inner radius < outer");
        arcrad1 = (int)getinteger(argv[8]);
        arcrad2 = (int)getinteger(argv[10]);
        if (optiony) {
            arcrad1 += 180;
            arcrad2 += 180;
        }
        while (arcrad1 < 0.0)arcrad1 += 360;
        while (arcrad2 < 0.0)arcrad2 += 360;
        if (arcrad1 == arcrad2)error((char *)"Radials");
        if (argc == 13)
            c = getint(argv[12], M_CLEAR, M_WHITE);
        else
            c = gui_fcolour;
        while (arcrad2 < arcrad1)arcrad2 += 360;
        arcrad3 = arcrad1 + 360;
        rstart = arcrad2;
        int quad1 = (arcrad1 / 45) % 8;
        x2 = x; y2 = y;
        int ints_per_line = RoundUptoInt((r2 * 2) + 1) / 32;
        uint32_t* br = (uint32_t*)GetTempMemory(((ints_per_line + 1) * ((r2 * 2) + 1)) * 4);
        DrawFilledCircle(x, y, r2, r2, 1, ints_per_line, br, 1.0, 1.0);
        DrawFilledCircle(x, y, r1, r2, 0, ints_per_line, br, 1.0, 1.0);
        while (rstart < arcrad3) {
            pointcalc(rstart, x, y, r2, &x0, &y0);
            quadr = (rstart / 45) % 8;
            if (quadr == quad1 && arcrad3 - rstart < 45) {
                pointcalc(arcrad3, x, y, r2, &x1, &y1);
                ClearTriangle(x0 - x + r2, y0 - y + r2, x1 - x + r2, y1 - y + r2, x2 - x + r2, y2 - y + r2, ints_per_line, br);
                rstart = arcrad3;
            }
            else {
                rstart += 45;
                rstart -= (rstart % 45);
                pointcalc(rstart, x, y, r2, &xr, &yr);
                ClearTriangle(x0 - x + r2, y0 - y + r2, xr - x + r2, yr - y + r2, x2 - x + r2, y2 - y + r2, ints_per_line, br);
            }
        }
        for (j = 0; j < r2 * 2 + 1; j++) {
            for (i = 0; i < ints_per_line; i++) {
                k = br[i + j * ints_per_line];
                for (m = 0; m < 32; m++) {
                    if (xs == -1 && (k & 0x80000000)) {
                        xs = m;
                        xi = i;
                    }
                    if (xs != -1 && !(k & 0x80000000)) {
                        DrawRectangle(x - r2 + xs + xi * 32, y - r2 + j, x - r2 + m + i * 32, y - r2 + j, c);
                        xs = -1;
                    }
                    k <<= 1;
                }
            }
            if (xs != -1) {
                DrawRectangle(x - r2 + xs + xi * 32, y - r2 + j, x - r2 + m + i * 32, y - r2 + j, c);
                xs = -1;
            }
        }
    }

    void fill_set_pen_color(int red, int green, int blue, int transparency)
    {
        main_fill.pen_color.red = red;
        main_fill.pen_color.green = green;
        main_fill.pen_color.blue = blue;
    }

    void fill_set_fill_color(int red, int green, int blue, int transparency)
    {
        main_fill.fill_color.red = red;
        main_fill.fill_color.green = green;
        main_fill.fill_color.blue = blue;
    }
    static void fill_begin_fill()
    {
        main_fill.filled = true;
        main_fill_poly_vertex_count = 0;
    }
#define ABS(X) ((X)>0 ? (X) : (-(X)))
    void CalcLine(int x1, int y1, int x2, int y2, short* xmin, short* xmax) {

        if (y1 == y2) {
            if (x1 < xmin[y1])xmin[y1] = x1;
            if (x2 < xmin[y1])xmin[y1] = x2;
            if (x1 > xmax[y1])xmax[y1] = x1;
            if (x2 > xmax[y1])xmax[y1] = x2;
            return;
        }
        if (x1 == x2) {
            if (y2 < y1)std::swap(y2, y1);
            for (int y = y1; y <= y2; y++) {
                if (x1 < xmin[y])xmin[y] = x1;
                if (x1 > xmax[y])xmax[y] = x1;
            }
            return;
        }
        // uses a variant of Bresenham's line algorithm:
        //   https://en.wikipedia.org/wiki/Talk:Bresenham%27s_line_algorithm
        if (y1 > y2) {
            std::swap(y1, y2);
            std::swap(x1, x2);
        }
        int absX = ABS(x1 - x2);          // absolute value of coordinate distances
        int absY = ABS(y1 - y2);
        int offX = x2 < x1 ? 1 : -1;      // line-drawing direction offsets
        int offY = y2 < y1 ? 1 : -1;
        int x = x2;                     // incremental location
        int y = y2;
        int err;
        if (x < xmin[y])xmin[y] = x;
        if (x > xmax[y])xmax[y] = x;
        if (absX > absY) {

            // line is more horizontal; increment along x-axis
            err = absX / 2;
            while (x != x1) {
                err = err - absY;
                if (err < 0) {
                    y += offY;
                    err += absX;
                }
                x += offX;
                if (x < xmin[y])xmin[y] = x;
                if (x > xmax[y])xmax[y] = x;
            }
        }
        else {

            // line is more vertical; increment along y-axis
            err = absY / 2;
            while (y != y1) {
                err = err - absX;
                if (err < 0) {
                    x += offX;
                    err += absY;
                }
                y += offY;
                if (x < xmin[y])xmin[y] = x;
                if (x > xmax[y])xmax[y] = x;
            }
        }
    }



    static void fill_end_fill(int count, int ystart, int yend)
    {
        // based on public-domain fill algorithm in C by Darel Rex Finley, 2007
        //   from http://alienryderflex.com/polygon_fill/

        TFLOAT* nodeX = (TFLOAT *)GetMemory(count * sizeof(TFLOAT));     // x-coords of polygon intercepts
        int nodes;                              // size of nodeX
        int y, i, j;                         // current pixel and loop indices
        TFLOAT temp;                            // temporary variable for sorting
        int64_t  f = (main_fill.fill_color.red << 16) | (main_fill.fill_color.green << 8) | main_fill.fill_color.blue;
        int64_t  c = (main_fill.pen_color.red << 16) | (main_fill.pen_color.green << 8) | main_fill.pen_color.blue;
        int xstart, xend;
        //  loop through the rows of the image

        for (y = ystart; y < yend; y++) {

            //  build a list of polygon intercepts on the current line
            nodes = 0;
            j = main_fill_poly_vertex_count - 1;
            for (i = 0; i < main_fill_poly_vertex_count; i++) {
                if ((main_fill_polyY[i] < (TFLOAT)y &&
                    main_fill_polyY[j] >= (TFLOAT)y) ||
                    (main_fill_polyY[j] < (TFLOAT)y &&
                        main_fill_polyY[i] >= (TFLOAT)y)) {

                    // intercept found; record it
                    nodeX[nodes++] = (main_fill_polyX[i] +
                        ((TFLOAT)y - main_fill_polyY[i]) /
                        (main_fill_polyY[j] - main_fill_polyY[i]) *
                        (main_fill_polyX[j] - main_fill_polyX[i]));
                }
                j = i;
            }

            //  sort the nodes via simple insertion sort
            for (i = 1; i < nodes; i++) {
                temp = nodeX[i];
                for (j = i; j > 0 && temp < nodeX[j - 1]; j--) {
                    nodeX[j] = nodeX[j - 1];
                }
                nodeX[j] = temp;
            }

            //  fill the pixels between node pairs
            for (i = 0; i < nodes; i += 2) {
                xstart = (int)floor(nodeX[i]) + 1;
                xend = (int)ceil(nodeX[i + 1]) - 1;
                DrawLine(xstart, y, xend, y, 1, f);
            }
        }

        main_fill.filled = false;

        // redraw polygon (filling is imperfect and can occasionally occlude sides)
        for (i = 0; i < main_fill_poly_vertex_count; i++) {
            int x0 = (int)round(main_fill_polyX[i]);
            int y0 = (int)round(main_fill_polyY[i]);
            int x1 = (int)round(main_fill_polyX[(i + 1) %
                main_fill_poly_vertex_count]);
            int y1 = (int)round(main_fill_polyY[(i + 1) %
                main_fill_poly_vertex_count]);
            DrawLine(x0, y0, x1, y1, 1, c);
        }
        FreeMemory((unsigned char *)nodeX);
    }
    static void fill_fast_fill(int count, int ystart, int yend)
        // this version only works on convex shapes
    {
        int64_t f = (main_fill.fill_color.transparency << 24) | (main_fill.fill_color.red << 16) | (main_fill.fill_color.green << 8) | main_fill.fill_color.blue;
        int64_t c = (main_fill.pen_color.transparency << 24) | (main_fill.pen_color.red << 16) | (main_fill.pen_color.green << 8) | main_fill.pen_color.blue;
        int i, y;
        int64_t Colour;
        Colour = f;
        short* xmin = (short*)linebuff;
        short* xmax = (short*)(linebuff + 2160); //max number of lines is 1080
        for (y = ystart; y <= yend; y++) {
            xmin[y] = 32767;
            xmax[y] = -1;
        }

        for (i = 0; i < main_fill_poly_vertex_count; i++) {
            int x0 = (int)(main_fill_polyX[i]);
            int y0 = (int)(main_fill_polyY[i]);
            int x1 = (int)(main_fill_polyX[(i + 1) %
                main_fill_poly_vertex_count]);
            int y1 = (int)(main_fill_polyY[(i + 1) %
                main_fill_poly_vertex_count]);
            CalcLine(x0, y0, x1, y1, xmin, xmax);
        }
        for (y = ystart; y <= yend; y++)DrawHLineFast(xmin[y], y, xmax[y], Colour);
        if (f != c) {
            for (i = 0; i < main_fill_poly_vertex_count; i++) {
                int x0 = (int)(main_fill_polyX[i]);
                int y0 = (int)(main_fill_polyY[i]);
                int x1 = (int)(main_fill_polyX[(i + 1) %
                    main_fill_poly_vertex_count]);
                int y1 = (int)(main_fill_polyY[(i + 1) %
                    main_fill_poly_vertex_count]);
                DrawLine(x0, y0, x1, y1, 1, c);
            }
        }
    }

    void DrawPolygon(int n, short* xcoord, short* ycoord, int face) {
        int i, facecount = struct3d[n]->facecount[face];
        int64_t c = struct3d[n]->line[face];
        int64_t f = struct3d[n]->fill[face];
        // first deal with outline only
        if (struct3d[n]->fill[face] == 0xFFFFFFFF) {
            for (i = 0; i < facecount; i++) {
                if (i < facecount - 1) {
                    DrawLine(xcoord[i], ycoord[i], xcoord[i + 1], ycoord[i + 1], 1, c);
                }
                else {
                    DrawLine(xcoord[i], ycoord[i], xcoord[0], ycoord[0], 1, c);
                }
            }
        }
        else {
            if (facecount == 3) {
                DrawTriangle(xcoord[0], ycoord[0], xcoord[1], ycoord[1], xcoord[2], ycoord[2], c, f);
            }
            else if (facecount == 4) {
                DrawTriangle(xcoord[0], ycoord[0], xcoord[1], ycoord[1], xcoord[2], ycoord[2], f, f);
                DrawTriangle(xcoord[0], ycoord[0], xcoord[2], ycoord[2], xcoord[3], ycoord[3], f, f);
                if (f != c) {
                    DrawLine(xcoord[0], ycoord[0], xcoord[1], ycoord[1], 1, c);
                    DrawLine(xcoord[1], ycoord[1], xcoord[2], ycoord[2], 1, c);
                    DrawLine(xcoord[2], ycoord[2], xcoord[3], ycoord[3], 1, c);
                    DrawLine(xcoord[0], ycoord[0], xcoord[3], ycoord[3], 1, c);
                }
            }
            else {
                int  ymax = -1000000, ymin = 1000000;
                fill_set_pen_color((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xF);
                fill_set_fill_color((f >> 16) & 0xFF, (f >> 8) & 0xFF, f & 0xFF, (f >> 24) & 0xF);
                main_fill_poly_vertex_count = 0;
                for (i = 0; i < facecount; i++) {
                    main_fill_polyX[main_fill_poly_vertex_count] = (TFLOAT)xcoord[i];
                    main_fill_polyY[main_fill_poly_vertex_count] = (TFLOAT)ycoord[i];
                    if (main_fill_polyY[main_fill_poly_vertex_count] > ymax)ymax = (int)main_fill_polyY[main_fill_poly_vertex_count];
                    if (main_fill_polyY[main_fill_poly_vertex_count] < ymin)ymin = (int)main_fill_polyY[main_fill_poly_vertex_count];
                    main_fill_polyX[main_fill_poly_vertex_count] = (TFLOAT)xcoord[i];
                    main_fill_poly_vertex_count++;
                }
                if (main_fill_polyY[main_fill_poly_vertex_count] != main_fill_polyY[0] || main_fill_polyX[main_fill_poly_vertex_count] != main_fill_polyX[0]) {
                    main_fill_polyX[main_fill_poly_vertex_count] = main_fill_polyX[0];
                    main_fill_polyY[main_fill_poly_vertex_count] = main_fill_polyY[0];
                    main_fill_poly_vertex_count++;
                }
                fill_fast_fill(main_fill_poly_vertex_count - 1, ymin, ymax);
            }
        }

    }

    void cmd_polygon(void) {
        int xcount = 0;
        long long int* xptr = NULL, * yptr = NULL, xptr2 = 0, yptr2 = 0, * polycount = NULL, * cptr = NULL, * fptr = NULL;
        MMFLOAT* polycountf = NULL, * cfptr = NULL, * ffptr = NULL, * xfptr = NULL, * yfptr = NULL, xfptr2 = 0, yfptr2 = 0;
        int i, xtot = 0, ymax = 0, ymin = 1000000;
        int64_t f = 0, c;
        int n = 0, nx = 0, ny = 0, nc = 0, nf = 0;
        getargs(&cmdline, 9, (unsigned char *)",");
        getargaddress(argv[0], &polycount, &polycountf, &n);
        if (n == 1) {
            xcount = xtot = (int)getinteger(argv[0]);
            if (xcount < 3 || xcount>9999)error((char *)"Invalid number of vertices");
            getargaddress(argv[2], &xptr, &xfptr, &nx);
            if (nx < xtot)error((char *)"X Dimensions %", nx);
            getargaddress(argv[4], &yptr, &yfptr, &ny);
            if (ny < xtot)error((char *)"Y Dimensions %", ny);
            if (xptr)xptr2 = *xptr;
            else xfptr2 = *xfptr;
            if (yptr)yptr2 = *yptr;
            else yfptr2 = *yfptr;
            c = gui_fcolour;                                    // setup the defaults
            if (argc > 5 && *argv[6]) c = getint(argv[6], M_CLEAR, M_WHITE);
            if (argc > 7 && *argv[8]) {
                main_fill_polyX = (TFLOAT*)GetTempMemory(xtot * sizeof(TFLOAT));
                main_fill_polyY = (TFLOAT*)GetTempMemory(xtot * sizeof(TFLOAT));
                f = getint(argv[8], M_CLEAR, M_WHITE);
                fill_set_fill_color((f >> 16) & 0xFF, (f >> 8) & 0xFF, f & 0xFF, (f >> 24) & 0xFF);
                fill_set_pen_color((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF);
                fill_begin_fill();
            }
            for (i = 0; i < xcount - 1; i++) {
                if (argc > 7) {
                    main_fill_polyX[main_fill_poly_vertex_count] = (xfptr == NULL ? (TFLOAT)*xptr++ : (TFLOAT)*xfptr++);
                    main_fill_polyY[main_fill_poly_vertex_count] = (yfptr == NULL ? (TFLOAT)*yptr++ : (TFLOAT)*yfptr++);
                    if (main_fill_polyY[main_fill_poly_vertex_count] > ymax)ymax = (int)main_fill_polyY[main_fill_poly_vertex_count];
                    if (main_fill_polyY[main_fill_poly_vertex_count] < ymin)ymin = (int)main_fill_polyY[main_fill_poly_vertex_count];
                    main_fill_poly_vertex_count++;
                }
                else {
                    int x1 = (xfptr == NULL ? (int)*xptr++ : (int)*xfptr++);
                    int x2 = (xfptr == NULL ? (int)*xptr : (int)*xfptr);
                    int y1 = (yfptr == NULL ? (int)*yptr++ : (int)*yfptr++);
                    int y2 = (yfptr == NULL ? (int)*yptr : (int)*yfptr);
                    DrawLine(x1, y1, x2, y2, 1, c);
                }
            }
            if (argc > 7) {
                main_fill_polyX[main_fill_poly_vertex_count] = (xfptr == NULL ? (TFLOAT)*xptr++ : (TFLOAT)*xfptr++);
                main_fill_polyY[main_fill_poly_vertex_count] = (yfptr == NULL ? (TFLOAT)*yptr++ : (TFLOAT)*yfptr++);
                if (main_fill_polyY[main_fill_poly_vertex_count] > ymax)ymax = (int)main_fill_polyY[main_fill_poly_vertex_count];
                if (main_fill_polyY[main_fill_poly_vertex_count] > ymax)ymax = (int)main_fill_polyY[main_fill_poly_vertex_count];
                if (main_fill_polyY[main_fill_poly_vertex_count] < ymin)ymin = (int)main_fill_polyY[main_fill_poly_vertex_count];
                if (main_fill_polyY[main_fill_poly_vertex_count] != main_fill_polyY[0] || main_fill_polyX[main_fill_poly_vertex_count] != main_fill_polyX[0]) {
                    main_fill_poly_vertex_count++;
                    main_fill_polyX[main_fill_poly_vertex_count] = main_fill_polyX[0];
                    main_fill_polyY[main_fill_poly_vertex_count] = main_fill_polyY[0];
                }
                main_fill_poly_vertex_count++;
                if (main_fill_poly_vertex_count > 5) {
                    fill_end_fill(xcount, ymin, ymax);
                }
                else if (main_fill_poly_vertex_count == 5) {
                    DrawTriangle((int)main_fill_polyX[0], (int)main_fill_polyY[0], (int)main_fill_polyX[1], (int)main_fill_polyY[1], (int)main_fill_polyX[2], (int)main_fill_polyY[2], f, f);
                    DrawTriangle((int)main_fill_polyX[0], (int)main_fill_polyY[0], (int)main_fill_polyX[2], (int)main_fill_polyY[2], (int)main_fill_polyX[3], (int)main_fill_polyY[3], f, f);
                    if (f != c) {
                        DrawLine((int)main_fill_polyX[0], (int)main_fill_polyY[0], (int)main_fill_polyX[1], (int)main_fill_polyY[1], 1, c);
                        DrawLine((int)main_fill_polyX[1], (int)main_fill_polyY[1], (int)main_fill_polyX[2], (int)main_fill_polyY[2], 1, c);
                        DrawLine((int)main_fill_polyX[2], (int)main_fill_polyY[2], (int)main_fill_polyX[3], (int)main_fill_polyY[3], 1, c);
                        DrawLine((int)main_fill_polyX[3], (int)main_fill_polyY[3], (int)main_fill_polyX[4], (int)main_fill_polyY[4], 1, c);
                    }
                }
                else {
                    DrawTriangle((int)main_fill_polyX[0], (int)main_fill_polyY[0], (int)main_fill_polyX[1], (int)main_fill_polyY[1], (int)main_fill_polyX[2], (int)main_fill_polyY[2], c, f);
                }
            }
            else {
                int x1 = (xfptr == NULL ? (int)*xptr : (int)*xfptr);
                int x2 = (xfptr == NULL ? (int)xptr2 : (int)xfptr2);
                int y1 = (yfptr == NULL ? (int)*yptr : (int)*yfptr);
                int y2 = (yfptr == NULL ? (int)yptr2 : (int)yfptr2);
                DrawLine(x1, y1, x2, y2, 1, c);
            }
        }
        else {
            int64_t* cc = (int64_t *)GetTempMemory(n * sizeof(int64_t)); //array for foreground colours
            int64_t* ff = (int64_t *)GetTempMemory(n * sizeof(int64_t)); //array for background colours
            int xstart, j, xmax = 0;
            for (i = 0; i < n; i++) {
                if ((polycountf == NULL ? (int)polycount[i] : (int)polycountf[i]) > xmax)xmax = (polycountf == NULL ? (int)polycount[i] : (int)polycountf[i]);
                if (!(polycountf == NULL ? (int)polycount[i] : (int)polycountf[i]))break;
                xtot += (polycountf == NULL ? (int)polycount[i] : (int)polycountf[i]);
                if ((polycountf == NULL ? (int)polycount[i] : (int)polycountf[i]) < 3 || (polycountf == NULL ? (int)polycount[i] : (int)polycountf[i]) > 9999)error((char *)"Invalid number of vertices, polygon %", i);
            }
            n = i;
            getargaddress(argv[2], &xptr, &xfptr, &nx);
            if (nx < xtot)error((char *)"X Dimensions %", nx);
            getargaddress(argv[4], &yptr, &yfptr, &ny);
            if (ny < xtot)error((char *)"Y Dimensions %", ny);
            main_fill_polyX = (TFLOAT*)GetTempMemory(xmax * sizeof(TFLOAT));
            main_fill_polyY = (TFLOAT*)GetTempMemory(xmax * sizeof(TFLOAT));
            if (argc > 5 && *argv[6]) { //foreground colour specified
                getargaddress(argv[6], &cptr, &cfptr, &nc);
                if (nc == 1) for (i = 0; i < n; i++)cc[i] = getint(argv[6], M_CLEAR, M_WHITE);
                else {
                    if (nc < n) error((char *)"Foreground colour Dimensions");
                    for (i = 0; i < n; i++) {
                        cc[i] = (cfptr == NULL ? cptr[i] : (int64_t)cfptr[i]);
                        if (cc[i] < 0 || cc[i] > M_WHITE) error((char *)"% is invalid (valid is % to %)", cc[i], M_CLEAR, M_WHITE);
                    }
                }
            }
            else for (i = 0; i < n; i++)cc[i] = gui_fcolour;
            if (argc > 7) { //background colour specified
                getargaddress(argv[8], &fptr, &ffptr, &nf);
                if (nf == 1) for (i = 0; i < n; i++) ff[i] = getint(argv[8], M_CLEAR, M_WHITE);
                else {
                    if (nf < n) error((char *)"Background colour Dimensions");
                    for (i = 0; i < n; i++) {
                        ff[i] = (ffptr == NULL ? fptr[i] : (int64_t)ffptr[i]);
                        if (ff[i] < 0 || ff[i] > M_WHITE) error((char *)"% is invalid (valid is % to %)", ff[i], M_CLEAR, M_WHITE);
                    }
                }
            }
            xstart = 0;
            for (i = 0; i < n; i++) {
                if (xptr)xptr2 = *xptr;
                else xfptr2 = *xfptr;
                if (yptr)yptr2 = *yptr;
                else yfptr2 = *yfptr;
                ymax = 0;
                ymin = 1000000;
                main_fill_poly_vertex_count = 0;
                xcount = (int)(polycountf == NULL ? polycount[i] : (int)polycountf[i]);
                if (argc > 7 && *argv[8]) {
                    fill_set_pen_color((cc[i] >> 16) & 0xFF, (cc[i] >> 8) & 0xFF, cc[i] & 0xFF, (cc[i] >> 24) & 0xFF);
                    fill_set_fill_color((ff[i] >> 16) & 0xFF, (ff[i] >> 8) & 0xFF, ff[i] & 0xFF, (ff[i] >> 24) & 0xFF);
                    fill_begin_fill();
                }
                for (j = xstart; j < xstart + xcount - 1; j++) {
                    if (argc > 7) {
                        main_fill_polyX[main_fill_poly_vertex_count] = (xfptr == NULL ? (TFLOAT)*xptr++ : (TFLOAT)*xfptr++);
                        main_fill_polyY[main_fill_poly_vertex_count] = (yfptr == NULL ? (TFLOAT)*yptr++ : (TFLOAT)*yfptr++);
                        if (main_fill_polyY[main_fill_poly_vertex_count] > ymax)ymax = (int)main_fill_polyY[main_fill_poly_vertex_count];
                        if (main_fill_polyY[main_fill_poly_vertex_count] < ymin)ymin = (int)main_fill_polyY[main_fill_poly_vertex_count];
                        main_fill_poly_vertex_count++;
                    }
                    else {
                        int x1 = (xfptr == NULL ? (int)*xptr++ : (int)*xfptr++);
                        int x2 = (xfptr == NULL ? (int)*xptr : (int)*xfptr);
                        int y1 = (yfptr == NULL ? (int)*yptr++ : (int)*yfptr++);
                        int y2 = (yfptr == NULL ? (int)*yptr : (int)*yfptr);
                        DrawLine(x1, y1, x2, y2, 1, cc[i]);
                    }
                }
                if (argc > 7) {
                    main_fill_polyX[main_fill_poly_vertex_count] = (xfptr == NULL ? (TFLOAT)*xptr++ : (TFLOAT)*xfptr++);
                    main_fill_polyY[main_fill_poly_vertex_count] = (yfptr == NULL ? (TFLOAT)*yptr++ : (TFLOAT)*yfptr++);
                    if (main_fill_polyY[main_fill_poly_vertex_count] > ymax)ymax = (int)main_fill_polyY[main_fill_poly_vertex_count];
                    if (main_fill_polyY[main_fill_poly_vertex_count] < ymin)ymin = (int)main_fill_polyY[main_fill_poly_vertex_count];
                    if (main_fill_polyY[main_fill_poly_vertex_count] != main_fill_polyY[0] || main_fill_polyX[main_fill_poly_vertex_count] != main_fill_polyX[0]) {
                        main_fill_poly_vertex_count++;
                        main_fill_polyX[main_fill_poly_vertex_count] = main_fill_polyX[0];
                        main_fill_polyY[main_fill_poly_vertex_count] = main_fill_polyY[0];
                    }
                    main_fill_poly_vertex_count++;
                    if (main_fill_poly_vertex_count > 5) {
                        fill_end_fill(xcount, ymin, ymax);
                    }
                    else if (main_fill_poly_vertex_count == 5) {
                        DrawTriangle((int)main_fill_polyX[0], (int)main_fill_polyY[0], (int)main_fill_polyX[1], (int)main_fill_polyY[1], (int)main_fill_polyX[2], (int)main_fill_polyY[2], ff[i], ff[i]);
                        DrawTriangle((int)main_fill_polyX[0], (int)main_fill_polyY[0], (int)main_fill_polyX[2], (int)main_fill_polyY[2], (int)main_fill_polyX[3], (int)main_fill_polyY[3], ff[i], ff[i]);
                        if (ff[i] != cc[i]) {
                            DrawLine((int)main_fill_polyX[0], (int)main_fill_polyY[0], (int)main_fill_polyX[1], (int)main_fill_polyY[1], 1, cc[i]);
                            DrawLine((int)main_fill_polyX[1], (int)main_fill_polyY[1], (int)main_fill_polyX[2], (int)main_fill_polyY[2], 1, cc[i]);
                            DrawLine((int)main_fill_polyX[2], (int)main_fill_polyY[2], (int)main_fill_polyX[3], (int)main_fill_polyY[3], 1, cc[i]);
                            DrawLine((int)main_fill_polyX[3], (int)main_fill_polyY[3], (int)main_fill_polyX[4], (int)main_fill_polyY[4], 1, cc[i]);
                        }
                    }
                    else {
                        DrawTriangle((int)main_fill_polyX[0], (int)main_fill_polyY[0], (int)main_fill_polyX[1], (int)main_fill_polyY[1], (int)main_fill_polyX[2], (int)main_fill_polyY[2], cc[i], ff[i]);
                    }
                }
                else {
                    int x1 = (xfptr == NULL ? (int)*xptr : (int)*xfptr);
                    int x2 = (xfptr == NULL ? (int)xptr2 : (int)xfptr2);
                    int y1 = (yfptr == NULL ? (int)*yptr : (int)*yfptr);
                    int y2 = (yfptr == NULL ? (int)yptr2 : (int)yfptr2);
                    DrawLine(x1, y1, x2, y2, 1, cc[i]);
                    if (xfptr != NULL)xfptr++;
                    else xptr++;
                    if (yfptr != NULL)yfptr++;
                    else yptr++;
                }

                xstart += xcount;
            }
        }
    }

    // get and decode the justify$ string used in TEXT and GUI CAPTION
// the values are returned via pointers
    extern "C" int GetJustification(char* p, int* jh, int* jv, int* jo) {
        switch (toupper(*p++)) {
        case 'L':   *jh = JUSTIFY_LEFT; break;
        case 'C':   *jh = JUSTIFY_CENTER; break;
        case 'R':   *jh = JUSTIFY_RIGHT; break;
        case  0:   return true;
        default:    p--;
        }
        skipspace(p);
        switch (toupper(*p++)) {
        case 'T':   *jv = JUSTIFY_TOP; break;
        case 'M':   *jv = JUSTIFY_MIDDLE; break;
        case 'B':   *jv = JUSTIFY_BOTTOM; break;
        case  0:   return true;
        default:    p--;
        }
        skipspace(p);
        switch (toupper(*p++)) {
        case 'N':   *jo = ORIENT_NORMAL; break;                     // normal
        case 'V':   *jo = ORIENT_VERT; break;                       // vertical text (top to bottom)
        case 'I':   *jo = ORIENT_INVERTED; break;                   // inverted
        case 'U':   *jo = ORIENT_CCW90DEG; break;                   // rotated CCW 90 degrees
        case 'D':   *jo = ORIENT_CW90DEG; break;                    // rotated CW 90 degrees
        case  0:   return true;
        default:    return false;
        }
        return *p == 0;
    }

    void cmd_text(void) {
        int x, y, font, scale;
        int64_t fc, bc;
        unsigned char* s;
        int jh = 0, jv = 0, jo = 0;

        getargs(&cmdline, 17, (unsigned char*)",");                                     // this is a macro and must be the first executable stmt
        if (!(argc & 1) || argc < 5) error((char*)"Argument count");
        x = (int)getinteger(argv[0]);
        y = (int)getinteger(argv[2]);
        s = getCstring(argv[4]);

        if (argc > 5 && *argv[6])
            if (!GetJustification((char *)argv[6], &jh, &jv, &jo))
                if (!GetJustification((char*)getCstring(argv[6]), &jh, &jv, &jo))
                    error((char*)"Justification");;

        font = (gui_font >> 4) + 1; scale = (gui_font & 0b1111); fc = gui_fcolour; bc = gui_bcolour;        // the defaults
        if (argc > 7 && *argv[8]) {
            if (*argv[8] == '#') argv[8]++;
            font = (int)getint(argv[8], 1, (long long int)FONT_TABLE_SIZE);
        }
        if (FontTable[font - 1] == NULL) error((char*)"Invalid font #%", font);
        if (argc > 9 && *argv[10]) scale = (int)getint(argv[10], 1, 15);
        if (argc > 11 && *argv[12]) fc = getint(argv[12], M_CLEAR, M_WHITE);
        if (argc == 15) bc = getint(argv[14], -1, M_WHITE);
        GUIPrintString(x, y, ((font - 1) << 4) | scale, jh, jv, jo, fc, bc, s);
    }

    // this function positions the cursor within a PRINT command
    void fun_at(void) {
        getargs(&ep, 5, (unsigned char *)",");
        if (commandfunction(cmdtoken) != cmd_print) error((char*)"Invalid function");
        //	if((argc == 3 || argc == 5)) error((char *)"Incorrect number of arguments");
        //	AutoLineWrap = false;
        lastx = CurrentX = (int)getinteger(argv[0]);
        if (argc >= 3 && *argv[2])lasty = CurrentY = (int)getinteger(argv[2]);
        if (argc == 5) {
            PrintPixelMode = (int)getinteger(argv[4]);
            if (PrintPixelMode < 0 || PrintPixelMode > 7) {
                PrintPixelMode = 0;
                error((char*)"Number out of bounds");
            }
        }
        else
            PrintPixelMode = 0;

        // BJR: VT100 set cursor location: <esc>[y;xf
        //      where x and y are ASCII string integers.
        //      Assumes overall font size of 6x12 pixels (480/80 x 432/36), including gaps between characters and lines

        targ = T_STR;
        sret = (unsigned char*)"\0";                                                    // normally pointing sret to a string in flash is illegal
    }
    extern "C" void ScrollBufferV(int lines, int blank) {
        uint32_t* s, * d;
        uint32_t wpa = (uint32_t)PageTable[WritePage].address;
        int y, yy, cursorhidden = 0;
        int maxH = PageTable[WritePage].ymax;
        int maxW = PageTable[WritePage].xmax;
        int Scale = 1;
        int n = (maxW * Scale * 4);
        if (lines > 0) {
            for (y = 0; y < maxH - lines; y++) {
                yy = y + lines;
                d = (uint32_t*)((y * maxW * Scale) * 4 + wpa);
                s = (uint32_t*)((yy * maxW * Scale) * 4 + wpa);
                memcpy(d, s, n);
            }
            if (blank) {
                DrawRectangle(0, maxH - lines, maxW - 1, maxH - 1, gui_bcolour); // erase the line to be scrolled off
            }
        }
        else if (lines < 0) {
            lines = -lines;
            for (y = maxH - 1; y >= lines; y--) {
                yy = y - lines;
                d = (uint32_t*)((y * maxW * Scale) * 4 + wpa);
                s = (uint32_t*)((yy * maxW * Scale) * 4 + wpa);
                memcpy(d, s, n);
            }
            if (blank)DrawRectangle(0, 0, maxW - 1, lines - 1, gui_bcolour); // erase the line to be scrolled off
        }
    }

    extern "C" void ScrollBufferH(int pixels) {
        if (!pixels)return;
        uint32_t* s, * d;
        uint32_t wpa = (uint32_t)PageTable[WritePage].address;
        int maxW = PageTable[WritePage].xmax;
        int maxH = PageTable[WritePage].ymax;
        int Scale = 1;
        int y, lmaxH = maxH * Scale, cursorhidden = 0;
        if (pixels > 0) {
            for (y = 0; y < lmaxH; y++) {
                d = (uint32_t*)((y * maxW * 4) + wpa);
                s = (uint32_t*)((y * maxW * 4) + wpa);
                d += pixels;
                memcpy(linebuff, s, (maxW - pixels) << 2);
                memcpy(d, linebuff, (maxW - pixels) << 2);
            }
        }
        else if (pixels < 0) {
            pixels = -pixels;
            for (y = 0; y < lmaxH; y++) {
                d = (uint32_t*)((y * maxW * 4) + wpa);
                s = (uint32_t*)((y * maxW * 4) + wpa);
                s += pixels;
                memcpy(linebuff, s, (maxW - pixels) << 2);
                memcpy(d, linebuff, (maxW - pixels) << 2);
            }
        }
    }
    void Stitch(uint32_t fadd1, uint32_t fadd2, uint32_t tadd, int offset) {
        uint32_t wpa = (uint32_t)PageTable[tadd].address;
        uint32_t rpa1 = (uint32_t)PageTable[fadd1].address;
        uint32_t rpa2 = (uint32_t)PageTable[fadd2].address;
        int maxW = PageTable[WritePage].xmax;
        int maxH = PageTable[WritePage].ymax;
        int y;
        uint32_t* s, * d;
        for (y = 0; y < maxH; y++) {
            s = (uint32_t*)(y * maxW * 4 + (offset * 4) + rpa1);
            d = (uint32_t*)((y * maxW * 4) + wpa);
            memcpy(d, s, (maxW - offset) << 2);
            s = (uint32_t*)((y * maxW * 4) + rpa2);
            d = (uint32_t*)(y * maxW * 4 + (maxW * 4) - (offset * 4) + wpa);
            memcpy(d, s, offset << 2);
        }
     }
    void PageAnd(uint32_t fadd1, uint32_t fadd2, uint32_t taddt) {
        uint32_t wpa = (uint32_t)PageTable[taddt].address;
        uint32_t rpa1 = (uint32_t)PageTable[fadd1].address;
        uint32_t rpa2 = (uint32_t)PageTable[fadd2].address;
        int maxW = PageTable[WritePage].xmax;
        int h = PageTable[taddt].ymax;
        int x, y;
        uint64_t* s1, * s2, * d;
        for (y = 0; y < h; y++) {
            for (y = 0; y < h; y++) {
                s1 = (uint64_t*)((y * maxW * 4) + rpa1);
                s2 = (uint64_t*)((y * maxW * 4) + rpa2);
                d = (uint64_t*)((y * maxW * 4) + wpa);
                for (x = 0; x < (maxW >> 1); x++)*d++ = ((*s1++) & (*s2++));
            }
        }
    }

    void PageOr(uint32_t fadd1, uint32_t fadd2, uint32_t taddt) {
        uint32_t wpa = (uint32_t)PageTable[taddt].address;
        uint32_t rpa1 = (uint32_t)PageTable[fadd1].address;
        uint32_t rpa2 = (uint32_t)PageTable[fadd2].address;
        int maxW = PageTable[WritePage].xmax;
        int h = PageTable[taddt].ymax;
        int x, y;
        uint64_t* s1, * s2, * d;
        for (y = 0; y < h; y++) {
            for (y = 0; y < h; y++) {
                s1 = (uint64_t*)((y * maxW * 4) + rpa1);
                s2 = (uint64_t*)((y * maxW * 4) + rpa2);
                d = (uint64_t*)((y * maxW * 4) + wpa);
                for (x = 0; x < (maxW >> 1); x++)*d++ = ((*s1++) | (*s2++));
            }
        }
    }

    void PageXor(uint32_t fadd1, uint32_t fadd2, uint32_t taddt) {
        uint32_t wpa = (uint32_t)PageTable[taddt].address;
        uint32_t rpa1 = (uint32_t)PageTable[fadd1].address;
        uint32_t rpa2 = (uint32_t)PageTable[fadd2].address;
        int maxW = PageTable[WritePage].xmax;
        int h = PageTable[taddt].ymax;
        int x, y;
        uint64_t* s1, * s2, * d;
        for (y = 0; y < h; y++) {
            for (y = 0; y < h; y++) {
                s1 = (uint64_t*)((y * maxW * 4) + rpa1);
                s2 = (uint64_t*)((y * maxW * 4) + rpa2);
                d = (uint64_t*)((y * maxW * 4) + wpa);
                for (x = 0; x < (maxW >> 1); x++)*d++ = ((*s1++) ^ (*s2++));
            }
        }
    }
    void swapcopy32(uint32_t* d, uint32_t* s, int n) { //copy skipping black
        int i;
        for (i = 0; i < (n / 2); i++) {
            d[i] = s[n - 1 - i];
            d[n - 1 - i] = s[i];
        }
    }
    void zerocopy32(uint32_t* d, uint32_t* s, int n, int flip) { //copy skipping black
        int i;
        if (flip) {
            uint32_t swap;
            uint8_t* buff = (uint8_t *)linebuff;
            uint32_t* lbuff = (uint32_t*)buff;
            memcpy(buff, (uint8_t*)s, n << 2);
            for (i = 0; i < (n / 2); i++) {
                swap = lbuff[n - 1 - i];
                lbuff[n - 1 - i] = lbuff[i];
                lbuff[i] = swap;
            }
            lbuff = (uint32_t*)buff;
            for (i = 0; i < n; i++) {
                if (*lbuff & (ARGBenabled ? 0xFFFFFFFF : 0xFFFFFF))*d++ = *lbuff++;
                else {
                    lbuff++;
                    d++;
                }
            }
        }
        else {
            for (i = 0; i < n; i++) {
                if (*s & (ARGBenabled ? 0xFFFFFFFF : 0xFFFFFF))*d++ = *s++;
                else {
                    s++;
                    d++;
                }
            }
        }

    }
    extern "C" void MoveBufferNormal(int x1, int y1, int x2, int y2, int w, int h, int flip) {
        uint32_t wpa = (uint32_t)PageTable[WritePage].address;
        uint32_t rpa = (uint32_t)PageTable[ReadPage].address;
        uint32_t hrw = (uint32_t)PageTable[WritePage].xmax;
        uint32_t hrr = (uint32_t)PageTable[ReadPage].xmax;
       if (optiony)y1 = PageTable[ReadPage].ymax - y1 - h;
        if (optiony)y2 = PageTable[WritePage].ymax - y2 - h;
        int yp;
        //Check if we need to process overlapping areas
        if (ReadPage == WritePage && !(x1 + w <= x2 || x1 >= x2 + w || y1 + h <= y2 || y1 >= y2 + h)) {
            uint32_t* buff = (uint32_t *)GetMemory(w * h * 4);
            ReadBuffer(x1, y1, x1 + w - 1, y1 + h - 1, buff);
            if (flip == 0) {
                DrawBuffer(x2, y2, x2 + w - 1, y2 + h - 1, buff);
            }  else if (flip == 1) {
                uint8_t* s = (uint8_t*)buff;//pointer to the start of the source data
                uint8_t* d = (uint8_t*)((y2 * hrw + x2) * 4 + wpa); //pointer to the start of the destination data
                for (yp = 0; yp < h; yp++) {
                    swapcopy32((uint32_t*)d, (uint32_t*)s, w);
                    s += (w << 2);
                    d += (hrw << 2);
                }
            }  else if (flip == 2) {
                uint8_t* s = (uint8_t*)buff;//pointer to the start of the source data
                uint8_t* d = (uint8_t*)(((y2 + h - 1) * hrw + x2) * 4 + wpa); //pointer to the start of the destination data
                for (yp = 0; yp < h; yp++) {
                    memcpy(d, s, (w << 2));
                    s += (w << 2);
                    d -= (hrw << 2);
                }
            }
            else if (flip == 3) {
                uint8_t* s = (uint8_t*)buff;//pointer to the start of the source data
                uint8_t* d = (uint8_t*)(((y2 + h - 1) * hrw + x2) * 4 + wpa); //pointer to the start of the destination data
                for (yp = 0; yp < h; yp++) {
                    swapcopy32((uint32_t*)d, (uint32_t*)s, w);
                    memcpy(d, s, w << 2);
                    s += (w << 2);
                    d -= (hrw << 2);
                }
            }
            else if (flip == 4 || flip == 5) {
                uint8_t* s = (uint8_t*)buff;//pointer to the start of the source data
                uint8_t* d = (uint8_t*)((y2 * hrw + x2) * 4 + wpa); //pointer to the start of the destination data
                for (yp = 0; yp < h; yp++) {
                    zerocopy32((uint32_t*)d, (uint32_t*)s, w, flip & 1);
                    s += (w << 2);
                    d += (hrw << 2);
                }
            }
            else if (flip == 6 || flip == 7) {
                uint8_t* s = (uint8_t*)buff;//pointer to the start of the source data
                uint8_t* d = (uint8_t*)(((y2 + h - 1) * hrw + x2) * 4 + wpa); //pointer to the start of the destination data
                for (yp = 0; yp < h; yp++) {
                    zerocopy32((uint32_t*)d, (uint32_t*)s, w, flip & 1);
                    s += (w << 2);
                    d -= (hrw << 2);
                }
            }
            FreeMemorySafe((void**)&buff);
            return;
        }
        else {
            if (flip == 0) {
                uint8_t* s = (uint8_t*)((y1 * hrr) * 4 + x1 * 4 + rpa);//pointer to the start of the source data
                uint8_t* d = (uint8_t*)((y2 * hrw + x2) * 4 + wpa); //pointer to the start of the destination data
                for (yp = 0; yp < h; yp++) {
                    memcpy(d, s, (w << 2));
                    s += (hrr << 2);
                    d += (hrw << 2);
                }
            }
            else if (flip == 1) {
                uint8_t* s = (uint8_t*)((y1 * hrr) * 4 + x1 * 4 + rpa);//pointer to the start of the source data
                uint8_t* d = (uint8_t*)((y2 * hrw + x2) * 4 + wpa); //pointer to the start of the destination data
                for (yp = 0; yp < h; yp++) {
                    swapcopy32((uint32_t*)d, (uint32_t*)s, w);
                    s += (hrr << 2);
                    d += (hrw << 2);
                }
            }
            else if (flip == 2) {
                uint8_t* s = (uint8_t*)((y1 * hrr) * 4 + x1 * 4 + rpa);//pointer to the start of the source data
                uint8_t* d = (uint8_t*)(((y2 + h - 1) * hrw + x2) * 4 + wpa); //pointer to the start of the destination data
                for (yp = 0; yp < h; yp++) {
                    memcpy(d, s, (w << 2));
                    s += (hrr << 2);
                    d -= (hrw << 2);
                }
            }
            else if (flip == 3) {
                uint8_t* s = (uint8_t*)((y1 * hrr) * 4 + x1 * 4 + rpa);//pointer to the start of the source data
                uint8_t* d = (uint8_t*)(((y2 + h - 1) * hrw + x2) * 4 + wpa); //pointer to the start of the destination data
                for (yp = 0; yp < h; yp++) {
                    swapcopy32((uint32_t*)d, (uint32_t*)s, w);
                    s += (hrr << 2);
                    d -= (hrw << 2);
                }
            }
            else if (flip == 4 || flip == 5) {
                uint8_t* s = (uint8_t*)((y1 * hrr) * 4 + x1 * 4 + rpa);//pointer to the start of the source data
                uint8_t* d = (uint8_t*)((y2 * hrw + x2) * 4 + wpa); //pointer to the start of the destination data
                for (yp = 0; yp < h; yp++) {
                    zerocopy32((uint32_t*)d, (uint32_t*)s, w, flip & 1);
                    s += (hrr << 2);
                    d += (hrw << 2);
                }
            }
            else if (flip == 6 || flip == 7) {
                uint8_t* s = (uint8_t*)((y1 * hrr) * 4 + x1 * 4 + rpa);//pointer to the start of the source data
                uint8_t* d = (uint8_t*)(((y2 + h - 1) * hrw + x2) * 4 + wpa); //pointer to the start of the destination data
                for (yp = 0; yp < h; yp++) {
                    zerocopy32((uint32_t*)d, (uint32_t*)s, w, flip & 1);
                    s += (hrr << 2);
                    d -= (hrw << 2);
                }
            }
        }
    }
    void PageCopy(uint32_t fadd, uint32_t tadd, int transparent) {
        uint8_t writesave = WritePage, readsave = ReadPage;
        ;
        WritePage = tadd;
        ReadPage = fadd;
        if(!transparent) {
            int n = (PageTable[fadd].xmax * PageTable[fadd].ymax * sizeof(uint32_t));
            uint32_t* d = (uint32_t*)getpageaddress(tadd);
            uint32_t* s = (uint32_t*)getpageaddress(fadd);
            memcpy(d, s, n);
        } else {
            MoveBufferNormal(0, 0, 0, 0, PageTable[fadd].xmax, PageTable[fadd].ymax, 4);
            WritePage = writesave;
            ReadPage = readsave;
        }
    }

    void cmd_page(void) {
        unsigned char* p;
        int maxH = PageTable[WritePage].ymax;
        int maxW = PageTable[WritePage].xmax;
        p = checkstring(cmdline, (unsigned char *)"WRITE");
        if (p) {
            if (checkstring(p, (unsigned char*)"FRAMEBUFFER")) {
                if (PageTable[WPN].address == NULL)error((char *)"Framebuffer not created");
                WritePage = ReadPage = WPN;
            }
            else {
                int32_t fadd = (int32_t)getint(p, 0, MAXPAGES);
                WritePage = ReadPage = fadd;
            }
            HRes = PageTable[WritePage].xmax;
            VRes = PageTable[WritePage].ymax;
            return;
        }
        p = checkstring(cmdline, (unsigned char*)"COPY");
        if (p) {
            int transparent = 0;
            unsigned char ss[3];														// this will be used to split up the argument line
            ss[0] = tokenTO;
            ss[1] = ',';
            ss[2] = 0;
            getargs(&p, 7, ss);
            if (argc < 3)error((char*)"Argument count");
            uint32_t savepage = WritePage;
            int32_t fadd = (int)getint(argv[0], 0, MAXPAGES);
            int32_t tadd = (int)getint(argv[2], 0, MAXPAGES);
            if (!(PageTable[fadd].xmax == PageTable[tadd].xmax && PageTable[fadd].ymax == PageTable[tadd].ymax))error((char *)"Page size mismatch - use BLIT");
            if (argc >= 5 && *argv[4]) {
            }
            if (argc == 7) {
                unsigned char* p = argv[6];
                if (toupper(*p) == 'T' || *p == '1')transparent = 1;
            }
            PageCopy(fadd, tadd, transparent);
            ReadPage = WritePage = savepage;
            return;
        }
        p = checkstring(cmdline, (unsigned char*)"SCROLL");
        if (p) {
            int x, y, blank = -2, m, n;
            uint32_t* current = NULL;
            uint32_t savepage = WritePage;
            getargs(&p, 7, (unsigned char*)",");
            int multiplier = 4;
            if (argc < 5)error((char*)"Syntax");
            x = (int)getint(argv[2], -maxW / 2 - 1, maxW / 2);
            y = (int)getint(argv[4], -maxH / 2 - 1, maxH / 2);
            if (x == 0 && y == 0)return;
            m = ((maxW * (y > 0 ? y : -y)) * multiplier);
            n = ((maxH * (x > 0 ? x : -x)) * multiplier);
            if (n > m)m = n;
            if (argc == 7)blank = (int)getColour(argv[6], 1);
            if (blank == -2)current = (uint32_t*)GetTempMemory(m);
            ReadPage = WritePage = (int)getint(argv[0], 0, MAXPAGES);
            if (x > 0) {
                if (blank == -2)ReadBuffer(maxW - x, 0, maxW - 1, maxH - 1, current);
                ScrollBufferH(x);
                if (blank == -2)DrawBuffer(0, 0, x - 1, maxH - 1, current);
                else if (blank != -1)DrawRectangle(0, 0, x - 1, maxH - 1, blank);
            }
            else if (x < 0) {
                x = -x;
                if (blank == -2)ReadBuffer(0, 0, x - 1, maxH - 1, current);
                ScrollBufferH(-x);
                if (blank == -2)DrawBuffer(maxW - x, 0, maxW - 1, maxH - 1, current);
                else if (blank != -1)DrawRectangle(maxW - x, 0, maxW - 1, maxH - 1, blank);
            }
            if (y > 0) {
                if (blank == -2)ReadBuffer(0, 0, maxW - 1, y - 1, current);
                ScrollBufferV(y, 0);
                if (blank == -2)DrawBuffer(0, maxH - y, maxW - 1, maxH - 1, current);
                else if (blank != -1)DrawRectangle(0, maxH - y, maxW - 1, maxH - 1, blank);
            }
            else if (y < 0) {
                y = -y;
                if (blank == -2)ReadBuffer(0, maxH - y, maxW - 1, maxH - 1, current);
                ScrollBufferV(-y, 0);
                if (blank == -2)DrawBuffer(0, 0, maxW - 1, y - 1, current);
                else if (blank != -1)DrawRectangle(0, 0, maxW - 1, y - 1, blank);
            }
            ReadPage = WritePage = savepage;
            return;
        }
        p = checkstring(cmdline, (unsigned char*)"STITCH");
        if (p) {
            getargs(&p, 7, (unsigned char*)",");
            if (argc != 7)error((char *)!"Argument count");
            uint32_t fadd1 = (int)getint(argv[0], (ARGBenabled ? 2 : 1), MAXPAGES);
            uint32_t fadd2 = (int)getint(argv[2], (ARGBenabled ? 2 : 1), MAXPAGES);
            uint32_t tadd = (int)getint(argv[4], 0, MAXPAGES);
            int newfadd1 = fadd1, newfadd2 = fadd2, newtadd = tadd;
            if (!(PageTable[fadd1].xmax == PageTable[tadd].xmax && PageTable[fadd2].xmax == PageTable[tadd].xmax &&
                PageTable[fadd1].ymax == PageTable[tadd].ymax && PageTable[fadd2].ymax == PageTable[tadd].ymax))error((char *)"Page size mismatch");
            int offset = (int)getint(argv[6], 0, maxW);
            Stitch(newfadd1, newfadd2, newtadd, offset);
            return;
        }
        p = checkstring(cmdline, (unsigned char*)"AND_PIXELS");
        if (p) {
            getargs(&p, 5, (unsigned char*)",");
            if (argc != 5)error((char*)"Argument count");
            uint32_t fadd1 = (uint32_t)getint(argv[0], 0, MAXPAGES);
            uint32_t fadd2 = (uint32_t)getint(argv[2], 0, MAXPAGES);
            uint32_t tadd = (uint32_t)getint(argv[4], 0, MAXPAGES);
            int newfadd1 = fadd1, newfadd2 = fadd2, newtadd = tadd;
            if (!(PageTable[fadd1].xmax == PageTable[tadd].xmax && PageTable[fadd2].xmax == PageTable[tadd].xmax &&
                PageTable[fadd1].ymax == PageTable[tadd].ymax && PageTable[fadd2].ymax == PageTable[tadd].ymax))error((char*)"Page size mismatch");
            PageAnd(newfadd1, newfadd2, newtadd);
            return;
        }
        p = checkstring(cmdline, (unsigned char*)"OR_PIXELS");
        if (p) {
            getargs(&p, 5, (unsigned char*)",");
            if (argc != 5)error((char*)"Argument count");
            uint32_t fadd1 = (uint32_t)getint(argv[0], 0, MAXPAGES);
            uint32_t fadd2 = (uint32_t)getint(argv[2], 0, MAXPAGES);
            uint32_t tadd = (uint32_t)getint(argv[4], 0, MAXPAGES);
            int newfadd1 = fadd1, newfadd2 = fadd2, newtadd = tadd;
            if (!(PageTable[fadd1].xmax == PageTable[tadd].xmax && PageTable[fadd2].xmax == PageTable[tadd].xmax &&
                PageTable[fadd1].ymax == PageTable[tadd].ymax && PageTable[fadd2].ymax == PageTable[tadd].ymax))error((char*)"Page size mismatch");
            PageOr(newfadd1, newfadd2, newtadd);
            return;
        }
        p = checkstring(cmdline, (unsigned char*)"XOR_PIXELS");
        if (p) {
            getargs(&p, 5, (unsigned char*)",");
            if (argc != 5)error((char*)"Argument count");
            uint32_t fadd1 = (uint32_t)getint(argv[0], 0, MAXPAGES);
            uint32_t fadd2 = (uint32_t)getint(argv[2], 0, MAXPAGES);
            uint32_t tadd = (uint32_t)getint(argv[4], 0, MAXPAGES);
            int newfadd1 = fadd1, newfadd2 = fadd2, newtadd = tadd;
            if (!(PageTable[fadd1].xmax == PageTable[tadd].xmax && PageTable[fadd2].xmax == PageTable[tadd].xmax &&
                PageTable[fadd1].ymax == PageTable[tadd].ymax && PageTable[fadd2].ymax == PageTable[tadd].ymax))error((char*)"Page size mismatch");
            PageXor(newfadd1, newfadd2, newtadd);
            return;
        }
        p = checkstring(cmdline, (unsigned char*)"RESIZE");
        if (p) {
            getargs(&p, 5, (unsigned char*)",");
            int maxHZ = PageTable[0].ymax;
            int maxWZ = PageTable[0].xmax;
            uint32_t page = (uint32_t)getint(argv[0], (ARGBenabled ? 2 : 1), MAXPAGES);
            int x = (int)getint(argv[2], 1, maxWZ);
            int y = (int)getint(argv[4], 1, maxHZ);
            PageTable[page].xmax = x;
            PageTable[page].ymax = y;
            return;
        }
        error((char*)"Syntax");
    }
    void WindowFrame(int x, int y) {
        //PageTable[ReadPage].address points to the start of the framebuffer
        //PageTable[WritePage].address points to the start of the page to be written
        //maxW is the width of the target page
        //maxH is the height of the target page
        //PageTable[WPN].xmax is the width of the framebuffer
        //PageTable[WPN].ymax is the height of the framebuffer
        //ModeScale is undefined for this operation so define it locally
        uint32_t wpa = (uint32_t)PageTable[WritePage].address;
        uint32_t rpa = (uint32_t)PageTable[WPN].address;
        uint32_t hrw = (uint32_t)PageTable[WritePage].xmax;
        uint32_t hrr = (uint32_t)PageTable[WPN].xmax;
        int maxW = PageTable[WritePage].xmax;
        int maxH = PageTable[WritePage].ymax;
        if (optiony)y = PageTable[WPN].ymax - 1 - y;
        int yp;
        uint8_t* s = (uint8_t*)((y * hrr + x) * 4 + rpa); //pointer to the start of the source data
        uint8_t* d = (uint8_t*)wpa;//pointer to the start of the destination data
        for (yp = 0; yp < maxH; yp++) {
            memcpy(d, s, (hrw << 2));
            d += (hrw << 2);
            s += (hrr << 2);
        }
     }

    void cmd_framebuffer(void) {
        unsigned char* p;
        p = checkstring(cmdline, (unsigned char*)"WINDOW");
        if (p) {
            int tempwrite;
            getargs(&p, 7, (unsigned char*)",");
            uint8_t oldwrite = WritePage;
            if (PageTable[WPN].address == NULL)error((char *)"World not created");
            int32_t fadd = (int)getint(argv[4], 0, MAXPAGES);
            tempwrite = fadd;
            int x = (int)getint(argv[0], 0, PageTable[WPN].xmax - PageTable[tempwrite].xmax);
            int y = (int)getint(argv[2], 0, PageTable[WPN].ymax - PageTable[tempwrite].ymax);
            if (argc == 7) {
            }
            WritePage = fadd;
            ReadPage = WPN;
            WindowFrame(x, y);
            WritePage = ReadPage = oldwrite;
            return;
        }
        p = checkstring(cmdline, (unsigned char*)"WRITE");
        if (p) {
            if (PageTable[WPN].address == NULL)error((char *)"Framebuffer not created");
            WritePage = ReadPage = WPN;
            HRes = PageTable[WritePage].xmax;
            VRes = PageTable[WritePage].ymax;
//            WritePageAddress = ReadPageAddress = (uint32_t *)PageTable[WritePage].address;
            return;
        }
        p = checkstring(cmdline, (unsigned char*)"CLOSE");
        if (p) {
            if (PageTable[WPN].address == NULL)error((char *)"Framebuffer not created");
            if (WritePage == WPN)error((char *)"Framebuffer is set for write");
            FreeMemorySafe((void**)&PageTable[BPN].address);
            FreeMemorySafe((void**)&PageTable[WPN].address);
            PageTable[BPN].address = NULL;
           PageTable[WPN].address = NULL;
            return;
        }
        p = checkstring(cmdline, (unsigned char*)"BACKUP");
        if (p) {
            if (PageTable[WPN].address == NULL)error((char *)"Framebuffer not created");
            if (PageTable[BPN].address == NULL) {
                if ((int)PageTable[WPN].xmax * (int)PageTable[WPN].ymax * (int)sizeof(uint32_t) > (int)FreeSpaceOnHeap() - (int)32768)error((char *)"Not enough memory for framebuffer backup");
                PageTable[BPN].address = (uint32_t *)GetMemory(PageTable[WPN].xmax * PageTable[WPN].ymax * sizeof(uint32_t));
                PageTable[BPN].address = PageTable[BPN].address;
                PageTable[BPN].xmax = PageTable[WPN].xmax;
                PageTable[BPN].ymax = PageTable[WPN].ymax;
                PageTable[BPN].size = PageTable[WPN].size;
            }
            int n = (PageTable[WPN].xmax * PageTable[WPN].ymax * sizeof(uint32_t)) >> 2;
            memcpy((uint32_t*)PageTable[WPN].address, (uint32_t*)PageTable[BPN].address, n);
            return;
        }
        p = checkstring(cmdline, (unsigned char*)"RESTORE");
        if (p) {
            getargs(&p, 7, (unsigned char*)",");
            if (PageTable[WPN].address == NULL)error((char *)"Framebuffer not created");
            if (PageTable[BPN].address == NULL)error((char *)"Framebuffer backup not created");
            if (argc) {
                uint8_t oldwrite = WritePage;
                int w = (int)getint(argv[4], 1, PageTable[WPN].xmax);
                int h = (int)getint(argv[6], 1, PageTable[WPN].ymax);
                int x = (int)getint(argv[0], 0, PageTable[WPN].xmax - w);
                int y = (int)getint(argv[2], 0, PageTable[WPN].ymax - h);
                ReadPage = BPN;
                WritePage = WPN;
                MoveBufferNormal(x, y, x, y, w, h, 0);
                ReadPage = WritePage = oldwrite;
            }
            else {
                uint32_t n = (PageTable[WPN].xmax * PageTable[WPN].ymax * sizeof(uint32_t)) >> 2;
                memcpy((void *)PageTable[BPN].address, (void*)PageTable[WPN].address, n);
            }

            return;
        }
        p = checkstring(cmdline, (unsigned char*)"CREATE");
        if (p) {
            getargs(&p, 3, (unsigned char*)",");
            PageTable[WPN].xmax = (int16_t)getint(argv[0], PageTable[WritePage].xmax, 3840*2);
            PageTable[WPN].ymax = (int16_t)getint(argv[2], PageTable[WritePage].ymax, 2160*2);
            FreeMemorySafe((void**)&PageTable[WPN].address);
            if ((int)PageTable[WPN].xmax * (int)PageTable[WPN].ymax * (int)sizeof(uint32_t) > (int)FreeSpaceOnHeap() - (int)32768)error((char *)"Not enough memory for framebuffer");
            PageTable[WPN].address = (uint32_t*)GetMemory((int)PageTable[WPN].xmax * PageTable[WPN].ymax * sizeof(uint32_t));
            PageTable[WPN].size = PageTable[WPN].xmax * PageTable[WPN].ymax * sizeof(uint32_t);
            return;
        }
        error((char *)"Syntax");
    }
    void copytofloat(uint8_t*** output, int xs, int ys, int w, int h) {
        union colourmap
        {
            char rgbbytes[4];
            unsigned int rgb;
        } c;
        int x, y;
        for (x = xs; x < xs + w; x++) {
            for (y = ys; y < ys + h; y++) {
                ReadBuffer(x, y, x, y, &c.rgb);
                output[0][y - ys][x - xs] = c.rgbbytes[0];
                output[1][y - ys][x - xs] = c.rgbbytes[1];
                output[2][y - ys][x - xs] = c.rgbbytes[2];
            }
        }
    }
    void copyfromfloat(uint8_t*** input, int xs, int ys, int w, int h, int dontcopyblack) {
        union colourmap
        {
            char rgbbytes[4];
            unsigned int rgb;
        } c;
        int x, y;
        for (x = xs; x < xs + w; x++) {
            for (y = ys; y < ys + h; y++) {
                c.rgbbytes[0] = (uint8_t)input[0][y - ys][x - xs];
                c.rgbbytes[1] = (uint8_t)input[1][y - ys][x - xs];
                c.rgbbytes[2] = (uint8_t)input[2][y - ys][x - xs];
                c.rgbbytes[3] = 0;
                if (c.rgb)c.rgbbytes[3] = 0x0f;
                if (c.rgb & 0xFFFFFF || dontcopyblack == 0)DrawPixel(x, y, c.rgb);
            }
        }
    }
    float bilinearly_interpolate(int top, int bottom, int left, int right,
        float horizontal_position, float vertical_position, unsigned char** input)
    {
        // Determine the values of the corners.
        float top_left = (float)input[top][left];
        float top_right = (float)input[top][right];
        float bottom_left = (float)input[bottom][left];
        float bottom_right = (float)input[bottom][right];

        // Figure out "how far" the output pixel being considered is
        // between *_left and *_right.
        float horizontal_progress = horizontal_position -
            (float)left;
        float vertical_progress = vertical_position -
            (float)top;

        // Combine top_left and top_right into one large, horizontal
        // block.
        float top_block = top_left + horizontal_progress
            * (top_right - top_left);

        // Combine bottom_left and bottom_right into one large, horizontal
        // block.
        float bottom_block = bottom_left +
            horizontal_progress
            * (bottom_right - bottom_left);

        // Combine the top_block and bottom_block using vertical
        // interpolation and return as the resulting pixel.
        return top_block + vertical_progress * (bottom_block - top_block);
    }
    unsigned char*** resize(unsigned char*** input, int M_in, int N_in, int output_height,
        int output_width)
    {
        // Prompt the user.
        // Figure out the dimensions of the new, resized image and define the
        // new array.
        int M_out_resized = output_height;
        int N_out_resized = output_width;
        unsigned char*** output = alloc3df(3, M_out_resized, N_out_resized);

        // Loop through each pixel of the new image and interpolate the pixels
        // based on what's in the input image.
        int i, j, k;
        for (i = 0; i < M_out_resized; ++i) {
            for (j = 0; j < N_out_resized; ++j) {
                // Figure out how far down or across (in percentage) we are with
                // respect to the original image.
                float vertical_position = i * ((float)M_in / M_out_resized);
                float horizontal_position = j * ((float)N_in / N_out_resized);

                // Figure out the four locations (and then, four pixels)
                // that we must interpolate from the original image.
                int top = (int)floorf(vertical_position);
                int bottom = top + 1;
                int left = (int)floorf(horizontal_position);
                int right = left + 1;

                // Check if any of the four locations are invalid. If they are,
                // simply access the valid adjacent pixel.
                if (bottom >= M_in) {
                    bottom = top;
                }
                if (right >= N_in) {
                    right = left;
                }

                // Interpolate the pixel according to the dimensions
                // set above and set the resulting pixel. Do so for each color.
                for (k = 0; k < 3; k++) {
                    float interpolated = bilinearly_interpolate(top, bottom, left,
                        right, horizontal_position, vertical_position, input[k]);
                    output[k][i][j] = (unsigned char)interpolated;
                }

            }
        }

        return output;
    }
    unsigned char*** rotate(unsigned char*** input, int M_in, int N_in, float rotation_factor)
    {
        // Prompt the user.

        // Prepare the output files.
        uint8_t*** output = alloc3df(3, M_in, N_in);

        // Define the center of the image.
        float vertical_center = floorf((float)M_in / (float)2.0);
        float horizontal_center = floorf((float)N_in / (float)2.0);
        if (fabsf((float)M_in / (float)2.0 - floorf((float)M_in / (float)2.0)) < (float)0.2)vertical_center -= (float)0.5;
        if (fabsf((float)N_in / (float)2.0 - floorf((float)N_in / (float)2.0)) < (float)0.2)horizontal_center -= (float)0.5;
        // Loop through each pixel of the new image, select the new vertical
        // and horizontal positions, and interpolate the image to make the change.
        int i, j, k;
        float angle = rotation_factor * (float)M_PI / (float)180;
        float cosangle = cosf(angle), sinangle = sinf(angle);
        for (i = 0; i < M_in; ++i) {
            for (j = 0; j < N_in; ++j) {
                //    // Figure out how rotated we want the image.
                float vertical_position = cosangle *
                    ((float)i - vertical_center) + sinangle * ((float)j - horizontal_center)
                    + vertical_center;
                float horizontal_position = -sinangle *
                    ((float)i - vertical_center) + cosangle * ((float)j - horizontal_center)
                    + horizontal_center;

                // Figure out the four locations (and then, four pixels)
                // that we must interpolate from the original image.
                int top = (int)floor(vertical_position);
                int bottom = top + 1;
                int left = (int)floor(horizontal_position);
                int right = left + 1;

                // Check if any of the four locations are invalid. If they are,
                // skip interpolating this pixel. Otherwise, interpolate the
                // pixel according to the dimensions set above and set the
                // resulting pixel.
                if (top >= 0 && bottom < M_in && left >= 0 && right < N_in) {
                    for (k = 0; k < 3; k++) {
                        float interpolated = bilinearly_interpolate(top, bottom,
                            left, right, horizontal_position, vertical_position,
                            input[k]);
                        output[k][i][j] = (uint8_t)interpolated;
                    }
                }
            }
        }

        return output;
    }

    void cmd_image(void) {
        unsigned char* p;
        if ((p = checkstring(cmdline, (unsigned char*)"ROTATE_FAST"))) {
            int dontcopyblack = 0;
            getargs(&p, 17, (unsigned char*)",");
            if (argc < 13)error((char *)"Argument count");
            int cursorhidden = 0;
            if (argc >= 15 && *argv[14]) {
                if (checkstring(argv[14], (unsigned char*)"FRAMEBUFFER")) {
                    ReadPage = WPN;
                }
                else {
                    ReadPage = (int)getint(argv[14], 0, MAXPAGES);
                }
            }
            int maxWR = PageTable[ReadPage].xmax;
            int maxHR = PageTable[ReadPage].ymax;
            int maxH = PageTable[WritePage].ymax;
            int maxW = PageTable[WritePage].xmax;
            int Scale = 1;
            uint32_t wpa = (uint32_t)PageTable[WritePage].address;
            uint32_t ox = (uint32_t)getint(argv[0], 0, maxWR - 1);
            uint32_t oy = (uint32_t)getint(argv[2], 0, maxHR - 1);
            uint32_t width = (uint32_t)getint(argv[4], 1, maxWR - ox);
            uint32_t height = (uint32_t)getint(argv[6], 1, maxHR - oy);
            uint32_t nx = (uint32_t)getint(argv[8], 0, maxW - 1);
            uint32_t ny = (uint32_t)getint(argv[10], 0, maxH - 1);
            float angle = (float)getnumber(argv[12]) * (float)M_PI / 180;
            if (argc == 17)dontcopyblack = (int)getint(argv[16], 0, 1);
            int sinma = (int)(sin(-angle) * (float)65536.0);
            int cosma = (int)(cos(-angle) * (float)65536.0);
            float hwidth = (float)width / (float)2.0;
            float hheight = (float)height / (float)2.0;
            if ((width & 1) == 0)hwidth -= 0.5;
            if ((height & 1) == 0)hheight -= 0.5;
            int c, x, y, xs, ys, xx, yy;
            float xt, yt;
            int multiplier = 4;
            uint32_t* buff;
            if (width * height * multiplier < sizeof(linebuff)) {
                buff = linebuff;
            }
            else {
                buff = (uint32_t*)GetTempMemory(width * height * multiplier);
            }
            ReadBuffer(ox, oy, ox + width - 1, oy + height - 1, buff);
            for (x = 0; x < (int)width; x++) {
                xt = x - hwidth;
                for (y = 0; y < (int)height; y++) {
                    yt = y - hheight;
                    xs = ((int)(cosma * xt - sinma * yt) >> 16) + width / 2;
                    ys = ((int)(sinma * xt + cosma * yt) >> 16) + height / 2;
                    //				PInt(xs);PIntComma(ys);PRet();
                    if (xs >= 0 && xs < (int)width && ys >= 0 && ys < (int)height) {
                             c = buff[xs + ys * width];
                            if (dontcopyblack == 0 || c & 0xFFFFFF) {
                                yy = y + ny;
                                xx = x + nx;
                                if (optiony)yy = maxH - 1 - yy;
                                if (xx < maxW && yy < maxH && xx >= 0 && yy >= 0) {
                                    *(uint32_t*)((yy * maxW + xx) * 4 + wpa) = (uint32_t)c;
                                }
                            }
                        }
                    }
            }
            ReadPage = WritePage;
            return;
        }
        if ((p = checkstring(cmdline, (unsigned char*)"ROTATE"))) {
            static uint8_t*** temp2 = NULL;
            int dontcopyblack = 0;
            getargs(&p, 17, (unsigned char*)",");
            if (argc < 13)error((char *)"Argument count");
            int cursorhidden = 0;
            if (argc >= 15 && *argv[14]) {
                if (checkstring(argv[14], (unsigned char*)"FRAMEBUFFER")) {
                    ReadPage = WPN;
                }
                else {
                    ReadPage = (int)getint(argv[14], 0, MAXPAGES);
                }
            }
            int maxWR = PageTable[ReadPage].xmax;
            int maxHR = PageTable[ReadPage].ymax;
            int maxH = PageTable[WritePage].ymax;
            int maxW = PageTable[WritePage].xmax;
            uint32_t x = (uint32_t)getint(argv[0], 0, maxWR - 1);
            uint32_t y = (uint32_t)getint(argv[2], 0, maxHR - 1);
            uint32_t w = (uint32_t)getint(argv[4], 1, maxWR - x);
            uint32_t h = (uint32_t)getint(argv[6], 1, maxHR - y);
            uint32_t nx = (uint32_t)getint(argv[8], 0, maxW - 1);
            uint32_t ny = (uint32_t)getint(argv[10], 0, maxH - 1);
            uint8_t*** output = alloc3df(3, h, w);
            float angle = (float)( - getnumber(argv[12]));
            if (argc == 17)dontcopyblack = (int)getint(argv[16], 0, 1);
            copytofloat(output, x, y, w, h);
            temp2 = rotate(output, h, w, angle);
            dealloc3df(output, 3, h, w);
            copyfromfloat(temp2, nx, ny, w, h, dontcopyblack);
            dealloc3df(temp2, 3, h, w);
            ReadPage = WritePage;
            return;
        }
        if ((p = checkstring(cmdline, (unsigned char*)"WARP_H"))) {
            getargs(&p, 23, (unsigned char*)",");
            if (argc < 19)error((char *)"Argument count");
            int cursorhidden = 0;
            int dontcopyblack = 0;
            if (argc >= 21 && *argv[20]) {
                if (checkstring(argv[20], (unsigned char*)"FRAMEBUFFER")) {
                    ReadPage = WPN;
                }
                else {
                    ReadPage = (int)getint(argv[20], 0, MAXPAGES);
                }
            }
            int maxWR = PageTable[ReadPage].xmax;
            int maxHR = PageTable[ReadPage].ymax;
            int maxH = PageTable[WritePage].ymax;
            int maxW = PageTable[WritePage].xmax;
            int Scale = 1;
            uint32_t wpa = (uint32_t)PageTable[WritePage].address;
            int xx, yy, ys, ye, ww, c, xp, yp;
            float ratiox, ratioy;
            int px, py, yshift;
            //get details of the source rectangle
            int32_t x = (int)getint(argv[0], 0, maxWR - 1);
            int32_t y = (int)getint(argv[2], 0, maxHR - 1);
            int32_t w1 = (int)getint(argv[4], 1, maxWR - x);
            int32_t h1 = (int)getint(argv[6], 1, maxHR - y);
            // get details of the trapezoid
            int32_t nx = (int)getint(argv[8], 0, maxW - 1);
            int32_t ny = (int)getint(argv[10], 0, maxH - 1);
            int32_t nh = (int)getint(argv[12], 1, maxH - ny);
            int32_t nx2 = (int)getint(argv[14], nx + 1, maxW - 1);
            int32_t ny2 = (int)getint(argv[16], 0, maxH - 1);
            int32_t nh2 = (int)getint(argv[18], 1, maxH - ny2);
            float x_ratio = ((float)w1 / (float)(nx2 - nx + 1));
            float y_ratio = ((float)h1 / (float)(nh2));
            // read in the rectangle
            if (argc == 23)dontcopyblack = (int)getint(argv[22], 0, 1);
            int multiplier = 4;
            uint32_t* buff;
            if (w1 * h1 * multiplier < sizeof(linebuff)) {
                buff = linebuff;
            }
            else {
                buff = (uint32_t *)GetTempMemory(w1 * h1 * multiplier);
            }
            ReadBuffer(x, y, x + w1 - 1, y + h1 - 1, buff);
            ww = nx2 - nx;
                for (xx = 0; xx < ww; xx++) {
                    ratiox = (float)xx / (float)(ww - 1);
                    ys = ny - (int)(ratiox * (float)(ny - ny2));
                    ye = ys + nh - (int)(ratiox * (float)(nh - nh2));
                    ratioy = (float)(nh2) / (float)(ye - ys);
                    px = (int)((float)xx*ratioy * x_ratio);
                    yshift = (int)(y_ratio * ratioy * (float)65536.0);
                    for (yy = 0; yy < ye - ys; yy++) {
                        py = (yy * yshift) >> 16;
                        c = buff[px + py * w1];
                        if (dontcopyblack == 0 || c & 0xFFFFFF) {
                            yp = yy + ys;
                            xp = xx + nx;
                            if (optiony)yy = maxH - 1 - yy;
                            if (xp < maxW && yp < maxH && xp >= 0 && yp >= 0) {
                                *(uint32_t*)((yp * maxW + xp) * 4 + wpa) = (uint32_t)c;
                            }
                        }
                    }
            }
            // tidy up
            ReadPage = WritePage;
            return;
        }

        if ((p = checkstring(cmdline, (unsigned char*)"WARP_V"))) {
            getargs(&p, 23, (unsigned char*)",");
            if (argc < 19)error((char *)"Argument count");
            int cursorhidden = 0;
            int dontcopyblack = 0;
            if (argc >= 21 && *argv[20]) {
                if (checkstring(argv[20], (unsigned char*)"FRAMEBUFFER")) {
                    ReadPage = WPN;
                }
                else {
                    ReadPage = (int)getint(argv[20], 0, MAXPAGES);
                }
            }
            int maxWR = PageTable[ReadPage].xmax;
            int maxHR = PageTable[ReadPage].ymax;
            int maxH = PageTable[WritePage].ymax;
            int maxW = PageTable[WritePage].xmax;
            int Scale = 1;
            uint32_t wpa = (uint32_t)PageTable[WritePage].address;
            int xx, yy, xs, xe, wh, c, xp, yp;
            float ratiox, ratioy;
            int px, py, xshift;
            //get details of the source rectangle
            int32_t x = (int)getint(argv[0], 0, maxWR - 1);
            int32_t y = (int)getint(argv[2], 0, maxHR - 1);
            int32_t w1 = (int)getint(argv[4], 1, maxWR - x);
            int32_t h1 = (int)getint(argv[6], 1, maxHR - y);
            // get details of the trapezoid
            int32_t nx = (int)getint(argv[8], 0, maxW - 1);
            int32_t ny = (int)getint(argv[10], 0, maxH - 1);
            int32_t nw = (int)getint(argv[12], 1, maxW - nx);
            int32_t nx2 = (int)getint(argv[14], 0, maxW - 1);
            int32_t ny2 = (int)getint(argv[16], ny + 1, maxH - 1);
            int32_t nw2 = (int)getint(argv[18], 1, maxW - nx2);
            float x_ratio = ((float)w1 / (float)nw2);
            float y_ratio = ((float)h1 / (float)(ny2 - ny + 1));
            // read in the rectangle
            if (argc == 23)dontcopyblack = (int)getint(argv[22], 0, 1);
            int multiplier = 4;
            uint32_t* buff;
            if (w1 * h1 * multiplier < sizeof(linebuff)) {
                buff = linebuff;
            }
            else {
                buff = (uint32_t*)GetTempMemory(w1 * h1 * multiplier);
            }
            ReadBuffer(x, y, x + w1 - 1, y + h1 - 1, buff);
            wh = ny2 - ny;
                for (yy = 0; yy < wh; yy++) {
                    ratioy = (float)yy / (float)(wh - 1);
                    xs = nx - (int)(ratioy * (float)(nx - nx2));
                    xe = xs + nw - (int)(ratioy * (float)(nw - nw2));
                    ratiox = (float)(nw2) / (float)(xe - xs);
                    py = (int)((float)yy*ratiox * y_ratio);
                    xshift = (int)(x_ratio * ratiox * (float)65536.0);
                    for (xx = 0; xx < xe - xs; xx++) {
                        px = (xx * xshift) >> 16;
                        c = buff[px + py * w1];
                        if (dontcopyblack == 0 || c & 0xFFFFFF) {
                            yp = yy + ny;
                            xp = xx + xs;
                            if (optiony)yy = maxH - 1 - yy;
                            if (xp < maxW && yp < maxH && xp >= 0 && yp >= 0) {
                                *(uint32_t*)((yp * maxW + xp) * 4 + wpa) = (uint32_t)c;
                            }
                        }
                    }
             }
            // tidy up
            ReadPage = WritePage;
            return;
        }
        if ((p = checkstring(cmdline, (unsigned char*)"RESIZE_FAST"))) {
            getargs(&p, 19, (unsigned char*)",");
            if (argc < 15)error((char *)"Argument count");
            int cursorhidden = 0;
            int dontcopyblack = 0;
            if (argc >= 17 && *argv[16]) {
                if (checkstring(argv[16], (unsigned char*)"FRAMEBUFFER")) {
                    ReadPage = WPN;
                }
                else {
                    ReadPage = (int)getint(argv[16], 0, MAXPAGES);
                }
            }
            int maxWR = PageTable[ReadPage].xmax;
            int maxHR = PageTable[ReadPage].ymax;
            int maxH = PageTable[WritePage].ymax;
            int maxW = PageTable[WritePage].xmax;
            uint32_t x = (uint32_t)getint(argv[0], 0, maxWR - 1);
            uint32_t y = (uint32_t)getint(argv[2], 0, maxHR - 1);
            uint32_t w1 = (uint32_t)getint(argv[4], 1, maxWR - x);
            uint32_t h1 = (uint32_t)getint(argv[6], 1, maxHR - y);
            uint32_t nx = (uint32_t)getint(argv[8], 0, maxW - 1);
            uint32_t ny = (uint32_t)getint(argv[10], 0, maxH - 1);
            uint32_t w2 = (uint32_t)getint(argv[12], 1, maxW - nx);
            uint32_t h2 = (uint32_t)getint(argv[14], 1, maxH - ny);
            if (argc == 19)dontcopyblack = (int)(getint(argv[18], 0, 1) << 2);
            int c, multiplier = 4;
            int px, py, yy;
            if (w1 == w2 && h1 == h2 && ReadPage != WritePage) {
                MoveBufferNormal(x, y, nx, ny, w1, h1, dontcopyblack);
            }
            else if (w1 == w2 && ReadPage != WritePage) {
                float y_ratio = ((float)h1 / (float)h2);
                    for (yy = 0; yy < (int)h2; yy++) {
                        py = (int)((float)yy * y_ratio) + y;
                        MoveBufferNormal(x, py, nx, ny + yy, w1, 1, dontcopyblack);
                    }
            }
            else {
                int x_ratio = (int)((float)w1 / (float)w2 * (float)65536.0);
                int y_ratio = (int)((float)h1 / (float)h2 * (float)65536.0);
                int Scale = 1;
                uint32_t wpa = (uint32_t)PageTable[WritePage].address;
                uint32_t* buff;
                if (w1 * h1 * multiplier < sizeof(linebuff)) {
                    buff = linebuff;
                }
                else {
                    buff = (uint32_t*)GetTempMemory(w1 * h1 * multiplier);
                }
                ReadBuffer(x, y, x + w1 - 1, y + h1 - 1, buff);
                    uint32_t* start1 = (uint32_t*)((ny * maxW + nx) * 4 + wpa);
                    uint32_t* cout1;
                    for (int i = 0; i < (float)h2; i++) {
                        cout1 = start1;
                        py = (i * y_ratio) >> 16;
                        for (int j = 0; j < (float)w2; j++) {
                            px = (j * x_ratio) >> 16;
                            c = buff[(int)(px + py * w1)];
                            if (dontcopyblack == 0 || c &0xFFFFFF) {
                                *cout1 = c;
                            }
                            cout1++;
                        }
                        start1 += maxW;
                    }
             }
            ReadPage = WritePage;
            return;
        }

        if ((p = checkstring(cmdline, (unsigned char*)"RESIZE"))) {
            int dontcopyblack = 0;
            static uint8_t*** temp2 = NULL;
            getargs(&p, 19, (unsigned char*)",");
            if (argc < 15)error((char *)"Argument count");
            int cursorhidden = 0;
            if (argc >= 17 && *argv[16]) {
                if (checkstring(argv[16], (unsigned char*)"FRAMEBUFFER")) {
                    ReadPage = WPN;
                }
                else {
                    ReadPage = (int)getint(argv[16], 0, MAXPAGES);
                }
            }
            int maxWR = PageTable[ReadPage].xmax;
            int maxHR = PageTable[ReadPage].ymax;
            int maxH = PageTable[WritePage].ymax;
            int maxW = PageTable[WritePage].xmax;
            uint32_t x = (uint32_t)getint(argv[0], 0, maxWR - 1);
            uint32_t y = (uint32_t)getint(argv[2], 0, maxHR - 1);
            uint32_t w = (uint32_t)getint(argv[4], 1, maxWR - x);
            uint32_t h = (uint32_t)getint(argv[6], 1, maxHR - y);
            uint32_t nx = (uint32_t)getint(argv[8], 0, maxW - 1);
            uint32_t ny = (uint32_t)getint(argv[10], 0, maxH - 1);
            uint32_t nw = (uint32_t)getint(argv[12], 1, maxW - nx);
            uint32_t nh = (uint32_t)getint(argv[14], 1, maxH - ny);
            if (argc == 19)dontcopyblack = (int)getint(argv[18], 0, 1);
            uint8_t*** output = alloc3df(3, h, w);
            copytofloat(output, x, y, w, h);
            temp2 = resize(output, h, w, nh, nw);
            dealloc3df(output, 3, h, w);
            copyfromfloat(temp2, nx, ny, nw, nh, dontcopyblack);
            dealloc3df(temp2, 3, nh, nw);
            ReadPage = WritePage;
            return;
        }
        error((char *)"Syntax");
    }
    void Free3DMemory(int i) {
        FreeMemorySafe((void**)&struct3d[i]->q_vertices);//array of original vertices
        FreeMemorySafe((void**)&struct3d[i]->r_vertices); //array of rotated vertices
        FreeMemorySafe((void**)&struct3d[i]->q_centroids);//array of original vertices
        FreeMemorySafe((void**)&struct3d[i]->r_centroids); //array of rotated vertices
        FreeMemorySafe((void**)&struct3d[i]->facecount); //number of vertices for each face
        FreeMemorySafe((void**)&struct3d[i]->facestart); //index into the face_x_vert table of the start of a given face
        FreeMemorySafe((void**)&struct3d[i]->fill); //fill colours
        FreeMemorySafe((void**)&struct3d[i]->line); //line colours
        FreeMemorySafe((void**)&struct3d[i]->colours);
        FreeMemorySafe((void**)&struct3d[i]->face_x_vert); //list of vertices for each face
        FreeMemorySafe((void**)&struct3d[i]->dots);
        FreeMemorySafe((void**)&struct3d[i]->depth);
        FreeMemorySafe((void**)&struct3d[i]->depthindex);
        FreeMemorySafe((void**)&struct3d[i]->normals);
        FreeMemorySafe((void**)&struct3d[i]->flags);
        FreeMemorySafe((void**)&struct3d[i]);
    }
    extern "C" void closeall3d(void) {
        int i;
        for (i = 0; i < MAX3D; i++) {
            if (struct3d[i] != NULL) {
                Free3DMemory(i);
            }
        }
        for (i = 1; i < 4; i++) {
            camera[i].viewplane = -32767;
        }
    }
    void T_Mult(FLOAT3D* q1, FLOAT3D* q2, FLOAT3D* n) {
        FLOAT3D a1 = q1[0], a2 = q2[0], b1 = q1[1], b2 = q2[1], c1 = q1[2], c2 = q2[2], d1 = q1[3], d2 = q2[3];
        n[0] = a1 * a2 - b1 * b2 - c1 * c2 - d1 * d2;
        n[1] = a1 * b2 + b1 * a2 + c1 * d2 - d1 * c2;
        n[2] = a1 * c2 - b1 * d2 + c1 * a2 + d1 * b2;
        n[3] = a1 * d2 + b1 * c2 - c1 * b2 + d1 * a2;
        n[4] = q1[4] * q2[4];
    }

    void T_Invert(FLOAT3D* q, FLOAT3D* n) {
        n[0] = q[0];
        n[1] = -q[1];
        n[2] = -q[2];
        n[3] = -q[3];
        n[4] = q[4];
    }

    void depthsort(FLOAT3D* farray, int n, int* index) {
        int i, j = n, s = 1;
        int t;
        FLOAT3D f;
        while (s) {
            s = 0;
            for (i = 1; i < j; i++) {
                if (farray[i] > farray[i - 1]) {
                    f = farray[i];
                    farray[i] = farray[i - 1];
                    farray[i - 1] = f;
                    s = 1;
                    if (index != NULL) {
                        t = index[i - 1];
                        index[i - 1] = index[i];
                        index[i] = t;
                    }
                }
            }
            j--;
        }
    }
    void q_rotate(s_quaternion* in, s_quaternion rotate, s_quaternion* out) {
        //	PFlt(in->x);PFltComma(in->y);PFltComma(in->z);PFltComma(in->m);PRet();
        s_quaternion temp, qtemp;
        T_Mult((FLOAT3D*)&rotate, (FLOAT3D*)in, (FLOAT3D*)&temp);
        T_Invert((FLOAT3D*)&rotate, (FLOAT3D*)&qtemp);
        T_Mult((FLOAT3D*)&temp, (FLOAT3D*)&qtemp, (FLOAT3D*)out);
        //	PFlt(out->x);PFltComma(out->y);PFltComma(out->z);PFltComma(out->m);PRet();
    }
    void normalise(s_vector* v) {
        FLOAT3D n = sqrt((v->x) * (v->x) + (v->y) * (v->y) + (v->z) * (v->z));
        v->x /= n;
        v->y /= n;
        v->z /= n;
    }
    void display3d(int n, FLOAT3D x, FLOAT3D y, FLOAT3D z, int clear, int nonormals) {
        s_vector ray, lighting = { 0 };
        s_vector p1, p2, p3, U, V;
        FLOAT3D x1, y1, z1, tmp;
        FLOAT3D at, bt, ct, t, /*A=0, B=0, */C = 1, D = -camera[struct3d[n]->camera].viewplane;
        int maxH = PageTable[WritePage].ymax;
        int maxW = PageTable[WritePage].xmax;
        int vp, v, f, sortindex;
        int64_t csave = 0, fsave = 0;
        if (struct3d[n]->vmax > 4) { //needed for polygon fill
            main_fill_polyX = (TFLOAT*)GetMemory(struct3d[n]->tot_face_x_vert * sizeof(TFLOAT));
            main_fill_polyY = (TFLOAT*)GetMemory(struct3d[n]->tot_face_x_vert * sizeof(TFLOAT));
        }
        if (struct3d[n]->xmin != 32767 && clear)DrawRectangle(struct3d[n]->xmin, struct3d[n]->ymin, struct3d[n]->xmax, struct3d[n]->ymax, 0);
        struct3d[n]->xmin = 32767;
        struct3d[n]->ymin = 32767;
        struct3d[n]->xmax = -32767;
        struct3d[n]->ymax = -32767;
        short xcoord[MAX_POLYGON_VERTICES], ycoord[MAX_POLYGON_VERTICES];
        struct3d[n]->distance = 0.0;
        for (f = 0; f < struct3d[n]->nf; f++) {
            // calculate the surface normals for each face
            vp = struct3d[n]->facestart[f];
            p1.x = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 1]].x * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 1]].m + x;
            p1.y = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 1]].y * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 1]].m + y;
            p1.z = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 1]].z * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 1]].m + z;
            p2.x = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 2]].x * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 2]].m + x;
            p2.y = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 2]].y * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 2]].m + y;
            p2.z = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 2]].z * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 2]].m + z;
            p3.x = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp]].x * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp]].m + x;
            p3.y = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp]].y * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp]].m + y;
            p3.z = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp]].z * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp]].m + z;
            U.x = p2.x - p1.x;  U.y = p2.y - p1.y;  U.z = p2.z - p1.z;
            V.x = p3.x - p1.x;  V.y = p3.y - p1.y;  V.z = p3.z - p1.z;
            struct3d[n]->normals[f].x = U.y * V.z - U.z * V.y;
            struct3d[n]->normals[f].y = U.z * V.x - U.x * V.z;
            struct3d[n]->normals[f].z = U.x * V.y - U.y * V.x;
            normalise(&struct3d[n]->normals[f]);
            ray.x = p1.x - camera[struct3d[n]->camera].x;
            ray.y = p1.y - camera[struct3d[n]->camera].y;
            ray.z = p1.z - camera[struct3d[n]->camera].z;
            normalise(&ray);
            lighting.x = p1.x - struct3d[n]->light.x;
            lighting.y = p1.y - struct3d[n]->light.y;
            lighting.z = p1.z - struct3d[n]->light.z;
            normalise(&lighting);
            struct3d[n]->dots[f] = ray.x * struct3d[n]->normals[f].x + ray.y * struct3d[n]->normals[f].y + ray.z * struct3d[n]->normals[f].z;
            tmp = struct3d[n]->r_centroids[f].m;
            struct3d[n]->depth[f] = sqrt(
                (struct3d[n]->r_centroids[f].z * tmp + z - camera[struct3d[n]->camera].z) *
                (struct3d[n]->r_centroids[f].z * tmp + z - camera[struct3d[n]->camera].z) +
                (struct3d[n]->r_centroids[f].y * tmp + y - camera[struct3d[n]->camera].y) *
                (struct3d[n]->r_centroids[f].y * tmp + y - camera[struct3d[n]->camera].y) +
                (struct3d[n]->r_centroids[f].x * tmp + x - camera[struct3d[n]->camera].x) *
                (struct3d[n]->r_centroids[f].x * tmp + x - camera[struct3d[n]->camera].x)
            );
            struct3d[n]->depthindex[f] = f;
            struct3d[n]->distance += struct3d[n]->depth[f];
        }
        struct3d[n]->distance /= f;
        // sort the distances from the faces to the camera
        depthsort(struct3d[n]->depth, struct3d[n]->nf, struct3d[n]->depthindex);
        // display the forward facing faces in the order of the furthest away first
        for (f = 0; f < struct3d[n]->nf; f++) {
            sortindex = struct3d[n]->depthindex[f];
            vp = struct3d[n]->facestart[sortindex];
            if (struct3d[n]->flags[sortindex] & 4)struct3d[n]->dots[sortindex] = -struct3d[n]->dots[sortindex];
            if (nonormals || struct3d[n]->dots[sortindex] < 0) {
                for (v = 0; v < struct3d[n]->facecount[sortindex]; v++) {
                    x1 = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + v]].x * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + v]].m + x;
                    y1 = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + v]].y * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + v]].m + y;
                    z1 = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + v]].z * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + v]].m + z;
                    // We now have the coordinates in real space so project them
                    at = x1 - camera[struct3d[n]->camera].x;
                    bt = y1 - camera[struct3d[n]->camera].y;
                    ct = z1 - camera[struct3d[n]->camera].z;
                    t = -(/*A * x1 + B * y1*/ +C * z1 + D) / (/*A * at + B * bt + */C * ct);
                    xcoord[v] = (short)(x1 + round(at * t) + (maxW >> 1) - camera[struct3d[n]->camera].x - camera[struct3d[n]->camera].panx);
                    if (optiony)ycoord[v] = (short)round(y1 + bt * t);
                    else ycoord[v] = (short)(maxH - round(y1 + bt * t) - 1);
                    ycoord[v] -= (short)((maxH >> 1) - camera[struct3d[n]->camera].y - camera[struct3d[n]->camera].pany);
                    if (clear) {
                        if (xcoord[v] > struct3d[n]->xmax)struct3d[n]->xmax = xcoord[v];
                        if (xcoord[v] < struct3d[n]->xmin)struct3d[n]->xmin = xcoord[v];
                        if (ycoord[v] > struct3d[n]->ymax)struct3d[n]->ymax = ycoord[v];
                        if (ycoord[v] < struct3d[n]->ymin)struct3d[n]->ymin = ycoord[v];
                    }
                }
                if ((struct3d[n]->flags[sortindex] & 1) == 0) {
                    if (struct3d[n]->flags[sortindex] & 10) {
                        fsave = struct3d[n]->fill[sortindex];
                        csave = struct3d[n]->line[sortindex];
                        if (struct3d[n]->flags[sortindex] & 2)struct3d[n]->fill[sortindex] = 0xFF0000;
                        if (struct3d[n]->flags[sortindex] & 8) {
                            FLOAT3D lightratio = fabs(lighting.x * struct3d[n]->normals[sortindex].x + lighting.y * struct3d[n]->normals[sortindex].y + lighting.z * struct3d[n]->normals[sortindex].z);
                            lightratio = (lightratio * struct3d[n]->ambient) + struct3d[n]->ambient;
                            int red = (struct3d[n]->fill[sortindex] & 0xFF0000) >> 16;
                            int green = (struct3d[n]->fill[sortindex] & 0xFF00) >> 8;
                            int blue = (struct3d[n]->fill[sortindex] & 0xFF);
                            int trans = (struct3d[n]->fill[sortindex] & 0xF000000);
                            red = (int)(round)((FLOAT3D)red * lightratio);
                            green = (int)(round)((FLOAT3D)green * lightratio);
                            blue = (int)(round)((FLOAT3D)blue * lightratio);
                            struct3d[n]->fill[sortindex] = trans | (red << 16) | (green << 8) | blue;
                            red = (struct3d[n]->line[sortindex] & 0xFF0000) >> 16;
                            green = (struct3d[n]->line[sortindex] & 0xFF00) >> 8;
                            blue = (struct3d[n]->line[sortindex] & 0xFF);
                            trans = (struct3d[n]->line[sortindex] & 0xF000000);
                            red = (int)(round)((FLOAT3D)red * lightratio);
                            green = (int)(round)((FLOAT3D)green * lightratio);
                            blue = (int)(round)((FLOAT3D)blue * lightratio);
                            struct3d[n]->line[sortindex] = trans | (red << 16) | (green << 8) | blue;
                        }
                    }
                    DrawPolygon(n, xcoord, ycoord, sortindex);
                    if (struct3d[n]->flags[sortindex] & 10) {
                        struct3d[n]->fill[sortindex] = fsave;
                        struct3d[n]->line[sortindex] = csave;
                    }
                }
            }
        }
        // Save information about how it was displayed for DRAW3D function and RESTORE command
        struct3d[n]->current.x = x;
        struct3d[n]->current.y = y;
        struct3d[n]->current.z = z;
        struct3d[n]->nonormals = nonormals;
        if (struct3d[n]->vmax > 4) { //needed for polygon fill
            FreeMemory((unsigned char *)main_fill_polyX);
            FreeMemory((unsigned char *)main_fill_polyY);
        }

    }
    void diagnose3d(int n, FLOAT3D x, FLOAT3D y, FLOAT3D z, int sort) {
        s_vector ray, normals;
        s_vector p1, p2, p3, U, V;
        FLOAT3D tmp;
        int vp, f, sortindex;
        for (f = 0; f < struct3d[n]->nf; f++) {
            // calculate the surface normals for each face
            vp = struct3d[n]->facestart[f];
            p1.x = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 1]].x * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 1]].m + x;
            p1.y = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 1]].y * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 1]].m + y;
            p1.z = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 1]].z * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 1]].m + z;
            p2.x = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 2]].x * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 2]].m + x;
            p2.y = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 2]].y * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 2]].m + y;
            p2.z = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 2]].z * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp + 2]].m + z;
            p3.x = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp]].x * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp]].m + x;
            p3.y = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp]].y * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp]].m + y;
            p3.z = struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp]].z * struct3d[n]->r_vertices[struct3d[n]->face_x_vert[vp]].m + z;
            U.x = p2.x - p1.x;  U.y = p2.y - p1.y;  U.z = p2.z - p1.z;
            V.x = p3.x - p1.x;  V.y = p3.y - p1.y;  V.z = p3.z - p1.z;
            normals.x = U.y * V.z - U.z * V.y;
            normals.y = U.z * V.x - U.x * V.z;
            normals.z = U.x * V.y - U.y * V.x;
            normalise(&normals);
            ray.x = p1.x - camera[struct3d[n]->camera].x;
            ray.y = p1.y - camera[struct3d[n]->camera].y;
            ray.z = p1.z/*  -camera[struct3d[n]->camera].z*/;
            normalise(&ray);
            struct3d[n]->dots[f] = ray.x * normals.x + ray.y * normals.y + ray.z * normals.z;
            tmp = struct3d[n]->r_centroids[f].m;
            struct3d[n]->depth[f] = sqrt(
                (struct3d[n]->r_centroids[f].z * tmp + z - camera[struct3d[n]->camera].z) *
                (struct3d[n]->r_centroids[f].z * tmp + z - camera[struct3d[n]->camera].z) +
                (struct3d[n]->r_centroids[f].y * tmp + y - camera[struct3d[n]->camera].y) *
                (struct3d[n]->r_centroids[f].y * tmp + y - camera[struct3d[n]->camera].y) +
                (struct3d[n]->r_centroids[f].x * tmp + x - camera[struct3d[n]->camera].x) *
                (struct3d[n]->r_centroids[f].x * tmp + x - camera[struct3d[n]->camera].x)
            );
            struct3d[n]->depthindex[f] = f;
        }
        // sort the dot products
        depthsort(struct3d[n]->depth, struct3d[n]->nf, struct3d[n]->depthindex);
        // display the forward facing faces in the order of the furthest away first
        for (f = 0; f < struct3d[n]->nf; f++) {
            if (sort)sortindex = struct3d[n]->depthindex[f];
            else sortindex = f;
            vp = struct3d[n]->facestart[sortindex];
            MMPrintString((char *)"Face "); PInt(sortindex);
            MMPrintString((char *)" at distance "); PFlt(struct3d[n]->depth[f]);
            MMPrintString((char *)" dot product is "); PFlt(struct3d[n]->dots[sortindex]);
            MMPrintString((char *)" so the face is "); MMPrintString(struct3d[n]->dots[sortindex] > 0 ? (char*)"Hidden" : (char*)"Showing"); PRet();
        }
    }
    void cmd_3D(void) {
        unsigned char* p;
        if ((p = checkstring(cmdline, (unsigned char *)"CREATE"))) {
            // parameters are
             // 3D object number (1 to MAX3D
             // # of vertices = nv
             // # of faces = nf
             // vertex structure (nv)
             // face array (face number, vertex number)
             // colours array
             // edge colour index array [nf]
             // fill colour index array [nf]
             // centroid structure [nf]
             // normals structure [nf]
            TFLOAT* vertex;
            TFLOAT tmp;
            long long int* faces, * facecount, * facecountindex, * colours, * linecolour = NULL, * fillcolour = NULL;
            getargs(&p, 19, (unsigned char *)",");
            if (argc < 17)error((char *)"Argument count");
            int c, colourcount = 0, vp, v, f, fc = 0, n = (int)getint(argv[0], 1, MAX3D);
            if (struct3d[n] != NULL)error((char *)"Object already exists");
            int nv = (int)getinteger(argv[2]);
            if (nv < 3)error((char *)"3D object must have a minimum of 3 vertices");
            int nf = (int)getinteger(argv[4]);
            if (nf < 1)error((char *)"3D object must have a minimum of 1 face");
            int cam = (int)getint(argv[6], 1, MAXCAM);
            vertex = (MMFLOAT*)findvar(argv[8], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
            if ((uint32_t)vertex != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Vertex array must be a 2D floating point array");
            if (vartbl[VarIndex].type & T_NBR) {
                if (vartbl[VarIndex].dims[2] != 0) error((char *)"Vertex array must be a 2D floating point array");
                if (vartbl[VarIndex].dims[0] - OptionBase != 2) {		// Not an array
                    error((char *)"Vertex array must have 3 elements in first dimension");
                }
                if (vartbl[VarIndex].dims[1] - OptionBase < nv - 1) {		// Not an array
                    error((char *)"Vertex array too small");
                }
            }
            else error((char *)"Vertex array must be a 2D floating point array");

            facecount = (long long int*)findvar(argv[10], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
            if ((uint32_t)facecount != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Vertex count array must be a 1D integer array");
            if (vartbl[VarIndex].type & T_INT) {
                if (vartbl[VarIndex].dims[1] != 0) error((char *)"Vertex count array must be a 1D integer array");
                if (vartbl[VarIndex].dims[0] - OptionBase < nf - 1) {		// Not an array
                    error((char *)"Vertex count array too small");
                }
            }
            else error((char *)"Vertex count array must be a 1D integer array");
            facecountindex = facecount;
            for (f = 0; f < nf; f++)fc += (int)(*facecountindex++);

            faces = (long long int*)findvar(argv[12], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
            if ((uint32_t)faces != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Face/vertex array must be a 1D integer array");
            if (vartbl[VarIndex].type & T_INT) {
                if (vartbl[VarIndex].dims[1] != 0) error((char *)"Face/vertex array must be a 1D integer array");
                if (vartbl[VarIndex].dims[0] - OptionBase < fc - 1) {		// Not an array
                    error((char *)"Face/vertex array too small");
                }
            }
            else error((char *)"Face/vertex array must be a 1D integer array");

            colours = (long long int*)findvar(argv[14], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
            if ((uint32_t)colours != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Colour array must be a 1D integer array");
            if (vartbl[VarIndex].type & T_INT) {
                if (vartbl[VarIndex].dims[1] != 0) error((char *)"Colour array must be a 1D integer array");
                colourcount = vartbl[VarIndex].dims[0] - OptionBase + 1;
            }
            else error((char *)"Colour array must be a 1D integer array");


            if (argc >= 17 && *argv[16]) {
                linecolour = (long long int*)findvar(argv[16], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
                if ((uint32_t)linecolour != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Line colour array must be a 1D integer array");
                if (vartbl[VarIndex].type & T_INT) {
                    if (vartbl[VarIndex].dims[1] != 0) error((char *)"Line colour  array must be a 1D integer array");
                    if (vartbl[VarIndex].dims[0] - OptionBase < nf - 1) {		// Not an array
                        error((char *)"Line colour  array too small");
                    }
                }
                else error((char *)"Line colour must be a 1D integer array");
            }

            if (argc == 19) {
                fillcolour = (long long int*)findvar(argv[18], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
                if ((uint32_t)fillcolour != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Fill colour array must be a 1D integer array");
                if (vartbl[VarIndex].type & T_INT) {
                    if (vartbl[VarIndex].dims[1] != 0) error((char *)"Fill colour array must be a 1D integer array");
                    if (vartbl[VarIndex].dims[0] - OptionBase < nf - 1) {		// Not an array
                        error((char *)"Fill colour array too small");
                    }
                }
                else error((char *)"Fill colour must be a 1D integer array");
            }
            // The data look valid so now create the object in memory
            struct3d[n] = (struct D3D *)GetMemory(sizeof(struct D3D));
            struct3d[n]->nf = nf;
            struct3d[n]->nv = nv;
            struct3d[n]->current.x = -32767;
            struct3d[n]->current.y = -32767;
            struct3d[n]->current.z = -32767;
            struct3d[n]->xmin = 32767;
            struct3d[n]->ymin = 32767;
            struct3d[n]->xmax = -32767;
            struct3d[n]->ymax = -32767;
            struct3d[n]->camera = cam;
            struct3d[n]->q_vertices = NULL;//array of original vertices
            struct3d[n]->r_vertices = NULL; //array of rotated vertices
            struct3d[n]->q_centroids = NULL;//array of original vertices
            struct3d[n]->r_centroids = NULL; //array of rotated vertices
            struct3d[n]->facecount = NULL; //number of vertices for each face
            struct3d[n]->facestart = NULL; //index into the face_x_vert table of the start of a given face
            struct3d[n]->fill = NULL; //fill colours
            struct3d[n]->line = NULL; //line colours
            struct3d[n]->colours = NULL;
            struct3d[n]->face_x_vert = NULL; //list of vertices for each face
            struct3d[n]->light.x = 0;
            struct3d[n]->light.y = 0;
            struct3d[n]->light.z = 0;
            struct3d[n]->ambient = 0;
            // load up things that have one entry per vertex
            struct3d[n]->q_vertices = (struct t_quaternion *)GetMemory(struct3d[n]->nv * sizeof(struct t_quaternion));
            struct3d[n]->r_vertices = (struct t_quaternion*)GetMemory(struct3d[n]->nv * sizeof(struct t_quaternion));
            for (v = 0; v < struct3d[n]->nv; v++) {
                FLOAT3D m = 0.0;
                struct3d[n]->q_vertices[v].x = (FLOAT3D)(*vertex++);
                m += struct3d[n]->q_vertices[v].x * struct3d[n]->q_vertices[v].x;
                struct3d[n]->q_vertices[v].y = *vertex++;
                m += struct3d[n]->q_vertices[v].y * struct3d[n]->q_vertices[v].y;
                struct3d[n]->q_vertices[v].z = *vertex++;
                m += struct3d[n]->q_vertices[v].z * struct3d[n]->q_vertices[v].z;
                if (m) {
                    m = sqrt(m);
                    struct3d[n]->q_vertices[v].x = struct3d[n]->q_vertices[v].x / m;
                    struct3d[n]->q_vertices[v].y = struct3d[n]->q_vertices[v].y / m;
                    struct3d[n]->q_vertices[v].z = struct3d[n]->q_vertices[v].z / m;
                    struct3d[n]->q_vertices[v].w = 0.0;
                    struct3d[n]->q_vertices[v].m = m;
                }
                else {
                    struct3d[n]->q_vertices[v].x = 0;
                    struct3d[n]->q_vertices[v].y = 0;
                    struct3d[n]->q_vertices[v].z = 0;
                    struct3d[n]->q_vertices[v].w = 0.0;
                    struct3d[n]->q_vertices[v].m = 1.0;
                }
                memcpy(&struct3d[n]->r_vertices[v], &struct3d[n]->q_vertices[v], sizeof(s_quaternion));
            }
            struct3d[n]->tot_face_x_vert = 0;
            //load up things that have one entry per face
            struct3d[n]->vmax = 0;
            struct3d[n]->facecount = (uint8_t*)GetMemory(struct3d[n]->nf * sizeof(uint16_t));
            struct3d[n]->facestart = (uint16_t*)GetMemory(struct3d[n]->nf * sizeof(uint16_t));
            struct3d[n]->fill = (int64_t*)GetMemory(struct3d[n]->nf * sizeof(int64_t));
            struct3d[n]->line = (int64_t*)GetMemory(struct3d[n]->nf * sizeof(int64_t));
            struct3d[n]->r_centroids = (struct t_quaternion*)GetMemory(struct3d[n]->nf * sizeof(struct t_quaternion));
            struct3d[n]->q_centroids = (struct t_quaternion*)GetMemory(struct3d[n]->nf * sizeof(struct t_quaternion));
            struct3d[n]->dots = (FLOAT3D*)GetMemory(struct3d[n]->nf * sizeof(FLOAT3D));
            struct3d[n]->depth = (FLOAT3D*)GetMemory(struct3d[n]->nf * sizeof(FLOAT3D));
            struct3d[n]->flags = (uint8_t*)GetMemory(struct3d[n]->nf * sizeof(uint8_t));
            struct3d[n]->depthindex = (int *)GetMemory(struct3d[n]->nf * sizeof(int));
            struct3d[n]->normals = (struct SVD*)GetMemory(struct3d[n]->nf * sizeof(struct SVD));
            for (f = 0; f < struct3d[n]->nf; f++) {
                struct3d[n]->facecount[f] = (uint8_t)*facecount++;
                if (struct3d[n]->facecount[f] < 3) {
                    Free3DMemory(n);
                    error((char *)"Vertex count less than 3 for face %", f + OptionBase);
                }
                if (struct3d[n]->facecount[f] > struct3d[n]->vmax)struct3d[n]->vmax = struct3d[n]->facecount[f];
                struct3d[n]->facestart[f] = struct3d[n]->tot_face_x_vert;
                struct3d[n]->tot_face_x_vert += struct3d[n]->facecount[f];
            }
            // load up the array that holds all the face vertex information
            struct3d[n]->face_x_vert = (uint16_t*)GetMemory(struct3d[n]->tot_face_x_vert * sizeof(uint16_t)); // allocate memory for the list of vertices per face
            struct3d[n]->colours = (int64_t*)GetMemory(colourcount * sizeof(int64_t));
            for (c = 0; c < colourcount; c++) {
                struct3d[n]->colours[c] = (uint64_t)*colours++;
            }
            for (f = 0; f < struct3d[n]->tot_face_x_vert; f++) {
                struct3d[n]->face_x_vert[f] = (int) * faces++;
            }
            for (f = 0; f < struct3d[n]->nf; f++) {
                if (linecolour != NULL) {
                    int index = (int)(*linecolour++) - OptionBase;
                    if (index >= colourcount || index < 0) {
                        Free3DMemory(n);
                        error((char *)"Edge colour Index %", index);
                    }
                    struct3d[n]->line[f] = struct3d[n]->colours[index];
                }
                else struct3d[n]->line[f] = gui_fcolour;
                if (fillcolour != NULL) {
                    int index = (int)(*fillcolour++) - OptionBase;
                    if (index >= colourcount || index < 0) {
                        Free3DMemory(n);
                        error((char *)"Fill colour Index %", index);
                    }
                    struct3d[n]->fill[f] = struct3d[n]->colours[index];
                }
                else struct3d[n]->fill[f] = 0xFFFFFFFF;
                FLOAT3D x = 0, y = 0, z = 0, scale;
                vp = struct3d[n]->facestart[f];
                // calculate the centroids of each face

                for (v = 0; v < struct3d[n]->facecount[f]; v++) {
                    tmp = struct3d[n]->q_vertices[struct3d[n]->face_x_vert[vp + v]].m;
                    x += struct3d[n]->q_vertices[struct3d[n]->face_x_vert[vp + v]].x * tmp;
                    y += struct3d[n]->q_vertices[struct3d[n]->face_x_vert[vp + v]].y * tmp;
                    z += struct3d[n]->q_vertices[struct3d[n]->face_x_vert[vp + v]].z * tmp;
                }
                x /= (FLOAT3D)struct3d[n]->facecount[f];
                y /= (FLOAT3D)struct3d[n]->facecount[f];
                z /= (FLOAT3D)struct3d[n]->facecount[f];
                struct3d[n]->q_centroids[f].x = x;
                struct3d[n]->q_centroids[f].y = y;
                struct3d[n]->q_centroids[f].z = z;
                scale = sqrt(struct3d[n]->q_centroids[f].x * struct3d[n]->q_centroids[f].x +
                    struct3d[n]->q_centroids[f].y * struct3d[n]->q_centroids[f].y +
                    struct3d[n]->q_centroids[f].z * struct3d[n]->q_centroids[f].z);
                struct3d[n]->q_centroids[f].x /= scale;
                struct3d[n]->q_centroids[f].y /= scale;
                struct3d[n]->q_centroids[f].z /= scale;
                struct3d[n]->q_centroids[f].m = scale;
                struct3d[n]->q_centroids[f].w = 0;
                memcpy(&struct3d[n]->r_centroids[f], &struct3d[n]->q_centroids[f], sizeof(s_quaternion));
            }
            return;
        } else if ((p = checkstring(cmdline, (unsigned char*)"DIAGNOSE"))) {
            getargs(&p, 9, (unsigned char *)",");
            if (argc < 7)error((char *)"Argument count");
            int n = (int)getint(argv[0], 1, MAX3D);
            int x = (int)getint(argv[2], -32766, 32766);
            int y = (int)getint(argv[4], -32766, 32766);
            int z = (int)getinteger(argv[6]);
            int sort = 1;
            if (argc == 9)sort = (int)getint(argv[8], 0, 1);
            if (struct3d[n] == NULL)error((char *)"Object % does not exist", n);
            if (camera[struct3d[n]->camera].viewplane == -32767)error((char *)"Camera position not defined");
            diagnose3d(n, x, y, z, sort);
            return;
        } else if ((p = checkstring(cmdline, (unsigned char *)"LIGHT"))) {
            getargs(&p, 9, (unsigned char *)",");
            if (argc != 9)error((char *)"Argument count");
            int n = (int)getint(argv[0], 1, MAX3D);
            struct3d[n]->light.x = (float)getint(argv[2], -32766, 32766);
            struct3d[n]->light.y = (float)getint(argv[4], -32766, 32766);
            struct3d[n]->light.z = (float)getint(argv[6], -32766, 32766);
            struct3d[n]->ambient = (FLOAT3D)(getint(argv[8], 0, 100)) / 100.0;
            return;
        } else if ((p = checkstring(cmdline, (unsigned char *)"SHOW"))) {
            getargs(&p, 9, (unsigned char *)",");
            if (argc < 7)error((char *)"Argument count");
            int n = (int)getint(argv[0], 1, MAX3D);
            int x = (int)getint(argv[2], -32766, 32766);
            int y = (int)getint(argv[4], -32766, 32766);
            int z = (int)getinteger(argv[6]);
            int nonormals = 0;
            if (argc == 9)nonormals = (int)getint(argv[8], 0, 1);
            if (struct3d[n] == NULL)error((char *)"Object % does not exist", n);
            if (camera[struct3d[n]->camera].viewplane == -32767)error((char *)"Camera position not defined");
            display3d(n, x, y, z, 1, nonormals);
            return;
        }
        else if ((p = checkstring(cmdline, (unsigned char *)"SET FLAGS"))) {
            int i, face, nbr;
            getargs(&p, ((MAX_ARG_COUNT - 1) * 2) - 1, (unsigned char *)",");
            if ((argc & 0b11) != 0b11) error((char *)"Invalid syntax");
            int n = (int)getint(argv[0], 1, MAX3D);
            int flag = (int)getint(argv[2], 0, 255);
            // step over the equals sign and get the value for the assignment
            for (i = 4; i < argc; i += 4) {
                face = (int)getinteger(argv[i]);
                nbr = (int)getinteger(argv[i + 2]);

                if (nbr <= 0 || nbr > struct3d[n]->nf - face) error((char *)"Invalid argument");

                while (--nbr >= 0) {
                    struct3d[n]->flags[face + nbr] = flag;
                }
            }
        }
        else if ((p = checkstring(cmdline, (unsigned char *)"ROTATE"))) {
            void* ptr1 = NULL;
            int i, n, v, f;
            s_quaternion q1;
            MMFLOAT* q = NULL;
            getargs(&p, (MAX_ARG_COUNT * 2) - 1, (unsigned char *)",");				// getargs macro must be the first executable stmt in a block
            if (((argc & 0x01) || argc < 3) == 0) error((char *)"Argument count");
            ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
            if (vartbl[VarIndex].type & T_NBR) {
                if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
                if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
                    error((char *)"Argument 1 must be a 5 element floating point array");
                }
                if (vartbl[VarIndex].dims[0] - OptionBase != 4)error((char *)"Argument 1 must be a 5 element floating point array");
                q = (MMFLOAT*)ptr1;
                if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
            }
            else error((char *)"Argument 1 must be a 5 element floating point array");
            q1.w = (FLOAT3D)(*q++);
            q1.x = (FLOAT3D)(*q++);
            q1.y = (FLOAT3D)(*q++);
            q1.z = (FLOAT3D)(*q++);
            q1.m = (FLOAT3D)(*q);
            for (i = 2; i < argc; i += 2) {
                n = (int)getint(argv[i], 1, MAX3D);
                if (struct3d[n] == NULL)error((char *)"Object % does not exist", n);
                for (v = 0; v < struct3d[n]->nv; v++) {
                    q_rotate(&struct3d[n]->q_vertices[v], q1, &struct3d[n]->r_vertices[v]);
                }
                for (f = 0; f < struct3d[n]->nf; f++) {
                    q_rotate(&struct3d[n]->q_centroids[f], q1, &struct3d[n]->r_centroids[f]);
                }
            }
            return;
        }
        else if ((p = checkstring(cmdline, (unsigned char *)"HIDE ALL"))) {
            for (int i = 1; i <= MAX3D; i++) {
                if (struct3d[i] != NULL && struct3d[i]->xmin != 32767) {
                    DrawRectangle(struct3d[i]->xmin, struct3d[i]->ymin, struct3d[i]->xmax, struct3d[i]->ymax, 0);
                    struct3d[i]->xmin = 32767;
                    struct3d[i]->ymin = 32767;
                    struct3d[i]->xmax = -32767;
                    struct3d[i]->ymax = -32767;
                }
            }
            return;
        }
        else if ((p = checkstring(cmdline, (unsigned char *)"RESET"))) {
            int i, n;
            int v, f;
            getargs(&p, (MAX_ARG_COUNT * 2) - 1, (unsigned char *)",");				// getargs macro must be the first executable stmt in a block
            if ((argc & 0x01 || argc < 3) == 0) error((char *)"Argument count");
            for (i = 0; i < argc; i += 2) {
                n = (int)getint(argv[i], 1, MAX3D);
                for (v = 0; v < struct3d[n]->nv; v++) {
                    memcpy(&struct3d[n]->q_vertices[v], &struct3d[n]->r_vertices[v], sizeof(s_quaternion));
                }
                for (f = 0; f < struct3d[n]->nf; f++) {
                    memcpy(&struct3d[n]->q_centroids[f], &struct3d[n]->r_centroids[f], sizeof(s_quaternion));
                }
            }
            return;
        }
        else if ((p = checkstring(cmdline, (unsigned char *)"HIDE"))) {
            int i, n;
            getargs(&p, (MAX_ARG_COUNT * 2) - 1, (unsigned char *)",");				// getargs macro must be the first executable stmt in a block
            if ((argc & 0x01 || argc < 3) == 0) error((char *)"Argument count");
            for (i = 0; i < argc; i += 2) {
                n = (int)getint(argv[i], 1, MAX3D);
                if (struct3d[n] == NULL)error((char *)"Object % does not exist", n);
                if (struct3d[n]->xmin == 32767)return;
                DrawRectangle(struct3d[n]->xmin, struct3d[n]->ymin, struct3d[n]->xmax, struct3d[n]->ymax, 0);
                struct3d[n]->xmin = 32767;
                struct3d[n]->ymin = 32767;
                struct3d[n]->xmax = -32767;
                struct3d[n]->ymax = -32767;
            }
            return;
        }
        else if ((p = checkstring(cmdline, (unsigned char *)"RESTORE"))) {
            int i, n;
            getargs(&p, (MAX_ARG_COUNT * 2) - 1, (unsigned char *)",");				// getargs macro must be the first executable stmt in a block
            if ((argc & 0x01 || argc < 3) == 0) error((char *)"Argument count");
            for (i = 0; i < argc; i += 2) {
                n = (int)getint(argv[i], 1, MAX3D);
                if (struct3d[n] == NULL)error((char *)"Object % does not exist", n);
                if (struct3d[n]->xmin != 32767)error((char *)"Object % is not hidden", n);
                display3d(n, struct3d[n]->current.x, struct3d[n]->current.y, struct3d[n]->current.z, 1, struct3d[n]->nonormals);
            }
            return;
        }
        else if ((p = checkstring(cmdline, (unsigned char *)"WRITE"))) {
            getargs(&p, 9, (unsigned char *)",");
            if (argc < 7)error((char *)"Argument count");
            int n = (int)getint(argv[0], 1, MAX3D);
            int x = (int)getint(argv[2], -32766, 32766);
            int y = (int)getint(argv[4], -32766, 32766);
            int z = (int)getinteger(argv[6]);
            int nonormals = 0;
            if (argc == 9)nonormals = (int)getint(argv[8], 0, 1);
            if (struct3d[n] == NULL)error((char *)"Object % does not exist", n);
            if (camera[struct3d[n]->camera].viewplane == -32767)error((char *)"Camera position not defined");
            display3d(n, x, y, z, 0, nonormals);
            return;
        }
        else if ((p = checkstring(cmdline, (unsigned char *)"CLOSE ALL"))) {
            closeall3d();
            return;
        }
        else if ((p = checkstring(cmdline, (unsigned char *)"CLOSE"))) {
            int i, n;
            getargs(&p, (MAX_ARG_COUNT * 2) - 1, (unsigned char *)",");				// getargs macro must be the first executable stmt in a block
            if ((argc & 0x01 || argc < 3) == 0) error((char *)"Argument count");
            for (i = 0; i < argc; i += 2) {
                n = (int)getint(argv[i], 1, MAX3D);
                if (struct3d[n] == NULL)error((char *)"Object % does not exist", n);
                if (struct3d[n]->xmin != 32767)DrawRectangle(struct3d[n]->xmin, struct3d[n]->ymin, struct3d[n]->xmax, struct3d[n]->ymax, 0);
                Free3DMemory(n);
            }
            return;
        }
        else if ((p = checkstring(cmdline, (unsigned char *)"CAMERA"))) {
            getargs(&p, 11, (unsigned char *)",");
            if (argc < 3)error((char *)"Argument count");
            int n = (int)getint(argv[0], 1, MAXCAM);
            camera[n].viewplane = (FLOAT3D)getnumber(argv[2]);
            camera[n].x = (FLOAT3D)0;
            camera[n].y = (FLOAT3D)0;
            camera[n].panx = (FLOAT3D)0;
            camera[n].pany = (FLOAT3D)0;
            camera[n].z = 0.0;
            if (argc >= 5 && *argv[4])	camera[n].x = (FLOAT3D)getnumber(argv[4]);
            if (camera[n].x > 32766 || camera[n].x < -32766)error((char *)"Valid is -32766 to 32766");
            if (argc >= 7 && *argv[6])	camera[n].y = (FLOAT3D)getnumber(argv[6]);
            if (camera[n].y > 32766 || camera[n].x < -32766)error((char *)"Valid is -32766 to 32766");
            if (argc >= 9 && *argv[8])	camera[n].panx = (FLOAT3D)getint(argv[8], -32766 - (int64_t)camera[n].x, 32766 - (int64_t)camera[n].x);
            if (argc == 11)camera[n].pany = (FLOAT3D)getint(argv[10], -32766 - (int64_t)camera[n].y, 32766 - (int64_t)camera[n].y);
            return;
        }
        else {
            error((char *)"Syntax");
        }
    }
    void fun_3D(void) {
        unsigned char* p;
        if ((p = checkstring(ep, (unsigned char *)"XMIN"))) {
            getargs(&p, 1, (unsigned char *)",");
            int n = (int)getint(argv[0], 1, MAX3D);
            if (struct3d[n] == NULL)error((char *)"Object does not exist");
            fret = struct3d[n]->xmin;
        }
        else if ((p = checkstring(ep, (unsigned char *)"XMAX"))) {
            getargs(&p, 1, (unsigned char *)",");
            int n = (int)getint(argv[0], 1, MAX3D);
            if (struct3d[n] == NULL)error((char *)"Object does not exist");
            fret = struct3d[n]->xmax;
        }
        else if ((p = checkstring(ep, (unsigned char *)"YMIN"))) {
            getargs(&p, 1, (unsigned char *)",");
            int n = (int)getint(argv[0], 1, MAX3D);
            if (struct3d[n] == NULL)error((char *)"Object does not exist");
            fret = struct3d[n]->ymin;
        }
        else if ((p = checkstring(ep, (unsigned char *)"YMAX"))) {
            getargs(&p, 1, (unsigned char *)",");
            int n = (int)getint(argv[0], 1, MAX3D);
            if (struct3d[n] == NULL)error((char *)"Object does not exist");
            fret = struct3d[n]->ymax;
        }
        else if ((p = checkstring(ep, (unsigned char *)"X"))) {
            getargs(&p, 1, (unsigned char *)",");
            int n = (int)getint(argv[0], 1, MAX3D);
            if (struct3d[n] == NULL)error((char *)"Object does not exist");
            fret = struct3d[n]->current.x;
        }
        else if ((p = checkstring(ep, (unsigned char *)"Y"))) {
            getargs(&p, 1, (unsigned char *)",");
            int n = (int)getint(argv[0], 1, MAX3D);
            if (struct3d[n] == NULL)error((char *)"Object does not exist");
            fret = struct3d[n]->current.y;
        }
        else if ((p = checkstring(ep, (unsigned char *)"DISTANCE"))) {
            getargs(&p, 1, (unsigned char *)",");
            int n = (int)getint(argv[0], 1, MAX3D);
            if (struct3d[n] == NULL)error((char *)"Object does not exist");
            fret = struct3d[n]->distance;
        }
        else if ((p = checkstring(ep, (unsigned char *)"Z"))) {
            getargs(&p, 1, (unsigned char *)",");
            int n = (int)getint(argv[0], 1, MAX3D);
            if (struct3d[n] == NULL)error((char *)"Object does not exist");
            fret = struct3d[n]->current.z;
        }
        else error((char *)"Syntax");
        targ = T_NBR;
    }
