
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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



#include "dbDXF.h"
#include "dbDXFReader.h"
#include "dbLoadLayoutOptions.h"
#include "layDXFReaderPlugin.h"
#include "ui_DXFReaderOptionPage.h"
#include "gsiDecl.h"

#include <QFrame>

namespace lay
{

// ---------------------------------------------------------------
//  DXFReaderOptionPage definition and implementation

DXFReaderOptionPage::DXFReaderOptionPage (QWidget *parent)
  : StreamReaderOptionsPage (parent)
{
  mp_ui = new Ui::DXFReaderOptionPage ();
  mp_ui->setupUi (this);
}

DXFReaderOptionPage::~DXFReaderOptionPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
DXFReaderOptionPage::setup (const db::FormatSpecificReaderOptions *o, const lay::Technology * /*tech*/)
{
  static const db::DXFReaderOptions default_options;
  const db::DXFReaderOptions *options = dynamic_cast<const db::DXFReaderOptions *> (o);
  if (!options) {
    options = &default_options;
  }

  mp_ui->dbu_le->setText (tl::to_qstring (tl::to_string (options->dbu)));
  mp_ui->unit_le->setText (tl::to_qstring (tl::to_string (options->unit)));
  mp_ui->text_scaling_le->setText (tl::to_qstring (tl::to_string (options->text_scaling)));
  mp_ui->circle_points_le->setText (tl::to_qstring (tl::to_string (options->circle_points)));
  mp_ui->circle_accuracy_le->setText (tl::to_qstring (tl::to_string (options->circle_accuracy)));
  mp_ui->contour_accuracy_le->setText (tl::to_qstring (tl::to_string (options->contour_accuracy)));
  mp_ui->render_texts_as_polygons_cbx->setChecked (options->render_texts_as_polygons);
  mp_ui->keep_other_cells_cbx->setChecked (options->keep_other_cells);
  mp_ui->polyline2poly_cbx->setCurrentIndex (options->polyline_mode);
  mp_ui->layer_map->set_layer_map (options->layer_map);
  mp_ui->read_all_cbx->setChecked (options->create_other_layers);
  mp_ui->keep_names_cbx->setChecked (options->keep_layer_names);
}

void 
DXFReaderOptionPage::commit (db::FormatSpecificReaderOptions *o, const lay::Technology * /*tech*/)
{
  db::DXFReaderOptions *options = dynamic_cast<db::DXFReaderOptions *> (o);
  if (options) {
    tl::from_string (tl::to_string (mp_ui->dbu_le->text ()), options->dbu);
    if (options->dbu > 1000.0 || options->dbu < 1e-9) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for database unit")));
    }
    tl::from_string (tl::to_string (mp_ui->unit_le->text ()), options->unit);
    if (options->unit > 1e9 || options->unit < 1e-9) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for the unit")));
    }
    tl::from_string (tl::to_string (mp_ui->text_scaling_le->text ()), options->text_scaling);
    if (options->text_scaling > 10000 || options->text_scaling < 1) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for the text scaling")));
    }
    tl::from_string (tl::to_string(mp_ui->circle_points_le->text ()), options->circle_points);
    if (options->circle_points < 4 || options->circle_points > 1000000) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for the number of points for arc interpolation")));
    }
    tl::from_string (tl::to_string(mp_ui->circle_accuracy_le->text ()), options->circle_accuracy);
    tl::from_string (tl::to_string(mp_ui->contour_accuracy_le->text ()), options->contour_accuracy);
    options->polyline_mode = mp_ui->polyline2poly_cbx->currentIndex ();
    options->render_texts_as_polygons = mp_ui->render_texts_as_polygons_cbx->isChecked ();
    options->keep_other_cells = mp_ui->keep_other_cells_cbx->isChecked ();
    options->layer_map = mp_ui->layer_map->get_layer_map ();
    options->create_other_layers = mp_ui->read_all_cbx->isChecked ();
    options->keep_layer_names = mp_ui->keep_names_cbx->isChecked ();
  }
}

// ---------------------------------------------------------------
//  DXFReaderPluginDeclaration definition and implementation

class DXFReaderPluginDeclaration
  : public StreamReaderPluginDeclaration
{
public:
  DXFReaderPluginDeclaration () 
    : StreamReaderPluginDeclaration (db::DXFReaderOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamReaderOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new DXFReaderOptionPage (parent);
  }

  db::FormatSpecificReaderOptions *create_specific_options () const
  {
    return new db::DXFReaderOptions ();
  }

  virtual tl::XMLElementBase *xml_element () const
  {
    return new lay::ReaderOptionsXMLElement<db::DXFReaderOptions> ("dxf",
      tl::make_member (&db::DXFReaderOptions::dbu, "dbu") +
      tl::make_member (&db::DXFReaderOptions::unit, "unit") +
      tl::make_member (&db::DXFReaderOptions::text_scaling, "text-scaling") +
      tl::make_member (&db::DXFReaderOptions::circle_points, "circle-points") +
      tl::make_member (&db::DXFReaderOptions::circle_accuracy, "circle-accuracy") +
      tl::make_member (&db::DXFReaderOptions::contour_accuracy, "contour-accuracy") +
      tl::make_member (&db::DXFReaderOptions::polyline_mode, "polyline-mode") +
      tl::make_member (&db::DXFReaderOptions::render_texts_as_polygons, "render-texts-as-polygons") +
      tl::make_member (&db::DXFReaderOptions::keep_other_cells, "keep-other-cells") +
      tl::make_member (&db::DXFReaderOptions::keep_layer_names, "keep-layer-names") +
      tl::make_member (&db::DXFReaderOptions::create_other_layers, "create-other-layers") +
      tl::make_member (&db::DXFReaderOptions::layer_map, "layer-map")
    );
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::DXFReaderPluginDeclaration (), 10000, "DXFReader");

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_dxf_dbu (db::LoadLayoutOptions *options, double dbu)
{
  options->get_options<db::DXFReaderOptions> ().dbu = dbu;
}

static double get_dxf_dbu (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::DXFReaderOptions> ().dbu;
}

static void set_dxf_text_scaling (db::LoadLayoutOptions *options, double text_scaling)
{
  options->get_options<db::DXFReaderOptions> ().text_scaling = text_scaling;
}

static double get_dxf_text_scaling (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::DXFReaderOptions> ().text_scaling;
}

static void set_dxf_unit (db::LoadLayoutOptions *options, double unit)
{
  options->get_options<db::DXFReaderOptions> ().unit = unit;
}

static double get_dxf_unit (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::DXFReaderOptions> ().unit;
}

static void set_dxf_render_texts_as_polygons (db::LoadLayoutOptions *options, bool value)
{
  options->get_options<db::DXFReaderOptions> ().render_texts_as_polygons = value;
}

static bool get_dxf_render_texts_as_polygons (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::DXFReaderOptions> ().render_texts_as_polygons;
}

static void set_dxf_keep_other_cells (db::LoadLayoutOptions *options, bool value)
{
  options->get_options<db::DXFReaderOptions> ().keep_other_cells = value;
}

static bool get_dxf_keep_other_cells (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::DXFReaderOptions> ().keep_other_cells;
}

static void set_dxf_circle_points (db::LoadLayoutOptions *options, int circle_points)
{
  options->get_options<db::DXFReaderOptions> ().circle_points = circle_points;
}

static int get_dxf_circle_points (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::DXFReaderOptions> ().circle_points;
}

static void set_dxf_circle_accuracy (db::LoadLayoutOptions *options, double circle_accuracy)
{
  options->get_options<db::DXFReaderOptions> ().circle_accuracy = circle_accuracy;
}

static double get_dxf_circle_accuracy (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::DXFReaderOptions> ().circle_accuracy;
}

static void set_dxf_contour_accuracy (db::LoadLayoutOptions *options, double contour_accuracy)
{
  options->get_options<db::DXFReaderOptions> ().contour_accuracy = contour_accuracy;
}

static double get_dxf_contour_accuracy (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::DXFReaderOptions> ().contour_accuracy;
}

static void set_dxf_polyline_mode (db::LoadLayoutOptions *options, int mode)
{
  if (mode < 0 || mode > 4) {
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid polygon mode")));
  }

  options->get_options<db::DXFReaderOptions> ().polyline_mode = mode;
}

static int get_dxf_polyline_mode (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::DXFReaderOptions> ().polyline_mode;
}

static void set_layer_map (db::LoadLayoutOptions *options, const db::LayerMap &lm, bool f)
{
  options->get_options<db::DXFReaderOptions> ().layer_map = lm;
  options->get_options<db::DXFReaderOptions> ().create_other_layers = f;
}

static db::LayerMap &get_layer_map (db::LoadLayoutOptions *options)
{
  return options->get_options<db::DXFReaderOptions> ().layer_map;
}

static void select_all_layers (db::LoadLayoutOptions *options)
{
  options->get_options<db::DXFReaderOptions> ().layer_map = db::LayerMap ();
  options->get_options<db::DXFReaderOptions> ().create_other_layers = true;
}

static bool create_other_layers (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::DXFReaderOptions> ().create_other_layers;
}

static void set_create_other_layers (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::DXFReaderOptions> ().create_other_layers = l;
}

static bool keep_layer_names (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::DXFReaderOptions> ().keep_layer_names;
}

static void set_keep_layer_names (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::DXFReaderOptions> ().keep_layer_names = l;
}

//  extend lay::LoadLayoutOptions with the DXF options
static
gsi::ClassExt<db::LoadLayoutOptions> dxf_reader_options (
  gsi::method_ext ("dxf_set_layer_map", &set_layer_map, gsi::arg ("map"), gsi::arg ("create_other_layers"),
    "@brief Sets the layer map\n"
    "This sets a layer mapping for the reader. The \"create_other_layers\" specifies whether to create layers that are not "
    "in the mapping and automatically assign layers to them.\n"
    "@param map The layer map to set."
    "@param create_other_layers The flag telling whether other layer should be created also. Set to false if just the layers in the mapping table should be read.\n"
    "\n"
    "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
    "in a format-specific fashion."
  ) +
  gsi::method_ext ("dxf_select_all_layers", &select_all_layers,
    "@brief Selects all layers and disables the layer map\n"
    "\n"
    "This disables any layer map and enables reading of all layers.\n"
    "New layers will be created when required.\n"
    "\n"
    "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
    "in a format-specific fashion."
  ) +
  gsi::method_ext ("dxf_layer_map", &get_layer_map,
    "@brief Gets the layer map\n"
    "@return A reference to the layer map\n"
    "\n"
    "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
    "in a format-specific fashion."
  ) +
  gsi::method_ext ("dxf_create_other_layers?", &create_other_layers,
    "@brief Gets a value indicating whether other layers shall be created\n"
    "@return True, if other layers should be created.\n"
    "\n"
    "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
    "in a format-specific fashion."
  ) +
  gsi::method_ext ("dxf_create_other_layers=", &set_create_other_layers, gsi::arg ("create"),
    "@brief Specifies whether other layers shall be created\n"
    "@param create True, if other layers should be created.\n"
    "\n"
    "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
    "in a format-specific fashion."
  ) +
  gsi::method_ext ("dxf_dbu=", &set_dxf_dbu,
    "@brief Specifies the database unit which the reader uses and produces\n"
    "@args dbu\n"
    "\nThis property has been added in version 0.21.\n"
  ) +
  gsi::method_ext ("dxf_dbu", &get_dxf_dbu,
    "@brief Specifies the database unit which the reader uses and produces\n"
    "\nThis property has been added in version 0.21.\n"
  ) +
  gsi::method_ext ("dxf_text_scaling=", &set_dxf_text_scaling,
    "@brief Specifies the text scaling in percent of the default scaling\n"
    "@args unit\n"
    "\n"
    "The default value 100, meaning that the letter pitch is roughly 92 percent of the specified text height. "
    "Decrease this value to get smaller fonts and increase it to get larger fonts.\n"
    "\nThis property has been added in version 0.21.20.\n"
  ) +
  gsi::method_ext ("dxf_text_scaling", &get_dxf_text_scaling,
    "@brief Gets the text scaling factor (see \\dxf_text_scaling=)\n"
    "\nThis property has been added in version 0.21.20.\n"
  ) +
  gsi::method_ext ("dxf_unit=", &set_dxf_unit,
    "@brief Specifies the unit in which the DXF file is drawn.\n"
    "@args unit\n"
    "\nThis property has been added in version 0.21.3.\n"
  ) +
  gsi::method_ext ("dxf_unit", &get_dxf_unit,
    "@brief Specifies the unit in which the DXF file is drawn\n"
    "\nThis property has been added in version 0.21.3.\n"
  ) +
  gsi::method_ext ("dxf_circle_points=", &set_dxf_circle_points,
    "@brief Specifies the number of points used per full circle for arc interpolation\n"
    "@args points\n"
    "See also \\dxf_circle_accuracy for how to specify the number of points based on "
    "an approximation accuracy.\n"
    "\n"
    "\\dxf_circle_points and \\dxf_circle_accuracy also apply to other \"round\" structures "
    "such as arcs, ellipses and splines in the same sense than for circles.\n"
    "\n"
    "\nThis property has been added in version 0.21.6.\n"
  ) +
  gsi::method_ext ("dxf_circle_points", &get_dxf_circle_points,
    "@brief Gets the number of points used per full circle for arc interpolation\n"
    "\nThis property has been added in version 0.21.6.\n"
  ) +
  gsi::method_ext ("dxf_circle_accuracy=", &set_dxf_circle_accuracy,
    "@brief Specifies the accuracy of the circle approximation\n"
    "@args accuracy\n"
    "\n"
    "In addition to the number of points per circle, the circle accuracy can be specified. "
    "If set to a value larger than the database unit, the number of points per circle will "
    "be chosen such that the deviation from the ideal circle becomes less than this value.\n"
    "\n"
    "The actual number of points will not become bigger than the points specified through "
    "\\dxf_circle_points=. The accuracy value is given in the DXF file units (see \\dxf_unit) which is usually micrometers.\n"
    "\n"
    "\\dxf_circle_points and \\dxf_circle_accuracy also apply to other \"round\" structures "
    "such as arcs, ellipses and splines in the same sense than for circles.\n"
    "\n"
    "\nThis property has been added in version 0.24.9.\n"
  ) +
  gsi::method_ext ("dxf_circle_accuracy", &get_dxf_circle_accuracy,
    "@brief Gets the accuracy of the circle approximation\n"
    "\nThis property has been added in version 0.24.9.\n"
  ) +
  gsi::method_ext ("dxf_contour_accuracy=", &set_dxf_contour_accuracy,
    "@brief Specifies the accuracy for contour closing\n"
    "@args accuracy\n"
    "\n"
    "When polylines need to be connected or closed, this\n"
    "value is used to indicate the accuracy. This is the value (in DXF units)\n"
    "by which points may be separated and still be considered\n"
    "connected. The default is 0.0 which implies exact\n"
    "(within one DBU) closing.\n"
    "\n"
    "This value is effective in polyline mode 3 and 4.\n"
    "\n"
    "\nThis property has been added in version 0.25.3.\n"
  ) +
  gsi::method_ext ("dxf_contour_accuracy", &get_dxf_contour_accuracy,
    "@brief Gets the accuracy for contour closing\n"
    "\n"
    "\nThis property has been added in version 0.25.3.\n"
  ) +
  gsi::method_ext ("dxf_render_texts_as_polygons=", &set_dxf_render_texts_as_polygons,
    "@brief If this option is set to true, text objects are rendered as polygons\n"
    "@args value\n"
    "\nThis property has been added in version 0.21.15.\n"
  ) +
  gsi::method_ext ("dxf_render_texts_as_polygons", &get_dxf_render_texts_as_polygons,
    "@brief If this option is true, text objects are rendered as polygons\n"
    "\nThis property has been added in version 0.21.15.\n"
  ) +
  gsi::method_ext ("dxf_keep_layer_names?", &keep_layer_names,
    "@brief Gets a value indicating whether layer names are kept\n"
    "@return True, if layer names are kept.\n"
    "\n"
    "When set to true, no attempt is made to translate "
    "layer names to GDS layer/datatype numbers. If set to false (the default), a layer named \"L2D15\" will be translated "
    "to GDS layer 2, datatype 15.\n"
    "\n"
    "This method has been added in version 0.25.3."
  ) +
  gsi::method_ext ("dxf_keep_layer_names=", &set_keep_layer_names, gsi::arg ("keep"),
    "@brief Gets a value indicating whether layer names are kept\n"
    "@param keep True, if layer names are to be kept.\n"
    "\n"
    "See \\cif_keep_layer_names? for a description of this property.\n"
    "\n"
    "This method has been added in version 0.25.3."
  ) +
  gsi::method_ext ("dxf_keep_other_cells=", &set_dxf_keep_other_cells,
    "@brief If this option is set to true, all cells are kept, not only the top cell and it's children\n"
    "@args value\n"
    "\nThis property has been added in version 0.21.15.\n"
  ) +
  gsi::method_ext ("dxf_keep_other_cells", &get_dxf_keep_other_cells,
    "@brief If this option is true, all cells are kept, not only the top cell and it's children\n"
    "\nThis property has been added in version 0.21.15.\n"
  ) +
  gsi::method_ext ("dxf_polyline_mode=", &set_dxf_polyline_mode,
    "@brief Specifies how to treat POLYLINE/LWPOLYLINE entities.\n"
    "@args mode\n"
    "The mode is 0 (automatic), 1 (keep lines), 2 (create polygons from closed polylines with width = 0), "
    "3 (merge all lines with width = 0 into polygons), 4 (as 3 plus auto-close open contours).\n"
    "\nThis property has been added in version 0.21.3.\n"
  ) +
  gsi::method_ext ("dxf_polyline_mode", &get_dxf_polyline_mode,
    "@brief Specifies whether closed POLYLINE and LWPOLYLINE entities with width 0 are converted to polygons.\n"
    "See \\dxf_polyline_mode= for a description of this property.\n"
    "\nThis property has been added in version 0.21.3.\n"
  ),
  ""
);

}





