#include <c64/vic.h>
#include <c64/types.h>
#include <stdlib.h>
#include <string.h>
#include "starfield.h"
#include "aliens.h"
#include "player.h"
#include "missile.h"
#include "bombs.h"
#include "sounds.h"
#include "gameover.h"
#include "bonus_ship.h"

// Top-level game state is in `game.h` / `game.c` (see `game_get_state()`)

// Character set
static const unsigned char charset[2048] = {
    #embed "charset/invaders_charset.bin"
};

// Sprite assets
static const unsigned char all_sprites_data[] = {
    #embed "sprites/invaders.bin" 
};

// Single Screen Buffer at $4400 (VIC Bank 1 offset $0400)
byte* Screen = (byte*)0x4400;
byte* const Font   = (byte*)0x5000;
byte* const Color  = (byte*)0xD800;

#define D018_SCREEN_4400_CHAR_5000 0x14

// --- SCREEN LAYOUT ---
#define ROW_23_OFFSET   (23 * 40) 
#define ROW_24_OFFSET   (24 * 40) 
#define CHAR_GROUND     64
#define CHAR_LIFE       157

static void vic_set_bank_4000(void)
{
    byte v = *(volatile byte*)0xDD00;
    v = (v & 0xFC) | 0x02;   // bank = 2 -> $4000-$7FFF
    *(volatile byte*)0xDD00 = v;
}

// --- SCORE STATE ---
/* Score stored in `g_game_state` defined in game.c */

void update_score_display(void)
{
    game_state* gs = game_get_state();

    if (gs->score >= gs->next_life_score) {
        gs->next_life_score += 1500;
        player_state* pstate = player_get_state();
        if (pstate->lives < gs->max_lives) {
            pstate->lives++;
            update_lives_display(); 
            sfx_high_score(); 
        }
    }

    unsigned int temp = gs->score;
    unsigned short offset = 17; 

    Screen[offset]     = 48 + (unsigned char)((temp / 100000) % 10);
    Screen[offset + 1] = 48 + (unsigned char)((temp / 10000) % 10);
    Screen[offset + 2] = 48 + (unsigned char)((temp / 1000) % 10);
    Screen[offset + 3] = 48 + (unsigned char)((temp / 100) % 10);
    Screen[offset + 4] = 48 + (unsigned char)((temp / 10) % 10);
    Screen[offset + 5] = 48 + (unsigned char)(temp % 10);

    for(int i=0; i<5; i++) Color[offset + i] = VCOL_WHITE;
}

extern void draw_ground(void)
{
    for (int i = 0; i < 40; i++) {
        Screen[ROW_23_OFFSET + i] = CHAR_GROUND;
        Color[ROW_23_OFFSET + i]  = VCOL_GREEN;
    }
}

static void screen_init(void)
{
    for (unsigned i = 0; i < 1000; i++)
        Screen[i] = ' '; 

    memset(Color, 1, 1000);

    draw_ground();

    update_score_display();
}

void update_lives_display(void)
{
    player_state* pstate = player_get_state();
    unsigned char reserves = (pstate->lives > 0) ? (pstate->lives - 1) : 0;

    game_state* gs = game_get_state();

    for (int i = 0; i < gs->max_lives; i++)
    {
        unsigned short pos = ROW_24_OFFSET + 3 + (i * 2);
        
        if (i < reserves) {
            Screen[pos]     = CHAR_LIFE;
            Screen[pos + 1] = CHAR_LIFE + 1;
            Color[pos]      = VCOL_GREEN; 
            Color[pos + 1]  = VCOL_GREEN; 
        } else {
            Screen[pos]     = 32; 
            Screen[pos + 1] = 32;
            Color[pos]      = VCOL_BLACK; 
            Color[pos + 1]  = VCOL_BLACK;
        }
    }
}

static void update_level(void)
{
    game_state* gs = game_get_state();
    unsigned char tens = gs->level / 10;
    unsigned char ones = gs->level % 10;

    Screen[ROW_24_OFFSET]     = tens + 48; 
    Color[ROW_24_OFFSET]      = VCOL_WHITE;

    Screen[ROW_24_OFFSET + 1] = ones + 48;
    Color[ROW_24_OFFSET + 1]  = VCOL_WHITE;
}

static void resources_init(void)
{
    vic_set_bank_4000();
    memcpy(Font, charset, 2048);
    vic_setmode(VICM_TEXT, Screen, Font);
    *(volatile unsigned char*)0xD018 = D018_SCREEN_4400_CHAR_5000;
    memcpy((void*)SPRITES_ADDR, all_sprites_data, sizeof(all_sprites_data));
}

static void random_init(void)
{
    unsigned seed;
    unsigned char cia_lo = *(volatile unsigned char*)0xDC04;
    unsigned char cia_hi = *(volatile unsigned char*)0xDC05;
    seed = ((unsigned)cia_hi << 8) | cia_lo;
    seed ^= vic.raster;
    srand(seed);
}

void clear_playfield(void) {
    for (int i = 40; i < 920; i++) {
        Screen[i] = ' ';
    }
}

void game_over(void) {
    game_over_sequence();

    game_state* gs = game_get_state();
    gs->score = 0;
    gs->next_life_score = 1500;
    update_score_display();
    
    gs->level = 1;
    update_level();
    
    player_state* pstate = player_get_state();
    pstate->lives = pstate->default_lives;
    gs->shots_fired = 0; 

    update_lives_display();
    clear_playfield();
    aliens_reset();
    bonus_reset(); 
    
    // player_reset_position(); 
}

void game_init(void) {
    vic.color_border = VCOL_BLACK;
    vic.color_back   = VCOL_BLACK;

    screen_init();
    resources_init();
    random_init();
    sound_init(); 

    update_lives_display();
    update_level();

    starfield_init(145, 20); 
    starfield_set_speed(2);          
    
    aliens_init();
    player_init();
    missile_init();
    bombs_init();
    bonus_init(); 

    player_state* pstate = player_get_state();
    pstate->lives = pstate->default_lives;
    game_get_state()->shots_fired = 0;
}

/* Forward declarations for loop-phase helpers */
static void game_input(void);
static void game_update(void);
static void game_render(void);

int main(void)
{
    game_init();

    for (;;)
    {
        game_input();
        game_update();
        game_render();
    }

    return 0;
}

// Split main responsibilities to improve readability and testability.
// - game_input(): handle input/polling only
// - game_update(): game logic updates (non-VBlank)
// - game_render(): VBlank wait + rendering routines

static void game_input(void)
{
    // Currently only player input is polled here. Move other input handling
    // here if needed in the future.
    player_update();
}

static void game_update(void)
{
    // --- LOGIC PHASE ---
    starfield_update_motion();
    aliens_update();
    missile_update();
    bombs_update();
    bonus_update();

    aliens_debug_speed();

    // --- LEVEL COMPLETION CHECK ---
    if (aliens_cleared()) {
        game_state* gs = game_get_state();
        gs->level++;
        if (gs->level > 99) gs->level = 1;
        update_level();

        clear_playfield();

        aliens_reset();
        missile_init();
        bombs_init();
        bonus_reset();

        gs->shots_fired = 0;
    }
}

static void game_render(void)
{
    // --- RENDER PHASE ---
    vic_waitFrame();

    starfield_render();
    aliens_render();
    player_render();
    missile_render();
    bombs_render();
    bonus_render();
}
