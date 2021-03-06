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
*/
#ifndef __wf_private_h__
#define __wf_private_h__

#ifndef __GTK_H__
#ifdef USE_GDK_PIXBUF
#include "limits.h"
#include <gtk/gtk.h>
#else
#define GdkColor void
#endif
#endif
#include "agl/fbo.h"
#include "waveform/waveform.h"

#define WF_PEAK_BLOCK_SIZE (256 * 256) // the number of audio frames per block (64k)
#define WF_CACHE_BUF_SIZE (1 << 15)
#define WF_PEAK_STD_TO_LO 16
#define WF_MED_TO_V_LOW (16 * 16)
#define WF_PEAK_RATIO_LOW (WF_PEAK_RATIO * WF_PEAK_STD_TO_LO) // the number of samples per entry in a low res peakbuf.
#define WF_TEXTURE_VISIBLE_SIZE (WF_PEAK_TEXTURE_SIZE - 2 * TEX_BORDER)
#define WF_SAMPLES_PER_TEXTURE (WF_PEAK_RATIO * (WF_PEAK_TEXTURE_SIZE - 2 * TEX_BORDER))
#define WF_MAX_AUDIO_BLOCKS (ULLONG_MAX / WF_SAMPLES_PER_TEXTURE)
#define WF_MAX_BLOCK_RANGE 512 // to prevent performance and resource consumption issues caused by rendering too many blocks simultaneously.

#define WF_TEXTURE0 GL_TEXTURE1 //0 is not used
#define WF_TEXTURE1 GL_TEXTURE2
#define WF_TEXTURE2 GL_TEXTURE3
#define WF_TEXTURE3 GL_TEXTURE4

#define TEX_BORDER 2
#define TEX_BORDER_HI (TEX_BORDER * 16.0) // HI has a different border size in order to preserve block boundaries between changes in resolution.

#define TIERS_TO_RESOLUTION(T) (256 / (1 << T))
#define RESOLUTION_TO_TIERS(R) (8 - (int)floor(log2(R)))

typedef struct _texture_cache TextureCache;

enum
{
	WF_LEFT = 0,
	WF_RIGHT,
	WF_MAX_CH
};

struct _WfPeakBuf {
	int        size;             // the number of shorts allocated.
	short*     buf[WF_MAX_CH];   // holds the complete peakfile. The second pointer is only used for stereo files.
};

//a single hires peak block
struct _Peakbuf {
	int        block_num;
	int        size;             // the number of shorts allocated. 2 shorts per value (plus + minus)
	int        res;
	int        resolution;       // 1 corresponds to full resolution (though peakbufs never have resolution of 1 as then the audio data is accessed directly)
	void*      buf[WF_STEREO];
	int        maxlevel;         // mostly just for debugging
};

typedef enum
{
	MODE_V_LOW = 0,
	MODE_LOW,
	MODE_MED,
	MODE_HI,
	MODE_V_HI,
	N_MODES
} Mode;

typedef struct                            // base type for Modes to inherit from.
{
	int             n_blocks;
} WaveformModeRender;

typedef enum {
	WAVEFORM_LOADING = 1 << 0,
} WaveformState;

struct _WfAudioData {
	int                n_blocks;          // the size of the buf array
	WfBuf16**          buf16;             // pointers to arrays of blocks, one per block.
	int                n_tiers_present;
};

struct _WaveformPriv
{
	WfPeakBuf       peak;           // single buffer of peakdata for use at MED and LOW resolution.
	GPtrArray*      hires_peaks;    // array of Peakbuf* for use at HI resolution.
	RmsBuf*         rms_buf0;
	RmsBuf*         rms_buf1;

	WfAudioData     audio;          // tiered hi-res audio data.

	int             num_peaks;      // peak_buflen / PEAK_VALUES_PER_SAMPLE
	int             n_blocks;
	short           max_db;         // TODO should be in db?

	                                // render_data is owned, managed, and shared by all the WfActor's using this waveform.
	WaveformModeRender* render_data[N_MODES];

	gboolean        checks_done;    // if audio file is accessed, the peakfile is validated.
	WaveformState   state;
};

struct _WfWorker {
    GAsyncQueue*  msg_queue;
    GList*        jobs;
};

struct _wf
{
	const char*     domain;
	int             peak_mem_size;
	GHashTable*     peak_cache;
	PeakLoader      load_peak;

	struct
	{
		GHashTable* cache;
		int         mem_size;
		int         access_counter;
	} audio;

	WfWorker        audio_worker;
};

//TODO refactor based on _texture_hi (eg reverse order of indirection)
struct _wf_texture_list                   // WfGlBlock - used at MED and LOW resolutions in gl1 mode.
{
	int             size;
	struct {
		unsigned*   main;                 // array of texture id
		unsigned*   neg;                  // array of texture id - only used in shader mode.
	}               peak_texture[WF_MAX_CH];
	AGlFBO**        fbo;
#ifdef USE_FX
	AGlFBO**        fx_fbo;
#endif
#ifdef WF_SHOW_RMS
	unsigned*       rms_texture;
#endif
};

/*
 *  Textures
 *
 *  for high-res textures, a hashtable is used to map the blocknum to the WfTextureHi object
 *  for med and lo textures, the blocknum is stored as part of the Texture object
 *
 *  the texture cache is not currently used for hi-res textures though this was the intention
 *  -a per actor list of textures is maintained. this gives quick access to an actors textures, but doesnt provide a list of ALL textures as no where is there a list of actors.
 *     ** if this per-actor list is kept, we would need to add a global actor list.
 *     alternatively, if we add a global hi-res texture cache, .....
 *
 *  ** TODO lookup how to have a hash table that uses WaveformBlock as key (both pointer and int)
 */
struct _t
{
	unsigned        main;                 // texture id
	unsigned        neg;
};

struct _texture_hi                        // WfTextureHi
{
	struct _t t[WF_MAX_CH];
};

struct _textures_hi                       // WfTexturesHi
{
	GHashTable*     textures;             // maps: blocknum => WfTextureHi.
};

typedef struct _buf_stereo
{
	float*          buf[WF_STEREO];
	guint           size;                 // number of floats, NOT bytes
} WfBuf;

typedef struct _waveform_block
{
	Waveform*   waveform;
	int         block;
} WaveformBlock;

typedef struct _texture
{
	guint         id;
	WaveformBlock wb;
	int           time_stamp;
} Texture;

typedef struct _peak_sample
{
	short positive;
	short negative;
} WfPeakSample;

typedef struct _wf_drect { double x1, y1, x2, y2; } WfDRect;

#ifdef __wf_utils_c__
char wf_bold     [16] = "\x1b[1;39m";
char wf_white    [16] = "\x1b[0;39m";
#else
extern char wf_bold  [16];
extern char wf_white [16];
#endif

WF*            wf_get_instance             ();
uint32_t       wf_peakbuf_get_max_size     (int n_tiers);

short*         waveform_peak_malloc        (Waveform*, uint32_t bytes);
Peakbuf*       waveform_get_peakbuf_n      (Waveform*, int);
void           waveform_peakbuf_assign     (Waveform*, int block_num, Peakbuf*);
void           waveform_peakbuf_regen      (Waveform*, WfBuf16*, Peakbuf*, int block_num, int min_output_resolution);
void           waveform_peakbuf_free       (Peakbuf*);
int            waveform_get_n_audio_blocks (Waveform*);
void           waveform_print_blocks       (Waveform*);

void           waveform_peak_to_alphabuf   (Waveform*, AlphaBuf*, int scale, int* start, int* end, GdkColor*);
void           waveform_peak_to_alphabuf_hi(Waveform*, AlphaBuf*, int block, WfSampleRegion, GdkColor*);
void           waveform_rms_to_alphabuf    (Waveform*, AlphaBuf*, int* start, int* end, double samples_per_px, GdkColor* colour, uint32_t colour_bg);

void           waveform_free_render_data   (Waveform*);

void           waveform_audio_free         (Waveform*);
gboolean       waveform_load_audio_block   (Waveform*, WfBuf16*, int block_num);

WfTextureHi*   waveform_texture_hi_new     ();
void           waveform_texture_hi_free    (WfTextureHi*);

WaveformActor* wf_actor_new                (Waveform*, WaveformCanvas*);

float          wf_canvas_gl_to_px          (WaveformCanvas*, float x);

#endif //__wf_private_h__
