/**
 * @file chip8.c
 *
 * @author David Matlack
 */
#include <chip8.h>

uint16_t opcode;
uint8_t  memory[MEM_SIZE];
uint8_t  V[16];
uint16_t I;
uint16_t PC;
uint8_t  gfx[GFX_SIZE];
uint8_t  delay_timer;
uint8_t  sound_timer;

