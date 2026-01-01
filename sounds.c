#include "sounds.h"

// --- SID REGISTERS ---
#define SID_V1_FREQ_LO  (*(volatile unsigned char*)0xD400)
#define SID_V1_FREQ_HI  (*(volatile unsigned char*)0xD401)
#define SID_V1_PW_LO    (*(volatile unsigned char*)0xD402)
#define SID_V1_PW_HI    (*(volatile unsigned char*)0xD403)
#define SID_V1_CTRL     (*(volatile unsigned char*)0xD404)
#define SID_V1_AD       (*(volatile unsigned char*)0xD405)
#define SID_V1_SR       (*(volatile unsigned char*)0xD406)

#define SID_V2_FREQ_LO  (*(volatile unsigned char*)0xD407)
#define SID_V2_FREQ_HI  (*(volatile unsigned char*)0xD408)
#define SID_V2_PW_LO    (*(volatile unsigned char*)0xD409)
#define SID_V2_PW_HI    (*(volatile unsigned char*)0xD40A)
#define SID_V2_CTRL     (*(volatile unsigned char*)0xD40B)
#define SID_V2_AD       (*(volatile unsigned char*)0xD40C)
#define SID_V2_SR       (*(volatile unsigned char*)0xD40D)

#define SID_V3_FREQ_LO  (*(volatile unsigned char*)0xD40E)
#define SID_V3_FREQ_HI  (*(volatile unsigned char*)0xD40F)
#define SID_V3_CTRL     (*(volatile unsigned char*)0xD412)
#define SID_V3_AD       (*(volatile unsigned char*)0xD413)
#define SID_V3_SR       (*(volatile unsigned char*)0xD414)

#define SID_MODE_VOL    (*(volatile unsigned char*)0xD418)

// Control Bits
#define GATE  0x01
#define SYNC  0x02
#define RING  0x04
#define TEST  0x08
#define TRI   0x10
#define SAW   0x20
#define PULSE 0x40
#define NOISE 0x80

// --- STATE VARIABLES ---
// Voice 1 (Shooting/Hits)
static unsigned char v1_timer = 0; 
static unsigned int  v1_freq  = 0;  
static unsigned char v1_mode  = 0;  // 0=Static (Hit), 1=Slide (Missile)

// Voice 2 (Death/Game Over)
static unsigned char v2_timer = 0; 
static unsigned int  v2_freq  = 0; 
static unsigned char v2_mode  = 0; 

// Voice 3 (Marching)
static unsigned char march_step = 0; 

// --- INIT ---
void sound_init(void) {
    SID_MODE_VOL = 0x0F; 

    // V1 Defaults
    SID_V1_AD = 0x00; 
    SID_V1_SR = 0x00; 
    
    // V2 Defaults
    SID_V2_AD = 0x09; 
    SID_V2_SR = 0x00;

    // V3 Defaults
    SID_V3_AD = 0x05; 
    SID_V3_SR = 0x00;
}

// --- UPDATE LOOP ---
void sound_update(void) {
    
    // --- VOICE 1 UPDATE (Missile/Hits) ---
    if (v1_timer > 0) {
        v1_timer--;
        
        // Mode 1: Missile Pew
        if (v1_mode == 1) {
            // Controlled downward sweep
            const unsigned int end_freq = 0x0A00;

            if (v1_freq > end_freq + 0x0080) {
                //v1_freq -= 0x0180;   // tweak this for speed of drop
                v1_freq -= 0x0220;
            } else {
                v1_freq = end_freq;
            }

            SID_V1_FREQ_LO = (unsigned char)(v1_freq & 0xFF);
            SID_V1_FREQ_HI = (unsigned char)(v1_freq >> 8);
        }

        if (v1_timer == 0) {
            SID_V1_CTRL = 0; // Gate Off
            v1_mode = 0;     // Reset mode
        }
    }

    // --- VOICE 2 UPDATE (Death/Game Over) ---
    if (v2_timer > 0) {
        v2_timer--;
        
        // Mode 1: Player Death
        if (v2_mode == 1) {
            if (v2_freq > 0x0300) v2_freq -= 0x0040; 
            SID_V2_FREQ_LO = (unsigned char)(v2_freq & 0xFF);
            SID_V2_FREQ_HI = (unsigned char)(v2_freq >> 8);
        }
        // Mode 2: Game Over
        else if (v2_mode == 2) {
             if (v2_freq > 0x0200) v2_freq -= 0x0010;
             unsigned int wobbly = v2_freq + ((v2_timer & 4) ? 0x0100 : 0);
             SID_V2_FREQ_LO = (unsigned char)(wobbly & 0xFF);
             SID_V2_FREQ_HI = (unsigned char)(wobbly >> 8);
        }

        if (v2_timer == 0) {
            SID_V2_CTRL = 0; 
            v2_mode = 0;
        }
    }
}

// --- SOUND EFFECTS ---

void sfx_fire_missile(void) {
    // Hard reset gate (interrupt anything currently on V1)
    SID_V1_CTRL = 0;

    // Sweep setup
    v1_mode  = 1;
    //v1_freq  = 0x2400;   // start pitch
    v1_freq  = 0x3000;   // start pitch
    v1_timer = 10;       // short, snappy

    //SID_V1_FREQ_LO = (unsigned char)(v1_freq & 0xFF);
    //SID_V1_FREQ_HI = (unsigned char)(v1_freq >> 8);
    SID_V1_FREQ_LO = (unsigned char)(v1_freq & 0xFF);
    SID_V1_FREQ_HI = (unsigned char)(v1_freq >> 8);

    // Pulse width (12-bit). Try 0x0200 to 0x0800 range.
    // This is important for the "zap" character.
    //SID_V1_PW_LO = 0x00;
    //SID_V1_PW_HI = 0x02; // 0x0200
    SID_V1_PW_LO = 0x02;
    SID_V1_PW_HI = 0x01; // 0x0200

    // Envelope: instant attack, quick decay, no sustain, short release
    // AD: Attack=0, Decay=4  (0x04)
    // SR: Sustain=0, Release=2 (0x02)
    SID_V1_AD = 0x04;
    SID_V1_SR = 0x02;

    // Pulse wave + Gate
    SID_V1_CTRL = PULSE | GATE;
    //SID_V1_CTRL = SAW | GATE;
}

void sfx_alien_hit(void) {
    // 1. HARD RESET GATE (Interrupts missile if needed)
    SID_V1_CTRL = 0;

    // 2. Disable Slide Mode (So update loop leaves us alone)
    v1_mode = 0; 
    
    // 3. Setup Noise
    SID_V1_FREQ_LO = 0x00;
    SID_V1_FREQ_HI = 0x15; 
    SID_V1_AD = 0x05;      // Fast decay
    SID_V1_SR = 0x00;
    
    // 4. TRIGGER (NOISE)
    SID_V1_CTRL = NOISE | GATE;
    v1_timer = 8;
}

void sfx_player_die(void) {
    SID_V2_CTRL = 0; // Reset V2
    v2_mode = 1;
    v2_freq = 0x1500; 
    v2_timer = 60;    
    SID_V2_CTRL = SAW | GATE; 
}

void sfx_game_over(void) {
    SID_V2_CTRL = 0; // Reset V2
    v2_mode = 2;
    v2_freq = 0x2000;
    v2_timer = 120;   
    SID_V2_CTRL = TRI | GATE; 
}

void sfx_high_score(void) {
    SID_V1_CTRL = 0; // Reset V1
    v1_mode = 0;
    SID_V1_FREQ_LO = 0x00;
    SID_V1_FREQ_HI = 0x60; 
    SID_V1_AD = 0x09;      
    SID_V1_SR = 0x00;

    SID_V1_CTRL = TRI | GATE;
    v1_timer = 20;
}

void sfx_march(void) {
    // Use your preferred frequencies
    unsigned int freq = (march_step) ? 0x0770 : 0x0970;
    march_step ^= 1;
    
    SID_V3_FREQ_LO = (unsigned char)(freq & 0xFF);
    SID_V3_FREQ_HI = (unsigned char)(freq >> 8);
    
    SID_V3_CTRL = 0; // Reset Gate
    SID_V3_CTRL = SAW | GATE; 
}