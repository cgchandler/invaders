#include "sounds.h"
#include <stdint.h>

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

#define SID_BASE ((volatile unsigned char*)0xD400)

// Voice 2 (UFO siren) registers
#define V2_FREQ_LO   0x07
#define V2_FREQ_HI   0x08
#define V2_PW_LO     0x09
#define V2_PW_HI     0x0A
#define V2_CTRL      0x0B
#define V2_AD        0x0C
#define V2_SR        0x0D

// Voice 3 (Explosion) registers
#define V3_FREQ_LO   0x0E
#define V3_FREQ_HI   0x0F
#define V3_PW_LO     0x10
#define V3_PW_HI     0x11
#define V3_CTRL      0x12
#define V3_AD        0x13
#define V3_SR        0x14

// Global SID registers
#define SID_FC_LO    0x15
#define SID_FC_HI    0x16
#define SID_RES_FILT 0x17

// Control bits
#define GATE_BIT     0x01
#define TRI_BIT      0x10
#define SAW_BIT      0x20
#define PULSE_BIT    0x40
#define NOISE_BIT    0x80

// --- STATE VARIABLES ---
typedef struct {
    unsigned char v1_timer;
    unsigned int  v1_freq;
    unsigned char v1_mode;

    unsigned char v2_timer;
    unsigned int  v2_freq;
    unsigned char v2_mode;

    unsigned char v3_timer;
    unsigned char march_step;

    unsigned char ufo_active;
    unsigned char ufo_tick;
    unsigned char ufo_pitch_dir;
} sounds_state;

static sounds_state s_sounds_state = {0};
static inline sounds_state* _sstate(void) { return &s_sounds_state; }
/*
static inline void sid_set_volume(unsigned char vol)
{
    SID_BASE[SID_MODE_VOL] =
        (SID_BASE[SID_MODE_VOL] & 0xF0) | (vol & 0x0F);
}
*/

static inline void sid_set_volume(unsigned char vol)
{
    SID_MODE_VOL = (SID_MODE_VOL & 0xF0) | (vol & 0x0F);
}

// --- INIT ---
void sound_init(void) {

    sid_set_volume(15);

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

// --- SOUND EFFECTS ---

void sfx_fire_missile(void) {
    // Hard reset gate (interrupt anything currently on V1)
    SID_V1_CTRL = 0;

    // Sweep setup
    sounds_state* s = _sstate();
    s->v1_mode  = 1;
    s->v1_freq  = 0x3000;   // start pitch
    s->v1_timer = 10;       // short, snappy

    //SID_V1_FREQ_LO = (unsigned char)(v1_freq & 0xFF);
    //SID_V1_FREQ_HI = (unsigned char)(v1_freq >> 8);
    SID_V1_FREQ_LO = (unsigned char)(s->v1_freq & 0xFF);
    SID_V1_FREQ_HI = (unsigned char)(s->v1_freq >> 8);

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
    sounds_state* s = _sstate();
    s->v1_mode = 0; 
    
    // 3. Setup Noise
    SID_V1_FREQ_LO = 0x00;
    SID_V1_FREQ_HI = 0x15; 
    SID_V1_AD = 0x05;      // Fast decay
    SID_V1_SR = 0x00;
    
    // 4. TRIGGER (NOISE)
    SID_V1_CTRL = NOISE | GATE;
    s->v1_timer = 8;
}

void sfx_player_die(void) {
    SID_V2_CTRL = 0; // Reset V2
    sounds_state* s = _sstate();
    s->v2_mode = 1;
    s->v2_freq = 0x1500; 
    s->v2_timer = 60;    
    SID_V2_CTRL = SAW | GATE; 
}

void sfx_game_over(void) {
    SID_V2_CTRL = 0; // Reset V2
    sounds_state* s = _sstate();
    s->v2_mode = 2;
    s->v2_freq = 0x2000;
    s->v2_timer = 120;   
    SID_V2_CTRL = TRI | GATE; 
}

void sfx_high_score(void) {
    SID_V1_CTRL = 0; // Reset V1
    sounds_state* s = _sstate();
    s->v1_mode = 0;
    SID_V1_FREQ_LO = 0x00;
    SID_V1_FREQ_HI = 0x60; 
    SID_V1_AD = 0x09;      
    SID_V1_SR = 0x00;

    SID_V1_CTRL = TRI | GATE;
    s->v1_timer = 20;
}

void sfx_march(void) {
    sounds_state* s = _sstate();
    unsigned int freq = (s->march_step) ? 0x0770 : 0x0970;
    s->march_step ^= 1;
    
    SID_V3_FREQ_LO = (unsigned char)(freq & 0xFF);
    SID_V3_FREQ_HI = (unsigned char)(freq >> 8);
    
    SID_V3_CTRL = 0; // Reset Gate
    SID_V3_CTRL = SAW | GATE; 
}

void sfx_ufo_start(void)
{
    sounds_state* s = _sstate();
    s->ufo_active = 1;
    s->ufo_tick = 0;

    sid_set_volume(15);

    // Voice 2 siren setup
    SID_V2_CTRL = 0;        // reset
    SID_V2_AD   = 0x11;     // quick attack, short decay
    SID_V2_SR   = 0xF2;     // sustain high, short release (keeps it present)

    // Start at an audible pitch immediately
    SID_V2_FREQ_LO = 0x00;
    SID_V2_FREQ_HI = 0x09;  // 0x0900

    // 50% pulse width-ish
    SID_V2_PW_LO = 0x00;
    SID_V2_PW_HI = 0x08;

    SID_V2_CTRL = PULSE | GATE;
}

void sfx_ufo_stop(void)
{
    sounds_state* s = _sstate();
    s->ufo_active = 0;

    // Hard stop Voice 2
    SID_V2_CTRL = 0;
}

static void sfx_ufo_update(void)
{
    sounds_state* s = _sstate();
    if (!s->ufo_active) return;

    s->ufo_tick++;

    // Toggle between two pitches every few frames for the "warble"
    // Tune these two for taste.
    if ((s->ufo_tick & 0x10) == 0) {
        SID_V2_FREQ_LO = 0x00;
        SID_V2_FREQ_HI = 0x09;  // 0x0900
    } else {
        SID_V2_FREQ_LO = 0x00;
        SID_V2_FREQ_HI = 0x0B;  // 0x0B00
    }

    // Small PWM wobble to add movement to the tone
    // (Avoids sounding like a static square wave)
    if ((s->ufo_tick & 0x08) == 0) {
        SID_V2_PW_LO = 0x00;
        SID_V2_PW_HI = 0x07;
    } else {
        SID_V2_PW_LO = 0x00;
        SID_V2_PW_HI = 0x09;
    }
}

void sfx_bonus_ship_hit(void)
{
    sid_set_volume(15);

    // Hard reset V3 so it retriggers cleanly
    SID_V3_CTRL = 0;

    // Big noise burst, but NO sustain so it doesn't hang forever
    SID_V3_AD = 0x09;   // A=0, D=9
    SID_V3_SR = 0x02;   // S=0, R=2 (no sustain)

    unsigned int freq = 0x1800;
    SID_V3_FREQ_LO = (unsigned char)(freq & 0xFF);
    SID_V3_FREQ_HI = (unsigned char)(freq >> 8);

    SID_V3_CTRL = NOISE | GATE;

    // Ensure gate gets shut off after a short time
    sounds_state* s = _sstate();
    s->v3_timer = 18;      // tweak 12..30 frames to taste
}

// --- UPDATE LOOP ---
void sound_update(void) {
    // --- VOICE 1/2/3 UPDATE (internal state) ---
    sounds_state* s = _sstate();

    // Voice 1
    if (s->v1_timer > 0) {
        s->v1_timer--;

        if (s->v1_mode == 1) {
            const unsigned int end_freq = 0x0A00;
            if (s->v1_freq > end_freq + 0x0080) {
                s->v1_freq -= 0x0220;
            } else {
                s->v1_freq = end_freq;
            }

            SID_V1_FREQ_LO = (unsigned char)(s->v1_freq & 0xFF);
            SID_V1_FREQ_HI = (unsigned char)(s->v1_freq >> 8);
        }

        if (s->v1_timer == 0) {
            SID_V1_CTRL = 0;
            s->v1_mode = 0;
        }
    }

    // Voice 2
    if (s->v2_timer > 0) {
        s->v2_timer--;

        if (s->v2_mode == 1) {
            if (s->v2_freq > 0x0300) s->v2_freq -= 0x0040;
            SID_V2_FREQ_LO = (unsigned char)(s->v2_freq & 0xFF);
            SID_V2_FREQ_HI = (unsigned char)(s->v2_freq >> 8);
        } else if (s->v2_mode == 2) {
            if (s->v2_freq > 0x0200) s->v2_freq -= 0x0010;
            unsigned int wobbly = s->v2_freq + ((s->v2_timer & 4) ? 0x0100 : 0);
            SID_V2_FREQ_LO = (unsigned char)(wobbly & 0xFF);
            SID_V2_FREQ_HI = (unsigned char)(wobbly >> 8);
        }

        if (s->v2_timer == 0) {
            SID_V2_CTRL = 0;
            s->v2_mode = 0;
        }
    }

    if (s->v2_timer == 0) sfx_ufo_update();

    // Voice 3
    if (s->v3_timer > 0) {
        s->v3_timer--;
        if (s->v3_timer == 0) SID_V3_CTRL = 0;
    }

}
