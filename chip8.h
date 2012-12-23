/**
 * @file chip8.h
 *
 * @author David Matlack
 */
#ifndef __CHIP8_H__
#define __CHIP8_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define true  1
#define false 0
#define bool  int

#define MEM_SIZE 4096
#define GFX_ROWS 32
#define GFX_COLS 64
#define GFX_SIZE 2048
#define STACK_SIZE 16
#define KEY_SIZE 16

#define GFX_INDEX(row, col) ((row) + (col)*GFX_COLS)

#define MAX_GAME_SIZE (0xFFF - 0x200)

extern int chip8_draw_flag;

uint8_t *chip8_get_gfx();

void chip8_initialize();
void chip8_loadgame(char *game);
void chip8_emulatecycle();
void chip8_setkeys();

#endif /* __CHIP8_H__ */
