
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


#include "layDensityMapDialog.h"

#include "dbClip.h"
#include "dbTilingProcessor.h"
#include "antService.h"
#include "tlException.h"
#include "tlString.h"
#include "tlExceptions.h"
#include "layUtils.h"
#include "imgObject.h"
#include "imgService.h"

namespace lay
{

// ------------------------------------------------------------
//  Declaration of the configuration options

class DensityMapDialogPluginDeclaration
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
    menu_entries.push_back (lay::menu_item ("density_map::show", "density_map_tool:edit", "tools_menu.post_verification_group", tl::to_string (QObject::tr ("Density Map"))));
  }
 
  virtual lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *root, lay::LayoutViewBase *view) const
  {
    if (lay::has_gui ()) {
      return new DensityMapDialog (root, view);
    } else {
      return 0;
    }
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new DensityMapDialogPluginDeclaration (), 3002, "lay::DensityMapPlugin");


// ------------------------------------------------------------

DensityMapDialog::DensityMapDialog (lay::Dispatcher *root, LayoutViewBase *vw)
  : lay::Browser (root, vw), 
    Ui::DensityMapDialog ()
{
  Ui::DensityMapDialog::setupUi (this);

  connect (rb_box1, SIGNAL (clicked ()), this, SLOT (box_selection_clicked ()));
  connect (rb_rulers, SIGNAL (clicked ()), this, SLOT (box_selection_clicked ()));
  connect (rb_whole_layout, SIGNAL (clicked ()), this, SLOT (box_selection_clicked ()));
  connect (rb_layer_bbox, SIGNAL (clicked ()), this, SLOT (box_selection_clicked ()));
  connect (rb_visible, SIGNAL (clicked ()), this, SLOT (box_selection_clicked ()));

  connect (rb_visible_layers, SIGNAL (clicked ()), this, SLOT (source_selection_clicked ()));
  connect (rb_of_layer, SIGNAL (clicked ()), this, SLOT (source_selection_clicked ()));
}

void
DensityMapDialog::box_selection_clicked ()
{
  rb_box1->setChecked (sender () == rb_box1);
  grp_box1->setEnabled (sender () == rb_box1);
  rb_rulers->setChecked (sender () == rb_rulers);
  rb_whole_layout->setChecked (sender () == rb_whole_layout);
  rb_layer_bbox->setChecked (sender () == rb_layer_bbox);
  cb_box_layer->setEnabled (sender () == rb_layer_bbox);
  rb_visible->setChecked (sender () == rb_visible);
}

void
DensityMapDialog::source_selection_clicked ()
{
  rb_visible_layers->setChecked (sender () == rb_visible_layers);
  cb_source_layer->setEnabled (sender () == rb_of_layer);
  rb_of_layer->setChecked (sender () == rb_of_layer);
}

void 
DensityMapDialog::menu_activated (const std::string &symbol)
{
  if (symbol == "density_map::show") {

    int cv_index = view ()->active_cellview_index ();

    lay::CellView cv = view ()->cellview (cv_index);
    if (cv.is_valid ()) {
      cb_box_layer->set_view (view (), cv_index);
      cb_source_layer->set_view (view (), cv_index);
      show ();
      activate ();
    }

  } else {
    lay::Browser::menu_activated (symbol);
  }
}

DensityMapDialog::~DensityMapDialog ()
{
  //  .. nothing yet ..
}

// @@@@
class TileRec
  : public db::TileOutputReceiver
{
public:
  TileRec (img::Object *img)
    : mp_img (img)
  {
    //  .. nothing yet ..
  }

  virtual void put (size_t ix, size_t iy, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double /*dbu*/, const db::ICplxTrans & /*trans*/, bool /*clip*/)
  {
    tl::info << "@@@ " << ix << "," << iy << " -> " << obj.to_string () << " -- " << tile.to_string ();
    mp_img->set_pixel (ix, iy, obj.to_double ());
  }

private:
  img::Object *mp_img;
};
// @@@

void 
DensityMapDialog::accept ()
{
BEGIN_PROTECTED

  //  Collects all cv_index/layer index pairs used for input
  std::vector<std::pair<unsigned int, unsigned int> > input_layers;

  int threads = std::max (1, sb_threads->value ());

  double pixel_size = 0.0;
  tl::from_string_ext (tl::to_string (le_pixel_size->text ()), pixel_size);

  if (pixel_size < 1e-6) {
    throw tl::Exception (tl::to_string (QObject::tr ("Pixel size must be positive and not zero")));
  }

  db::DBox region;

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

    region = db::DBox (db::DPoint (x1, y1), db::DPoint (x2, y2));

  } else if (rb_rulers->isChecked ()) {

    ant::Service *ant_service = view ()->get_plugin <ant::Service> ();
    if (ant_service) {
      ant::AnnotationIterator ant = ant_service->begin_annotations ();
      while (! ant.at_end ()) {
        region += db::DBox (ant->p1 (), ant->p2 ());
        ++ant;
      }
    }

  } else if (rb_layer_bbox->isChecked ()) {

    lay::CellView ccv = view ()->cellview (cb_box_layer->cv_index ());
    int sel_layer = cb_box_layer->current_layer ();

    if (! ccv.is_valid () || sel_layer < 0 || ! ccv->layout ().is_valid_layer (sel_layer)) {
      throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected to get clip boxes from")));
    }

    region = db::CplxTrans (ccv->layout ().dbu ()) * ccv->layout ().cell (ccv.cell_index ()).bbox (sel_layer);

  } else if (rb_visible->isChecked ()) {

    region = view ()->box ();

  } else {

    lay::CellView cv = view ()->cellview (view ()->active_cellview_index ());
    region = db::CplxTrans (cv->layout ().dbu ()) * cv->layout ().cell (cv.cell_index ()).bbox ();

  }

  if (rb_of_layer->isChecked ()) {

    int cvi = cb_box_layer->cv_index ();
    lay::CellView ccv = view ()->cellview (cvi);
    int sel_layer = cb_box_layer->current_layer ();

    if (! ccv.is_valid () || sel_layer < 0 || ! ccv->layout ().is_valid_layer (sel_layer)) {
      throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected to get clip boxes from")));
    }

    input_layers.push_back (std::make_pair (cvi, sel_layer));

  } else {

    for (auto l = view ()->begin_layers (); !l.at_end (); ++l) {

      if (! l->has_children () && l->visible (true)) {

        int cvi = (l->cellview_index () >= 0) ? l->cellview_index () : 0;
        lay::CellView ccv = view ()->cellview (cvi);
        int li = l->layer_index ();

        if (ccv.is_valid () || li >= 0 || ! ccv->layout ().is_valid_layer (li)) {
          input_layers.push_back (std::make_pair (cvi, li));
        }

      }

    }

  }

  if (region.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Density map region is empty")));
  }

  if (input_layers.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No input layers given")));
  }

  //  Compute the tile origins

  const double max_wh = 100000.0;
  const int max_pixels = 100000000;

  double dnx = std::max (1.0, ceil (region.width () / pixel_size - db::epsilon));
  double dny = std::max (1.0, ceil (region.height () / pixel_size - db::epsilon));
  if (dnx > max_wh || dny > max_wh) {
    throw tl::Exception (tl::sprintf (tl::to_string (QObject::tr ("Density map dimensions exceed maximum limit of %g pixels in one direction"))), max_wh);
  }

  int nx = int (dnx);
  int ny = int (dny);
  if (dnx * dny > max_pixels) {
    throw tl::Exception (tl::sprintf (tl::to_string (QObject::tr ("Density map array exceed maximum limit of %d pixels in total"))), max_pixels);
  }

  double x0 = region.center ().x () - nx * 0.5 * pixel_size;
  double y0 = region.center ().y () - ny * 0.5 * pixel_size;

  double dbu = view ()->cellview (input_layers.front ().first)->layout ().dbu ();

  //  Set up the tiling processor

  db::TilingProcessor tp;

  tp.tiles (nx, ny);
  tp.tile_origin (x0, y0);
  tp.tile_size (pixel_size, pixel_size);

  tp.set_threads (threads);
  tp.set_dbu (dbu);

  int ninput = 1;
  std::string in_expr;

  for (auto i = input_layers.begin (); i != input_layers.end (); ++i, ++ninput) {

    const lay::CellView &cv = view ()->cellview (i->first);
    const db::Layout &ly = cv->layout ();
    const db::Cell &cell = ly.cell (cv.cell_index ());

    std::string in_name = "in" + tl::to_string (ninput);
    tp.input (in_name, db::RecursiveShapeIterator (ly, cell, i->second, true), db::ICplxTrans (ly.dbu () / dbu), db::TilingProcessor::TypeRegion, true);

    if (! in_expr.empty ()) {
      in_expr += "+";
    }
    in_expr += in_name;

  }

  tp.queue (std::string ("var inp = ") + in_expr + "; _tile && _output(dens, to_f(inp.area(_tile.bbox)) / to_f(_tile.bbox.area))");

  //  Prepare the Image for receiving

  img::Object *img_object = 0;

  img::Service *img_service = view ()->get_plugin <img::Service> ();
  if (img_service) {

    //  TODO: we could selectively clear all images that are used for density maps (-> needs a property that identifies them)
    img_service->clear_images ();

    img::DataMapping dm;
    dm.false_color_nodes.clear ();

    //  default mapping blue -> red
    //  TODO: we could use that from a previous image ...
    dm.false_color_nodes.push_back (std::make_pair (0.0, std::make_pair (0x0000ff, 0x0000ff)));
    dm.false_color_nodes.push_back (std::make_pair (1.0, std::make_pair (0xff0000, 0xff0000)));

    img::Object img (nx, ny, db::DCplxTrans (pixel_size, 0.0, false, region.center () - db::DPoint ()), false, false);
    img.set_data_mapping (dm);

    img_object = img_service->insert_image (img);

  }

  tl_assert (img_object);
  tp.output ("dens", 0, new TileRec (img_object), db::ICplxTrans ());

  //  Execute the tiling processor

  tp.execute (tl::to_string (tr ("Computing density map")));

  //  close this dialog
  QDialog::accept ();

END_PROTECTED
}

bool 
DensityMapDialog::configure (const std::string & /*name*/, const std::string & /*value*/)
{
  //  .. nothing yet ..
  return false;
}

}

