#include "../libbluos_ssc_ipc.h"
#include "ipc_structs.h"
#include "mqa-files/mqascan.h"
#include <portaudio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// code partially stolen from
// https://github.com/hosackm/wavplayer/blob/master/src/wavplay.c

static int portaudio_callback(const void *input, void *output,
                              ulong frame_count,
                              const PaStreamCallbackTimeInfo *time_info,
                              PaStreamCallbackFlags status_flags,
                              void *user_data);
static ipc_mqa_struct *handle;
static SNDFILE *in_file;

int main(int argc, char **argv) {
  mqa_sample_rates rates;
  int num_folds;
  SF_INFO in_file_info;
  PaStream *stream;

  memset(&in_file_info, 0, sizeof(in_file_info));
  in_file = sf_open(argv[1], SFM_READ, &in_file_info);
  if (!in_file) {
    return 1;
  }

  // Begin the decoder
  rates = bluos_ssc_sndfile_mqa_rate_info(in_file, &in_file_info);
  num_folds = (rates.original_rate / rates.compressed_rate) / 2;
  if (num_folds > 2) {
    printf("warning! libbluos_ssc_ipc.so only supports a maximum of two MQA "
           "folds. setting fold count to 2.\n");
    num_folds = 2;
  }
  printf("initialising decoder with %d folds\n", num_folds);

  // TODO convert rates into number of folds
  handle = bluos_ssc_ipc_init(in_file_info, num_folds);

  // PASTART
  int error = Pa_Initialize();
  if (error != paNoError) {
    printf("Pa_Initialise failed!\n");
    return 1;
  }
  error = Pa_OpenDefaultStream(
      &stream, 0, in_file_info.channels, paInt32, rates.original_rate,
      BUF_SIZE * handle->number_of_folds, portaudio_callback, NULL);
  if (error != paNoError) {
    printf("Pa_OpenDefaultStream failed!\n");
    return 1;
  }

  error = Pa_StartStream(stream);
  if (error != paNoError) {
    printf("Pa_StartStream failed!\n");
    return 1;
  }

  while (Pa_IsStreamActive(stream)) {
    Pa_Sleep(100);
  }

  sf_close(in_file);

  error = Pa_Terminate();
  if (error != paNoError) {
    printf("Pa_Terminate failed!\n");
    return 1;
  }
  // PAEND
  return 0;
}

static int portaudio_callback(const void *input, void *output,
                              ulong frame_count,
                              const PaStreamCallbackTimeInfo *time_info,
                              PaStreamCallbackFlags status_flags,
                              void *user_data) {

  int *out = (int *)output;

  if (handle->turn != FINISHED) {
    while (true) {
      switch (handle->turn) {
      case AMDREAD:
        // read;
        bluos_ssc_sndfile_read_samples_shm(handle, in_file);
        bluos_ssc_ipc_handoff(handle, ARMPROCESS);
        break;
      case AMDWRITE:
        // write
        memcpy(out, handle->output_buffer,
               sizeof(int) * handle->write_samples_var.len *
                   handle->audio_info.channels * handle->number_of_folds);
        bluos_ssc_ipc_handoff(handle, ARMPROCESS);
        // we're done with this execution of the function
        return paContinue;
      default: // In the case the ARM encoder is running, we wait.
        continue;
        // bluos_ssc_ipc_wait(handle, AMDREAD | AMDWRITE);
      }
    }
  } else {
    return paComplete;
  }
}
