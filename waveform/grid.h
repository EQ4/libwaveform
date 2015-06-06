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
#ifndef __wf_grid_h__
#define __wf_grid_h__

AGlActor* grid_actor(WaveformActor*);

typedef struct {
    AGlActor        actor;
    WaveformActor*  wf_actor;     // TODO needs refactoring. maybe move some properties to the canvas.
} GridActor;

#endif //__wf_grid_h__
