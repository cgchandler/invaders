#include "aliens.h"
#include <stdlib.h>
#include <c64/vic.h>
#include <c64/types.h>

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

extern unsigned int g_player_x; 

// --- LOOKUP TABLES ---
// FIX: Expanded table to 32 entries to prevent overflow when aliens hit bottom
static const unsigned short g_row_offsets[32] = {
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

static const unsigned char SPEED_TABLE[57] = {
    2, 2, 2, 3, 3, 4, 4, 5, 5, 6,       
    6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 
    13, 13, 14, 14, 15, 16, 17, 18, 19, 20, 21, 22, 
    24, 26, 28, 29, 30, 30, 31, 32, 33, 34, 35, 36, 
    37, 38, 39, 40, 41, 42, 43, 44, 45, 56                      
};

// --- STATE ---
struct Alien {
    unsigned char active;
    unsigned char type;
    unsigned char rel_x;
    unsigned char rel_y;
    unsigned char color;
};

static struct Alien g_aliens[TOTAL_ALIENS];
unsigned char g_alive_count = TOTAL_ALIENS;

static int g_grid_x = START_COL;
static int g_grid_y = START_ROW;
static int g_old_grid_x = START_COL;
static int g_old_grid_y = START_ROW;

static int g_dir = 1;       
static int g_next_dir = 1;  
static int g_state = 0;     
static unsigned char g_anim_frame = 0;
static unsigned char g_timer = 0;

static unsigned char g_render_dirty = 1; 

// --- LOGIC ---

void aliens_init(byte* screen) {
    g_alive_count = 0;
    g_grid_x = START_COL; 
    g_grid_y = START_ROW;
    g_dir = 1;
    g_timer = MOVEMENT_DELAY;
    g_render_dirty = 1;

    static const unsigned char ROW_COLORS[5] = {
        VCOL_LT_RED, VCOL_YELLOW, VCOL_GREEN, VCOL_PURPLE, VCOL_CYAN
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

            g_aliens[i].active = 1;
            g_aliens[i].type = type;
            g_aliens[i].rel_y = r * 2; 
            g_aliens[i].rel_x = c * 3; 
            g_aliens[i].color = row_color;
            
            g_alive_count++;
            i++;
        }
    }
}

void aliens_update(void) {
    g_old_grid_x = g_grid_x;
    g_old_grid_y = g_grid_y;

    if (g_timer > 0) {
        g_timer--;
        return; 
    }

    sfx_march();

    unsigned char safe_count = g_alive_count;
    if (safe_count > 55) safe_count = 55; 

    //g_timer = SPEED_TABLE[g_alive_count];
    g_timer = safe_count + 1 - (g_level - 1);   // linear speed instead of table lookup
    if (g_timer < 2) {g_timer = 2;}
    g_render_dirty = 1;       

    g_anim_frame ^= 1;

    if (g_state == 0) {
        int min_x = 100;
        int max_x = -100;
        
        for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
            if (!g_aliens[i].active) continue;
            
            int ax = g_grid_x + g_aliens[i].rel_x;
            if (ax < min_x) min_x = ax;
            if (ax > max_x) max_x = ax;
        }

        if (g_dir == 1) { 
            if (max_x >= 38) { 
                g_state = 1; 
                g_next_dir = -1; 
            } else {
                g_grid_x++;
            }
        } else { 
            if (min_x <= 0) {
                g_state = 1; 
                g_next_dir = 1; 
            } else {
                g_grid_x--;
            }
        }
    } 
    else {
        if (g_grid_y < COLLIDE_ROW) {
            g_grid_y += DROP_ROWS;
        }
        g_dir = g_next_dir;
        g_state = 0; 

        for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
             if (!g_aliens[i].active) continue;
             int alien_y = g_grid_y + g_aliens[i].rel_y;

             if (alien_y >= COLLIDE_ROW) {
                 player_die();
                 return; 
             }
        }
    }

    int p_pixel_x = g_player_x;
    if (p_pixel_x < 24) p_pixel_x = 24;

    int p_col_start = (p_pixel_x - 24) / 8;
    int p_col_end   = (p_pixel_x - 24 + 23) / 8; 

    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
         if (!g_aliens[i].active) continue;

         int alien_y = g_grid_y + g_aliens[i].rel_y;
         int alien_x = g_grid_x + g_aliens[i].rel_x; 

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

void aliens_render(byte* screen) {
    if (!g_render_dirty) return;
    g_render_dirty = 0;

    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
        if (!g_aliens[i].active) continue;
        
        unsigned short r = g_old_grid_y + g_aliens[i].rel_y;
        if (r >= 23) continue; 

        // Use safe offset calculation or ensure r < 32
        unsigned short offset = g_row_offsets[r] + g_old_grid_x + g_aliens[i].rel_x;
        
        screen[offset] = ' '; 
        screen[offset + 1] = ' ';
    }

    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
        if (!g_aliens[i].active) continue;

        unsigned short r = g_grid_y + g_aliens[i].rel_y;
        if (r > 24) continue;

        unsigned short offset = g_row_offsets[r] + g_grid_x + g_aliens[i].rel_x;
        unsigned char t = g_aliens[i].type;
        
        screen[offset]     = ALIEN_CHARS[t][g_anim_frame][0];
        screen[offset + 1] = ALIEN_CHARS[t][g_anim_frame][1];

        unsigned char c = g_aliens[i].color;
        Color[offset]     = c;
        Color[offset + 1] = c;
    }
}

extern byte* Screen; 

int aliens_check_hit(unsigned char col, unsigned char row) {
    if (col < g_grid_x || row < g_grid_y) return 0;

    unsigned char target_rel_x = col - g_grid_x;
    unsigned char target_rel_y = row - g_grid_y;

    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
        if (!g_aliens[i].active) continue;

        // FIX: Check Top Row (rel_y) AND Bottom Row (rel_y + 1)
        // This ensures the missile hits even if aligned with the alien's feet.
        if (target_rel_y == g_aliens[i].rel_y || target_rel_y == g_aliens[i].rel_y + 1) {
            
            if (g_aliens[i].rel_x == target_rel_x || g_aliens[i].rel_x + 1 == target_rel_x) {
                
                unsigned short r = g_grid_y + g_aliens[i].rel_y;
                // Calculate Offset manually to be safe or use expanded table
                unsigned short offset = (r * 40) + g_grid_x + g_aliens[i].rel_x;
                
                if (offset < 1000) { 
                    Screen[offset] = ' ';
                    Screen[offset + 1] = ' ';
                }

                unsigned short old_r = g_old_grid_y + g_aliens[i].rel_y;
                unsigned short old_offset = (old_r * 40) + g_old_grid_x + g_aliens[i].rel_x;

                if (old_offset < 1000) { 
                    Screen[old_offset] = ' ';
                    Screen[old_offset + 1] = ' ';
                }

                g_aliens[i].active = 0;
                sfx_alien_hit();
                g_alive_count--;
                g_render_dirty = 1; 

                if (g_aliens[i].type == 0)      g_score += 30;
                else if (g_aliens[i].type == 1) g_score += 20;
                else                            g_score += 10;

                update_score_display();
                return 1; 
            }
        }
    }
    return 0;
}

int aliens_cleared(void) {
    return g_alive_count < 1;
}

void aliens_reset(void) {
    g_grid_x = START_COL; 
    g_grid_y = START_ROW;
    
    g_timer      = 0;
    g_anim_frame = 0;
    g_dir        = 1;  
    g_next_dir   = 1;  
    g_state      = 0;  
    
    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
        g_aliens[i].active = 1;
    }
    g_alive_count = TOTAL_ALIENS;
    g_render_dirty = 1;
}