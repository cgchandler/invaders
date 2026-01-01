#ifndef SOUNDS_H
#define SOUNDS_H

void sound_init(void);
void sound_update(void); // Call this every frame!

// Triggers
void sfx_fire_missile(void);
void sfx_alien_hit(void);
void sfx_player_die(void);
void sfx_game_over(void);
void sfx_high_score(void);
void sfx_march(void); // Call this when aliens move

#endif