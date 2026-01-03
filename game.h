/* game.h
 * Centralized runtime game state accessor.
 */

#ifndef GAME_H
#define GAME_H

#include <c64/types.h>

typedef struct {
    unsigned int score;
    unsigned int shots_fired;
    unsigned int next_life_score;
    unsigned char level;
    unsigned char max_lives;
} game_state;

extern game_state g_game_state;

static inline game_state* game_get_state(void) { return &g_game_state; }

/* NOTE: old `g_*` macro aliases removed — use `game_get_state()` */

#endif /* GAME_H */
