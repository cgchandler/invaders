// © 2026 Christopher G Chandler
// Licensed under the MIT License. See LICENSE file in the project root.

#include "bases.h"
#include <string.h>
#include <c64/vic.h>

/* Initial character layout for a single base (5x2)
   Top row: 176,177,177,177,178
   Bottom: 192,193,193,193,194
*/
/* Repeat the middle (solid block) character so bases are 4 chars wide */
static const unsigned char base_top_chars[BASE_WIDTH] = {176, 177, 177, 177, 178};
static const unsigned char base_bottom_chars[BASE_WIDTH] = {192, 193, 193, 193, 194};

/* Damage stages per character: 0 = new, 1 = slightly damaged, 2 = more damaged.
    When stage == BASE_DAMAGE_STAGES the cell is destroyed/empty.
    The stage character tables are laid out [stage][col] so you can later
    replace these values with your damaged character codes when you design them.
*/
static unsigned char base_top_stage_chars[BASE_DAMAGE_STAGES][BASE_WIDTH];
static unsigned char base_bottom_stage_chars[BASE_DAMAGE_STAGES][BASE_WIDTH];

/* Current damage stage for each base cell (0..BASE_DAMAGE_STAGES). */
static unsigned char base_stage[BASE_COUNT][2][BASE_WIDTH];

/* Explicit start columns for the four bases (0-based) as requested */
static const unsigned char base_starts[BASE_COUNT] = {4, 13, 22, 31};
static unsigned char base_start_col(unsigned idx) {
    if (idx >= BASE_COUNT) return 0;
    return base_starts[idx];
}

void bases_init(void) {
    /* Initialize stage character tables.
       Stage 0 = original (new)
       Stage 1 = slightly damaged (user-provided)
       Stage 2 = more damaged (user-provided)

       Note: base width is 5; repeat the middle damaged codes so the
       pattern aligns with the base shape.
    */
    /* Stage 0 - original */
    for (int c = 0; c < BASE_WIDTH; c++) {
        base_top_stage_chars[0][c] = base_top_chars[c];
        base_bottom_stage_chars[0][c] = base_bottom_chars[c];
    }

    /* Stage 1 - slightly damaged: TOP: 179,180,180,181 -> expand to 5 cols */
    base_top_stage_chars[1][0] = 179;
    base_top_stage_chars[1][1] = 180;
    base_top_stage_chars[1][2] = 180;
    base_top_stage_chars[1][3] = 180;
    base_top_stage_chars[1][4] = 181;

    base_bottom_stage_chars[1][0] = 195;
    base_bottom_stage_chars[1][1] = 196;
    base_bottom_stage_chars[1][2] = 196;
    base_bottom_stage_chars[1][3] = 196;
    base_bottom_stage_chars[1][4] = 197;

    /* Stage 2 - more damaged: TOP: 182,183,183,184 -> expand to 5 cols */
    base_top_stage_chars[2][0] = 182;
    base_top_stage_chars[2][1] = 183;
    base_top_stage_chars[2][2] = 183;
    base_top_stage_chars[2][3] = 183;
    base_top_stage_chars[2][4] = 184;

    base_bottom_stage_chars[2][0] = 198;
    base_bottom_stage_chars[2][1] = 199;
    base_bottom_stage_chars[2][2] = 199;
    base_bottom_stage_chars[2][3] = 199;
    base_bottom_stage_chars[2][4] = 200;

    /* Mark all cells at stage 0 (new) and draw to screen once */
    for (unsigned b = 0; b < BASE_COUNT; b++) {
        unsigned char sc = base_start_col(b);
        for (unsigned r = 0; r < 2; r++) {
            for (unsigned c = 0; c < BASE_WIDTH; c++) {
                base_stage[b][r][c] = 0;
                unsigned short row = (r == 0) ? BASE_TOP_ROW : BASE_BOTTOM_ROW;
                unsigned short offset = (row * 40) + sc + c;
                if (offset < 1000) {
                    Screen[offset] = (r == 0) ? base_top_stage_chars[0][c] : base_bottom_stage_chars[0][c];
                    Color[offset]  = VCOL_GREEN;
                }
            }
        }
    }
}

void bases_render(void) {
    /* Re-draw bases each frame to ensure they overwrite starfield artifacts */
    /* Only redraw live base characters so we don't repeatedly write spaces
       (destruction clears the cell once in bases_check_hit). */
    for (unsigned b = 0; b < BASE_COUNT; b++) {
        unsigned char sc = base_start_col(b);
        for (unsigned c = 0; c < BASE_WIDTH; c++) {
            unsigned short top_off = (BASE_TOP_ROW * 40) + sc + c;
            unsigned short bot_off = (BASE_BOTTOM_ROW * 40) + sc + c;
            /* Top */
            if (base_stage[b][0][c] < BASE_DAMAGE_STAGES) {
                unsigned char s = base_stage[b][0][c];
                if (top_off < 1000) {
                    Screen[top_off] = base_top_stage_chars[s][c];
                    Color[top_off]  = VCOL_GREEN;
                }
            }
            /* Bottom */
            if (base_stage[b][1][c] < BASE_DAMAGE_STAGES) {
                unsigned char s = base_stage[b][1][c];
                if (bot_off < 1000) {
                    Screen[bot_off] = base_bottom_stage_chars[s][c];
                    Color[bot_off]  = VCOL_GREEN;
                }
            }
        }
    }
}

int bases_check_hit(unsigned char col, unsigned char row, bool destroy_on_hit) {
    /* Quick bounds check for the rows we care about */

    if (row != BASE_TOP_ROW && row != BASE_BOTTOM_ROW) return 0;

    /* Quick bounding check: bases occupy cols 4..35 inclusive */
    if (col < 4 || col >= 36) return 0; /* outside base region */

    /* Determine base index by matching against explicit starts */
    unsigned idx = 0xFF;
    for (unsigned i = 0; i < BASE_COUNT; i++) {
        unsigned sc = base_start_col(i);
        if (col >= sc && col < sc + BASE_WIDTH) { idx = i; break; }
    }
    if (idx == 0xFF) return 0;

    unsigned sc = base_start_col(idx);
    unsigned local_c = col - sc;

    unsigned row_idx = (row == BASE_TOP_ROW) ? 0 : 1;
    unsigned char cur_stage = base_stage[idx][row_idx][local_c];
    if (cur_stage >= BASE_DAMAGE_STAGES) return 0; /* already destroyed */

    /* Increment damage stage */
    if (destroy_on_hit) {
        cur_stage = BASE_DAMAGE_STAGES;
    }
    else {
        cur_stage++;
    }
    base_stage[idx][row_idx][local_c] = cur_stage;

    unsigned short offset = (row * 40) + sc + local_c;
    if (cur_stage >= BASE_DAMAGE_STAGES) {
        /* Now destroyed: clear cell once */
        if (offset < 1000) {
            Screen[offset] = 32;
            Color[offset]  = VCOL_BLACK;
        }
    } else {
        /* Update the visible damage stage character */
        if (offset < 1000) {
            if (row_idx == 0) {
                Screen[offset] = base_top_stage_chars[cur_stage][local_c];
            } else {
                Screen[offset] = base_bottom_stage_chars[cur_stage][local_c];
            }
            Color[offset] = VCOL_GREEN;
        }
    }
    return 1;
}
