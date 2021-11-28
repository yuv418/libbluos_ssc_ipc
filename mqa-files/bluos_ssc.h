#ifndef BLUOS_SSC_H
#define BLUOS_SSC_H

#include <stdint.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm_rate.h>

enum mqa_options {
	MQA_48K			= 1 << 0,
	MQA_NO_RENDER_RESAMP	= 1 << 1,
	MQA_LOW_POWER		= 1 << 2,
	MQA_DECODE_ONLY		= 1 << 3,
};

int ssc_init(const char *lib);

struct ssc_decode;

typedef int (*get_samples_fn)(int frame_size, uint8_t **buf, int eof, int *end);
typedef void (*consume_fn)(int len);
typedef size_t (*write_samples_fn)(void *, void *buf, size_t len);
typedef int (*write_size_fn)(void *);

int ssc_decode_open(struct ssc_decode **sscd,
		    int channels, int rate1, int bits, int rate2,
		    get_samples_fn get_samples, consume_fn consume,
		    write_samples_fn write_samples, write_size_fn write_size,
		    void *cb_data, int options);
int ssc_decode_read(struct ssc_decode *);
const char *ssc_decode_status(struct ssc_decode *);
void ssc_decode_close(struct ssc_decode *);

struct ssc_render;

int ssc_render_init(struct ssc_render **rend,
		    const snd_pcm_rate_info_t *info,
		    int mapping, int *chmap);
void ssc_render(struct ssc_render *rend,
		const snd_pcm_channel_area_t *dst_areas,
		int dst_offset, int dst_frames,
		const snd_pcm_channel_area_t *src_areas,
		int src_offset, int src_frames,
		int mute, unsigned int channels, snd_pcm_format_t format);
void ssc_render_free(struct ssc_render *rend);
void ssc_render_close(struct ssc_render *rend);

#endif
