#include "bombs.h"
#include "aliens.h"
#include "player.h" 
#include "config.h"
#include "bases.h"
#include <c64/vic.h>
#include <stdlib.h> 

// --- CONFIGURATION ---
#define BOMB_SPEED      2       // Pixels per frame        
#define GROUND_Y        225     // Y position of ground (player row)
#define FIRST_SPRITE    2       // First bomb uses Sprite 2 (Sprites 0 and 1 are Player and Missile)
#define BOMB_COLOR      VCOL_YELLOW

// Bomb Spawn Rate - inverse probability (or the "1-in-N chance") of a bomb spawning
// N / NTSC 60 frames per second is the seconds estimation
#define BOMB_SPAWN_RATE 50 // 2.0% chance or roughly every 0.8 seconds   
//#define BOMB_SPAWN_RATE 100 // 1.0% chance or roughly every 1.6 seconds   
//#define BOMB_SPAWN_RATE 200 // 0.5% chance or roughly every 3.3 seconds   
//#define BOMB_SPAWN_RATE 500 // 0.2% chance or roughly every 8.3 seconds   

// Module-local storage moved into bombs_state
static bombs_state s_bombs_state = { 0 };

static inline bombs_state* _bstate(void) { return &s_bombs_state; }

void bombs_init(void) {
    // POINTER SETUP
    // The VIC looks for sprite pointers at the last 8 bytes of Screen RAM.
    // Screen is at 0x4400, so Pointers are at Screen + 1016.
    
    // Get the pointer value used by the Missile (Sprite 1)
    // We read from Screen + 1016 + 1 (Sprite 1's slot)
    unsigned char bomb_ptr = Screen[1016 + 1];

    // Safety: If missile_init hasn't run or used a different method, 
    // default to 33 (which is what missile.c uses: 0x4840 / 64)
    if (bomb_ptr == 0) bomb_ptr = 33; 

    bombs_state* b = _bstate();
    for (int i = 0; i < MAX_BOMBS; i++) {
        b->active[i] = 0;
        
        // Set Sprite Pointer (Sprite 2, 3, 4, 5, 6)
        // We write to Screen + 1016 + Sprite Index
        Screen[1016 + FIRST_SPRITE + i] = bomb_ptr;
        
        // Set Color (Yellow)
        vic.spr_color[FIRST_SPRITE + i] = BOMB_COLOR;
        
        // Disable initially
        vic.spr_enable   &= ~(1 << (FIRST_SPRITE + i));
        vic.spr_multi    &= ~(1 << (FIRST_SPRITE + i));          // Hi-Res
        vic.spr_expand_x &= ~(1 << (FIRST_SPRITE + i));          // Normal width
        vic.spr_expand_y &= ~(1 << (FIRST_SPRITE + i));          // Normal height
            
        // Clear MSB just in case
        vic.spr_msbx &= ~(1 << (FIRST_SPRITE + i));
    }
}

void bombs_update(void) {
    // SPAWN LOGIC
    player_state* pstate = player_get_state();
    bombs_state* b = _bstate();
    if ((rand() % BOMB_SPAWN_RATE) == 0) {

        // There is a maximum number of bombs; find an inactive slot
        int slot = -1;
        for (int i = 0; i < MAX_BOMBS; i++) {
            if (!b->active[i]) {
                slot = i;
                break;
            }
        }
        
        if (slot != -1) {
            // Get random alien shooter position
            int start_x, start_y;
            if (aliens_get_random_shooter(&start_x, &start_y)) {
                b->active[slot] = 1;
                b->x[slot] = start_x;
                b->y[slot] = start_y;
                
                // Turn on Sprite
                vic.spr_enable |= (1 << (FIRST_SPRITE + slot));
            }
        }
    }

    // MOVEMENT & COLLISION
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (!b->active[i]) continue;

        // Move Down
        b->y[i] += BOMB_SPEED;

        /* Check collision with bases (grid conversion).
           Use a point slightly below the sprite Y to detect earlier
           (prevents the bomb from overlapping the base char before collision). */
        {
            if (b->x[i] >= SCREEN_LEFT_EDGE) {
                    unsigned int px1 = b->x[i] + 11;
                    unsigned int px2 = b->x[i] + 12;
                    int check_pixel_y = b->y[i] + 8; /* sample a point ~mid/bottom of sprite */
                    int row = (check_pixel_y - SCREEN_TOP_EDGE) / 8;
                    int col1 = (int)(px1 - SCREEN_LEFT_EDGE) / 8;
                    int col2 = (int)(px2 - SCREEN_LEFT_EDGE) / 8;
                    if (row >= 0 && row < 25) {
                        if ((col1 >= 0 && col1 < 40 && bases_check_hit((unsigned char)col1, (unsigned char)row)) ||
                            (col2 >= 0 && col2 < 40 && bases_check_hit((unsigned char)col2, (unsigned char)row))) {
                            b->active[i] = 0;
                            vic.spr_enable &= ~(1 << (FIRST_SPRITE + i));
                            continue;
                        }
                }
            }
        }

        // A. Check Ground Collision
        if (b->y[i] > GROUND_Y) {
            b->active[i] = 0;
            vic.spr_enable &= ~(1 << (FIRST_SPRITE + i));
            continue;
        }

        // B. Check Player Collision
        // Player Y Hitbox (Approx 216-231)
        if (b->y[i] > 222 && b->y[i] < 231) {

            // Use the bomb sprite's visible pixel columns (offsets 11 and 12)
            // to compute a tight X collision band, and compare against the
            // player's pixel range (width ~24 pixels).
            unsigned int bomb_px1 = b->x[i] + 11;
            unsigned int bomb_px2 = b->x[i] + 12;

            int player_left = (int)pstate->player_x;
            int player_right = player_left + 23; // inclusive end (24 pixels)

            // Collision if any visible bomb pixel intersects player range
            if (!((int)bomb_px2 < player_left || (int)bomb_px1 > player_right)) {
                // HIT!
                b->active[i] = 0;
                vic.spr_enable &= ~(1 << (FIRST_SPRITE + i));
                player_die();
            }
        }
    }
}

void bombs_render(void) {
    bombs_state* b = _bstate();
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (b->active[i]) {
            int s = FIRST_SPRITE + i;
            
            // Update Hardware Registers
            vic.spr_pos[s].x = b->x[i] & 0xFF;
            vic.spr_pos[s].y = b->y[i];
            
            // Handle MSB (X > 255)
            // Note: Using spr_msbx as corrected previously
            if (b->x[i] > 255) {
                vic.spr_msbx |= (1 << s);
            } else {
                vic.spr_msbx &= ~(1 << s);
            }
        }
    }
}

bombs_state* bombs_get_state(void) {
    return &s_bombs_state;
}