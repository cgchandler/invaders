// © 2026 Christopher G Chandler
// Licensed under the MIT License. See LICENSE file in the project root. 

#ifndef BASES_H
#define BASES_H

#include <c64/types.h>
#include "config.h"

#define BASE_COUNT 4
#define BASE_WIDTH 5
#define BASE_DAMAGE_STAGES 3 /* number of visible damage stages before destroyed */
#define BASE_TOP_ROW 20   /* Top row index (row 21 visually) */
#define BASE_BOTTOM_ROW 21 /* Bottom row index (row 22 visually) */

/* Initialize bases for a level */
void bases_init(void);

/* Called each frame after starfield_render() to draw bases over stars */
void bases_render(void);

/* Check if a grid cell (col,row) contains a base block. If so, destroy that
   character and return 1, otherwise return 0. */
int bases_check_hit(unsigned char col, unsigned char row, bool destroy_on_hit);

#endif /* BASES_H */
