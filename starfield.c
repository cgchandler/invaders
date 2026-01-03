#include "starfield.h"
#include <c64/types.h>
#include <stdlib.h>   // rand

extern byte* const Color;

#include "config.h"

#define TOP_ROW     1
#define BOTTOM_ROW  20
#define SCREEN_COLS 40
#define STAR_FRAMES 8
#define STAR_OFF    32 

// Lookup table is now ONLY used in Update (Main Time), not Render (VBlank)
static const unsigned short row_offsets[26] = {
    0,   40,  80,  120, 160, 200, 240, 280, 320, 360,
    400, 440, 480, 520, 560, 600, 640, 680, 720, 760,
    800, 840, 880, 920, 960, 1000
};

static starfield_state static_starfield_state = { .char_base = 145, .speed_delay = 0, .speed_counter = 0, .active_stars = MAX_STARS };
// Inline accessor for local module use
static inline starfield_state* _sstate(void) { return &static_starfield_state; }

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
    starfield_state* s = _sstate();
    s->speed_delay = frames_per_step;
    s->speed_counter = 0;
}

void starfield_init(unsigned char char_base_index, unsigned char num_stars)
{
    starfield_state* s = _sstate();
    s->char_base = char_base_index;
    if (num_stars > MAX_STARS) num_stars = MAX_STARS;
    s->active_stars = num_stars;
    
    for (unsigned i = 0; i < s->active_stars; i++)
    {
        // Init Offsets to a safe dummy value
        s->next_pos[i] = 0; 

        // Find a spot
        for (int k=0; k<50; k++) 
        {
            unsigned char r = fast_rand_row();
            unsigned char c = fast_rand_col();
            unsigned short p = row_offsets[r] + c;
            
            if (is_screen_spot_free(p, Screen)) {
                s->next_pos[i] = p;
                break;
            }
        }
        
        s->next_phase[i] = (unsigned char)rand() & 7; 
        s->next_color[i] = star_random_color();
        
        // Sync State
        s->curr_pos[i] = s->next_pos[i];
        s->curr_phase[i] = s->next_phase[i];
        s->curr_color[i] = s->next_color[i];
    }
    
    starfield_render();
}

void starfield_update_motion()
{
    // STATE COMMIT (Moved from Render to here)
    // We copy Next -> Curr here, outside of VBlank.
    // This makes 'curr' the "Old" position (what is currently on screen)
    // and prepares 'next' to calculate the "New" position.
    starfield_state* s = _sstate();
    for(unsigned i=0; i<s->active_stars; i++) {
        s->curr_pos[i]   = s->next_pos[i];
        s->curr_phase[i] = s->next_phase[i];
        s->curr_color[i] = s->next_color[i];
    }

    // Speed Check
    if (s->speed_delay > 0) {
        s->speed_counter++;
        if (s->speed_counter < s->speed_delay) {
            // No movement: Next remains same as Curr.
            // Render will see (Curr == Next) and skip drawing.
            return; 
        }
        s->speed_counter = 0;
    }

    // Calculate New Positions
    unsigned short bottom_limit = row_offsets[BOTTOM_ROW] + SCREEN_COLS; // ~920

    for (unsigned i = 0; i < s->active_stars; i++)
    {
        unsigned char phase = s->curr_phase[i] + 1;
        unsigned short pos = s->curr_pos[i];
        
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
                    unsigned short p = row_offsets[r] + c;
                    
                    if (is_screen_spot_free(p, Screen)) {
                        pos = p;
                        s->next_color[i] = star_random_color();
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
        
                s->next_phase[i] = phase;
                s->next_pos[i]   = pos;
    }
}

// ULTRA-FAST RENDER (Runs in VBlank)
// No math, no copying, no checks unless necessary.
void starfield_render()
{
    // Define the range of characters that belong to stars
    // We only erase if we see one of these.
    starfield_state* s = _sstate();
    unsigned char star_char_min = s->char_base;
    unsigned char star_char_max = s->char_base + STAR_FRAMES; // Includes tail

    for (unsigned i = 0; i < s->active_stars; i++)
    {
        // Optimization: Skip if no change in pos or phase
        if (s->curr_pos[i] == s->next_pos[i] && s->curr_phase[i] == s->next_phase[i]) continue;

        // --- 1. POLITE ERASE (Don't delete Aliens!) ---
        unsigned short p = s->curr_pos[i];
        unsigned char c_on_screen = Screen[p];

        // Check: Is the thing on screen actually a star? 
        // If it's an Alien (range 128-139), this check fails and we don't erase it.
        if (c_on_screen >= star_char_min && c_on_screen <= star_char_max) {
             Screen[p] = STAR_OFF;
        }
        
        // Handle the tail (if it exists)
        if (s->curr_phase[i] == (STAR_FRAMES - 1)) {
             unsigned short p2 = p + SCREEN_COLS;
             if (p2 < 1000) {
                 c_on_screen = Screen[p2];
                 if (c_on_screen >= star_char_min && c_on_screen <= star_char_max) {
                    Screen[p2] = STAR_OFF;
                 }
             }
        }

        // --- 2. POLITE DRAW (Don't overwrite Aliens!) ---
        p = s->next_pos[i];
        
        // Check: Is the target spot empty?
        // Only draw if it is SPACE (STAR_OFF)
        if (Screen[p] == STAR_OFF) {
            unsigned char phase = s->next_phase[i];
            
            Screen[p] = (unsigned char)(s->char_base + phase);
            Color[p] = s->next_color[i];

            // Tail Draw
            if (phase == (STAR_FRAMES - 1)) {
                unsigned short p2 = p + SCREEN_COLS;
                // Check tail spot too!
                if (p2 < 1000 && Screen[p2] == STAR_OFF) {
                    Screen[p2] = (unsigned char)(s->char_base + STAR_FRAMES);
                    Color[p2] = s->next_color[i];
                }
            }
        }
    }
}

starfield_state* starfield_get_state(void)
{
    return &static_starfield_state;
}