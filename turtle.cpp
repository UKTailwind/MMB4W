/*
 * turtle.c
 *
 *  Created on: 14 Mar 2019
 *      Author: Peter
 */
 /*
     turtle.c

     Simple array-based turtle graphics engine in C. Exports to BMP files.

     Author: Mike Lam, James Madison University, August 2015

     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.

     Known issues:

         - "filled" polygons are not always entirely filled, especially when
           side angles are very acute; there is probably an incorrect floating-
           point or integer conversion somewhere

 */


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "olcPixelGameEngine.h"
#include "MainThread.h"
#include "MMBasic_Includes.h"


 /**  DEFINITIONS  **/

#define ABS(X) ((X)>0 ? (X) : (-(X)))

#define PI 3.141592653589793


// pixel data (red, green, blue triplet)
typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char trans;
} rgb_t;


/**  GLOBAL TURTLE STATE  **/

typedef struct {
    MMFLOAT  xpos;       // current position and heading
    MMFLOAT  ypos;       // (uses floating-point numbers for
    MMFLOAT  heading;    //  increased accuracy)

    rgb_t  pen_color;   // current pen color
    rgb_t  fill_color;  // current fill color
    bool   pendown;     // currently drawing?
    bool   filled;      // currently filling?
} turtle_t;

turtle_t main_turtle;
turtle_t backup_turtle;


int    main_field_width = 0;           // size in pixels
int    main_field_height = 0;

// beginning of video
int    main_turtle_poly_vertex_count = 0;       // polygon vertex count
MMFLOAT* main_turtle_polyX = NULL; // polygon vertex x-coords
MMFLOAT* main_turtle_polyY = NULL; // polygon vertex y-coords
int turtle_init_not_done = 1;

/**  TURTLE FUNCTIONS  **/

void turtle_init(void)
{
    int maxH = PageTable[WritePage].ymax;
    int maxW = PageTable[WritePage].xmax;
    FreeMemorySafe((void**)&main_turtle_polyX);
    FreeMemorySafe((void**)&main_turtle_polyY);
    main_turtle_polyX = (MMFLOAT*)GetMemory(MAX_POLYGON_VERTICES * sizeof(MMFLOAT));
    main_turtle_polyY = (MMFLOAT*)GetMemory(MAX_POLYGON_VERTICES * sizeof(MMFLOAT));
    // save field size for later
    main_field_width = maxW;
    main_field_height = maxH;

    // reset turtle position and color
    turtle_reset();
    turtle_init_not_done = 0;
}

void turtle_reset()
{
    int maxH = PageTable[WritePage].ymax;
    int maxW = PageTable[WritePage].xmax;
    // move turtle to middle of the field
    main_turtle.xpos = maxW / 2;
    main_turtle.ypos = maxH / 2;

    // orient to the right (0 deg)
    main_turtle.heading = 270.0;

    // default draw color is white
    main_turtle.pen_color.red = 255;
    main_turtle.pen_color.green = 255;
    main_turtle.pen_color.blue = 255;
    main_turtle.pen_color.trans = 15;

    // default fill color is green
    main_turtle.fill_color.red = 0;
    main_turtle.fill_color.green = 255;
    main_turtle.fill_color.blue = 0;
    main_turtle.fill_color.trans = 15;

    // default pen position is down
    main_turtle.pendown = true;

    // default fill status is off
    main_turtle.filled = false;
    main_turtle_poly_vertex_count = 0;
}

void turtle_backup() {
    backup_turtle = main_turtle;
}

void turtle_restore() {
    main_turtle = backup_turtle;
}

void turtle_forward(int pixels)
{
    // calculate (x,y) movement vector from heading
    MMFLOAT radians = main_turtle.heading * (MMFLOAT)PI / (MMFLOAT)180.0;
    MMFLOAT dx = cos(radians) * pixels;
    MMFLOAT dy = sin(radians) * pixels;

    // delegate to another method to actually move
    turtle_goto_real(main_turtle.xpos + dx, main_turtle.ypos + dy);
}

void turtle_backward(int pixels)
{
    // opposite of "forward"
    turtle_forward(-pixels);
}

void turtle_strafe_left(int pixels) {
    turtle_turn_left((MMFLOAT)90);
    turtle_forward(pixels);
    turtle_turn_right((MMFLOAT)90);
}

void turtle_strafe_right(int pixels) {
    turtle_turn_right((MMFLOAT)90);
    turtle_forward(pixels);
    turtle_turn_left((MMFLOAT)90);
}

void turtle_turn_left(MMFLOAT angle)
{
    // rotate turtle heading
    main_turtle.heading += angle;

    // constrain heading to range: [0.0, 360.0)
    if (main_turtle.heading < (MMFLOAT)0.0) {
        main_turtle.heading += (MMFLOAT)360.0;
    }
    else if (main_turtle.heading >= (MMFLOAT)360.0) {
        main_turtle.heading -= (MMFLOAT)360.0;
    }
}

void turtle_turn_right(MMFLOAT angle)
{
    // opposite of "turn left"
    turtle_turn_left(-angle);
}

void turtle_pen_up()
{
    main_turtle.pendown = false;
}

void turtle_pen_down()
{
    main_turtle.pendown = true;
}

void turtle_begin_fill()
{
    main_turtle.filled = true;
    main_turtle_poly_vertex_count = 0;
}

void turtle_end_fill(int count)
{
    // based on public-domain fill algorithm in C by Darel Rex Finley, 2007
    //   from http://alienryderflex.com/polygon_fill/

    MMFLOAT* nodeX = (MMFLOAT *)GetTempMemory(count * sizeof(MMFLOAT));     // x-coords of polygon intercepts
    int nodes;                              // size of nodeX
    int y, i, j;                         // current pixel and loop indices
    MMFLOAT temp;                            // temporary variable for sorting
    int f = (main_turtle.fill_color.red << 16) | (main_turtle.fill_color.green << 8) | main_turtle.fill_color.blue;
    int c = (main_turtle.pen_color.red << 16) | (main_turtle.pen_color.green << 8) | main_turtle.pen_color.blue;
    int xstart, xend;
    //  loop through the rows of the image
    for (y = 0; y < main_field_height; y++) {
        //  build a list of polygon intercepts on the current line
        nodes = 0;
        j = main_turtle_poly_vertex_count - 1;
        for (i = 0; i < main_turtle_poly_vertex_count; i++) {
            if ((main_turtle_polyY[i] < (MMFLOAT)y &&
                main_turtle_polyY[j] >= (MMFLOAT)y) ||
                (main_turtle_polyY[j] < (MMFLOAT)y &&
                    main_turtle_polyY[i] >= (MMFLOAT)y)) {

                // intercept found; record it
                nodeX[nodes++] = (main_turtle_polyX[i] +
                    ((MMFLOAT)y - main_turtle_polyY[i]) /
                    (main_turtle_polyY[j] - main_turtle_polyY[i]) *
                    (main_turtle_polyX[j] - main_turtle_polyX[i]));
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

    main_turtle.filled = false;

    // redraw polygon (filling is imperfect and can occasionally occlude sides)
    for (i = 0; i < main_turtle_poly_vertex_count; i++) {
        int x0 = (int)round(main_turtle_polyX[i]);
        int y0 = (int)round(main_turtle_polyY[i]);
        int x1 = (int)round(main_turtle_polyX[(i + 1) %
            main_turtle_poly_vertex_count]);
        int y1 = (int)round(main_turtle_polyY[(i + 1) %
            main_turtle_poly_vertex_count]);
        DrawLine(x0, y0, x1, y1, 1, c);
    }
}

void turtle_goto(int x, int y)
{
    turtle_goto_real((MMFLOAT)x, (MMFLOAT)y);
}

void turtle_goto_real(MMFLOAT x, MMFLOAT y)
{
    // draw line if pen is down
    if (main_turtle.pendown) {
        turtle_draw_line((int)round(main_turtle.xpos),
            (int)round(main_turtle.ypos),
            (int)round(x),
            (int)round(y));
    }

    // change current turtle position
    main_turtle.xpos = (MMFLOAT)x;
    main_turtle.ypos = (MMFLOAT)y;

    // track coordinates for filling
    if (main_turtle.filled && main_turtle.pendown &&
        main_turtle_poly_vertex_count < MAX_POLYGON_VERTICES) {
        main_turtle_polyX[main_turtle_poly_vertex_count] = x;
        main_turtle_polyY[main_turtle_poly_vertex_count] = y;
        main_turtle_poly_vertex_count++;
    }
}

void turtle_set_heading(MMFLOAT angle)
{
    main_turtle.heading = angle;
}

void turtle_set_pen_color(int red, int green, int blue, int trans)
{
    main_turtle.pen_color.red = red;
    main_turtle.pen_color.green = green;
    main_turtle.pen_color.blue = blue;
    main_turtle.pen_color.trans = trans;
}

void turtle_set_fill_color(int red, int green, int blue, int trans)
{
    main_turtle.fill_color.red = red;
    main_turtle.fill_color.green = green;
    main_turtle.fill_color.blue = blue;
    main_turtle.fill_color.trans = trans;
}

void turtle_dot()
{
    // draw a pixel at the current location, regardless of pen status
    turtle_draw_pixel((int)round(main_turtle.xpos),
        (int)round(main_turtle.ypos));
}


void turtle_draw_pixel(int x, int y)
{
    int c = (main_turtle.pen_color.trans << 24) | (main_turtle.pen_color.red << 16) | (main_turtle.pen_color.green << 8) | main_turtle.pen_color.blue;
    DrawPixel(x, y, c);
}

void turtle_fill_pixel(int x, int y)
{
    // calculate pixel offset in image data array
    int c = (main_turtle.fill_color.trans << 24) | (main_turtle.fill_color.red << 16) | (main_turtle.fill_color.green << 8) | main_turtle.fill_color.blue;
    DrawPixel(x, y, c);
}

void turtle_draw_line(int x0, int y0, int x1, int y1)
{
    // uses a variant of Bresenham's line algorithm:
    //   https://en.wikipedia.org/wiki/Talk:Bresenham%27s_line_algorithm

    int absX = ABS(x1 - x0);          // absolute value of coordinate distances
    int absY = ABS(y1 - y0);
    int offX = x0 < x1 ? 1 : -1;      // line-drawing direction offsets
    int offY = y0 < y1 ? 1 : -1;
    int x = x0;                     // incremental location
    int y = y0;
    int err;

    turtle_draw_pixel(x, y);
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
            turtle_draw_pixel(x, y);
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
            turtle_draw_pixel(x, y);
        }
    }
}

void turtle_draw_circle(int x0, int y0, int radius)
{
    // implementation based on midpoint circle algorithm:
    //   https://en.wikipedia.org/wiki/Midpoint_circle_algorithm

    int x = radius;
    int y = 0;
    int switch_criteria = 1 - x;

    if (main_turtle.filled) {
        turtle_fill_circle(x0, y0, radius);
    }

    while (x >= y) {
        turtle_draw_pixel(x + x0, y + y0);
        turtle_draw_pixel(y + x0, x + y0);
        turtle_draw_pixel(-x + x0, y + y0);
        turtle_draw_pixel(-y + x0, x + y0);
        turtle_draw_pixel(-x + x0, -y + y0);
        turtle_draw_pixel(-y + x0, -x + y0);
        turtle_draw_pixel(x + x0, -y + y0);
        turtle_draw_pixel(y + x0, -x + y0);
        y++;
        if (switch_criteria <= 0) {
            switch_criteria += 2 * y + 1;       // no x-coordinate change
        }
        else {
            x--;
            switch_criteria += 2 * (y - x) + 1;
        }
    }
}

void turtle_fill_circle(int x0, int y0, int radius) {

    int rad_sq = radius * radius;

    // Naive algorithm, pretty ugly due to no antialiasing:
    for (int x = x0 - radius; x < x0 + radius; x++) {
        for (int y = y0 - radius; y < y0 + radius; y++) {
            int dx = x - x0;
            int dy = y - y0;
            int dsq = (dx * dx) + (dy * dy);
            if (dsq < rad_sq) turtle_fill_pixel(x, y);
        }
    }
}

void turtle_fill_circle_here(int radius)
{
    turtle_fill_circle((int)main_turtle.xpos, (int)main_turtle.ypos, radius);
}

void turtle_draw_turtle()
{
    // We are going to make our own backup of the turtle, since turtle_backup()
    // only gives us one level of undo.
    turtle_t original_turtle = main_turtle;

    turtle_pen_up();

    // Draw the legs
    for (int i = -1; i < 2; i += 2) {
        for (int j = -1; j < 2; j += 2) {
            turtle_backup();
            turtle_forward(i * 7);
            turtle_strafe_left(j * 7);

            turtle_set_fill_color(
                main_turtle.pen_color.red,
                main_turtle.pen_color.green,
                main_turtle.pen_color.blue,
                main_turtle.pen_color.trans
            );
            turtle_fill_circle_here(5);

            turtle_set_fill_color(
                original_turtle.fill_color.red,
                original_turtle.fill_color.green,
                original_turtle.fill_color.blue,
                original_turtle.fill_color.trans
            );
            turtle_fill_circle_here(3);
            turtle_restore();
        }
    }

    // Draw the head
    turtle_backup();
    turtle_forward(10);
    turtle_set_fill_color(
        main_turtle.pen_color.red,
        main_turtle.pen_color.green,
        main_turtle.pen_color.blue,
        main_turtle.pen_color.trans
    );
    turtle_fill_circle_here(5);

    turtle_set_fill_color(
        original_turtle.fill_color.red,
        original_turtle.fill_color.green,
        original_turtle.fill_color.blue,
        original_turtle.fill_color.trans
    );
    turtle_fill_circle_here(3);
    turtle_restore();

    // Draw the body
    for (int i = 9; i >= 0; i -= 4) {
        turtle_backup();
        turtle_set_fill_color(
            main_turtle.pen_color.red,
            main_turtle.pen_color.green,
            main_turtle.pen_color.blue,
            main_turtle.pen_color.trans
        );
        turtle_fill_circle_here(i + 2);

        turtle_set_fill_color(
            original_turtle.fill_color.red,
            original_turtle.fill_color.green,
            original_turtle.fill_color.blue,
            original_turtle.fill_color.trans
        );
        turtle_fill_circle_here(i);
        turtle_restore();
    }

    // Restore the original turtle position:
    main_turtle = original_turtle;
}


MMFLOAT turtle_get_x()
{
    return main_turtle.xpos;
}

MMFLOAT turtle_get_y()
{
    return main_turtle.ypos;
}

const int TURTLE_DIGITS[10][20] = {

    {0,1,1,0,       // 0
     1,0,0,1,
     1,0,0,1,
     1,0,0,1,
     0,1,1,0},

    {0,1,1,0,       // 1
     0,0,1,0,
     0,0,1,0,
     0,0,1,0,
     0,1,1,1},

    {1,1,1,0,       // 2
     0,0,0,1,
     0,1,1,0,
     1,0,0,0,
     1,1,1,1},

    {1,1,1,0,       // 3
     0,0,0,1,
     0,1,1,0,
     0,0,0,1,
     1,1,1,0},

    {0,1,0,1,       // 4
     0,1,0,1,
     0,1,1,1,
     0,0,0,1,
     0,0,0,1},

    {1,1,1,1,       // 5
     1,0,0,0,
     1,1,1,0,
     0,0,0,1,
     1,1,1,0},

    {0,1,1,0,       // 6
     1,0,0,0,
     1,1,1,0,
     1,0,0,1,
     0,1,1,0},

    {1,1,1,1,       // 7
     0,0,0,1,
     0,0,1,0,
     0,1,0,0,
     0,1,0,0},

    {0,1,1,0,       // 8
     1,0,0,1,
     0,1,1,0,
     1,0,0,1,
     0,1,1,0},

    {0,1,1,0,       // 9
     1,0,0,1,
     0,1,1,1,
     0,0,0,1,
     0,1,1,0},

};

void turtle_draw_int(int value)
{
    // calculate number of digits to draw
    int ndigits = 1;
    if (value > 9) {
        ndigits = (int)(ceil(log10(value)));
    }

    // draw each digit
    for (int i = ndigits - 1; i >= 0; i--) {
        int digit = value % 10;
        for (int y = 0; y < 5; y++) {
            for (int x = 0; x < 4; x++) {
                if (TURTLE_DIGITS[digit][y * 4 + x] == 1) {
                    turtle_draw_pixel((int)main_turtle.xpos + i * 5 + x, (int)main_turtle.ypos - y);
                }
            }
        }
        value = value / 10;
    }
}

void turtle_cleanup()
{
}
MMFLOAT headingrange(MMFLOAT n) {
    while (n < 0)n += 360.0;
    while (n >= 360.0)n -= 360.0;
    return n;
}

void cmd_turtle(void) {
    unsigned char* tp;
    int maxH = PageTable[WritePage].ymax;
    int maxW = PageTable[WritePage].xmax;
    tp = checkstring(cmdline, (unsigned char *)"RESET");
    if (tp) {
        if (turtle_init_not_done)turtle_init();
        ClearScreen(gui_bcolour);
        turtle_reset();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"DRAW TURTLE");
    if (tp) {
        if (turtle_init_not_done)turtle_init();
        turtle_draw_turtle();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"PEN UP");
    if (tp) {
        if (turtle_init_not_done)turtle_init();
        turtle_pen_up();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"PEN DOWN");
    if (tp) {
        if (turtle_init_not_done)turtle_init();
        turtle_pen_down();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"DOT");
    if (tp) {
        if (turtle_init_not_done)turtle_init();
        turtle_dot();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"BEGIN FILL");
    if (tp) {
        if (turtle_init_not_done)turtle_init();
        turtle_begin_fill();
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"END FILL");
    if (tp) {
        if (turtle_init_not_done)turtle_init();
        turtle_end_fill(main_turtle_poly_vertex_count);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"FORWARD");
    if (tp) {
        int n = (int)getint(tp, (int64_t)( - sqrt(maxW * maxW + maxH * maxH)), (int64_t)(sqrt(maxW * maxW + maxH * maxH)));
        if (turtle_init_not_done)turtle_init();
        turtle_forward(n);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"BACKWARD");
    if (tp) {
        int n = (int)getint(tp, (int64_t)(-sqrt(maxW * maxW + maxH * maxH)), (int64_t)(sqrt(maxW * maxW + maxH * maxH)));
        if (turtle_init_not_done)turtle_init();
        turtle_backward(n);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"TURN LEFT");
    if (tp) {
        MMFLOAT n = getnumber(tp);
        n = headingrange(n);
        if (turtle_init_not_done)turtle_init();
        turtle_turn_right(n);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"HEADING");
    if (tp) {
        MMFLOAT n = getnumber(tp);
        n = n - 90;
        n = headingrange(n);
        if (turtle_init_not_done)turtle_init();
        turtle_set_heading(n);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"PEN COLOUR");
    if (tp) {
        int n = (int)getint(tp, 0, M_WHITE);
        if (turtle_init_not_done)turtle_init();
        turtle_set_pen_color((n >> 16) & 0xFF, (n >> 8) & 0xFF, n & 0xFF, (n >> 24) & 0xF);
        return;
    }

    tp = checkstring(cmdline, (unsigned char*)"FILL COLOUR");
    if (tp) {
        int n = (int)getint(tp, 0, M_WHITE);
        if (turtle_init_not_done)turtle_init();
        turtle_set_fill_color((n >> 16) & 0xFF, (n >> 8) & 0xFF, n & 0xFF, (n >> 24) & 0xF);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"TURN RIGHT");
    if (tp) {
        MMFLOAT n = getnumber(tp);
        n = headingrange(n);
        if (turtle_init_not_done)turtle_init();
        turtle_turn_left(n);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"MOVE");
    if (tp) {
        getargs(&tp, 3, (unsigned char*)",");                              // this is a macro and must be the first executable stmt in a block
        int x = (int)getint(argv[0], 0, maxW);
        int y = (int)getint(argv[2], 0, maxH);
        if (turtle_init_not_done)turtle_init();
        turtle_goto(x, y);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"DRAW PIXEL");
    if (tp) {
        getargs(&tp, 3, (unsigned char*)",");                              // this is a macro and must be the first executable stmt in a block
        int x = (int)getint(argv[0], 0, maxW);
        int y = (int)getint(argv[2], 0, maxH);
        if (turtle_init_not_done)turtle_init();
        turtle_draw_pixel(x, y);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"FILL PIXEL");
    if (tp) {
        getargs(&tp, 3, (unsigned char*)",");                              // this is a macro and must be the first executable stmt in a block
        int x = (int)getint(argv[0], 0, maxW);
        int y = (int)getint(argv[2], 0, maxH);
        if (turtle_init_not_done)turtle_init();
        turtle_fill_pixel(x, y);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"DRAW LINE");
    if (tp) {
        getargs(&tp, 7, (unsigned char*)",");                              // this is a macro and must be the first executable stmt in a block
        int x0 = (int)getint(argv[0], 0, maxW);
        int y0 = (int)getint(argv[2], 0, maxH);
        int x1 = (int)getint(argv[4], 0, maxW);
        int y1 = (int)getint(argv[6], 0, maxH);
        if (turtle_init_not_done)turtle_init();
        turtle_draw_line(x0, y0, x1, y1);
        return;
    }
    tp = checkstring(cmdline, (unsigned char*)"DRAW CIRCLE");
    if (tp) {
        getargs(&tp, 5, (unsigned char*)",");                              // this is a macro and must be the first executable stmt in a block
        int x = (int)getint(argv[0], 0, maxW);
        int y = (int)getint(argv[2], 0, maxH);
        int r = (int)getint(argv[4], 0, std::max(maxW, maxH));
        if (turtle_init_not_done)turtle_init();
        turtle_draw_circle(x, y, r);
        return;
    }
    error((char*)"Syntax");

}

