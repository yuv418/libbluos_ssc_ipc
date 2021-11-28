// TODO don't hardcode anything
#include <portaudio.h>
#include <sndfile.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

#define BUF_SIZE 4096

typedef enum { AMDREAD, AMDWRITE, ARMPROCESS, FINISHED } LIB;
typedef struct {
  int buf_pos;
  int buf_end;
  int frame_size;
  int size;
} get_samples_data;
typedef struct {
  int len;
} write_samples_data;

typedef struct {
  uint8_t input_buffer[BUF_SIZE];
  int32_t output_buffer[BUF_SIZE * 4];
  get_samples_data get_samples_var;
  write_samples_data write_samples_var;
  LIB turn;
  SF_INFO audio_info;
} ipc_mqa_struct;

// Copied from the ARM version
void get_samples(SNDFILE *in_file, ipc_mqa_struct *mqa_s) {
  // printf("reading audio\n");
  get_samples_data *gs_var = &mqa_s->get_samples_var;

  gs_var->size = 0;
  if (gs_var->buf_pos > 0) {
    gs_var->size = gs_var->buf_end - gs_var->buf_pos;
    memmove(mqa_s->input_buffer, mqa_s->input_buffer + gs_var->buf_pos,
            gs_var->size);

    gs_var->buf_pos = 0;
    gs_var->buf_end = gs_var->size;
  }

  if (gs_var->buf_end < BUF_SIZE) {
    int frames = (BUF_SIZE - gs_var->buf_end) / gs_var->frame_size;

    frames = sf_readf_int(in_file, (int *)&mqa_s->input_buffer[gs_var->buf_end],
                          frames);
    gs_var->buf_end += frames * gs_var->frame_size;
  }

  gs_var->size = gs_var->buf_end - gs_var->buf_pos;
  gs_var->size -= gs_var->size % gs_var->frame_size;
  // printf("gsvar size %d\n", gs_var->size);
}

void play_audio(ipc_mqa_struct *mqa_s, SNDFILE *outfile) {
  // UNIMPLEMENTED
  printf("playing audio\n");
  printf("len * 2 is %d\n", mqa_s->write_samples_var.len * 2);
  int ret = sf_writef_int(outfile, mqa_s->output_buffer,
                          mqa_s->write_samples_var.len * 2);
  // printf("audio ret %d\n", ret);
}

int main(int argc, char **argv) {
  int shmid;
  ipc_mqa_struct *shared_mqa_struct;

  SNDFILE *in_file;
  SNDFILE *out_file;
  SF_INFO in_file_info;
  SF_INFO out_file_info;

  PaStream *stream;
  PaError error_buffer;

  // Get the shared memory for this location
  shmid = shmget(25589, sizeof(ipc_mqa_struct), IPC_CREAT | 0777);
  shared_mqa_struct = shmat(shmid, NULL, 0);

  // Open the sound file we will be working with
  // TODO we might end up wanting a different input, and that's fine.
  memset(&in_file_info, 0, sizeof(in_file_info));
  in_file = sf_open(argv[1], SFM_READ, &in_file_info);

  // in_file is NULL if sf_open fails
  if (!in_file) {
    // printf("sf_open failed!\n");
    return 1;
  }

  // Set up outfile TODO don't hardcode
  out_file_info.samplerate = 192000; // infmt.samplerate << rs;
  out_file_info.channels = in_file_info.channels;
  out_file_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

  out_file = sf_open(argv[2], SFM_WRITE, &out_file_info);
  if (!out_file) {
    // printf("sf_open (out) failed!\n");
    return 1;
  }

  // Initialise PortAudio
  // error_buffer = Pa_Initialize();
  if (error_buffer != paNoError) {
    // printf("Pa_Initialize failed!");
    return 1;
  }
  // error_buffer = Pa_OpenDefaultStream(&stream, 0, in_file_info.channels,
  // paFloat32, 192000, 512, callback, void *userData)

  // IPC renderer code
  ipc_mqa_struct temp_mqa_struct = {
      .input_buffer = {0},
      .output_buffer = {0},
      .get_samples_var =
          {
              .buf_end = 0,
              .buf_pos = 0,
              .frame_size = 0,
              // This allows the while loop to start
              // later. It won't affect anything since
              // size gets overwritten immediately anyway.
              .size = 1,
          },
      .write_samples_var = {.len = 0},
      .turn = ARMPROCESS,
      .audio_info = in_file_info,
  };
  // Copy the initial struct to shared memory.
  // We will start to use the shared_mqa_struct pointer exclusively from here
  // onwards.
  *shared_mqa_struct = temp_mqa_struct;

  // Block until the ARM library has handed off execution to this component
  // (with the frame_size variable)
  while (shared_mqa_struct->turn == ARMPROCESS) {
  }
  // printf("received control 1, with frame_size %d\n",
  // shared_mqa_struct->get_samples_var.frame_size);

  while (true) {
    switch (shared_mqa_struct->turn) {
    case ARMPROCESS:
      continue;
    case AMDREAD:
      get_samples(in_file, shared_mqa_struct);
      shared_mqa_struct->turn = ARMPROCESS;
      break;
    case AMDWRITE:
      play_audio(shared_mqa_struct, out_file);
      shared_mqa_struct->turn = ARMPROCESS;
      break;
    case FINISHED:
      goto END;
    }
  }
END:
  sf_close(in_file);
  return 0;
}
