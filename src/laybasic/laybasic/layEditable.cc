
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#include "layEditable.h"
#include "dbClipboard.h"
#include "tlAssert.h"

#include "layPropertiesDialog.h"

#include <algorithm>
#include <memory>

namespace lay
{

// ----------------------------------------------------------------
//  A helper compare functor

template <class X, class Y>
struct first_of_pair_cmp_f 
{
  bool operator() (const std::pair<X,Y> &p1, const std::pair<X,Y> &p2) const
  {
    return p1.first < p2.first;
  }
};

// ----------------------------------------------------------------
//  Editable implementation

Editable::Editable (lay::Editables *editables)
  : mp_editables (editables)
{
  if (editables) {
    editables->m_editables.push_back (this);
  }
}

Editable::~Editable ()
{
  //  Reasoning for reset (): on MSVC, virtual functions must not be called inside
  //  the destructor. reset () avoids that lay::Editables::enable calls us.
  reset ();

  //  erase the object from the table of enabled services
  if (mp_editables) {
    mp_editables->enable (this, false);
  }
}

// ----------------------------------------------------------------
//  Editables implementation

Editables::Editables (db::Manager *manager)
  : db::Object (manager), mp_properties_dialog (0), m_move_selection (false), m_any_move_operation (false)
{
  //  .. nothing yet ..
}

Editables::~Editables ()
{
  cancel_edits ();

  if (mp_properties_dialog) {
    delete mp_properties_dialog;
    mp_properties_dialog = 0;
  }
}

void 
Editables::del (db::Transaction *transaction)
{
  std::unique_ptr<db::Transaction> trans_holder (transaction ? transaction : new db::Transaction (manager (), tl::to_string (QObject::tr ("Delete"))));

  if (has_selection ()) {

    try {

      trans_holder->open ();

      cancel_edits ();

      //  this dummy operation will update the screen:
      manager ()->queue (this, new db::Op ());

      for (iterator e = begin (); e != end (); ++e) {
        e->del ();
      }

    } catch (...) {
      trans_holder->cancel ();
      throw;
    }

  }
}

void 
Editables::cut ()
{
  if (has_selection ()) {

    cancel_edits ();

    //  this dummy operation will update the screen:
    manager ()->queue (this, new db::Op ());

    db::Clipboard::instance ().clear ();
    for (iterator e = begin (); e != end (); ++e) {
      e->cut ();
    }

  }
}

void 
Editables::copy ()
{
  if (has_selection ()) {
    db::Clipboard::instance ().clear ();
    for (iterator e = begin (); e != end (); ++e) {
      e->copy ();
    }
  }
}

db::DBox 
Editables::selection_bbox ()
{
  db::DBox sel_bbox;
  for (iterator e = begin (); e != end (); ++e) {
    sel_bbox += e->selection_bbox ();
  }
  return sel_bbox;
}

db::DBox
Editables::selection_catch_bbox ()
{
  db::DBox sel_bbox;
  for (iterator e = begin (); e != end (); ++e) {
    double l = e->catch_distance ();
    sel_bbox += e->selection_bbox ().enlarged (db::DVector (l, l));
  }
  return sel_bbox;
}

void
Editables::transform (const db::DCplxTrans &tr, db::Transaction *transaction)
{
  std::unique_ptr<db::Transaction> trans_holder (transaction ? transaction : new db::Transaction (manager (), tl::to_string (QObject::tr ("Transform"))));

  if (has_selection ()) {

    try {

      trans_holder->open ();

      //  this dummy operation will update the screen:
      manager ()->queue (this, new db::Op ());

      for (iterator e = begin (); e != end (); ++e) {
        e->transform (tr);
      }

    } catch (...) {
      trans_holder->cancel ();
      throw;
    }

  }
}

void 
Editables::paste ()
{
  if (! db::Clipboard::instance ().empty ()) {

    cancel_edits ();

    //  this dummy operation will update the screen:
    if (manager ()->transacting ()) {
      manager ()->queue (this, new db::Op ());
    }

    for (iterator e = begin (); e != end (); ++e) {
      e->paste ();
    }

  }
}

void 
Editables::enable (lay::Editable *obj, bool en)
{
  if (en) {
    m_enabled.insert (obj);
  } else {
    cancel_edits ();
    obj->select (db::DBox (), lay::Editable::Reset);  //  clear selection
    m_enabled.erase (obj);
  }
}

void 
Editables::transient_select (const db::DPoint &pt)
{
  bool same_point = (m_last_selected_point.is_point () && m_last_selected_point.center ().sq_double_distance (pt) < 1e-10);
  if (! same_point) {
    clear_previous_selection ();
  }

  //  in a first pass evaluate the point selection proximity value to select
  //  those plugins that are active. This code is a copy of the code for the single-point selection below.

  std::vector< std::pair <double, iterator> > plugins;

  for (iterator e = begin (); e != end (); ++e) {
    if (m_enabled.find (&*e) != m_enabled.end ()) {
      plugins.push_back (std::make_pair (e->click_proximity (pt, lay::Editable::Replace), e));
    }
  }

  //  sort the plugins found by the proximity
  std::sort (plugins.begin (), plugins.end (), first_of_pair_cmp_f<double, iterator> ());

  //  and call select on those objects until the first one (with the least proximity)
  //  has something selected
  std::vector< std::pair<double, iterator> >::const_iterator pi = plugins.begin (); 
  for ( ; pi != plugins.end (); ++pi) {
    if (pi->second->transient_select (pt)) {
      break;
    }
  }

  //  if no plugin has selected anything, try a reset (clear previous selection) and select again: this is supposed 
  //  to implement the cycling protocol which allows the plugins to cycle through different
  //  selections for repeated clicks on the same point. Let this happen for replace mode 
  //  only because otherwise clearing the selection does not make sense.
  if (same_point && pi == plugins.end ()) {

    clear_previous_selection ();

    plugins.clear ();

    for (iterator e = begin (); e != end (); ++e) {
      if (m_enabled.find (&*e) != m_enabled.end ()) {
        plugins.push_back (std::make_pair (e->click_proximity (pt, lay::Editable::Replace), e));
      }
    }

    //  sort the plugins found by the proximity
    std::sort (plugins.begin (), plugins.end (), first_of_pair_cmp_f<double, iterator> ());

    for (pi = plugins.begin (); pi != plugins.end (); ++pi) {
      if (pi->second->transient_select (pt)) {
        break;
      }
    }

  }

  m_last_selected_point = db::DBox (pt, pt);

  //  send a signal to the observers
  signal_transient_selection_changed ();
}

void 
Editables::clear_previous_selection ()
{
  m_last_selected_point = db::DBox ();
  for (iterator e = begin (); e != end (); ++e) {
    e->clear_previous_selection ();
  }
}

void 
Editables::clear_transient_selection ()
{
  bool had_transient_selection = false;
  for (iterator e = begin (); e != end (); ++e) {
    if (e->has_transient_selection ()) {
      had_transient_selection = true;
    }
    e->clear_transient_selection ();
  }

  //  send a signal to the observers
  if (had_transient_selection) {
    signal_transient_selection_changed ();
  }
}

void
Editables::transient_to_selection ()
{
  bool had_transient_selection = false;
  bool had_selection = false;
  cancel_edits ();
  for (iterator e = begin (); e != end (); ++e) {
    if (e->has_selection ()) {
      had_selection = true;
    }
    if (e->has_transient_selection ()) {
      had_transient_selection = true;
    }
    e->select (db::DBox (), lay::Editable::Reset);  //  clear selection
    e->clear_previous_selection ();
    e->transient_to_selection ();
    e->clear_transient_selection ();
  }

  //  send a signal to the observers
  if (had_transient_selection) {
    signal_transient_selection_changed ();
  }
  if (had_selection || had_transient_selection) {
    signal_selection_changed ();
  }
}

void 
Editables::clear_selection ()
{
  cancel_edits ();

  bool had_transient_selection = false;
  bool had_selection = false;

  for (iterator e = begin (); e != end (); ++e) {
    if (e->has_selection ()) {
      had_selection = true;
    }
    if (e->has_transient_selection ()) {
      had_transient_selection = true;
    }
    e->select (db::DBox (), lay::Editable::Reset);  //  clear selection
    e->clear_transient_selection ();
    e->clear_previous_selection ();
  }

  //  send a signal to the observers
  if (had_transient_selection) {
    signal_transient_selection_changed ();
  }
  if (had_selection) {
    signal_selection_changed ();
  }
}

void 
Editables::select ()
{
  cancel_edits ();
  clear_transient_selection ();
  clear_previous_selection ();

  for (iterator e = begin (); e != end (); ++e) {
    if (m_enabled.find (&*e) != m_enabled.end ()) {
      e->select (db::DBox (), lay::Editable::Replace);  //  select "all"
    }
  }

  //  send a signal to the observers
  signal_selection_changed ();
}

void 
Editables::select (const db::DBox &box, lay::Editable::SelectionMode mode)
{
  if (box.is_point ()) {
    select (box.center (), mode);
  } else {

    cancel_edits ();
    clear_transient_selection ();
    clear_previous_selection ();

    for (iterator e = begin (); e != end (); ++e) {
      if (m_enabled.find (&*e) != m_enabled.end ()) {
        e->select (box, mode); 
      }
    }

    //  send a signal to the observers
    signal_selection_changed ();

  }
}

void 
Editables::select (const db::DPoint &pt, lay::Editable::SelectionMode mode)
{
  bool same_point = (m_last_selected_point.is_point () && m_last_selected_point.center ().sq_double_distance (pt) < 1e-10);
  if (! same_point) {
    clear_previous_selection ();
  }

  cancel_edits ();
  clear_transient_selection ();

  //  in a first pass evaluate the point selection proximity value to select
  //  those plugins that are active.

  std::vector< std::pair <double, iterator> > plugins;

  for (iterator e = begin (); e != end (); ++e) {
    if (m_enabled.find (&*e) != m_enabled.end ()) {
      plugins.push_back (std::make_pair (e->click_proximity (pt, mode), e));
    }
  }

  //  sort the plugins found by the proximity
  std::sort (plugins.begin (), plugins.end (), first_of_pair_cmp_f<double, iterator> ());

  //  and call select on those objects until the first one (with the least proximity)
  //  has something selected
  std::vector< std::pair<double, iterator> >::const_iterator pi = plugins.begin (); 
  for ( ; pi != plugins.end (); ++pi) {
    if (pi->second->select (db::DBox (pt, pt), mode)) {
      break;
    }
  }

  //  if no plugin has selected anything, try a reset (clear_previous selection) and select again: this is supposed 
  //  to implement the cycling protocol which allows the plugins to cycle through different
  //  selections for repeated clicks on the same point. Let this happen for replace mode 
  //  only because otherwise clearing the selection does not make sense.
  if (same_point && pi == plugins.end () && mode == lay::Editable::Replace) {

    clear_previous_selection ();

    plugins.clear ();

    for (iterator e = begin (); e != end (); ++e) {
      if (m_enabled.find (&*e) != m_enabled.end ()) {
        plugins.push_back (std::make_pair (e->click_proximity (pt, mode), e));
      }
    }

    std::sort (plugins.begin (), plugins.end (), first_of_pair_cmp_f<double, iterator> ());

    for (pi = plugins.begin (); pi != plugins.end (); ++pi) {
      if (pi->second->select (db::DBox (pt, pt), mode)) {
        break;
      }
    }

  }

  //  in replace mode clear the selections on the following plugins
  if (mode == lay::Editable::Replace && pi != plugins.end ()) {
    for (++pi ; pi != plugins.end (); ++pi) {
      pi->second->select (db::DBox (), lay::Editable::Reset);
    }
  }

  m_last_selected_point = db::DBox (pt, pt);

  //  send a signal to the observers
  signal_selection_changed ();
}

void
Editables::repeat_selection (Editable::SelectionMode mode)
{
  if (m_last_selected_point.is_point ()) {
    select (m_last_selected_point, mode);
  }
}

bool 
Editables::begin_move (const db::DPoint &p, lay::angle_constraint_type ac)
{
  cancel_edits ();
  clear_previous_selection ();

  m_move_selection = false;
  m_any_move_operation = false;

  //  In a first pass evaluate the point selection proximity value to select
  //  those plugins that are close to the given point. This code is a copy of the code for the single-point selection below.
  std::vector< std::pair <double, iterator> > plugins;

  for (iterator e = begin (); e != end (); ++e) {
    if (m_enabled.find (&*e) != m_enabled.end ()) {
      plugins.push_back (std::make_pair (e->click_proximity (p, lay::Editable::Replace), e));
    }
  }

  //  sort the plugins found by the proximity
  std::sort (plugins.begin (), plugins.end (), first_of_pair_cmp_f<double, iterator> ());

  if (has_selection () && selection_catch_bbox ().contains (p)) {

    //  if anything is selected and we are within the selection bbox, 
    //  issue a move operation on all editables: first try a Partial mode begin_move
    for (std::vector< std::pair<double, iterator> >::const_iterator pi = plugins.begin (); pi != plugins.end (); ++pi) {

      if (pi->second->begin_move (Editable::Partial, p, ac)) {

        //  clear the selection on all other plugins, because we focus on one plugin
        for (std::vector< std::pair<double, iterator> >::const_iterator pii = plugins.begin (); pii != plugins.end (); ++pii) {
          if (pii->second != pi->second) {
            pii->second->select (db::DBox (), lay::Editable::Reset);
          }
        }

        return true;

      }

    }

    //  if something is selected, issue a move operation on all editables
    for (iterator e = begin (); e != end (); ++e) {
      e->begin_move (Editable::Selected, p, ac);
    }

    return true;

  } else {

    //  don't move the selection - clear the existing one first
    clear_selection ();

    //  if nothing is selected, issue a move operation on all editables
    std::vector< std::pair<double, iterator> >::const_iterator pi = plugins.begin ();
    //  HACK: only the first plugin (measured through click_proximity) has a chance to intercept
    //  the standard procedure which is select + move selected. The reason behind that is that
    //  for example edtService does not implement Any but rather relies on select + Selected mode.
    //  Hence allowing some "later" plugin to override it at this point would break the least
    //  proximity priority rule.
    if (pi != plugins.end () && pi->second->begin_move (Editable::Any, p, ac)) {
      return true;
    }

    //  nothing particular selected - select ourselves and start over
    select (p, Editable::Replace);

    //  now we assume we have a selection - try to begin_move on this.
    if (has_selection ()) {
      m_move_selection = true;
      for (iterator e = begin (); e != end (); ++e) {
        e->begin_move (Editable::Selected, p, ac);
      }
      return true;
    } else {
      return false;
    }

  }
}

void 
Editables::move (const db::DPoint &p, lay::angle_constraint_type ac)
{
  m_any_move_operation = true;
  for (iterator e = begin (); e != end (); ++e) {
    e->move (p, ac);
  }
}

void 
Editables::move_transform (const db::DPoint &p, db::DFTrans t, lay::angle_constraint_type ac)
{
  m_any_move_operation = true;
  for (iterator e = begin (); e != end (); ++e) {
    e->move_transform (p, t, ac);
  }
}

void 
Editables::end_move (const db::DPoint &p, lay::angle_constraint_type ac, db::Transaction *transaction)
{
  std::unique_ptr<db::Transaction> trans_holder (transaction ? transaction : new db::Transaction (manager (), tl::to_string (QObject::tr ("Move"))));

  if (m_any_move_operation) {

    trans_holder->open ();

    //  this dummy operation will update the screen:
    manager ()->queue (this, new db::Op ());

    for (iterator e = begin (); e != end (); ++e) {
      e->end_move (p, ac);
    }

    //  clear the selection that was set previously
    if (m_move_selection) {
      clear_selection ();
    }

  } else {

    trans_holder->cancel ();

    //  if nothing was moved, treat the end_move as a select which makes the move sticky or
    //  replaces a complex selection by a simple one
    edit_cancel ();
    select (p, Editable::Replace);

  }
}

size_t 
Editables::selection_size ()
{
  size_t c = 0;
  for (iterator e = begin (); e != end (); ++e) {
    c += e->selection_size ();
  }
  return c;
}

bool
Editables::has_selection ()
{
  for (iterator e = begin (); e != end (); ++e) {
    if (e->has_selection ()) {
      return true;
    }
  }
  return false;
}

void
Editables::edit_cancel ()
{
  clear_previous_selection ();
  for (iterator e = begin (); e != end (); ++e) {
    e->edit_cancel ();
  }
}

void
Editables::cancel_edits ()
{
  //  close the property dialog
  if (mp_properties_dialog) {
    mp_properties_dialog->hide ();
  }

  //  cancel any edit operations
  for (iterator e = begin (); e != end (); ++e) {
    e->edit_cancel ();
  }
}

void
Editables::show_properties (QWidget *parent)
{
  if (! has_selection ()) {
    //  try to use the transient selection for the real one
    transient_to_selection ();
  }

  //  re-create a new properties dialog
  if (mp_properties_dialog) {
    delete mp_properties_dialog;
  }
  mp_properties_dialog = new lay::PropertiesDialog (parent, manager (), this);
  mp_properties_dialog->show ();
}

}

