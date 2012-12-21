/**
 * @file chip8.c
 *
 * @author David Matlack
 */
#include "chip8.h"

#define unknown_opcode(op) fprintf(stderr, "Unknown opcode: 0x%x\n", op)
#define IS_BIT_SET(byte, bit) (((0x80 >> (bit)) & (byte)) != 0x0)
#define GFX_INDEX(row, col) ((row) + (col)*GFX_COLS)

unsigned char chip8_fontset[80] = 
{ 
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F 
};

/* private */
static uint16_t opcode;
static uint8_t  memory[MEM_SIZE];
static uint8_t  V[16];
static uint16_t I;
static uint16_t PC;
static uint8_t  gfx[GFX_SIZE];
static uint8_t  delay_timer;
static uint8_t  sound_timer;
static uint16_t stack[STACK_SIZE];
static uint16_t SP;
static uint8_t  key[KEY_SIZE];

/* public */
bool chip8_draw_flag;

static inline uint8_t randbyte() { return (uint8_t) rand(); }

void chip8_initialize() {
    int i;

    PC     = 0x200;
    opcode = 0;
    I      = 0;
    SP     = 0;

    memset(memory, 0, MEM_SIZE);
    memset(V,      0, 16);
    memset(gfx,    0, GFX_SIZE);
    memset(stack,  0, STACK_SIZE);

    for (i = 0; i < 80; i++) {
        memory[0x50 + i] = chip8_fontset[i];
    }
    
    chip8_draw_flag   = false;    
    delay_timer = 0;
    sound_timer = 0;
    srand(time(NULL));
}

void chip8_loadgame(char *game) {
    FILE *fgame;

    fgame = fopen(game, "rb");
    
    if (NULL == fgame) {
        fprintf(stderr, "Unable to open game: %s\n", game);
        exit(42);
    }

    fread(&memory[0x200], 1, MAX_GAME_SIZE, fgame);
    
    fclose(fgame);
}

void chip8_emulatecycle() {
    int i, j;
    uint8_t x, y, n;
    uint8_t kk, byte;
    uint16_t nnn;

    // fetch
    opcode = memory[PC] << 8 | memory[PC + 1];
    x   = (opcode >> 8) & 0x000F;
    y   = (opcode >> 4) & 0x000F;
    n   = (opcode >> 0) & 0x000F;
    kk  = (opcode >> 0) & 0x00FF;
    nnn = (opcode >> 0) & 0x0FFF; 

    // decode & execute
    switch (opcode & 0xF000) {

        case 0x0000:
            switch (opcode) {
                case 0x00E0: // clear the screen
                    memset(gfx, 0, GFX_SIZE);
                    PC += 2;
                    break;
                case 0x00EE: // ret
                    PC = stack[SP];
                    --SP;
                    break;
            }
            break;
        case 0x1000: // 1nnn: jump to address nnn
            PC = nnn;
            break;
        case 0x2000: // 2nnn: call address nnn
            ++SP;
            stack[SP] = PC;
            PC = nnn;
            break;
        case 0x3000: // 3xkk: skip next instr if V[x] = kk
            PC += (V[x] == kk) ? 4 : 2;
            break;
        case 0x4000: // 4xkk: skip next instr if V[x] != kk
            PC += (V[x] != kk) ? 4 : 2;
            break;
        case 0x5000: // 5xy0: skip next instr if V[x] == V[y]
            PC += (V[x] == V[y]);
            break;
        case 0x6000: // 6xkk: set V[x] = kk
            V[x] = kk;
            PC += 2;
            break;
        case 0x7000: // 7xkk: set V[x] = V[x] + kk
            V[x] += kk;
            PC += 2;
            break;
        case 0x8000: // 8xyn: Arithmetic stuff
            switch (n) {
                case 0x0: 
                    V[x] = V[y];
                    break;
                case 0x1:
                    V[x] = V[x] | V[y];
                    break;
                case 0x2:
                    V[x] = V[x] & V[y];
                    break;
                case 0x3:
                    V[x] = V[x] ^ V[y];
                    break;
                case 0x4:
                    V[x] = V[x] + V[y];
                    V[0xF] = ((V[x] + V[y]) > 255) ? 1 : 0;
                    break;
                case 0x5: 
                    V[x] = V[x] - V[y];
                    V[0xF] = (V[x] > V[y]) ? 1 : 0;
                    break;
                case 0x6:
                    V[x] = V[x] / 2;
                    V[0xF] = (V[x] % 2 == 1) ? 1 : 0;
                    break;
                case 0x7:
                    V[x] = V[y] - V[x];
                    V[0xF] = (V[y] > V[x]) ? 1 : 0;
                    break;
                case 0xE:
                    V[x] *= 2;
                    V[0xF] = ((V[x] >> 7) % 2 == 1) ? 1 : 0;
                    break;
                default:
                    unknown_opcode(opcode);
            }
            PC += 2;
            break;
        case 0x9000: // 9xy0: skip instruction if Vx != Vy
            switch (n) {
                case 0x0:
                    PC += (V[x] != V[y]) ? 4 : 2;
                    break;
                default:
                    unknown_opcode(opcode);
            }
            break;
        case 0xA000: // Annn: set I to address nnn
            I = opcode & 0xFFF;
            PC += 2;
            break;
        case 0xB000: // Bnnn: jump to location nnn + V[0]
            PC = nnn + V[0];
            break;
        case 0xC000: // Cxkk: V[x] = random byte AND kk
            V[x] = randbyte() & kk;
            PC += 2;
            break;
        case 0xD000: // Dxyn: Display an n-byte sprite starting at memory
                     // location I at (Vx, Vy) on the screen, VF = collision
            V[0xF] = 0;
            for (i = 0; i < n; i++) {
                byte = memory[I + i];
                for (j = 0; j < 8; j++) {
                    if (IS_BIT_SET(byte, j)) {
                        if (gfx[GFX_INDEX(x + j, y + i)] == 0) {
                            V[0xF] = 1;
                        }
                        gfx[GFX_INDEX(x + j, y + i)] ^= 1;
                    }     
                }
            }
            PC += 2;
            chip8_draw_flag = true;
            break;
        case 0xE000: // key-pressed events
            switch (kk) {
                case 0x9E: // skip next instr if key[Vx] is pressed
                    PC += (key[V[x]]) ? 4 : 2;
                    break;
                case 0xA1: // skip next instr if key[Vx] is not pressed
                    PC += (!key[V[x]]) ? 4 : 2;
                    break;
                default:
                    unknown_opcode(opcode);
            }
            break;
        case 0xF000: // misc
            switch (kk) {
                case 0x07:
                    V[x] = delay_timer;
                    PC += 2;
                    break;
                case 0x0A:
                    if (key[V[x]]) PC += 2;
                    break;
                case 0x15:
                    delay_timer = V[x];
                    PC += 2;
                    break;
                case 0x18:
                    sound_timer = V[x];
                    PC += 2;
                    break;
                case 0x1E:
                    I = I + V[x];
                    PC += 2;
                    break;
                case 0x29:
                    I = memory[0x50 + V[x]]; // FIXME maybe?
                    PC += 2;
                    break;
                case 0x33:
                    memory[I]   = (x % 1000) / 100;
                    memory[I+1] = (x % 100) / 10;
                    memory[I+2] = (x % 10);
                    PC += 2;
                    break;
                case 0x55:
                    for (i = 0; i < x; i++) { memory[I + i] = V[i]; }
                    PC += 2;
                    break;
                case 0x65:
                    for (i = 0; i < x; i++) { V[i] = memory[I + i]; }
                    PC += 2;
                    break;
                default:
                    unknown_opcode(opcode);
            }
        default:
            unknown_opcode(opcode);
    }

    // update timers
    if (delay_timer > 0) {
        --delay_timer;
    }
    if (sound_timer > 0) {
        --sound_timer;
        if (sound_timer == 0) {
            printf("BEEP!\n");
        }
    }

}
