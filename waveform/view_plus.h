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

  ---------------------------------------------------------------

  WaveformViewPlus is a Gtk widget based on GtkDrawingArea.
  It displays an audio waveform derived from a peak file.

*/
#ifndef __waveform_view_plus_h__
#define __waveform_view_plus_h__

#include <glib.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include "waveform/waveform.h"
#include "waveform/actors/background.h"
#include "waveform/actors/grid.h"
#include "waveform/actors/spp.h"
#include "waveform/actors/text.h"
#include "waveform/view.h"

G_BEGIN_DECLS

#define TYPE_WAVEFORM_VIEW_PLUS (waveform_view_plus_get_type ())
#define WAVEFORM_VIEW_PLUS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_WAVEFORM_VIEW_PLUS, WaveformViewPlus))
#define WAVEFORM_VIEW_PLUS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_WAVEFORM_VIEW_PLUS, WaveformViewPlusClass))
#define IS_WAVEFORM_VIEW_PLUS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_WAVEFORM_VIEW_PLUS))
#define IS_WAVEFORM_VIEW_PLUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_WAVEFORM_VIEW_PLUS))
#define WAVEFORM_VIEW_PLUS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_WAVEFORM_VIEW_PLUS, WaveformViewPlusClass))

typedef struct _WaveformViewPlusClass WaveformViewPlusClass;
typedef struct _WaveformViewPlusPrivate WaveformViewPlusPrivate;

struct _WaveformViewPlus {
	GtkDrawingArea           parent_instance;

	Waveform*                waveform;
	float                    zoom;
	uint64_t                 start_frame;

	uint32_t                 fg_colour;
	uint32_t                 bg_colour;

	WaveformViewPlusPrivate* priv;
};

struct _WaveformViewPlusClass {
	GtkDrawingAreaClass parent_class;
};


GType             waveform_view_plus_get_type      () G_GNUC_CONST;
void              waveform_view_plus_set_gl        (GdkGLContext*);

WaveformViewPlus* waveform_view_plus_new           (Waveform*);
void              waveform_view_plus_load_file     (WaveformViewPlus*, const char*, WfCallback2, gpointer); // be careful, it force loads, even if already loaded.
void              waveform_view_plus_set_waveform  (WaveformViewPlus*, Waveform*);
void              waveform_view_plus_set_zoom      (WaveformViewPlus*, float);
void              waveform_view_plus_set_start     (WaveformViewPlus*, int64_t);
void              waveform_view_plus_set_region    (WaveformViewPlus*, int64_t, int64_t);
void              waveform_view_plus_set_colour    (WaveformViewPlus*, uint32_t fg, uint32_t bg);
void              waveform_view_plus_set_show_rms  (WaveformViewPlus*, gboolean);
AGlActor*         waveform_view_plus_add_layer     (WaveformViewPlus*, AGlActor*, int z);
AGlActor*         waveform_view_plus_get_layer     (WaveformViewPlus*, int);
void              waveform_view_plus_remove_layer  (WaveformViewPlus*, AGlActor*);

WaveformCanvas*   waveform_view_plus_get_canvas    (WaveformViewPlus*);
WaveformActor*    waveform_view_plus_get_actor     (WaveformViewPlus*);


G_END_DECLS

#endif //__waveform_view_plus_h__
