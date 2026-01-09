#include "game.h"

/* Define the canonical game-state structure. Other modules should use
 * `game_get_state()` or the `g_score`/`g_level` aliases while migration
 * continues.
 */
game_state g_game_state = {
    .score = 0,
    .high_score = 0,
    .shots_fired = 0,
    .next_life_score = 1500,
    .level = 1,
    .max_lives = 18,
    .mode = MODE_INTRO,
    .control = KEYBOARD,
    .demo = 0
};

game_state* game_get_state(void) {
    return &g_game_state;
}
