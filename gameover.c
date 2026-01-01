#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <conio.h> 
#include <c64/vic.h>

// -- CONFIGURATION --
#define BLOCK_CHAR    159   // Solid character in invaders custom charset
#define TEXT_COLOR    VCOL_WHITE
#define SCREEN_RAM    0x4400
#define COLOR_RAM     0xD800
#define SCREEN_W      40

enum GameOverCharacters { CHAR_G, CHAR_A, CHAR_M, CHAR_E, CHAR_O, CHAR_V, CHAR_R };

// Simple 5x5 font for G, A, M, E, O, V, R
// 1 = draw block, 0 = empty space
const uint8_t BIG_FONT[][25] = {
    { // G
        1,1,1,1,1,
        1,0,0,0,0,
        1,0,0,1,1,
        1,0,0,0,1,
        1,1,1,1,1
    },
    { // A
        0,1,1,1,0,
        1,0,0,0,1,
        1,1,1,1,1,
        1,0,0,0,1,
        1,0,0,0,1
    },
    { // M
        1,0,0,0,1,
        1,1,0,1,1,
        1,0,1,0,1,
        1,0,0,0,1,
        1,0,0,0,1
    },
    { // E
        1,1,1,1,1,
        1,0,0,0,0,
        1,1,1,1,0,
        1,0,0,0,0,
        1,1,1,1,1
    },
    { // O
        1,1,1,1,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,1,1,1,1
    },
    { // V
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        0,1,0,1,0,
        0,0,1,0,0
    },
    { // R
        1,1,1,1,1,
        1,0,0,0,1,
        1,1,1,1,0,
        1,0,0,1,0,
        1,0,0,0,1
    }
};

// Helper to draw a single big character
// x, y: Top-left coordinate of the letter
// char_idx: Index in BIG_FONT array (0=G, 1=A, etc.)
void draw_big_char(int x, int y, int char_idx) {
    uint8_t* screen = (uint8_t*)SCREEN_RAM;
    uint8_t* color  = (uint8_t*)COLOR_RAM;
    
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            if (BIG_FONT[char_idx][(row * 5) + col]) {
                int offset = (y + row) * SCREEN_W + (x + col);
                screen[offset] = BLOCK_CHAR;
                color[offset]  = TEXT_COLOR;
            }
        }
    }
}

// Simple VBlank wait (assuming standard 60fps NTSC)
void wait_frames(int frames) {
    for (int i = 0; i < frames; i++) {
        vic_waitFrame();
    }
}

void game_over_sequence(void) {

    // Display "GAME"
    // Each letter is 5 wide + 1 pixel space = 6 width. 
    // Total width = 4 * 6 = 24.
    // Start X = (40 - 24) / 2 = 8.
    // Row 6 seems like a good vertical spot.
    int X = 8;
    int Y1 = 6;
    int Y2 = 14;
    
    draw_big_char(X, Y1, CHAR_G);
    draw_big_char(X + 6, Y1, CHAR_A);
    draw_big_char(X + 12, Y1, CHAR_M);
    draw_big_char(X + 18, Y1, CHAR_E);

    // 3. Display "OVER"
    // Same horizontal centering. Push down to Row 14.
    draw_big_char(X, Y2, CHAR_O);
    draw_big_char(X + 6, Y2, CHAR_V);
    draw_big_char(X + 12, Y2, CHAR_E);
    draw_big_char(X + 18, Y2, CHAR_R);

    // 4. Wait 3 Seconds
    wait_frames(180);

    // 5. Cleanup / Restart
    // Just return. The main loop should handle resetting variables 
    // when this function finishes.
}