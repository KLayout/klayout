
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
#include "layMainWindow.h"
#include "tlExceptions.h"
#include "layCellSelectionForm.h"
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
    menu_entries.push_back (lay::MenuEntry ("fill_tool::show", "fill_tool:edit_mode", "edit_menu.utils_menu.end", tl::to_string (QObject::tr ("Fill Tool"))));
  }
 
   virtual lay::Plugin *create_plugin (db::Manager *, lay::PluginRoot *root, lay::LayoutView *view) const
   {
     return new FillDialog (root, view);
   }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new FillDialogPluginDeclaration (), 20000, "FillDialogPlugin");


// ------------------------------------------------------------

FillDialog::FillDialog (lay::PluginRoot *main, lay::LayoutView *view)
  : QDialog (view),
    lay::Plugin (main),
    Ui::FillDialog (),
    mp_view (view)
{
  setObjectName (QString::fromUtf8 ("fill_dialog"));

  Ui::FillDialog::setupUi (this);

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

    lay::CellView cv = mp_view->cellview (mp_view->active_cellview_index ());
    if (cv.is_valid ()) {
      cb_layer->set_layout (&cv->layout ());
      fc_boundary_layer->set_layout (&cv->layout ());
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

static void 
collect_fill_regions (const db::Layout &layout, 
                      db::cell_index_type cell_index, 
                      unsigned int layer, 
                      const db::CplxTrans &trans, 
                      std::vector <db::Polygon> &regions)
{
  const db::Cell &cell = layout.cell (cell_index);
  if (! cell.bbox (layer).empty ()) {

    //  any shapes to consider ..
    for (db::ShapeIterator sh = cell.shapes (layer).begin (db::ShapeIterator::Polygons | db::ShapeIterator::Paths | db::ShapeIterator::Boxes); ! sh.at_end (); ++sh) {
      regions.push_back (db::Polygon ());
      sh->polygon (regions.back ());
    }

    for (db::Cell::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {
      for (db::CellInstArray::iterator a = inst->cell_inst ().begin (); ! a.at_end (); ++a) {
        collect_fill_regions (layout, inst->cell_index (), layer, trans * inst->cell_inst ().complex_trans (*a), regions);
      }
    }

  }
}

void 
collect_fill_regions (const db::Layout &layout, 
                    db::cell_index_type cell_index, 
                    unsigned int layer, 
                    std::vector <db::Polygon> &regions)
{
  collect_fill_regions (layout, cell_index, layer, db::CplxTrans (), regions);
}

void 
FillDialog::ok_pressed ()
{
BEGIN_PROTECTED

  if (tl::verbosity () >= 10) {
    tl::info << "Running fill";
  }

  lay::CellView cv = mp_view->cellview (mp_view->active_cellview_index ());

  std::vector <unsigned int> exclude_layers;
  
  if (layer_spec_cbx->currentIndex () == 0) {

    //  all layers
    for (db::Layout::layer_iterator l = cv->layout ().begin_layers (); l != cv->layout ().end_layers (); ++l) {
      exclude_layers.push_back ((*l).first);
    }

  } else if (layer_spec_cbx->currentIndex () == 1) {

    //  visible layers
    for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); ! l.at_end (); ++l) {
      if (! l->has_children () && l->visible (true)) {
        exclude_layers.push_back (l->layer_index ());
      }
    }

  } else if (layer_spec_cbx->currentIndex () == 2) {

    //  selected layers
    std::vector<lay::LayerPropertiesConstIterator> s = mp_view->selected_layers ();
    for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = s.begin (); l != s.end (); ++l) {
      if (! (*l)->has_children ()) {
        exclude_layers.push_back ((*l)->layer_index ());
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

  db::Coord exclude_x = db::coord_traits<db::Coord>::rounded (x / cv->layout ().dbu ());
  db::Coord exclude_y = db::coord_traits<db::Coord>::rounded (y / cv->layout ().dbu ());

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

  db::Coord distance_x = db::coord_traits<db::Coord>::rounded (x / cv->layout ().dbu ());
  db::Coord distance_y = db::coord_traits<db::Coord>::rounded (y / cv->layout ().dbu ());

  //  read fill cell margin
  db::Vector fill_margin (exclude_x, exclude_y);
  x = 0.0, y = 0.0;
  s = tl::to_string (fill_margin_le->text ());
  ex = tl::Extractor (s.c_str ());
  if (ex.try_read (x)) {
    if (ex.test (",") && ex.try_read (y)) {
      // take x, y
    } else {
      y = x;
    }
    fill_margin = db::Vector (db::coord_traits<db::Coord>::rounded (x / cv->layout ().dbu ()), db::coord_traits<db::Coord>::rounded (y / cv->layout ().dbu ()));
  }

  //  read fill cell 2 margin
  db::Vector fill2_margin (exclude_x, exclude_y);
  x = 0.0, y = 0.0;
  s = tl::to_string (fill2_margin_le->text ());
  ex = tl::Extractor (s.c_str ());
  if (ex.try_read (x)) {
    if (ex.test (",") && ex.try_read (y)) {
      // take x, y
    } else {
      y = x;
    }
    fill2_margin = db::Vector (db::coord_traits<db::Coord>::rounded (x / cv->layout ().dbu ()), db::coord_traits<db::Coord>::rounded (y / cv->layout ().dbu ()));
  }

  //  get the fill cell
  std::pair<bool, db::cell_index_type> fc = cv->layout ().cell_by_name (tl::to_string (fill_cell_le->text ()).c_str ());
  if (! fc.first) {
    throw tl::Exception (tl::to_string (QObject::tr ("Fill cell not found: ")) + tl::to_string (fill_cell_le->text ()));
  }

  const db::Cell *fill_cell = &cv->layout ().cell (fc.second); 

  int fc_bbox_layer = fc_boundary_layer->current_layer ();
  if (fc_bbox_layer < 0 || ! cv->layout ().is_valid_layer (fc_bbox_layer)) {
    throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected to get fill cell's bounding box from")));
  }

  db::Box fc_bbox = fill_cell->bbox (fc_bbox_layer);
  if (fc_bbox.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected to get fill cell's bounding box from - layer is empty for the fill cell")));
  }

  bool enhanced_fill = enhanced_cb->isChecked ();

  const db::Cell *fill_cell2 = 0;
  db::Box fc_bbox2;
  
  if (second_order_fill_cb->isChecked ()) {

    std::pair<bool, db::cell_index_type> fc = cv->layout ().cell_by_name (tl::to_string (fill_cell_2nd_le->text ()).c_str ());
    if (! fc.first) {
      throw tl::Exception (tl::to_string (QObject::tr ("Second order fill cell not found: ")) + tl::to_string (fill_cell_2nd_le->text ()));
    }

    fill_cell2 = &cv->layout ().cell (fc.second); 

    fc_bbox2 = fill_cell2->bbox (fc_bbox_layer);
    if (fc_bbox2.empty ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("Second order fill cell is empty for the given boundary layer")));
    }

  }

  if (tl::verbosity () >= 20) {
    tl::info << "Collecting fill regions";
  }

  //  get the fill regions
  std::vector <db::Polygon> fill_regions;

  if (fill_area_cbx->currentIndex () == 3) {

    //  explicit fill box

    if (le_x1->text ().isEmpty () || le_x2->text ().isEmpty () ||
        le_y1->text ().isEmpty () || le_y2->text ().isEmpty ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("All four coordinates of the fill box must be given")));
    }

    double x1 = 0.0, y1 = 0.0;
    double x2 = 0.0, y2 = 0.0;
    tl::from_string (tl::to_string (le_x1->text ()), x1);
    tl::from_string (tl::to_string (le_x2->text ()), x2);
    tl::from_string (tl::to_string (le_y1->text ()), y1);
    tl::from_string (tl::to_string (le_y2->text ()), y2);

    fill_regions.push_back (db::Polygon (db::Box (db::DBox (db::DPoint (x1, y1), db::DPoint (x2, y2)) * (1.0 / cv->layout ().dbu ()))));

  } else if (fill_area_cbx->currentIndex () == 4) {

    //  ruler

    ant::Service *ant_service = mp_view->get_plugin <ant::Service> ();
    if (ant_service) {
      ant::AnnotationIterator ant = ant_service->begin_annotations ();
      while (! ant.at_end ()) {
        fill_regions.push_back (db::Polygon (db::Box (db::DBox (ant->p1 (), ant->p2 ()) * (1.0 / cv->layout ().dbu ()))));
        ++ant;
      }
    }

  } else if (fill_area_cbx->currentIndex () == 1) {

    //  specified layer

    int sel_layer = cb_layer->current_layer ();
    if (sel_layer < 0 || ! cv->layout ().is_valid_layer (sel_layer)) {
      throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected to get fill regions from")));
    }

    collect_fill_regions (cv->layout (), cv.cell_index (), (unsigned int) sel_layer, fill_regions);

  } else if (fill_area_cbx->currentIndex () == 0) {

    //  whole cell
    fill_regions.push_back (db::Polygon (cv.cell ()->bbox ()));

  } else if (fill_area_cbx->currentIndex () == 2) {

    //  selection
    std::vector<edt::Service *> edt_services = mp_view->get_plugins <edt::Service> ();
    for (std::vector<edt::Service *>::const_iterator s = edt_services.begin (); s != edt_services.end (); ++s) {
      for (edt::Service::objects::const_iterator sel = (*s)->selection ().begin (); sel != (*s)->selection ().end (); ++sel) {
        if (! sel->is_cell_inst () && (sel->shape ().is_polygon () || sel->shape ().is_path () || sel->shape ().is_box ())) {
          fill_regions.push_back (db::Polygon ());
          sel->shape ().polygon (fill_regions.back ());
        }
      }
    }

  }

  mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Fill")));

  if (! fill_regions.empty ()) {

    db::EdgeProcessor ep;
    
    if (tl::verbosity () >= 20) {
      tl::info << "Preprocessing fill regions";
    }

    //  TODO: progress
    
    //  preprocess fill regions
    if (distance_x != 0 || distance_y != 0) {

      std::vector <db::Polygon> fp;
      ep.enable_progress (tl::to_string (QObject::tr ("Preparing fill regions")));
      ep.size (fill_regions, -distance_x, -distance_y, fp, 2 /*mode*/, false /*=don't resolve holes*/);
      ep.disable_progress ();

      fill_regions.swap (fp);

    }

    std::sort (fill_regions.begin (), fill_regions.end ());
    fill_regions.erase (std::unique (fill_regions.begin (), fill_regions.end ()), fill_regions.end ());

    //  determine the fill region's bbox for selectively getting the exclude shapes
    db::Box fr_bbox;
    for (std::vector <db::Polygon>::const_iterator fr = fill_regions.begin (); fr != fill_regions.end (); ++fr) {
      fr_bbox += fr->box ();
    }

    if (tl::verbosity () >= 20) {
      tl::info << "Collecting exclude areas";
    }

    //  collect sized shapes from the exclude layers
    std::vector <db::Polygon> es;
    for (std::vector <unsigned int>::const_iterator l = exclude_layers.begin (); l != exclude_layers.end (); ++l) {

      std::vector <db::Polygon> shapes;

      size_t n = 0;
      for (db::RecursiveShapeIterator si (cv->layout (), *cv.cell (), *l); ! si.at_end (); ++si) {
        if (si->is_polygon () || si->is_path () || si->is_box ()) {
          ++n;
        }
      }

      shapes.reserve (n);

      for (db::RecursiveShapeIterator si (cv->layout (), *cv.cell (), *l); ! si.at_end (); ++si) {
        if (si->is_polygon () || si->is_path () || si->is_box ()) {
          shapes.push_back (db::Polygon ());
          si->polygon (shapes.back ());
          shapes.back ().transform (si.trans ());
        }
      }

      ep.enable_progress (tl::to_string (QObject::tr ("Preparing exclude regions")));
      ep.size (shapes, exclude_x, exclude_y, es, 2 /*mode*/, false /*=don't resolve holes*/);
      ep.disable_progress ();

    }

    if (tl::verbosity () >= 20) {
      tl::info << "Computing effective fill region";
    }

    //  Perform the NOT operation to create the fill region
    std::vector <db::Polygon> fill_area;
    ep.enable_progress (tl::to_string (QObject::tr ("Computing fill region")));
    ep.boolean (fill_regions, es, fill_area, db::BooleanOp::ANotB, false /*=don't resolve holes*/);
    ep.disable_progress ();

    std::vector <db::Polygon> new_fill_area;

    int step = 0;

    do {

      ++step;

      if (tl::verbosity () >= 20) {
        tl::info << "Major iteration (primary/secondary fill cell)";
      }

      std::vector <db::Polygon> non_filled_area;

      int iteration = 0;

      do {

        ++iteration;

        if (tl::verbosity () >= 20 && enhanced_fill) {
          tl::info << "Minor iteration (enhanced fill)";
        }

        tl::RelativeProgress progress (tl::sprintf (tl::to_string (QObject::tr ("Fill iteration %d (%s fill step)")), iteration, step == 1 ? tl::to_string (QObject::tr ("primary")) : tl::to_string (QObject::tr ("secondary"))), fill_area.size (), 10);

        new_fill_area.clear ();

        for (std::vector <db::Polygon>::const_iterator fp0 = fill_area.begin (); fp0 != fill_area.end (); ++fp0) {

          if (tl::verbosity () >= 30) {
            tl::info << "Compute fill for one region :" << fp0->to_string ();
          }

          bool any_fill = fill_region (cv.cell (), *fp0, fill_cell->cell_index (), fc_bbox, fr_bbox.p1 (), enhanced_fill, (enhanced_fill || fill_cell2) ? &new_fill_area : 0, fill_margin);
          if (! any_fill) {
            non_filled_area.push_back (*fp0);
          }

          ++progress;

        }

        fill_area.swap (new_fill_area);

      } while (enhanced_fill && ! fill_area.empty ());

      if (fill_area.empty ()) {
        fill_area.swap (non_filled_area);
      } else if (fill_cell2) {
        fill_area.insert (fill_area.end (), non_filled_area.begin (), non_filled_area.end ());
      }

      fill_cell = fill_cell2;
      fc_bbox = fc_bbox2;
      fill_margin = fill2_margin;

      fill_cell2 = 0;
      fc_bbox2 = db::Box ();

    } while (fill_cell != 0 && ! fill_area.empty ());

  }

  if (tl::verbosity () >= 20) {
    tl::info << "Fill done";
  }

  mp_view->manager ()->commit ();

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


