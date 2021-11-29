// TODO don't hardcode anything
#include <sndfile.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

#include "libbluos_ssc_ipc.h"
#include "mqa-files/mqascan.h"

// Copied from the ARM version
// Code from Mans Rullgard:
// https://code.videolan.org/mansr/mqa/-/blob/master/mqadec.c How do I "cite"
// this? Hmm...
void bluos_ssc_sndfile_read_samples_shm(ipc_mqa_struct *handle,
                                        SNDFILE *in_file) {
  // printf("reading audio\n");
  get_samples_data *gs_var = &handle->get_samples_var;

  gs_var->size = 0;
  if (gs_var->buf_pos > 0) {
    gs_var->size = gs_var->buf_end - gs_var->buf_pos;
    memmove(handle->input_buffer, handle->input_buffer + gs_var->buf_pos,
            gs_var->size);

    gs_var->buf_pos = 0;
    gs_var->buf_end = gs_var->size;
  }

  if (gs_var->buf_end < BUF_SIZE) {
    int frames = (BUF_SIZE - gs_var->buf_end) / gs_var->frame_size;

    frames = sf_readf_int(
        in_file, (int *)&handle->input_buffer[gs_var->buf_end], frames);
    gs_var->buf_end += frames * gs_var->frame_size;
  }

  gs_var->size = gs_var->buf_end - gs_var->buf_pos;
  gs_var->size -= gs_var->size % gs_var->frame_size;
  // printf("gsvar size %d\n", gs_var->size);
}

// Initialise the IPC structs for shared memory communication
ipc_mqa_struct *bluos_ssc_ipc_init(SF_INFO in_file_info,
                                   uint8_t number_of_folds) {
  int shmid;
  ipc_mqa_struct *shared_mqa_struct;

  // Get the shared memory for this location
  shmid = shmget(SHM_KEY, sizeof(ipc_mqa_struct), IPC_CREAT | 0777);
  shared_mqa_struct = shmat(shmid, NULL, 0);

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
      .number_of_folds = number_of_folds,
  };
  // Copy the initial struct to shared memory.
  // We will start to use the shared_mqa_struct pointer exclusively from here
  // onwards.
  *shared_mqa_struct = temp_mqa_struct;

  // We block until the decoder has put the initial details required into the
  // shared_mqa_struct, and then return the handle.
  while (shared_mqa_struct->turn != AMDREAD) {
  }

  // TODO fork mqaplay_ipc here using a config file
  // that has paths for qemu-arm-static and the binary file

  return shared_mqa_struct;
}

void bluos_ssc_ipc_handoff(ipc_mqa_struct *handle) {
  handle->turn = ARMPROCESS;
}

mqa_sample_rates bluos_ssc_sndfile_mqa_rate_info(SNDFILE *in_file,
                                                 SF_INFO *in_file_info) {
  // wrapper for the MQA scan stuff
  scan_file(in_file, in_file_info);
  return get_mqa_sample_rates();
}
