#include "main.h"
#include <fcntl.h>

struct g_params_t g_params;
const char *filename_mid;
const char *filenames_sf2[] = {
    "/usr/share/soundfonts/FluidR3_GM.sf2",
    "/home/jerry/Music/midi/sf2/jv1080.sf2",
    NULL};

bool init_fm(void) {
  if (file_load(filename_mid)) {
    int midi_ret = midi_init(file.mem, file.len);
    if (midi_ret == MIDI_OK) {
      fprintf(stderr, "[main] I: %hu tracks, %hu division\n", midi.ntrk, midi.divs);
      return true;
    } else if (midi_ret == MIDI_E_SEE_ERRNO)
      perror("[main] E: midi_init");
    else
      fprintf(stderr, "[main] E: midi_init code = %d\n", midi_ret);
    file_free();
  } else
    perror("[main] E: file_load");
  return false;
}
void free_fm(void) {
  midi_free();
  file_free();
}

bool setup_recvpipes(void);
pid_t start_receiver(void);

void assign_params(void) {
  g_params.enable_audio = true;
  g_params.use_audio_file = false;
  g_params.filename_audio = "";

  PARAM_ASSIGN(g_aparams, freq, 44100);
  PARAM_ASSIGN(g_aparams, voices, 512);
  PARAM_ASSIGN(g_aparams, buftime, 0.5);
  PARAM_ASSIGN(g_aparams, cpulimit, 95.0);

  PARAM_ASSIGN(g_aparams, filenames_sf2, filenames_sf2);

  PARAM_ASSIGN(g_vparams, frame_w, 1280);
  PARAM_ASSIGN(g_vparams, frame_h, 720);
  PARAM_ASSIGN(g_vparams, fps_up, 120);
  PARAM_ASSIGN(g_vparams, fps_dn, 1);
  PARAM_ASSIGN(g_vparams, draw_barborder, true);
  PARAM_ASSIGN(g_vparams, draw_keyboard, true);
  PARAM_ASSIGN(g_vparams, keyboard_h, PARAM_VAL(g_vparams, frame_h) / 10);
  PARAM_ASSIGN(g_vparams, blackkey_h, PARAM_VAL(g_vparams, frame_h) / 15);
  PARAM_ASSIGN(g_vparams, screen_height,
               PARAM_VAL(g_vparams, frame_h) -
                   (PARAM_VAL(g_vparams, draw_keyboard) ? PARAM_VAL(g_vparams, keyboard_h) : 0));

  // PARAM_VAL(g_vparams, screen_height) >>= 1;

  PARAM_ASSIGN(g_vparams, enable_text, true);
  PARAM_ASSIGN(g_vparams, filename_hint, false);
  PARAM_ASSIGN(g_vparams, enable_metalist, false);
  //
  PARAM_ASSIGN(g_vparams, queu_poolsize, 1024);
  PARAM_ASSIGN(g_vparams, tckk_poolsize, 1048576);
  PARAM_ASSIGN(g_vparams, bars_poolsize, 128 * PARAM_VAL(g_vparams, screen_height));
}

int main(int argc, char **argv) {
  if (argc < 2)
    return -1;
  else
    filename_mid = argv[1];
  assign_params();
  g_params.write_empty = false;
  g_params.early_close = true;
  audio_running = false, video_running = false;
  if (!init_fm())
    return -1;
  if (g_params.enable_audio) {
    if (g_params.use_audio_file)
      g_params.enable_audio = false;
    else if (!audio_init())
      return free_fm(), -1;
  } else
    g_params.use_audio_file = false;

  if (!video_init()) {
    if (g_params.enable_audio)
      audio_free();
    return free_fm(), -1;
  }
  int ret = -1;
  pid_t recvpid;
  if (setup_recvpipes()) {
    if ((recvpid = start_receiver()) != -1) {
      if (g_params.enable_audio) {
        pthread_t pth_audio, pthret = 0;
        if ((pthret = pthread_create(&pth_audio, NULL, audio_thr, NULL)) != 0) {
          errno = pthret;
          perror("[main] E: create audio thread");
        }
        video_thr(NULL);
        pthread_join(pth_audio, NULL);
        if (!g_params.early_close)
          close(g_aconsts.fd_write);
        ret = 0;
      } else {
        video_thr(NULL);
        ret = 0;
      }
      if (!g_params.early_close)
        close(g_vconsts.fd_write);
      // fprintf(stderr, "[main] I: end !\n");
      // if (g_params.enable_audio)
      //   close(g_aconsts.fd_write);
      // if (g_params.enable_video)
      //   close(g_vconsts.fd_write);
      fprintf(stderr, "[main] I: waiting receiver\n");
      waitpid(recvpid, NULL, 0);
    }
  } else
    perror("[main] E: create pipe");
  video_free();
  if (g_params.enable_audio)
    audio_free();
  free_fm();
  return ret;
}

bool setup_recvpipes(void) {
  int vpipefd[2], apipefd[2], tmp_errno;
  if (pipe(vpipefd) == -1)
    return false;
  g_vconsts.fd_read = vpipefd[0];
  g_vconsts.fd_write = vpipefd[1];
  if (g_params.enable_audio) {
    if (pipe(apipefd) == -1) {
      tmp_errno = errno;
      close(vpipefd[0]);
      close(vpipefd[1]);
      errno = tmp_errno;
      return false;
    }
    g_aconsts.fd_read = apipefd[0];
    g_aconsts.fd_write = apipefd[1];
  }
  return true;
}
pid_t start_receiver(void) {
  int argc = 0;
  char *argv[64], args[1024];

  CALL_RECV_FUNC(mpv, sizeof(argv) / sizeof(char *), sizeof(args) / sizeof(char), &argc, argv, args);

  pid_t recv_pid = vfork();
  switch (recv_pid) {
  case -1:
    return perror("[main] E: fork receiver"), -1;
  case 0: // child
    close(g_vconsts.fd_write);
    if (g_params.enable_audio)
      close(g_aconsts.fd_write);

    fprintf(stderr, "[child] I: ");
    for (int i = 0; i < argc; i++)
      fprintf(stderr, "%s ", argv[i]);
    fprintf(stderr, "\n");

    if (execvp(argv[0], argv) == -1)
      perror("[child] E: exec receiver");
    _exit(-1);
    return -1;
  default: // parent
    close(g_vconsts.fd_read);
    if (g_params.enable_audio)
      close(g_aconsts.fd_read);
    return 0;
  }
}
