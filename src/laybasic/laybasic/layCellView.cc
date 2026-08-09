
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#include "layCellView.h"
#include "layLayoutViewBase.h"

namespace lay
{

// -------------------------------------------------------------
//  CellView implementation

CellView::CellView () 
  : mp_ctx_cell (0), m_ctx_cell_index (0), mp_cell (0), m_cell_index (cell_index_type (-1))
{ }

bool 
CellView::operator== (const CellView &cv) const
{
  return m_layout_href == cv.m_layout_href 
        && mp_ctx_cell == cv.mp_ctx_cell && m_ctx_cell_index == cv.m_ctx_cell_index 
        && mp_cell == cv.mp_cell && m_cell_index == cv.m_cell_index 
        && m_unspecific_path == cv.m_unspecific_path && m_specific_path == cv.m_specific_path;
}

bool 
CellView::is_valid () const
{
  if (m_layout_href.get () == 0 || mp_cell == 0) {
    return false;
  }

  //  check, if the path references valid cell indices.
  for (unspecific_cell_path_type::const_iterator pp = m_unspecific_path.begin (); pp != m_unspecific_path.end (); ++pp) {
    if (! m_layout_href.get ()->layout ().is_valid_cell_index (*pp)) {
      return false;
    }
  }
  for (specific_cell_path_type::const_iterator pp = m_specific_path.begin (); pp != m_specific_path.end (); ++pp) {
    if (! pp->inst_ptr.instances () || ! pp->inst_ptr.instances ()->is_valid (pp->inst_ptr) || ! m_layout_href.get ()->layout ().is_valid_cell_index (pp->inst_ptr.cell_index ())) {
      return false;
    }
  }

  return true;
}

void 
CellView::set_unspecific_path (const unspecific_cell_path_type &p)
{
  tl_assert (m_layout_href.get () != 0);

  mp_cell = 0;
  m_cell_index = 0;
  m_unspecific_path = p;
  m_specific_path.clear ();

  if (p.size () > 0 && m_layout_href.get () && p.back () < m_layout_href->layout ().cells ()) {
    m_cell_index = p.back ();
    mp_cell = &m_layout_href->layout ().cell (p.back ());
  }

  mp_ctx_cell = mp_cell;
  m_ctx_cell_index = m_cell_index;
}

void 
CellView::set_specific_path (const specific_cell_path_type &p)
{
  tl_assert (m_layout_href.get () != 0);

  m_specific_path = p;
  for (specific_cell_path_type::iterator pp = m_specific_path.begin (); pp != m_specific_path.end (); ++pp) {
    //  fix elements of the path not associated with a certain array instance (this may happen if 
    //  unspecific selections are put into the path)
    if (pp->array_inst.at_end ()) {
      pp->array_inst = pp->inst_ptr.begin ();
    }
  }

  if (p.empty ()) {
    m_cell_index = m_ctx_cell_index;
    mp_cell = mp_ctx_cell;
  } else if (m_layout_href.get () && p.back ().inst_ptr.cell_index () < m_layout_href->layout ().cells ()) {
    m_cell_index = p.back ().inst_ptr.cell_index ();
    mp_cell = &m_layout_href->layout ().cell (m_cell_index);
  } else {
    reset_cell ();
  }
}

CellView::unspecific_cell_path_type 
CellView::combined_unspecific_path () const
{
  CellView::unspecific_cell_path_type path;
  path.reserve (m_unspecific_path.size () + m_specific_path.size ());
  path.insert (path.end (), m_unspecific_path.begin (), m_unspecific_path.end ());
  for (CellView::specific_cell_path_type::const_iterator p = m_specific_path.begin (); p != m_specific_path.end (); ++p) {
    path.push_back (p->inst_ptr.cell_index ());
  }
  return path;
}

void
CellView::set_cell (cell_index_type index)
{
  tl_assert (m_layout_href.get () != 0);

  db::Layout &layout = m_layout_href->layout ();
  
  if (! layout.is_valid_cell_index (index)) {

    reset_cell ();

  } else {

    m_cell_index = index;
    mp_cell = &layout.cell (m_cell_index);

    m_unspecific_path.clear ();
    m_specific_path.clear ();
    m_unspecific_path.push_back (index);

    while (! layout.cell (index).is_top ()) {
      index = *layout.cell (index).begin_parent_cells ();
      m_unspecific_path.push_back (index);
    }

    std::reverse (m_unspecific_path.begin (), m_unspecific_path.end ());

    mp_ctx_cell = mp_cell;
    m_ctx_cell_index = m_cell_index;

  }
}

void 
CellView::set_cell (const std::string &name)
{
  tl_assert (m_layout_href.get () != 0);

  std::pair<bool, db::cell_index_type> cp = m_layout_href->layout ().cell_by_name (name.c_str ());
  if (cp.first) {
    set_cell (cp.second);
  } else {
    reset_cell ();
  }
}

void 
CellView::reset_cell ()
{
  mp_cell = 0;
  m_cell_index = cell_index_type (-1);
  mp_ctx_cell = 0;
  m_ctx_cell_index = 0;
  m_unspecific_path.clear ();
  m_specific_path.clear ();
}

void 
CellView::set (lay::LayoutHandle *handle)
{
  reset_cell ();
  m_layout_href.set (handle);
}

CellView
CellView::deep_copy (db::Manager *manager) const
{
  CellView r;
  r.set (new lay::LayoutHandle (new db::Layout (manager), ""));
  r->layout () = (*this)->layout ();
  r.set_unspecific_path (unspecific_path ());
  r.set_specific_path (specific_path ());
  return r;
}

db::ICplxTrans
CellView::context_trans () const
{
  db::ICplxTrans trans;
  for (std::vector <db::InstElement>::const_iterator p = specific_path ().begin (); p != specific_path ().end (); ++p) {
    trans = trans * p->complex_trans ();
  }
  return trans;
}

db::DCplxTrans
CellView::context_dtrans () const
{
  tl_assert (m_layout_href.get () != 0);

  db::CplxTrans dbu_trans (m_layout_href->layout ().dbu ());
  return dbu_trans * context_trans () * dbu_trans.inverted ();
}


// -------------------------------------------------------------
//  CellView implementation

CellViewRef::CellViewRef ()
{
  // .. nothing yet ..
}

CellViewRef::CellViewRef (lay::CellView *cv, lay::LayoutViewBase *view)
  : mp_cv (cv), mp_view (view)
{
  // .. nothing yet ..
}

bool
CellViewRef::operator== (const CellView &cv) const
{
  if (! is_valid ()) {
    return false;
  } else {
    return mp_cv->operator== (cv);
  }
}

bool
CellViewRef::is_valid () const
{
  return mp_view && mp_cv;
}

int
CellViewRef::index () const
{
  if (!is_valid ()) {
    return -1;
  } else {
    return mp_view->index_of_cellview (mp_cv.get ());
  }
}

lay::LayoutViewBase *
CellViewRef::view ()
{
  return mp_view.get ();
}

lay::LayoutHandle *
CellViewRef::operator-> () const
{
  if (mp_cv) {
    return mp_cv->handle ();
  } else {
    return 0;
  }
}

void
CellViewRef::set_name (const std::string &name)
{
  if (is_valid ()) {
    mp_view->rename_cellview (name, mp_view->index_of_cellview (mp_cv.get ()));
  }
}

void
CellViewRef::set_unspecific_path (const CellViewRef::unspecific_cell_path_type &p)
{
  if (is_valid ()) {
    lay::CellView cv = *mp_cv;
    cv.set_unspecific_path (p);
    mp_view->select_cellview (mp_view->index_of_cellview (mp_cv.get ()), cv);
  }
}

void
CellViewRef::set_specific_path (const CellViewRef::specific_cell_path_type &p)
{
  if (is_valid ()) {
    lay::CellView cv = *mp_cv;
    cv.set_specific_path (p);
    mp_view->select_cellview (mp_view->index_of_cellview (mp_cv.get ()), cv);
  }
}

void
CellViewRef::set_cell (cell_index_type ci)
{
  if (is_valid ()) {
    lay::CellView cv = *mp_cv;
    cv.set_cell (ci);
    mp_view->select_cellview (mp_view->index_of_cellview (mp_cv.get ()), cv);
  }
}

void
CellViewRef::set_cell (const std::string &name)
{
  if (is_valid ()) {
    lay::CellView cv = *mp_cv;
    cv.set_cell (name);
    mp_view->select_cellview (mp_view->index_of_cellview (mp_cv.get ()), cv);
  }
}

void
CellViewRef::reset_cell ()
{
  if (is_valid ()) {
    lay::CellView cv = *mp_cv;
    cv.reset_cell ();
    mp_view->select_cellview (mp_view->index_of_cellview (mp_cv.get ()), cv);
  }
}

db::Cell *
CellViewRef::ctx_cell () const
{
  return is_valid () ? mp_cv->ctx_cell () : 0;
}

db::Cell *
CellViewRef::cell () const
{
  return is_valid () ? mp_cv->cell () : 0;
}

CellViewRef::unspecific_cell_path_type
CellViewRef::combined_unspecific_path () const
{
  if (is_valid ()) {
    return mp_cv->combined_unspecific_path ();
  } else {
    return CellViewRef::unspecific_cell_path_type ();
  }
}

const CellViewRef::unspecific_cell_path_type &
CellViewRef::unspecific_path () const
{
  if (is_valid ()) {
    return mp_cv->unspecific_path ();
  } else {
    static CellViewRef::unspecific_cell_path_type empty_path;
    return empty_path;
  }
}

const CellViewRef::specific_cell_path_type &
CellViewRef::specific_path () const
{
  if (is_valid ()) {
    return mp_cv->specific_path ();
  } else {
    static CellViewRef::specific_cell_path_type empty_path;
    return empty_path;
  }
}

db::ICplxTrans
CellViewRef::context_trans () const
{
  if (is_valid ()) {
    return mp_cv->context_trans ();
  } else {
    return db::ICplxTrans ();
  }
}

db::DCplxTrans
CellViewRef::context_dtrans () const
{
  if (is_valid ()) {
    return mp_cv->context_dtrans ();
  } else {
    return db::DCplxTrans ();
  }
}

}

