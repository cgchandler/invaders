#include "starfield.h"
#include <c64/types.h>
#include <stdlib.h>   // rand

extern byte* const Color;

#define TOP_ROW     1
#define BOTTOM_ROW  22
#define SCREEN_COLS 40
#define STAR_FRAMES 8
#define STAR_OFF    32 
#define MAX_STARS   50 

// Lookup table is now ONLY used in Update (Main Time), not Render (VBlank)
static const unsigned short g_row_offsets[26] = {
    0,   40,  80,  120, 160, 200, 240, 280, 320, 360,
    400, 440, 480, 520, 560, 600, 640, 680, 720, 760,
    800, 840, 880, 920, 960, 1000
};

static unsigned char g_char_base = 145;
static unsigned char g_speed_delay = 0;
static unsigned char g_speed_counter = 0;
static unsigned char g_active_stars = MAX_STARS;

// STATE ARRAYS
// We now store the Raw Screen Offset (0-999) instead of Row/Col.
// This allows the Render loop to just "Point and Shoot".
static unsigned short g_curr_pos[MAX_STARS];
static unsigned char  g_curr_phase[MAX_STARS];
static unsigned char  g_curr_color[MAX_STARS];

static unsigned short g_next_pos[MAX_STARS];
static unsigned char  g_next_phase[MAX_STARS];
static unsigned char  g_next_color[MAX_STARS];

// --- FAST HELPERS ---

static unsigned char star_random_color(void)
{
    static const unsigned char palette[] = { 1, 15, 14, 7, 8, 3, 5 };
    unsigned char r = (unsigned char)rand() & 127; 
    if (r < 80) return palette[r & 3]; 
    return palette[3 + (r & 3)]; 
}

static unsigned char fast_rand_col(void) {
    unsigned char c;
    do { c = (unsigned char)rand() & 63; } while (c >= 40);
    return c;
}

static unsigned char fast_rand_row(void) {
    unsigned char r;
    do { r = (unsigned char)rand() & 31; } while (r < TOP_ROW || r > BOTTOM_ROW); 
    return r;
}

// Check collision using direct screen peek
static int is_screen_spot_free(unsigned short pos, byte* screen)
{
    if (pos >= 1000) return 0;
    if (screen[pos] != STAR_OFF) return 0;
    if (pos < 960 && screen[pos + SCREEN_COLS] != STAR_OFF) return 0;
    return 1;
}

// --- API ---

void starfield_set_speed(unsigned char frames_per_step)
{
    g_speed_delay = frames_per_step;
    g_speed_counter = 0;
}

void starfield_init(unsigned char char_base_index, byte* target_screen, unsigned char num_stars)
{
    g_char_base = char_base_index;
    if (num_stars > MAX_STARS) num_stars = MAX_STARS;
    g_active_stars = num_stars;
    
    for (unsigned i = 0; i < g_active_stars; i++)
    {
        // Init Offsets to a safe dummy value
        g_next_pos[i] = 0; 

        // Find a spot
        for (int k=0; k<50; k++) 
        {
            unsigned char r = fast_rand_row();
            unsigned char c = fast_rand_col();
            unsigned short p = g_row_offsets[r] + c;
            
            if (is_screen_spot_free(p, target_screen)) {
                g_next_pos[i] = p;
                break;
            }
        }
        
        g_next_phase[i] = (unsigned char)rand() & 7; 
        g_next_color[i] = star_random_color();
        
        // Sync State
        g_curr_pos[i] = g_next_pos[i];
        g_curr_phase[i] = g_next_phase[i];
        g_curr_color[i] = g_next_color[i];
    }
    
    starfield_render(target_screen);
}

void starfield_update_motion(byte* screen)
{
    // 1. STATE COMMIT (Moved from Render to here)
    // We copy Next -> Curr here, outside of VBlank.
    // This makes 'curr' the "Old" position (what is currently on screen)
    // and prepares 'next' to calculate the "New" position.
    for(unsigned i=0; i<g_active_stars; i++) {
        g_curr_pos[i]   = g_next_pos[i];
        g_curr_phase[i] = g_next_phase[i];
        g_curr_color[i] = g_next_color[i];
    }

    // 2. Speed Check
    if (g_speed_delay > 0) {
        g_speed_counter++;
        if (g_speed_counter < g_speed_delay) {
            // No movement: Next remains same as Curr.
            // Render will see (Curr == Next) and skip drawing.
            return; 
        }
        g_speed_counter = 0;
    }

    // 3. Calculate New Positions
    unsigned short bottom_limit = g_row_offsets[BOTTOM_ROW] + SCREEN_COLS; // ~920

    for (unsigned i = 0; i < g_active_stars; i++)
    {
        unsigned char phase = g_curr_phase[i] + 1;
        unsigned short pos = g_curr_pos[i];
        
        if (phase >= STAR_FRAMES)
        {
            phase = 0;
            unsigned short new_pos = pos + SCREEN_COLS; // Move Down 40 bytes

            if (new_pos >= bottom_limit)
            {
                // Respawn Logic
                for (int k=0; k<10; k++) {
                    unsigned char r = TOP_ROW;
                    unsigned char c = fast_rand_col();
                    unsigned short p = g_row_offsets[r] + c;
                    
                    if (is_screen_spot_free(p, screen)) {
                        pos = p;
                        g_next_color[i] = star_random_color();
                        break;
                    }
                }
            }
            else
            {
                // Blind Move Down (Fastest)
                pos = new_pos;
            }
        }
        
        g_next_phase[i] = phase;
        g_next_pos[i]   = pos;
    }
}

// ULTRA-FAST RENDER (Runs in VBlank)
// No math, no copying, no checks unless necessary.
void starfield_render(byte* target_screen)
{
    // Define the range of characters that belong to stars
    // We only erase if we see one of these.
    unsigned char star_char_min = g_char_base;
    unsigned char star_char_max = g_char_base + STAR_FRAMES; // Includes tail

    for (unsigned i = 0; i < g_active_stars; i++)
    {
        // Optimization: Skip if no change in pos or phase
        if (g_curr_pos[i] == g_next_pos[i] && g_curr_phase[i] == g_next_phase[i]) continue;

        // --- 1. POLITE ERASE (Don't delete Aliens!) ---
        unsigned short p = g_curr_pos[i];
        unsigned char c_on_screen = target_screen[p];

        // Check: Is the thing on screen actually a star? 
        // If it's an Alien (range 128-139), this check fails and we don't erase it.
        if (c_on_screen >= star_char_min && c_on_screen <= star_char_max) {
             target_screen[p] = STAR_OFF;
        }
        
        // Handle the tail (if it exists)
        if (g_curr_phase[i] == (STAR_FRAMES - 1)) {
             unsigned short p2 = p + SCREEN_COLS;
             if (p2 < 1000) {
                 c_on_screen = target_screen[p2];
                 if (c_on_screen >= star_char_min && c_on_screen <= star_char_max) {
                    target_screen[p2] = STAR_OFF;
                 }
             }
        }

        // --- 2. POLITE DRAW (Don't overwrite Aliens!) ---
        p = g_next_pos[i];
        
        // Check: Is the target spot empty?
        // Only draw if it is SPACE (STAR_OFF)
        if (target_screen[p] == STAR_OFF) {
            unsigned char phase = g_next_phase[i];
            
            target_screen[p] = (unsigned char)(g_char_base + phase);
            Color[p] = g_next_color[i];

            // Tail Draw
            if (phase == (STAR_FRAMES - 1)) {
                unsigned short p2 = p + SCREEN_COLS;
                // Check tail spot too!
                if (p2 < 1000 && target_screen[p2] == STAR_OFF) {
                    target_screen[p2] = (unsigned char)(g_char_base + STAR_FRAMES);
                    Color[p2] = g_next_color[i];
                }
            }
        }
    }
}