#ifndef MQASCAN_H_
#define MQASCAN_H_

#include <sndfile.h>

typedef struct {
  uint64_t original_rate;
  uint64_t compressed_rate;
} mqa_sample_rates;

static int scan_file(SNDFILE *file, SF_INFO *fmt, int start, int mqabit);
static mqa_sample_rates get_mqa_sample_rates();

#endif // MQASCAN_H_
