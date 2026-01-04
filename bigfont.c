#include "bigfont.h"
#include <c64/vic.h>
#include "config.h"

// Config
#define SCREEN_W 40
#define BIG_WIDTH 4     // Each big char is 4 columns wide
#define BIG_HEIGHT 5    // Each big char is 5 rows tall

#define CH_SPACE  32    // space character
#define CH_BLOCK  159   // solid block
#define CH_TL     105   // top-left corner
#define CH_TR     95    // top-right corner 
#define CH_BL     168   // bottom-left corner
#define CH_BR     169   // bottom-right corner

// BIG_FONT contains only the letters needed by the game: G A M E O V R I N D S
// Order: G,A,M,E,O,V,R,I,N,D,S
static const unsigned char BIG_FONT[][BIG_WIDTH * 5] = {
    { // G
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_SPACE,
        CH_BLOCK,CH_SPACE,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK
    },
    { // A
        CH_SPACE,CH_BR,   CH_BL,   CH_SPACE,
        CH_BR,   CH_TL,   CH_TR,   CH_BL,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK
    },
    { // M  
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BL,   CH_BR,   CH_BLOCK,
        CH_BLOCK,CH_TR,   CH_TL,   CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK
    },
    { // E
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_SPACE,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_SPACE,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_SPACE,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK
    },
    { // O
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK
    },
    { // V
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_TR,   CH_BL,   CH_BR,   CH_TL,
        CH_SPACE,CH_TR,   CH_TL,   CH_SPACE
    },
    { // R
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BL,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_TL,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BL,
        CH_BLOCK,CH_SPACE,CH_TR,   CH_BLOCK
    },
    { // I
        CH_SPACE,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_SPACE,CH_SPACE,CH_BLOCK,CH_SPACE,
        CH_SPACE,CH_SPACE,CH_BLOCK,CH_SPACE,
        CH_SPACE,CH_SPACE,CH_BLOCK,CH_SPACE,
        CH_SPACE,CH_BLOCK,CH_BLOCK,CH_BLOCK
    },
    { // N
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BL,   CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_TR,   CH_BL,   CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_TR,   CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK
    },
    { // D
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BL,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_TL
    },
    { // S
        CH_BLOCK,CH_BLOCK,CH_BLOCK, CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_SPACE,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_SPACE,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK
    }
    ,{ // L
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_SPACE,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_SPACE,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_SPACE,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_SPACE,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK
    }
    ,{ // 0
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK
    }
    ,{ // 1
        CH_SPACE,CH_BR,CH_BLOCK,CH_SPACE,
        CH_SPACE,CH_BLOCK,CH_BLOCK,CH_SPACE,
        CH_SPACE,CH_SPACE,CH_BLOCK,CH_SPACE,
        CH_SPACE,CH_SPACE,CH_BLOCK,CH_SPACE,
        CH_SPACE,CH_BLOCK,CH_BLOCK,CH_BLOCK
    }
    ,{ // 2
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_SPACE,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_SPACE,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK
    }
    ,{ // 3
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_SPACE,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_SPACE,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK
    }
    ,{ // 4
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_SPACE,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_SPACE,CH_SPACE,CH_SPACE,CH_BLOCK
    }
    ,{ // 5
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_SPACE,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_SPACE,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK
    }
    ,{ // 6
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_SPACE,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK
    }
    ,{ // 7
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_SPACE,CH_SPACE,CH_BR,   CH_BLOCK,
        CH_SPACE,CH_BR,   CH_BLOCK,CH_TL,
        CH_SPACE,CH_BLOCK,CH_TL,CH_SPACE ,
        CH_SPACE,CH_BLOCK,CH_SPACE,CH_SPACE
    }
    ,{ // 8
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK
    }
    ,{ // 9
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_BLOCK,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK,
        CH_SPACE,CH_SPACE,CH_SPACE,CH_BLOCK,
        CH_BLOCK,CH_BLOCK,CH_BLOCK,CH_BLOCK
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
        case 'L': return 11;
        case '0': return 12;
        case '1': return 13;
        case '2': return 14;
        case '3': return 15;
        case '4': return 16;
        case '5': return 17;
        case '6': return 18;
        case '7': return 19;
        case '8': return 20;
        case '9': return 21;
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
                //Screen[offset] = BLOCK_CHAR;
                Screen[offset] = BIG_FONT[idx][row * BIG_WIDTH + col];
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
