#include "aliens.h"
#include <stdlib.h>
#include <c64/vic.h>
#include <c64/types.h>
#include "player.h"
#include "bases.h"
#include "config.h"

// --- CONFIGURATION ---
#define START_ROW       2
#define START_COL       2
#define COLLIDE_ROW     23
#define DROP_ROWS       1
#define ALIENS_PER_ROW  11
#define NUM_ROWS        5
#define MOVEMENT_DELAY  20  
#define PLAYER_HIT_ROW  22   

#define TOTAL_ALIENS    (NUM_ROWS * ALIENS_PER_ROW)

#define STATE_DEAD       0
#define STATE_ALIVE      1
#define STATE_EXPLODING  2

#define EXPLOSION_SPEED  8   // How many frames to stay on each stage (slower = bigger number)
#define EXPLOSION_BASE   160 // The first character of your explosion set

extern void game_over(void);

// --- LOOKUP TABLES ---
// FIX: Expanded table to 32 entries to prevent overflow when aliens hit bottom
static const unsigned short row_offsets[32] = {
    0,   40,  80,  120, 160, 200, 240, 280, 320, 360,
    400, 440, 480, 520, 560, 600, 640, 680, 720, 760,
    800, 840, 880, 920, 960, 1000, 
    1040, 1080, 1120, 1160, 1200, 1240 // Safety padding
};

static const unsigned char ALIEN_CHARS[3][2][2] = {
    { {132, 133}, {138, 139} },
    { {130, 131}, {136, 137} },
    { {128, 129}, {134, 135} }
};

// non-linear speed table based on number of alive aliens
// quadratic ease-in curve formula: speed = 2 + ( (n * n) * 28 ) / (55 * 55);
static const unsigned char SPEED_TABLE[55] = {
    2, 2, 2, 2, 2, 
    2, 2, 2, 2, 2,
    4, 4, 4, 4, 4, 
    6, 6, 6, 6, 6,
    11, 11, 11, 11, 11,
    13, 13, 13, 13, 13,
    16, 16, 16, 16, 16,
    19, 19, 19, 19, 19,
    22, 22, 22, 22, 22,
    26, 26, 26, 26, 26,
    30, 30, 30, 30, 30   
};

// --- STATE ---
struct Alien {
    unsigned char state;    // Changed from 'active'. 0=Dead, 1=Alive, 2=Exploding
    unsigned char type;
    unsigned char rel_x;
    unsigned char rel_y;
    unsigned char color;
    unsigned char score_value;
    unsigned char anim_timer; // Counts the frames between graphic changes
    unsigned char anim_stage; // Tracks which of the 4 explosion sizes to draw (0-3)
};

static struct Alien aliens[TOTAL_ALIENS];
// Encapsulated public state (backwards-compatible names live in header macros)
aliens_state g_aliens_state = { 0 };

// --- LOGIC ---

void aliens_init(void) {
    aliens_state* a = aliens_get_state();
    a->alive_count = 0;
    a->grid_x = START_COL; 
    a->grid_y = START_ROW;
    a->dir = 1;
    a->timer = MOVEMENT_DELAY;
    a->render_dirty = 1;

    static const unsigned char ROW_COLORS[5] = {
        VCOL_LT_RED, VCOL_YELLOW, VCOL_GREEN, VCOL_PURPLE, VCOL_CYAN
    };

    static const unsigned char TYPE_SCORES[3] = {
        30, 20, 10
    };

    unsigned char i = 0;
    
    for (unsigned char r = 0; r < NUM_ROWS; r++) {
        unsigned char type;
        if (r == 0) type = 0;      
        else if (r < 3) type = 1;  
        else type = 2;             

        unsigned char row_color = ROW_COLORS[r];

        for (unsigned char c = 0; c < ALIENS_PER_ROW; c++) {
            if (i >= TOTAL_ALIENS) break; 

            aliens[i].state = STATE_ALIVE;
            aliens[i].type = type;
            aliens[i].rel_y = r * 2; 
            aliens[i].rel_x = c * 3; 
            aliens[i].color = row_color;
            aliens[i].score_value = TYPE_SCORES[type];
            a->alive_count++;
            i++;
        }
    }
}

static void update_explosions(void) {
    aliens_state* a = aliens_get_state();
    
    for (int i = 0; i < TOTAL_ALIENS; i++) {
        // We only care about aliens currently exploding
        if (aliens[i].state == STATE_EXPLODING) {
            
            // Handle Timing
            aliens[i].anim_timer++;
            
            if (aliens[i].anim_timer >= EXPLOSION_SPEED) {
                // Time to move to next frame
                aliens[i].anim_timer = 0;
                aliens[i].anim_stage++;

                // Check if Explosion is Done
                if (aliens[i].anim_stage >= 4) {
                    // Animation finished (0, 1, 2, 3 are done)
                    aliens[i].state = STATE_DEAD;

                    // Use GRID coordinates to calculate screen pos
                    int r = a->grid_y + aliens[i].rel_y;
                    int c = a->grid_x + aliens[i].rel_x;
                    
                    int offset = (r * 40) + c;
                    
                    if(offset < 1000) {
                        Screen[offset]     = 32; // Space
                        Screen[offset + 1] = 32; 
                    }
                    continue; // Done with this alien
                }
            }

            //  Draw the Explosion Frame
                int current_char = EXPLOSION_BASE + (aliens[i].anim_stage * 2);

                // Use GRID coordinates to calculate screen pos
                    int r = a->grid_y + aliens[i].rel_y;
                    int c = a->grid_x + aliens[i].rel_x;
            
            int offset = (r * 40) + c;
            
            if(offset < 1000) {
                // Draw Left half
                Screen[offset] = current_char;
                Color[offset]  = VCOL_WHITE; 
                
                // Draw Right half
                Screen[offset + 1] = current_char + 1;
                Color[offset + 1]  = VCOL_WHITE; 
            }
        }
    }
}

void aliens_update(void) {
    aliens_state* a = aliens_get_state();
    a->old_grid_x = a->grid_x;
    a->old_grid_y = a->grid_y;

    player_state* pstate = player_get_state();

    if (a->timer > 0) {
        a->timer--;
        return; 
    }

    sfx_march();

    // Calculate speed based on level and number of alive aliens
    game_state* gs = game_get_state();
    int speed_index = a->alive_count - 1 - gs->level;
    if (speed_index < 0) {
        speed_index = 0;
    } else if (speed_index > 54) {
        speed_index = 54;
    }
    int calc_speed = SPEED_TABLE[speed_index];

    /* REPLACED IN FAVOR OF CALCULATION BASED ON LEVEL
       THIS CODE HAS BEEN LEFT IN FOR REFERENCE
    // Calculate speed deduction based on level
    game_state* gs = game_get_state();
    int level_bonus = gs->level - 1;
    if (level_bonus > 50) level_bonus = 50; 
    
    int calc_speed = safe_count + 1 - level_bonus;
    if (calc_speed < 2) calc_speed = 2;
    */

    // STORE IT so we can debug it without recalculating
    a->current_delay = (unsigned char)calc_speed; 
    a->timer = a->current_delay;

    // For debugging
    //g_timer = 56;

    a->render_dirty = 1;       

    a->anim_frame ^= 1;

    if (a->state == 0) {
        int min_x = 100;
        int max_x = -100;
        
        for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
            if (!aliens[i].state) continue;
            
            int ax = a->grid_x + aliens[i].rel_x;
            if (ax < min_x) min_x = ax;
            if (ax > max_x) max_x = ax;
        }

        if (a->dir == 1) { 
            if (max_x >= 38) { 
                a->state = 1; 
                a->next_dir = -1; 
            } else {
                a->grid_x++;
            }
        } else { 
            if (min_x <= 0) {
                a->state = 1; 
                a->next_dir = 1; 
            } else {
                a->grid_x--;
            }
        }
    } 
    else {
        if (a->grid_y < COLLIDE_ROW) {
            a->grid_y += DROP_ROWS;
        }
        a->dir = a->next_dir;
        a->state = 0; 

        // Check if ANY active alien has hit the bottom
           for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
               if (!aliens[i].state) continue;
               int alien_y = a->grid_y + aliens[i].rel_y;

             if (alien_y >= COLLIDE_ROW) {
                 // If they hit the ground, the base is lost regardless of lives.
                 game_over(); 
                 return; 
             }
        }
    }

    int p_pixel_x = pstate->player_x;
    if (p_pixel_x < 24) p_pixel_x = 24;

    int p_col_start = (p_pixel_x - 24) / 8;
    int p_col_end   = (p_pixel_x - 24 + 23) / 8; 

    /* Check for alien collisions with bases. If an alien overlaps a base
       character its character is destroyed and the alien is removed and
       the player awarded points. */
    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
        if (aliens[i].state != STATE_ALIVE) continue;

        int alien_y_top = a->grid_y + aliens[i].rel_y;
        int alien_x = a->grid_x + aliens[i].rel_x;

        /* Aliens occupy two text rows (top and bottom). Check both. */
        for (int check_row = alien_y_top; check_row <= alien_y_top + 1; check_row++) {
            if (check_row == BASE_TOP_ROW || check_row == BASE_BOTTOM_ROW) {
                /* Check both left and right character columns */
                for (int cx = 0; cx <= 1; cx++) {
                    int col = alien_x + cx;
                    if (col < 0 || col >= 40) continue;
                    if (bases_check_hit((unsigned char)col, (unsigned char)check_row, true)) {
                        /* Start explosion animation (same as missile hit) */
                        unsigned short r = a->grid_y + aliens[i].rel_y;
                        unsigned short offset = (r * 40) + a->grid_x + aliens[i].rel_x;
                        if (offset < 1000) {
                            Screen[offset] = ' ';
                            Screen[offset + 1] = ' ';
                        }

                        unsigned short old_r = a->old_grid_y + aliens[i].rel_y;
                        unsigned short old_offset = (old_r * 40) + a->old_grid_x + aliens[i].rel_x;
                        if (old_offset < 1000) {
                            Screen[old_offset] = ' ';
                            Screen[old_offset + 1] = ' ';
                        }

                        aliens[i].state = STATE_EXPLODING;
                        aliens[i].anim_stage = 0;
                        aliens[i].anim_timer = 0;
                        sfx_alien_hit();
                        a->alive_count--;
                        a->render_dirty = 1;
                        update_score_display();
                        break; /* stop checking columns for this alien */
                    }
                }
            }
            if (aliens[i].state == STATE_EXPLODING) break;
        }
    }

    /* Check for alien collisions with player. If any alien overlaps the
       player's character, the player dies. */
    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
        if (aliens[i].state != STATE_ALIVE) continue;

        int alien_y = a->grid_y + aliens[i].rel_y;
        int alien_x = a->grid_x + aliens[i].rel_x; 

        if (alien_y >= PLAYER_HIT_ROW) {
            int alien_right = alien_x + 1;
            
            if (alien_x <= p_col_end && alien_right >= p_col_start) {
                player_die();
                return;
            }
        }
    }
}

// --- RENDER ---

void aliens_render() {

    aliens_state* a = aliens_get_state();

    // Update explosions even if aliens aren't moving (dirty=0)
    if (!a->render_dirty) {
        update_explosions();
        return;
    }
    
    a->render_dirty = 0;

    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
        // Clearing loop: Clear old positions for ALL non-dead aliens
        // We still need to clear old position for Exploding aliens if the grid moved
        if (aliens[i].state == STATE_DEAD) continue;
        
        unsigned short r = a->old_grid_y + aliens[i].rel_y;
        if (r >= 23) continue; 

        // Use safe offset calculation or ensure r < 32
        unsigned short offset = row_offsets[r] + a->old_grid_x + aliens[i].rel_x;
        
        Screen[offset] = ' '; 
        Screen[offset + 1] = ' ';
    }

    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
        // DRAWING loop
        // Do NOT draw normal aliens if they are Exploding.
        // update_explosions handles drawing them.
        if (aliens[i].state != STATE_ALIVE) continue;

        unsigned short r = a->grid_y + aliens[i].rel_y;
        if (r > 24) continue;

        unsigned short offset = row_offsets[r] + a->grid_x + aliens[i].rel_x;
        unsigned char t = aliens[i].type;
        
        Screen[offset]     = ALIEN_CHARS[t][a->anim_frame][0];
        Screen[offset + 1] = ALIEN_CHARS[t][a->anim_frame][1];

        unsigned char c = aliens[i].color;
        Color[offset]     = c;
        Color[offset + 1] = c;
    }
    
    // Update explosions AFTER the clearing/drawing loops.
    // This ensures the explosion is drawn on top and not wiped by the clearing loop.
    update_explosions();
}

int aliens_check_hit(unsigned char col, unsigned char row) {
    aliens_state* a = aliens_get_state();
    game_state* gs = game_get_state();
    if (col < a->grid_x || row < a->grid_y) return 0;

    unsigned char target_rel_x = col - a->grid_x;
    unsigned char target_rel_y = row - a->grid_y;

    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
        if (aliens[i].state == STATE_DEAD) continue;

        // Check Top Row (rel_y) AND Bottom Row (rel_y + 1)
        // This ensures the missile hits even if aligned with the alien's feet.
        if (target_rel_y == aliens[i].rel_y || target_rel_y == aliens[i].rel_y + 1) {
            
            if (aliens[i].rel_x == target_rel_x || aliens[i].rel_x + 1 == target_rel_x) {
                
                unsigned short r = a->grid_y + aliens[i].rel_y;
                // Calculate Offset manually to be safe or use expanded table
                unsigned short offset = (r * 40) + a->grid_x + aliens[i].rel_x;
                
                if (offset < 1000) { 
                    Screen[offset] = ' ';
                    Screen[offset + 1] = ' ';
                }

                unsigned short old_r = a->old_grid_y + aliens[i].rel_y;
                unsigned short old_offset = (old_r * 40) + a->old_grid_x + aliens[i].rel_x;

                if (old_offset < 1000) { 
                    Screen[old_offset] = ' ';
                    Screen[old_offset + 1] = ' ';
                }

                aliens[i].state = STATE_EXPLODING;
                aliens[i].anim_stage = 0;
                aliens[i].anim_timer = 0;
                sfx_alien_hit();
                a->alive_count--;
                a->render_dirty = 1;
                gs->score += aliens[i].score_value;

                update_score_display();
                return 1; 
            }
        }
    }
    return 0;
}

int aliens_cleared(void) {
    aliens_state* a = aliens_get_state();
    return a->alive_count < 1;
}

void aliens_reset_postion(aliens_state* a) {
    a->grid_x = START_COL; 
    a->grid_y = START_ROW;    
    a->render_dirty = 1;
}

void aliens_reset(void) {
    aliens_state* a = aliens_get_state();
    aliens_reset_postion(a);
    
    a->timer      = 0;
    a->anim_frame = 0;
    a->dir        = 1;  
    a->next_dir   = 1;  
    a->state      = 0;  
    
    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
        aliens[i].state = STATE_ALIVE;
    }
    a->alive_count = TOTAL_ALIENS;
}

// Helper to find a random active alien for bomb dropping
int aliens_get_random_shooter(int* out_x, int* out_y) {
    aliens_state* a = aliens_get_state();
    if (a->alive_count == 0) return 0;
    // Efficient single-pass: determine bottom-most alive alien per column
    // (columns are spaced by 3 pixels in rel_x: rel_x = col*3).
    int bottom_idx[ALIENS_PER_ROW];
    for (int c = 0; c < ALIENS_PER_ROW; c++) bottom_idx[c] = -1;

    for (int i = 0; i < TOTAL_ALIENS; i++) {
        if (aliens[i].state != STATE_ALIVE) continue;
        int col = aliens[i].rel_x / 3; // rel_x was initialized as c*3
        if (col < 0 || col >= ALIENS_PER_ROW) continue; // safety
        int prev = bottom_idx[col];
        if (prev == -1 || aliens[i].rel_y > aliens[prev].rel_y) {
            bottom_idx[col] = i;
        }
    }

    int shooters[ALIENS_PER_ROW];
    int sc = 0;
    for (int c = 0; c < ALIENS_PER_ROW; c++) {
        if (bottom_idx[c] != -1) shooters[sc++] = bottom_idx[c];
    }

    if (sc == 0) return 0;
    int pick = rand() % sc;
    int idx = shooters[pick];

    // Center the bomb spawn coordinates under the alien
    *out_x = (a->grid_x + aliens[idx].rel_x) * 8 + 24 - 4; 
    *out_y = (a->grid_y + aliens[idx].rel_y) * 8 + 50;
    return 1;
}

// ... end of aliens.c ...

// DEBUG: Display current speed (Delay Frames) on bottom right
void aliens_debug_speed(void) {
    /* Respect compile-time debug flag — no-op when disabled */
#if !DEBUG_INFO_ENABLED
    return;
#endif
    // 1. Get the digits from our stored setting
    aliens_state* a = aliens_get_state();
    unsigned char tens = a->current_delay / 10;
    unsigned char ones = a->current_delay % 10;
    
    // 2. Draw at Row 24, Columns 38-39 (Right Aligned)
    unsigned short offset = 960 + 38;
    
    // Draw Digits
    Screen[offset]     = tens + 48; 
    Screen[offset + 1] = ones + 48; 
    
    // Force White Color
    Color[offset]     = VCOL_WHITE;
    Color[offset + 1] = VCOL_WHITE;
}

aliens_state* aliens_get_state(void) {
    return &g_aliens_state;
}