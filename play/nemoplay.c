#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoplay.h>
#include <nemomisc.h>

void __attribute__((constructor(101))) nemoplay_initialize(void)
{
	av_register_all();
}

void __attribute__((destructor(101))) nemoplay_finalize(void)
{
}

struct nemoplay *nemoplay_create(void)
{
	struct nemoplay *play;

	play = (struct nemoplay *)malloc(sizeof(struct nemoplay));
	if (play == NULL)
		return NULL;
	memset(play, 0, sizeof(struct nemoplay));

	if (pthread_mutex_init(&play->lock, NULL) != 0)
		goto err1;

	if (pthread_cond_init(&play->signal, NULL) != 0)
		goto err2;

	play->video_queue = nemoplay_queue_create();
	play->audio_queue = nemoplay_queue_create();
	play->subtitle_queue = nemoplay_queue_create();

	play->video_clock = nemoplay_clock_create();
	play->audio_clock = nemoplay_clock_create();

	return play;

err2:
	pthread_mutex_destroy(&play->lock);

err1:
	free(play);

	return NULL;
}

void nemoplay_destroy(struct nemoplay *play)
{
	nemoplay_clock_destroy(play->video_clock);
	nemoplay_clock_destroy(play->audio_clock);

	nemoplay_queue_destroy(play->video_queue);
	nemoplay_queue_destroy(play->audio_queue);
	nemoplay_queue_destroy(play->subtitle_queue);

	if (play->swr != NULL)
		swr_free(&play->swr);

	if (play->container != NULL)
		avformat_close_input(&play->container);

	pthread_cond_destroy(&play->signal);
	pthread_mutex_destroy(&play->lock);

	free(play);
}

int nemoplay_prepare_media(struct nemoplay *play, const char *mediapath)
{
	AVFormatContext *container;
	AVCodecContext *context;
	AVCodec *codec;
	AVCodecContext *video_context = NULL;
	AVCodecContext *audio_context = NULL;
	AVCodecContext *subtitle_context = NULL;
	int video_stream = -1;
	int audio_stream = -1;
	int subtitle_stream = -1;
	int i;

	container = avformat_alloc_context();
	if (container == NULL)
		return -1;

	if (avformat_open_input(&container, mediapath, NULL, NULL) < 0)
		goto err1;

	if (avformat_find_stream_info(container, NULL) < 0)
		goto err1;

	for (i = 0; i < container->nb_streams; i++) {
		context = container->streams[i]->codec;

		if (context->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream = i;
			video_context = context;

			codec = avcodec_find_decoder(context->codec_id);
			if (codec != NULL)
				avcodec_open2(context, codec, NULL);
		} else if (context->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream = i;
			audio_context = context;

			codec = avcodec_find_decoder(context->codec_id);
			if (codec != NULL)
				avcodec_open2(context, codec, NULL);
		} else if (context->codec_type == AVMEDIA_TYPE_SUBTITLE) {
			subtitle_stream = i;
			subtitle_context = context;
		}
	}

	video_stream = av_find_best_stream(container, AVMEDIA_TYPE_VIDEO, video_stream, -1, NULL, 0);
	audio_stream = av_find_best_stream(container, AVMEDIA_TYPE_AUDIO, audio_stream, video_stream, NULL, 0);
	subtitle_stream = av_find_best_stream(container, AVMEDIA_TYPE_SUBTITLE, subtitle_stream, audio_stream >= 0 ? audio_stream : video_stream, NULL, 0);

	if (video_context != NULL) {
		play->video_width = video_context->width;
		play->video_height = video_context->height;
		play->video_timebase = av_q2d(container->streams[video_stream]->time_base);
	}

	if (audio_context != NULL) {
		SwrContext *swr;

		play->audio_channels = audio_context->channels;
		play->audio_samplerate = audio_context->sample_rate;
		play->audio_samplebits = 16;
		play->audio_timebase = av_q2d(container->streams[audio_stream]->time_base);

		swr = swr_alloc();
		av_opt_set_int(swr, "in_channel_layout", audio_context->channel_layout, 0);
		av_opt_set_int(swr, "out_channel_layout", audio_context->channel_layout, 0);
		av_opt_set_int(swr, "in_sample_rate", audio_context->sample_rate, 0);
		av_opt_set_int(swr, "out_sample_rate", audio_context->sample_rate, 0);
		av_opt_set_sample_fmt(swr, "in_sample_fmt", audio_context->sample_fmt, 0);
		av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
		swr_init(swr);

		play->swr = swr;
	}

	play->container = container;

	play->video_context = video_context;
	play->audio_context = audio_context;
	play->subtitle_context = subtitle_context;
	play->video_stream = video_stream;
	play->audio_stream = audio_stream;
	play->subtitle_stream = subtitle_stream;

	play->video_framerate = av_q2d(av_guess_frame_rate(container, container->streams[video_stream], NULL));

	play->state = NEMOPLAY_PLAY_STATE;

	return 0;

err1:
	avformat_close_input(&container);

	return -1;
}

void nemoplay_finish_media(struct nemoplay *play)
{
}

int nemoplay_decode_media(struct nemoplay *play, int reqcount, int maxcount)
{
	AVFormatContext *container = play->container;
	AVCodecContext *video_context = play->video_context;
	AVCodecContext *audio_context = play->audio_context;
	AVCodecContext *subtitle_context = play->subtitle_context;
	AVFrame *frame;
	AVPacket packet;
	int video_stream = play->video_stream;
	int audio_stream = play->audio_stream;
	int subtitle_stream = play->subtitle_stream;
	int finished = 0;
	int i;

	frame = av_frame_alloc();

	for (i = 0; i < reqcount && play->state == NEMOPLAY_PLAY_STATE; i++) {
		if (av_read_frame(container, &packet) < 0) {
			play->state = NEMOPLAY_IDLE_STATE;
		} else if (packet.stream_index == video_stream) {
			avcodec_decode_video2(video_context, frame, &finished, &packet);

			if (finished != 0) {
				struct playone *one;
				uint8_t *y, *u, *v;

				y = (uint8_t *)malloc(frame->linesize[0] * frame->height);
				u = (uint8_t *)malloc(frame->linesize[1] * frame->height / 2);
				v = (uint8_t *)malloc(frame->linesize[2] * frame->height / 2);

				memcpy(y, frame->data[0], frame->linesize[0] * frame->height);
				memcpy(u, frame->data[1], frame->linesize[1] * frame->height / 2);
				memcpy(v, frame->data[2], frame->linesize[2] * frame->height / 2);

				one = nemoplay_queue_create_one();
				one->cmd = NEMOPLAY_QUEUE_NORMAL_COMMAND;
				one->pts = play->video_timebase * av_frame_get_best_effort_timestamp(frame);
				one->serial = play->video_queue->serial;

				one->y = y;
				one->u = u;
				one->v = v;

				nemoplay_queue_enqueue(play->video_queue, one);
			}

			av_frame_unref(frame);
		} else if (packet.stream_index == audio_stream) {
			uint8_t *buffer;
			int buffersize;
			int samplesize;

			avcodec_decode_audio4(audio_context, frame, &finished, &packet);

			buffersize = av_samples_get_buffer_size(NULL, audio_context->channels, frame->nb_samples, audio_context->sample_fmt, 1);

			if (finished != 0) {
				struct playone *one;

				buffer = (uint8_t *)malloc(buffersize);

				samplesize = swr_convert(play->swr, &buffer, buffersize, (const uint8_t **)frame->extended_data, frame->nb_samples);

				one = nemoplay_queue_create_one();
				one->cmd = NEMOPLAY_QUEUE_NORMAL_COMMAND;
				one->pts = play->audio_timebase * av_frame_get_best_effort_timestamp(frame);
				one->serial = play->audio_queue->serial;

				one->data = buffer;
				one->size = samplesize * audio_context->channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

				nemoplay_queue_enqueue(play->audio_queue, one);
			}

			av_frame_unref(frame);
		} else if (packet.stream_index == subtitle_stream) {
		}

		if (nemoplay_queue_get_count(play->video_queue) > maxcount &&
				nemoplay_queue_get_count(play->audio_queue) > maxcount) {
			play->state = NEMOPLAY_FULL_STATE;
		}

		av_free_packet(&packet);
	}

	av_frame_free(&frame);

	return 0;
}

int nemoplay_seek_media(struct nemoplay *play, double pos, double min, double max)
{
	nemoplay_queue_flush(play->video_queue);
	nemoplay_queue_flush(play->audio_queue);
	nemoplay_queue_flush(play->subtitle_queue);

	if (avformat_seek_file(play->container, -1, (int64_t)(min * AV_TIME_BASE), (int64_t)(pos * AV_TIME_BASE), (int64_t)(max * AV_TIME_BASE), 0) < 0)
		return -1;

	return 0;
}

void nemoplay_wait_media(struct nemoplay *play)
{
	pthread_mutex_lock(&play->lock);

	if (play->state == NEMOPLAY_FULL_STATE || play->state == NEMOPLAY_STOP_STATE || play->state == NEMOPLAY_IDLE_STATE)
		pthread_cond_wait(&play->signal, &play->lock);

	pthread_mutex_unlock(&play->lock);
}

void nemoplay_set_state(struct nemoplay *play, int state)
{
	if (play->state == NEMOPLAY_DONE_STATE)
		return;

	pthread_mutex_lock(&play->lock);

	play->state = state;

	pthread_cond_signal(&play->signal);

	pthread_mutex_unlock(&play->lock);

	if (state == NEMOPLAY_PLAY_STATE) {
		nemoplay_queue_set_state(play->video_queue, NEMOPLAY_QUEUE_NORMAL_STATE);
		nemoplay_queue_set_state(play->audio_queue, NEMOPLAY_QUEUE_NORMAL_STATE);
		nemoplay_queue_set_state(play->subtitle_queue, NEMOPLAY_QUEUE_NORMAL_STATE);
	} else if (state == NEMOPLAY_STOP_STATE) {
		nemoplay_queue_set_state(play->video_queue, NEMOPLAY_QUEUE_STOP_STATE);
		nemoplay_queue_set_state(play->audio_queue, NEMOPLAY_QUEUE_STOP_STATE);
		nemoplay_queue_set_state(play->subtitle_queue, NEMOPLAY_QUEUE_STOP_STATE);
	} else if (state == NEMOPLAY_DONE_STATE) {
		nemoplay_queue_set_state(play->video_queue, NEMOPLAY_QUEUE_DONE_STATE);
		nemoplay_queue_set_state(play->audio_queue, NEMOPLAY_QUEUE_DONE_STATE);
		nemoplay_queue_set_state(play->subtitle_queue, NEMOPLAY_QUEUE_DONE_STATE);
	}
}

void nemoplay_enter_thread(struct nemoplay *play)
{
	pthread_mutex_lock(&play->lock);

	play->threadcount++;

	pthread_cond_signal(&play->signal);

	pthread_mutex_unlock(&play->lock);
}

void nemoplay_leave_thread(struct nemoplay *play)
{
	pthread_mutex_lock(&play->lock);

	play->threadcount--;

	pthread_cond_signal(&play->signal);

	pthread_mutex_unlock(&play->lock);
}

void nemoplay_wait_thread(struct nemoplay *play)
{
	pthread_mutex_lock(&play->lock);

	while (play->threadcount > 0)
		pthread_cond_wait(&play->signal, &play->lock);

	pthread_mutex_unlock(&play->lock);
}