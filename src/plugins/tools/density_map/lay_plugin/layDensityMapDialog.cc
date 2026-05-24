
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

static const std::string layer_mode_specific ("layer-mode-specific");
static const std::string layer_mode_visible ("layer-mode-visible");
static const std::string layer_mode_selected ("layer-mode-selected");

//  items in layer_cb
static const std::vector<std::string> layer_modes = { layer_mode_specific, layer_mode_visible, layer_mode_selected };

static const std::string region_mode_global_bbox ("region-mode-global-bbox");
static const std::string region_mode_layer_bbox ("region-mode-layer-bbox");
static const std::string region_mode_single_box ("region-mode-single-box");
static const std::string region_mode_visible_region ("region-mode-visible-region");
static const std::string region_mode_by_rulers ("region-mode-by-rulers");

//  items in region_cb
static const std::vector<std::string> region_modes = {
  region_mode_global_bbox,
  region_mode_layer_bbox,
  region_mode_single_box,
  region_mode_visible_region,
  region_mode_by_rulers
};

static const std::string boundary_mode_periodic ("boundary-mode-periodic");
static const std::string boundary_mode_zero ("boundary-mode-zero");
static const std::string boundary_mode_one ("boundary-mode-one");
static const std::string boundary_mode_average ("boundary-mode-average");

//  items in boundary_mode_cb
static const std::vector<std::string> boundary_modes = { boundary_mode_periodic, boundary_mode_zero, boundary_mode_one, boundary_mode_average };

static const std::string averaging_mode_square ("averaging-mode-square");
static const std::string averaging_mode_gaussian ("averaging-mode-gaussian");

//  items in average_mode_cb
static const std::vector<std::string> averaging_modes = { averaging_mode_square, averaging_mode_gaussian };

static const std::string cfg_density_map_region_mode ("density-map-region-mode");
static const std::string cfg_density_map_layer_mode ("density-map-layer-mode");
static const std::string cfg_density_map_pixel_size ("density-map-pixel-size");
static const std::string cfg_density_map_window_size ("density-map-window-size");
static const std::string cfg_density_map_averaging_mode ("density-map-averaging-mode");
static const std::string cfg_density_map_boundary_mode ("density-map-boundary-mode");
static const std::string cfg_density_map_threads ("density-map-threads");
static const std::string cfg_density_map_source_layer ("density-map-source-layer");
static const std::string cfg_density_map_box_layer ("density-map-box-layer");
static const std::string cfg_density_map_single_box ("density-map-single-box");

// ------------------------------------------------------------
//  Declaration of the configuration options

class DensityMapDialogPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::make_pair (cfg_density_map_layer_mode, layer_mode_specific));
    options.push_back (std::make_pair (cfg_density_map_region_mode, region_mode_global_bbox));
    options.push_back (std::make_pair (cfg_density_map_pixel_size, "100"));
    options.push_back (std::make_pair (cfg_density_map_window_size, "0"));
    options.push_back (std::make_pair (cfg_density_map_boundary_mode, boundary_mode_periodic));
    options.push_back (std::make_pair (cfg_density_map_averaging_mode, averaging_mode_square));
    options.push_back (std::make_pair (cfg_density_map_threads, "1"));
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

  connect (layer_cb, SIGNAL (activated (int)), SLOT (layer_mode_changed (int)));
  connect (region_cb, SIGNAL (activated (int)), SLOT (region_mode_changed (int)));

  connect (button_box->button (QDialogButtonBox::Apply), SIGNAL (clicked ()), this, SLOT (apply ()));

  region_mode_changed (region_cb->currentIndex ());
  layer_mode_changed (layer_cb->currentIndex ());
}

void
DensityMapDialog::region_mode_changed (int mode)
{
  if (mode < 0 || mode >= int (region_modes.size ())) {
    return;
  }

  const auto &rm = region_modes [mode];
  if (rm == region_mode_layer_bbox) {
    region_stack->setCurrentIndex (0);
  } else if (rm == region_mode_single_box) {
    region_stack->setCurrentIndex (1);
  } else {
    region_stack->setCurrentIndex (2);
  }
}

void
DensityMapDialog::layer_mode_changed (int mode)
{
  if (mode < 0 || mode >= int (layer_modes.size ())) {
    return;
  }

  const auto &lm = layer_modes [mode];
  if (lm == layer_mode_specific) {
    layer_stack->setCurrentIndex (0);
  } else {
    layer_stack->setCurrentIndex (1);
  }
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

namespace
{

  class DensityMapTileReceiver
    : public db::TileOutputReceiver
  {
  public:
    DensityMapTileReceiver (img::Object *img)
      : mp_img (img)
    {
      //  .. nothing yet ..
    }

    virtual void put (size_t ix, size_t iy, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double /*dbu*/, const db::ICplxTrans & /*trans*/, bool /*clip*/)
    {
      if (tl::verbosity () >= 30) {
        tl::info << "Density map value: " << ix << "," << iy << " " << tile.to_string () << " -> " << obj.to_string ();
      }

      mp_img->set_pixel (ix, iy, obj.to_double ());
    }

  private:
    img::Object *mp_img;
  };

}

void
DensityMapDialog::apply ()
{
BEGIN_PROTECTED

  make_density_map ();

END_PROTECTED
}

void
DensityMapDialog::accept ()
{
BEGIN_PROTECTED

  make_density_map ();

  //  close this dialog
  QDialog::accept ();

END_PROTECTED
}

bool
DensityMapDialog::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_density_map_layer_mode) {

    int mode = 0;
    for (size_t i = 0; i < layer_modes.size (); ++i) {
      if (layer_modes[i] == value) {
        mode = int (i);
      }
    }

    layer_cb->setCurrentIndex (mode);
    layer_mode_changed (mode);
    return true;

  } else if (name == cfg_density_map_region_mode) {

    int mode = 0;
    for (size_t i = 0; i < region_modes.size (); ++i) {
      if (region_modes[i] == value) {
        mode = int (i);
      }
    }

    region_cb->setCurrentIndex (mode);
    region_mode_changed (mode);
    return true;

  } else if (name == cfg_density_map_pixel_size) {

    double px = 100.0;
    try {
      tl::from_string (value, px);
      le_pixel_size->setText (tl::to_qstring (tl::to_string (px)));
    } catch (...) {
    }
    return true;

  } else if (name == cfg_density_map_window_size) {

    double ws = 0.0;
    try {
      tl::from_string (value, ws);
      if (ws > 0) {
        le_window_size->setText (tl::to_qstring (tl::to_string (ws)));
      } else {
        le_window_size->setText (QString ());
      }
    } catch (...) {
    }
    return true;

  } else if (name == cfg_density_map_boundary_mode) {

    int mode = 0;
    for (size_t i = 0; i < boundary_modes.size (); ++i) {
      if (boundary_modes[i] == value) {
        mode = int (i);
      }
    }

    cb_boundary_mode->setCurrentIndex (mode);
    return true;

  } else if (name == cfg_density_map_averaging_mode) {

    int mode = 0;
    for (size_t i = 0; i < averaging_modes.size (); ++i) {
      if (averaging_modes[i] == value) {
        mode = int (i);
      }
    }

    cb_averaging_mode->setCurrentIndex (mode);
    return true;

  } else if (name == cfg_density_map_threads) {

    int thr = 1;
    try {
      tl::from_string (value, thr);
      sb_threads->setValue (thr);
    } catch (...) {
    }
    return true;

  } else if (name == cfg_density_map_source_layer) {

    db::LayerProperties lp;
    try {
      tl::from_string (value, lp);
      cb_source_layer->set_current_layer (lp);
    } catch (...) {
    }
    return true;

  } else if (name == cfg_density_map_box_layer) {

    db::LayerProperties lp;
    try {
      tl::from_string (value, lp);
      cb_box_layer->set_current_layer (lp);
    } catch (...) {
    }
    return true;

  } else if (name == cfg_density_map_single_box) {

    db::DBox bx;
    try {
      tl::from_string (value, bx);
      if (bx.empty ()) {
        le_x1->setText (QString ());
        le_y1->setText (QString ());
        le_x2->setText (QString ());
        le_y2->setText (QString ());
      } else {
        le_x1->setText (tl::to_qstring (tl::to_string (bx.left ())));
        le_y1->setText (tl::to_qstring (tl::to_string (bx.bottom ())));
        le_x2->setText (tl::to_qstring (tl::to_string (bx.right ())));
        le_y2->setText (tl::to_qstring (tl::to_string (bx.top ())));
      }
    } catch (...) {
    }
    return true;

  } else {
    return false;
  }
}

void
DensityMapDialog::make_density_map ()
{
  DensityMapParameters par;

  par.threads = std::max (1, sb_threads->value ());

  par.pixel_size = 0.0;
  tl::from_string_ext (tl::to_string (le_pixel_size->text ()), par.pixel_size);

  if (par.pixel_size < 1e-6) {
    throw tl::Exception (tl::to_string (QObject::tr ("The pixel size must be positive and not zero")));
  }

  par.window_size = 0.0;
  if (! le_window_size->text ().simplified ().isEmpty ()) {
    tl::from_string_ext (tl::to_string (le_window_size->text ()), par.window_size);
    if (par.window_size < 1e-6) {
      throw tl::Exception (tl::to_string (QObject::tr ("The window size must be positive and not zero or empty")));
    }
  }

  int boundary_mode = cb_boundary_mode->currentIndex ();
  if (boundary_mode < 0 || boundary_mode >= int (boundary_modes.size ())) {
    return;
  }
  par.boundary_mode = boundary_modes [boundary_mode];

  int averaging_mode = cb_averaging_mode->currentIndex ();
  if (averaging_mode < 0 || averaging_mode >= int (averaging_modes.size ())) {
    return;
  }
  par.averaging_mode = averaging_modes [averaging_mode];

  int region_mode = region_cb->currentIndex ();
  if (region_mode < 0 || region_mode >= int (region_modes.size ())) {
    return;
  }
  const auto &rm = region_modes [region_mode];

  int layer_mode = layer_cb->currentIndex ();
  if (layer_mode < 0 || layer_mode >= int (layer_modes.size ())) {
    return;
  }
  const auto &lm = layer_modes [layer_mode];

  db::LayerProperties region_layer;
  db::LayerProperties source_layer;

  if (rm == region_mode_single_box) {

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

    par.region = db::DBox (db::DPoint (x1, y1), db::DPoint (x2, y2));

  } else if (rm == region_mode_by_rulers) {

    ant::Service *ant_service = view ()->get_plugin <ant::Service> ();
    if (ant_service) {
      ant::AnnotationIterator ant = ant_service->begin_annotations ();
      while (! ant.at_end ()) {
        par.region += db::DBox (ant->p1 (), ant->p2 ());
        ++ant;
      }
    }

  } else if (rm == region_mode_layer_bbox) {

    lay::CellView ccv = view ()->cellview (cb_box_layer->cv_index ());
    int sel_layer = cb_box_layer->current_layer ();

    region_layer = cb_box_layer->current_layer_props ();

    if (! ccv.is_valid () || sel_layer < 0 || ! ccv->layout ().is_valid_layer (sel_layer)) {
      throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected to get clip boxes from")));
    }

    par.region = db::CplxTrans (ccv->layout ().dbu ()) * ccv->layout ().cell (ccv.cell_index ()).bbox (sel_layer);

  } else if (rm == region_mode_visible_region) {

    par.region = view ()->box ();

  } else {

    lay::CellView cv = view ()->cellview (view ()->active_cellview_index ());
    par.region = db::CplxTrans (cv->layout ().dbu ()) * cv->layout ().cell (cv.cell_index ()).bbox ();

  }

  if (lm == layer_mode_specific) {

    int cvi = cb_source_layer->cv_index ();
    lay::CellView ccv = view ()->cellview (cvi);
    int sel_layer = cb_source_layer->current_layer ();

    source_layer = cb_source_layer->current_layer_props ();

    if (! ccv.is_valid () || sel_layer < 0 || ! ccv->layout ().is_valid_layer (sel_layer)) {
      throw tl::Exception (tl::to_string (QObject::tr ("No valid layer selected to get clip boxes from")));
    }

    par.input_layers.push_back (std::make_pair (cvi, sel_layer));

  } else if (lm == layer_mode_visible) {

    for (auto l = view ()->begin_layers (); !l.at_end (); ++l) {

      if (! l->has_children () && l->visible (true)) {

        int cvi = (l->cellview_index () >= 0) ? l->cellview_index () : 0;
        lay::CellView ccv = view ()->cellview (cvi);
        int li = l->layer_index ();

        if (ccv.is_valid () || li >= 0 || ! ccv->layout ().is_valid_layer (li)) {
          par.input_layers.push_back (std::make_pair (cvi, li));
        }

      }

    }

  } else if (lm == layer_mode_selected) {

    auto sel = view ()->selected_layers ();
    for (auto s = sel.begin (); s != sel.end (); ++s) {

      auto l = *s;
      if (! l->has_children ()) {

        int cvi = (l->cellview_index () >= 0) ? l->cellview_index () : 0;
        lay::CellView ccv = view ()->cellview (cvi);
        int li = l->layer_index ();

        if (ccv.is_valid () || li >= 0 || ! ccv->layout ().is_valid_layer (li)) {
          par.input_layers.push_back (std::make_pair (cvi, li));
        }

      }

    }

  }

  //  Commit the parameters
  dispatcher ()->config_set (cfg_density_map_layer_mode, lm);
  dispatcher ()->config_set (cfg_density_map_region_mode, rm);
  dispatcher ()->config_set (cfg_density_map_boundary_mode, par.boundary_mode);
  dispatcher ()->config_set (cfg_density_map_averaging_mode, par.averaging_mode);
  dispatcher ()->config_set (cfg_density_map_threads, tl::to_string (par.threads));
  dispatcher ()->config_set (cfg_density_map_pixel_size, tl::to_string (par.pixel_size));
  dispatcher ()->config_set (cfg_density_map_window_size, tl::to_string (par.window_size));

  if (lm == layer_mode_specific) {
    dispatcher ()->config_set (cfg_density_map_source_layer, source_layer.to_string ());
  }

  if (rm == region_mode_layer_bbox) {
    dispatcher ()->config_set (cfg_density_map_box_layer, region_layer.to_string ());
  } else if (rm == region_mode_single_box) {
    dispatcher ()->config_set (cfg_density_map_single_box, par.region.to_string ());
  }

  dispatcher ()->config_setup ();

  compute_density_map (par);
}

void
DensityMapDialog::compute_density_map (const DensityMapParameters &par)
{
  if (par.region.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Density map region is empty")));
  }

  if (par.input_layers.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No input layers given")));
  }

  //  Compute the tile origins

  const double max_wh = 100000.0;
  const int max_pixels = 100000000;

  double dnx = std::max (1.0, ceil (par.region.width () / par.pixel_size - db::epsilon));
  double dny = std::max (1.0, ceil (par.region.height () / par.pixel_size - db::epsilon));
  if (dnx > max_wh || dny > max_wh) {
    throw tl::Exception (tl::sprintf (tl::to_string (QObject::tr ("Density map dimensions exceed maximum limit of %g pixels in one direction"))), max_wh);
  }

  int nx = int (dnx);
  int ny = int (dny);
  if (dnx * dny > max_pixels) {
    throw tl::Exception (tl::sprintf (tl::to_string (QObject::tr ("Density map array exceed maximum limit of %d pixels in total"))), max_pixels);
  }

  double x0 = par.region.center ().x () - nx * 0.5 * par.pixel_size;
  double y0 = par.region.center ().y () - ny * 0.5 * par.pixel_size;

  double dbu = view ()->cellview (par.input_layers.front ().first)->layout ().dbu ();

  //  Set up the tiling processor

  db::TilingProcessor tp;

  tp.tiles (nx, ny);
  tp.tile_origin (x0, y0);
  tp.tile_size (par.pixel_size, par.pixel_size);

  tp.set_threads (par.threads);
  tp.set_dbu (dbu);

  int ninput = 1;
  std::string in_expr;

  for (auto i = par.input_layers.begin (); i != par.input_layers.end (); ++i, ++ninput) {

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

    const std::string img_tag = "density-map-image";

    img::DataMapping dm;
    dm.false_color_nodes.clear ();

    //  default mapping blue -> red
    //  TODO: we could use that from a previous image ...
    dm.false_color_nodes.push_back (std::make_pair (0.0, std::make_pair (0x0000ff, 0x0000ff)));
    dm.false_color_nodes.push_back (std::make_pair (1.0, std::make_pair (0xff0000, 0xff0000)));

    for (auto i = img_service->begin_images (); ! i.at_end (); ++i) {
      if (i->tag () == img_tag) {
        //  inherit data mapping from previous image
        dm = i->data_mapping ();
        img_service->erase_image_by_id (i->id ());
        break;
      }
    }

    img::Object img (nx, ny, db::DCplxTrans (par.pixel_size, 0.0, false, par.region.center () - db::DPoint ()), false, false);
    img.set_data_mapping (dm);
    img.set_tag (img_tag);

    img_object = img_service->insert_image (img);

  }

  tl_assert (img_object);
  tp.output ("dens", 0, new DensityMapTileReceiver (img_object), db::ICplxTrans ());

  //  Execute the tiling processor

  tp.execute (tl::to_string (tr ("Computing density map")));

  //  Do the averaging if requested

  unsigned int nw = (unsigned int) (std::max (0.0, std::min (1000.0, floor (par.window_size / par.pixel_size + 0.5))));

  if (nw > 1) {

    if (par.averaging_mode == averaging_mode_square) {

      std::vector<double> weights;

      int wh = nw / 2;
      weights.resize (wh * 2 + 1, 1.0);
      if (nw % 2 == 0) {
        weights.front () = weights.back () = 0.5;
      }

      average_window (*img_object, par.boundary_mode, wh, weights);

    } else if (par.averaging_mode == averaging_mode_gaussian) {

      std::vector<double> weights;

      int wh = nw * 3 / 2;

      //  Note: the averaging kernel (f = exp(-r^2) = exp(-(x^2+y^2)) can be decomposed
      //  into f = f(x)*f(y); f(x) = exp(-x^2); f(y) = exp(-y^2).
      //  Hence we can use a 1d-kernel here and average vertically first, then horizontally.

      weights.reserve (wh * 2 + 1);
      for (int i = -wh; i <= wh; ++i) {
        double x = double (i) / (nw * 0.5);
        weights.push_back (exp (-x * x));
      }

      average_window (*img_object, par.boundary_mode, wh, weights);

    }

  }
}

inline int safe_mod (int a, int b)
{
  if (a < 0) {
    return b - (-a % b);
  } else {
    return a % b;
  }
}

void
DensityMapDialog::average_window (img::Object &img_object, const std::string boundary_mode, int wh, const std::vector<double> &weights)
{
  tl_assert (weights.size () == size_t (wh * 2 + 1));

  double ws = 0.0;
  for (auto i = weights.begin (); i != weights.end (); ++i) {
    ws += *i;
  }

  bool periodic = (boundary_mode == boundary_mode_periodic);

  int nx = img_object.width ();
  int ny = img_object.height ();

  //  compute the outside value if not periodic
  double outside = 0.0;
  if (boundary_mode == boundary_mode_one) {

    outside = 1.0;

  } else if (boundary_mode == boundary_mode_average) {

    const float *d = img_object.float_data ();
    for (size_t i = size_t (nx) * size_t (ny); i > 0; --i) {
      outside += *d++;
    }
    outside /= double (nx) * double (ny);

  }

  std::vector<double> vavg_data;
  vavg_data.resize (size_t (nx) * size_t (ny), 0.0);

  //  vertical sum

  for (int y = 0; y < ny; ++y) {

    for (int dy = -wh; dy <= wh; ++dy) {

      //  top and bottom row count half in case of even nw
      double f = weights [dy + wh];

      std::vector<double>::iterator d = vavg_data.begin () + y * nx;
      if (periodic || (y + dy >= 0 && y + dy < ny)) {
        const float *s = img_object.float_data () + safe_mod (y + dy, ny) * nx;
        for (int ix = 0; ix < nx; ++ix) {
          *d++ += f * *s++;
        }
      } else {
        for (int ix = 0; ix < nx; ++ix) {
          *d++ += f * outside;
        }
      }

    }

  }

  //  horizontal sum

  outside *= ws;   //  because we do normalization later

  //  TODO: transposing the image would make things more efficient

  std::vector<double> havg_data;
  havg_data.resize (size_t (nx) * size_t (ny), 0.0);

  for (int x = 0; x < nx; ++x) {

    for (int dx = -wh; dx <= wh; ++dx) {

      //  top and bottom row count half in case of even nw
      double f = weights [dx + wh];

      std::vector<double>::iterator d = havg_data.begin () + x;

      if (periodic || (x + dx >= 0 && x + dx < nx)) {
        std::vector<double>::const_iterator s = vavg_data.begin () + safe_mod (x + dx, nx);
        for (int iy = 0; iy < ny; ++iy) {
          *d += f * *s;
          d += nx;
          s += nx;
        }
      } else {
        for (int iy = 0; iy < ny; ++iy) {
          *d += f * outside;
          d += nx;
        }
      }

    }

  }

  //  take the average
  double s = 1.0 / (ws * ws);
  for (auto i = havg_data.begin (); i != havg_data.end (); ++i) {
    *i *= s;
  }

  img_object.set_data (nx, ny, havg_data);
}

}

