/*
  copyright (C) 2012-2015 Tim Orford <tim@orford.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 3
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

  ---------------------------------------------------------

  peakgen
  -------

  description:
  - generates a peakfile from an audio file.
  - output is 16bit, alternating positive and negative peaks
  - output has riff header so we know what type of peak file it is. Could possibly revert to headerless file.
  - peak files are cached in ~/.cache/

  todo:
  - what is maximum file size?

*/
#define __wf_private__
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <sndfile.h>
#include "waveform/waveform.h"
#include "waveform/audio.h"
#include "waveform/audio_file.h"
#include "waveform/worker.h"
#include "waveform/loaders/ardour.h"
#include "waveform/peakgen.h"

#define BUFFER_LEN 256 // length of the buffer to hold audio during processing. currently must be same as WF_PEAK_RATIO
#define MAX_CHANNELS 2

#define DEFAULT_USER_CACHE_DIR ".cache/peak"

static int           peak_mem_size = 0;
static bool          need_file_cache_check = true;

static inline void   process_data        (short* data, int count, int channels, short max[], short min[]);
static unsigned long sample2time         (SF_INFO, long samplenum);
static bool          wf_file_is_newer    (const char*, const char*);
static bool          wf_create_cache_dir ();
static char*         get_cache_dir       ();
static void          maintain_file_cache ();

static WfWorker peakgen = {0,};


static char*
waveform_get_peak_filename(const char* filename)
{
	// filename should be absolute.
	// caller must g_free the returned value.

	if(wf->load_peak == wf_load_ardour_peak){
		gwarn("cannot automatically determine path of Ardour peakfile");
		return NULL;
	}

	GError* error = NULL;
	gchar* uri = g_filename_to_uri(filename, NULL, &error);
	if(error){
		gwarn("%s", error->message);
		return NULL;
	}
	dbg(1, "uri=%s", uri);

	gchar* md5 = g_compute_checksum_for_string(G_CHECKSUM_MD5, uri, -1);
	g_free(uri);
	gchar* peak_basename = g_strdup_printf("%s.peak", md5);
	g_free(md5);
	char* cache_dir = get_cache_dir();
	gchar* peak_filename = g_build_filename(cache_dir, peak_basename, NULL);
	g_free(cache_dir);
	dbg(1, "peak_filename=%s", peak_filename);
	g_free(peak_basename);

	return peak_filename;
}


static bool
peakfile_is_current(const char* audio_file, const char* peak_file)
{
	/*
	note that this test will fail to detect a modified file, if an older file is now stored at this location.

	The freedesktop thumbnailer spec identifies modifications by comparing with both url and mtime stored in the thumbnail.
	This will mostly work, but strictly speaking still won't identify a changed file in all cases.

	One relatively simple improvement would be to match the sizes.
	*/

	if(!g_file_test(peak_file, G_FILE_TEST_EXISTS)){
		dbg(1, "peakfile does not exist: %s", peak_file);
		return false;
	}

	if(wf_file_is_newer(audio_file, peak_file)){
		dbg(1, "peakfile is too old");
		return false;
	}

	return true;
}


/*
 *  Caller must g_free the returned filename.
 */
void
waveform_ensure_peakfile (Waveform* w, WfPeakfileCallback callback, gpointer user_data)
{
	if(!wf_create_cache_dir()) return;

	char* filename = g_path_is_absolute(w->filename) ? g_strdup(w->filename) : g_build_filename(g_get_current_dir(), w->filename, NULL);

	gchar* peak_filename = waveform_get_peak_filename(filename);
	if(!peak_filename){
		callback(w, NULL, user_data);
		goto out;
	}

	if(w->offline || peakfile_is_current(filename, peak_filename)){
		callback(w, peak_filename, user_data);
		goto out;
	}

	typedef struct {
		Waveform* waveform;
		WfPeakfileCallback callback;
		char* filename;
		gpointer user_data;
	} C;
	C* c = g_new0(C, 1);
	*c = (C){
		.waveform = g_object_ref(w),
		.callback = callback,
		.filename = peak_filename,
		.user_data = user_data
	};

	void waveform_ensure_peakfile_done(Waveform* w, gpointer user_data)
	{
		C* c = (C*)user_data;
		if(c->callback) c->callback(c->waveform, c->filename, c->user_data);
		else g_free(c->filename);
		g_object_unref(c->waveform);
		g_free(c);
	}

	waveform_peakgen(w, peak_filename, waveform_ensure_peakfile_done, c);

  out:
	g_free(filename);
}


char*
waveform_ensure_peakfile__sync (Waveform* w)
{
	if(!wf_create_cache_dir()) return NULL;

	char* filename = g_path_is_absolute(w->filename) ? g_strdup(w->filename) : g_build_filename(g_get_current_dir(), w->filename, NULL);

	gchar* peak_filename = waveform_get_peak_filename(filename);
	if(!peak_filename) goto out;

	if(g_file_test(peak_filename, G_FILE_TEST_EXISTS)){ 
		dbg (1, "peak file exists. (%s)", peak_filename);

		/*
		note that this test will fail to detect a modified file, if an older file is now stored at this location.

		The freedesktop thumbnailer spec identifies modifications by comparing with both url and mtime stored in the thumbnail.
		This will mostly work, but strictly speaking still won't identify a changed file in all cases.
		*/
		if(w->offline || wf_file_is_newer(peak_filename, filename)) return peak_filename;

		dbg(1, "peakfile is too old");
	}

	if(!wf_peakgen__sync(filename, peak_filename)){ g_free0(peak_filename); goto out; }

  out:
	g_free(filename);

	return peak_filename;
}


#ifdef USE_FFMPEG
bool
wf_ff_peakgen(const char* infilename, const char* peak_filename)
{
	FF f = {0,};

	if(!wf_ff_open(&f, infilename)) return false;

	SNDFILE* outfile;
	SF_INFO sfinfo = {
		.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16,
		.channels = f.info.channels,
		.samplerate = f.info.sample_rate,
	};

	gchar* basename = g_path_get_basename(peak_filename);
	gchar* tmp_path = g_build_filename("/tmp", basename, NULL);
	g_free(basename);

	if(!(outfile = sf_open(tmp_path, SFM_WRITE, &sfinfo))){
		printf ("Not able to open output file %s.\n", tmp_path);
		puts(sf_strerror(NULL));
		return false;
	}

	int total_frames_written = 0;
	short total_max[sfinfo.channels]; memset(total_max, 0, sizeof(short) * sfinfo.channels);
	short total_min[sfinfo.channels]; memset(total_min, 0, sizeof(short) * sfinfo.channels);

	int read_len = 256 * f.info.channels;
	float* sf_data = g_malloc(sizeof(float) * read_len);
	float max[sfinfo.channels];
	float min[sfinfo.channels];

	int readcount;
	do {
		readcount = wf_ff_read(&f, sf_data, read_len);

		memset(max, 0, sizeof(float) * sfinfo.channels);
		memset(min, 0, sizeof(float) * sfinfo.channels);
		short w[sfinfo.channels][2];

		int k; for (k = 0; k < readcount; k+=sfinfo.channels){
			int c; for(c=0;c<sfinfo.channels;c++){
				const float temp = sf_data[k + c];
				max[c] = MAX(max[c], temp);
				min[c] = MIN(min[c], temp);
			}
		};
		int c; for(c=0;c<sfinfo.channels;c++){
			w[c][0] = max[c] * (1 << 14);
			w[c][1] = min[c] * (1 << 14);
			total_max[c] = MAX(total_max[c], w[c][0]);
			total_min[c] = MIN(total_min[c], w[c][1]);
		}
		total_frames_written += sf_write_short (outfile, (short*)w, WF_PEAK_VALUES_PER_SAMPLE * sfinfo.channels);

	} while (readcount > 0);

#if 0
	if(sfinfo.channels > 1) dbg(0, "max=%i,%i min=%i,%i", total_max[0], total_max[1], total_min[0], total_min[1]);
	else dbg(0, "max=%i min=%i", total_max[0], total_min[0]);
#endif

	wf_ff_close(&f);
	sf_close (outfile);
	g_free(sf_data);

	int renamed = !rename(tmp_path, peak_filename);
	g_free(tmp_path);
	if(!renamed) return false;

	return true;
}
#endif


#define FAIL_ \
	g_free(tmp_path); \
	sf_close (infile); \
	return false;


void
waveform_peakgen(Waveform* w, const char* peak_filename, WfCallback2 callback, gpointer user_data)
{
	if(!peakgen.msg_queue) wf_worker_init(&peakgen);

	typedef struct {
		char*         infilename;
		const char*   peak_filename;
		struct {
			bool          failed; // returned true if peakgen failed
		}             out;
		WfCallback2   callback;
		void*         user_data;
	} PeakJob;

	PeakJob* job = g_new0(PeakJob, 1);
	*job = (PeakJob){
		.infilename = g_path_is_absolute(w->filename) ? g_strdup(w->filename) : g_build_filename(g_get_current_dir(), w->filename, NULL),
		.peak_filename = peak_filename,
		.callback = callback,
		.user_data = user_data,
	};

	void peakgen_execute_job(Waveform* w, gpointer _job)
	{
		// runs in worker thread
		PeakJob* job = _job;

		if(!wf_peakgen__sync(job->infilename, job->peak_filename)){
#ifdef DEBUG
			if(wf_debug) gwarn("peakgen failed");
#endif
			job->out.failed = true; // writing to object owned by main thread
		}
	}

	void peakgen_free(gpointer item)
	{
		PeakJob* job = item;
		g_free0(job->infilename);
		g_free(job);
	}

	void peakgen_post(Waveform* waveform, gpointer item)
	{
		// runs in the main thread
		// will not be called if the waveform is destroyed.

		PeakJob* job = item;
// TODO if job->out.failed, still need to notify original caller...
		job->callback(waveform, job->user_data);
	}

	wf_worker_push_job(&peakgen, w, peakgen_execute_job, peakgen_post, peakgen_free, job);
}


void
waveform_peakgen_cancel(Waveform* w)
{
	wf_worker_cancel_jobs(&peakgen, w);
}


/*
 *  Generate a peak file on disk for the given audio file.
 *  Returns true on success.
 */
bool
wf_peakgen__sync(const char* infilename, const char* peak_filename)
{
	g_return_val_if_fail(infilename, false);
	PF;

	SNDFILE *infile, *outfile;

	SF_INFO sfinfo;
	if(!(infile = sf_open(infilename, SFM_READ, &sfinfo))){
		if(!g_file_test(infilename, G_FILE_TEST_EXISTS)){
			if(wf_debug) printf("peakgen: no such input file: '%s'\n", infilename);
			return false;
		}else{
#ifdef USE_FFMPEG
			if(wf_ff_peakgen(infilename, peak_filename)){
				return true;
			}
#endif
			if(wf_debug) printf("peakgen: not able to open input file %s: %s\n", infilename, sf_strerror(NULL));
			return false;
		}
	}
	dbg(1, "n_frames=%Lu %i", sfinfo.frames, ((int)sfinfo.frames/256));

	gchar* basename = g_path_get_basename(peak_filename);
	gchar* tmp_path = g_build_filename("/tmp", basename, NULL);
	g_free(basename);

	if (sfinfo.channels > MAX_CHANNELS){
		printf ("Not able to process more than %d channels\n", MAX_CHANNELS);
		return false;
	};
	if(wf_debug) printf("samplerate=%i channels=%i frames=%i\n", sfinfo.samplerate, sfinfo.channels, (int)sfinfo.frames);
	if(sfinfo.channels > 16){ printf("format not supported. unexpected number of channels: %i\n", sfinfo.channels); FAIL_; }

	float* buf_f = NULL;
	if((sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT){
		dbg(1, "32 bit float");
		buf_f = g_malloc0(sizeof(float) * BUFFER_LEN * sfinfo.channels);
	}
	gboolean is_float = ((sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT);

	short* data = g_malloc0((is_float ? sizeof(float) : sizeof(short)) * BUFFER_LEN * sfinfo.channels);

	//copy the sfinfo for the output file:
	SF_INFO sfinfo_w = sfinfo;
	sfinfo_w.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	int bytes_per_frame = sfinfo.channels * sizeof(short);

	if(!(outfile = sf_open(tmp_path, SFM_WRITE, &sfinfo_w))){
		printf ("Not able to open output file %s: %s.\n", tmp_path, sf_strerror(NULL));
		FAIL_;
	}

	#define EIGHT_HOURS (60 * 60 * 8)
	#define MAX_READ_ITER (44100 * EIGHT_HOURS / BUFFER_LEN)

	short total_max[sfinfo.channels];
	int readcount, i = 0;
	long long samples_read = 0;
	gint32 total_bytes_written = 0;
	gint32 total_frames_written = 0;
	short* read_buf = is_float ? (short*)buf_f : data;
	typedef sf_count_t (*read_fn)(SNDFILE*, short*, sf_count_t);
	read_fn r = is_float ? (read_fn)sf_readf_float : sf_readf_short;
	while((readcount = r(infile, read_buf, BUFFER_LEN))){
		if(wf_debug && (readcount < BUFFER_LEN)){
			dbg(1, "EOF i=%i readcount=%i total_frames_written=%i", i, readcount, total_frames_written);
		}
		if(is_float){
			int i; for(i=0;i<readcount;i++){
				float* b = (float*)read_buf;
				data[i] = b[i] * (1 << 14);
			}
		}

		short max[sfinfo.channels];
		short min[sfinfo.channels];
		memset(max, 0, sizeof(short) * sfinfo.channels);
		memset(min, 0, sizeof(short) * sfinfo.channels);
		process_data(data, readcount, sfinfo.channels, max, min);
		samples_read += readcount;
		short w[sfinfo.channels][2];
		int c; for(c=0;c<sfinfo.channels;c++){
			w[c][0] = max[c];
			w[c][1] = min[c];
			total_max[c] = MAX(total_max[c], max[c]);
		}
		total_frames_written += sf_write_short (outfile, (short*)w, WF_PEAK_VALUES_PER_SAMPLE * sfinfo.channels);
		total_bytes_written += sizeof(short);
#if 0
		if(sfinfo.channels == 2){
			short* z = w;
			if(i<10) printf("  %i %i %i %i\n", z[0], z[1], z[2], z[3]);
		}else{
			if(i<10) printf("  %i %i\n", max[0], min[0]);
		}
#endif
		if(++i > MAX_READ_ITER){ printf("warning: stopped before EOF.\n"); break; }
	}

	if(wf_debug){
		long secs = sample2time(sfinfo, samples_read) / 1000;
		long ms   = sample2time(sfinfo, samples_read) - 1000 * secs;
		long mins = secs / 60;
		secs = secs - mins * 60;
		dbg(0, "size: %'Li bytes %li:%02li:%03li. maxlevel=%i(%fdB)", samples_read * sizeof(short), mins, secs, ms, total_max[0], wf_int2db(total_max[0]));
	}

	dbg(1, "total_items_written=%i items_per_channel=%i peaks_per_channel=%i", total_frames_written, total_frames_written/sfinfo.channels, total_frames_written/(sfinfo.channels * WF_PEAK_VALUES_PER_SAMPLE));
	dbg(2, "total bytes written: %i (of%Li)", total_bytes_written, (long long)sfinfo.frames * bytes_per_frame);

	sf_close (outfile);
	sf_close (infile);
	g_free(data);
	if(buf_f) g_free(buf_f);

	if(need_file_cache_check){
		maintain_file_cache();
		need_file_cache_check = false;
	}

	int renamed = !rename(tmp_path, peak_filename);
	g_free(tmp_path);
	if(!renamed) return false;

	return true;
}


static inline void
process_data (short* data, int data_size_frames, int n_channels, short max[], short min[])
{
	//produce peak data from audio data.
	//
	//      data - interleaved audio.
	//      data_size_frames - the number of frames in data to process.
	//      pos  - is the *sample* count. (ie not frame count, and not byte count.)
	//      max  - (output) positive peaks. must be allocated with size n_channels
	//      min  - (output) negative peaks. must be allocated with size n_channels

	memset(max, 0, sizeof(short) * n_channels);
	memset(min, 0, sizeof(short) * n_channels);

	int k; for(k=0;k<data_size_frames;k+=n_channels){
		int c; for(c=0;c<n_channels;c++){
			max[c] = (data[k + c] > max[c]) ? data[k + c] : max[c];
			min[c] = (data[k + c] < min[c]) ? data[k + c] : min[c];
		}
	}
}


/*
 * Returns time in milliseconds, given the sample number.
 */
static unsigned long
sample2time(SF_INFO sfinfo, long samplenum)
{
	// long is good up to 4.2GB
	
	return (10 * samplenum) / ((sfinfo.samplerate/100) * sfinfo.channels);
}


static gboolean
wf_create_cache_dir()
{
	gchar* path = get_cache_dir();
	gboolean ret  = !g_mkdir_with_parents(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP);
	if(!ret) gwarn("cannot access cache dir: %s", path);
	g_free(path);
	return ret;
}


static void*
peakbuf_allocate(Peakbuf* peakbuf, int c)
{
	g_return_val_if_fail(c < WF_STEREO, NULL);
	if(peakbuf->buf[c]){ gwarn("buffer already allocated. c=%i", c); return NULL; }

	peakbuf->buf[c] = g_malloc0(sizeof(short) * peakbuf->size);
	peak_mem_size += (peakbuf->size * sizeof(short));

	dbg(2, "c=%i b=%i: size=%i tot_peak_mem=%ikB", c, peakbuf->block_num, peakbuf->size, peak_mem_size / 1024);
	return peakbuf->buf[c];
}


void
waveform_peakbuf_free(Peakbuf* p)
{
	int c; for(c=0;c<WF_STEREO;c++) if(p->buf[c]) g_free0(p->buf[c]);
	g_free(p);
}


static void
peakbuf_set_n_tiers(Peakbuf* peakbuf, int n_tiers, int resolution)
{
	if(n_tiers < 1 || n_tiers > MAX_TIERS){ gwarn("n_tiers out of range: %i", n_tiers); n_tiers = MAX_TIERS; }
	peakbuf->resolution = resolution;
	dbg(2, "n_tiers=%i", n_tiers);
}


/*
 *  Generate peak data for the given block number.
 *  The audio is supplied in @audiobuf and output in @peakbuf.
 *  @min_tiers specifies the _minimum_ resolution. The peakbuf produced may be of higher resolution than this.
 *  For thread-safety, the Waveform is not modified.
 */
void
waveform_peakbuf_regen(Waveform* waveform, WfBuf16* audiobuf, Peakbuf* peakbuf, int block_num, int min_output_tiers)
{
	// TODO caching: consider saving to disk, and using kernel caching. clear cache on exit.

	dbg(2, "%i", block_num);

	WF* wf = wf_get_instance();
	g_return_if_fail(audiobuf);
	g_return_if_fail(peakbuf);

	int input_resolution = TIERS_TO_RESOLUTION(MAX_TIERS);

	//decide the size of the peakbuf:
	//-------------------------------
	// -here we jump in peakbuf sizes of x16 - could change to using 4 stages of x4 if beneficial.
	int output_resolution = WF_PEAK_RATIO; // output_resolution goes from 1 meaning full resolution, to 256 for normal lo-res peakbuf
	int output_tiers      = 0;
	if(min_output_tiers >= 0){ output_resolution = WF_PEAK_RATIO >> 4; output_tiers = 4; } //=16
	if(min_output_tiers >  3){ output_resolution = 1;                  output_tiers = 8; } //use the whole file!
	int io_ratio = output_resolution / input_resolution; //number of input bytes for 1 byte of output
	dbg(2, "%i: min_output_tiers=%i input_resolution=%i output_resolution=%i io_ratio=%i", block_num, min_output_tiers, input_resolution, output_resolution, io_ratio);

	/*

	input_resolution:  the amount of audio data that is available. 1 corresponds to the full audio file. 256 would correspond to the cached peakfile.
	output_resolution: the resolution of the produced Peakbuf.
	io_ratio:          the number of input bytes for 1 byte of output
	peakbuf_size:      =blocksize*?

	the most common case is for input_resolution=1 and output_resolution=16

	n_tiers_available  input_resolution  output_resolution  io_ratio  output_n_tiers peakbuf_size
	-----------------  ----------------  -----------------  --------  -------------- ------------
	8                  1                   1                1
	7                  2                   2                1
	6                                      4
	5                                      8
	4                                     16
	3                                     32
	2                                     64
	1                  128               128                1
	0                                    256 (the lo-res case)

	the case we want is where the whole file is available, but peakbuf size is 1/16 size
	8                                      1                 1                     8          1MB
	8                                      4                 4                     6        256kB
	8                  1                  16                16                     4         64kB
	8                  1                  64                64                     2         16kB
	8                                    128               128                     1          8kB

	for lo-res peaks, one peak block contains 256 * 8 frames.                    =4kB
	for hi-res peaks, one peak block contains 256 * 8 * resolution frames ?      =8k --> 1MB

	buffer sizes in bytes:

		if WF_PEAK_BLOCK_SIZE == 256 * 32:
			10s file has 54 blocks
			full res audio:      16k  <-- this is too small, can increase.
			with io=16, peakbuf:  2k

		if WF_PEAK_BLOCK_SIZE == 256 * 256: (equiv to a regular med res peak file)
			10s file has ~7 blocks
			full res audio:      128k <-- seems reasonable but still not large.
			with io=16, peakbuf:  16k

	*/

	short* buf = peakbuf->buf[WF_LEFT];
	dbg(3, "peakbuf=%p buf0=%p", peakbuf, peakbuf->buf);
	if(!buf){
		//peakbuf->size = peakbuf_get_max_size_by_resolution(output_resolution);
		//peakbuf->size = wf_peakbuf_get_max_size(output_tiers);
		peakbuf->size = audiobuf->size * WF_PEAK_VALUES_PER_SAMPLE / io_ratio;
		dbg(2, "buf->size=%i blocksize=%i", peakbuf->size, WF_PEAK_BLOCK_SIZE * WF_PEAK_VALUES_PER_SAMPLE / io_ratio);
		int c; for(c=0;c<waveform_get_n_channels(waveform);c++){
			buf = peakbuf_allocate(peakbuf, c);
		}
	}

	short maxplus[2] = {0,0};      // highest positive audio value encountered (per block). One for each channel
	short maxmin [2] = {0,0};      // highest negative audio value encountered.
	short totplus = 0;
	short totmin  = 0;

	int n_chans = waveform_get_n_channels(waveform);
	int c; for(c=0;c<n_chans;c++){
		short* buf = peakbuf->buf[c];
		WfBuf16* audio_buf = audiobuf;
									g_return_if_fail(peakbuf->size >= WF_PEAK_BLOCK_SIZE * WF_PEAK_VALUES_PER_SAMPLE / io_ratio);
		audio_buf->stamp = ++wf->audio.access_counter;
		int i, p; for(i=0, p=0; p<WF_PEAK_BLOCK_SIZE; i++, p+= io_ratio){

			process_data(&audio_buf->buf[c][p], io_ratio, 1, (short*)&maxplus, (short*)&maxmin);

#if 0
			short* dd = &audio_buf->buf[c][p];
			if(i < 20) printf("    %i %i %i %i\n", audio_buf->buf[WF_LEFT][p], dd[0], maxplus[0], maxmin [0]);
#endif

			buf[2 * i    ] = maxplus[0];
			buf[2 * i + 1] = maxmin [0];

			totplus = MAX(totplus, maxplus[0]);
			totmin  = MIN(totmin,  maxmin [0]);
		}

		peakbuf->maxlevel = MAX(peakbuf->maxlevel, MAX(totplus, -totmin));

		/*
		for(i=0;i<10;i++){
			printf("      %i %i\n", buf[2 * i], buf[2 * i + 1]);
		}
		*/
	}
	dbg(2, "maxlevel=%i,%i (%.3fdB)", totplus, totmin, wf_int2db(MAX(totplus, -totmin)));

	peakbuf_set_n_tiers(peakbuf, output_tiers, output_resolution);
}


static bool
wf_file_is_newer(const char* file1, const char* file2)
{
	//return TRUE if file1 date is newer than file2 date.

	struct stat info1;
	struct stat info2;
	if(stat(file1, &info1)) return false;
	if(stat(file2, &info2)) return false;
	if(info1.st_mtime < info2.st_mtime) dbg(2, "%i %i %i", info1.st_mtime, info2.st_mtime, sizeof(time_t));
	return (info1.st_mtime > info2.st_mtime);
}


static char*
get_cache_dir()
{
	const gchar* env = g_getenv("XDG_CACHE_HOME");
	if(env) dbg(0, "cache_dir=%s", env);
	if(env) return g_strdup(env);

	gchar* dir_name = g_build_filename(g_get_home_dir(), DEFAULT_USER_CACHE_DIR, NULL);
	return dir_name;
}


static void
maintain_file_cache()
{
	// http://people.freedesktop.org/~vuntz/thumbnail-spec-cache/delete.html

	#define CACHE_EXPIRY_DAYS 30

	bool _maintain_file_cache()
	{
		char* dir_name = get_cache_dir();
		dbg(2, "dir=%s", dir_name);
		GError* error = NULL;
		GDir* d = g_dir_open(dir_name, 0, &error);

		struct timeval time;
		gettimeofday(&time, NULL);
		time_t now = time.tv_sec;

		int n_deleted = 0;
		struct stat info;
		const char* leaf;
		while ((leaf = g_dir_read_name(d))) {
			if (g_str_has_suffix(leaf, ".peak")) {
				gchar* filename = g_build_filename(dir_name, leaf, NULL);
				if(!stat(filename, &info)){
					time_t days_old = (now - info.st_mtime) / (60 * 60 * 24);
					//dbg(0, "%i days_old=%i", info.st_mtime, days_old);
					if(days_old > CACHE_EXPIRY_DAYS){
						dbg(2, "deleting: %s", filename);
						g_unlink(filename);
						n_deleted++;
					}
				}
				g_free(filename);
			}
		}
		dbg(1, "peak files deleted: %i", n_deleted);

		g_dir_close(d);
		g_free(dir_name);

		return G_SOURCE_REMOVE;
	}

	g_idle_add(_maintain_file_cache, NULL);
}


