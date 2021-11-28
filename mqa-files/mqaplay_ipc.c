/*
  Copyright (c) 2021, Mans Rullgard <mans@mansr.com>, xe <icqw1@protonmail.com>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include <sndfile.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

#include "bluos_ssc.h"

#define BUF_SIZE 4096

static int bluos_api;
static SNDFILE *outfile;

// MQA Render vars
static struct ssc_render *rend = NULL;
static snd_pcm_channel_area_t in_areas[2];
static bool in_areas_initialised = false;
static snd_pcm_channel_area_t out_areas[2];
static int channels;
static int ratio;

typedef enum { AMDREAD, AMDWRITE, ARMPROCESS, FINISHED } LIB;
typedef enum { READ, WRITE } ACTION;
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

static ipc_mqa_struct *ipc_com;

static void setup_areas(snd_pcm_channel_area_t *a, void *buf, int channels,
                        int bits) {
  int i;

  for (i = 0; i < channels; i++) {
    a[i].addr = buf;
    a[i].first = i * bits;
    a[i].step = channels * bits;
  }
}

static int get_samples(int frame_size, uint8_t **samples, int eof, int *end) {
  printf("begin get samples\n");

  ipc_com->get_samples_var.frame_size = frame_size;
  ipc_com->turn = AMDREAD; // hand the execution off to the AMD lib

  printf("rescinding control to AMDlib\n");
  // Wait for other process to hand-off to ARM decoder environment
  while (ipc_com->turn != ARMPROCESS) {
    printf("turn %d\n", ipc_com->turn);
  }

  printf("received control at get_samples\n");
  if (end) {
    if (!ipc_com->get_samples_var.size) {
      *end = 1;
      return 0;
    }

    *end = 0;
  }

  *samples = &ipc_com->input_buffer[ipc_com->get_samples_var.buf_pos];

  if (bluos_api > 0)
    ipc_com->get_samples_var.size /= ipc_com->get_samples_var.frame_size;

  printf("return size is %d\n", ipc_com->get_samples_var.size);
  return ipc_com->get_samples_var.size;
}

static void consume(int len) {
  printf("consumed %d\n", len);
  ipc_com->get_samples_var.buf_pos += len;
}

static size_t write_samples(void *p, void *buf, size_t len) {
  // s will be used as the mqa render inbuf
  int32_t *s = buf;
  size_t ret;
  int i;

  if (bluos_api < 1)
    len /= 8;

  if (!in_areas_initialised) {
    printf("initialising in_areas\n");
    for (i = 0; i < channels; i++) {
      in_areas[i].addr = s;
      in_areas[i].first = i * 32;
      in_areas[i].step = channels * 32;
    }
    in_areas_initialised = true;
  }

  ssc_render(rend, out_areas, 0, len * ratio, in_areas, 0, len, 0, channels,
             SND_PCM_FORMAT_S24);

  for (i = 0; i < len * ratio * channels; i++)
    ipc_com->output_buffer[i] <<= 8;

  // we are writing to the IPC mechanism instead of this
  // ret = sf_writef_int(outfile, outbuf, len * ratio);
  ipc_com->write_samples_var.len = len;
  ipc_com->turn = AMDWRITE; // hand the execution off to the AMD lib
  while (ipc_com->turn != ARMPROCESS) {
    printf("write turn %d\n", ipc_com->turn);
  }

  if (bluos_api < 1) {
    ret *= 8;
  }

  // printf("ret %d\n", ret);
  // we have to return len since that's how much we're "writing" to the buffer
  return len;
}

static int write_size(void *p) {
  return bluos_api < 1 ? BUF_SIZE * 8 : BUF_SIZE;
}

int main(int argc, char **argv) {
  struct ssc_decode *sscd;
  const char *lib = NULL;
  int rate1 = 0;
  int bits = 32;
  int options = 0;
  int mqa = 0;

  int shmid = shmget(25589, sizeof(ipc_mqa_struct), IPC_CREAT | 0777);
  ipc_com = shmat(shmid, NULL, 0);

  rate1 = ipc_com->audio_info.samplerate;
  channels = ipc_com->audio_info.channels;

  if (ssc_init(lib))
    return 1;

  // MQA render setup stuff

  snd_pcm_rate_info_t ri;

  ratio = 2;
  int outbuf_size = BUF_SIZE * ratio;
  // outbuf = malloc(8 * outbuf_size);

  ri.channels = channels;
  ri.in.format = SND_PCM_FORMAT_S24;
  // double fold

  // input rate is after the first fold
  ri.in.rate = rate1 * 2;
  ri.in.buffer_size = BUF_SIZE;
  ri.in.period_size = BUF_SIZE;

  ri.out.format = SND_PCM_FORMAT_S24;
  // double fold so we multiply
  ri.out.rate = rate1 * 4;
  ri.out.buffer_size = outbuf_size;
  ri.out.period_size = outbuf_size;

  if (ssc_render_init(&rend, &ri, 0, NULL) < 0) {
    fprintf(stderr, "ssc_render_init failure\n");
    return 1;
  }

  setup_areas(out_areas, &ipc_com->output_buffer, channels, bits);

  bluos_api =
      ssc_decode_open(&sscd, channels, rate1, bits, 96000, get_samples, consume,
                      write_samples, write_size, &outfile, options);

  if (bluos_api < 0)
    return 1;

  while (ssc_decode_read(sscd) > 0) {
    if (!mqa) {
      const char *s = ssc_decode_status(sscd);
      if (s[0] != '0') {
        fprintf(stderr, "%s\n", s);
        mqa = 1;
      }
    }
  }

  // Last block finished decoding
  printf("done\n");
  ipc_com->turn = FINISHED;

  ssc_decode_close(sscd);

  return 0;
}