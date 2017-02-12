
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#include "dbLayoutStateModel.h"

namespace db 
{

LayoutStateModel::LayoutStateModel (bool busy)
  : m_hier_dirty (false), m_bboxes_dirty (false), m_busy (busy)
{
  //  .. nothing yet ..
}

LayoutStateModel::LayoutStateModel (const LayoutStateModel &d)
  : m_hier_dirty (d.m_hier_dirty), m_bboxes_dirty (d.m_bboxes_dirty), m_busy (d.m_busy)
{
  //  .. nothing yet ..
}

LayoutStateModel &
LayoutStateModel::operator= (const LayoutStateModel &d)
{
  m_hier_dirty = d.m_hier_dirty;
  m_bboxes_dirty = d.m_bboxes_dirty;
  m_busy = d.m_busy;
  return *this;
}

LayoutStateModel::~LayoutStateModel ()
{
  //  .. nothing yet ..
}

void 
LayoutStateModel::do_invalidate_hier ()
{
  hier_changed_event ();
}

void 
LayoutStateModel::do_invalidate_bboxes ()
{
  bboxes_changed_event ();
}

}

