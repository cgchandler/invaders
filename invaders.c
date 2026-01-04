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
#include "bigfont.h"

// Top-level game state is in `game.h` / `game.c` (see `game_get_state()`)

// Character set
static const unsigned char charset[2048] = {
// compiler directive to ignore for intellisense parsing which isn't recognizing #embed
#ifdef __INTELLISENSE__
    0
#else
    #embed "charset/invaders_charset.bin"
#endif
};

// Sprite assets
static const unsigned char all_sprites_data[] = {
// compiler directive to ignore for intellisense parsing which isn't recognizing #embed
#ifdef __INTELLISENSE__
    0
#else
    #embed "sprites/invaders.bin"
#endif
};

// Single Screen Buffer at $4400 (VIC Bank 1 offset $0400)
byte* Screen = (byte*)0x4400;
byte* const Font   = (byte*)0x5000;
byte* const Color  = (byte*)0xD800;

#define D018_SCREEN_4400_CHAR_5000 0x14

// --- SCREEN LAYOUT ---
#define ROW_23_OFFSET   920  // (23 * 40) 
#define ROW_24_OFFSET   960  // (24 * 40) 
#define CHAR_GROUND     64
#define CHAR_LIFE       157

static void vic_set_bank_4000(void)
{
    byte v = *(volatile byte*)0xDD00;
    v = (v & 0xFC) | 0x02;   // bank = 2 -> $4000-$7FFF
    *(volatile byte*)0xDD00 = v;
}

// --- SCORE STATE ---

void update_score_display(void)
{
    game_state* gs = game_get_state();

    // increase high score if needed
    if (gs->score > gs->high_score) {
        gs->high_score = gs->score;
    }
    
    // Check for extra life
    if (gs->score >= gs->next_life_score) {
        gs->next_life_score += 1500;
        player_state* pstate = player_get_state();
        if (pstate->lives < gs->max_lives) {
            pstate->lives++;
            update_lives_display(); 
            sfx_high_score(); 
        }
    }

    // Display Score at Row 0, Col 12 (6 chars)
    unsigned int temp = gs->score;
    unsigned short offset = 6; 

    Screen[offset]     = 48 + (unsigned char)((temp / 100000) % 10);
    Screen[offset + 1] = 48 + (unsigned char)((temp / 10000) % 10);
    Screen[offset + 2] = 48 + (unsigned char)((temp / 1000) % 10);
    Screen[offset + 3] = 48 + (unsigned char)((temp / 100) % 10);
    Screen[offset + 4] = 48 + (unsigned char)((temp / 10) % 10);
    Screen[offset + 5] = 48 + (unsigned char)(temp % 10);

    // Display High Score at Row 0, Col 22 (6 chars)
    temp = gs->high_score;
    offset = 28;
    Screen[offset]     = 48 + (unsigned char)((temp / 100000) % 10);
    Screen[offset + 1] = 48 + (unsigned char)((temp / 10000) % 10);
    Screen[offset + 2] = 48 + (unsigned char)((temp / 1000) % 10);
    Screen[offset + 3] = 48 + (unsigned char)((temp / 100   ) % 10);
    Screen[offset + 4] = 48 + (unsigned char)((temp / 10) % 10);
    Screen[offset + 5] = 48 + (unsigned char)(temp % 10);    
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

    //for (unsigned i = 0; i < 1000; i++)
    //    Screen[i] = ' '; 

    // set all screen chars to space
    memset(Screen, ' ', 1000);

    // set text color to white for entire screen
    memset(Color, VCOL_WHITE, 1000);

    // Display the game name "INVADERS" at Row 0, centered at Col 16
    const char* title = "INVADERS";
    unsigned short offset = 16; 
    for (unsigned char i = 0; title[i] != 0; i++) {
        Screen[offset + i] = title[i] - 'A' + 1;
        Color[offset + i]  = VCOL_YELLOW;
    }                   
    
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

    //DEBUGGING CODE - DISPLAY RESERVES & MAX LIVES AS NUMBERS
    /*
    unsigned short pos2 = ROW_24_OFFSET + 20;
    unsigned char tens = reserves / 10;
    unsigned char ones = reserves % 10;
    Screen[pos2]     = tens + 48; 
    Color[pos2]      = VCOL_WHITE;
    Screen[pos2 + 1] = ones + 48;
    Color[pos2 + 1]  = VCOL_WHITE;

    pos2 = ROW_24_OFFSET + 25;
    tens = gs->max_lives / 10;
    ones = gs->max_lives % 10;
    Screen[pos2]     = tens + 48; 
    Color[pos2]      = VCOL_WHITE;
    Screen[pos2 + 1] = ones + 48;
    Color[pos2 + 1]  = VCOL_WHITE;
    */
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

// --- INTRO SCREEN MODE (forward declarations) ---
typedef enum { MODE_INTRO = 0, MODE_PLAY = 1 } game_mode_t;
static game_mode_t g_mode; // defined/initialized later in file
static void intro_draw(void);
// Intro bonus toggle state: alternate between 50 and 300 every N frames
#define INTRO_TOGGLE_FRAMES 180 /* ~3 seconds at 60Hz */
static unsigned intro_bonus_timer = 0;
static unsigned char intro_bonus_state = 0; /* 0 => 50, 1 => 300 */

/* Position for bonus value on intro screen (see intro_draw layout for details).
The bonus value text is rendered at row 14, column 22. */
static const unsigned char INTRO_BONUS_ROW = 14;
static const unsigned char INTRO_BONUS_COL = 22;

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
    // Return to intro screen
    g_mode = MODE_INTRO;
    intro_draw();
}

void game_init(void) {
    vic.color_border = VCOL_BLACK;
    vic.color_back   = VCOL_BLACK;

    player_state* pstate = player_get_state();
    pstate->lives = pstate->default_lives;
    game_state* gs = game_get_state();    
    // Ensure top-level game state is explicitly initialized here so
    // display code called during startup sees the expected values.
    gs->score = 0;
    gs->high_score = 0;
    gs->shots_fired = 0;
    gs->next_life_score = 1500;
    gs->level = 1;
    gs->max_lives = 18;

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
}

// --- INTRO SCREEN ---
// Local game mode for intro vs play (defined above)

// Direct Hardware Scan for Spacebar (same matrix as missile.c)
static int is_space_pressed_local(void) {
    *(volatile byte*)0xDC00 = 0x7F; 
    return ((*(volatile byte*)0xDC01) & 0x10) == 0;
}

// Helper: draw text using custom font where 'A' == 1
static void draw_custom_text(unsigned char row, unsigned char col, const char* text, byte color) {
    unsigned short offset = (row * 40) + col;
    for (unsigned i = 0; text[i]; i++) {
        char c = text[i];
        if (c >= 'A' && c <= 'Z') {
            Screen[offset + i] = (unsigned char)(c - 'A' + 1);
        } else if (c == ' ') {
            Screen[offset + i] = 32;
        } else {
            Screen[offset + i] = (unsigned char)c;
        }
        Color[offset + i] = color;
    }
}

static void intro_draw(void) {
    // Ensure row 0 score/title is up-to-date
    update_score_display();

    // Clear playfield area (rows 1..22)
    for (int r = 1; r < 23; r++) {
        unsigned short off = r * 40;
        for (int c = 0; c < 40; c++) {
            Screen[off + c] = ' ';
            Color[off + c] = VCOL_WHITE;
        }
    }

    // Large block-letter title (4x5) - draw "INVADERS" with 1-column spacing and centered
    // Each letter is 4 cols wide; with spacing=1 total width = 8*4 + 7*1 = 39 -> start X = (40-39)/2 = 0
    draw_big_text_at(0, 3, "INVADERS", VCOL_YELLOW, 1);

    // Author name - row 8
    draw_custom_text(10, 13, "COPYRIGHT 2026", VCOL_CYAN);
    draw_custom_text(12, 13, "CHRIS CHANDLER", VCOL_CYAN);

    // Bonus ship sprite: enable sprite 7, red, expanded X, positioned center
    // Sprite pointer for bonus in sprites is BASE_SPRITE_PTR + BONUS_PTR_OFFSET (see bonus_ship.c)
    // Using hardcoded value from module: BASE_SPRITE_PTR(32) + BONUS_PTR_OFFSET(2) = 34
    byte* sprite_ptrs = (byte*)(Screen + 1016);
    const int BONUS_SPRITE_INDEX = 7;
    sprite_ptrs[BONUS_SPRITE_INDEX] = 34;
    vic.spr_color[BONUS_SPRITE_INDEX] = VCOL_RED;
    vic.spr_expand_x |= (1 << BONUS_SPRITE_INDEX);
    vic.spr_expand_y &= ~(1 << BONUS_SPRITE_INDEX);
    // Position (Y will be aligned with bonus text row after we choose base_row)
    vic.spr_pos[BONUS_SPRITE_INDEX].x = 136;  //120;
    vic.spr_msbx &= ~(1 << BONUS_SPRITE_INDEX);
    vic.spr_enable |= (1 << BONUS_SPRITE_INDEX);

    // Draw three alien types and their point values (30,20,10)
    // Use the same graphic bytes as the aliens module (frame 0)
    unsigned short base_row = 16;
    unsigned short col = 16;

     /* Now that we know where the bonus label will be drawn, align sprite Y
         to the same text row so the ship appears level with the text. */
     unsigned short bonus_text_row = base_row - 2;
    // Text rows are 8 pixels high on the C64 text screen
    vic.spr_pos[BONUS_SPRITE_INDEX].y = 50 + (bonus_text_row * 8);

    // Type 0 (30 pts) - chars 132,133 color LT_RED
    unsigned short off = base_row * 40 + col;
    Screen[off] = 132; 
    Screen[off + 1] = 133; 
    Color[off] = VCOL_LT_RED; 
    Color[off+1] = VCOL_LT_RED;
    draw_custom_text(base_row, INTRO_BONUS_COL, "30", VCOL_LT_RED);

    // Type 1 (20 pts) - chars 130,131 color YELLOW
    off = (base_row + 2) * 40 + col;
    Screen[off] = 130;
    Screen[off + 1] = 131; 
    Color[off] = VCOL_YELLOW; 
    Color[off+1] = VCOL_YELLOW;
    draw_custom_text(base_row + 2, INTRO_BONUS_COL, "20", VCOL_YELLOW);

    // Type 2 (10 pts) - chars 128,129 color GREEN
    off = (base_row + 4) * 40 + col;
    Screen[off] = 128; 
    Screen[off + 1] = 129; 
    Color[off] = VCOL_GREEN; 
    Color[off + 1] = VCOL_GREEN;
    draw_custom_text(base_row + 4, INTRO_BONUS_COL, "10", VCOL_GREEN);

    // Bonus points value (alternate between 50 and 300)
    draw_custom_text(INTRO_BONUS_ROW, INTRO_BONUS_COL, intro_bonus_state ? "300" : "50 ", VCOL_RED);

    // Draw the ground
    draw_ground();
}

static void intro_update(void) {
    // Keep starfield moving
    starfield_update_motion();
    // Update score row in case score/high score changed
    update_score_display();
    // Toggle bonus display every INTRO_TOGGLE_FRAMES frames
    intro_bonus_timer++;
    if (intro_bonus_timer >= INTRO_TOGGLE_FRAMES) {
        intro_bonus_timer = 0;
        intro_bonus_state = !intro_bonus_state;
        // Redraw the small numeric label
        draw_custom_text(INTRO_BONUS_ROW, INTRO_BONUS_COL, intro_bonus_state ? "300" : "50 ", VCOL_RED);
    }
}

static void intro_render(void) {
    // VBlank
    vic_waitFrame();

    // Render starfield
    starfield_render();

    // Hide the player sprite while on intro
    vic.spr_enable &= ~1; // Sprite 0 off

    // Render other static screen elements (aliens/bonus were written directly to Screen)
    // Ensure bonus sprite is shown (already configured in intro_draw)
}

/* Forward declarations for loop-phase helpers */
static void game_input(void);
static void game_update(void);
static void game_render(void);

int main(void)
{
    game_init();

    // Start in intro mode
    g_mode = MODE_INTRO;

    // Initial draw
    intro_draw();

    for (;;)
    {
        if (g_mode == MODE_INTRO) {
            // Poll input specifically for starting the game
            if (is_space_pressed_local()) {
                // cLear intro elements
                screen_init();
                update_lives_display();
                update_level();                
                // Disable intro-only sprites
                vic.spr_expand_x &= ~(1 << 7);
                vic.spr_enable &= ~(1 << 7);
                // Re-enable player sprite now that play begins
                vic.spr_enable |= 1;
                // Enter play mode
                g_mode = MODE_PLAY;
            } else {
                intro_update();
                intro_render();
            }
        } else {
            game_input();
            game_update();
            // If game_update() switched to intro (e.g. via game_over()),
            // avoid rendering the gameplay frame which would draw aliens
            // over the intro. Render according to the current mode.
            if (g_mode == MODE_PLAY) {
                game_render();
            } else {
                // We switched to intro mid-frame; render intro instead.
                intro_render();
            }
        }
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
