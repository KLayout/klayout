
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


#include "layAnnotationShapes.h"

namespace lay
{

// ---------------------------------------------------------------------------------------
//  layer_op implementation

void 
AnnotationLayerOp::insert (AnnotationShapes *shapes)
{
  shapes->insert (m_shapes.begin (), m_shapes.end ());
}

void 
AnnotationLayerOp::erase (AnnotationShapes *shapes)
{
  if (size_t (std::distance (shapes->begin (), shapes->end ())) <= m_shapes.size ()) {
    //  If all shapes are to be removed, just clear the list
    shapes->clear ();
  } else {

    //  look up the shapes to delete and collect them in a sorted list. Then pass this to 
    //  the erase_positions method of the shapes object
    std::vector<bool> done;
    done.resize (m_shapes.size (), false);

    std::sort (m_shapes.begin (), m_shapes.end ());

    std::vector<shape_type>::const_iterator s_begin = m_shapes.begin ();
    std::vector<shape_type>::const_iterator s_end = m_shapes.end ();

    std::vector<AnnotationShapes::layer_type::iterator> to_erase;
    to_erase.reserve (m_shapes.size ());

    //  This is not quite effective but seems to be the simplest way
    //  of implementing this: search for each element and erase these.
    for (AnnotationShapes::layer_type::iterator lsh = shapes->begin (); lsh != shapes->end (); ++lsh) {
      std::vector<shape_type>::const_iterator s = std::lower_bound (s_begin, s_end, *lsh);
      while (s != s_end && *s == *lsh && done [std::distance(s_begin, s)]) {
        ++s;
      }
      if (s != s_end && *s == *lsh) {
        done [std::distance(s_begin, s)] = true;
        to_erase.push_back (lsh);
      }
    }

    shapes->erase_positions (to_erase.begin (), to_erase.end ());

  }
}

// ---------------------------------------------------------------------------------------
//  Shapes implementation

AnnotationShapes::AnnotationShapes (db::Manager *manager) 
  : db::LayoutStateModel (true /*busy*/), db::Object (manager)
{
  // .. nothing yet ..
}

AnnotationShapes::AnnotationShapes (const AnnotationShapes &d)
  : db::LayoutStateModel (true /*busy*/), db::Object (d)
{
  operator= (d);
}

AnnotationShapes::AnnotationShapes (const AnnotationShapes &&d)
  : db::LayoutStateModel (true /*busy*/), db::Object (d)
{
  operator= (d);
}

AnnotationShapes::~AnnotationShapes ()
{
  clear ();
}

AnnotationShapes &
AnnotationShapes::operator= (const AnnotationShapes &d)
{
  if (&d != this) {
    clear ();
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new AnnotationLayerOp (true /*insert*/, d.m_layer.begin (), d.m_layer.end ()));
    }
    m_layer = d.m_layer;
  }
  return *this;
}

AnnotationShapes &
AnnotationShapes::operator= (const AnnotationShapes &&d)
{
  if (&d != this) {
    clear ();
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new AnnotationLayerOp (true /*insert*/, d.m_layer.begin (), d.m_layer.end ()));
    }
    m_layer = d.m_layer;
  }
  return *this;
}

void
AnnotationShapes::clear ()
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new AnnotationLayerOp (false /*not insert*/, m_layer.begin (), m_layer.end ()));
  }
  invalidate_state ();  //  HINT: must come before the change is done!
  m_layer.clear ();
}

const AnnotationShapes::shape_type & 
AnnotationShapes::insert (const shape_type &sh)
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new AnnotationLayerOp (true /*insert*/, sh));
  }
  invalidate_state ();  //  HINT: must come before the change is done!
  return *m_layer.insert (sh);
}

const AnnotationShapes::shape_type &
AnnotationShapes::insert (const shape_type &&sh)
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new AnnotationLayerOp (true /*insert*/, sh));
  }
  invalidate_state ();  //  HINT: must come before the change is done!
  return *m_layer.insert (sh);
}

void
AnnotationShapes::reserve (size_t n)
{
  m_layer.reserve (n);
}

void 
AnnotationShapes::erase (layer_type::iterator pos)
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new AnnotationLayerOp (false /*not insert*/, *pos));
  }
  invalidate_state ();  //  HINT: must come before the change is done!
  m_layer.erase (pos);
}

const AnnotationShapes::shape_type &
AnnotationShapes::replace (iterator pos, const shape_type &sh)
{
  if (&*pos != &sh && *pos != sh) {
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new AnnotationLayerOp (false /*not insert*/, *pos));
      manager ()->queue (this, new AnnotationLayerOp (true /*insert*/, sh));
    }
    invalidate_state ();  //  HINT: must come before the change is done!
    m_layer.replace (pos, sh);
  } 
  return *pos;
}

const AnnotationShapes::shape_type &
AnnotationShapes::replace (iterator pos, const shape_type &&sh)
{
  if (&*pos != &sh && *pos != sh) {
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new AnnotationLayerOp (false /*not insert*/, *pos));
      manager ()->queue (this, new AnnotationLayerOp (true /*insert*/, sh));
    }
    invalidate_state ();  //  HINT: must come before the change is done!
    m_layer.replace (pos, std::move (sh));
  }
  return *pos;
}

void
AnnotationShapes::redo (db::Op *op)
{
  AnnotationLayerOp *layop = dynamic_cast<AnnotationLayerOp *> (op);
  if (layop) {
    layop->redo (this);
  } 
}

void 
AnnotationShapes::undo (db::Op *op) 
{
  AnnotationLayerOp *layop = dynamic_cast<AnnotationLayerOp *> (op);
  if (layop) {
    layop->undo (this);
  } 
}

void
AnnotationShapes::mem_stat (db::MemStatistics *stat, db::MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  m_layer.mem_stat (stat, purpose, cat, no_self, parent);
}

void
AnnotationShapes::do_update ()
{
  m_layer.sort ();
}

}

