#include "draw.h"

#ifdef TEST_DRAW
// ./a.out | ffmpeg -f rawvideo -s 1280x720 -pix_fmt bgra -i - -y picture.bmp
int main(void) {
  if (draw_init(1280, 720) == -1)
    return -1;
  { // draw here
    COLOR c1 = BGRA(0x00, 0x00, 0x00, 255);
    COLOR c2 = BGRA(0xff, 0xff, 0xff, 255);
    for (ui16 i = 0, y = 0; y < 720; y += 80, i++)
      for (ui16 j = 0, x = 0; x < 1280; x += 80, j++)
        draw_block(x, y, x + 80, y + 80, ((i + j) % 2) ? c1 : c2);
    draw_block(40, 40, 1280 - 40, 720 - 40, BGRA(0, 255, 0, 127));
    draw_block(21, 21, 59, 59, BGRA(0, 0, 255, 127));
    draw_xline(20, 20, 60, BGRA(0, 0, 127, 127));
    draw_xline(59, 20, 60, BGRA(0, 0, 127, 127));
    draw_yline(20, 21, 59, BGRA(0, 0, 127, 127));
    draw_yline(59, 21, 59, BGRA(0, 0, 127, 127));
    draw_yline(10, 10, 1280 - 10, BGRA(0, 0, 128, 128));
    COLOR rb[] = {BGRA(255, 0, 0, 255), BGRA(255, 165, 0, 255),
                  BGRA(255, 255, 0, 255), BGRA(0, 128, 0, 255),
                  BGRA(0, 0, 255, 255), BGRA(75, 0, 130, 255),
                  BGRA(238, 130, 238, 255)};
    for (ui16 i = 0, y = 120; i < 7; i++, y += 18)
      draw_block(120, y, 120 + 224, y + 18, rb[i]);
    for (ui16 i = 0, y = 130; i < 7; i++, y += 18)
      draw_block(130, y, 130 + 224, y + 18, rb[i]);
  }
  fwrite(frame.mem, 1, frame.len, stdout);
  draw_free();
  return 0;
}
#endif

struct draw_frame frame;
static struct {
  ui32 *offs;
  byte *data;
} font;

int draw_free(void) {
  free(font.offs);
  free(frame.mem);
  return 0;
}
int draw_init(ui16 frame_w, ui16 frame_h) {
  frame.w = frame_w;
  frame.h = frame_h;
  frame.len = sizeof(COLOR) * frame_w * frame_h;
  if ((frame.mem = malloc(frame.len)) == NULL)
    return -1;
  int ret = -1, tmp_errno = 0;
  FILE *fp = fopen("unifont.bin", "rb");
  if (fp != NULL) {
    long length;
    if (fseek(fp, 0, SEEK_END) == 0 && (length = ftell(fp)) != -1 && fseek(fp, 0, SEEK_SET) == 0) {
      if ((font.offs = malloc(length)) != NULL) {
        if (fread(font.offs, 1, length, fp) == (size_t)length) {
          font.data = ((byte *)font.offs) + (sizeof(ui32) * 65536);
          ret = 0;
        } else
          tmp_errno = errno, free(font.offs);
      } else
        tmp_errno = errno;
    } else
      tmp_errno = errno;
    fclose(fp);
  } else
    tmp_errno = errno;
  errno = tmp_errno;
  if (ret)
    free(frame.mem);
  return ret;
}
inline void draw_xline(ui16 xx, ui16 yu, ui16 yd, COLOR clr) {
  if (xx > frame.w || yu > frame.h || yu >= yd)
    return;
  if (yd > frame.h)
    yd = frame.h;
  ui16 dst, src;
  const ui16 alp = clr.bs[3];
  COLOR *pix = frame.mem + yu * frame.w + xx;
  if (alp == 255) {
    for (ui16 y = yu; y < yd; y++)
      *pix = clr, pix += frame.w;
  } else
    for (ui16 y = yu; y < yd; y++) {
      for (int i = 0; i < 3; i++) {
        src = clr.bs[i], dst = pix->bs[i];
        pix->bs[i] = src * alp / 255 + dst * (255 - alp) / 255;
      }
      pix->bs[3] = 255;
      pix += frame.w;
    }
}
inline void draw_yline(ui16 yy, ui16 xl, ui16 xr, COLOR clr) {
  if (yy > frame.h || xl > frame.w || xl >= xr)
    return;
  if (xr > frame.w)
    xr = frame.w;
  ui16 dst, src;
  const ui16 alp = clr.bs[3];
  COLOR *pix = frame.mem + yy * frame.w + xl;
  if (alp == 255) {
    for (ui16 x = xl; x < xr; x++)
      *(pix++) = clr;
  } else
    for (ui16 x = xl; x < xr; x++) {
      for (int i = 0; i < 3; i++) {
        src = clr.bs[i], dst = pix->bs[i];
        pix->bs[i] = src * alp / 255 + dst * (255 - alp) / 255;
      }
      pix->bs[3] = 255;
      pix++;
    }
}
inline void draw_block(ui16 xl, ui16 yu, ui16 xr, ui16 yd, COLOR clr) {
  if (xl > frame.w || yu > frame.h || xl >= xr || yu >= yd)
    return;
  if (xr > frame.w)
    xr = frame.w;
  if (yd > frame.h)
    yd = frame.h;
  ui16 dst, src;
  const ui16 alp = clr.bs[3], line_step = frame.w + xl - xr;
  COLOR *pix = frame.mem + yu * frame.w + xl;
  if (alp == 255)
    for (ui16 y = yu; y < yd; y++) {
      for (ui16 x = xl; x < xr; x++)
        *(pix++) = clr;
      pix += line_step;
    }
  else {
    for (ui16 y = yu; y < yd; y++) {
      for (ui16 x = xl; x < xr; x++) {
        for (int i = 0; i < 3; i++) {
          src = clr.bs[i], dst = pix->bs[i];
          pix->bs[i] = src * alp / 255 + dst * (255 - alp) / 255;
        }
        pix->bs[3] = 255;
        pix++;
      }
      pix += line_step;
    }
  }
}

static inline byte draw_char(ui16 rune, si16 dx, si16 dy, COLOR fg, COLOR bg) {
  byte charWidth = 1;
  if (font.offs[rune] == 0xffffffff)
    rune = '?';
  ui32 offset = font.offs[rune];
  if (offset & (1 << 31)) {
    charWidth = 2;
    offset &= ~(1 << 31);
  }
  si16 x, y;
  ui16 dst, src;
  byte *charData = font.data + offset, currByte;
  for (ui16 fy = 0; fy < TEXT_LINE_HEIGHT; fy++) {
    for (ui16 bi = 0, fx = 0; bi < charWidth; bi++) {
      currByte = *(charData++);
      for (ui16 bx = 0; bx < 8; bx++, fx++)
        if ((x = dx + fx) >= 0 && (y = dy + fy) >= 0)
          if (x < frame.w && y < frame.h) {
            COLOR *pix = frame.mem + y * frame.w + x;
            COLOR clr = (currByte & (1 << (7 - bx))) ? fg : bg;
            const ui16 alp = clr.bs[3];
            for (int i = 0; i < 3; i++) {
              src = clr.bs[i], dst = pix->bs[i];
              pix->bs[i] = src * alp / 255 + dst * (255 - alp) / 255;
            }
            pix->bs[3] = 255;
          }
    }
  }
  return charWidth * 8;
}
inline void draw_utf8(const char *text, si16 dx, si16 dy, COLOR fg, COLOR bg) {
  byte *p = (byte *)text;
  ui16 rune = 0, left = dx;
  while (*p) {
    rune = 0;
    byte b1 = *p;
    p++;
    if ((b1 & 0x80) == 0) {
      rune = b1;
    } else {
      byte b2 = *p;
      p++;
      if (b2 == 0) {
        break;
      }
      if ((b1 & 0xE0) == 0xC0) {
        rune = ((b1 & 0x1F) << 6) | (b2 & 0x3F);
      } else {
        byte b3 = *p;
        p++;
        if (b3 == 0) {
          break;
        }
        if ((b1 & 0xf0) == 0xE0) {
          rune =
              ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        } else {
          break;
        }
      }
    }
    if (rune == '\n')
      dx = left, dy += TEXT_LINE_HEIGHT;
    else {
      byte charWidth = draw_char(rune, dx, dy, fg, bg);
      dx += charWidth;
    }
  }
}
