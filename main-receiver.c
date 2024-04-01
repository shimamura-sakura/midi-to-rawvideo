#include "main.h"
#define ADD_ARG(FMT, ...) RECV_ADD_ARG(FMT, ##__VA_ARGS__)

BEG_RECV_FUNC(mpv) {

  g_params.write_empty = false;
  g_params.early_close = true;

  ADD_ARG("mpv");
  if (g_params.use_audio_file)
    ADD_ARG("--audio-file=%s", g_params.filename_audio);
  if (g_params.enable_audio) {
    ADD_ARG("--audio-demuxer=rawaudio");
    ADD_ARG("--demuxer-rawaudio-rate=44100");
    ADD_ARG("--demuxer-rawaudio-format=floatle");
    ADD_ARG("--demuxer-rawaudio-channels=stereo");
  }
#define DMB_BLKSIZE (1048576 * 16)
  ui64 demuxer_max_bytes = frame.len * 1 * PARAM_VAL(g_vparams, fps_up) / PARAM_VAL(g_vparams, fps_dn);
  demuxer_max_bytes = ((demuxer_max_bytes + DMB_BLKSIZE - 1) / DMB_BLKSIZE) * DMB_BLKSIZE;
  ADD_ARG("--demuxer-max-bytes=%ld", demuxer_max_bytes);
  ADD_ARG("--demuxer=rawvideo");
  ADD_ARG("--demuxer-rawvideo-format=BGRA");
  ADD_ARG("--demuxer-rawvideo-w=%hu", PARAM_VAL(g_vparams, frame_w));
  ADD_ARG("--demuxer-rawvideo-h=%hu", PARAM_VAL(g_vparams, frame_h));
  ADD_ARG("--demuxer-rawvideo-fps=%.3lf", (double)PARAM_VAL(g_vparams, fps_up) / PARAM_VAL(g_vparams, fps_dn));
  ADD_ARG("--demuxer-rawvideo-size=%u", (ui32)PARAM_VAL(g_vparams, frame_w) * PARAM_VAL(g_vparams, frame_h) * 4);
  if (g_params.enable_audio)
    ADD_ARG("--audio-file=fdclose://%d", g_aconsts.fd_read);
  ADD_ARG("--force-media-title=midiRawVideo %s", filename_mid);
  ADD_ARG("fdclose://%d", g_vconsts.fd_read);
}

BEG_RECV_FUNC(ffmpeg) {

  g_params.write_empty = true;
  g_params.early_close = true;

  ADD_ARG("ffmpeg");
  ADD_ARG("-f");
  ADD_ARG("rawvideo");
  ADD_ARG("-pixel_format");
  ADD_ARG("bgra");
  ADD_ARG("-video_size");
  ADD_ARG("%hux%hu", PARAM_VAL(g_vparams, frame_w), PARAM_VAL(g_vparams, frame_h));
  ADD_ARG("-framerate");
  ADD_ARG("%.3lf", (double)PARAM_VAL(g_vparams, fps_up) / PARAM_VAL(g_vparams, fps_dn));
  ADD_ARG("-i");
  ADD_ARG("pipe:%d", g_vconsts.fd_read);
  if (g_params.enable_audio) {
    ADD_ARG("-f");
    ADD_ARG("f32le");
    ADD_ARG("-ar");
    ADD_ARG("%d", PARAM_VAL(g_aparams, freq));
    ADD_ARG("-ac");
    ADD_ARG("2");
    ADD_ARG("-i");
    ADD_ARG("pipe:%d", g_aconsts.fd_read);
  }
  if (g_params.use_audio_file) {
    ADD_ARG("-i");
    ADD_ARG("%s", g_params.filename_audio);
  }
  if (g_params.enable_audio || g_params.use_audio_file) {
    ADD_ARG("-c:a");
    ADD_ARG("aac");
    ADD_ARG("-b:a");
    ADD_ARG("192k");
  }

  // ADD_ARG("-shortest");

  ADD_ARG("-c:v");
  ADD_ARG("h264_nvenc");
  ADD_ARG("-profile:v");
  ADD_ARG("high");
  ADD_ARG("-preset:v");
  ADD_ARG("fast");
  ADD_ARG("-qp");
  ADD_ARG("0");

  ADD_ARG("-y");
  ADD_ARG("/tmp/output.mkv");
}