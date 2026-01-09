#include "missile.h"
#include "aliens.h"  // To report hits
#include "bonus_ship.h" // report bonus ship hits
#include "player.h"  // To get player position
#include <c64/vic.h>
#include <c64/types.h>
#include "player_input.h"

#include "config.h"
#include "bases.h"
// --- CONFIGURATION ---
// Sprite Pointer is defined in config.h (MISSILE_SPRITE_PTR)
#define MISSILE_SPEED       4   // Pixels per frame
#define MISSILE_COLOR       VCOL_WHITE

// --- STATE ---
static missile_state s_missile_state = { 0 };

static inline missile_state* _mstate(void) { return &s_missile_state; }

static unsigned demo_fire_counter = 0;

// Need access to Player X to know where to fire from (provided via player.h macros)

// --- HELPERS ---

/* Track previous fire state to detect a single key or fire button press.
   false = not pressed last frame, true = pressed last frame */
static bool prev_fire = false;

// Helper to check a sprite's visible pixels (two columns at offsets 11 and 12)
// against the text grid. `sprite_x` is the sprite's hardware X (left) coordinate.
// Returns 1 if hit, 0 if miss.
static int check_grid_hit_from_sprite(unsigned int sprite_x, unsigned int pixel_y) {

    // convert both visible sprite pixel columns (11 and 12 within sprite)
    unsigned int px1 = sprite_x + 11;
    unsigned int px2 = sprite_x + 12;

    // Boundary Checks (use right-most pixel)
    if (px2 < SCREEN_LEFT_EDGE || pixel_y < SCREEN_TOP_EDGE) return 0;

    // Convert Pixel to Grid for both pixels
    int col1 = (int)(px1 - SCREEN_LEFT_EDGE) / 8;
    int col2 = (int)(px2 - SCREEN_LEFT_EDGE) / 8;
    int row  = (int)(pixel_y - SCREEN_TOP_EDGE) / 8;

    // Validate row
    if (row < 0 || row >= 25) return 0;

    // Check both columns (they may map to same column)
    int cols[2] = { col1, col2 };
    for (int i = 0; i < 2; i++) {
        int col = cols[i];
        if (col < 0 || col >= 40) continue;

        unsigned char char_code = Screen[row * 40 + col];

        // Bonus ship is a sprite, not in Screen RAM
        if (bonus_check_hit((unsigned char)col, (unsigned char)row) != 0) {
            return 1;
        }

        // Check for Alien (Range 128-143)
        if (char_code >= 128 && char_code <= 143) {
            if (aliens_check_hit((unsigned char)col, (unsigned char)row)) {
                return 1; // Real hit confirmed
            }
        }

        // Prefer hitting the lower base cell first (missile travels up).
        // If a base exists on the row below, damage that first.
        int lower_row = row + 1;
        if (lower_row < 25) {
            if (bases_check_hit((unsigned char)col, (unsigned char)lower_row, false)) {
                return 1;
            }
        }

        // Then check the computed row
        if (bases_check_hit((unsigned char)col, (unsigned char)row, false)) {
            return 1;
        }
    }
    return 0;
}

// --- PUBLIC API ---

void missile_init(void) {
    missile_state* m = _mstate();
    m->active = 0;

    /* Reset previous fire state to avoid suppressed firing when entering demo */
    prev_fire = false;
    
    demo_fire_counter = 0;
    
    // Set pointer for Sprite 1 (Offset + 1 from Player)
    // Pointer table lives at Screen + 0x3F8 (1016)
    byte* ptrs = (byte*)(Screen + 1016);
    const byte VIC_BANK_BASE_PTR = (byte)(((unsigned)Sprites - 0x4000) >> 6);
    ptrs[1] = VIC_BANK_BASE_PTR + 1; // missile sprite is the 2nd slot in sprite data

    // Config Sprite 1
    vic.spr_enable   &= ~2;          // Disable initially (Bit 1 = 0)
    vic.spr_multi    &= ~2;          // Hi-Res
    vic.spr_expand_x &= ~2;          // Normal width
    vic.spr_expand_y &= ~2;          // Normal height
    vic.spr_color[1] = MISSILE_COLOR;
}

void missile_update(void) {
    // FIRE LOGIC
    player_state* pstate = player_get_state();
    missile_state* m = _mstate();
    if (!m->active) {
        player_input_t input;
        game_state* gs = game_get_state();
        if (gs->mode == MODE_DEMO) {
            /* Simulate occasional firing during demo mode */
            demo_fire_counter++;
            input.left = 0;
            input.right = 0;
            input.fire = (demo_fire_counter % DEMO_FIRE_INTERVAL) == 0;
        } else {
            player_input_update(&input);
        }

        if (input.fire && !prev_fire) {
            m->active = 1;
            sfx_fire_missile();
            // Launch Alignment:
            // Since the missile art is centered in the 24px sprite,
            // aligning Sprite X to Player X aligns them perfectly.
            // X: Shift 1 pixel left from player position
            m->x = pstate->player_x - 1;
            m->y = 211;
        }
        /* Update previous state so subsequent frames require key release */
        prev_fire = input.fire;        
        return;
    }

    // MOVEMENT
    if (m->y > MISSILE_SPEED + 40) { 
        m->y -= MISSILE_SPEED;
    } else {
        m->active = 0; 
        return;
    }

    // ROBUST COLLISION DETECTION
    // We check the "Tip" and the "Body" to prevent tunneling through rows.
    
    // The missile art's visible pixels are at offsets 11 and 12 within the sprite.
    unsigned int visual_y = m->y + 7;

    // Check Tip (Top pixel)
    if (check_grid_hit_from_sprite(m->x, visual_y)) {
        m->active = 0;
        return;
    }

    // Check Body (length of missle pixels down) to prevent tunnelling
    if (check_grid_hit_from_sprite(m->x, visual_y + 7)) {
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