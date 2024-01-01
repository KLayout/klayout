
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#include "layClipDialog.h"

#include "dbClip.h"
#include "antService.h"
#include "tlException.h"
#include "tlString.h"
#include "tlExceptions.h"
#include "layUtils.h"

namespace lay
{

// ------------------------------------------------------------
//  Declaration of the configuration options

class ClipDialogPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > & /*options*/) const
  {
    //  .. no options yet ..
  }

  virtual lay::ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    return 0; //  .. no config page yet ..
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::menu_item ("clip_tool::show", "clip_tool:edit_mode", "edit_menu.utils_menu.end", tl::to_string (QObject::tr ("Clip Tool"))));
  }
 
  virtual lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *root, lay::LayoutViewBase *view) const
  {
    if (lay::has_gui ()) {
      return new ClipDialog (root, view);
    } else {
      return 0;
    }
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new ClipDialogPluginDeclaration (), 20000, "ClipDialogPlugin");


// ------------------------------------------------------------

ClipDialog::ClipDialog (lay::Dispatcher *root, LayoutViewBase *vw)
  : lay::Browser (root, vw), 
    Ui::ClipDialog ()
{
  Ui::ClipDialog::setupUi (this);

  connect (rb_box1, SIGNAL (clicked ()), this, SLOT (box1_clicked ()));
  connect (rb_box2, SIGNAL (clicked ()), this, SLOT (box2_clicked ()));
  connect (rb_rulers, SIGNAL (clicked ()), this, SLOT (rulers_clicked ()));
  connect (rb_shapes, SIGNAL (clicked ()), this, SLOT (shapes_clicked ()));
  connect (button_box, SIGNAL (accepted ()), this, SLOT (ok_pressed ()));

  box1_clicked ();
}

void 
ClipDialog::menu_activated (const std::string &symbol)
{
  if (symbol == "clip_tool::show") {

    int cv_index = view ()->active_cellview_index ();

    lay::CellView cv = view ()->cellview (cv_index);
    if (cv.is_valid ()) {
      cb_layer->set_view (view (), cv_index);
      show ();
      activate ();
    }

  } else {
    lay::Browser::menu_activated (symbol);
  }
}

ClipDialog::~ClipDialog ()
{
  //  .. nothing yet ..
}

void 
ClipDialog::ok_pressed ()
{
BEGIN_PROTECTED

  std::string clip_cell_name (tl::to_string (le_cell_name->text ()));
  if (clip_cell_name.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Clip cell name must not be empty")));
  }

  std::vector <db::Box> clip_boxes;
  lay::CellView cv = view ()->cellview (view ()->active_cellview_index ());

  if (rb_box1->isChecked ()) {

    if (le_x1->text ().isEmpty () || le_x2->text ().isEmpty () ||
        le_y1->text ().isEmpty () || le_y2->text ().isEmpty ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("All four coordinates of the clip box must be given")));
    }

    double x1 = 0.0, y1 = 0.0;
    double x2 = 0.0, y2 = 0.0;
    tl::from_string_ext (tl::to_string (le_x1->text ()), x1);
    tl::from_string_ext (tl::to_string (le_x2->text ()), x2);
    tl::from_string_ext (tl::to_string (le_y1->text ()), y1);
    tl::from_string_ext (tl::to_string (le_y2->text ()), y2);

    clip_boxes.push_back (db::Box (db::DBox (db::DPoint (x1, y1), db::DPoint (x2, y2)) * (1.0 / cv->layout ().dbu ())));

  } else if (rb_box2->isChecked ()) {

    if (le_x->text ().isEmpty () || le_y->text ().isEmpty () ||
        le_w->text ().isEmpty () || le_h->text ().isEmpty ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("All four coordinates of the clip box must be given")));
    }

    double x = 0.0, y = 0.0;
    double w = 0.0, h = 0.0;
    tl::from_string_ext (tl::to_string (le_x->text ()), x);
    tl::from_string_ext (tl::to_string (le_y->text ()), y);
    tl::from_string_ext (tl::to_string (le_w->text ()), w);
    tl::from_string_ext (tl::to_string (le_h->text ()), h);

    clip_boxes.push_back (db::Box (db::DBox (db::DPoint (x - 0.5 * w, y - 0.5 * h), db::DPoint (x + 0.5 * w, y + 0.5 * h)) * (1.0 / cv->layout ().dbu ())));

  } else if (rb_rulers->isChecked ()) {

    ant::Service *ant_service = view ()->get_plugin <ant::Service> ();
    if (ant_service) {
      ant::AnnotationIterator ant = ant_service->begin_annotations ();
      while (! ant.at_end ()) {
        clip_boxes.push_back (db::Box (db::DBox (ant->p1 (), ant->p2 ()) * (1.0 / cv->layout ().dbu ())));
        ++ant;
      }
    }

  } else if (rb_shapes->isChecked ()) {

    int sel_layer = cb_layer->current_layer ();
    if (sel_layer < 0 || ! cv->layout ().is_valid_layer (sel_layer)) {
      throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected to get clip boxes from")));
    }

    db::collect_clip_boxes (cv->layout (), cv.cell_index (), (unsigned int) sel_layer, clip_boxes);

  }

  //  large scale operation - do not provide undo (TODO: warn about that?)
  view ()->manager ()->clear ();

  if (! clip_boxes.empty ()) {

    std::sort (clip_boxes.begin (), clip_boxes.end ());
    clip_boxes.erase (std::unique (clip_boxes.begin (), clip_boxes.end ()), clip_boxes.end ());

    std::vector<db::cell_index_type> new_cells = db::clip_layout (cv->layout (), cv->layout (), cv.cell_index (), clip_boxes, false);

    if (new_cells.size () > 1) {

      //  need to create a new master top cell
      db::cell_index_type clip_top = cv->layout ().add_cell (clip_cell_name.c_str ());
      db::Cell &clip_top_cell = cv->layout ().cell (clip_top);

      for (std::vector <db::cell_index_type>::const_iterator cc = new_cells.begin (); cc != new_cells.end (); ++cc) {
        clip_top_cell.insert (db::CellInstArray (db::CellInst (*cc), db::Trans ()));
      }

      //  select that cell as new cell
      view ()->select_cell (clip_top, view ()->active_cellview_index ());

    } else if (new_cells.size () > 0 && new_cells [0] != cv.cell_index ()) {

      //  it is sufficient to rename the new cell ..
      cv->layout ().rename_cell (new_cells [0], cv->layout ().uniquify_cell_name (clip_cell_name.c_str ()).c_str ());

      //  select that cell as new cell
      view ()->select_cell (new_cells [0], view ()->active_cellview_index ());

    } 

  }

  //  close this dialog
  QDialog::accept ();

END_PROTECTED
}

void 
ClipDialog::box1_clicked ()
{
  rb_box2->setChecked (false);
  rb_shapes->setChecked (false);
  rb_rulers->setChecked (false);
  cb_layer->setEnabled (false);
  grp_box1->setEnabled (true);
  grp_box2->setEnabled (false);
}

void 
ClipDialog::box2_clicked ()
{
  rb_box1->setChecked (false);
  rb_shapes->setChecked (false);
  rb_rulers->setChecked (false);
  cb_layer->setEnabled (false);
  grp_box1->setEnabled (false);
  grp_box2->setEnabled (true);
}

void 
ClipDialog::rulers_clicked ()
{
  rb_box1->setChecked (false);
  rb_box2->setChecked (false);
  rb_shapes->setChecked (false);
  cb_layer->setEnabled (false);
  grp_box1->setEnabled (false);
  grp_box2->setEnabled (false);
}

void 
ClipDialog::shapes_clicked ()
{
  rb_box1->setChecked (false);
  rb_box2->setChecked (false);
  rb_rulers->setChecked (false);
  cb_layer->setEnabled (true);
  grp_box1->setEnabled (false);
  grp_box2->setEnabled (false);
}

bool 
ClipDialog::configure (const std::string & /*name*/, const std::string & /*value*/)
{
  //  .. nothing yet ..
  return false;
}

}

