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

    // Save screen chars/colors for the 5 rows of the top big-text and
    // the 5 rows of the bottom big-text so we can restore them afterwards.
    unsigned char saved_chars[10][40];
    unsigned char saved_colors[10][40];
    int rows[10];
    for (int i = 0; i < 5; i++) rows[i] = Y1 + i;
    for (int i = 0; i < 5; i++) rows[5 + i] = Y2 + i;

    for (int i = 0; i < 10; i++) {
        int r = rows[i];
        if (r < 0 || r >= 25) {
            for (int c = 0; c < 40; c++) { saved_chars[i][c] = ' '; saved_colors[i][c] = VCOL_WHITE; }
        } else {
            int off = r * 40;
            for (int c = 0; c < 40; c++) {
                saved_chars[i][c] = Screen[off + c];
                saved_colors[i][c] = Color[off + c];
            }
        }
    }

    draw_big_text_at(X1, Y1, top, VCOL_WHITE, spacing);
    draw_big_text_at(X2, Y2, bot, VCOL_WHITE, spacing);

    // Wait 2 Seconds (keep audio alive)
    wait_frames(120);   // 120 frames at 60fps = 2 seconds

    // Restore saved screen chars/colors
    for (int i = 0; i < 10; i++) {
        int r = rows[i];
        if (r < 0 || r >= 25) continue;
        int off = r * 40;
        for (int c = 0; c < 40; c++) {
            Screen[off + c] = saved_chars[i][c];
            Color[off + c] = saved_colors[i][c];
        }
    }

}