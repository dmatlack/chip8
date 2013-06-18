/**
 * @file chip8.c
 *
 * @author David Matlack
 */
#include "chip8.h"

#define DEBUG

#define unknown_opcode(op) \
    do { \
        fprintf(stderr, "Unknown opcode: 0x%x\n", op); \
        fprintf(stderr, "kk: 0x%02x\n", kk); \
        exit(42); \
    } while (0)

#define IS_BIT_SET(byte, bit) (((0x80 >> (bit)) & (byte)) != 0x0)

#define FONTSET_ADDRESS 0x50
#define FONTSET_BYTES_PER_CHAR 5
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

uint16_t opcode;
uint8_t  memory[MEM_SIZE];
uint8_t  V[16];
uint16_t I;
uint16_t PC;
uint8_t  gfx[GFX_ROWS][GFX_COLS];
uint8_t  delay_timer;
uint8_t  sound_timer;
uint16_t stack[STACK_SIZE];
uint16_t SP;
uint8_t  key[KEY_SIZE];
bool     chip8_draw_flag;

static inline uint8_t randbyte() { return (rand() % 256); }

void draw_sprite(uint8_t x, uint8_t y, uint8_t n) {
    unsigned row = y, col = x;
    unsigned byte_index;
    unsigned bit_index;

    // set the collision flag to 0
    V[0xF] = 0;
    for (byte_index = 0; byte_index < n; byte_index++) {
        uint8_t byte = memory[I + byte_index];

        for (bit_index = 0; bit_index < 8; bit_index++) {
            // the value of the bit in the sprite
            uint8_t bit = (byte >> bit_index) & 0x1;
            // the value of the current pixel on the screen
            uint8_t *pixelp = &gfx[(row + byte_index) % GFX_ROWS]
                                  [(col + (7 - bit_index)) % GFX_COLS];

            // if drawing to the screen would cause any pixel to be erased,
            // set the collision flag to 1
            if (bit == 1 && *pixelp ==1) V[0xF] = 1;

            // draw this pixel by XOR
            *pixelp = *pixelp ^ bit;
        }
    }
}

static void debug_draw() {
    int x, y;

    for (y = 0; y < GFX_ROWS; y++) {
        for (x = 0; x < GFX_COLS; x++) {
            if (gfx[y][x] == 0) printf("0");
            else printf(" ");
        }
        printf("\n");
    }
    printf("\n");
}

static void print_state() {
    printf("------------------------------------------------------------------\n");
    printf("\n");

    printf("V0: 0x%02x  V4: 0x%02x  V8: 0x%02x  VC: 0x%02x\n", 
           V[0], V[4], V[8], V[12]);
    printf("V1: 0x%02x  V5: 0x%02x  V9: 0x%02x  VD: 0x%02x\n",
           V[1], V[5], V[9], V[13]);
    printf("V2: 0x%02x  V6: 0x%02x  VA: 0x%02x  VE: 0x%02x\n",
           V[2], V[6], V[10], V[14]);
    printf("V3: 0x%02x  V7: 0x%02x  VB: 0x%02x  VF: 0x%02x\n",
           V[3], V[7], V[11], V[15]);

    printf("\n");
    printf("PC: 0x%04x\n", PC);
    printf("\n");
    printf("\n");
}

void chip8_initialize() {
    int i;

    PC     = 0x200;
    opcode = 0;
    I      = 0;
    SP     = 0;

    memset(memory, 0, sizeof(uint8_t)  * MEM_SIZE);
    memset(V,      0, sizeof(uint8_t)  * 16);
    memset(gfx,    0, sizeof(uint8_t)  * GFX_SIZE);
    memset(stack,  0, sizeof(uint16_t) * STACK_SIZE);
    memset(key,    0, sizeof(uint8_t)  * KEY_SIZE);

    for (i = 0; i < 80; i++) {
        memory[FONTSET_ADDRESS + i] = chip8_fontset[i];
    }
    
    chip8_draw_flag = true; 
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
    int i;
    uint8_t x, y, n;
    uint8_t kk;
    uint16_t nnn;

    // fetch
    opcode = memory[PC] << 8 | memory[PC + 1];
    x   = (opcode >> 8) & 0x000F; // the lower 4 bits of the high byte
    y   = (opcode >> 4) & 0x000F; // the upper 4 bits of the low byte
    n   = (opcode >> 0) & 0x000F; // the lowest 4 bits
    kk  = (opcode >> 0) & 0x00FF; // the lowest 8 bits
    nnn = (opcode >> 0) & 0x0FFF; // the lowest 12 bits

#ifdef DEBUG 
    printf("PC: 0x%04x Op: 0x%04x\n", PC, opcode);
#endif

    // decode & execute: case on the highest order byte
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (kk) {
                case 0x00E0: // clear the screen
                    memset(gfx, 0, GFX_SIZE);
                    chip8_draw_flag = true;
                    PC += 2;
                    break;
                case 0x00EE: // ret
                    PC = stack[SP--];
                    break;
                default:
                    unknown_opcode(opcode);
            }
            break;
        case 0x1000: // 1nnn: jump to address nnn
            PC = nnn;
            break;
        case 0x2000: // 2nnn: call address nnn
            stack[++SP] = PC;
            PC = nnn;
            break;
        case 0x3000: // 3xkk: skip next instr if V[x] = kk
            PC += (V[x] == kk) ? 4 : 2;
            break;
        case 0x4000: // 4xkk: skip next instr if V[x] != kk
            PC += (V[x] != kk) ? 4 : 2;
            break;
        case 0x5000: // 5xy0: skip next instr if V[x] == V[y]
            PC += (V[x] == V[y]) ? 4 : 2;
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
                    V[0xF] = ((V[x] + V[y]) > 255) ? 1 : 0;
                    V[x] = V[x] + V[y];
                    break;
                case 0x5: 
                    V[0xF] = (V[x] > V[y]) ? 1 : 0;
                    V[x] = V[x] - V[y];
                    break;
                case 0x6:
                    V[0xF] = (V[x] % 2 == 1) ? 1 : 0;
                    V[x] = (V[x] >> 1);
                    break;
                case 0x7:
                    V[0xF] = (V[y] > V[x]) ? 1 : 0;
                    V[x] = V[y] - V[x];
                    break;
                case 0xE:
                    V[0xF] = ((V[x] >> 7) % 2 == 1) ? 1 : 0;
                    V[x] = (V[x] << 1);
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
            I = nnn;
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
            draw_sprite(V[x], V[y], n);
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
                    i = 0;
                    while (true) {
                        for (i = 0; i < KEY_SIZE; i++) {
                            if (key[i]) {
                                V[x] = i;
                                goto got_key_press;
                            }
                        }
                    }
                    got_key_press:
                    PC += 2;
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
                    I = FONTSET_BYTES_PER_CHAR * V[x];
                    PC += 2;
                    break;
                case 0x33:
                    memory[I]   = (V[x] % 1000) / 100; // hundred's digit
                    memory[I+1] = (V[x] % 100) / 10;   // ten's digit
                    memory[I+2] = (V[x] % 10);         // one's digit
                    PC += 2;
                    break;
                case 0x55:
                    for (i = 0; i <= x; i++) { memory[I + i] = V[i]; }
                    I += x + 1;
                    PC += 2;
                    break;
                case 0x65:
                    for (i = 0; i <= x; i++) { V[i] = memory[I + i]; }
                    I += x + 1;
                    PC += 2;
                    break;
                default:
                    unknown_opcode(opcode);
            }
            break;
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

#ifdef DEBUG
    //if (chip8_draw_flag) debug_draw();
    //print_state();
#endif

}
