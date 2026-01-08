#include "player_input.h"
#include <c64/keyboard.h>
#include <c64/joystick.h>
#include "game.h"

// Use the Oscar64 joystick/keyboard helpers to avoid directly manipulating CIA
// state. This prevents accidental corruption of CIA registers which can
// destabilize the system and drop back to BASIC.

// Poll the current input method (joystick or keyboard) based on game state
// This should be called once per frame before calling player_input_update()
void player_input_poll(void)
{
    game_state* gs = game_get_state();
    if (gs->control == JOYSTICK) {
        // poll the joystick state
        joy_poll(JOYSTICK_2);
    }
    else {
        // poll the keyboard state
        keyb_poll();
    }
}

void player_input_update(player_input_t* in)
{

    int left = 0;
    int right = 0;
    bool fire = false;

    game_state* gs = game_get_state();
    if (gs->control == JOYSTICK) {
        // Determine left/right/fire from joystick
        left  = (joyx[JOYSTICK_2] < 0);
        right = (joyx[JOYSTICK_2] > 0);
        fire = joyb[JOYSTICK_2];
    }
    else {
        // Check for shifted keys
        int is_shifted = key_pressed(KSCAN_SHIFT_LOCK) || key_pressed(KSCAN_RSHIFT);
        // Determine left/right/fire from keys
        left = key_pressed(KSCAN_A) || key_pressed(KSCAN_ARROW_LEFT) ||
                       (key_pressed(KSCAN_CSR_RIGHT) && is_shifted);
        right = key_pressed(KSCAN_D) || (key_pressed(KSCAN_CSR_RIGHT) && !is_shifted);
        fire = key_pressed(KSCAN_SPACE);
    }

    in->left = left;
    in->right = right;
    in->fire = fire;

}