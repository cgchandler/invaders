#include "player.h"
#include <c64/vic.h>
#include <c64/types.h>
#include <c64/keyboard.h>

// --- CONFIGURATION ---
#define SPRITE_PTR_BASE     32 
#define PLAYER_IMAGE_IDX    (SPRITE_PTR_BASE + 0)

// Top Border (50) + Row 22 (176) - Sprite Top Padding (6) = 220
#define PLAYER_Y_POS        220
#define PLAYER_SPEED        2    
#define MIN_X               24   
#define MAX_X               320  

// --- STATE ---
static unsigned int g_player_x; 

// --- PLAYER MOVEMENT ---
void player_init(void) {
    // 1. Set Sprite Pointer
    byte* screen_ptr_area = (byte*)(0x4400 + 1016);
    screen_ptr_area[0] = PLAYER_IMAGE_IDX; 

    // 2. Configure VIC-II Sprite 0
    vic.spr_enable   |= 1;           // Enable Sprite 0
    
    // FIX 1: Ensure Multicolor is OFF using the correct register name
    vic.spr_multi    &= ~1;          
    
    vic.spr_expand_x &= ~1;          // Normal Width (|= 1 for double width)
    vic.spr_expand_y &= ~1;          // Normal Height (|= 1 for double height)
    vic.spr_color[0]  = VCOL_GREEN;  

    // FIX 2: Explicitly clear the 9th X-Bit (MSB) on startup
    // If this starts as 1, the ship is drawn off-screen (X+256)
    vic.spr_msbx     &= ~1;

    // 3. Init State
    g_player_x = 172; 
}

void player_update(void) {
    keyb_poll();

    // Move Left (A) 
    if (key_pressed(KSCAN_A)) {
        if (g_player_x > MIN_X) {
            g_player_x -= PLAYER_SPEED;
        }
    }
    
    // Move Right (D) 
    if (key_pressed(KSCAN_D)) {
        if (g_player_x < MAX_X) {
            g_player_x += PLAYER_SPEED;
        }
    }
}

void player_render(void) {
    vic.spr_pos[0].y = PLAYER_Y_POS;

    if (g_player_x > 255) {
        vic.spr_pos[0].x = (byte)(g_player_x & 0xFF);
        // FIX 3: Use the correct struct member 'spr_msbx'
        vic.spr_msbx |= 1; 
    } else {
        vic.spr_pos[0].x = (byte)g_player_x;
        // FIX 3: Use the correct struct member 'spr_msbx'
        vic.spr_msbx &= ~1;
    }
}