#include "aliens.h"
#include <stdlib.h>
#include <c64/vic.h>
#include <c64/types.h>

// --- CONFIGURATION ---
#define START_ROW       4
#define COLLIDE_ROW     22
#define DROP_ROWS       1
#define ALIENS_PER_ROW  11
#define NUM_ROWS        5
#define MOVEMENT_DELAY  20  // Frames to wait before moving (Speed)

#define TOTAL_ALIENS    (NUM_ROWS * ALIENS_PER_ROW)

// --- LOOKUP TABLES ---
// Duplicated here to allow compiler optimizations (Zero Page access potential)
static const unsigned short g_row_offsets[26] = {
    0,   40,  80,  120, 160, 200, 240, 280, 320, 360,
    400, 440, 480, 520, 560, 600, 640, 680, 720, 760,
    800, 840, 880, 920, 960, 1000
};

// Alien Definitions (2 chars per frame)
// Structure: [Type][Frame][LeftChar, RightChar]
static const unsigned char ALIEN_CHARS[3][2][2] = {
    // Type 0 (Top): 132, 133 / 138, 139
    { {132, 133}, {138, 139} },
    // Type 1 (Mid): 130, 131 / 136, 137
    { {130, 131}, {136, 137} },
    // Type 2 (Bot): 128, 129 / 134, 135
    { {128, 129}, {134, 135} }
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

// Grid Position (Top-Left of the swarm)
static int g_grid_x = 0;
static int g_grid_y = START_ROW;
static int g_old_grid_x = 0;
static int g_old_grid_y = START_ROW;

// Movement State
static int g_dir = 1;       // 1 = Right, -1 = Left
static int g_next_dir = 1;  // Used during drop state
static int g_state = 0;     // 0=Move Horiz, 1=Drop Down
static unsigned char g_anim_frame = 0;
static unsigned char g_timer = 0;

// Render State
static unsigned char g_render_dirty = 1; // 1 = Redraw needed

// --- LOGIC ---

void aliens_init(byte* screen) {
    g_alive_count = 0;
    g_grid_x = 2; 
    g_grid_y = START_ROW;
    g_dir = 1;
    g_timer = MOVEMENT_DELAY;
    g_render_dirty = 1;

    // Define colors for Rows 0 through 4
    static const unsigned char ROW_COLORS[5] = {
        VCOL_LT_RED, // Top Row
        VCOL_YELLOW, // Second Row
        VCOL_GREEN,  // Third Row
        VCOL_PURPLE, // Fourth Row
        VCOL_CYAN    // Bottom Row
    };

    unsigned char i = 0;
    
    for (unsigned char r = 0; r < NUM_ROWS; r++) {
        unsigned char type;
        if (r == 0) type = 0;      
        else if (r < 3) type = 1;  
        else type = 2;             

        // Grab the color for this row
        unsigned char row_color = ROW_COLORS[r];

        // FIX: Only loop for the number of aliens in THIS ROW
        for (unsigned char c = 0; c < ALIENS_PER_ROW; c++) {
            
            // Safety check (optional but good for debugging)
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
    // 1. Commit positions for Render (The "Old" pos becomes current render pos)
    g_old_grid_x = g_grid_x;
    g_old_grid_y = g_grid_y;

    // 2. Wait for Timer
    if (g_timer > 0) {
        g_timer--;
        return; 
    }
    g_timer = MOVEMENT_DELAY; // Reset timer
    g_render_dirty = 1;       // Flag that we moved

    // 3. Animation Toggle
    g_anim_frame ^= 1;

    // 4. Calculate Edges of the ACTIVE swarm
    // We only care about edges if moving horizontally
    if (g_state == 0) {
        int min_x = 100;
        int max_x = -100;
        
        for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
            if (!g_aliens[i].active) continue;
            
            // Calculate Global X for this alien
            int ax = g_grid_x + g_aliens[i].rel_x;
            if (ax < min_x) min_x = ax;
            if (ax > max_x) max_x = ax;
        }

        // 5. Check Collisions
        if (g_dir == 1) { // Moving Right
            // Right edge of alien is x+1. Screen edge is 39.
            if (max_x >= 38) { 
                g_state = 1; // Switch to Drop State
                g_next_dir = -1;
            } else {
                g_grid_x++;
            }
        } 
        else { // Moving Left
            if (min_x <= 0) {
                g_state = 1; // Switch to Drop State
                g_next_dir = 1;
            } else {
                g_grid_x--;
            }
        }
    } 
else {
        // 6. Drop Down Logic
        g_grid_y += DROP_ROWS;
        g_dir = g_next_dir;
        g_state = 0; // Resume Horizontal

        // *** FIX: Check if ANY active alien has hit the bottom ***
        for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
             if (!g_aliens[i].active) continue;

             // Calculate absolute Y for this specific alien
             int alien_y = g_grid_y + g_aliens[i].rel_y;

             if (alien_y >= COLLIDE_ROW) {
                 // RESTART GAME
                 // Reset grid to top
                 g_grid_y = START_ROW;
                 g_grid_x = 2; // Reset X too or they might be stuck at edge
                 
                 // If you want to respawn dead aliens, call aliens_init(Screen) here.
                 // For now, we just loop the current survivors to the top:
                 return; 
             }
        }
    }
}

// --- RENDER ---

void aliens_render(byte* screen) {
    if (!g_render_dirty) return;
    g_render_dirty = 0;

    // 1. Erase at OLD position
    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
        if (!g_aliens[i].active) continue;
        
        // Safety check
        unsigned short r = g_old_grid_y + g_aliens[i].rel_y;
        if (r > 24) continue; 

        unsigned short offset = g_row_offsets[r] + g_old_grid_x + g_aliens[i].rel_x;
        
        // Erase Characters
        screen[offset] = ' '; 
        screen[offset + 1] = ' ';
        
        // OPTIONAL: Reset Color to black or white if you want strictly clean cleanup
        // But usually, just clearing the character is enough if we draw correctly next time.
    }

    // 2. Draw at NEW position
    for (unsigned char i = 0; i < TOTAL_ALIENS; i++) {
        if (!g_aliens[i].active) continue;

        unsigned short r = g_grid_y + g_aliens[i].rel_y;
        if (r > 24) continue;

        unsigned short offset = g_row_offsets[r] + g_grid_x + g_aliens[i].rel_x;
        unsigned char t = g_aliens[i].type;
        
        // Draw Characters
        screen[offset]     = ALIEN_CHARS[t][g_anim_frame][0];
        screen[offset + 1] = ALIEN_CHARS[t][g_anim_frame][1];

        // Use Stored Row Color
        unsigned char c = g_aliens[i].color;
        Color[offset]     = c;
        Color[offset + 1] = c;
    }
}