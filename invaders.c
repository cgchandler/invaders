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

// Access to Score in invaders.c
extern unsigned int g_score;
static unsigned int g_next_life_score = 1500;
extern void update_score_display(void);
extern void update_lives_display(void);

// --- GAME STATE ---
extern unsigned char g_lives;
extern unsigned char g_default_lives;
unsigned char g_max_lives = 18;
unsigned char g_level = 1;

// Shot Counter for Bonus Ship Logic
unsigned int g_shots_fired = 0;

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
#define SPRITES_ADDR 0x4800
#define PLAYER_SPRITE_PTR   32

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
unsigned int g_score = 0; 

void update_score_display(void)
{
    if (g_score >= g_next_life_score) {
        g_next_life_score += 1500;
        if (g_lives < g_max_lives) {
            g_lives++;
            update_lives_display(); 
            sfx_high_score(); 
        }
    }

    unsigned int temp = g_score;
    unsigned short offset = 17; 

    Screen[offset]     = 48 + (unsigned char)((temp / 100000) % 10);
    Screen[offset + 1] = 48 + (unsigned char)((temp / 10000) % 10);
    Screen[offset + 2] = 48 + (unsigned char)((temp / 1000) % 10);
    Screen[offset + 3] = 48 + (unsigned char)((temp / 100) % 10);
    Screen[offset + 4] = 48 + (unsigned char)((temp / 10) % 10);
    Screen[offset + 5] = 48 + (unsigned char)(temp % 10);

    for(int i=0; i<5; i++) Color[offset + i] = VCOL_WHITE;
}

static void screen_init(void)
{
    for (unsigned i = 0; i < 1000; i++)
        Screen[i] = ' '; 

    memset(Color, 1, 1000);

    for (int i = 0; i < 40; i++) {
        Screen[ROW_23_OFFSET + i] = CHAR_GROUND;
        Color[ROW_23_OFFSET + i]  = VCOL_GREEN;
    }

    update_score_display();
}

void update_lives_display(void)
{
    unsigned char reserves = (g_lives > 0) ? (g_lives - 1) : 0;

    for (int i = 0; i < g_max_lives; i++)
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
    unsigned char tens = g_level / 10;
    unsigned char ones = g_level % 10;

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

    g_score = 0;
    g_next_life_score = 1500;
    update_score_display();
    
    g_level = 1;
    update_level();
    
    g_lives = g_default_lives;
    g_shots_fired = 0; 

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

    starfield_init(145, Screen, 20); 
    starfield_set_speed(2);          
    
    aliens_init(Screen);
    player_init();
    missile_init();
    bombs_init();
    bonus_init(); 

    g_lives = g_default_lives;
    g_shots_fired = 0;
}

int main(void)
{
    game_init();

    for (;;)
    {
        // --- LOGIC PHASE ---
        starfield_update_motion(Screen);
        aliens_update();
        player_update();
        missile_update();
        bombs_update();
        bonus_update(); 
        
        aliens_debug_speed(); 

        // --- LEVEL COMPLETION CHECK ---
        if (aliens_cleared()) {
            g_level++;
            if (g_level > 99) g_level = 1; 
            update_level();

            clear_playfield();

            aliens_reset();
            missile_init(); 
            bombs_init();
            bonus_reset(); 
            
            g_shots_fired = 0; 
        }
        
        // --- RENDER PHASE ---
        vic_waitFrame();

        starfield_render(Screen);
        aliens_render(Screen);       
        player_render();
        missile_render();
        bombs_render();
        bonus_render(); 
    }

    return 0;
}