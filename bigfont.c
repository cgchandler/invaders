#include "bigfont.h"
#include <c64/vic.h>

// Config
#define BLOCK_CHAR 159
#define SCREEN_W 40
#define BIG_WIDTH 4

// BIG_FONT contains only the letters needed by the game: G A M E O V R I N D S
// Order: G,A,M,E,O,V,R,I,N,D,S
static const unsigned char BIG_FONT[][BIG_WIDTH * 5] = {
    { // G
        1,1,1,1,
        1,0,0,0,
        1,0,1,1,
        1,0,0,1,
        1,1,1,1
    },
    { // A
        0,1,1,0,
        1,0,0,1,
        1,1,1,1,
        1,0,0,1,
        1,0,0,1
    },
    { // M
        1,0,0,1,
        1,1,1,1,
        1,0,1,1,
        1,0,0,1,
        1,0,0,1
    },
    { // E
        1,1,1,1,
        1,0,0,0,
        1,1,1,0,
        1,0,0,0,
        1,1,1,1
    },
    { // O
        1,1,1,1,
        1,0,0,1,
        1,0,0,1,
        1,0,0,1,
        1,1,1,1
    },
    { // V
        1,0,0,1,
        1,0,0,1,
        1,0,0,1,
        1,0,0,1,
        0,1,1,0
    },
    { // R
        1,1,1,0,
        1,0,0,1,
        1,1,1,0,
        1,0,0,1,
        1,0,0,1
    },
    { // I
        0,1,1,1,
        0,0,1,0,
        0,0,1,0,
        0,0,1,0,
        0,1,1,1
    },
    { // N
        1,0,0,1,
        1,1,0,1,
        1,0,1,1,
        1,0,0,1,
        1,0,0,1
    },
    { // D
        1,1,1,0,
        1,0,0,1,
        1,0,0,1,
        1,0,0,1,
        1,1,1,0
    },
    { // S
        1,1,1,1,
        1,0,0,0,
        1,1,1,1,
        0,0,0,1,
        1,1,1,1
    }
};

// Map ASCII uppercase letter to index in BIG_FONT. Returns -1 if not found.
static int char_to_index(char ch) {
    switch (ch) {
        case 'G': return 0;
        case 'A': return 1;
        case 'M': return 2;
        case 'E': return 3;
        case 'O': return 4;
        case 'V': return 5;
        case 'R': return 6;
        case 'I': return 7;
        case 'N': return 8;
        case 'D': return 9;
        case 'S': return 10;
        default: return -1;
    }
}

void draw_big_char_at(int x, int y, char ch, byte color) {
    int idx = char_to_index(ch);
    if (idx < 0) return;
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < BIG_WIDTH; col++) {
            if (BIG_FONT[idx][row * BIG_WIDTH + col]) {
                int offset = (y + row) * SCREEN_W + (x + col);
                Screen[offset] = BLOCK_CHAR;
                Color[offset]  = color;
            }
        }
    }
}

void draw_big_text_at(int x, int y, const char* text, byte color, int spacing) {
    for (int i = 0; text[i]; i++) {
        char ch = text[i];
        // Only uppercase supported
        if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
        int xpos = x + i * (BIG_WIDTH + spacing);
        draw_big_char_at(xpos, y, ch, color);
    }
}
