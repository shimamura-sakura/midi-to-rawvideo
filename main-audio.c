#include "main.h"
volatile bool audio_running;
struct g_aparams_t g_aparams;
struct g_aconsts_t g_aconsts;

static BYTE *buffer;
static QWORD buflen;
static HSTREAM stream;

void audio_free(void) {
  free(buffer);
  BASS_Free();
}
bool audio_init(void) {
  if (!(PARAM_SET(g_aparams, freq) &&
        PARAM_SET(g_aparams, voices) &&
        PARAM_SET(g_aparams, buftime) &&
        PARAM_SET(g_aparams, cpulimit) &&
        PARAM_SET(g_aparams, filenames_sf2)))
    return fprintf(stderr, "[audio] E: insufficient params\n"), false;
  BASS_SetConfig(BASS_CONFIG_MIDI_VOICES, PARAM_VAL(g_aparams, voices));
  BASS_SetConfig(BASS_CONFIG_MIDI_IN_PORTS, 0);
  BASS_SetConfig(BASS_CONFIG_MIDI_AUTOFONT, 0);
  BASS_SetConfig(BASS_CONFIG_MIDI_COMPACT, FALSE);
  BASS_SetConfigPtr(BASS_CONFIG_MIDI_DEFFONT, NULL);
  if (!BASS_Init(0, PARAM_VAL(g_aparams, freq), BASS_DEVICE_STEREO, NULL, NULL)) {
    fprintf(stderr, "[audio] E: BASS_Init: %d\n", BASS_ErrorGetCode());
    return false;
  }

  size_t nSoundFonts = 0;
  const char **filenames_sf2 = PARAM_VAL(g_aparams, filenames_sf2);
  for (; filenames_sf2[nSoundFonts] != NULL; nSoundFonts++)
    ;
  BASS_MIDI_FONT bmfs[nSoundFonts];
  for (size_t i = 0; i < nSoundFonts; i++) {
    fprintf(stderr, "[audio] I: SF2 '%s'\n", filenames_sf2[i]);
    HSOUNDFONT sf = BASS_MIDI_FontInit(filenames_sf2[i], BASS_MIDI_FONT_NOFX);
    bmfs[i] = (BASS_MIDI_FONT){sf, -1, 0};
    if (sf == 0) {
      fprintf(stderr, "[audio] E: FontInit: %d\n", BASS_ErrorGetCode());
      BASS_Free();
      return false;
    }
  }
  stream = BASS_MIDI_StreamCreateFile(
      TRUE, file.mem, 0, file.len,
      BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT | BASS_MIDI_DECAYEND | BASS_MIDI_NOFX, 0);
  if (stream == 0) {
    fprintf(stderr, "[audio] E: StreamCreate %d\n", BASS_ErrorGetCode());
    BASS_Free();
    return false;
  }
  BASS_ChannelSetAttribute(stream, BASS_ATTRIB_MIDI_CPU, PARAM_VAL(g_aparams, cpulimit));
  {
    HFX fx = BASS_ChannelSetFX(stream, BASS_FX_BFX_COMPRESSOR2, 0);
    BASS_BFX_COMPRESSOR2 fx_param;
    BASS_FXGetParameters(fx, &fx_param);
    fx_param.fGain = 0.0;
    fx_param.fThreshold = -20.0;
    fx_param.fRatio = 6.0;
    fx_param.fAttack = 10.0;
    fx_param.fRelease = 500.0;
    BASS_FXSetParameters(fx, &fx_param);
  }
  BASS_MIDI_StreamSetFonts(stream, &bmfs, nSoundFonts);
  BASS_MIDI_StreamLoadSamples(stream);
  buflen = BASS_ChannelSeconds2Bytes(stream, PARAM_VAL(g_aparams, buftime));
  buffer = malloc(buflen);
  if (buffer == NULL) {
    perror("[audio] E: alloc buffer");
    BASS_Free();
    return false;
  }
  g_aconsts.duration = BASS_ChannelBytes2Seconds(stream, BASS_ChannelGetLength(stream, BASS_POS_BYTE));
  return true;
}
void *audio_thr(void *arg) {
  audio_running = true;
  signal(SIGPIPE, SIG_IGN);
  DWORD got;
  while ((got = BASS_ChannelGetData(stream, buffer, buflen | BASS_DATA_FLOAT)) != (DWORD)-1) {
    size_t offset = 0;
    ssize_t wrote;
    while (offset < got) {
      wrote = write(g_aconsts.fd_write, buffer + offset, got - offset);
      if (wrote == -1 && errno != EPIPE) {
        perror("[audio] E: write");
        break;
      }
      offset += wrote;
    }
    if (wrote == -1)
      break;
  }
  if (got == (DWORD)-1)
    if (BASS_ErrorGetCode() != BASS_ERROR_ENDED)
      fprintf(stderr, "[audio] E: ChanGetData: %d\n", BASS_ErrorGetCode());
  fprintf(stderr, "[audio] I: end !\n");
  audio_running = false;
  if (g_params.write_empty && video_running) {
    bzero(buffer, buflen);
    while (video_running) {
      if (write(g_aconsts.fd_write, buffer, buflen) == -1)
        break;
      // fprintf(stderr, "[audio] I: write silence !\n");
    }
  }
  if (g_params.early_close)
    close(g_aconsts.fd_write);
  fprintf(stderr, "[audio] I: exit !\n");
  return NULL;
}
