#pragma once
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "util.h"
#include "file-load.h"
#include "midi.h"
#include "tckk.h"
#include "bars.h"
#include "queu.h"
#include "draw.h"

#include "bass.h"
#include "bassenc.h"
#include "bass_fx.h"
#include "bassmidi.h"

// param
#define PARAM_DEF(TYPE, NAME) \
  struct {                    \
    bool set;                 \
    TYPE value;               \
  } NAME
#define PARAM_SET(ST, NAME) ((ST).NAME).set
#define PARAM_VAL(ST, NAME) ((ST).NAME).value
#define PARAM_ASSIGN(ST, NAME, VALUE) \
  {                                   \
    PARAM_VAL(ST, NAME) = (VALUE);    \
    PARAM_SET(ST, NAME) = true;       \
  }
// recv
#define RECV_ADD_ARG(FMT, ...)                                                      \
  if ((*argc) + 1 < max_argc && written < max_argl) {                               \
    argv[(*argc)++] = args + written;                                               \
    int sngot = snprintf(args + written, max_argl - written, (FMT), ##__VA_ARGS__); \
    argv[(*argc)] = NULL;                                                           \
    written += sngot + 1;                                                           \
  }
#define HDR_RECV_FUNC(NAME) \
  void recv_##NAME(int max_argc, int max_argl, int *argc, char **argv, char *args, int written);
#define BEG_RECV_FUNC(NAME) \
  void recv_##NAME(int max_argc, int max_argl, int *argc, char **argv, char *args, int written)
#define CALL_RECV_FUNC(NAME, max_argc, max_argl, pArgc, argv, args) \
  recv_##NAME((max_argc), (max_argl), (pArgc), (argv), (args), 0)

extern const char *filename_mid;
extern struct g_params_t {
  bool enable_audio, use_audio_file;
  const char *filename_audio;

  bool early_close, write_empty;
} g_params;

extern volatile bool audio_running, video_running;

bool audio_init(void);
void audio_free(void);
void *audio_thr(void *arg);
extern struct g_aparams_t {
  PARAM_DEF(DWORD, freq);
  PARAM_DEF(DWORD, voices);
  PARAM_DEF(float, cpulimit);
  PARAM_DEF(double, buftime);
  PARAM_DEF(const char **, filenames_sf2);
} g_aparams;
extern struct g_aconsts_t {
  int fd_read, fd_write;
  double duration;
} g_aconsts;

bool video_init(void);
void video_free(void);
void *video_thr(void *arg);
extern struct g_vparams_t {
  PARAM_DEF(ui16, frame_w);
  PARAM_DEF(ui16, frame_h);
  PARAM_DEF(ui16, fps_up);
  PARAM_DEF(ui16, fps_dn);
  PARAM_DEF(tk_t, screen_height);
  PARAM_DEF(bool, draw_keyboard);
  PARAM_DEF(ui16, keyboard_h);
  PARAM_DEF(ui16, blackkey_h);
  PARAM_DEF(bool, draw_barborder);
  PARAM_DEF(bool, enable_text);
  PARAM_DEF(bool, enable_metalist);
  PARAM_DEF(bool, filename_hint);

  PARAM_DEF(size_t, tckk_poolsize);
  PARAM_DEF(size_t, bars_poolsize);
  PARAM_DEF(size_t, queu_poolsize);
} g_vparams;
extern struct g_vconsts_t {
  int fd_read, fd_write;
  ui16 key_x[129];
  ui16 *bar_y;
  ui16 fall_h;
  ui16 half_key;
  ui16 keyboard_y;
} g_vconsts;

HDR_RECV_FUNC(mpv)
HDR_RECV_FUNC(ffmpeg)