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
#define __wf_private__
#include "config.h"
#include <stdio.h>
#include "glib.h"
#include "waveform/utils.h"
#include "waveform/promise.h"

typedef struct {
    AMPromiseCallback callback;
    gpointer          user_data;
} Item;


AMPromise*
am_promise_new(gpointer user_data)
{
	AMPromise* p = g_new0(AMPromise, 1);
	p->user_data = user_data;
	p->refcount = 1;
	return p;
}


void
am_promise_unref(AMPromise* p)
{
	if(!--p->refcount){
		g_list_free_full(p->callbacks, g_free);
		g_free(p);
	}
}


void
_am_promise_finish(AMPromise* p)
{
	if(p->callbacks){
		GList* l = p->callbacks;
		for(;l;l=l->next){
			Item* item = l->data;
			item->callback(p->user_data, item->user_data);
		}

		// each callback is only ever called once
		g_list_free_full(p->callbacks, g_free);
		p->callbacks = NULL;

#if 0 // the promise should not be free'd by default
		am_promise_unref(p);
#endif
	}
}


void
_add_callback(AMPromise* p, AMPromiseCallback callback, gpointer user_data)
{
	Item* item = g_new0(Item, 1);
	*item = (Item){
		.callback = callback,
		.user_data = user_data
	};
	p->callbacks = g_list_append(p->callbacks, item);
}


void
am_promise_add_callback(AMPromise* p, AMPromiseCallback callback, gpointer user_data)
{
	_add_callback(p, callback, user_data);
	if(p->is_resolved) _am_promise_finish(p);
}


void
am_promise_resolve(AMPromise* p, PromiseVal* value)
{
	p->is_resolved = true;
	_am_promise_finish(p);
}


void
am_promise_fail(AMPromise* p)
{
	p->is_failed = true;
}


/*
 *  The promise will be resolved when all the child promises are resolved.
 */
void
am_promise_when(AMPromise* promise, AMPromise* p, ...)
{
	if(!p) return;

	void then(gpointer _, gpointer _parent)
	{
		AMPromise* parent = _parent;
		g_return_if_fail(parent);

		bool complete = true;
		GList* l = parent->children;
		for(;l;l=l->next){
			AMPromise* p = l->data;
			if(!p->is_resolved){
				complete = false;
				break;
			}
		}
		if(complete) am_promise_resolve(parent, &(PromiseVal){.i=-1});
	}

	void add_child(AMPromise* promise, AMPromise* child)
	{
		g_return_if_fail(child);
		promise->children = g_list_append(promise->children, child);
		am_promise_add_callback(child, then, promise);
	}
	add_child(promise, p);

	va_list args;
	va_start(args, p);
	AMPromise* q;
	while((q = va_arg (args, AMPromise*))){
		add_child(promise, q);
	}
	va_end(args);
}


