#ifndef BIGFONT_H
#define BIGFONT_H

#include <c64/types.h>

// Draw a single big character (5x5 blocks) at text-cell coords (x,y)
// ch is ASCII uppercase letter; color is VIC color constant
void draw_big_char_at(int x, int y, char ch, byte color);

// Draw a string of big characters. spacing is number of empty columns between letters (0..)
void draw_big_text_at(int x, int y, const char* text, byte color, int spacing);

#endif
