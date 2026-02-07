
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "edtTextService.h"

#include "layLayoutViewBase.h"
#include "layConverters.h"
#include "layEditorOptionsPage.h"

#if defined(HAVE_QT)
#  include "edtPropertiesPages.h"
#  include "layTipDialog.h"
#endif

namespace edt
{

// -----------------------------------------------------------------------------
//  TextService implementation

TextService::TextService (db::Manager *manager, lay::LayoutViewBase *view)
  : ShapeEditService (manager, view, db::ShapeIterator::Texts),
    m_rot (0)
{ 
  //  .. nothing yet ..
}

TextService::~TextService ()
{
  //  .. nothing yet ..
}

#if defined(HAVE_QT)
std::vector<lay::PropertiesPage *>
TextService::properties_pages (db::Manager *manager, QWidget *parent)
{
  std::vector<lay::PropertiesPage *> pages;
  pages.push_back (new edt::TextPropertiesPage (this, manager, parent));
  return pages;
}
#endif

void 
TextService::do_begin_edit (const db::DPoint &p)
{
  get_edit_layer ();

  m_text.trans (db::DTrans (m_rot, snap2 (p) - db::DPoint ()));

  open_editor_hooks ();

  lay::DMarker *marker = new lay::DMarker (view ());
  marker->set_vertex_shape (lay::ViewOp::Cross);
  marker->set_vertex_size (9 /*cross vertex size*/);
  set_edit_marker (marker);
  update_marker ();
}

void
TextService::update_marker ()
{
  lay::DMarker *marker = dynamic_cast<lay::DMarker *> (edit_marker ());
  if (marker) {

    marker->set (m_text);

    std::string pos = std::string ("x: ") +
                      tl::micron_to_string (m_text.trans ().disp ().x ()) + 
                      std::string ("  y: ") +
                      tl::micron_to_string (m_text.trans ().disp ().y ());
    if (m_text.trans ().rot () != 0) {
      pos += std::string ("  ") + ((const db::DFTrans &) m_text.trans ()).to_string ();
    }

    view ()->message (pos);

  }

  //  call hooks with new shape
  if (! editor_hooks ().empty ()) {
    call_editor_hooks (editor_hooks (), &edt::EditorHooks::begin_new_shapes);
    try {
      deliver_shape_to_hooks (get_text ());
    } catch (...) {
      //  ignore exceptions
    }
    call_editor_hooks (editor_hooks (), &edt::EditorHooks::end_new_shapes);
  }
}

bool
TextService::do_activated ()
{
  m_rot = 0;

  return true;  //  start editing immediately
}

void
TextService::do_mouse_move_inactive (const db::DPoint &p)
{
  lay::PointSnapToObjectResult snap_details = snap2_details (p);
  mouse_cursor_from_snap_details (snap_details);
}

void
TextService::do_mouse_move (const db::DPoint &p)
{
  do_mouse_move_inactive (p);

  set_cursor (lay::Cursor::cross);
  m_text.trans (db::DTrans (m_rot, snap2 (p) - db::DPoint ()));
  update_marker ();
}

void 
TextService::do_mouse_transform (const db::DPoint &p, db::DFTrans trans)
{
  m_rot = (db::DFTrans (m_rot) * trans).rot ();
  m_text.trans (db::DTrans (m_rot, p - db::DPoint ()));
  update_marker ();
}

bool 
TextService::do_mouse_click (const db::DPoint &p)
{
  do_mouse_move (p);
  return true;
}

db::Text
TextService::get_text () const
{
  db::Point p_dbu = trans () * (db::DPoint () + m_text.trans ().disp ());
  return db::Text (m_text.string (), db::Trans (m_text.trans ().rot (), p_dbu - db::Point ()), db::coord_traits<db::Coord>::rounded (trans ().ctrans (m_text.size ())), db::NoFont, m_text.halign (), m_text.valign ());
}

void 
TextService::do_finish_edit (bool /*accept*/)
{
  {
    db::Transaction transaction (manager (), tl::to_string (tr ("Create text")));
    cell ().shapes (layer ()).insert (get_text ());
  }

  commit_recent ();

#if defined(HAVE_QT)
  if (! view ()->text_visible ()) {

    lay::TipDialog td (QApplication::activeWindow (),
                       tl::to_string (tr ("A text object is created but texts are disabled for drawing and are not visible. Do you want to enable drawing of texts?\n\nChoose \"Yes\" to enable text drawing now.")),
                       "text-created-but-not-visible",
                       lay::TipDialog::yesno_buttons);

    lay::TipDialog::button_type button = lay::TipDialog::null_button;
    td.exec_dialog (button);
    if (button == lay::TipDialog::yes_button) {
      view ()->text_visible (true);
    }

  }
#endif

  close_editor_hooks (true);
}

void 
TextService::do_cancel_edit ()
{
  close_editor_hooks (false);
}

bool 
TextService::selection_applies (const lay::ObjectInstPath &sel) const
{
  return !sel.is_cell_inst () && sel.shape ().is_text ();
}

bool 
TextService::configure (const std::string &name, const std::string &value)
{
  auto tb = toolbox_widget ();
  if (tb) {
    tb->configure (name, value);
  }

  if (name == cfg_edit_text_size) {
    double size (0);
    tl::from_string (value, size);
    if (m_text.size () != size) {
      m_text.size (size);
      update_marker ();
    }
    return true; // taken
  }

  if (name == cfg_edit_text_halign) {
    db::HAlign ha = db::HAlignLeft;
    lay::HAlignConverter hac;
    hac.from_string (value, ha);
    if (m_text.halign () != ha) {
      m_text.halign (ha);
      update_marker ();
    }
    return true; // taken
  }

  if (name == cfg_edit_text_valign) {
    db::VAlign va = db::VAlignBottom;
    lay::VAlignConverter vac;
    vac.from_string (value, va);
    if (m_text.valign () != va) {
      m_text.valign (va);
      update_marker ();
    }
    return true; // taken
  }

  if (name == cfg_edit_text_string) {
    if (m_text.string () != value) {
      m_text.string (value);
      update_marker ();
    }
    return true; // taken
  }

  return ShapeEditService::configure (name, value);
}

} // namespace edt
