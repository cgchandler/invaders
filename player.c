#include "player.h"
#include <c64/vic.h>
#include <c64/types.h>
#include <c64/keyboard.h>

// --- CONFIGURATION ---
#define SPRITE_PTR_BASE     32 
#define PLAYER_IMAGE_IDX    (SPRITE_PTR_BASE + 0)

// Top Border (50) + Row 22 (176) - Sprite Top Padding (6) = 220
#define PLAYER_Y_POS        222
#define PLAYER_SPEED        2    
#define MIN_X               24   
#define MAX_X               320  

// --- STATE ---
unsigned char g_lives = 3;
unsigned int g_player_x = 160; 

// Extern: We will create this in invaders.c shortly
extern void game_over(void);

void player_die(void) {
    // 1. Decrement Lives
    if (g_lives > 0) g_lives--;

    update_lives_display();
    
    if (g_lives > 0) {
        sfx_player_die();
    }
    else {
        sfx_game_over();
    }

    // 2. Blocking Animation (The "Pause")
    // We loop for ~60 frames (approx 1 second)
    // Since we don't return to main(), the rest of the game is frozen.
    for (int i = 0; i < 60; i++) {
        vic_waitFrame();
        sound_update();         // <--- CRITICAL: Keep sound engine running during pause!
        vic.spr_enable ^= 1;    // Flicker: Toggle Sprite 0 Enable Bit
    }
    // Ensure sprite is ON before we continue
    vic.spr_enable |= 1;

    // 3. Check Game Over
    if (g_lives == 0) {
        game_over();
    } else {
        // 4. Soft Reset (Just reset positions)
        // Aliens hit the bottom, so they must reset to top
        aliens_reset();
        
        // Reset Player to Center
        player_reset_position(); 
    }
}

void player_reset_position(void) {
    g_player_x = 160;
}

// --- INPUT HELPER ---
// Direct hardware poll. 100% smooth, no "repeat delay".
// We write to DC00 to select a Row, then read DC01 to see the Columns.
static int is_key_down(unsigned char row_mask, unsigned char col_mask) {
    *(volatile byte*)0xDC00 = row_mask; 
    byte col = *(volatile byte*)0xDC01; 
    return (col & col_mask) == 0; // 0 means the switch is closed (pressed)
}

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

/*
void player_update(void) {

    // Move Left (Key 'A')
    // Row 1 (0xFD), Bit 2 (0x04)
    if (is_key_down(0xFD, 0x04)) {
        if (g_player_x > MIN_X) {
            g_player_x -= PLAYER_SPEED;
        }
    }
    
    // Move Right (Key 'D') 
    // Row 2 (0xFB), Bit 2 (0x04)
    if (is_key_down(0xFB, 0x04)) {
        if (g_player_x < MAX_X) {
            g_player_x += PLAYER_SPEED;
        }
    }
}
*/

void player_update(void) {
    keyb_poll();

    // Check if ANY Shift key is held
    // Note: KSCAN_SHIFT_LOCK in this enum corresponds to the Left Shift matrix line
    int is_shifted = key_pressed(KSCAN_SHIFT_LOCK) || key_pressed(KSCAN_RSHIFT);

    // Handle LEFT Movement
    // triggers on: 'A' OR 'Physical Left Arrow (Top-Left)' OR 'Shift + Cursor Key'
    if (key_pressed(KSCAN_A) || 
        key_pressed(KSCAN_ARROW_LEFT) || 
        (key_pressed(KSCAN_CSR_RIGHT) && is_shifted)) 
    {
        if (g_player_x > MIN_X) {
            g_player_x -= PLAYER_SPEED;
        }
    }

    // Handle RIGHT Movement
    // triggers on: 'D' OR 'Cursor Key (without Shift)'
    // We use 'else if' to prevent moving both ways if keys are mashed
    else if (key_pressed(KSCAN_D) || 
            (key_pressed(KSCAN_CSR_RIGHT) && !is_shifted)) 
    {
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