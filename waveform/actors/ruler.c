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

  WaveformGrid draws timeline markers onto a shared opengl drawable.

*/
#define __wf_private__
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <gtk/gtk.h>
#include <GL/gl.h>
#include "waveform/waveform.h"
#include "waveform/canvas.h"
#include "waveform/actors/grid.h"
typedef struct {
    AGlActor        actor;
	WaveformCanvas* context;
} RulerActor;

static AGl* agl = NULL;

static bool ruler_actor_paint(AGlActor*);


AGlActor*
ruler_actor(WaveformActor* wf_actor)
{
	g_return_val_if_fail(wf_actor, NULL);

	agl = agl_get_instance();

	void ruler_actor_size(AGlActor* actor)
	{
		//actor->region = actor->parent->region;
		actor->region.x1 = actor->parent->region.x1;
		actor->region.x2 = actor->parent->region.x2;
	}

	void ruler_init(AGlActor* actor)
	{
		RulerActor* ruler = (RulerActor*)actor;
		if(agl->use_shaders){
			if(!ruler->context->shaders.ruler->shader.program)
				agl_create_program(&ruler->context->shaders.ruler->shader);
		}
#ifdef AGL_ACTOR_RENDER_CACHE
		actor->fbo = agl_fbo_new(actor->region.x2 - actor->region.x1, actor->region.y2 - actor->region.y1, 0, 0);
#endif
	}

	void ruler_set_state(AGlActor* actor)
	{
		if(!agl->use_shaders) return;

		RulerActor* ruler = (RulerActor*)actor;
		RulerShader* shader = ruler->context->shaders.ruler;

		shader->uniform.fg_colour = 0xffffff7f;
		shader->uniform.beats_per_pixel = ruler->context->samples_per_pixel / ruler->context->sample_rate; // TODO

		agl_use_program((AGlShader*)shader);
	}

	RulerActor* ruler = g_new0(RulerActor, 1);
	ruler->context = wf_actor->canvas;

	AGlActor* actor = (AGlActor*)ruler;
#ifdef AGL_DEBUG_ACTOR
	actor->name = "ruler";
#endif
	actor->init = ruler_init;
	actor->set_state = ruler_set_state;
	actor->paint = ruler_actor_paint;
	actor->set_size = ruler_actor_size;
	return actor;
}


static bool
ruler_actor_paint(AGlActor* actor)
{
	if(!agl->use_shaders) return false;

#if 0 //shader debugging
	{
		float smoothstep(float edge0, float edge1, float x)
		{
			float t = CLAMP((x - edge0) / (edge1 - edge0), 0.0, 1.0);
			return t * t * (3.0 - 2.0 * t);
		}

		float pixels_per_beat = 1.0 / wfc->priv->shaders.ruler->uniform.beats_per_pixel;
		dbg(0, "ppb=%.2f", pixels_per_beat);
		int x; for(x=0;x<30;x++){
			float m = (x * 100) % ((int)pixels_per_beat * 100);
			float m_ = x - pixels_per_beat * floor(x / pixels_per_beat);
			printf("  %.2f %.2f %.2f\n", m / 100, m_, smoothstep(0.0, 0.5, m_));
		}
	}
#endif

	glPushMatrix();
	glScalef(1.0, -1.0, 1.0);           // inverted vertically to make alignment of marks to bottom easier in the shader
	glTranslatef(0.0, -(actor->region.y2 - actor->region.y1), 0.0); // making more negative moves downward
	glRecti(actor->region.x1, actor->region.y1, actor->region.x2, actor->region.y2);
	glPopMatrix();
	return true;
}


