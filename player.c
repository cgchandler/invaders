#include "player.h"
#include <c64/vic.h>
#include <c64/types.h>
#include <c64/keyboard.h>

// --- CONFIGURATION ---
// Sprite pointer base is centralized in config.h as PLAYER_SPRITE_PTR
#define PLAYER_IMAGE_IDX    (PLAYER_SPRITE_PTR + 0)

// Top Border (50) + Row 22 (176) - Sprite Top Padding (6) = 220
#define PLAYER_Y_POS        222
#define PLAYER_SPEED        2    
#define MIN_X               24   
#define MAX_X               320  

// --- STATE ---
static player_state s_player_state = { .lives = 3, .default_lives = 3, .player_x = 172 };

static inline player_state* _pstate(void) { return &s_player_state; }

// Extern: We will create this in invaders.c shortly
extern void game_over(void);

void player_die(void) {
    player_state* p = _pstate();
    // Decrement Lives
    if (p->lives > 0) p->lives--;

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
        aliens_reset();
        
        // Reset Player to Center
        player_reset_position(); 

        // Redraw the ground which may have been partially erased by aliens
        draw_ground();

    }
}

void player_reset_position(void) {
    player_state* p = _pstate();
    p->player_x = 160;
}

// --- PLAYER MOVEMENT ---
void player_init(void) {
    // Set Sprite Pointer
    //byte* screen_ptr_area = (byte*)(0x4400 + 1016);
    byte* screen_ptr_area = (byte*)(Screen + 1016);
    screen_ptr_area[0] = PLAYER_IMAGE_IDX; 

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
    player_state* p = _pstate();
    p->player_x = 172; 
}

player_state* player_get_state(void) {
    return &s_player_state;
}

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
        player_state* p = _pstate();
        if (p->player_x > MIN_X) {
                p->player_x -= PLAYER_SPEED;
            }
    }

    // Handle RIGHT Movement
    // triggers on: 'D' OR 'Cursor Key (without Shift)'
    // We use 'else if' to prevent moving both ways if keys are mashed
    else if (key_pressed(KSCAN_D) || 
            (key_pressed(KSCAN_CSR_RIGHT) && !is_shifted)) 
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