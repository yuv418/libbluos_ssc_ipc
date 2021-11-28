/*
  Copyright (c) 2017, Mans Rullgard <mans@mansr.com>
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

#include <gmp.h>
#include <inttypes.h>
#include <setjmp.h>
#include <sndfile.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bits.h"
#include "blake2.h"
#include "mqa-common.h"
#include "mqascan.h"
#include "sfbits.h"

#define MQA_MAGIC 0x11319207d5

struct metadata {
  unsigned size;
  uint8_t data[256];
};

static struct bitreader br;

static struct metadata *metadata;
static int metadata_frags;
static int metadata_type = 1;

static jmp_buf eofjmp;

static unsigned bitnum;
static unsigned abspos;

static uint32_t csum;

static int verbosity = 1;
static int packet_type = -1;
static int single;

static mqa_sample_rates mqa_samplerates;

static uint32_t update_csum(uint64_t bits, int n) {
  while (n--) {
    int x = csum & 1;
    csum = csum >> 1 | bits << 31;
    if (x)
      csum ^= 3;
    bits >>= 1;
  }

  return csum & 15;
}

static void get_bits_cb(uint64_t bits, int n) {
  bitnum += n;
  update_csum(bits, n);
}

static void eof_cb(void) { longjmp(eofjmp, 1); }

static int handle_hole(struct bitreader *b, unsigned ptype) {
  int size = print_field(b, 4, 0, "size", NULL);

  if (size == 15)
    size = print_field(b, 12, 0, "size", NULL);

  print_data(b, size, 0, "hole", NULL);

  return 0;
}

static const struct bitfield auth_fields[] = {{63, BF_HEX, "unknown"},
                                              {1, 0, "check_lsb"},
                                              {24, BF_HEX, "unknown"},
                                              {
                                                  4,
                                                  BF_HEX,
                                                  "auth_info",
                                              },
                                              {4, 0, "auth_level"},
                                              {32, BF_HEX, "stage2_crc"},
                                              {256, BF_DATA, "stage1_hash"},
                                              {256, BF_DATA, "lsb_hash[0]"},
                                              {256, BF_DATA, "lsb_hash[1]"},
                                              {256, BF_DATA, "lsb_hash[2]"},
                                              {256, BF_DATA, "lsb_hash[3]"},
                                              {256, BF_DATA, "lsb_hash[4]"},
                                              {256, BF_DATA, "lsb_hash[5]"},
                                              {256, BF_DATA, "lsb_hash[6]"},
                                              {256, BF_DATA, "lsb_hash[7]"},
                                              {256, BF_DATA, "lsb_hash[8]"},
                                              {32, BF_HEX, "unknown"},
                                              {96, BF_DATA, "unknown"},
                                              {256, BF_DATA, "auth_hash"},
                                              {0}};

static int handle_auth(struct bitreader *b, unsigned ptype) {
  struct bitreader br;
  blake2s_state b2s;
  uint8_t data[384];
  size_t size;
  mpz_t mod;
  mpz_t msg;
  int key;
  int i;

  key = print_field(b, 4, 0, "auth_level", NULL);
  print_data(b, 3072, 0, "auth_data", data);

  if (print_verbosity(-1) < 1)
    return 0;

  mpz_inits(mod, msg, NULL);

  mpz_import(mod, 384, 1, 1, 0, 0, mqa_auth_keys[key]);
  mpz_import(msg, 384, 1, 1, 0, 0, data);

  mpz_powm_ui(msg, msg, 3, mod);
  mpz_export(data, &size, 1, 1, 0, 0, msg);

  mpz_clears(mod, msg, NULL);

  memmove(data + 384 - size, data, size);

  blake2s_init(&b2s, 32);
  blake2s_update(&b2s, data + 352, 32);

  for (i = 0; i < 11; i++) {
    blake2s_state b2i = b2s;
    uint8_t il[4] = {i};
    uint8_t h[32];
    int j;

    blake2s_update(&b2i, &il, 4);
    blake2s_final(&b2i, h, 32);

    for (j = 0; j < 32; j++)
      data[32 * i + j] ^= h[j];
  }

  data[0] &= 0x7f;

  init_bits(&br);
  br.fill_bits = fill_bits_buf;
  br.data = data;

  print_verbose(1, "%10s[begin decrypted auth_data]\n", "");
  print_fields(&br, auth_fields, NULL);
  print_verbose(1, "%10s[end decrypted auth_data]\n", "");

  return 0;
}

static const struct bitfield datasync_fields[] = {
    {36, BF_HEX, "magic"},
    {1, BF_HEX, "stream_pos_flag"},
    {1, 0, "pad"},
    {5, BF_HEX | BF_RET, "orig_rate", mqa_rates},
    {5, BF_HEX | BF_RET, "src_rate", mqa_rates},
    {5, 0, "render_filter"},
    {2, 0, "unknown_1"},
    {2, 0, "render_bitdepth", mqb_bitdepths},
    {4, BF_HEX, "unknown_2"},
    {4, BF_HEX, "auth_info"},
    {4, BF_HEX, "auth_level"},
    {7, 0, "item_count"},
    {0}};

static int handle_datasync(struct bitreader *b, unsigned ptype) {
  uint64_t val[2];
  int stream_pos_flag;
  unsigned pos = 0;
  int offs;
  int n;
  int size[128];
  int type[128];
  int i;

  print_fields(b, datasync_fields, val);
  // get val
  for (int j = 0; j < 2; j++) {
    printf("i %d, data %lu\n", j, val[j]);
  }

  mqa_samplerates.compressed_rate = val[1];
  mqa_samplerates.original_rate = val[0];

  return 0;
}

static const struct bitfield metadata_fields[] = {{7, BF_HEX | BF_RET, "type"},
                                                  {1, 0, "is_last"},
                                                  {12, BF_RET, "fragment"},
                                                  {8, BF_RET, "size"},
                                                  {0}};

static int handle_metadata(struct bitreader *b, unsigned ptype) {
  uint64_t val[3];
  int type, frag, size;
  uint8_t *buf = NULL;

  print_fields(b, metadata_fields, val);

  type = val[0];
  frag = val[1];
  size = val[2] + 1;

  if (type == metadata_type) {
    if (frag >= metadata_frags) {
      int n = frag + 1;
      metadata = realloc(metadata, n * sizeof(*metadata));
      memset(&metadata[metadata_frags], 0,
             (n - metadata_frags) * sizeof(*metadata));
      metadata_frags = n;
    }

    metadata[frag].size = size;
    buf = metadata[frag].data;
  }

  print_data(b, 8 * size, BF_CHAR, "data", buf);

  return 0;
}

static const struct bitfield recon_fields[] = {
    {12, BF_HEX, "size"}, {0, BF_DATA | BF_SIZE, "data"}, {0}};

static const struct bitfield terminate_fields[] = {{17, BF_HEX, "bits_to_end"},
                                                   {0}};

static const struct frame packets[16] = {
    [0] =
        {
            .name = "hole",
            .handler = handle_hole,
        },
    [1] =
        {
            .name = "reconstruction",
            .fields = recon_fields,
        },
    [3] =
        {
            .name = "terminate",
            .fields = terminate_fields,
        },
    [4] =
        {
            .name = "authentication",
            .handler = handle_auth,
        },
    [5] =
        {
            .name = "datasync",
            .handler = handle_datasync,
        },
    [7] =
        {
            .name = "metadata",
            .handler = handle_metadata,
        },
};

static int find_mqa_sync(struct bitreader *b) {
  int err = find_sync(b, MQA_MAGIC, 40, 65536);

  if (err)
    return err;

  if (peek_bits(b, 1, 40)) {
    int n = peek_bits(b, 7, 73);
    abspos = peek_bits(b, 32, 80 + 16 * n);
  }

  csum = abspos & 15;

  return 0;
}

static void scan_mqa(struct bitreader *b) {
  for (;;) {
    int startbit = bitnum;
    int type = get_ubits(b, 4);
    const struct frame *p = &packets[type];
    int pcs;
    int cs;

    if (!p->name) {
      printf("%08x: unknown packet type %d, abort\n", startbit, type);
      return;
    }

    if (packet_type >= 0 && type != packet_type)
      print_verbosity(0);

    p->handler(b, type);

    // cs = update_csum(0, -(bitnum - startbit) & 31);
    pcs = print_field(b, 4, BF_HEX, "checksum", NULL);

    print_verbosity(verbosity);

    if (single && (packet_type < 0 || type == packet_type))
      break;

    // We really don't care about this part
    // since single = true for our uses
    if (cs != pcs) {
      printf("%08x: checksum error, resynchronising\n", bitnum);
      find_mqa_sync(b);
      continue;
    }

    abspos += bitnum - startbit;
    csum = abspos & 15;
  }
}

static void write_metadata(const char *file) {
  FILE *md = fopen(file, "w");
  int i;

  if (!md) {
    perror(file);
    return;
  }

  for (i = 0; i < metadata_frags; i++) {
    if (!metadata[i].size)
      fprintf(stderr, "%s: fragment %d missing\n", file, i);
    fwrite(metadata[i].data, 1, metadata[i].size, md);
  }

  fclose(md);

  free(metadata);
  metadata = NULL;
  metadata_frags = 0;
}

static int scan_file(SNDFILE *file, SF_INFO *fmt, int start, int mqabit) {
  int i;

  abspos = 0;
  // only run one scan, since that's all we need
  single = 1;

  if (fmt->channels != 2) {
    fprintf(stderr, "file: %d channels not allowed\n", fmt->channels);
    return -1;
  }

  print_verbosity(verbosity);

  if (!setjmp(eofjmp)) {
    for (i = mqabit; i < 16; i++) {
      sf_seek(file, start, SEEK_SET);
      init_bits_sf(&br, file, i);
      br.msb = 0;
      br.get_bits_cb = get_bits_cb;
      br.eof_cb = eof_cb;
      bitnum = 0;

      if (!find_mqa_sync(&br))
        break;
    }

    if (i < 16) {
      printf("%08x: MQA signature at bit %d\n", bitnum, i);
      scan_mqa(&br);
    }
  }

  print_end();
  // Re-seek to beginning for later playback/decoding
  sf_seek(file, start, SEEK_SET);

  return 0;
}

static mqa_sample_rates get_mqa_sample_rates() {
  // only run this after scan_file is complete
  return mqa_samplerates;
}
