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

#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include "bluos_ssc.h"

static struct ssc_decode *(*lssc_decode_open)(
	int *channels, int *rate1, int *bits, int rate2,
	get_samples_fn, consume_fn, write_samples_fn, write_size_fn,
	int *fd, void **cb_data, int options, const char *device);
static struct ssc_decode *(*lssc_decode_open_reader)(
	int *channels, int *rate1, int *bits, int rate2,
	get_samples_fn, consume_fn, write_samples_fn, write_size_fn,
	int *fd, void **cb_data, int options, int device_options,
	int16_t *debug, const char *device, const char **devstr,
	const char *blob);
static int (*lssc_decode_read)(struct ssc_decode *);
static const char *(*lssc_decode_device)(struct ssc_decode *);
static const char *(*lssc_decode_status)(struct ssc_decode *);
static void (*lssc_decode_close)(struct ssc_decode *);

static int (*lssc_render_init)(
	struct ssc_render **sscr, const snd_pcm_rate_info_t *info,
	int mapping, int *chmap);
static void (*lssc_render)(
	struct ssc_render *sscr,
	const snd_pcm_channel_area_t *dst_areas, int dst_offset, int dst_frames,
	const snd_pcm_channel_area_t *src_areas, int src_offset, int src_frames,
	int mute, unsigned int channels, snd_pcm_format_t format);
static void (*lssc_render_free)(struct ssc_render *sscr);
static void (*lssc_render_close)(struct ssc_render *sscr);

static int ssc_error;

static void *do_dlsym(void *dl, const char *sym, int opt)
{
	void *r = dlsym(dl, sym);

	if (!r && !opt) {
		fprintf(stderr, "%s\n", dlerror());
		ssc_error |= 1;
	}

	return r;
}

int ssc_init(const char *lib)
{
	void *dl;

	if (!lib)
		lib = "libbluos_ssc.so";

	dl = dlopen(lib, RTLD_NOW | RTLD_LOCAL);
	if (!dl) {
		fprintf(stderr, "%s: %s\n", lib, dlerror());
		return -1;
	}

	lssc_decode_open = do_dlsym(dl, "ssc_decode_open", 1);
	lssc_decode_open_reader = do_dlsym(dl, "ssc_decode_open_reader", 1);

	if (!lssc_decode_open && !lssc_decode_open_reader) {
		fprintf(stderr, "%s\n", dlerror());
		return -1;
	}

	lssc_decode_read = do_dlsym(dl, "ssc_decode_read", 0);
	lssc_decode_device = do_dlsym(dl, "ssc_decode_device", 1);
	lssc_decode_status = do_dlsym(dl, "ssc_decode_status", 0);
	lssc_decode_close = do_dlsym(dl, "ssc_decode_close", 0);

	lssc_render_init = do_dlsym(dl, "ssc_render_init", 0);
	lssc_render = do_dlsym(dl, "ssc_render", 0);
	lssc_render_free = do_dlsym(dl, "ssc_render_free", 0);
	lssc_render_close = do_dlsym(dl, "ssc_render_close", 0);

	return ssc_error ? -1 : 0;
}

int ssc_decode_open(struct ssc_decode **sscd,
		    int channels, int rate1, int bits, int rate2,
		    get_samples_fn get_samples, consume_fn consume,
		    write_samples_fn write_samples, write_size_fn write_size,
		    void *cb_data, int options)
{
	const char *dev = NULL;
	int ssc_api;
	int dummy = 0;
	int nc = 0;

	if (lssc_decode_open_reader) {
		*sscd = lssc_decode_open_reader(
			&channels, &rate1, &bits, rate2,
			get_samples, consume, write_samples, write_size,
			&dummy, &cb_data, options, 0,
			NULL, "dev", &dev, "default");

		if (!*sscd) {
			fprintf(stderr, "ssc_decode_open_reader failed\n");
			return -1;
		}

		return 2;
	}

	*sscd = lssc_decode_open(&channels, &rate1, &bits, rate2,
				 get_samples, consume, write_samples,
				 write_size, &dummy, &cb_data,
				 options, "dev");

	if (!*sscd) {
		fprintf(stderr, "ssc_decode_open failed\n");
		return -1;
	}

	dev = lssc_decode_device(*sscd);

	while ((dev = strchr(dev, ','))) {
		dev++;
		nc++;
	}

	if (nc == 2) {
		ssc_api = 0;
	} else if (nc == 3) {
		ssc_api = 1;
	} else {
		fprintf(stderr, "warning: unknown libbluos_ssc version\n");
		ssc_api = 1;
	}

	return ssc_api;
}

int ssc_decode_read(struct ssc_decode *sscd)
{
	return lssc_decode_read(sscd);
}

const char *ssc_decode_status(struct ssc_decode *sscd)
{
	return lssc_decode_status(sscd);
}

void ssc_decode_close(struct ssc_decode *sscd)
{
	lssc_decode_close(sscd);
}

int ssc_render_init(struct ssc_render **sscr, const snd_pcm_rate_info_t *info,
		    int mapping, int *chmap)
{
	return lssc_render_init(sscr, info, mapping, chmap);
}

void ssc_render(struct ssc_render *sscr,
		const snd_pcm_channel_area_t *dst_areas,
		int dst_offset, int dst_frames,
		const snd_pcm_channel_area_t *src_areas,
		int src_offset, int src_frames,
		int mute, unsigned int channels,
		snd_pcm_format_t format)
{
	return lssc_render(sscr, dst_areas, dst_offset, dst_frames,
			   src_areas, src_offset, src_frames,
			   mute, channels, format);
}

void ssc_render_free(struct ssc_render *sscr)
{
	lssc_render_free(sscr);
}

void ssc_render_close(struct ssc_render *sscr)
{
	lssc_render_close(sscr);
}
