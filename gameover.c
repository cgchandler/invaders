// © 2026 Christopher G Chandler
// Licensed under the MIT License. See LICENSE file in the project root.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <conio.h> 
#include <c64/vic.h>
#include "bigfont.h"
#include <string.h>

// Simple VBlank wait (assuming standard 60fps NTSC) - file-local
static void wait_frames(int frames) {
    for (int i = 0; i < frames; i++) {
        vic_waitFrame();
        sound_update(); // Keep sound engine running during waits
    }
}

void game_over_sequence(void) {

    // Display centered big text for "GAME" and "OVER"
    const char* top = "GAME";
    const char* bot = "OVER";
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

    draw_big_text_at(X1, Y1, top, VCOL_WHITE, spacing);
    draw_big_text_at(X2, Y2, bot, VCOL_WHITE, spacing);

    // Wait 3 Seconds
    wait_frames(180);   // 180 frames at 60fps = 3 seconds

    // 5. Cleanup / Restart
    // Just return. The main loop should handle resetting variables 
    // when this function finishes.
}