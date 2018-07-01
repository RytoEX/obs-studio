/******************************************************************************
    Copyright (C) 2014 by Ruwen Hahn <palana@stunned.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "media-remux.h"

#include "../util/base.h"
#include "../util/bmem.h"
#include "../util/platform.h"

#include <libavformat/avformat.h>

#include <sys/types.h>
#include <sys/stat.h>

#if LIBAVCODEC_VERSION_MAJOR >= 58
#define CODEC_FLAG_GLOBAL_H AV_CODEC_FLAG_GLOBAL_HEADER
#else
#define CODEC_FLAG_GLOBAL_H CODEC_FLAG_GLOBAL_HEADER
#endif

struct media_remux_job {
	int64_t in_size;
	AVFormatContext *ifmt_ctx, *ofmt_ctx;
};


// experiment
typedef struct {
	struct AVAESCTR* aes_ctr;
	uint8_t* auxiliary_info;
	size_t auxiliary_info_size;
	size_t auxiliary_info_alloc_size;
	uint32_t auxiliary_info_entries;

	/* subsample support */
	int use_subsamples;
	uint16_t subsample_count;
	size_t auxiliary_info_subsample_start;
	uint8_t* auxiliary_info_sizes;
	size_t  auxiliary_info_sizes_alloc_size;
} MOVMuxCencContext;

typedef struct MOVIentry {
	uint64_t     pos;
	int64_t      dts;
	unsigned int size;
	unsigned int samples_in_chunk;
	unsigned int chunkNum;              ///< Chunk number if the current entry is a chunk start otherwise 0
	unsigned int entries;
	int          cts;
#define MOV_SYNC_SAMPLE         0x0001
#define MOV_PARTIAL_SYNC_SAMPLE 0x0002
#define MOV_DISPOSABLE_SAMPLE   0x0004
	uint32_t     flags;
} MOVIentry;

typedef struct HintSample {
	uint8_t *data;
	int size;
	int sample_number;
	int offset;
	int own_data;
} HintSample;

typedef struct HintSampleQueue {
	int size;
	int len;
	HintSample *samples;
} HintSampleQueue;

typedef struct MOVFragmentInfo {
	int64_t offset;
	int64_t time;
	int64_t duration;
	int64_t tfrf_offset;
	int size;
} MOVFragmentInfo;

typedef struct MOVTrack {
	int         mode;
	int         entry;
	unsigned    timescale;
	uint64_t    time;
	int64_t     track_duration;
	int         last_sample_is_subtitle_end;
	long        sample_count;
	long        sample_size;
	long        chunkCount;
	int         has_keyframes;
	int         has_disposable;
#define MOV_TRACK_CTTS         0x0001
#define MOV_TRACK_STPS         0x0002
#define MOV_TRACK_ENABLED      0x0004
	uint32_t    flags;
#define MOV_TIMECODE_FLAG_DROPFRAME     0x0001
#define MOV_TIMECODE_FLAG_24HOURSMAX    0x0002
#define MOV_TIMECODE_FLAG_ALLOWNEGATIVE 0x0004
	uint32_t    timecode_flags;
	int         language;
	int         track_id;
	int         tag; ///< stsd fourcc
	AVStream        *st;
	AVCodecParameters *par;
	int multichannel_as_mono;

	int         vos_len;
	uint8_t     *vos_data;
	MOVIentry   *cluster;
	unsigned    cluster_capacity;
	int         audio_vbr;
	int         height; ///< active picture (w/o VBI) height for D-10/IMX
	uint32_t    tref_tag;
	int         tref_id; ///< trackID of the referenced track
	int64_t     start_dts;
	int64_t     start_cts;
	int64_t     end_pts;
	int         end_reliable;
	int64_t     dts_shift;

	int         hint_track;   ///< the track that hints this track, -1 if no hint track is set
	int         src_track;    ///< the track that this hint (or tmcd) track describes
	AVFormatContext *rtp_ctx; ///< the format context for the hinting rtp muxer
	uint32_t    prev_rtp_ts;
	int64_t     cur_rtp_ts_unwrapped;
	uint32_t    max_packet_size;

	int64_t     default_duration;
	uint32_t    default_sample_flags;
	uint32_t    default_size;

	HintSampleQueue sample_queue;

	AVIOContext *mdat_buf;
	int64_t     data_offset;
	int64_t     frag_start;
	int         frag_discont;
	int         entries_flushed;

	int         nb_frag_info;
	MOVFragmentInfo *frag_info;
	unsigned    frag_info_capacity;

	struct {
		int     first_packet_seq;
		int     first_packet_entry;
		int     first_packet_seen;
		int     first_frag_written;
		int     packet_seq;
		int     packet_entry;
		int     slices;
	} vc1_info;

	void       *eac3_priv;

	MOVMuxCencContext cenc;

	uint32_t palette[AVPALETTE_COUNT];
	int pal_done;

	int is_unaligned_qt_rgb;
} MOVTrack;

typedef enum {
	MOV_ENC_NONE = 0,
	MOV_ENC_CENC_AES_CTR,
} MOVEncryptionScheme;

typedef enum {
	MOV_PRFT_NONE = 0,
	MOV_PRFT_SRC_WALLCLOCK,
	MOV_PRFT_SRC_PTS,
	MOV_PRFT_NB
} MOVPrftBox;

typedef struct MOVMuxContext {
	const AVClass *av_class;
	int     mode;
	int64_t time;
	int     nb_streams;
	int     nb_meta_tmcd;  ///< number of new created tmcd track based on metadata (aka not data copy)
	int     chapter_track; ///< qt chapter track number
	int64_t mdat_pos;
	uint64_t mdat_size;
	MOVTrack *tracks;

	int flags;
	int rtp_flags;

	int iods_skip;
	int iods_video_profile;
	int iods_audio_profile;

	int moov_written;
	int fragments;
	int max_fragment_duration;
	int min_fragment_duration;
	int max_fragment_size;
	int ism_lookahead;
	AVIOContext *mdat_buf;
	int first_trun;

	int video_track_timescale;

	int reserved_moov_size; ///< 0 for disabled, -1 for automatic, size otherwise
	int64_t reserved_header_pos;

	char *major_brand;

	int per_stream_grouping;
	AVFormatContext *fc;

	int use_editlist;
	float gamma;

	int frag_interleave;
	int missing_duration_warned;

	char *encryption_scheme_str;
	MOVEncryptionScheme encryption_scheme;
	uint8_t *encryption_key;
	int encryption_key_len;
	uint8_t *encryption_kid;
	int encryption_kid_len;

	int need_rewrite_extradata;

	int use_stream_ids_as_track_ids;
	int track_ids_ok;
	int write_tmcd;
} MOVMuxContext;


// end exp


static inline void init_size(media_remux_job_t job, const char *in_filename)
{
#ifdef _MSC_VER
	struct _stat64 st = {0};
	_stat64(in_filename, &st);
#else
	struct stat st = {0};
	stat(in_filename, &st);
#endif
	job->in_size = st.st_size;
}

static inline bool init_input(media_remux_job_t job, const char *in_filename)
{
	int ret = avformat_open_input(&job->ifmt_ctx, in_filename, NULL, NULL);
	if (ret < 0) {
		blog(LOG_ERROR, "media_remux: Could not open input file '%s'",
		     in_filename);
		return false;
	}

	ret = avformat_find_stream_info(job->ifmt_ctx, NULL);
	if (ret < 0) {
		blog(LOG_ERROR, "media_remux: Failed to retrieve input stream"
				" information");
		return false;
	}

#ifndef NDEBUG
	av_dump_format(job->ifmt_ctx, 0, in_filename, false);
#endif
	return true;
}

static inline bool init_output(media_remux_job_t job, const char *out_filename)
{
	int ret;

	avformat_alloc_output_context2(&job->ofmt_ctx, NULL, NULL,
				       out_filename);
	if (!job->ofmt_ctx) {
		blog(LOG_ERROR, "media_remux: Could not create output context");
		return false;
	}

	for (unsigned i = 0; i < job->ifmt_ctx->nb_streams; i++) {
		AVStream *in_stream = job->ifmt_ctx->streams[i];
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
		AVStream *out_stream = avformat_new_stream(job->ofmt_ctx, NULL);
#else
		AVStream *out_stream = avformat_new_stream(
			job->ofmt_ctx, in_stream->codec->codec);
#endif
		if (!out_stream) {
			blog(LOG_ERROR, "media_remux: Failed to allocate output"
					" stream");
			return false;
		}

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
		ret = avcodec_parameters_copy(out_stream->codecpar,
					      in_stream->codecpar);
#else
		ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
#endif

		if (ret < 0) {
			blog(LOG_ERROR,
			     "media_remux: Failed to copy parameters");
			return false;
		}

		av_dict_copy(&out_stream->metadata, in_stream->metadata, 0);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
		out_stream->codecpar->codec_tag = 0;
#else
		out_stream->codec->codec_tag = 0;
		out_stream->time_base = out_stream->codec->time_base;
		if (job->ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_H;
#endif
	}

#ifndef NDEBUG
	av_dump_format(job->ofmt_ctx, 0, out_filename, true);
#endif

	if (!(job->ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&job->ofmt_ctx->pb, out_filename,
				AVIO_FLAG_WRITE);
		if (ret < 0) {
			blog(LOG_ERROR,
			     "media_remux: Failed to open output"
			     " file '%s'",
			     out_filename);
			return false;
		}
	}

	return true;
}

bool media_remux_job_create(media_remux_job_t *job, const char *in_filename,
			    const char *out_filename)
{
	if (!job)
		return false;

	*job = NULL;
	if (!os_file_exists(in_filename))
		return false;

	if (strcmp(in_filename, out_filename) == 0)
		return false;

	*job = (media_remux_job_t)bzalloc(sizeof(struct media_remux_job));
	if (!*job)
		return false;

	init_size(*job, in_filename);

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	av_register_all();
#endif

	if (!init_input(*job, in_filename))
		goto fail;

	if (!init_output(*job, out_filename))
		goto fail;

	return true;

fail:
	media_remux_job_destroy(*job);
	return false;
}

static inline void process_packet(AVPacket *pkt, AVStream *in_stream,
				  AVStream *out_stream)
{
	pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base,
				    out_stream->time_base,
				    AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
	pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base,
				    out_stream->time_base,
				    AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
	pkt->duration = (int)av_rescale_q(pkt->duration, in_stream->time_base,
					  out_stream->time_base);
	pkt->pos = -1;
}

static inline int process_packets(media_remux_job_t job,
				  media_remux_progress_callback callback,
				  void *data)
{
	AVPacket pkt;

	int ret, throttle = 0;
	for (;;) {
		ret = av_read_frame(job->ifmt_ctx, &pkt);
		if (ret < 0) {
			if (ret != AVERROR_EOF)
				blog(LOG_ERROR,
				     "media_remux: Error reading"
				     " packet: %s",
				     av_err2str(ret));
			break;
		}

		if (callback != NULL && throttle++ > 10) {
			float progress = pkt.pos / (float)job->in_size * 100.f;
			if (!callback(data, progress))
				break;
			throttle = 0;
		}

		process_packet(&pkt, job->ifmt_ctx->streams[pkt.stream_index],
			       job->ofmt_ctx->streams[pkt.stream_index]);

		ret = av_interleaved_write_frame(job->ofmt_ctx, &pkt);
		av_packet_unref(&pkt);

		if (ret < 0) {
			blog(LOG_ERROR, "media_remux: Error muxing packet: %s",
			     av_err2str(ret));

			/* Treat "Invalid data found when processing input" and
			 * "Invalid argument" as non-fatal */
			if (ret == AVERROR_INVALIDDATA || ret == -EINVAL)
				continue;

			break;
		}
	}

	return ret;
}

bool media_remux_job_process(media_remux_job_t job,
			     media_remux_progress_callback callback, void *data)
{
	int ret;
	bool success = false;

	if (!job)
		return success;

	ret = avformat_write_header(job->ofmt_ctx, NULL);
	if (ret < 0) {
		blog(LOG_ERROR, "media_remux: Error opening output file: %s",
		     av_err2str(ret));
		return success;
	}

	if (callback != NULL)
		callback(data, 0.f);

	ret = process_packets(job, callback, data);
	success = ret >= 0 || ret == AVERROR_EOF;

	ret = av_write_trailer(job->ofmt_ctx);
	if (ret < 0) {
		blog(LOG_ERROR, "media_remux: av_write_trailer: %s",
		     av_err2str(ret));
		success = false;
	}

	if (callback != NULL)
		callback(data, 100.f);

	return success;
}

void media_remux_job_destroy(media_remux_job_t job)
{
	if (!job)
		return;

	avformat_close_input(&job->ifmt_ctx);

	if (job->ofmt_ctx && !(job->ofmt_ctx->oformat->flags & AVFMT_NOFILE))
		avio_close(job->ofmt_ctx->pb);

	avformat_free_context(job->ofmt_ctx);

	bfree(job);
}
