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
  It displays an audio waveform represented by a Waveform object.
  It also displays decorative title text and information text.

  When the widget is focussed, the following keyboard shortcuts are active:
    left         scroll left
    right        scroll right
    -            zoom in
    +            zoom out

  For a demonstration of usage, see the file test/view_plus_test.c

*/
#define __wf_private__
#define __waveform_view_private__
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include "agl/utils.h"
#include "waveform/waveform.h"
#include "waveform/gl_utils.h"
#include "view_plus.h"

#define DIRECT 1
#define DEFAULT_HEIGHT 64
#define DEFAULT_WIDTH 256

static AGl*          agl = NULL;

static GdkGLConfig*  glconfig = NULL;
static GdkGLContext* gl_context = NULL;

#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

//-----------------------------------------

typedef void (KeyHandler)(WaveformViewPlus*);

typedef struct
{
	int         key;
	KeyHandler* handler;
} Key;

typedef struct
{
	guint          timer;
	KeyHandler*    handler;
} KeyHold;

static KeyHandler
	zoom_in,
	zoom_out,
	scroll_left,
	scroll_right;

static Key keys[] = {
	{KEY_Left,  scroll_left},
	{KEY_Right, scroll_right},
	{61,        zoom_in},
	{45,        zoom_out},
	{0},
};

//-----------------------------------------

struct _WaveformViewPlusPrivate {
	WaveformCanvas* canvas;
	WaveformActor*  actor;
	AGlActor*       root;

	AMPromise*      ready;
};

enum {
    PROMISE_DISP_READY,
    PROMISE_WAVE_READY,
    PROMISE_MAX
};
#define promise(A) ((AMPromise*)g_list_nth_data(view->priv->ready->children, A))

static int      waveform_view_plus_get_width            (WaveformViewPlus*);
static int      waveform_view_plus_get_height           (WaveformViewPlus*);

static gpointer waveform_view_plus_parent_class = NULL;

#define WAVEFORM_VIEW_PLUS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_WAVEFORM_VIEW_PLUS, WaveformViewPlusPrivate))
enum  {
	WAVEFORM_VIEW_PLUS_DUMMY_PROPERTY
};
static gboolean waveform_view_plus_on_expose            (GtkWidget*, GdkEventExpose*);
static gboolean waveform_view_plus_button_press_event   (GtkWidget*, GdkEventButton*);
static gboolean waveform_view_plus_button_release_event (GtkWidget*, GdkEventButton*);
static gboolean waveform_view_plus_motion_notify_event  (GtkWidget*, GdkEventMotion*);
static gboolean waveform_view_plus_focus_in_event       (GtkWidget*, GdkEventFocus*);
static gboolean waveform_view_plus_focus_out_event      (GtkWidget*, GdkEventFocus*);
static gboolean waveform_view_plus_enter_notify_event   (GtkWidget*, GdkEventCrossing*);
static gboolean waveform_view_plus_leave_notify_event   (GtkWidget*, GdkEventCrossing*);
static void     waveform_view_plus_realize              (GtkWidget*);
static void     waveform_view_plus_unrealize            (GtkWidget*);
static void     waveform_view_plus_allocate             (GtkWidget*, GdkRectangle*);
static void     waveform_view_plus_finalize             (GObject*);
static void     waveform_view_plus_set_projection       (GtkWidget*);
static bool     waveform_view_plus_init_drawable        (WaveformViewPlus*);
static void     waveform_view_plus_display_ready        (WaveformViewPlus*);

static void     waveform_view_plus_gl_on_allocate       (WaveformViewPlus*);
static void     waveform_view_plus_draw                 (WaveformViewPlus*);

static void     add_key_handlers                        (GtkWindow*, WaveformViewPlus*, Key keys[]);
static void     remove_key_handlers                     (GtkWindow*, WaveformViewPlus*);

static AGlActor* waveform_actor                         (WaveformViewPlus*);


static gboolean
__init ()
{
	dbg(2, "...");

	if(!gl_context){
		gtk_gl_init(NULL, NULL);
		if(wf_debug){
			gint major, minor;
			gdk_gl_query_version (&major, &minor);
			g_print ("GtkGLExt version %d.%d\n", major, minor);
		}
	}

	glconfig = gdk_gl_config_new_by_mode( GDK_GL_MODE_RGBA | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE );
	if (!glconfig) { gerr ("Cannot initialise gtkglext."); return false; }

	return true;
}


/*
 *  For use where the widget needs to share an opengl context with other items.
 *  Should be called before any widgets are instantiated.
 *
 *  Alternatively, just use the context returned by agl_get_gl_context() for other widgets.
 */
void
waveform_view_plus_set_gl (GdkGLContext* _gl_context)
{
	gl_context = _gl_context;
}


static WaveformViewPlus*
construct ()
{
	WaveformViewPlus* self = (WaveformViewPlus*) g_object_new (TYPE_WAVEFORM_VIEW_PLUS, NULL);
	gtk_widget_add_events ((GtkWidget*) self, (gint) ((GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK) | GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK));
	if(!gtk_widget_set_gl_capability((GtkWidget*)self, glconfig, gl_context ? gl_context : agl_get_gl_context(), DIRECT, GDK_GL_RGBA_TYPE)){
		gwarn("failed to set gl capability");
	}

	return self;
}


WaveformViewPlus*
waveform_view_plus_new (Waveform* waveform)
{
	PF;
	g_return_val_if_fail(glconfig || __init(), NULL);

	WaveformViewPlus* view = construct ();
	GtkWidget* widget = (GtkWidget*)view;
	WaveformViewPlusPrivate* v = view->priv;

	view->waveform = waveform ? g_object_ref(waveform) : NULL;

#ifdef HAVE_GTK_2_18
	gtk_widget_set_can_focus(widget, TRUE);
#endif
	gtk_widget_set_size_request(widget, DEFAULT_WIDTH, DEFAULT_HEIGHT);

	bool waveform_view_plus_load_new_on_idle(gpointer _view)
	{
		WaveformViewPlus* view = _view;
		g_return_val_if_fail(view, G_SOURCE_REMOVE);
		if(!promise(PROMISE_DISP_READY)->is_resolved){
			waveform_view_plus_init_drawable(view);
			gtk_widget_queue_draw((GtkWidget*)view); //testing.
		}
		return G_SOURCE_REMOVE;
	}
	// delay initialisation to allow for additional options to be set.
	g_idle_add(waveform_view_plus_load_new_on_idle, view);

	view->priv->ready = am_promise_new(view);
	am_promise_when(view->priv->ready, am_promise_new(view), am_promise_new(view), NULL);

	// TODO find a better way to ensure the canvas is ready before calling waveform_view_plus_gl_init
	gboolean try_canvas(gpointer _a)
	{
		AGlActor* a = _a;
		WaveformViewPlus* view = (WaveformViewPlus*)((AGlRootActor*)a)->widget;

		if(agl->use_shaders && !agl->shaders.plain->shader.program) return G_SOURCE_CONTINUE;

		waveform_view_plus_display_ready(view);

		return G_SOURCE_REMOVE;
	}

	void root_ready(AGlActor* a)
	{
		if(try_canvas(a)) g_idle_add(try_canvas, a);
	}

	v->root = agl_actor__new_root(widget);
	v->root->init = root_ready;
	v->canvas = wf_canvas_new((AGlRootActor*)v->root);

	v->actor = (WaveformActor*)waveform_actor(view);
	((AGlActor*)v->actor)->z = 2;

	void _waveform_view_plus_on_draw(AGlScene* scene, gpointer _view)
	{
		gtk_widget_queue_draw((GtkWidget*)_view);
	}
	AGlScene* scene = (AGlScene*)v->root;
	scene->draw = _waveform_view_plus_on_draw;
	scene->user_data = view;

	return view;
}


static void
_waveform_view_plus_set_actor (WaveformViewPlus* view)
{

	waveform_view_plus_gl_on_allocate(view);
}


static void
_waveform_view_plus__show_waveform(gpointer _view, gpointer _c)
{
	// this must NOT be called until the canvas is ready

	WaveformViewPlus* view = _view;
	g_return_if_fail(view);
	WaveformViewPlusPrivate* v = view->priv;
	g_return_if_fail(v->canvas);

	if(!((AGlActor*)v->actor)->parent){
		if(view->waveform){ // it is valid for the widget to not have a waveform set.
			agl_actor__add_child(v->root, (AGlActor*)v->actor);

			if(promise(PROMISE_DISP_READY)->is_resolved/* && !v->root->children*/){
				agl_actor__set_size((AGlActor*)v->actor);
			}

			uint64_t n_frames = waveform_get_n_frames(view->waveform);
			if(n_frames){
				if(!v->actor->region.len){
					wf_actor_set_region(v->actor, &(WfSampleRegion){0, n_frames});
				}
				wf_actor_set_colour(v->actor, view->fg_colour);

				void _waveform_view_plus__show_waveform_done(Waveform* w, gpointer _view)
				{
					WaveformViewPlus* view = _view;
					WaveformViewPlusPrivate* v = view->priv;

					if(w == view->waveform){ // it may have changed during load
						if(!g_list_find (v->root->children, v->actor)){
							((AGlActor*)v->actor)->set_size((AGlActor*)v->actor);
						}

						am_promise_resolve(promise(PROMISE_WAVE_READY), NULL);
					}
				}

				g_signal_connect (view->waveform, "peakdata-ready", (GCallback)_waveform_view_plus__show_waveform_done, view);
			}
			_waveform_view_plus_set_actor(view);
		}
	}
	gtk_widget_queue_draw((GtkWidget*)view);
}


void
waveform_view_plus_load_file (WaveformViewPlus* view, const char* filename, WfCallback2 callback, gpointer user_data)
{
	dbg(1, "%s", filename);

	WaveformViewPlusPrivate* v = view->priv;

	if(view->waveform){
		_g_object_unref0(view->waveform);
	}

	if(!filename){
		gtk_widget_queue_draw((GtkWidget*)view);
		return;
	}

	WfClosure* c = g_new0(WfClosure, 1);
	*c = (WfClosure){ .callback = callback, .user_data = user_data};

	void waveform_view_plus_load_file_done(WaveformActor* a, gpointer _c)
	{
		WfClosure* c = (WfClosure*)_c;

		if(((AGlActor*)a)->parent) agl_actor__invalidate(((AGlActor*)a)->parent); // we dont seem to track the layers, so have to invalidate everything.

		call(c->callback, a->waveform, c->user_data);
		g_free(c);
	}

	view->waveform = waveform_new(filename);
	if(v->actor){
		wf_actor_set_waveform(v->actor, view->waveform, waveform_view_plus_load_file_done, c);
	}

	am_promise_add_callback(promise(PROMISE_DISP_READY), _waveform_view_plus__show_waveform, NULL);
}


void
waveform_view_plus_set_waveform (WaveformViewPlus* view, Waveform* waveform)
{
	PF;
	WaveformViewPlusPrivate* v = view->priv;

	if(view->waveform){
		g_object_unref(view->waveform);
		v->actor->region.len = 0; // force it to be set once the wav is loaded
	}
	view->waveform = g_object_ref(waveform);

	if(v->actor){
		wf_actor_set_waveform_sync(v->actor, waveform);
	}

	view->zoom = 1.0;

	am_promise_add_callback(promise(PROMISE_DISP_READY), _waveform_view_plus__show_waveform, NULL);
}


void
waveform_view_plus_set_zoom (WaveformViewPlus* view, float zoom)
{
	#define MAX_ZOOM 51200.0 //TODO
	g_return_if_fail(view);
	WaveformViewPlusPrivate* v = view->priv;
	dbg(1, "zoom=%.2f", zoom);

	view->zoom = CLAMP(zoom, 1.0, MAX_ZOOM);

	wf_actor_set_region(v->actor, &(WfSampleRegion){
		view->start_frame,
		(waveform_get_n_frames(view->waveform) - view->start_frame) / view->zoom
	});

#ifdef AGL_ACTOR_RENDER_CACHE
	((AGlActor*)v->actor)->cache.valid = false;
#endif

	if(!((AGlActor*)v->actor)->root->draw) gtk_widget_queue_draw((GtkWidget*)view);
}


void
waveform_view_plus_set_start (WaveformViewPlus* view, int64_t start_frame)
{
	WaveformViewPlusPrivate* v = view->priv;

	uint32_t length = waveform_get_n_frames(view->waveform) / view->zoom;
	view->start_frame = CLAMP(start_frame, 0, (int64_t)waveform_get_n_frames(view->waveform) - 10);
	view->start_frame = MIN(view->start_frame, waveform_get_n_frames(view->waveform) - length);
	dbg(1, "start=%Lu", view->start_frame);
	wf_actor_set_region(view->priv->actor, &(WfSampleRegion){
		view->start_frame,
		length
	});
	if(!((AGlActor*)v->actor)->root->draw) gtk_widget_queue_draw((GtkWidget*)view);
}


void
waveform_view_plus_set_region (WaveformViewPlus* view, int64_t start_frame, int64_t end_frame)
{
	g_return_if_fail(view->waveform);
	g_return_if_fail(end_frame > start_frame);
	WaveformViewPlusPrivate* v = view->priv;
	g_return_if_fail(v->actor);

	WfSampleRegion region = (WfSampleRegion){start_frame, end_frame - start_frame};

	region.len = MIN(region.len, waveform_get_n_frames(view->waveform) - region.start);

	view->start_frame = CLAMP(region.start, 0, (int64_t)waveform_get_n_frames(view->waveform) - 10);
	view->zoom = view->waveform->n_frames / (float)region.len;
	dbg(1, "start=%Lu", view->start_frame);

	wf_actor_set_region(v->actor, &region);

	if(!((AGlActor*)v->actor)->root->draw) gtk_widget_queue_draw((GtkWidget*)view);
}


void
waveform_view_plus_set_colour(WaveformViewPlus* view, uint32_t fg, uint32_t bg/*, uint32_t text1, uint32_t text2*/)
{
	WaveformViewPlusPrivate* v = view->priv;

	view->fg_colour = fg;
	view->bg_colour = bg;
	if(view->priv->actor) wf_actor_set_colour(v->actor, fg);

	if(agl_get_instance()->use_shaders){
	}
}


void
waveform_view_plus_set_show_rms (WaveformViewPlus* view, gboolean _show)
{
	//FIXME this idle hack is because wa is not created until realise.
	//      (still true?)

	static gboolean show; show = _show;

	gboolean _on_idle(gpointer _view)
	{
		WaveformViewPlus* view = _view;
		if(!view->priv->canvas) return G_SOURCE_CONTINUE;

		view->priv->canvas->show_rms = show;
		gtk_widget_queue_draw((GtkWidget*)view);

		return G_SOURCE_REMOVE;
	}
	g_idle_add(_on_idle, view);
}


AGlActor*
waveform_view_plus_add_layer(WaveformViewPlus* view, AGlActor* actor, int z)
{
	PF;
	WaveformViewPlusPrivate* v = view->priv;

	actor->z = z;
	agl_actor__add_child(v->root, actor);

	agl_actor__invalidate(v->root);

	return actor;
}


AGlActor*
waveform_view_plus_get_layer(WaveformViewPlus* view, int z)
{
	return agl_actor__find_by_z(view->priv->root, z);
}


void
waveform_view_plus_remove_layer(WaveformViewPlus* view, AGlActor* actor)
{
	g_return_if_fail(actor);
	WaveformViewPlusPrivate* v = view->priv;

	agl_actor__remove_child(v->root, actor);
}


static void
waveform_view_plus_realize (GtkWidget* widget)
{
	PF2;
	WaveformViewPlus* view = (WaveformViewPlus*)widget;
	GdkWindowAttr attrs = {0};
	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
	memset (&attrs, 0, sizeof (GdkWindowAttr));
	attrs.window_type = GDK_WINDOW_CHILD;
	attrs.width = widget->allocation.width;
	attrs.wclass = GDK_INPUT_OUTPUT;
	attrs.event_mask = gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK | GDK_KEY_PRESS_MASK | GDK_ENTER_NOTIFY_MASK;
	_g_object_unref0(widget->window);
	widget->window = gdk_window_new (gtk_widget_get_parent_window(widget), &attrs, 0);
	gdk_window_set_user_data (widget->window, view);
	gtk_widget_set_style (widget, gtk_style_attach(gtk_widget_get_style(widget), widget->window));
	gtk_style_set_background (gtk_widget_get_style (widget), widget->window, GTK_STATE_NORMAL);
	gdk_window_move_resize (widget->window, widget->allocation.x, widget->allocation.y, widget->allocation.width, widget->allocation.height);

	if(!view->fg_colour){
		// currently the waveform background is always dark, so a light colour is needed for the foreground
		uint32_t base_colour = wf_get_gtk_base_color(widget, GTK_STATE_NORMAL, 0xaa);
		view->fg_colour = wf_colour_is_dark_rgba(base_colour)
			? wf_get_gtk_fg_color(widget, GTK_STATE_NORMAL)
			: base_colour;
	}

	if(!promise(PROMISE_DISP_READY)->is_resolved) waveform_view_plus_init_drawable(view);
}


static void
waveform_view_plus_unrealize (GtkWidget* widget)
{
	// view->waveform can not be unreffed here as it needs to be re-used if the widget is realized again.
	// The gl context and actors are now preserved through an unrealize/realize cycle - the only thing that changes is the GlDrawable.
	PF;
	WaveformViewPlus* view = (WaveformViewPlus*)widget;
	WaveformViewPlusPrivate* v = view->priv;
	gdk_window_set_user_data (widget->window, NULL);

	am_promise_unref(promise(PROMISE_DISP_READY));

	// create a new promise that will be resolved if and when the canvas is available again.
	GList* nth = g_list_nth(v->ready->children, PROMISE_DISP_READY);
	nth->data = am_promise_new(view);
	am_promise_add_callback(nth->data, _waveform_view_plus__show_waveform, NULL);
}


static gboolean
waveform_view_plus_on_expose (GtkWidget* widget, GdkEventExpose* event)
{
	WaveformViewPlus* view = (WaveformViewPlus*)widget;
	g_return_val_if_fail (event, FALSE);

	if(!GTK_WIDGET_REALIZED(widget)) return true;
	if(!promise(PROMISE_DISP_READY)->is_resolved) return true;

	AGL_ACTOR_START_DRAW(view->priv->root) {
		// needed for the case of shared contexts, where one of the other contexts modifies the projection.
		waveform_view_plus_set_projection(widget);

		AGlColourFloat bg; wf_colour_rgba_to_float(&bg, view->bg_colour);
		glClearColor(bg.r, bg.g, bg.b, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if(promise(PROMISE_DISP_READY)->is_resolved){
			waveform_view_plus_draw(view);
		}

		gdk_gl_drawable_swap_buffers(((AGlRootActor*)view->priv->root)->gl.gdk.drawable);
	} AGL_ACTOR_END_DRAW(view->priv->root)

	return true;
}


static gboolean
waveform_view_plus_button_press_event (GtkWidget* widget, GdkEventButton* event)
{
	g_return_val_if_fail (event != NULL, false);
	gboolean handled = false;

	switch (event->type){
		case GDK_BUTTON_PRESS:
			dbg(0, "GDK_BUTTON_PRESS");
			gtk_widget_grab_focus(widget);
			handled = true;
			break;
		default:
			dbg(0, "unexpected event type");
			break;
	}
	return handled;
}


static gboolean
waveform_view_plus_button_release_event (GtkWidget* widget, GdkEventButton* event)
{
	g_return_val_if_fail(event, false);
	gboolean result = false;
	return result;
}


static gboolean
waveform_view_plus_motion_notify_event (GtkWidget* widget, GdkEventMotion* event)
{
	g_return_val_if_fail(event, false);
	gboolean result = false;
	return result;
}


static gboolean
waveform_view_plus_focus_in_event(GtkWidget* widget, GdkEventFocus* event)
{
	WaveformViewPlus* view = (WaveformViewPlus*)widget;

	add_key_handlers((GtkWindow*)gtk_widget_get_toplevel(widget), view, keys);

	return false;
}


static gboolean
waveform_view_plus_focus_out_event(GtkWidget* widget, GdkEventFocus* event)
{
	WaveformViewPlus* view = (WaveformViewPlus*)widget;

	remove_key_handlers((GtkWindow*)gtk_widget_get_toplevel(widget),  view);

	return false;
}


static gboolean
waveform_view_plus_enter_notify_event(GtkWidget* widget, GdkEventCrossing* event)
{
	return false;
}


static gboolean
waveform_view_plus_leave_notify_event(GtkWidget* widget, GdkEventCrossing* event)
{
	return false;
}


static void
waveform_view_plus_display_ready(WaveformViewPlus* view)
{

	dbg(1, "READY");
	am_promise_resolve(promise(PROMISE_DISP_READY), NULL);
}


static bool
waveform_view_plus_init_drawable (WaveformViewPlus* view)
{
	GtkWidget* widget = (GtkWidget*)view;
	WaveformViewPlusPrivate* v = view->priv;

	if(GTK_WIDGET_REALIZED(widget) && ((AGlRootActor*)v->root)->gl.gdk.context){
		waveform_view_plus_display_ready(view);
		return true;
	}

	return false;
}


static void
waveform_view_plus_allocate (GtkWidget* widget, GdkRectangle* allocation)
{
	PF;
	g_return_if_fail (allocation);

	WaveformViewPlus* view = (WaveformViewPlus*)widget;
	widget->allocation = (GtkAllocation)(*allocation);
	if ((GTK_WIDGET_FLAGS (widget) & GTK_REALIZED) == 0) return;
	gdk_window_move_resize(widget->window, widget->allocation.x, widget->allocation.y, widget->allocation.width, widget->allocation.height);

	if(!promise(PROMISE_DISP_READY)->is_resolved)
		if(!waveform_view_plus_init_drawable(view)) return;

	waveform_view_plus_gl_on_allocate(view);

	waveform_view_plus_set_projection(widget);
}


static void
waveform_view_plus_class_init (WaveformViewPlusClass * klass)
{
	waveform_view_plus_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (WaveformViewPlusPrivate));
	GTK_WIDGET_CLASS (klass)->expose_event = waveform_view_plus_on_expose;
	GTK_WIDGET_CLASS (klass)->button_press_event = waveform_view_plus_button_press_event;
	GTK_WIDGET_CLASS (klass)->button_release_event = waveform_view_plus_button_release_event;
	GTK_WIDGET_CLASS (klass)->motion_notify_event = waveform_view_plus_motion_notify_event;
	GTK_WIDGET_CLASS (klass)->focus_in_event = waveform_view_plus_focus_in_event;
	GTK_WIDGET_CLASS (klass)->focus_out_event = waveform_view_plus_focus_out_event;
	GTK_WIDGET_CLASS (klass)->enter_notify_event = waveform_view_plus_enter_notify_event;
	GTK_WIDGET_CLASS (klass)->leave_notify_event = waveform_view_plus_leave_notify_event;
	GTK_WIDGET_CLASS (klass)->realize = waveform_view_plus_realize;
	GTK_WIDGET_CLASS (klass)->unrealize = waveform_view_plus_unrealize;
	GTK_WIDGET_CLASS (klass)->size_allocate = waveform_view_plus_allocate;
	G_OBJECT_CLASS (klass)->finalize = waveform_view_plus_finalize;

	agl = agl_get_instance();
}


static void
waveform_view_plus_instance_init (WaveformViewPlus * self)
{
	self->zoom = 1.0;
	self->start_frame = 0;
	self->priv = WAVEFORM_VIEW_PLUS_GET_PRIVATE (self);
	self->priv->canvas = NULL;
	self->priv->actor = NULL;
}


static void
waveform_view_plus_finalize (GObject* obj)
{
	WaveformViewPlus* view = WAVEFORM_VIEW_PLUS(obj);
	WaveformViewPlusPrivate* v = view->priv;

	// these should really be done in dispose
	if(v->actor){
		wf_actor_clear(v->actor);
		wf_canvas_remove_actor(v->canvas, v->actor);
		v->actor = NULL;

	}
	if(v->canvas) wf_canvas_free0(v->canvas);
	if(view->waveform) waveform_unref0(view->waveform);

	G_OBJECT_CLASS (waveform_view_plus_parent_class)->finalize(obj);
}


GType
waveform_view_plus_get_type ()
{
	static volatile gsize waveform_view_plus_type_id__volatile = 0;
	if (g_once_init_enter (&waveform_view_plus_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (WaveformViewPlusClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) waveform_view_plus_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (WaveformViewPlus), 0, (GInstanceInitFunc) waveform_view_plus_instance_init, NULL };
		GType waveform_view_plus_type_id;
		waveform_view_plus_type_id = g_type_register_static (GTK_TYPE_DRAWING_AREA, "WaveformViewPlus", &g_define_type_info, 0);
		g_once_init_leave (&waveform_view_plus_type_id__volatile, waveform_view_plus_type_id);
	}
	return waveform_view_plus_type_id__volatile;
}


static void
waveform_view_plus_set_projection(GtkWidget* widget)
{
	int vx = 0;
	int vy = 0;
	int vw = waveform_view_plus_get_width((WaveformViewPlus*)widget);
	int vh = waveform_view_plus_get_height((WaveformViewPlus*)widget);
	glViewport(vx, vy, vw, vh);
	dbg (2, "viewport: %i %i %i %i", vx, vy, vw, vh);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// set clipping volume (left, right, bottom, top, near, far):
	int width = vw;
	double hborder = 0 * width / 32;

	double left = -hborder;
	double right = width + hborder;
	double top   = 0.0;
	double bottom = vh;
	glOrtho (left, right, bottom, top, 10.0, -100.0);
}


/*
 *  Returns the underlying canvas that the WaveformViewPlus is using.
 */
WaveformCanvas*
waveform_view_plus_get_canvas(WaveformViewPlus* view)
{
	g_return_val_if_fail(view, NULL);
	return view->priv->canvas;
}


WaveformActor*
waveform_view_plus_get_actor(WaveformViewPlus* view)
{
	g_return_val_if_fail(view, NULL);
	return view->priv->actor;
}


static int
waveform_view_plus_get_width(WaveformViewPlus* view)
{
	GtkWidget* widget = (GtkWidget*)view;

	return GTK_WIDGET_REALIZED(widget) ? widget->allocation.width : 256;
}


static int
waveform_view_plus_get_height(WaveformViewPlus* view)
{
	GtkWidget* widget = (GtkWidget*)view;

	return GTK_WIDGET_REALIZED(widget) ? widget->allocation.height : DEFAULT_HEIGHT;
}


static void
add_key_handlers(GtkWindow* window, WaveformViewPlus* waveform, Key keys[])
{
	//list of keys must be terminated with a key of value zero.

	static KeyHold key_hold = {0, NULL};
	static bool key_down = false;

	static GHashTable* key_handlers = NULL;
	if(!key_handlers){
		key_handlers = g_hash_table_new(g_int_hash, g_int_equal);

		int i = 0; while(true){
			Key* key = &keys[i];
			if(i > 100 || !key->key) break;
			g_hash_table_insert(key_handlers, &key->key, key->handler);
			i++;
		}
	}

	gboolean key_hold_on_timeout(gpointer user_data)
	{
		WaveformViewPlus* waveform = user_data;
		if(key_hold.handler) key_hold.handler(waveform);
		return TIMER_CONTINUE;
	}

	gboolean key_press(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
	{
		WaveformViewPlus* waveform = user_data;

		if(key_down){
			// key repeat
			return true;
		}
		key_down = true;

		KeyHandler* handler = g_hash_table_lookup(key_handlers, &event->keyval);
		if(handler){
			if(key_hold.timer) gwarn("timer already started");
			key_hold.timer = g_timeout_add(100, key_hold_on_timeout, waveform);
			key_hold.handler = handler;
	
			handler(waveform);
		}
		else dbg(1, "%i", event->keyval);

		return true;
	}

	gboolean key_release(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
	{
		PF;
		if(!key_down){ /* gwarn("key_down not set"); */ return true; } //sometimes happens at startup

		key_down = false;
		if(key_hold.timer) g_source_remove(key_hold.timer);
		key_hold.timer = 0;

		return true;
	}

	g_signal_connect(waveform, "key-press-event", G_CALLBACK(key_press), waveform);
	g_signal_connect(waveform, "key-release-event", G_CALLBACK(key_release), waveform);
}


static void
remove_key_handlers(GtkWindow* window, WaveformViewPlus* waveform)
{
	g_signal_handlers_disconnect_matched (waveform, G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_DATA, g_signal_lookup ("key-press-event", GTK_TYPE_WIDGET), 0, NULL, NULL, waveform);
	g_signal_handlers_disconnect_matched (waveform, G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_DATA, g_signal_lookup ("key-release-event", GTK_TYPE_WIDGET), 0, NULL, NULL, waveform);
}


static void
scroll_left(WaveformViewPlus* view)
{
	if(!view->waveform) return;

	int n_visible_frames = ((float)view->waveform->n_frames) / view->zoom;
	waveform_view_plus_set_start(view, view->start_frame - n_visible_frames / 10);
}


static void
scroll_right(WaveformViewPlus* view)
{
	if(!view->waveform) return;

	int n_visible_frames = ((float)view->waveform->n_frames) / view->zoom;
	waveform_view_plus_set_start(view, view->start_frame + n_visible_frames / 10);
}


static void
zoom_in(WaveformViewPlus* view)
{
	waveform_view_plus_set_zoom(view, view->zoom * 1.5);
}


static void
zoom_out(WaveformViewPlus* view)
{
	waveform_view_plus_set_zoom(view, view->zoom / 1.5);
}


static void
waveform_view_plus_draw(WaveformViewPlus* view)
{
	WaveformViewPlusPrivate* v = view->priv;

#if 0 //white border
	glPushMatrix(); /* modelview matrix */
		glNormal3f(0, 0, 1); glDisable(GL_TEXTURE_2D);
		glLineWidth(1);
		glColor3f(1.0, 1.0, 1.0);

		int wid = waveform_view_plus_get_width(view);
		int h   = waveform_view_plus_get_height(view);
		glBegin(GL_LINES);
		glVertex3f(0.0, 0.0, 1); glVertex3f(wid, 0.0, 1);
		glVertex3f(wid, h,   1); glVertex3f(0.0,   h, 1);
		glEnd();
	glPopMatrix();
#endif

	if(view->waveform) agl_actor__paint(v->root);
}


static void
waveform_view_plus_gl_on_allocate(WaveformViewPlus* view)
{
	WaveformViewPlusPrivate* v = view->priv;

	if(!v->actor) return;

	int w = waveform_view_plus_get_width(view);
	int h = waveform_view_plus_get_height(view);

	if(w != v->root->region.x2 || h != v->root->region.y2){
		v->root->region = (AGliRegion){0, 0, w, h};
		agl_actor__set_size(v->root);
	}

	wf_canvas_set_viewport(v->canvas, NULL);
}


static AGlActor*
waveform_actor(WaveformViewPlus* view)
{
	void waveform_actor_size(AGlActor* actor)
	{
		#define V_BORDER 4

		actor->region = (AGliRegion){
			.x1 = 0,
			.y1 = V_BORDER,
			.x2 = actor->parent->region.x2,
			.y2 = actor->parent->region.y2 - V_BORDER,
		};
		wf_actor_set_rect((WaveformActor*)actor, &(WfRectangle){0, 0, actor->region.x2, actor->region.y2 - actor->region.y1});
	}

	void waveform_actor_size0(AGlActor* actor)
	{
		waveform_actor_size(actor);
#ifdef AGL_ACTOR_RENDER_CACHE
		actor->fbo = agl_fbo_new(actor->region.x2 - actor->region.x1, actor->region.y2 - actor->region.y1, 0, 0);
		actor->cache.enabled = true;
#endif
		actor->set_size = waveform_actor_size;
	}

	AGlActor* actor = (AGlActor*)wf_canvas_add_new_actor(view->priv->canvas, view->waveform);
	actor->colour = view->fg_colour;
	actor->set_size = waveform_actor_size0;
	return actor;
}


