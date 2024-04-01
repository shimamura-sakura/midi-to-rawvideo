#include "main.h"

volatile bool video_running;
struct g_vparams_t g_vparams;
struct g_vconsts_t g_vconsts;

static byte *framebuffer;
static struct {
  tk_t screen_height;
  bool draw_keyboard;
  ui16 keyboard_h, blackkey_h;
  ui16 frame_w, frame_h, fps_up, fps_dn;
  bool draw_barborder;
  size_t tckk_poolsize;
  size_t bars_poolsize;
  size_t queu_poolsize;
  bool enable_text;
  bool filename_hint;
  bool enable_metalist;
} param;
static bool check_params(void) {
  if (PARAM_SET(g_vparams, frame_w) &&
      PARAM_SET(g_vparams, frame_h) &&
      PARAM_SET(g_vparams, fps_up) &&
      PARAM_SET(g_vparams, fps_dn) &&
      PARAM_SET(g_vparams, screen_height) &&
      PARAM_SET(g_vparams, draw_barborder)) {
    param.frame_w = PARAM_VAL(g_vparams, frame_w);
    param.frame_h = PARAM_VAL(g_vparams, frame_h);
    param.fps_up = PARAM_VAL(g_vparams, fps_up);
    param.fps_dn = PARAM_VAL(g_vparams, fps_dn);
    param.screen_height = PARAM_VAL(g_vparams, screen_height);
    param.draw_barborder = PARAM_VAL(g_vparams, draw_barborder);
  } else
    return false;
  if (PARAM_SET(g_vparams, draw_keyboard)) {
    if ((param.draw_keyboard = PARAM_VAL(g_vparams, draw_keyboard))) {
      if (PARAM_SET(g_vparams, keyboard_h) && PARAM_SET(g_vparams, blackkey_h)) {
        param.keyboard_h = PARAM_VAL(g_vparams, keyboard_h);
        param.blackkey_h = PARAM_VAL(g_vparams, blackkey_h);
      } else
        return false;
    }
  } else
    return false;
  if (PARAM_SET(g_vparams, tckk_poolsize) &&
      PARAM_SET(g_vparams, bars_poolsize) &&
      PARAM_SET(g_vparams, queu_poolsize)) {
    param.tckk_poolsize = PARAM_VAL(g_vparams, tckk_poolsize);
    param.bars_poolsize = PARAM_VAL(g_vparams, bars_poolsize);
    param.queu_poolsize = PARAM_VAL(g_vparams, queu_poolsize);
  } else
    return false;

  if (PARAM_SET(g_vparams, enable_text)) {
    if ((param.enable_text = PARAM_VAL(g_vparams, enable_text))) {
      if (PARAM_SET(g_vparams, enable_metalist) && PARAM_SET(g_vparams, filename_hint)) {
        param.filename_hint = PARAM_VAL(g_vparams, filename_hint);
        param.enable_metalist = PARAM_VAL(g_vparams, enable_metalist);
      } else
        return false;
    } else {
      param.filename_hint = false;
      param.enable_metalist = false;
    }
  } else
    return false;
  return true;
}
static bool init_vconsts(void) {
  if ((g_vconsts.bar_y = malloc(sizeof(ui16) * (param.screen_height + 1))) == NULL)
    return perror("[video] E: alloc bar_y"), false;
  g_vconsts.keyboard_y = param.frame_h - (param.draw_keyboard ? param.keyboard_h : 0);
  g_vconsts.half_key = param.frame_w / 128 / 2;
  for (ui16 x = 0; x <= 128; x++)
    g_vconsts.key_x[x] = x * param.frame_w / 128;
  g_vconsts.fall_h = param.frame_h;
  if (param.draw_keyboard) {
    g_vconsts.fall_h -= param.keyboard_h;
  }
  for (tk_t u = 0; u <= param.screen_height; u++)
    g_vconsts.bar_y[u] = g_vconsts.fall_h - u * g_vconsts.fall_h / param.screen_height;
  return true;
}
bool video_init(void) {
  if (!check_params())
    return fprintf(stderr, "[video] E: insufficient params\n"), false;
  if (!init_vconsts())
    return false;
  if (tckk_init(midi.ntrk, param.tckk_poolsize) == 0) {
    if (bars_init(param.bars_poolsize) == 0) {
      if (queu_init(param.queu_poolsize) == 0) {
        if (draw_init(param.frame_w, param.frame_h) == 0) {
          framebuffer = (byte *)frame.mem;
          return true;
        } else
          perror("[video] E: draw_init");
        queu_free();
      } else
        perror("[video] E: queu_init");
      bars_free();
    } else
      perror("[video] E: bars_init");
    tckk_free();
  } else
    perror("[video] E: tckk_init");
  return false;
}
void video_free(void) {
  free(g_vconsts.bar_y); // free vconsts
  draw_free();
  queu_free();
  bars_free();
  tckk_free();
}

#define METALIST_LESS 8
#define METALIST_MID 16
#define METALIST_MAX 32
#define METALIST_EXTREME 512
static const COLOR trkcolors[] = {                                       //
    RGB(0xFF, 0x00, 0x00), RGB(0x00, 0xFF, 0x00), RGB(0x00, 0x00, 0xFF), //
    RGB(0xFF, 0xFF, 0x00), RGB(0x00, 0xFF, 0xFF), RGB(0xFF, 0x00, 0xFF), //
    RGB(0x7F, 0x00, 0x00), RGB(0x00, 0x7F, 0x00), RGB(0x00, 0x00, 0x7F), //
    RGB(0x7F, 0x7F, 0x00), RGB(0x00, 0x7F, 0x7F), RGB(0x7F, 0x00, 0x7F), //
    RGB(0x7F, 0xFF, 0x00), RGB(0x00, 0x7F, 0xFF), RGB(0x7F, 0x00, 0xFF), //
    RGB(0xFF, 0x7F, 0x00), RGB(0x00, 0xFF, 0x7F), RGB(0xFF, 0x00, 0x7F), };
static const ui32 trkclrcnt = sizeof(trkcolors) / sizeof(ui32);
static const COLOR color_black = RGB(0, 0, 0), color_white = RGB(255, 255, 255);
static const COLOR color_bg = RGB(240, 240, 240), color_border = RGB(0, 0, 0);
#define MY_TEXT_COLOR RGB(255, 255, 255), RGBA(0, 0, 0, 80)

static const bool g_iswhitekey[] = {1, 0, 1, 0, 1, /**/ 1, 0, 1, 0, 1, 0, 1};
static const bool g_white_left[] = {0, 0, 1, 0, 1, /**/ 0, 0, 1, 0, 1, 0, 1};
static const bool g_white_righ[] = {1, 0, 1, 0, 0, /**/ 1, 0, 1, 0, 1, 0, 0};
#define LIST_META 0xff
#define LIST_FEND 0x2f
#define LIST_MPQN 0x51
#define LIST_NOTE_UP 8
#define LIST_NOTE_DN 9
TNumList q_meta;
TNumList q_list;
bool tick_ended;
tk_t tick_procd; // processed tick count
static void tick_begin() {
  midi.trk = midi.trks;
  for (int it = 0; it < midi.ntrk; it++, midi.trk++)
    if (midi.trk->track_left)
      midi.trk->last_delta = mtrk_dt();
  tick_procd = 0;
  tick_ended = false;
}
static tk_t tick_runto(tk_t wanted) { // should call everytime scr top time increases (every tick)
  if (tick_ended)
    return tick_procd;
  if (tick_procd < wanted) {
    bool end_old_no[128] = {false};
    no_t oldnoteids[128];
    for (int jk = 0; jk < 128; jk++)
      oldnoteids[jk] = tckk_keys[jk].tail->note_id;
    do {
      bool lef;
      ui64 mdt = MIDI_MAXDT + 1, dt,
           notedn_cnt = 0,
           noteup_cnt = 0;
      midi.trk = midi.trks;
      for (int it = 0; it < midi.ntrk; it++, midi.trk++)
        if (midi.trk->track_left) {
          if ((dt = midi.trk->last_delta) == 0) {
            while (true) {
              lef = mtrk_evt();
              // process event
              switch (midi.evt.b >> 4) {
              case 0x9:
                if (midi.evt.a2 > 0) {
                  tckk_keydn(it, midi.evt.b & 0xf, midi.evt.a1);
                  notedn_cnt++;
                  break;
                }
                goto ft;
              case 0x8: ft: {
                no_t up_id = tckk_keyup(it, midi.evt.b & 0xf, midi.evt.a1);
                if (up_id > 0) {
                  noteup_cnt++;
                  if (up_id == oldnoteids[midi.evt.a1])
                    end_old_no[midi.evt.a1] = true;
                }
                break;
              }
              case 0xf:
                if (midi.evt.b == 0xff) {
                  if (param.enable_metalist) {
                    TNum *tnm = TNL_push(&q_meta);
                    tnm->tick = tick_procd;
                    tnm->type = LIST_META;
                    tnm->numb_byte = midi.evt.msys.type;
                    tnm->numb_ui16 = (ui16)it;
                    tnm->numb_ui32 = midi.evt.msys.size;
                    tnm->numb_ui64 = (ui64)midi.evt.msys.data;
                  }
                  if (midi.evt.msys.type == 0x51 && midi.evt.msys.size >= 3) {
                    ui32 mpqn = midi.evt.msys.data[0];
                    mpqn = (mpqn << 8) | midi.evt.msys.data[1];
                    mpqn = (mpqn << 8) | midi.evt.msys.data[2];
                    if (mpqn) {
                      TNum *tnm = TNL_push(&q_list);
                      tnm->tick = tick_procd;
                      tnm->type = LIST_MPQN;
                      tnm->numb_ui32 = mpqn;
                    }
                  }
                }
              }
              if (lef) {
                if ((dt = mtrk_dt()) > 0) {
                  if ((midi.trk->last_delta = dt) < mdt)
                    mdt = dt;
                  break;
                }
              } else {
                midi.trk->track_left = false;
                break;
              }
            }
          } else if (dt < mdt)
            mdt = dt;
        }

      for (int jk = 0; jk < 128; jk++) {
        TCKNote *tail = tckk_keys[jk].tail;
        no_t old_noteid = oldnoteids[jk];
        no_t new_noteid = tail->note_id;
        if (new_noteid != old_noteid) {
          Bar *bar = bars_add_bar(jk, tick_procd, end_old_no[jk], new_noteid > old_noteid);
          bar->n_id = new_noteid;
          bar->trak = tail->trak;
          bar->chan = tail->chan;
          oldnoteids[jk] = new_noteid;
        }
        end_old_no[jk] = false;
      }

      TNum *tnm;
      tnm = TNL_push(&q_list);
      tnm->tick = tick_procd;
      tnm->type = LIST_NOTE_DN;
      tnm->numb_ui32 = notedn_cnt;

      tnm = TNL_push(&q_list);
      tnm->tick = tick_procd;
      tnm->type = LIST_NOTE_UP;
      tnm->numb_ui32 = noteup_cnt;

      if (mdt > MIDI_MAXDT) {
        tnm = TNL_push(&q_list);
        tnm->tick = tick_procd;
        tnm->type = LIST_FEND;
        tick_ended = true;
      } else {
        tick_procd += mdt;
        for (int it = 0; it < midi.ntrk; it++)
          midi.trks[it].last_delta -= mdt;
      }
    } while (tick_procd < wanted && !tick_ended);
  }
  return tick_procd;
}

static inline COLOR calc_border_color(COLOR bdrcolor) {
  for (int i = 0; i < 3; i++)
    bdrcolor.bs[i] *= 0.75;
  return bdrcolor;
}

static inline void draw_keyboard_borders() {
  draw_yline(g_vconsts.fall_h, 0, param.frame_w, color_border);
  ui16 bar_u = g_vconsts.fall_h + 1, bar_l, bar_r,
       bar_y = g_vconsts.fall_h + param.blackkey_h,
       bar_z = bar_y - 1;
  for (int jk = 0; jk < 128; jk++) {
    ui16 chord_k = jk % 12;
    bar_r = g_vconsts.key_x[jk + 1] - 1;
    if (g_iswhitekey[chord_k]) { // white key border
      if (chord_k == 4 || chord_k == 11)
        draw_xline(bar_r, bar_u, param.frame_h, color_black);
    } else { // black key border
      bar_l = g_vconsts.key_x[jk];
      draw_xline(bar_l, bar_u, bar_y, color_black);
      draw_xline(bar_r, bar_u, bar_y, color_black);
      draw_yline(bar_z, bar_l, bar_r, color_black);
    }
  }
}

static void real_main() {
  TNL_ini(&q_list);
  TNL_ini(&q_meta);
  // setup bars
  bars.screen_bot = 0;
  bars.screen_top = bars.screen_bot + param.screen_height;
  for (int jk = 0; jk < 128; jk++) {
    Bar *bar = bars_add_bar(jk, 0, false, false);
    bar->n_id = 0;
  }
  // begin tick
  tick_begin();
  bool midi_endd = false;
  ui32 mpqn = 500000;
  tm_t curr_time = 0,
       next_midi = 0, next_frame = 0;
  tm_t midi_interval = (tm_t)mpqn * param.fps_up,
       frame_interval = (tm_t)1000000 * midi.divs * param.fps_dn;

  ui64 frame_cnt = 0, notedn_cnt = 0, noteup_cnt = 0, polyphony_peak = 0;

  char message[256];
  ui32 d1_second_frame_count = param.fps_up / param.fps_dn,
       d2_second_frame_count = param.fps_up / param.fps_dn / 2,
       d4_second_frame_count = param.fps_up / param.fps_dn / 4,
       d8_second_frame_count = param.fps_up / param.fps_dn / 8;
  //
  if (param.draw_keyboard)
    draw_keyboard_borders();
  while (true) {
    bool run_midi = (curr_time == next_midi);
    if (run_midi) {
      tick_runto(bars.screen_top);
      // if (bars.screen_bot >
      //   midi_endd = true;    -> change to FEND
      TNum *tnm;
      tnm = q_list.head.next;
      while (tnm && tnm->tick <= bars.screen_bot) {
        switch (tnm->type) {
        case LIST_NOTE_UP:
          noteup_cnt += tnm->numb_ui32;
          break;
        case LIST_NOTE_DN:
          notedn_cnt += tnm->numb_ui32;
          {
            ui64 polyphony = notedn_cnt - noteup_cnt;
            if (polyphony > polyphony_peak)
              polyphony_peak = polyphony;
          }
          break;
        case LIST_FEND:
          midi_endd = true;
          break;
        case LIST_MPQN:
          mpqn = tnm->numb_ui32, midi_interval = (tm_t)mpqn * param.fps_up;
          break;
        }
        tnm = TNL_pop(&q_list);
      }
      next_midi += midi_interval;
    }
    if (curr_time == next_frame) {
      frame_cnt++;
      next_frame += frame_interval;
      KBarList *kbls = bars.keys;

      for (int jk = 0; jk < 128; jk++, kbls++) {
        Bar *bar, *nex = bars_del_bef(kbls); // iterate
        COLOR bar_color, bdr_color;
        tk_t bar_beg = bars.screen_bot, bar_end, bar_up, bar_dn; // bar ticks
        ui16 bar_u, bar_d,                                       //
            bar_l = g_vconsts.key_x[jk],                         // bar l, r, u, d
            bar_r = g_vconsts.key_x[jk + 1];                     //
        bool bdr_bot, bdr_top, bdr_lr, bar_long_enough;
        while ((bar = nex)) {
          nex = bar->next, bar_end = bar->bend;

          if ((bar_dn = bar_beg - bars.screen_bot) < 0) // calc bar_dn tick
            bar_dn = 0;                                 //
          if (bar_end > bars.screen_top) {              //
            bar_up = param.screen_height;               //
          } else                                        //
            bar_up = bar_end - bars.screen_bot;         // calc bar_up tick

          bar_d = g_vconsts.bar_y[bar_dn]; // bar down y
          bar_u = g_vconsts.bar_y[bar_up]; // bar   up y

          bdr_top = false, bdr_bot = false, bdr_lr = param.draw_barborder;
          bar_long_enough = true;

          if (bar_d - bar_u < 3)
            bdr_lr = false, bar_long_enough = false;

          if (bar->n_id == 0)                                           // bar color
            bar_color = color_bg;                                       // no key -> bg color
          else {                                                        //
            bar_color = trkcolors[(bar->trak + bar->chan) % trkclrcnt]; //
            bdr_color = calc_border_color(bar_color);
            if (!bar_long_enough)
              bar_color = bdr_color;
            if (bdr_lr) {
              bar_r--;
              draw_xline(bar_l, bar_u, bar_d, bdr_color);
              draw_xline(bar_r, bar_u, bar_d, bdr_color);
              bar_l++;
            }
          }
          if (bar->n_id > 0 && bar_long_enough) {
            bdr_bot = bar->nbeg == bar_beg;   // whether draw bottom border
            if (bar_end == TICK_INF)          //
              bdr_top = false;                //
            else                              //
              bdr_top = bar_end == bar->nend; // whether draw top border

            if (param.draw_barborder) {
              if (bdr_bot) {
                bar_d--;
                draw_yline(bar_d, bar_l, bar_r, bdr_color);
              }
              if (bdr_top) {
                draw_yline(bar_u, bar_l, bar_r, bdr_color);
                bar_u++;
              }
            }
          }
          draw_block(bar_l, bar_u, bar_r, bar_d, bar_color);
          if (bdr_lr && bar->n_id > 0) {
            bar_r++;
            bar_l--;
          }
          bar_beg = bar_end;
          if (bar_beg >= bars.screen_top) // out of screen
            break;
        }
        // draw keyboard
        if (param.draw_keyboard) {
          ui16 chord_k = jk % 12;
          bool iswhite = g_iswhitekey[chord_k];
          bar_u = g_vconsts.fall_h + 1;
          bar_d = g_vconsts.fall_h + param.blackkey_h;
          if (iswhite) {
            bar_color = color_white;
            if (chord_k == 4 || chord_k == 11)
              bar_r--;
          } else {
            bar_l++;
            bar_r--;
            bar_d--;
            bar_color = color_black;
          }
          if ((bar = kbls->head.next)->n_id > 0) // no NULL check, bec'of design
            bar_color = trkcolors[(bar->trak + bar->chan) % trkclrcnt];
          draw_block(bar_l, bar_u, bar_r, bar_d, bar_color);
          if (iswhite) {
            bar_u = bar_d;
            bar_d = param.frame_h;
            if (g_white_left[chord_k]) {
              if (bar_l < g_vconsts.half_key)
                bar_l = 0;
              else
                bar_l -= g_vconsts.half_key;
            }
            if (g_white_righ[chord_k])
              if ((bar_r += g_vconsts.half_key) > param.frame_w)
                bar_r = param.frame_w;
            draw_block(bar_l, bar_u, bar_r, bar_d, bar_color);
            if (bar_l > 0)
              draw_xline(bar_l - 1, bar_u, bar_d, color_border);
          }
        }
      }
      // if (param.draw_keyboard)
      //   draw_keyboard_borders();
      // draw text
      if (param.enable_text) {
        ui32 secs = frame_cnt * param.fps_dn / param.fps_up, mins;
        ui32 frms = frame_cnt - secs * param.fps_up / param.fps_dn;
        mins = secs / 60, secs = secs % 60;

        if (mins == 0 && secs < 3 && param.filename_hint) {
          snprintf(message, sizeof(message),
                   "文件: %s\n"
                   "类型: %hu 轨道数: %hu 分辨率: %hu",
                   filename_mid, midi.type, midi.ntrk, midi.divs);
          draw_utf8(message, 64, 64, MY_TEXT_COLOR);
        }
        if (param.enable_metalist) {
          static ui64 last_event_increase = 0, last_event_count = 0;
          bool arrow_not_drawn = true;
          TNum *tnm, *nex = q_meta.head.next;
          if (nex) {
            if (q_meta.cnt > last_event_count)
              last_event_increase = frame_cnt;
            si16 dx = param.frame_w - 480, dy = 16;
            snprintf(message, sizeof(message), "Tick ⬇ Meta事件 (%u)", q_meta.cnt);
            draw_utf8(message, dx, dy, MY_TEXT_COLOR);
            while ((tnm = nex) && (dy += TEXT_LINE_HEIGHT) < (g_vconsts.keyboard_y - TEXT_LINE_HEIGHT)) {
              nex = tnm->next;
              byte *data = (byte *)tnm->numb_ui64;
              switch (tnm->numb_byte) {
              case 0x01:
              case 0x02:
              case 0x03:
              case 0x04:
              case 0x05:
              case 0x06:
              case 0x07: {
                size_t i = snprintf(message, sizeof(message), "%6ld:%02hd %hhd:", tnm->tick, tnm->numb_ui16, tnm->numb_byte);
                for (size_t j = 0; i < sizeof(message) - 2 && j < tnm->numb_ui32; i++, j++)
                  message[i] = data[j];
                message[i] = '\0';
                break;
              }
              case 0x2f:
                snprintf(message, sizeof(message), "%6ld:轨道#%hd结束",
                         tnm->tick, tnm->numb_ui16);
                break;
              case 0x51: {
                ui32 x_mpqn = data[0];
                x_mpqn = (x_mpqn << 8) | data[1];
                x_mpqn = (x_mpqn << 8) | data[2];
                snprintf(message, sizeof(message), "%6ld:变速 BPM=%.3lf",
                         tnm->tick, 60000000.0 / x_mpqn);
                break;
              }
              default:
                snprintf(message, sizeof(message), "%6ld:#%02hd %02hhx L=%d",
                         tnm->tick, tnm->numb_ui16, tnm->numb_byte, tnm->numb_ui32);
              }
              draw_utf8(message, dx, dy, MY_TEXT_COLOR);
              if (arrow_not_drawn && tnm->tick >= bars.screen_bot) {
                draw_utf8("⟶", dx - 16, dy, MY_TEXT_COLOR);
                arrow_not_drawn = false;
              }
            }
            if (frame_cnt > d2_second_frame_count) {
              ui64 frame_since = frame_cnt - last_event_increase;
              ui32 secs_since = frame_since * param.fps_dn / param.fps_up;
              ui32 frms_since = frame_since - secs_since * param.fps_up / param.fps_dn;
              if (q_meta.cnt > METALIST_MID) {
                if (q_meta.cnt > METALIST_MAX) {
                  if (q_meta.cnt > METALIST_EXTREME) { // extreme ->
                    nex = q_meta.head.next;
                    while ((tnm = nex) && (q_meta.cnt > METALIST_MAX) && (tnm->tick < bars.screen_bot))
                      nex = TNL_pop(&q_meta);
                  } else if (q_meta.head.next->tick < bars.screen_bot) // max -> extreme
                    TNL_pop(&q_meta);
                } else if (frame_since >= d8_second_frame_count && frame_since % d8_second_frame_count == 0) // mid -> max
                  TNL_pop(&q_meta);
              } else if (q_meta.cnt > METALIST_LESS) { // less -> mid
                if (frame_since >= d4_second_frame_count && frame_since % d4_second_frame_count == 0)
                  TNL_pop(&q_meta);
              } else if (frame_since >= d1_second_frame_count && frms_since == 0) // -> less
                TNL_pop(&q_meta);
              // snprintf(message, sizeof(message), "frame_since = %lu\n", frame_since);
              // draw_utf8(message, 128, 128, MY_TEXT_COLOR);
            }
          }
          last_event_count = q_meta.cnt;
        }
        snprintf(message, sizeof(message), "复音 %9lu\nBPM %10.3lf",
                 notedn_cnt - noteup_cnt,
                 60000000.0 / mpqn);
        draw_utf8(message, 0, 0, MY_TEXT_COLOR);
        snprintf(message, sizeof(message), "音符: %9lu\n时间: %02d:%02d.%03df\nTick: %9ld (%ld)",
                 notedn_cnt,
                 mins, secs, frms,
                 bars.screen_bot, tick_procd);
        draw_utf8(message, 0, g_vconsts.keyboard_y - 3 * TEXT_LINE_HEIGHT, MY_TEXT_COLOR);
      }
      {
        size_t offset = 0;
        ssize_t wrote;
        while (offset < frame.len) {
          wrote = write(g_vconsts.fd_write, framebuffer + offset, frame.len - offset);
          if (wrote == -1 && errno != EPIPE) {
            perror("[video] E: write");
            break;
          }
          offset += wrote;
        }
        if (wrote == -1)
          break;
      }
      if (midi_endd)
        break;
    }
    if (run_midi) {
      bars.screen_bot++;
      bars.screen_top = bars.screen_bot + param.screen_height;
    }
    curr_time = next_midi < next_frame ? next_midi : next_frame;
  }
  fprintf(stderr, "[video] I: frames = %lu, ticks = %ld, notes = %lu, maxpoly = %lu\n",
          frame_cnt, tick_procd, notedn_cnt, polyphony_peak);
  TNL_clr(&q_meta);
  TNL_clr(&q_list);
}
void *video_thr(void *arg) {
  video_running = true;
  signal(SIGPIPE, SIG_IGN);
  real_main();
  fprintf(stderr, "[video] I: end !\n");
  video_running = false;
  if (g_params.write_empty && audio_running) {
    COLOR c1 = RGB(0x00, 0x00, 0x00);
    COLOR c2 = RGB(0xff, 0xff, 0xff);
    for (ui16 i = 0, y = 0; y < 720; y += 80, i++)
      for (ui16 j = 0, x = 0; x < 1280; x += 80, j++)
        draw_block(x, y, x + 80, y + 80, ((i + j) % 2) ? c1 : c2);
    size_t offset = 0;
    ssize_t wrote;
    while (audio_running) {
      // fprintf(stderr, "[video] I: write chessboard !\n");
      while (audio_running && offset < frame.len) {
        wrote = write(g_vconsts.fd_write, framebuffer + offset, frame.len - offset);
        if (wrote == -1)
          break;
        offset += wrote;
      }
      if (wrote == -1)
        break;
      offset = 0;
    }
  }
  if (g_params.early_close)
    close(g_vconsts.fd_write);
  fprintf(stderr, "[video] I: exit !\n");
  return NULL;
}
