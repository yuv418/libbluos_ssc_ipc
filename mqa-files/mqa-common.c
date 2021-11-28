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

#include "mqa-common.h"

const char *mqa_rates[32] = {
	"44.1 kHz",
	"88.1 KHz",
	"176.4 kHz",
	"352.8 kHz",
	"705.6 kHz",
	"1.4112 MHz",
	"2.8224 MHz",
	"5.6448 MHz",

	"48 kHz",
	"96 kHz",
	"192 kHz",
	"384 kHz",
	"768 kHz",
	"1.536 MHz",
	"3.072 MHz",
	"6.144 MHz",

	"64 kHz",
	"128 kHz",
	"256 kHz",
	"512 kHz",
	"1.024 MHz",
	"2.048 MHz",
	"4.096 MHz",
	"8.192 MHz",
};

const char *mqb_bitdepths[4] = {
	"20 bits",
	"18 bits",
	"16 bits",
	"15 bits",
};
