#include "player_input.h"
#include <c64/keyboard.h>
#include <c64/joystick.h>
#include "game.h"
#include "config.h"
#include <stdlib.h>
#include "player.h"

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
    if (gs->mode == MODE_DEMO) {
        /* Demo movement: pick a random target X every DEMO_MOVE_INTERVAL frames
         * and move toward it — this avoids getting stuck bouncing between two
         * fixed points.
         */
        static int demo_dir = 1; /* 1 = right, -1 = left */
        static unsigned demo_move_timer = 0;
        static int demo_target_x = -1;
        demo_move_timer++;
        player_state* pstate = player_get_state();
        int px = (int)pstate->player_x;

        if (demo_target_x < 0 || demo_move_timer >= DEMO_MOVE_INTERVAL) {
            demo_move_timer = 0;
            /* choose a random target within allowed demo bounds */
            int range = DEMO_MAX_X - DEMO_MIN_X + 1;
            demo_target_x = DEMO_MIN_X + (rand() % range);
        }

        if (px < demo_target_x - 2) {
            left = 0;
            right = 1;
            demo_dir = 1;
        } else if (px > demo_target_x + 2) {
            left = 1;
            right = 0;
            demo_dir = -1;
        } else {
            /* reached target — clear target to pick a new one next interval */
            left = 0;
            right = 0;
            demo_target_x = -1;
        }

        fire = false; /* missile.c handles demo auto-fire */
        in->left = left;
        in->right = right;
        in->fire = fire;
        return;
    }
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