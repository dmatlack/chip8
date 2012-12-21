/**
 * @file play.c
 *
 * @author David Matlack
 */
#include "chip8.h"
#include <OpenGL/gl.h>
#include <GLUT/glut.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./play <game>\n");
    }

    //setup graphics
    //setup input

    chip8_initialize();
    chip8_loadgame(argv[1]);

    // emulation loop
    for (;;) {
        chip8_emulatecycle();

        if (chip8_draw_flag) {
            //draw graphics
        }

        //chip8_setkeys();
    }
}
