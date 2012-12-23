/**
 * @file play.c
 *
 * @author David Matlack
 */
#include "chip8.h"

#include <OpenGL/gl.h>
#include <GLUT/glut.h>

#define PIXEL_SIZE 5

#define BLACK 0
#define WHITE 255

int width  = (GFX_COLS * PIXEL_SIZE);
int height = (GFX_ROWS * PIXEL_SIZE);
uint8_t screen[GFX_ROWS][GFX_COLS][3];

uint8_t *chip8_gfx;
extern bool chip8_draw_bit;

void gfx_setup() {
    memset(screen, BLACK, sizeof(uint8_t) * width * height * 3);
    
    // Create a texture 
    glTexImage2D(GL_TEXTURE_2D, 0, 3, GFX_COLS, GFX_ROWS, 0, 
                 GL_RGB, GL_UNSIGNED_BYTE, (void *) screen);
 
    // Set up the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); 
 
    // Enable textures
    glEnable(GL_TEXTURE_2D);
}

void loop() {
    int x, y;

    chip8_emulatecycle();
    
    if (chip8_draw_flag) {
        // Clear framebuffer
        glClear(GL_COLOR_BUFFER_BIT);
     
        // Draw pixels to the buffer
        for (y = 0; y < GFX_ROWS; y++) {
            for (x = 0; x < GFX_COLS; x++) {
                screen[y][x][0] = screen[y][x][1] = screen[y][x][2] = 
                    (chip8_gfx[GFX_INDEX(y,x)]) ? WHITE : BLACK;
            }
        }

        // Update Texture
        glTexSubImage2D(GL_TEXTURE_2D, 0 ,0, 0, GFX_COLS, GFX_ROWS, GL_RGB, 
                        GL_UNSIGNED_BYTE, (void *) screen);
        glBegin(GL_QUADS);
            glTexCoord2d(0.0, 0.0); glVertex2d(0.0,   0.0);
            glTexCoord2d(1.0, 0.0); glVertex2d(width, 0.0);
            glTexCoord2d(1.0, 1.0); glVertex2d(width, height);
            glTexCoord2d(0.0, 1.0); glVertex2d(0.0,   height);
        glEnd();

        glutSwapBuffers();  
    
        chip8_draw_flag = false;
    }
}

void reshape_window(GLsizei w, GLsizei h) {
    glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);        
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);
 
    // Resize quad
    width = w;
    height = h;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./play <game>\n");
    }

    // Setup Chip8
    chip8_initialize();
    chip8_loadgame(argv[1]);
    chip8_gfx = chip8_get_gfx();

    // Setup OpenGL
    glutInit(&argc, argv);          
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
 
    glutInitWindowSize(width, height);
    glutInitWindowPosition(320, 320);
    glutCreateWindow("chip8");
 
    glutDisplayFunc(loop);
    glutIdleFunc(loop);
    glutReshapeFunc(reshape_window);
 
    // Run the emulator
    glutMainLoop();  

    return 0;
}
