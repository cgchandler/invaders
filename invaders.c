#include <c64/vic.h>
#include <c64/types.h>
#include <stdlib.h>
#include <string.h>

#include "starfield.h"
#include "aliens.h"

// Character set
static const unsigned char charset[2048] = {
    #embed "charset/invaders_charset.bin"
};

// Single Screen Buffer at $4400 (VIC Bank 1 offset $0400)
byte* Screen = (byte*)0x4400;
byte* const Font   = (byte*)0x5000;
byte* const Color  = (byte*)0xD800;

#define D018_SCREEN_4400_CHAR_5000 0x14

static void vic_set_bank_4000(void)
{
    byte v = *(volatile byte*)0xDD00;
    v = (v & 0xFC) | 0x02;   // bank = 2 -> $4000-$7FFF
    *(volatile byte*)0xDD00 = v;
}

static void screen_init(void)
{
    // Clear the one and only screen
    for (unsigned i = 0; i < 1000; i++)
        Screen[i] = ' '; 

    // Init Color RAM (White)
    memset(Color, 1, 1000);
}

static void charset_install(void)
{
    vic_set_bank_4000();
    memcpy(Font, charset, 2048);
    vic_setmode(VICM_TEXT, Screen, Font);

    // Force pointer to our single screen
    *(volatile unsigned char*)0xD018 = D018_SCREEN_4400_CHAR_5000;
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

int main(void)
{
    vic.color_border = VCOL_BLACK;
    vic.color_back   = VCOL_BLACK;

    screen_init();
    charset_install();
    random_init();

    // Init Subsystems
    starfield_init(145, Screen, 20); // 20 stars
    starfield_set_speed(2);          // Slower stars
    
    aliens_init(Screen);

    for (;;)
    {
        // --- LOGIC PHASE (Safe to run anytime) ---
        // 1. Compute star motion
        starfield_update_motion(Screen);
        
        // 2. Compute alien motion and collision
        aliens_update();


        // --- RENDER PHASE (Must run in VBlank) ---
        // 3. Wait for Vertical Blank
        vic_waitFrame();

        //vic.color_border = VCOL_RED;

        // 4. Draw the starfield first
        starfield_render(Screen);

        // 5. Draw the aliens second
        aliens_render(Screen);       
        
        //vic.color_border = VCOL_BLACK;
    }

    return 0;
}