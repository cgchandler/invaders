// © 2026 Christopher G Chandler
// Licensed under the MIT License. See LICENSE file in the project root.

#ifndef PLAYER_INPUT_H
#define PLAYER_INPUT_H

#include <stdbool.h>

#define JOYSTICK_2 0
#define JOYSTICK_1 1

typedef struct {
    bool left;
    bool right;
    bool fire;   // one-shot (edge detected)
} player_input_t;

void player_input_poll(void);
void player_input_update(player_input_t* in);

#endif
