/***********************************************************************************************************************
MMBasic for Windows

Draw.h

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
// format:
//      void cmd_???(void)
//      void fun_???(void)
//      void op_???(void)
void cmd_text(void);
void cmd_pixel(void);
void cmd_circle(void);
void cmd_line(void);
void cmd_box(void);
void cmd_rbox(void);
void cmd_arc(void);
void cmd_triangle(void);
void cmd_blit(void);
void fun_pixel(void);
void cmd_cls(void);
void cmd_font(void);
void cmd_colour(void);
void cmd_polygon(void);
void cmd_gui(void);
void cmd_mode(void);
void cmd_page(void);
void cmd_framebuffer(void);
void cmd_image(void);
void cmd_3D(void);
void cmd_turtle(void);

void fun_rgb(void);
void fun_mmhres(void);
void fun_mmvres(void);
void fun_mmcharwidth(void);
void fun_mmcharheight(void);
void fun_at(void);
void fun_pixel(void);
void fun_3D(void);

#endif




/**********************************************************************************
 All command tokens tokens (eg, PRINT, FOR, etc) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_COMMAND_TABLE
// the format is:
//    TEXT      	TYPE                P  FUNCTION TO CALL
// where type is always T_CMD
// and P is the precedence (which is only used for operators and not commands)
//{ (unsigned char*)"Text", T_CMD, 0, cmd_text	},
//
#ifdef PICOMITEVGA
//{ (unsigned char*)"GUI",            T_CMD,                      0, cmd_guiMX170 },
#else
//{ (unsigned char*)"GUI",            T_CMD,                      0, cmd_gui },
#endif
{ (unsigned char*)"Pixel",          T_CMD,                      0, cmd_pixel },
{ (unsigned char*)"CLS",			T_CMD,						0, cmd_cls },
{ (unsigned char*)"Circle",         T_CMD,                      0, cmd_circle },
{ (unsigned char*)"Line",           T_CMD,                      0, cmd_line },
{ (unsigned char*)"Colour",         T_CMD,                      0, cmd_colour },
{ (unsigned char*)"Box",            T_CMD,                      0, cmd_box },
{ (unsigned char*)"RBox",           T_CMD,                      0, cmd_rbox },
{ (unsigned char*)"Mode",           T_CMD,                      0, cmd_mode },
{ (unsigned char*)"Triangle",       T_CMD,                      0, cmd_triangle },
{ (unsigned char*)"Arc",            T_CMD,                      0, cmd_arc },
{ (unsigned char*)"Polygon",        T_CMD,                  	0, cmd_polygon },
//{ (unsigned char*)"Blit",           T_CMD,                      0, cmd_blit },
{ (unsigned char*)"Text",			T_CMD,						0, cmd_text }, 
{ (unsigned char*)"Page",			T_CMD,						0, cmd_page },
{ (unsigned char*)"FrameBuffer",	T_CMD,						0, cmd_framebuffer },
{ (unsigned char*)"Image",	 		T_CMD,						0, cmd_image },
{ (unsigned char*)"Draw3D",         T_CMD,                      0, cmd_3D },
{ (unsigned char*)"Turtle",			T_CMD,						0, cmd_turtle },
#endif


/**********************************************************************************
 All other tokens (keywords, functions, operators) should be inserted in this table
**********************************************************************************/
#ifdef INCLUDE_TOKEN_TABLE
//{ (unsigned char*)"@(",				T_FUN | T_STR,		0, fun_at },
// the format is:
//    TEXT      	TYPE                P  FUNCTION TO CALL
// where type is T_NA, T_FUN, T_FNA or T_OPER augmented by the types T_STR and/or T_NBR
// and P is the precedence (which is only used for operators)
	{ (unsigned char*)"RGB(",           	T_FUN | T_INT,		0, fun_rgb },
	{ (unsigned char*)"Pixel(",           	T_FUN | T_INT,		0, fun_pixel },
	{ (unsigned char*)"MM.HRes",	    	T_FNA | T_INT,		0, fun_mmhres },
	{ (unsigned char*)"MM.VRes",	    	T_FNA | T_INT,		0, fun_mmvres },
	{ (unsigned char*)"@(",					T_FUN | T_STR,		0, fun_at },
	{ (unsigned char*)"DRAW3D(",	    T_FUN | T_INT,		0, fun_3D, },
//	{ (unsigned char *)"MM.FontWidth",   T_FNA | T_INT,		0, fun_mmcharwidth 	},
//	{ (unsigned char *)"MM.FontHeight",  T_FNA | T_INT,		0, fun_mmcharheight },

#endif
#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
#ifndef DRAW_H_INCL
#define FONT_BUILTIN_NBR     8
#define FONT_TABLE_SIZE      16
#define JUSTIFY_LEFT        0
#define JUSTIFY_CENTER      1
#define JUSTIFY_RIGHT       2

#define JUSTIFY_TOP         0
#define JUSTIFY_MIDDLE      1
#define JUSTIFY_BOTTOM      2

#define ORIENT_NORMAL       0
#define ORIENT_VERT         1
#define ORIENT_INVERTED     2
#define ORIENT_CCW90DEG     3
#define ORIENT_CW90DEG      4
#define M_CLEAR                RGB(0,    0,       0)
#define M_WHITE               (RGB(255,  255,   255) | 0xFF000000)
#define M_YELLOW              (RGB(255,  255,     0) | 0xFF000000)
#define M_LILAC               (RGB(255,  128,   255) | 0xFF000000)
#define M_BROWN               (RGB(255,  128,     0) | 0xFF000000)
#define M_FUCHSIA             (RGB(255,  64,    255) | 0xFF000000)
#define M_RUST                (RGB(255,  64,      0) | 0xFF000000)
#define M_MAGENTA             (RGB(255,  0,     255) | 0xFF000000)
#define M_RED                 (RGB(255,  0,       0) | 0xFF000000)
#define M_CYAN                (RGB(0,    255,   255) | 0xFF000000)
#define M_GREEN               (RGB(0,    255,     0) | 0xFF000000)
#define M_CERULEAN            (RGB(0,    128,   255) | 0xFF000000)
#define M_MIDGREEN            (RGB(0,    128,     0) | 0xFF000000)
#define M_COBALT              (RGB(0,    64,    255) | 0xFF000000)
#define M_MYRTLE              (RGB(0,    64,      0) | 0xFF000000)
#define M_BLUE                (RGB(0,    0,     255) | 0xFF000000)
#define M_BLACK               (RGB(0,    0,       0) | 0xFF000000)
#define M_BROWN               (RGB(255,  128,     0) | 0xFF000000)
#define M_GRAY                (RGB(128,  128,   128) | 0xFF000000)
#define M_LITEGRAY            (RGB(210,  210,   210) | 0xFF000000)
#define M_ORANGE              (RGB(0xff, 0xA5,	  0) | 0xFF000000)
#define M_PINK				  (RGB(0xFF, 0xA0, 0xAB) | 0xFF000000)
#define M_GOLD				  (RGB(0xFF, 0xD7, 0x00) | 0xFF000000)
#define M_SALMON			  (RGB(0xFA, 0x80, 0x72) | 0xFF000000)
#define M_BEIGE				  (RGB(0xF5, 0xF5, 0xDC) | 0xFF000000)
#define M_BLANK				  (RGB(0,	 0,	   0))
#define DrawPixel(a,b,c) DrawRectangle(a,b,a,b,c);
extern "C" void DrawRectangle(int x1, int y1, int x2, int y2, int64_t c);
extern "C" void DrawBitmap(int x1, int y1, int width, int height, int scale, int64_t fc, int64_t bc, unsigned char* bitmap);
extern "C" void GUIPrintString(int x, int y, int fnt, int jh, int jv, int jo, int64_t fc, int64_t bc, unsigned char* str);
extern "C" void SetFont(int fnt);
extern "C" void DrawCircle(int x, int y, int radius, int w, int64_t c, int64_t fill, MMFLOAT aspect);
extern "C" void DisplayPutS(unsigned char* s);
extern "C" void DisplayPutC(unsigned char c);
extern "C" void ShowMMBasicCursor(int show);
extern "C" void DrawLine(int x1, int y1, int x2, int y2, int w, int64_t c);
extern "C" void ScrollLCD(int lines);
extern "C" void DrawBox(int x1, int y1, int x2, int y2, int w, int64_t c, int64_t fill);
extern "C" void ClearScreen(int64_t c);
extern "C" void ReadBuffer(int x1, int y1, int x2, int y2, uint32_t * c);
extern "C" void DrawBuffer(int x1, int y1, int x2, int y2, uint32_t * p);
extern "C" void DrawBuffer32(int x1, int y1, int x2, int y2, char * p, int skip);
extern "C" int  GetJustification(char* p, int* jh, int* jv, int* jo);
extern "C" void DrawRBox(int x1, int y1, int x2, int y2, int radius, int64_t c, int64_t fill);
extern "C" void DrawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int64_t c, int64_t fill);
extern "C" uint32_t * getpageaddress(int pageno);
extern "C" int64_t getColour(unsigned char* c, int minus);
extern "C" void MoveBufferNormal(int x1, int y1, int x2, int y2, int w, int h, int flip);
extern "C" void ScrollBufferV(int lines, int blank);
extern "C" void ScrollBufferH(int pixels);
extern "C" void closeall3d(void);
extern "C" void setmode(int mode, int argb, bool fullscreen);
extern int CurrentX, CurrentY;
extern int64_t gui_fcolour , gui_bcolour ;
extern int gui_font_width, gui_font_height;
extern int PrintPixelMode;
extern int gui_font;
extern  volatile unsigned char* FontTable[];
extern const int xres[] ;
extern const int yres[] ;
extern const int pixeldensity[] ;
extern const int defaultfont[];
struct s_pagetable {
	uint32_t* address;	//points to the framebuffer for the page
	uint16_t xmax;		//store width
	uint16_t ymax;		//store height
	uint32_t size;		//size of frame
};
extern struct s_pagetable PageTable[];
typedef struct SVD {
	FLOAT3D x;
	FLOAT3D y;
	FLOAT3D z;
}s_vector;
typedef struct t_quaternion {
	FLOAT3D w;
	FLOAT3D x;
	FLOAT3D y;
	FLOAT3D z;
	FLOAT3D m;
}s_quaternion;

struct D3D {
	s_quaternion* q_vertices;//array of original vertices
	s_quaternion* r_vertices; //array of rotated vertices
	s_quaternion* q_centroids;//array of original vertices
	s_quaternion* r_centroids; //array of rotated vertices
	s_vector* normals;
	uint8_t* facecount; //number of vertices for each face
	uint16_t* facestart; //index into the face_x_vert table of the start of a given face
	int64_t* fill; //fill colours
	int64_t* line; //line colours
	int64_t* colours;
	uint16_t* face_x_vert; //list of vertices for each face
	uint8_t* flags;
	FLOAT3D* dots;
	FLOAT3D* depth;
	FLOAT3D distance;
	FLOAT3D ambient;
	int* depthindex;
	s_vector light;
	s_vector current;
	short tot_face_x_vert;
	short xmin, xmax, ymin, ymax;
	uint16_t nv;	//number of vertices to describe the object
	uint16_t nf; // number of faces in the object
	uint8_t dummy;
	uint8_t vmax;  // maximum verticies for any face on the object
	uint8_t camera; // camera to use for the object
	uint8_t nonormals;
};
typedef struct {
	FLOAT3D x;
	FLOAT3D y;
	FLOAT3D z;
	FLOAT3D viewplane;
	FLOAT3D panx;
	FLOAT3D pany;
}s_camera;
extern struct D3D* struct3d[MAX3D + 1];
extern s_camera camera[MAXCAM + 1];

#endif
#endif
