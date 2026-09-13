#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <stdint.h>

typedef struct cinavdecode
{
	AVFormatContext *demuxer;
	AVIOContext *avio;
	AVCodecContext **dec_ctx;
	AVFrame *dec_frame;
	AVPacket *packet;
	fileHandle_t file;
	int file_size;
	int file_pos;
	char file_name[MAX_OSPATH];
	struct SwsContext *swsctx_image;
	SwrContext *swr_audio;
	int eof;

	/* video */
	float fps;
	int width, height;
	uint8_t *video_res;
	long video_buffsize;
	long video_pos;
	long video_curr;
	long video_frame_size;
	double video_timestamp;

	/* audio */
	int channels, rate;
	uint8_t *audio_res;
	long audio_buffsize;
	long audio_pos;
	long audio_curr;
	long audio_frame_size;
	double audio_timestamp;
} cinavdecode_t;

static int
cinavdecode_read(void *opaque, uint8_t *buffer, int size)
{
	cinavdecode_t *state = opaque;
	int result;

	if (size <= 0 || state->file == 0)
	{
		return 0;
	}

	if (state->file_pos >= state->file_size)
	{
		return AVERROR_EOF;
	}

	if (size > state->file_size - state->file_pos)
	{
		size = state->file_size - state->file_pos;
	}

	result = FS_Read(buffer, size, state->file);
	if (result > 0)
	{
		state->file_pos += result;
	}

	return result > 0 ? result : AVERROR_EOF;
}

static int64_t
cinavdecode_seek(void *opaque, int64_t offset, int whence)
{
	cinavdecode_t *state = opaque;
	int64_t target;

	if (whence == AVSEEK_SIZE)
	{
		return state->file_size;
	}

	switch (whence & ~AVSEEK_FORCE)
	{
		case SEEK_SET:
			target = offset;
			break;
		case SEEK_CUR:
			target = state->file_pos + offset;
			break;
		case SEEK_END:
			target = state->file_size + offset;
			break;
		default:
			return -1;
	}

	if (target < 0 || target > state->file_size)
	{
		return -1;
	}

	if (target < state->file_pos)
	{
		FS_FCloseFile(state->file);
		if (FS_FOpenFile(state->file_name, &state->file, false) != state->file_size)
		{
			state->file = 0;
			return -1;
		}
		state->file_pos = 0;
	}

	while (state->file_pos < target)
	{
		byte discard[4096];
		int count = target - state->file_pos;

		if (count > (int)sizeof(discard))
		{
			count = sizeof(discard);
		}

		if (cinavdecode_read(state, discard, count) < count)
		{
			return -1;
		}
	}

	return state->file_pos;
}

static void
cinavdecode_close(cinavdecode_t *state)
{
	if (!state)
	{
		return;
	}

	if (state->video_res)
	{
		free(state->video_res);
	}

	if (state->audio_res)
	{
		free(state->audio_res);
	}

	if (state->packet)
	{
		av_packet_free(&state->packet);
	}

	if (state->dec_frame)
	{
		av_frame_free(&state->dec_frame);
	}

	if (state->swsctx_image)
	{
		sws_freeContext(state->swsctx_image);
	}

	if (state->swr_audio)
	{
		swr_free(&state->swr_audio);
	}

	if (state->demuxer)
	{
		int i;

		for (i = 0; i < state->demuxer->nb_streams; i++)
		{
			if (state->dec_ctx)
			{
				avcodec_free_context(&state->dec_ctx[i]);
			}
		}

		if (state->dec_ctx)
		{
			av_free(state->dec_ctx);
		}

		avformat_close_input(&state->demuxer);
	}

	if (state->avio)
	{
		avio_context_free(&state->avio);
	}

	if (state->file)
	{
		FS_FCloseFile(state->file);
	}

	av_free(state);
}

static cinavdecode_t *
cinavdecode_init(cinavdecode_t *state, int max_width, int max_height)
{
	int ret, i, count;

	if ((ret = avformat_find_stream_info(state->demuxer, NULL)) < 0)
	{
		/* Can't get steam info */
		cinavdecode_close(state);
		return NULL;
	}

	/* Check video info by av_dump_format(state->demuxer, 0, name, 0); */

	state->dec_ctx = av_calloc(state->demuxer->nb_streams, sizeof(AVCodecContext *));
	if (!state->dec_ctx)
	{
		/* can't allocate streams */
		cinavdecode_close(state);
		return NULL;
	}

	for (i = 0; i < state->demuxer->nb_streams; i++)
	{
		AVStream *stream = state->demuxer->streams[i];
		const AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
		AVCodecContext *codec_ctx;
		if (!dec)
		{
			/* Has no decoder */
			cinavdecode_close(state);
			return NULL;
		}

		codec_ctx = avcodec_alloc_context3(dec);
		if (!codec_ctx)
		{
			/* Failed alloc context */
			cinavdecode_close(state);
			return NULL;
		}

		ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
		if (ret < 0) {
			/* Set parameters to contex */
			cinavdecode_close(state);
			return NULL;
		}

		/* Inform the decoder about the timebase for the packet timestamps.
		 * This is highly recommended, but not mandatory. */
		codec_ctx->pkt_timebase = stream->time_base;

		/* Reencode video & audio and remux subtitles etc. */
		if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO ||
			codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				int width, height;

				codec_ctx->framerate = av_guess_frame_rate(
					state->demuxer, stream, NULL);

				width = codec_ctx->width;
				height = codec_ctx->height;

				/* smaller by width */
				if (width > max_width)
				{
					height = height * max_width / width;
					width = max_width;
				}

				/* smaller by height */
				if (height > max_height)
				{
					width = width * max_height / height;
					height = max_height;
				}

				state->fps = (float)codec_ctx->framerate.num / codec_ctx->framerate.den;
				state->width = width;
				state->height = height;

				state->swsctx_image = sws_getContext(
					codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
					width, height, AV_PIX_FMT_RGBA, 0, 0, 0, 0);
			}
			else
			{
				state->channels = codec_ctx->ch_layout.nb_channels;
				state->rate = codec_ctx->sample_rate;

				ret = swr_alloc_set_opts2(&state->swr_audio,
					&codec_ctx->ch_layout, AV_SAMPLE_FMT_S16, codec_ctx->sample_rate,
					&codec_ctx->ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate,
					0, NULL);

				if (ret < 0)
				{
					/* Can't open format sound convert */
					cinavdecode_close(state);
					return NULL;
				}

				ret = swr_init(state->swr_audio);
				if (ret < 0)
				{
					/* Can't open format sound convert */
					cinavdecode_close(state);
					return NULL;
				}
			}

			/* Open decoder */
			ret = avcodec_open2(codec_ctx, dec, NULL);
			if (ret < 0)
			{
				/* Can't open decoder */
				cinavdecode_close(state);
				return NULL;
			}

			state->dec_ctx[i] = codec_ctx;
		}
	}

	state->dec_frame = av_frame_alloc();
	if (!state->dec_frame)
	{
		/* Can't allocate frame */
		cinavdecode_close(state);
		return NULL;
	}

	state->packet = av_packet_alloc();
	if (!state->packet)
	{
		/* Can't allocate frame */
		cinavdecode_close(state);
		return NULL;
	}

	if (state->fps < 1 || state->rate < 5500)
	{
		/* too rare update */
		cinavdecode_close(state);
		return NULL;
	}

	/* Fix here if audio not in sync */
	count = state->rate * state->channels / state->fps;

	/* round up to channels and width */
	count = (count + (state->channels) - 1) & (~(state->channels) - 1);
	state->audio_frame_size = count * 2;

	/* buffer 1 second of sound */
	state->audio_buffsize = state->audio_frame_size * (state->fps + 1);
	state->audio_res = malloc(state->audio_buffsize);

	/* buffer half second of video */
	state->video_frame_size = 4 * state->width * state->height;
	state->video_buffsize = state->video_frame_size * (state->fps / 2 + 1);
	state->video_res = malloc(state->video_buffsize);

	return state;
}

/*
 * Rewrite to use callback?
 */
static cinavdecode_t *
cinavdecode_open(const char *name, int max_width, int max_height)
{
	cinavdecode_t *state = NULL;
	unsigned char *avio_buffer;
	int ret;

	state = av_calloc(1, sizeof(cinavdecode_t));
	memset(state, 0, sizeof(cinavdecode_t));

	state->file_size = FS_FOpenFile(name, &state->file, false);
	if (state->file_size < 0)
	{
		cinavdecode_close(state);
		return NULL;
	}

	Q_strlcpy(state->file_name, name, sizeof(state->file_name));
	avio_buffer = av_malloc(4096);
	if (!avio_buffer)
	{
		cinavdecode_close(state);
		return NULL;
	}

	state->avio = avio_alloc_context(avio_buffer, 4096, 0, state,
		cinavdecode_read, NULL, cinavdecode_seek);
	if (!state->avio)
	{
		av_free(avio_buffer);
		cinavdecode_close(state);
		return NULL;
	}

	state->demuxer = avformat_alloc_context();
	if (!state->demuxer)
	{
		cinavdecode_close(state);
		return NULL;
	}

	state->demuxer->pb = state->avio;
	state->demuxer->flags |= AVFMT_FLAG_CUSTOM_IO;
	ret = avformat_open_input(&state->demuxer, NULL, NULL, NULL);
	if (ret < 0)
	{
		/* can't open file */
		cinavdecode_close(state);
		return NULL;
	}

	return cinavdecode_init(state, max_width, max_height);
}

static int
next_frame_ready(const cinavdecode_t *state)
{
	if (state->eof)
	{
		/* nothing to parse */
		return 1;
	}

	if ((state->video_pos - state->video_curr) < state->video_frame_size)
	{
		/* need more frames */
		return 0;
	}

	if ((state->audio_pos - state->audio_curr) < state->audio_frame_size)
	{
		/* need more audio */
		return 0;
	}

	return 1;
}

static void
cinavdecode_parse_next(cinavdecode_t *state)
{
	do
	{
		int stream_index, ret;

		if ((ret = av_read_frame(state->demuxer, state->packet)) < 0)
		{
			state->eof = 1;
			break;
		}

		stream_index = state->packet->stream_index;

		/* send the packet with the compressed data to the decoder */
		ret = avcodec_send_packet(state->dec_ctx[stream_index], state->packet);
		if (ret < 0)
		{
			state->eof = 1;
			/* can't send packet for decode */
			break;
		}

		/* read all the output frames (in general there may be any number of them) */
		while (ret >= 0)
		{
			/* unroll of loop doesn't help with speed as internally
			 * decoded several frames */
			ret = avcodec_receive_frame(state->dec_ctx[stream_index], state->dec_frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			{
				/* End of package */
				break;
			}
			else if (ret < 0)
			{
				state->eof = 1;
				/* Decode error */
				break;
			}

			if (state->dec_ctx[stream_index]->codec_type == AVMEDIA_TYPE_AUDIO)
			{
				uint8_t *out[1];
				int ret, required_space;

				state->audio_timestamp = (
					(double)state->dec_frame->pts *
					state->dec_ctx[stream_index]->pkt_timebase.num /
					state->dec_ctx[stream_index]->pkt_timebase.den
				);

				required_space = state->dec_frame->nb_samples * 2 * state->channels;

				/* compact frame */
				if ((state->audio_pos + required_space) > state->audio_buffsize &&
					(state->audio_curr > 0))
				{
					memmove(state->audio_res,
						state->audio_res + state->audio_curr,
						state->audio_pos - state->audio_curr);
					state->audio_pos -= state->audio_curr;
					state->audio_curr = 0;
				}

				if ((state->audio_pos + required_space) > state->audio_buffsize)
				{
					uint8_t *new_buffer;

					/* add current frame size */
					state->audio_buffsize += required_space;
					/* double buffer */
					state->audio_buffsize *= 2;
					/* realloacate buffer */
					new_buffer = realloc(state->audio_res, state->audio_buffsize);
					if (!new_buffer)
					{
						/* Memalloc error */
						state->eof = 1;
						break;
					}
					state->audio_res = new_buffer;
				}

				out[0] = state->audio_res + state->audio_pos;
				ret = swr_convert(state->swr_audio, out, state->dec_frame->nb_samples,
					(const uint8_t **)state->dec_frame->data, state->dec_frame->nb_samples);
				if (ret < 0)
				{
					/* Decode error */
					state->eof = 1;
					break;
				}

				/* move current pos */
				state->audio_pos += required_space;
			}
			else if (state->dec_ctx[stream_index]->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				uint8_t *out[1];
				int linesize[1], ret, required_space;

				state->video_timestamp = (
					(double)state->dec_frame->pts *
					state->dec_ctx[stream_index]->pkt_timebase.num /
					state->dec_ctx[stream_index]->pkt_timebase.den
				);

				required_space = state->video_frame_size;

				/* compact frame */
				if ((state->video_pos + required_space) > state->video_buffsize &&
					(state->video_curr > 0))
				{
					memmove(state->video_res,
						state->video_res + state->video_curr,
						state->video_pos - state->video_curr);
					state->video_pos -= state->video_curr;
					state->video_curr = 0;
				}

				/* realloc memory */
				if ((state->video_pos + required_space) > state->video_buffsize)
				{
					uint8_t *new_buffer;
					/* add current frame size */
					state->video_buffsize += required_space;
					/* double buffer */
					state->video_buffsize *= 2;
					/* realloacate buffer */
					new_buffer = realloc(state->video_res, state->video_buffsize);
					if (!new_buffer)
					{
						/* Memalloc error */
						state->eof = 1;
						break;
					}
					state->video_res = new_buffer;
				}

				/* RGB stride with single plane */
				linesize[0] = 4 * state->width;
				out[0] = state->video_res + state->video_pos;

				ret = sws_scale(state->swsctx_image,
					(const uint8_t * const*)state->dec_frame->data,
					state->dec_frame->linesize, 0,
					state->dec_frame->height, out, linesize);

				if (ret < 0)
				{
					/* Decode error */
					state->eof = 1;
					break;
				}
				state->video_pos += required_space;
			}
		}
		av_packet_unref(state->packet);

		/* use 2.5 second of audio buffering and 1.5 video buffer for async in
		 * media stream */
		if (((float)(state->audio_pos - state->audio_curr) / state->audio_frame_size) >
				(state->fps * 3.0) ||
			((float)(state->video_pos - state->video_curr) / state->video_frame_size) >
				(state->fps * 1.5))
		{
			/* flush buffer if more than 1.5 second async in video stream */
			break;
		}
	}
	while (!next_frame_ready(state));
}

/*
 * Caller should alloacate buffer enough for single frame
 */
static int
cinavdecode_next_frame(cinavdecode_t *state, uint8_t *video, uint8_t *audio)
{
	/* read all packets */
	if (!state->eof)
	{
		cinavdecode_parse_next(state);
	};

	memcpy(audio, state->audio_res + state->audio_curr, state->audio_frame_size);
	state->audio_curr += state->audio_frame_size;
	if (state->audio_curr >= state->audio_pos)
	{
		state->audio_curr = 0;
		state->audio_pos = 0;
	}

	memcpy(video, state->video_res + state->video_curr, state->video_frame_size);
	state->video_curr += state->video_frame_size;
	if (state->video_curr >= state->video_pos)
	{
		state->video_curr = 0;
		state->video_pos = 0;
	}

	if (!state->eof ||
		((state->video_pos - state->video_curr) > 0) ||
		((state->audio_pos - state->audio_curr) > 0))
	{
		return 1;
	}
	else
	{
		/* no frames any more */
		return -1;
	}
}

