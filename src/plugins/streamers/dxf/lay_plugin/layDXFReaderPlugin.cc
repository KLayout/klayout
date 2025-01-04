
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

#include "dbDXF.h"
#include "dbDXFReader.h"
#include "dbLoadLayoutOptions.h"
#include "layDXFReaderPlugin.h"
#include "ui_DXFReaderOptionPage.h"

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
DXFReaderOptionPage::setup (const db::FormatSpecificReaderOptions *o, const db::Technology * /*tech*/)
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
DXFReaderOptionPage::commit (db::FormatSpecificReaderOptions *o, const db::Technology * /*tech*/)
{
  db::DXFReaderOptions *options = dynamic_cast<db::DXFReaderOptions *> (o);
  if (options) {
    tl::from_string_ext (tl::to_string (mp_ui->dbu_le->text ()), options->dbu);
    if (options->dbu > 1000.0 || options->dbu < 1e-9) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for database unit")));
    }
    tl::from_string_ext (tl::to_string (mp_ui->unit_le->text ()), options->unit);
    if (options->unit > 1e9 || options->unit < 1e-9) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for the unit")));
    }
    tl::from_string_ext (tl::to_string (mp_ui->text_scaling_le->text ()), options->text_scaling);
    if (options->text_scaling > 10000 || options->text_scaling < 1) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for the text scaling")));
    }
    tl::from_string_ext (tl::to_string(mp_ui->circle_points_le->text ()), options->circle_points);
    if (options->circle_points < 4 || options->circle_points > 1000000) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for the number of points for arc interpolation")));
    }
    tl::from_string_ext (tl::to_string(mp_ui->circle_accuracy_le->text ()), options->circle_accuracy);
    tl::from_string_ext (tl::to_string(mp_ui->contour_accuracy_le->text ()), options->contour_accuracy);
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
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::DXFReaderPluginDeclaration (), 10000, "DXFReader");

}





