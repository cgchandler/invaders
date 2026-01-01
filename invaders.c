#include <c64/vic.h>
#include <c64/types.h>
#include <stdlib.h>
#include <string.h>
#include "starfield.h"
#include "aliens.h"
#include "player.h"
#include "missile.h"
#include "sounds.h"

// Access to Score in invaders.c
extern unsigned int g_score;
extern void update_score_display(void);
extern void update_lives_display(void);

// --- GAME STATE ---
extern unsigned char g_lives = 3;
unsigned char g_max_lives = 18;
unsigned char g_level = 1;

// Character set
static const unsigned char charset[2048] = {
    #embed "charset/invaders_charset.bin"
};

// Sprite assets
// Contains: [Player] (and later [Bonus], [Missile], [Explosion])
// Currently 64 bytes, but will grow automatically.
static const unsigned char all_sprites_data[] = {
    #embed "sprites/invaders.bin" 
};

// Single Screen Buffer at $4400 (VIC Bank 1 offset $0400)
byte* Screen = (byte*)0x4400;
byte* const Font   = (byte*)0x5000;
byte* const Color  = (byte*)0xD800;

#define D018_SCREEN_4400_CHAR_5000 0x14

// Your Screen is at 0x4400. It ends at 0x47E8 (1000 chars).
// 0x4800 is a safe, aligned spot right after the screen.
#define SPRITES_ADDR 0x4800

// Calculate the Sprite Pointer Index
// The VIC sees offsets from the start of the Bank (0x4000).
// Offset = 0x4800 - 0x4000 = 0x0800.
// Pointer = 0x0800 / 64 bytes = 32.
#define PLAYER_SPRITE_PTR   32

// --- SCREEN LAYOUT ---
#define ROW_23_OFFSET   (23 * 40) // 920: The "Ground" line
#define ROW_24_OFFSET   (24 * 40) // 960: The HUD / Lives row
#define CHAR_GROUND     64
#define CHAR_LIFE       157

static void vic_set_bank_4000(void)
{
    byte v = *(volatile byte*)0xDD00;
    v = (v & 0xFC) | 0x02;   // bank = 2 -> $4000-$7FFF
    *(volatile byte*)0xDD00 = v;
}

// --- SCORE STATE ---
unsigned int g_score = 0; // Global Score (0 - 65535)

// Display 5 digits at Row 0, centered (Offset 18)
// NOT static, so aliens.c can call it.
void update_score_display(void)
{
    unsigned int temp = g_score;
    unsigned short offset = 18; // Approx center of top row

    // Extract digits (Ten-Thousands down to Ones)
    // 0 + 48 is ASCII '0'
    Screen[offset]     = 48 + (unsigned char)((temp / 10000) % 10);
    Screen[offset + 1] = 48 + (unsigned char)((temp / 1000) % 10);
    Screen[offset + 2] = 48 + (unsigned char)((temp / 100) % 10);
    Screen[offset + 3] = 48 + (unsigned char)((temp / 10) % 10);
    Screen[offset + 4] = 48 + (unsigned char)(temp % 10);

    // Set Color to White
    for(int i=0; i<5; i++) Color[offset + i] = VCOL_WHITE;
}

static void screen_init(void)
{
    // Clear the one and only screen
    for (unsigned i = 0; i < 1000; i++)
        Screen[i] = ' '; 

    // Init Color RAM (White)
    memset(Color, 1, 1000);

    // Draw the Ground Line
    for (int i = 0; i < 40; i++) {
        Screen[ROW_23_OFFSET + i] = CHAR_GROUND;
        Color[ROW_23_OFFSET + i]  = VCOL_GREEN;
    }

    update_score_display();
}

void update_lives_display(void)
{
    // Calculate Reserve Count (Protect against 0 lives underflow)
    unsigned char reserves = (g_lives > 0) ? (g_lives - 1) : 0;

    // Draw Player Lives in Reserve
    // We loop through g_max_lives to ensure we clear empty slots too
    for (int i = 0; i < g_max_lives; i++)
    {
        unsigned short pos = ROW_24_OFFSET + 3 + (i * 2);
        
        if (i < reserves) {
            // DRAW ICON
            Screen[pos]     = CHAR_LIFE;
            Screen[pos + 1] = CHAR_LIFE + 1;
            Color[pos]      = VCOL_GREEN; 
            Color[pos + 1]  = VCOL_GREEN; 
        } else {
            // CLEAR ICON (Empty Slot)
            Screen[pos]     = 32; // Space
            Screen[pos + 1] = 32;
            Color[pos]      = VCOL_BLACK; // Good practice to hide color too
            Color[pos + 1]  = VCOL_BLACK;
        }
    }
}

static void update_level(void)
{
    // Calculate digits
    unsigned char tens = g_level / 10;
    unsigned char ones = g_level % 10;

    // Draw Tens Digit (e.g., the '0' in "01")
    Screen[ROW_24_OFFSET]     = tens + 48; 
    Color[ROW_24_OFFSET]      = VCOL_WHITE;

    // Draw Ones Digit (e.g., the '1' in "01")
    Screen[ROW_24_OFFSET + 1] = ones + 48;
    Color[ROW_24_OFFSET + 1]  = VCOL_WHITE;
}

static void resources_init(void)
{
    vic_set_bank_4000();

    // 1. Install Charset
    memcpy(Font, charset, 2048);
    vic_setmode(VICM_TEXT, Screen, Font);
    *(volatile unsigned char*)0xD018 = D018_SCREEN_4400_CHAR_5000;

    // 2. Install Sprites
    // Copies the ENTIRE sheet to 0x4800. 
    // As "invaders.bin" grows, this automatically copies the new sprites.
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

// Helper to clear just the playfield (Rows 1-22)
// leaving the Score (Row 0), Ground (Row 23) and HUD (Row 24) intact.
void clear_playfield(void) {
    // 22 rows * 40 columns = 920 characters
    for (int i = 40; i < 920; i++) {
        Screen[i] = ' ';
    }
}

// This handles the "Hard Reset" when lives == 0
void game_over(void) {
    // 1. Reset Game State
    g_score = 0;
    update_score_display();
    
    g_level = 1;
    update_level();
    
    g_lives = 3;
    
    update_lives_display();

    // 2. Clean Visuals
    clear_playfield(); // Erase old alien graphics from bottom

    // 3. Reset Entities
    aliens_reset(); // Back to top, full speed reset
    
    // Reset Player X (We can manually set it or add a player_reset func)
    // Assuming you have access to g_player_x or a reset function:
    player_reset_position();
}

int main(void)
{
    vic.color_border = VCOL_DARK_GREY;
    vic.color_back   = VCOL_BLACK;

    screen_init();
    resources_init();
    random_init();
    sound_init(); 

    update_lives_display();
    update_level();

    // Init Subsystems
    starfield_init(145, Screen, 20); // 20 stars
    starfield_set_speed(2);          // Slower stars
    
    aliens_init(Screen);
    player_init();
    missile_init();

    for (;;)
    {
        // --- LOGIC PHASE (Safe to run anytime) ---
        starfield_update_motion(Screen);
        aliens_update();
        player_update();
        missile_update();

        // --- LEVEL COMPLETION CHECK ---
        if (aliens_cleared()) {
            
            // Increase Level
            g_level++;
            if (g_level > 99) g_level = 1; // Wrap around if needed
            update_level();

            // Clean Screen
            // We must wipe the old aliens from the bottom before resetting
            clear_playfield();

            // Reset Systems
            aliens_reset();
            
            // Kill active missile so you don't instantly snipe a new alien
            missile_init(); 
            
            // Refresh the 
            // Brief delay could go here
        }
        
        // --- RENDER PHASE (Must run in VBlank) ---
        vic_waitFrame();

        //vic.color_border = VCOL_RED;
        starfield_render(Screen);
        aliens_render(Screen);       
        player_render();
        missile_render();
        //vic.color_border = VCOL_BLACK;
    }

    return 0;
}