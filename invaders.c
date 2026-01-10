#include <c64/vic.h>
#include <c64/types.h>
#include "player_input.h"
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
#include "bases.h"
#include "bigfont.h"
#include "leveldisplay.h"
#include <c64/joystick.h>
#include <c64/keyboard.h>

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

// --- GLOBAL SCREEN / FONT / SPRITE POINTERS ---
byte* const Screen  = (byte*)0x6000;  // 1K aligned
byte* const Sprites = (byte*)0x6400;  // reserve 1K for sprite shapes
byte* const Font    = (byte*)0x6800;  // 2K aligned (2KB charset)
byte* const Color   = (byte*)0xD800;   // Color RAM (not relocatable)

//#define D018_SCREEN_4400_CHAR_5000 0x14
#define D018_SCREEN_6000_CHAR_6800 0x8A

// --- SCREEN LAYOUT ---
#define ROW_23_OFFSET   920  // (23 * 40) 
#define ROW_24_OFFSET   960  // (24 * 40) 
#define CHAR_GROUND     64
#define CHAR_LIFE       157

static void vic_set_bank_4000(void)
{
    byte v = *(volatile byte*)0xDD00;
    v = (v & 0xFC) | 0x02;   // VIC bank base = $4000 (VIC sees $4000-$7FFF)
    *(volatile byte*)0xDD00 = v;
}

// --- SCORE STATE ---

void update_score_display(void)
{
    game_state* gs = game_get_state();

    // increase high score if needed
    if (!gs->demo) {
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
    Screen[offset + 3] = 48 + (unsigned char)((temp / 100) % 10);
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

/*
static void resources_init(void)
{
    vic_set_bank_4000();
    memcpy(Font, charset, 2048);
    vic_setmode(VICM_TEXT, Screen, Font);
    *(volatile unsigned char*)0xD018 = D018_SCREEN_4400_CHAR_5000;
    //memcpy((void*)SPRITES_ADDR, all_sprites_data, sizeof(all_sprites_data));
    memcpy(Sprites, all_sprites_data, sizeof(all_sprites_data));
}
*/
static void resources_init(void)
{
    vic_set_bank_4000();

    memcpy(Font, charset, 2048);

    // If vic_setmode already programs D018, you can rely on it.
    // If not, keep the explicit write below.
    vic_setmode(VICM_TEXT, Screen, Font);

    *(volatile unsigned char*)0xD018 = D018_SCREEN_6000_CHAR_6800;

    memcpy(Sprites, all_sprites_data, sizeof(all_sprites_data));

    // IMPORTANT: set sprite pointers (these live at Screen + 0x3F8)
    // Pointers are (sprite_address - bank_base) / 64.
    // Sprites at $6400 in bank $4000 => ($6400-$4000)/64 = $2400/64 = $90.
    /*
    volatile unsigned char* sprptr = (volatile unsigned char*)(0x6000 + 0x03F8);
    sprptr[0] = 0x90; // sprite 0 at $6400
    sprptr[1] = 0x91; // sprite 1 at $6440
    sprptr[2] = 0x92; // ...
    sprptr[3] = 0x93;
    sprptr[4] = 0x94;
    sprptr[5] = 0x95;
    sprptr[6] = 0x96;
    sprptr[7] = 0x97;
    */
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
//static game_mode_t g_mode; // defined/initialized later in file
static void intro_draw(void);
// Intro bonus toggle state: alternate between 50 and 300 every N frames
#define INTRO_TOGGLE_FRAMES 180 /* ~3 seconds at 60Hz */
static unsigned intro_bonus_timer = 0;
static unsigned char intro_bonus_state = 0; /* 0 => 50, 1 => 300 */
/* Idle timer used to trigger attract/demo mode when on the intro screen */
static unsigned intro_idle_timer = 0;
/* Demo runtime timer (counts frames while in MODE_DEMO) */
static unsigned demo_timer = 0;

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
    bases_init();
    // Return to intro screen
    gs->mode = MODE_INTRO;
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
    bases_init();
    player_init();
    missile_init();
    bombs_init();
    bonus_init(); 
}

// --- INTRO SCREEN ---
// Local game mode for intro vs play (defined above)

// Check for Joystick button
static int is_fire_pressed_local(void) {
    joy_poll(JOYSTICK_2);                // poll joystick port 2
    return joyb[JOYSTICK_2];             // joystick 2 button pressed
}

// Direct Hardware Scan for Spacebar
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
    byte* sprite_ptrs = (byte*)(Screen + 1016);
    // Disable all sprites to avoid artifacts from previous modes (missiles/bombs)
    vic.spr_enable = 0;
    // Also ensure expansion bit for bonus is cleared before reconfiguring
    vic.spr_expand_x &= ~(1 << 7);

    // Reinitialize missile/bomb state so any active sprite data is cleared
    missile_init();
    bombs_init();
    bonus_reset();
    const int BONUS_SPRITE_INDEX = 7;
    const byte VIC_BANK_BASE_PTR = (byte)(((unsigned)Sprites - 0x4000) >> 6); // /64
    const byte BONUS_PTR_OFFSET  = 2; // same meaning as before
    sprite_ptrs[BONUS_SPRITE_INDEX] = (byte)(VIC_BANK_BASE_PTR + BONUS_PTR_OFFSET);

    // place and show sprite
    vic.spr_color[BONUS_SPRITE_INDEX] = VCOL_RED;
    vic.spr_expand_x |= (1 << BONUS_SPRITE_INDEX);
    vic.spr_expand_y &= ~(1 << BONUS_SPRITE_INDEX);
    // Position (Y will be aligned with bonus text row after we choose base_row)
    vic.spr_pos[BONUS_SPRITE_INDEX].x = 136; // center X (approx)
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

static void game_input(void)
{
    // Currently only player input is polled here. Move other input handling
    // here if needed in the future.
    player_input_poll();
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
        bases_init();
        missile_init();
        bombs_init();
        bonus_reset();

        // Reset Player Position
        player_init();

        // Redraw the ground which may have been partially erased by aliens
        draw_ground();        

        gs->shots_fired = 0;

        // display the level number sequence if we're on the game play screen
        if (gs->mode == MODE_PLAY) {
            gs->mode = MODE_LEVEL_DISPLAY;
        }
    }
}

static void game_render(void)
{
    // --- RENDER PHASE ---
    vic_waitFrame();

    starfield_render();
    bases_render();
    aliens_render();
    player_render();
    missile_render();
    bombs_render();
    bonus_render();
    draw_ground();

#if DEBUG_INFO_ENABLED
    /* Respect compile-time debug flag — no-op when disabled */
    // output control method to screen for debugging
    game_state* gs = game_get_state();
    draw_custom_text(24, 36, (gs->control == JOYSTICK) ? "J" : "K", VCOL_WHITE);
#endif
}

int main(void)
{
    game_init();

    // Start in intro mode
    game_state* gs = game_get_state();
    gs->mode = MODE_INTRO;
    // Initial draw
    intro_draw();

    for (;;)
    {
        /* Track previous mode to detect transitions — only run the
         * level display when we just transitioned from PLAY to
         * LEVEL_DISPLAY. This avoids accidental invocation while in
         * other modes (e.g. INTRO).
         */
        game_mode_t prev_mode = gs->mode;

        if (gs->mode == MODE_INTRO) {
            int fire_pressed = is_fire_pressed_local();
            int space_pressed = 0;
            if (!fire_pressed) {
                space_pressed = is_space_pressed_local();
            }
            if (fire_pressed || space_pressed) {

                /* Reset idle timer since user started a game */
                intro_idle_timer = 0;
                // Set control method based on input used to start game
                gs->control = (fire_pressed) ? JOYSTICK : KEYBOARD;

                // Full game reset so a play started after demo begins fresh
                gs->score = 0;
                gs->next_life_score = 1500;
                gs->shots_fired = 0;
                gs->level = 1;

                // Reset player lives to defaults
                player_state* pstate = player_get_state();
                pstate->lives = pstate->default_lives;

                // Prepare playfield and modules
                screen_init();
                update_score_display();
                update_lives_display();
                update_level();

                // Reinitialize game entities
                clear_playfield();
                aliens_init();
                bases_init();
                player_init();
                missile_init();
                bombs_init();
                bonus_init();

                // Disable intro-only sprites and ensure player sprite is on
                vic.spr_expand_x &= ~(1 << 7);
                vic.spr_enable &= ~(1 << 7);
                vic.spr_enable |= 1;

                // Show level display immediately, then enter PLAY
                level_display_sequence();
                gs->mode = MODE_PLAY;

            } else {
                /* No input — increment idle timer and possibly start demo */
                intro_update();
                intro_render();

                intro_idle_timer++;
                if (intro_idle_timer >= INTRO_DEMO_TIMEOUT_FRAMES) {
                    /* Auto-start demo/game: prepare playfield then enter DEMO mode */
                    intro_idle_timer = 0;

                    // Use keyboard as default control for demo
                    gs->control = KEYBOARD;

                    screen_init();
                    update_lives_display();
                    update_level();

                    player_init();
                    missile_init();
                    bombs_init();
                    bonus_init();

                    vic.spr_expand_x &= ~(1 << 7);
                    vic.spr_enable &= ~(1 << 7);
                    vic.spr_enable |= 1;

                    // Begin demo play
                    gs->mode = MODE_DEMO;
                    gs->demo = 1;
                    demo_timer = 0;
                }
            }

        } else {
            /* If we're in DEMO mode, allow user input to abort back to intro
             * and track demo runtime for automatic return to intro. */
            if (gs->mode == MODE_DEMO) {
                int fire_pressed = is_fire_pressed_local();
                int space_pressed = 0;
                if (!fire_pressed) space_pressed = is_space_pressed_local();
                if (fire_pressed || space_pressed) {
                    /* Abort demo and return to intro immediately */
                    gs->mode = MODE_INTRO;
                    gs->demo = 0;
                    intro_idle_timer = 0;
                    demo_timer = 0;
                    intro_draw();
                    continue;
                }
            }

            game_input();
            game_update();

            if (gs->mode == MODE_DEMO) {
                demo_timer++;
                if (demo_timer >= INTRO_DEMO_TIMEOUT_FRAMES) {
                    /* End demo and return to intro */
                    gs->demo = 0;
                    gs->mode = MODE_INTRO;
                    gs->score = 0;
                    gs->next_life_score = 1500;
                    gs->shots_fired = 0;
                    update_score_display();
                    intro_draw();
                    intro_idle_timer = 0;
                    demo_timer = 0;
                    continue;
                }
            }

            /* Only perform the modal sequence when the mode changed from
             * PLAY -> LEVEL_DISPLAY this frame. This prevents accidental
             * modal runs when other code (or corruption) temporarily
             * writes the mode while we're on the intro screen.
             */
            if (gs->mode == MODE_LEVEL_DISPLAY && (prev_mode == MODE_PLAY || prev_mode == MODE_INTRO)) {
                /* Snapshot sprite pointer table to detect accidental overwrites
                 * during the modal sequence. The screen pointer table lives
                 * at Screen+1016; save 8 entries used by sprites.
                 */
                byte* screen_ptr_area = (byte*)(Screen + 1016);
                byte spr_before[8];
                for (int i = 0; i < 8; i++) spr_before[i] = screen_ptr_area[i];

                level_display_sequence();
                gs->mode = MODE_PLAY;

            }
           
            if (gs->mode == MODE_PLAY || gs->mode == MODE_DEMO) {
                game_render();
            } else {
                /* Any other non-play mode (including MODE_INTRO) should
                 * draw the intro screen.
                 */
                intro_render();
            }

            // update the sound system every frame
            sound_update();
        }
    }

    return 0;
}