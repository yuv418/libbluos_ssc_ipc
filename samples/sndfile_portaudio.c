#include "../libbluos_ssc_ipc.h"
#include "ipc_structs.h"
#include "mqa-files/mqascan.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  // lol help

  ipc_mqa_struct *handle;
  mqa_sample_rates rates;
  int num_folds;
  SNDFILE *in_file;
  SNDFILE *out_file;
  SF_INFO in_file_info;
  SF_INFO out_file_info;

  memset(&in_file_info, 0, sizeof(in_file_info));
  in_file = sf_open(argv[1], SFM_READ, &in_file_info);
  if (!in_file) {
    return 1;
  }

  // Begin the decoder
  rates = bluos_ssc_sndfile_mqa_rate_info(in_file, &in_file_info);
  num_folds = (rates.original_rate / rates.compressed_rate) / 2;
  printf("initialising decoder with %d folds\n", num_folds);

  // Set up outfile
  out_file_info.samplerate = rates.original_rate; // infmt.samplerate << rs;
  out_file_info.channels = in_file_info.channels;
  out_file_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
  out_file = sf_open(argv[2], SFM_WRITE, &out_file_info);
  if (!out_file) {
    // printf("sf_open (out) failed!\n");
    return 1;
  }

  // TODO convert rates into number of folds
  handle = bluos_ssc_ipc_init(in_file_info, num_folds);

  out_file_info.samplerate = rates.original_rate; // infmt.samplerate << rs;
  out_file_info.channels = in_file_info.channels;
  out_file_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

  while (handle->turn != FINISHED) {
    switch (handle->turn) {
    case AMDREAD:
      // read;
      bluos_ssc_sndfile_read_samples_shm(handle, in_file);
      goto handoff_point;
    case AMDWRITE:
      // write
      sf_writef_int(out_file, handle->output_buffer,
                    handle->write_samples_var.len * num_folds);
      goto handoff_point;
    handoff_point:
      bluos_ssc_ipc_handoff(handle);
      break;
    default:
      continue;
    }
  }
}
