#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <conio.h> 
#include <c64/vic.h>
#include "bigfont.h"
#include "config.h"
#include <string.h>
#include "game.h"
#include "sounds.h"

// Simple VBlank wait (assuming standard 60fps NTSC) - file-local
static void wait_frames(int frames) {
    for (int i = 0; i < frames; i++) {
        vic_waitFrame();
        sound_update(); /* keep sound engine running during blocking wait */
    }
}

void level_display_sequence(void) {

    // Retrieve current level
    game_state* gs = game_get_state();
    unsigned char tens = gs->level / 10;
    unsigned char ones = gs->level % 10;

    // Display centered big text for "LEVEL" and the current level number
    const char* top = "LEVEL";
    // Convert level number to string
    char bot_str[3];    
    bot_str[0] = tens + '0';
    bot_str[1] = ones + '0';
    bot_str[2] = '\0';
    const char* bot = bot_str;

    int Y1 = 5;
    int Y2 = 13;
    int letter_w = 4; // corresponds to BIG_WIDTH in bigfont.c
    int spacing = 1;  // spacing used when drawing

    int len_top = (int)strlen(top);
    int total_top = len_top * letter_w + (len_top - 1) * spacing;
    int X1 = (40 - total_top) / 2;

    int len_bot = (int)strlen(bot);
    int total_bot = len_bot * letter_w + (len_bot - 1) * spacing;
    int X2 = (40 - total_bot) / 2;

    // Clamp X positions to valid text columns (0..39)
    if (X1 < 0) X1 = 0;
    if (X1 > 39) X1 = 39;
    if (X2 < 0) X2 = 0;
    if (X2 > 39) X2 = 39;

    // Save screen chars/colors for the 5 rows of the top big-text and
    // the 5 rows of the bottom big-text so we can restore them afterwards.
    unsigned char saved_chars[10][40];
    unsigned char saved_colors[10][40];

    // Save top block (Y1 .. Y1+4)
    for (int i = 0; i < 5; i++) {
        int r = Y1 + i;
        if (r < 0 || r >= 25) {
            for (int c = 0; c < 40; c++) { saved_chars[i][c] = (unsigned char)' '; saved_colors[i][c] = (unsigned char)VCOL_WHITE; }
        } else {
            int off = r * 40;
            for (int c = 0; c < 40; c++) {
                saved_chars[i][c] = Screen[off + c];
                saved_colors[i][c] = Color[off + c];
            }
        }
    }

    // Save bottom block (Y2 .. Y2+4)
    for (int i = 0; i < 5; i++) {
        int r = Y2 + i;
        int idx = 5 + i;
        if (r < 0 || r >= 25) {
            for (int c = 0; c < 40; c++) { saved_chars[idx][c] = (unsigned char)' '; saved_colors[idx][c] = (unsigned char)VCOL_WHITE; }
        } else {
            int off = r * 40;
            for (int c = 0; c < 40; c++) {
                saved_chars[idx][c] = Screen[off + c];
                saved_colors[idx][c] = Color[off + c];
            }
        }
    }

    draw_big_text_at(X1, Y1, top, VCOL_WHITE, spacing);
    draw_big_text_at(X2, Y2, bot, VCOL_WHITE, spacing);

    // Wait 2 Seconds (keep audio alive)
    wait_frames(120);   // 120 frames at 60fps = 2 seconds

    // Restore saved screen chars/colors (top block)
    for (int i = 0; i < 5; i++) {
        int r = Y1 + i;
        if (r < 0 || r >= 25) continue;
        int off = r * 40;
        for (int c = 0; c < 40; c++) {
            Screen[off + c] = saved_chars[i][c];
            Color[off + c] = saved_colors[i][c];
        }
    }

    // Restore saved screen chars/colors (bottom block)
    for (int i = 0; i < 5; i++) {
        int r = Y2 + i;
        int idx = 5 + i;
        if (r < 0 || r >= 25) continue;
        int off = r * 40;
        for (int c = 0; c < 40; c++) {
            Screen[off + c] = saved_chars[idx][c];
            Color[off + c] = saved_colors[idx][c];
        }
    }

}