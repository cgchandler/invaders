#include "bonus_ship.h"
#include "aliens.h"
#include <c64/vic.h>
#include <c64/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "sounds.h"

// --- CONSTANTS ---
#define BONUS_SPRITE_INDEX   7        // Reserved 8th sprite
#define BONUS_PTR_OFFSET     2        
#define EXPLOSION_PTR_OFFSET 3        

#define SCREEN_MIN_X         24
#define SCREEN_MAX_X         344      

#define BONUS_Y_POS          58     // (50 + 15*1 = 58)
//#define BONUS_Y_POS          170    // Change position to Row 15 (50 + 15*8 = 170) for testing

#define BONUS_ALIEN_MIN_ROW  4       // Minimum alien row for bonus ship to spawn

#define STATE_OFF            0
#define STATE_MOVING         1
#define STATE_EXPLODING      2
#define STATE_SHOW_SCORE     3

#define SPAWN_RATE           500      // 1 in N chance per frame to spawn   
#define MOVE_SPEED           1       // Pixels per frame
#define EXPLOSION_SPEED      20      // Frames per explosion frame    
#define SCORE_SHOW_TIME      150     // Frames to show score after explosion

#define BONUS_ACTIVE_Y_OFFSET   0   // 0 if your ship pixels start at top of sprite
#define BONUS_ACTIVE_HEIGHT     8   // ship is only 8 pixels tall

// Module state (file-local)
static bonus_ship_state s_bonus_state = { 0 };

static inline bonus_ship_state* _bstate(void) { return &s_bonus_state; }

#define BASE_SPRITE_PTR 32 

void bonus_init(void) {
    bonus_ship_state* b = _bstate();
    b->state = STATE_OFF;
    b->timer = 0;
    
    // Ensure Sprite 7 properties are reset
    vic.spr_expand_x &= ~(1 << BONUS_SPRITE_INDEX);
    vic.spr_expand_y &= ~(1 << BONUS_SPRITE_INDEX);
    vic.spr_enable   &= ~(1 << BONUS_SPRITE_INDEX);

    /* Stop any UFO siren that might still be active (safety for mode switches) */
    sfx_ufo_stop();
}

void bonus_reset(void) {
    bonus_init();
}

void bonus_update(void) {

    // Always stop UFO movement sound unless actively moving
    bonus_ship_state* b = _bstate();
    if (b->state != STATE_MOVING) {
        sfx_ufo_stop();
    }

    if (b->state == STATE_OFF) {
        aliens_state* astate = aliens_get_state();
        if (
            (astate->alive_count >= 8)      // Minimum of eight aliens
            && (astate->grid_y >= BONUS_ALIEN_MIN_ROW)    // Alien minimum row - clear space for alien
            && (rand() % SPAWN_RATE) == 0)  // Random Spawn Success
        {
            b->state = STATE_MOVING;

            sfx_ufo_start();

            if (rand() % 2 == 0) {
                b->x = SCREEN_MIN_X;
                b->dir = 1;
            } else {
                b->x = SCREEN_MAX_X;
                b->dir = -1;
            }
        }
        return;
    }

    if (b->state == STATE_MOVING) {
        b->x += (b->dir * MOVE_SPEED);

        // Bounds Check
        if ((b->dir == 1 && b->x > SCREEN_MAX_X) ||
            (b->dir == -1 && b->x < SCREEN_MIN_X)) {
            b->state = STATE_OFF;
            sfx_ufo_stop();
        }
        return;
    }

    if (b->state == STATE_EXPLODING) {
        // EXPLOSION: runs on b->timer
        b->timer++;
        if (b->timer >= EXPLOSION_SPEED) {
            b->timer = 0;
            b->anim_frame++;

            if (b->anim_frame >= 4) {
                // Explosion finished, but score may still be showing.
                // Do NOT force state change if score is still active.
                if (b->score_timer <= 0) {
                    b->state = STATE_OFF;
                    sfx_ufo_stop();
                } else {
                    // Stay in a score-showing state while timer runs
                    b->state = STATE_SHOW_SCORE;
                }
                return;
            }
        }

        // SCORE: independent timer
        if (b->score_timer > 0) {
            b->score_timer--;
            if (b->score_timer == 0) {
                Screen[b->score_grid_pos]   = ' ';
                Screen[b->score_grid_pos+1] = ' ';
                Screen[b->score_grid_pos+2] = ' ';

                // If explosion already ended, we can go OFF now
                if (b->anim_frame >= 4) {
                    b->state = STATE_OFF;
                    sfx_ufo_stop();
                }
            }
        }

        return;
    }

    if (b->state == STATE_SHOW_SCORE) {
        // Keep counting score time even after explosion has ended
        if (b->score_timer > 0) {
            b->score_timer--;
            if (b->score_timer == 0) {
                Screen[b->score_grid_pos]   = ' ';
                Screen[b->score_grid_pos+1] = ' ';
                Screen[b->score_grid_pos+2] = ' ';

                b->state = STATE_OFF;
                sfx_ufo_stop();
            }
        } else {
            // Safety
            b->state = STATE_OFF;
            sfx_ufo_stop();
        }

        return;
    }
}

void bonus_render(void) {
    bonus_ship_state* b = _bstate();
    if (b->state == STATE_OFF || b->state == STATE_SHOW_SCORE) {
        vic.spr_enable &= ~(1 << BONUS_SPRITE_INDEX);
        return;
    }

    // Enable Sprite 7
    vic.spr_enable |= (1 << BONUS_SPRITE_INDEX);
    
    vic.spr_color[BONUS_SPRITE_INDEX] = VCOL_RED;

    // Expand X only
    vic.spr_expand_x |= (1 << BONUS_SPRITE_INDEX);
    vic.spr_expand_y &= ~(1 << BONUS_SPRITE_INDEX);

    // Set Pointer: sprite pointer table lives at Screen + 0x3F8
    byte* sprite_ptrs = (byte*)(Screen + 0x3F8);
    const byte VIC_BANK_BASE_PTR = (byte)(((unsigned)Sprites - 0x4000) >> 6);

    if (b->state == STATE_MOVING) {
        sprite_ptrs[BONUS_SPRITE_INDEX] = VIC_BANK_BASE_PTR + BONUS_PTR_OFFSET; 
    } else {
        sprite_ptrs[BONUS_SPRITE_INDEX] = VIC_BANK_BASE_PTR + EXPLOSION_PTR_OFFSET + b->anim_frame;
    }

    vic.spr_pos[BONUS_SPRITE_INDEX].y = BONUS_Y_POS;
    
    // MSB Handling
    if (b->x > 255) {
        vic.spr_msbx |= (1 << BONUS_SPRITE_INDEX);
        vic.spr_pos[BONUS_SPRITE_INDEX].x = b->x & 0xFF;
    } else {
        vic.spr_msbx &= ~(1 << BONUS_SPRITE_INDEX);
        vic.spr_pos[BONUS_SPRITE_INDEX].x = b->x;
    }
}

int bonus_check_hit(int m_col, int m_row)
{
    // Match these to whatever missile.c uses for its pixel->grid conversion.
    // On a stock C64 text screen, top-left text cell begins at X=24, Y=50.

    // UFO sprite geometry (X-expanded)
    const int BONUS_SPRITE_WIDTH_PX = 48;

    bonus_ship_state* b = _bstate();
    if (b->state != STATE_MOVING) return 0;

    // X-expanded sprite: 24 px wide becomes 48 px wide.
    // b->x is the sprite's LEFT EDGE.
    int x_left  = b->x;
    int x_right = b->x + (BONUS_SPRITE_WIDTH_PX - 1); // inclusive

    // Convert sprite pixel X bounds into character columns using SAME origin as missile code.
    int col_min = (x_left  - SCREEN_LEFT_EDGE) / 8;
    int col_max = (x_right - SCREEN_LEFT_EDGE) / 8;

    // Clamp to screen
    if (col_min < 0) col_min = 0;
    if (col_max > 39) col_max = 39;

    // Collision band (only the 8-pixel art inside the 21-pixel sprite)
    int y0 = BONUS_Y_POS + BONUS_ACTIVE_Y_OFFSET;
    int y1 = y0 + (BONUS_ACTIVE_HEIGHT - 1);

    int row_min = (y0 - SCREEN_TOP_EDGE) / 8;
    int row_max = (y1 - SCREEN_TOP_EDGE) / 8;

    if (row_min < 0) row_min = 0;
    if (row_max > 24) row_max = 24;

    if (m_row < row_min || m_row > row_max) return 0;
    if (m_col < col_min || m_col > col_max) return 0;

    // HIT
    b->state = STATE_EXPLODING;
    b->anim_frame = 0;
    b->timer = 0;

    sfx_ufo_stop();
    sfx_bonus_ship_hit();

    int points = 50;
    game_state* gs = game_get_state();
    if (gs->shots_fired == 23 ||
        (gs->shots_fired > 23 && ((gs->shots_fired - 23) % 15 == 0)))
    {
        points = 300;
    }

    gs->score += points;
    b->last_score_val = points;

    // Center score text over the sprite
    int score_width = (points == 300) ? 3 : 2;

    int sprite_center_x = b->x + (BONUS_SPRITE_WIDTH_PX / 2);
    int center_col = (sprite_center_x - SCREEN_LEFT_EDGE) / 8;

    int start_col = center_col - (score_width / 2);
    if (start_col < 0) start_col = 0;
    if (start_col > (40 - score_width)) start_col = (40 - score_width);

    int score_row = (BONUS_Y_POS - SCREEN_TOP_EDGE) / 8;
    if (score_row < 0) score_row = 0;
    if (score_row > 24) score_row = 24;

    b->score_grid_pos = (score_row * 40) + start_col;

    // Clear area (always clear 3 for safety)
    Screen[b->score_grid_pos]   = ' ';
    Screen[b->score_grid_pos+1] = ' ';
    Screen[b->score_grid_pos+2] = ' ';

    // Draw score
    if (points == 300) {
        Screen[b->score_grid_pos]   = '3';
        Screen[b->score_grid_pos+1] = '0';
        Screen[b->score_grid_pos+2] = '0';
    } else {
        Screen[b->score_grid_pos+1] = '5';
        Screen[b->score_grid_pos+2] = '0';
    }

    // Set score color (safe to set all 3)
    Color[b->score_grid_pos]   = VCOL_WHITE;
    Color[b->score_grid_pos+1] = VCOL_WHITE;
    Color[b->score_grid_pos+2] = VCOL_WHITE;

    // Start score timer now, independent of explosion
    b->score_timer = SCORE_SHOW_TIME;

    update_score_display();

    return points;
}

bonus_ship_state* bonus_ship_get_state(void) {
    return &s_bonus_state;
}