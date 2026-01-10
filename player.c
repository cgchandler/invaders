// © 2026 Christopher G Chandler
// Licensed under the MIT License. See LICENSE file in the project root.

#include "player.h"
#include <c64/vic.h>
#include <c64/types.h>
#include "player_input.h"

// --- CONFIGURATION ---
// Sprite pointer base is centralized in config.h as PLAYER_SPRITE_PTR
#define PLAYER_IMAGE_IDX    (PLAYER_SPRITE_PTR + 0)

// Top Border (50) + Row 22 (176) - Sprite Top Padding (6) = 220
#define PLAYER_Y_POS        228 // 220
#define PLAYER_X_POS        172
#define PLAYER_SPEED        2    
#define MIN_X               24   
#define MAX_X               320  

// --- STATE ---
static player_state s_player_state = { .lives = 3, .default_lives = 3, .player_x = PLAYER_X_POS };

static inline player_state* _pstate(void) { return &s_player_state; }

// Extern: We will create this in invaders.c shortly
extern void game_over(void);

void player_die(void) {
    player_state* p = _pstate();
    // Decrement Lives
    if (p->lives > 0) p->lives--;

    bombs_init();       // Clear any active bombs
    missile_init();     // Clear any active missiles

    update_lives_display();
    
    if (p->lives > 0) {
        sfx_player_die();
    }
    else {
        sfx_game_over();
    }

    // Blocking Animation (The "Pause")
    // We loop for ~60 frames (approx 1 second)
    // Since we don't return to main(), the rest of the game is frozen.
    for (int i = 0; i < 60; i++) {
        vic_waitFrame();
        sound_update();         // <--- CRITICAL: Keep sound engine running during pause!
        vic.spr_enable ^= 1;    // Flicker: Toggle Sprite 0 Enable Bit
    }
    // Ensure sprite is ON before we continue
    vic.spr_enable |= 1;

    // Check Game Over
    if (p->lives == 0) {
        game_over();
    } else {
        // Soft Reset (Just reset positions)
        // Aliens hit the bottom, so they must reset to top
        aliens_reset_postion(aliens_get_state());
        
        // Reset Player to Center
        player_reset_position(); 

        // Redraw the ground which may have been partially erased by aliens
        draw_ground();

    }
}

void player_reset_position(void) {
    player_state* p = _pstate();
    p->player_x = PLAYER_X_POS;
}

// --- PLAYER MOVEMENT ---
void player_init(void) {
    // Set Sprite Pointer (use runtime bank base computed from `Sprites`)
    byte* screen_ptr_area = (byte*)(Screen + 1016);
    const byte VIC_BANK_BASE_PTR = (byte)(((unsigned)Sprites - 0x4000) >> 6);
    screen_ptr_area[0] = VIC_BANK_BASE_PTR + 0; // player image offset 0

    // Configure VIC-II Sprite 0
    vic.spr_enable   |= 1;           // Enable Sprite 0
    
    // Ensure Multicolor is OFF using the correct register name
    vic.spr_multi    &= ~1;          
    
    vic.spr_expand_x &= ~1;          // Normal Width (|= 1 for double width)
    vic.spr_expand_y &= ~1;          // Normal Height (|= 1 for double height)
    vic.spr_color[0]  = VCOL_GREEN;  

    // If this starts as 1, the ship is drawn off-screen (X+256)
    vic.spr_msbx     &= ~1;

    // Init State
    player_reset_position();
    player_render();

}

player_state* player_get_state(void) {
    return &s_player_state;
}

void player_update(void) {

    player_input_t input;
    player_input_update(&input);

    if (input.left) 
    {
        player_state* p = _pstate();
        if (p->player_x > MIN_X) {
                p->player_x -= PLAYER_SPEED;
            }
    }
    else if (input.right)
    {
        player_state* p = _pstate();
        if (p->player_x < MAX_X) {
            p->player_x += PLAYER_SPEED;
        }
    }

}

void player_render(void) {
    vic.spr_pos[0].y = PLAYER_Y_POS;

    player_state* p = _pstate();
    if (p->player_x > 255) {
        vic.spr_pos[0].x = (byte)(p->player_x & 0xFF);
        vic.spr_msbx |= 1; 
    } else {
        vic.spr_pos[0].x = (byte)p->player_x;
        vic.spr_msbx &= ~1;
    }
}