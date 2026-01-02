#ifndef BONUS_SHIP_H
#define BONUS_SHIP_H

// Initializes the bonus ship system
void bonus_init(void);

// Resets the ship state (e.g. on new level or game over)
void bonus_reset(void);

// Handles movement, state changes, and spawning logic
void bonus_update(void);

// Draws the sprite to the screen
void bonus_render(void);

// checks collision with missile (returns points awarded, 0 if miss)
int bonus_check_hit(int m_col, int m_row);

#endif