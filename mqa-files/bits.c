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

#include <ctype.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bits.h"

#define array_size(a) (sizeof(a) / sizeof(a[0]))
#define min(a, b) ((a) < (b) ? (a) : (b))

void put_byte(struct bitreader *b, int v) {
  int w = b->bitend >> 5;
  int p = b->bitend & 31;
  uint32_t d;

  d = b->bitbuf[w];
  d &= ~(255 << p);
  d |= v << p;
  b->bitbuf[w] = d;

  if (p > 24) {
    w++;
    p = 32 - p;
    d = b->bitbuf[w];
    d &= ~(255 >> p);
    d |= v >> p;
    b->bitbuf[w] = d;
  }

  b->bitend += 8;
}

void fill_bits_buf(struct bitreader *b, int n) {
  uint8_t *p = b->data;

  while (b->bitend < b->bitpos + n)
    put_byte(b, *p++);

  b->data = p;
}

static void fill_bits(struct bitreader *b, int n) {
  if (b->bitpos) {
    int s = b->bitpos >> 5;

    memmove(b->bitbuf, &b->bitbuf[s], 4 * (array_size(b->bitbuf) - s));
    b->bitpos -= 32 * s;
    b->bitend -= 32 * s;
  }

  b->fill_bits(b, n);
}

uint32_t peek_bits(struct bitreader *b, unsigned n, unsigned offs) {
  unsigned end = offs + n;
  unsigned shift = 32 - n;
  uint32_t bits;
  int w, s;

  if (b->bitend < b->bitpos + end)
    fill_bits(b, end);

  if (b->bitend < b->bitpos + end) {
    if (b->eof_cb)
      b->eof_cb();
    return 0;
  }

  w = (b->bitpos + offs) >> 5;
  s = (b->bitpos + offs) & 31;

  if (b->msb) {
    bits = b->bitbuf[w] << s;
    if (s)
      bits |= b->bitbuf[w + 1] >> (32 - s);

    return bits >> shift;
  }

  bits = b->bitbuf[w] >> s;
  if (s)
    bits |= b->bitbuf[w + 1] << (32 - s);

  return bits << shift >> shift;
}

uint64_t peek_bits64(struct bitreader *b, unsigned n, unsigned offs) {
  uint64_t bits;

  if (n <= 32)
    return peek_bits(b, n, offs);

  if (b->msb) {
    bits = (uint64_t)peek_bits(b, n - 32, offs) << 32;
    bits |= peek_bits(b, 32, offs + n - 32);
  } else {
    bits = peek_bits(b, 32, offs);
    bits |= (uint64_t)peek_bits(b, n - 32, offs + 32) << 32;
  }

  return bits;
}

uint64_t get_ubits(struct bitreader *b, int n) {
  uint64_t bits = peek_bits64(b, n, 0);

  b->bitpos += n;

  if (b->get_bits_cb)
    b->get_bits_cb(bits, n);

  return bits;
}

int64_t get_sbits(struct bitreader *b, int n) {
  int64_t v = get_ubits(b, n);
  int s = 64 - n;

  return v << s >> s;
}

int find_sync(struct bitreader *b, uint64_t magic, int len, int max) {
  while (max--) {
    if (peek_bits64(b, len, 0) == magic)
      return 0;
    get_ubits(b, 1);
  }

  return -1;
}

void init_bits(struct bitreader *b) { memset(b, 0, sizeof(*b)); }

static int verbosity = 1;
static int need_nl;

int print_verbosity(int v) {
  int old = verbosity;

  if (v >= 0)
    verbosity = v;

  return old;
}

int print_verbose(int v, const char *fmt, ...) {
  va_list ap;
  int ret;

  if (verbosity < v)
    return 0;

  va_start(ap, fmt);
  ret = vprintf(fmt, ap);
  va_end(ap);

  return ret;
}

void print_data(struct bitreader *b, int size, int flags, const char *name,
                uint8_t *out) {
  int n = (size + 7) >> 3;
  int w;
  int i;

  if (!size)
    return;

  if (flags & BF_CHAR)
    w = 31;
  else
    w = 15;

  print_verbose(1, "%10s%-24s%4d: ", "", name, size);
  if (verbosity == 1)
    printf("[skipped]\n");
  else if (verbosity)
    printf("\n");

  for (i = 0; i < n; i++) {
    int s, v;

    s = min(size, 8);
    v = get_ubits(b, s);
    size -= s;

    if (out)
      *out++ = v;

    if (verbosity < 2)
      continue;

    if (!(i & w)) {
      printf("%14s%08x:", "", 8 * i);
      if (flags & BF_CHAR)
        printf(" |");
      need_nl = 1;
    }

    if (!(flags & BF_CHAR) && !(i & w >> 1))
      printf(" ");

    if (flags & BF_CHAR)
      printf("%c", isprint(v) ? v : '.');
    else
      printf(" %02x", v);

    if ((i & w) == w) {
      if (flags & BF_CHAR)
        printf("|");
      printf("\n");
      need_nl = 0;
    }
  }

  if (need_nl) {
    printf("\n");
    need_nl = 0;
  }
}

uint64_t print_field(struct bitreader *b, int size, int flags, const char *name,
                     const char **values) {
  uint64_t v;

  if (flags & BF_DATA) {
    print_data(b, size, flags, name, NULL);
    return 0;
  }

  if (flags & BF_SIGNED)
    v = get_sbits(b, size);
  else
    v = get_ubits(b, size);

  if (verbosity < 1)
    return v;

  printf("%10s%-24s%4d: ", "", name, size);
  if (flags & BF_HEX)
    printf("0x%0*" PRIx64, (size + 3) >> 2, v);
  else if (flags & BF_SIGNED)
    printf("%" PRId64, v);
  else if (flags & BF_CHAR)
    printf("0x%02x '%c'", (unsigned)v, (unsigned)v);
  else
    printf("%" PRIu64, v);

  if (values && values[v])
    printf(" [%s]", values[v]);

  if (!(flags & BF_NONL))
    printf("\n");

  return v;
}

uint64_t print_fields(struct bitreader *b, const struct bitfield *bf,
                      uint64_t *out) {
  uint64_t val[16];
  uint64_t ret = 0;
  uint64_t v;
  int i = 0;

  while (bf->name) {
    int size;

    if (bf->flags & BF_SIZE)
      size = val[bf->size & 15];
    else
      size = bf->size;

    v = print_field(b, size, bf->flags, bf->name, bf->values);

    val[i++ & 15] = v;

    if (bf->flags & BF_RET) {
      if (out)
        *out++ = v;
      ret = v;
    }

    bf++;
  }

  return ret;
}

uint64_t print_frame(struct bitreader *b, const struct frame *f, int type,
                     uint64_t pos) {
  uint64_t ret = 0;

  if (verbosity)
    printf("%08" PRIx64 ": [%x] %s\n", pos, type,
           f->name ? f->name : "unknown type");

  if (f->handler) {
    ret = f->handler(b, type);
  } else if (f->fields) {
    ret = print_fields(b, f->fields, NULL);
  }

  return ret;
}

void print_end(void) {
  if (need_nl)
    printf("\n");
  need_nl = 0;
}
