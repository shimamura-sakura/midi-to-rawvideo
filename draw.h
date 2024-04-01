#pragma once
#include "util.h"

typedef union {
  ui32 color;
  byte bs[4];
} COLOR;

#define RGB(R, G, B) RGBA(R, G, B, 0xff)
#define RGBA(R, G, B, A) (COLOR)(ui32)(((A) << 24) + ((R) << 16) + ((G) << 8) + (B))

extern struct draw_frame {
  ui16 w, h;
  COLOR *mem;
  size_t len;
} frame;

int draw_init(ui16 frame_w, ui16 frame_h);
int draw_free(void);

void draw_xline(ui16 xx, ui16 yu, ui16 yd, COLOR clr);
void draw_yline(ui16 yy, ui16 xl, ui16 xr, COLOR clr);
void draw_block(ui16 xl, ui16 yu, ui16 xr, ui16 yd, COLOR clr);

void draw_utf8(const char *text, si16 dx, si16 dy, COLOR fg, COLOR bg);
#define TEXT_LINE_HEIGHT 16