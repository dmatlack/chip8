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

extern uint8_t key[KEY_SIZE];
extern uint8_t gfx[GFX_ROWS][GFX_COLS];
extern bool chip8_draw_flag;

void gfx_setup() {
    memset(screen, BLACK, sizeof(unsigned char) * SCREEN_ROWS * SCREEN_COLS * 3);
    glClear(GL_COLOR_BUFFER_BIT);
}

void keypress(unsigned char k, int x, int y) {
    printf("Key Press: %c, %d, %d\n", k, x, y);
    switch (k) {
        case '1': key[0x1] = 1; break;
        case '2': key[0x2] = 1; break;
        case '3': key[0x3] = 1; break;
        case '4': key[0xC] = 1; break;
        case 'q': key[0x4] = 1; break;
        case 'w': key[0x5] = 1; break;
        case 'e': key[0x6] = 1; break;
        case 'r': key[0xD] = 1; break;
        case 'a': key[0x7] = 1; break;
        case 's': key[0x8] = 1; break;
        case 'd': key[0x9] = 1; break;
        case 'f': key[0xE] = 1; break;
        case 'z': key[0xA] = 1; break;
        case 'x': key[0x0] = 1; break;
        case 'c': key[0xB] = 1; break;
        case 'v': key[0xF] = 1; break;
    }
}

inline void paint_pixel(int row, int col, unsigned char color) {
    row = SCREEN_ROWS - 1 - row;
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
    int row, col;

    chip8_emulatecycle();

    //memset(gfx, 0, sizeof(uint8_t)  * GFX_SIZE);

    if (chip8_draw_flag) {
        // Clear framebuffer
        glClear(GL_COLOR_BUFFER_BIT);
     
        // Draw pixels to the buffer
        for (row = 0; row < GFX_ROWS; row++) {
            for (col = 0; col < GFX_COLS; col++) {
                paint_cell(row, col, gfx[row][col] ? WHITE : BLACK);
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
    (void) w; (void) h;
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
    glutKeyboardFunc(keypress);

    gfx_setup();
 
    // Run the emulator
    glutMainLoop();  

    return 0;
}
