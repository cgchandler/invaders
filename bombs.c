#include "bombs.h"
#include "aliens.h"
#include "player.h" 
#include <c64/vic.h>
#include <stdlib.h> 

// --- CONFIGURATION ---
#define MAX_BOMBS       5
#define BOMB_SPEED      2    
#define GROUND_Y        225  
#define FIRST_SPRITE    2    

// Bomb Spawn Rate - inverse probability (or the "1-in-N chance") of a bomb spawning
// N / NTSC 60 frames per second is the seconds estimation
//#define BOMB_SPAWN_RATE 100 // 1.0% chance or roughly every 1.6 seconds   
#define BOMB_SPAWN_RATE 200 // 0.5% chance or roughly every 3.3 seconds   
//#define BOMB_SPAWN_RATE 500 // 0.2% chance or roughly every 8.3 seconds   

// Access the global screen pointer (defined in main/invaders.c)
extern byte* Screen; 

struct Bomb {
    unsigned char active;
    unsigned int x;
    unsigned int y;
};

static struct Bomb g_bombs[MAX_BOMBS];

void bombs_init(void) {
    // 1. POINTER SETUP
    // The VIC looks for sprite pointers at the last 8 bytes of Screen RAM.
    // Screen is at 0x4400, so Pointers are at Screen + 1016.
    
    // Get the pointer value used by the Missile (Sprite 1)
    // We read from Screen + 1016 + 1 (Sprite 1's slot)
    unsigned char bomb_ptr = Screen[1016 + 1];

    // Safety: If missile_init hasn't run or used a different method, 
    // default to 33 (which is what missile.c uses: 0x4840 / 64)
    if (bomb_ptr == 0) bomb_ptr = 33; 

    for (int i = 0; i < MAX_BOMBS; i++) {
        g_bombs[i].active = 0;
        
        // Set Sprite Pointer (Sprite 2, 3, 4, 5, 6)
        // We write to Screen + 1016 + Sprite Index
        Screen[1016 + FIRST_SPRITE + i] = bomb_ptr;
        
        // Set Color (Yellow)
        vic.spr_color[FIRST_SPRITE + i] = VCOL_YELLOW;
        
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
    // 1. SPAWN LOGIC
    if ((rand() % BOMB_SPAWN_RATE) == 0) {
        int slot = -1;
        for (int i = 0; i < MAX_BOMBS; i++) {
            if (!g_bombs[i].active) {
                slot = i;
                break;
            }
        }
        
        if (slot != -1) {
            int start_x, start_y;
            if (aliens_get_random_shooter(&start_x, &start_y)) {
                g_bombs[slot].active = 1;
                g_bombs[slot].x = start_x;
                g_bombs[slot].y = start_y;
                
                // Turn on Sprite
                vic.spr_enable |= (1 << (FIRST_SPRITE + slot));
            }
        }
    }

    // 2. MOVEMENT & COLLISION
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (!g_bombs[i].active) continue;

        // Move Down
        g_bombs[i].y += BOMB_SPEED;

        // A. Check Ground Collision
        if (g_bombs[i].y > GROUND_Y) {
            g_bombs[i].active = 0;
            vic.spr_enable &= ~(1 << (FIRST_SPRITE + i));
            continue;
        }

        // B. Check Player Collision
        // Player Y Hitbox (Approx 210-225)
        if (g_bombs[i].y > 210 && g_bombs[i].y < 225) {
            
            // Check X Overlap (Player width approx 24px)
            if (g_bombs[i].x + 8 >= g_player_x && g_bombs[i].x <= g_player_x + 20) {
                
                // HIT!
                g_bombs[i].active = 0;
                vic.spr_enable &= ~(1 << (FIRST_SPRITE + i));
                
                player_die();
            }
        }
    }
}

void bombs_render(void) {
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (g_bombs[i].active) {
            int s = FIRST_SPRITE + i;
            
            // Update Hardware Registers
            vic.spr_pos[s].x = g_bombs[i].x & 0xFF;
            vic.spr_pos[s].y = g_bombs[i].y;
            
            // Handle MSB (X > 255)
            // Note: Using spr_msbx as corrected previously
            if (g_bombs[i].x > 255) {
                vic.spr_msbx |= (1 << s);
            } else {
                vic.spr_msbx &= ~(1 << s);
            }
        }
    }
}