#include "missile.h"
#include "aliens.h"  // To report hits
#include "bonus_ship.h" // report bonus ship hits
#include "player.h"  // To get player position
#include <c64/vic.h>
#include <c64/types.h>

#include "config.h"
// --- CONFIGURATION ---
// Sprite Pointer is defined in config.h (MISSILE_SPRITE_PTR)
#define MISSILE_SPEED       2   // Pixels per frame
#define MISSILE_COLOR       VCOL_WHITE

// Screen Geometry for Collision
#define SCREEN_LEFT_EDGE    24
#define SCREEN_TOP_EDGE     50

// --- STATE ---
static missile_state s_missile_state = { 0 };

static inline missile_state* _mstate(void) { return &s_missile_state; }

// Need access to Player X to know where to fire from (provided via player.h macros)

// --- HELPERS ---

// Direct Hardware Scan for Spacebar
// Row 7 (0x7F), Bit 4 (0x10)
static int is_space_pressed(void) {
    *(volatile byte*)0xDC00 = 0x7F; 
    return ((*(volatile byte*)0xDC01) & 0x10) == 0;
}

// Helper to check a specific pixel point against the grid
// Returns 1 if hit, 0 if miss
static int check_grid_hit(unsigned int pixel_x, unsigned int pixel_y) {
    
    // Boundary Checks
    if (pixel_x < SCREEN_LEFT_EDGE || pixel_y < SCREEN_TOP_EDGE) return 0;

    // Convert Pixel to Grid
    unsigned char col = (pixel_x - SCREEN_LEFT_EDGE) / 8;
    unsigned char row = (pixel_y - SCREEN_TOP_EDGE) / 8;

    if (col >= 40 || row >= 25) return 0;

    // Peek Screen Memory
    unsigned char char_code = Screen[row * 40 + col];

    // Bonus ship is a sprite, not in Screen RAM
    // bonus_check_hit returns 0 or the points awarded
    if (bonus_check_hit(col, row) != 0) {
        return 1;
    }

    // Check for Alien (Range 128-143)
    if (char_code >= 128 && char_code <= 143) {
        // We found a char, now ask aliens.c if it's a valid target
        if (aliens_check_hit(col, row)) {
            return 1; // Real hit confirmed
        }
    }
    return 0;
}

// --- PUBLIC API ---

void missile_init(void) {
    missile_state* m = _mstate();
    m->active = 0;
    
    // Set pointer for Sprite 1 (Offset + 1 from Player)
    byte* ptrs = (byte*)(0x4400 + 1016);
    ptrs[1] = MISSILE_SPRITE_PTR;

    // Config Sprite 1
    vic.spr_enable   &= ~2;          // Disable initially (Bit 1 = 0)
    vic.spr_multi    &= ~2;          // Hi-Res
    vic.spr_expand_x &= ~2;          // Normal width
    vic.spr_expand_y &= ~2;          // Normal height
    vic.spr_color[1] = MISSILE_COLOR;
}

void missile_update(void) {
    // 1. FIRE LOGIC
    player_state* pstate = player_get_state();
    missile_state* m = _mstate();
    if (!m->active) {
        if (is_space_pressed()) {
            m->active = 1;
            sfx_fire_missile();
            // Launch Alignment:
            // Since the missile art is centered in the 24px sprite, 
            // aligning Sprite X to Player X aligns them perfectly.
            // X: Shift 1 pixel left from player position
            m->x = pstate->player_x - 1; 
            m->y = 211; 
        }
        return;
    }

    // 2. MOVEMENT
    if (m->y > MISSILE_SPEED + 40) { 
        m->y -= MISSILE_SPEED;
    } else {
        m->active = 0; 
        return;
    }

    // 3. ROBUST COLLISION DETECTION
    // We check the "Tip" and the "Body" to prevent tunneling through rows.
    
    // The missile art is centered in the 24px sprite, so add 12 to find the visual center.
    unsigned int visual_x = m->x + 12;
    unsigned int visual_y = m->y + 7;

    // Check Tip (Top pixel)
    if (check_grid_hit(visual_x, visual_y)) {
        m->active = 0;
        return;
    }

    // Check Body (length of missle pixels down) to prevent tunnelling
    if (check_grid_hit(visual_x, visual_y + 7)) {
        m->active = 0;
        return;
    }
}

void missile_render(void) {
    missile_state* m = _mstate();
    if (m->active) {
        vic.spr_enable |= 2; // Enable Sprite 1 (Bit 1)
        
        vic.spr_pos[1].y = (byte)m->y;

        // X Position & MSB Logic
        if (m->x > 255) {
            vic.spr_pos[1].x = (byte)(m->x & 0xFF);
            vic.spr_msbx |= 2; // Set MSB for Sprite 1 (Bit 1)
        } else {
            vic.spr_pos[1].x = (byte)m->x;
            vic.spr_msbx &= ~2; // Clear MSB for Sprite 1
        }
    } else {
        vic.spr_enable &= ~2; // Disable Sprite 1
    }
}

missile_state* missile_get_state(void) {
    return &s_missile_state;
}