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

#ifndef BITS_H
#define BITS_H

#include <stdint.h>

struct bitreader {
	uint32_t bitbuf[32];
	unsigned bitpos;
	unsigned bitend;
	unsigned msb;
	void (*fill_bits)(struct bitreader *, int n);
	void (*get_bits_cb)(uint64_t bits, int n);
	void (*eof_cb)(void);
	void *data;
};

void init_bits(struct bitreader *b);
int find_sync(struct bitreader *b, uint64_t magic, int len, int max);
uint64_t get_ubits(struct bitreader *b, int n);
int64_t get_sbits(struct bitreader *b, int n);
uint32_t peek_bits(struct bitreader *b, unsigned n, unsigned offs);
uint64_t peek_bits64(struct bitreader *b, unsigned n, unsigned offs);
void put_byte(struct bitreader *b, int v);
void fill_bits_buf(struct bitreader *b, int n);

struct bitfield {
	int size;
	int flags;
	const char *name;
	const char **values;
};

#define BF_SIGNED	(1 << 0)
#define BF_HEX		(1 << 1)
#define BF_CHAR		(1 << 2)
#define BF_DATA		(1 << 3)
#define BF_RET		(1 << 4)
#define BF_SIZE		(1 << 5)
#define BF_NONL		(1 << 6)

struct frame {
	const char *name;
	const struct bitfield *fields;
	int (*handler)(struct bitreader *b, unsigned type);
};

int print_verbosity(int v);
int print_verbose(int v, const char *fmt, ...);
void print_data(struct bitreader *b, int size, int flags, const char *name,
		uint8_t *out);
uint64_t print_field(struct bitreader *b, int size, int flags,
		     const char *name, const char **values);
uint64_t print_fields(struct bitreader *b, const struct bitfield *bf,
		      uint64_t *out);
uint64_t print_frame(struct bitreader *b, const struct frame *f,
		     int type, uint64_t pos);
void print_end(void);

#endif
