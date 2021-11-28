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

#include <stdint.h>
#include <string.h>
#include <setjmp.h>
#include <sndfile.h>

#include "bits.h"

#define BUF_SIZE 4096

static SNDFILE *file;
static int32_t buf[2 * BUF_SIZE];
static unsigned bufpos;
static unsigned bufend;

static unsigned xorbit;

static void fill_buf(void)
{
	int n;

	if (bufpos) {
		memmove(buf, &buf[2 * bufpos], 8 * (BUF_SIZE - bufpos));
		bufend -= bufpos;
		bufpos = 0;
	}

	n = sf_readf_int(file, &buf[2 * bufend], BUF_SIZE - bufend);

	if (n <= 0)
		return;

	bufend += n;
}

static void set_bit(uint32_t *buf, int msb, int pos, int v)
{
	uint32_t d = buf[pos >> 5];
	int b = pos & 31;

	if (msb)
		b = 31 - b;

	d &= ~(1 << b);
	d |= v << b;

	buf[pos >> 5] = d;
}

static void fill_bits(struct bitreader *b, int n)
{
	int32_t sl, sr, x;

	while (b->bitend < 8 * sizeof(b->bitbuf)) {
		if (bufpos == bufend)
			fill_buf();

		if (bufpos == bufend)
			break;

		sl = buf[2 * bufpos];
		sr = buf[2 * bufpos + 1];
		x = (sl ^ sr) >> xorbit & 1;

		set_bit(b->bitbuf, b->msb, b->bitend, x);

		bufpos++;
		b->bitend++;
	}
}

void init_bits_sf(struct bitreader *b, SNDFILE *f, int xbit)
{
	init_bits(b);
	b->fill_bits = fill_bits;

	file = f;
	bufpos = 0;
	bufend = 0;
	xorbit = xbit + 8;
}
