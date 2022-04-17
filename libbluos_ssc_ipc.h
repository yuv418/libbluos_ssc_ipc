#ifndef LIBBLUOS_SSC_IPC_H_
#define LIBBLUOS_SSC_IPC_H_

#include "ipc_structs.h"
#include "mqa-files/mqascan.h"
#include <stdint.h>

// You don't actually have to use libsndfile and read from a file,
// the in_file_info has to be stored in this struct format.
//
// It would stll work for streams.
//
// The number of folds is the number of times MQA will be unfolded is a
// parameter. One fold doubles the the sample rate, and two folds quadruples it.
// That's why our output_buffer is the size (BUF_SIZE * 4)—the maximum size we
// might need when unfolding.
//
// In my testing, the library fails to unfold more than twice. Which is a shame
// for the one 384kHz test MQA file. In practice, I haven't found any audio that
// needs more than two folds.
ipc_mqa_struct *bluos_ssc_ipc_init(SF_INFO in_file_info,
                                   uint8_t number_of_folds);
// Helper functions for SNDFILEs

// Read block from SNDFILE for processing in decoder context
void bluos_ssc_sndfile_read_samples_shm(ipc_mqa_struct *handle,
                                        SNDFILE *in_file);
// TODO make sure this seeks to the original position
// Returns the original and final sample rates of the given input file (length
// of uint64_t is two)
mqa_sample_rates bluos_ssc_sndfile_mqa_rate_info(SNDFILE *in_file,
                                                 SF_INFO *in_file_info);

#endif // LIBBLUOS_SSC_IPC_H_
