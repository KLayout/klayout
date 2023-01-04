
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#include <limits>

namespace db 
{

LayoutStateModel::LayoutStateModel (bool busy)
  : m_hier_dirty (false), m_hier_generation_id (0), m_all_bboxes_dirty (false), m_busy (busy)
{
  //  .. nothing yet ..
}

LayoutStateModel::LayoutStateModel (const LayoutStateModel &d)
  : m_hier_dirty (d.m_hier_dirty), m_hier_generation_id (d.m_hier_generation_id), m_bboxes_dirty (d.m_bboxes_dirty), m_all_bboxes_dirty (d.m_all_bboxes_dirty), m_busy (d.m_busy)
{
  //  .. nothing yet ..
}

LayoutStateModel &
LayoutStateModel::operator= (const LayoutStateModel &d)
{
  m_hier_dirty = d.m_hier_dirty;
  m_hier_generation_id = d.m_hier_generation_id;
  m_bboxes_dirty = d.m_bboxes_dirty;
  m_all_bboxes_dirty = d.m_all_bboxes_dirty;
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
LayoutStateModel::do_invalidate_bboxes (unsigned int index)
{
  bboxes_changed_event (index);
  bboxes_changed_any_event ();
}

void
LayoutStateModel::invalidate_bboxes (unsigned int index)
{
  if (index == std::numeric_limits<unsigned int>::max ()) {
    if (! m_all_bboxes_dirty || m_busy) {
      do_invalidate_bboxes (index);  //  must be called before the bboxes are invalidated (stopping of redraw thread requires this)
      m_all_bboxes_dirty = true;
    }
  } else {
    if ((! m_all_bboxes_dirty && (index >= (unsigned int) m_bboxes_dirty.size () || ! m_bboxes_dirty [index])) || m_busy) {
      do_invalidate_bboxes (index);  //  must be called before the bboxes are invalidated (stopping of redraw thread requires this)
      if (index >= (unsigned int) m_bboxes_dirty.size ()) {
        m_bboxes_dirty.resize (index + 1, false);
      }
      m_bboxes_dirty [index] = true;
    }
  }
}

bool
LayoutStateModel::bboxes_dirty () const
{
  return ! m_bboxes_dirty.empty () || m_all_bboxes_dirty;
}

void
LayoutStateModel::update ()
{
  if (bboxes_dirty () || m_hier_dirty) {
    do_update ();
    m_bboxes_dirty.clear ();
    m_all_bboxes_dirty = false;
    m_hier_dirty = false;
  }
}

}

