// Standalone preview compatibility entry point.
// The upstream GPL turtle engine remains in the source archive but is not linked.
#include "MainThread.h"
#include "MMBasic_Includes.h"

int turtle_init_not_done = 1;
int main_turtle_poly_vertex_count = 0;
MMFLOAT* main_turtle_polyX = nullptr;
MMFLOAT* main_turtle_polyY = nullptr;
int main_field_width = 0;
int main_field_height = 0;

void cmd_turtle(void) {
    error((char*)"TURTLE is unavailable in this standalone preview");
}
