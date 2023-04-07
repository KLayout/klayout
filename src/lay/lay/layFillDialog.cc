
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


#include "layFillDialog.h"

#include "dbClip.h"
#include "dbEdgeProcessor.h"
#include "dbRecursiveShapeIterator.h"
#include "dbPolygonTools.h"
#include "dbFillTool.h"
#include "antService.h"
#include "tlException.h"
#include "tlString.h"
#include "tlExceptions.h"
#include "layMainWindow.h"
#include "layCellSelectionForm.h"
#include "layUtils.h"
#include "edtService.h"

namespace lay
{

// ------------------------------------------------------------
//  Declaration of the configuration options

class FillDialogPluginDeclaration
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
    menu_entries.push_back (lay::menu_item ("fill_tool::show", "fill_tool:edit_mode", "edit_menu.utils_menu.end", tl::to_string (QObject::tr ("Fill Tool"))));
  }
 
  virtual lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *, lay::LayoutViewBase *view) const
  {
    if (lay::has_gui ()) {
      return new FillDialog (lay::MainWindow::instance (), view);
    } else {
      return 0;
    }
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new FillDialogPluginDeclaration (), 20000, "FillDialogPlugin");


// ------------------------------------------------------------

FillDialog::FillDialog (QWidget *parent, LayoutViewBase *view)
  : QDialog (parent),
    lay::Plugin (view),
    Ui::FillDialog (),
    mp_view (view)
{
  setObjectName (QString::fromUtf8 ("fill_dialog"));

  Ui::FillDialog::setupUi (this);

  fc_boundary_layer->set_no_layer_available (true);

  fill_area_stack->setCurrentIndex (0);
  connect (fill_area_cbx, SIGNAL (currentIndexChanged (int)), this, SLOT (fill_area_changed (int)));
  connect (button_box, SIGNAL (accepted ()), this, SLOT (ok_pressed ()));
  connect (choose_fc_pb, SIGNAL (clicked ()), this, SLOT (choose_fc ()));
  connect (choose_fc_2nd_pb, SIGNAL (clicked ()), this, SLOT (choose_fc_2nd ()));
}

void 
FillDialog::menu_activated (const std::string &symbol)
{
  if (symbol == "fill_tool::show") {

    int cv_index = mp_view->active_cellview_index ();

    lay::CellView cv = mp_view->cellview (cv_index);
    if (cv.is_valid ()) {
      cb_layer->set_view (mp_view, cv_index);
      fc_boundary_layer->set_view (mp_view, cv_index);
      show ();
    }

  } else {
    lay::Plugin::menu_activated (symbol);
  }
}

FillDialog::~FillDialog ()
{
  //  .. nothing yet ..
}

void 
FillDialog::choose_fc ()
{
  CellSelectionForm form (this, mp_view, "browse_cell", true /*simple mode*/);
  if (form.exec ()) {
    const lay::CellView &cv = form.selected_cellview ();
    fill_cell_le->setText (tl::to_qstring (cv->layout ().cell_name (cv.cell_index ())));
  }
}

void 
FillDialog::choose_fc_2nd ()
{
  CellSelectionForm form (this, mp_view, "browse_cell", true /*simple mode*/);
  if (form.exec ()) {
    const lay::CellView &cv = form.selected_cellview ();
    fill_cell_2nd_le->setText (tl::to_qstring (cv->layout ().cell_name (cv.cell_index ())));
  }
}

void
FillDialog::generate_fill (const FillParameters &fp)
{
  if (tl::verbosity () >= 10) {
    tl::info << "Running fill";
  }

  lay::CellView cv = mp_view->cellview (mp_view->active_cellview_index ());
  db::Layout &ly = cv->layout ();

  std::vector <unsigned int> exclude_layers;

  if (fp.exclude_all_layers) {
    //  all layers
    for (db::Layout::layer_iterator l = ly.begin_layers (); l != ly.end_layers (); ++l) {
      exclude_layers.push_back ((*l).first);
    }
  } else {
    //  some layers
    for (std::vector<db::LayerProperties>::const_iterator l = fp.exclude_layers.begin (); l != fp.exclude_layers.end (); ++l) {
      exclude_layers.push_back (ly.get_layer (*l));
    }
  }

  bool enhanced_fill = enhanced_cb->isChecked ();

  db::Coord exclude_x = db::coord_traits<db::Coord>::rounded (fp.exclude_distance.x () / ly.dbu ());
  db::Coord exclude_y = db::coord_traits<db::Coord>::rounded (fp.exclude_distance.y () / ly.dbu ());

  db::Coord distance_x = db::coord_traits<db::Coord>::rounded (fp.border_distance.x () / ly.dbu ());
  db::Coord distance_y = db::coord_traits<db::Coord>::rounded (fp.border_distance.y () / ly.dbu ());

  db::Vector fill_margin = db::CplxTrans (ly.dbu ()).inverted () * fp.fill_cell_margin;
  db::Vector fill_margin2 = db::CplxTrans (ly.dbu ()).inverted () * fp.fill_cell_margin2;

  std::pair<bool, db::cell_index_type> fc = cv->layout ().cell_by_name (fp.fill_cell_name.c_str ());
  if (! fc.first) {
    throw tl::Exception (tl::to_string (QObject::tr ("Fill cell not found: ")) + fp.fill_cell_name);
  }

  const db::Cell *fill_cell = &ly.cell (fc.second);

  const db::Cell *fill_cell2 = 0;
  if (! fp.fill_cell_name2.empty ()) {
    std::pair<bool, db::cell_index_type> fc2 = cv->layout ().cell_by_name (fp.fill_cell_name2.c_str ());
    if (! fc2.first) {
      throw tl::Exception (tl::to_string (QObject::tr ("Secondary fill cell not found: ")) + fp.fill_cell_name2);
    }
    fill_cell2 = &ly.cell (fc2.second);
  }

  db::Vector row_step = db::CplxTrans (ly.dbu ()).inverted () * fp.row_step;
  db::Vector column_step = db::CplxTrans (ly.dbu ()).inverted () * fp.column_step;
  db::Box fc_bbox = db::CplxTrans (ly.dbu ()).inverted () * fp.fc_bbox;

  db::Vector row_step2 = db::CplxTrans (ly.dbu ()).inverted () * fp.row_step2;
  db::Vector column_step2 = db::CplxTrans (ly.dbu ()).inverted () * fp.column_step2;
  db::Box fc_bbox2 = db::CplxTrans (ly.dbu ()).inverted () * fp.fc_bbox2;


  if (tl::verbosity () >= 20) {
    tl::info << "Collecting fill regions";
  }

  db::Region fill_region;
  if (fp.fill_region_mode == FillParameters::Region) {
    fill_region = fp.fill_region;
  } else if (fp.fill_region_mode == FillParameters::WholeCell) {
    fill_region.insert (cv->layout ().cell (cv.cell_index ()).bbox ());
  } else if (fp.fill_region_mode == FillParameters::Layer) {
    unsigned int layer_index = cv->layout ().get_layer (fp.fill_region_layer);
    fill_region = db::Region (db::RecursiveShapeIterator (cv->layout (), *cv.cell (), layer_index));
  }

  fill_region.enable_progress (tl::to_string (tr ("Computing fill region")));

  if (! fill_region.empty ()) {

    db::EdgeProcessor ep;

    if (tl::verbosity () >= 20) {
      tl::info << "Preprocessing fill regions";
    }

    //  preprocess fill regions
    if (distance_x != 0 || distance_y != 0) {
      fill_region.size (-distance_x, -distance_y);
    } else {
      fill_region.merge ();
    }

    db::Box fr_bbox = fill_region.bbox ();

    if (tl::verbosity () >= 20) {
      tl::info << "Collecting exclude areas";
    }

    //  collect sized shapes from the exclude layers
    db::Region es;
    es.enable_progress (tl::to_string (tr ("Preparing exclude layers")));
    for (std::vector <unsigned int>::const_iterator l = exclude_layers.begin (); l != exclude_layers.end (); ++l) {

      db::Region exclude (db::RecursiveShapeIterator (cv->layout (), *cv.cell (), *l));
      exclude.enable_progress (tl::to_string (tr ("Preparing exclude layer: ")) + cv->layout ().get_properties (*l).to_string ());

      if (exclude_x != 0 || exclude_y != 0) {
        exclude.size (exclude_x, exclude_y);
      } else {
        exclude.merge ();
      }

      es += exclude;

    }

    if (tl::verbosity () >= 20) {
      tl::info << "Computing effective fill region";
    }

    //  Perform the NOT operation to create the fill region
    fill_region -= es;

    db::Region new_fill_area;

    int step = 0;

    do {

      ++step;

      if (tl::verbosity () >= 20) {
        tl::info << "Major iteration (primary/secondary fill cell)";
      }

      if (! enhanced_fill) {
        db::fill_region (cv.cell (), fill_region, fill_cell->cell_index (), fc_bbox, row_step, column_step, fr_bbox.p1 (), false, fill_cell2 ? &fill_region : 0, fill_margin, fill_cell2 ? &fill_region : 0);
      } else {
        db::fill_region_repeat (cv.cell (), fill_region, fill_cell->cell_index (), fc_bbox, row_step, column_step, fill_margin, fill_cell2 ? &fill_region : 0);
      }

      fill_cell = fill_cell2;
      row_step = row_step2;
      column_step = column_step2;
      fc_bbox = fc_bbox2;
      fill_margin = fill_margin2;

      fill_cell2 = 0;

    } while (fill_cell != 0 && ! fill_region.empty ());

  }

  if (tl::verbosity () >= 20) {
    tl::info << "Fill done";
  }
}

FillParameters
FillDialog::get_fill_parameters ()
{
  FillParameters fp;

  lay::CellView cv = mp_view->cellview (mp_view->active_cellview_index ());

  fp.exclude_all_layers = false;

  if (layer_spec_cbx->currentIndex () == 0) {

    fp.exclude_all_layers = true;

  } else if (layer_spec_cbx->currentIndex () == 1) {

    //  visible layers
    for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); ! l.at_end (); ++l) {
      if (! l->has_children () && l->visible (true) && cv->layout ().is_valid_layer (l->layer_index ())) {
        fp.exclude_layers.push_back (cv->layout ().get_properties (l->layer_index ()));
      }
    }

  } else if (layer_spec_cbx->currentIndex () == 2) {

    //  get selected layers
    std::vector<lay::LayerPropertiesConstIterator> s = mp_view->selected_layers ();

    for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = s.begin (); l != s.end (); ++l) {
      if (! (*l)->has_children () && cv->layout ().is_valid_layer ((*l)->layer_index ())) {
        fp.exclude_layers.push_back (cv->layout ().get_properties ((*l)->layer_index ()));
      }
    }

  }

  //  read exclude spacing
  double x = 0.0, y = 0.0;
  std::string s (tl::to_string (exclude_le->text ()));
  tl::Extractor ex (s.c_str ());
  if (ex.try_read (x)) {
    if (ex.test (",") && ex.try_read (y)) {
      // take x, y
    } else {
      y = x;
    }
  }

  fp.exclude_distance = db::DVector (x, y);

  //  get the fill regions

  if (fill_area_cbx->currentIndex () == 3) {

    fp.fill_region_mode = FillParameters::Region;

    //  explicit fill box

    if (le_x1->text ().isEmpty () || le_x2->text ().isEmpty () ||
        le_y1->text ().isEmpty () || le_y2->text ().isEmpty ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("All four coordinates of the fill box must be given")));
    }

    double x1 = 0.0, y1 = 0.0;
    double x2 = 0.0, y2 = 0.0;
    tl::from_string_ext (tl::to_string (le_x1->text ()), x1);
    tl::from_string_ext (tl::to_string (le_x2->text ()), x2);
    tl::from_string_ext (tl::to_string (le_y1->text ()), y1);
    tl::from_string_ext (tl::to_string (le_y2->text ()), y2);

    fp.fill_region.insert (db::Box (db::DBox (db::DPoint (x1, y1), db::DPoint (x2, y2)) * (1.0 / cv->layout ().dbu ())));

  } else if (fill_area_cbx->currentIndex () == 4) {

    fp.fill_region_mode = FillParameters::Region;

    //  ruler

    ant::Service *ant_service = mp_view->get_plugin <ant::Service> ();
    if (ant_service) {
      ant::AnnotationIterator ant = ant_service->begin_annotations ();
      while (! ant.at_end ()) {
        fp.fill_region.insert (db::Box (db::DBox (ant->p1 (), ant->p2 ()) * (1.0 / cv->layout ().dbu ())));
        ++ant;
      }
    }

  } else if (fill_area_cbx->currentIndex () == 1) {

    fp.fill_region_mode = FillParameters::Layer;

    //  specified layer

    int sel_layer = cb_layer->current_layer ();
    if (sel_layer < 0 || ! cv->layout ().is_valid_layer (sel_layer)) {
      throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected to get fill regions from")));
    }

    fp.fill_region_layer = cv->layout ().get_properties (sel_layer);

  } else if (fill_area_cbx->currentIndex () == 0) {

    fp.fill_region_mode = FillParameters::WholeCell;

  } else if (fill_area_cbx->currentIndex () == 2) {

    fp.fill_region_mode = FillParameters::Region;

    //  selection
    std::vector<edt::Service *> edt_services = mp_view->get_plugins <edt::Service> ();
    for (std::vector<edt::Service *>::const_iterator s = edt_services.begin (); s != edt_services.end (); ++s) {
      for (edt::Service::objects::const_iterator sel = (*s)->selection ().begin (); sel != (*s)->selection ().end (); ++sel) {
        if (! sel->is_cell_inst () && (sel->shape ().is_polygon () || sel->shape ().is_path () || sel->shape ().is_box ())) {
          db::Polygon poly;
          sel->shape ().polygon (poly);
          fp.fill_region.insert (poly);
        }
      }
    }

  }

  //  read distance to border
  x = 0.0, y = 0.0;
  s = tl::to_string (distance_le->text ());
  ex = tl::Extractor (s.c_str ());
  if (ex.try_read (x)) {
    if (ex.test (",") && ex.try_read (y)) {
      // take x, y
    } else {
      y = x;
    }
  }

  fp.border_distance = db::DVector (x, y);

  //  read fill cell margin
  x = 0.0, y = 0.0;
  s = tl::to_string (fill_margin_le->text ());
  ex = tl::Extractor (s.c_str ());
  if (ex.try_read (x)) {
    if (ex.test (",") && ex.try_read (y)) {
      // take x, y
    } else {
      y = x;
    }
  }

  fp.fill_cell_margin = db::DVector (x, y);

  //  read fill cell 2 margin
  x = 0.0, y = 0.0;
  s = tl::to_string (fill2_margin_le->text ());
  ex = tl::Extractor (s.c_str ());
  if (ex.try_read (x)) {
    if (ex.test (",") && ex.try_read (y)) {
      // take x, y
    } else {
      y = x;
    }
  }

  fp.fill_cell_margin2 = db::DVector (x, y);

  fp.fill_cell_name = tl::to_string (fill_cell_le->text ());

  //  get the fill cell
  std::pair<bool, db::cell_index_type> fc = cv->layout ().cell_by_name (fp.fill_cell_name.c_str ());
  if (! fc.first) {
    throw tl::Exception (tl::to_string (QObject::tr ("Fill cell not found: ")) + tl::to_string (fill_cell_le->text ()));
  }

  const db::Cell *fill_cell = &cv->layout ().cell (fc.second);

  int fc_bbox_layer = fc_boundary_layer->current_layer ();
  if (fc_bbox_layer >= 0 && ! cv->layout ().is_valid_layer (fc_bbox_layer)) {
    throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected to get fill cell's bounding box from")));
  }

  fp.enhanced_fill = enhanced_cb->isChecked ();

  db::DBox fc_bbox = db::CplxTrans (cv->layout ().dbu ()) * (fc_bbox_layer < 0 ? fill_cell->bbox () : fill_cell->bbox (fc_bbox_layer));
  if (fc_bbox.empty ()) {
    if (fc_bbox_layer >= 0) {
      throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected to get fill cell's bounding box from - layer is empty for the fill cell")));
    } else {
      throw tl::Exception (tl::to_string (QObject::tr ("Fill cell is empty")));
    }
  }

  s = tl::to_string (row_le->text ());
  ex = tl::Extractor (s.c_str ());
  if (ex.try_read (x) && ex.test (",") && ex.try_read (y)) {
    fp.row_step = db::DVector (x, y);
  } else {
    fp.row_step = db::DVector (fc_bbox.width (), 0.0);
  }

  s = tl::to_string (column_le->text ());
  ex = tl::Extractor (s.c_str ());
  if (ex.try_read (x) && ex.test (",") && ex.try_read (y)) {
    fp.column_step = db::DVector (x, y);
  } else {
    fp.column_step = db::DVector (0.0, fc_bbox.height ());
  }

  fp.fc_bbox = fc_bbox;

  if (second_order_fill_cb->isChecked ()) {

    db::DBox fc_bbox2;

    fp.fill_cell_name2 = tl::to_string (fill_cell_2nd_le->text ());

    std::pair<bool, db::cell_index_type> fc = cv->layout ().cell_by_name (fp.fill_cell_name2.c_str ());
    if (! fc.first) {
      throw tl::Exception (tl::to_string (QObject::tr ("Second order fill cell not found: ")) + tl::to_string (fill_cell_2nd_le->text ()));
    }

    const db::Cell *fill_cell2 = &cv->layout ().cell (fc.second);

    fc_bbox2 = db::CplxTrans (cv->layout ().dbu ()) * (fc_bbox_layer < 0 ? fill_cell2->bbox () : fill_cell2->bbox (fc_bbox_layer));
    if (fc_bbox2.empty ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("Second order fill cell is empty for the given boundary layer")));
    }

    s = tl::to_string (row_2nd_le->text ());
    ex = tl::Extractor (s.c_str ());
    if (ex.try_read (x) && ex.test (",") && ex.try_read (y)) {
      fp.row_step2 = db::DVector (x, y);
    } else {
      fp.row_step2 = db::DVector (fc_bbox2.width (), 0.0);
    }

    s = tl::to_string (column_2nd_le->text ());
    ex = tl::Extractor (s.c_str ());
    if (ex.try_read (x) && ex.test (",") && ex.try_read (y)) {
      fp.column_step2 = db::DVector (x, y);
    } else {
      fp.column_step2 = db::DVector (0.0, fc_bbox2.height ());
    }

    fp.fc_bbox2 = fc_bbox2;

  }

  return fp;
}

void 
FillDialog::ok_pressed ()
{
BEGIN_PROTECTED

  FillParameters fp = get_fill_parameters ();

  mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Fill")));

  try {
    generate_fill (fp);
    mp_view->manager ()->commit ();
  } catch (...) {
    mp_view->manager ()->cancel ();
    throw;
  }

  //  close this dialog
  QDialog::accept ();

END_PROTECTED
}

void 
FillDialog::fill_area_changed (int fa)
{
  if (fa == 1) {
    fill_area_stack->setCurrentIndex (1);
  } else if (fa == 3) {
    fill_area_stack->setCurrentIndex (2);
  } else {
    fill_area_stack->setCurrentIndex (0);
  }
}

bool 
FillDialog::configure (const std::string & /*name*/, const std::string & /*value*/)
{
  //  .. nothing yet ..
  return false;
}

}


