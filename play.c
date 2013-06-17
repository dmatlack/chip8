/**
 * @file play.c
 *
 * @author David Matlack
 */
#include "chip8.h"

#include <GL/gl.h>
#include <GL/glut.h>

#define PIXEL_SIZE 5

#define BLACK 0
#define WHITE 255

#define SCREEN_ROWS (GFX_ROWS * PIXEL_SIZE)
#define SCREEN_COLS (GFX_COLS * PIXEL_SIZE)
unsigned char screen[SCREEN_ROWS][SCREEN_COLS][3];

extern uint8_t gfx[GFX_SIZE];
extern bool chip8_draw_flag;

void gfx_setup() {
    memset(screen, BLACK, sizeof(unsigned char) * SCREEN_ROWS * SCREEN_COLS * 3);
}

inline void paint_pixel(int row, int col, unsigned char color) {
    screen[row][col][0] = screen[row][col][1] = screen[row][col][2] = color;
}

void paint_cell(int row, int col, unsigned char color) {
    int pixel_row = row * PIXEL_SIZE;
    int pixel_col = col * PIXEL_SIZE;
    int drow, dcol;

    for (drow = 0; drow < PIXEL_SIZE; drow++) {
        for (dcol = 0; dcol < PIXEL_SIZE; dcol++) {
            paint_pixel(pixel_row + drow, pixel_col + dcol, color);
        }
    }
}

void loop() {
    int x, y;

    chip8_emulatecycle();

    if (true || chip8_draw_flag) {
        // Clear framebuffer
        glClear(GL_COLOR_BUFFER_BIT);
     
        // Draw pixels to the buffer
        for (y = 0; y < GFX_ROWS; y++) {
            for (x = 0; x < GFX_COLS; x++) {
                paint_cell(y, x, gfx[GFX_INDEX(y,x)] ? WHITE : BLACK);
            }
        }

        // Update Texture
        glDrawPixels(SCREEN_COLS, SCREEN_ROWS, GL_RGB, GL_UNSIGNED_BYTE, 
                     (void *) screen);
        glutSwapBuffers(); 
    
        chip8_draw_flag = false;
    }
}

void reshape_window(GLsizei w, GLsizei h) {
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./play <game>\n");
        exit(2);
    }

    // Setup Chip8
    chip8_initialize();
    chip8_loadgame(argv[1]);

    // Setup OpenGL
    glutInit(&argc, argv);          
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
 
    glutInitWindowSize(SCREEN_COLS, SCREEN_ROWS);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("chip8");
 
    glutDisplayFunc(loop);
    glutIdleFunc(loop);
    glutReshapeFunc(reshape_window);
 
    // Run the emulator
    glutMainLoop();  

    return 0;
}
