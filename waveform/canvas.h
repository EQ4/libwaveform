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
#ifndef __waveform_context_h__
#define __waveform_context_h__
#include "config.h"
#include <glib.h>
#include <glib-object.h>
#ifdef USE_SDL
#  include "SDL2/SDL.h"
#endif
#include "waveform/typedefs.h"
#include "waveform/shader.h"
#include "waveform/utils.h"

#define USE_CANVAS_SCALING 1

#define TYPE_WAVEFORM_CONTEXT (waveform_context_get_type ())
#define WAVEFORM_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_WAVEFORM_CONTEXT, WaveformContext))
#define WAVEFORM_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_WAVEFORM_CONTEXT, WaveformContextClass))
#define IS_WAVEFORM_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_WAVEFORM_CONTEXT))
#define IS_WAVEFORM_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_WAVEFORM_CONTEXT))
#define WAVEFORM_CONTEXT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_WAVEFORM_CONTEXT, WaveformContextClass))

typedef struct _WfContextPriv WfContextPriv;

struct _WaveformContext {
	GObject        parent_instance;

	gboolean       show_rms;
	gboolean       use_1d_textures;
	gboolean       enable_animations;
	gboolean       blend;              // true by default - set to false to increase performance if using without background (doesnt make much difference). Flag currently not honoured in all cases.

	AGlScene*      root;               // optional

	uint32_t       sample_rate;
	float          samples_per_pixel;  // application can specify the base sppx. Is multiplied by zoom to get the actual sppx.
	float          zoom;
	float          rotation;
	float          v_gain;

	struct {
		RulerShader*    ruler;
	}              shaders;

	WfContextPriv* priv;
	int           _draw_depth;
	AGlTextureUnit* texture_unit[4];
};

#ifdef __wf_canvas_priv__
struct _WfContextPriv {
	bool           scaled;   // scaled mode uses the WfContext time scale. Non-scaled mode uses only the actor rect and sample-region.
	WfAnimatable   zoom;     // (float) samples_per_pixel
#ifdef USE_FRAME_CLOCK
	guint64       _last_redraw_time;
#endif
	guint         _queued;
	guint         pending_init;
};
#endif

struct _WaveformContextClass {
	GObjectClass parent_class;
};

struct _WfViewPort { double left, top, right, bottom; };

WaveformContext* wf_canvas_new                        (AGlRootActor*);
#ifdef USE_SDL
WaveformContext* wf_canvas_new_sdl                    (SDL_GLContext*);
#endif
void             wf_canvas_free                       (WaveformContext*);
void             wf_canvas_set_viewport               (WaveformContext*, WfViewPort*);
void             wf_canvas_set_share_list             (WaveformContext*);
void             wf_canvas_set_rotation               (WaveformContext*, float);
#ifdef USE_CANVAS_SCALING
void             wf_canvas_set_zoom                   (WaveformContext*, float);
#endif
void             wf_canvas_set_gain                   (WaveformContext*, float);
WaveformActor*   wf_canvas_add_new_actor              (WaveformContext*, Waveform*);
void             wf_canvas_remove_actor               (WaveformContext*, WaveformActor*);
void             wf_canvas_queue_redraw               (WaveformContext*);
void             wf_canvas_load_texture_from_alphabuf (WaveformContext*, int texture_id, AlphaBuf*);

#define wf_canvas_free0(A) (wf_canvas_free(A), A = NULL)

#endif //__waveform_context_h__
