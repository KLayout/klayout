
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

#include "dbMAG.h"
#include "dbMAGReader.h"
#include "dbLoadLayoutOptions.h"
#include "layMAGReaderPlugin.h"
#include "ui_MAGReaderOptionPage.h"
#include "gsiDecl.h"

#include <QFrame>
#include <QFileDialog>

namespace lay
{

// ---------------------------------------------------------------
//  MAGReaderOptionPage definition and implementation

MAGReaderOptionPage::MAGReaderOptionPage (QWidget *parent)
  : StreamReaderOptionsPage (parent)
{
  mp_ui = new Ui::MAGReaderOptionPage ();
  mp_ui->setupUi (this);

  connect (mp_ui->add_lib_path, SIGNAL (clicked ()), this, SLOT (add_lib_path_clicked ()));
  connect (mp_ui->add_lib_path_with_choose, SIGNAL (clicked ()), this, SLOT (add_lib_path_clicked_with_choose ()));
  connect (mp_ui->del_lib_path, SIGNAL (clicked ()), mp_ui->lib_path, SLOT (delete_selected_items ()));
  connect (mp_ui->move_lib_path_up, SIGNAL (clicked ()), mp_ui->lib_path, SLOT (move_selected_items_up ()));
  connect (mp_ui->move_lib_path_down, SIGNAL (clicked ()), mp_ui->lib_path, SLOT (move_selected_items_down ()));
}

MAGReaderOptionPage::~MAGReaderOptionPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
MAGReaderOptionPage::setup (const db::FormatSpecificReaderOptions *o, const db::Technology * /*tech*/)
{
  static const db::MAGReaderOptions default_options;
  const db::MAGReaderOptions *options = dynamic_cast<const db::MAGReaderOptions *> (o);
  if (!options) {
    options = &default_options;
  }

  mp_ui->dbu_le->setText (tl::to_qstring (tl::to_string (options->dbu)));
  mp_ui->lambda_le->setText (tl::to_qstring (tl::to_string (options->lambda)));
  mp_ui->layer_map->set_layer_map (options->layer_map);
  mp_ui->read_all_cbx->setChecked (options->create_other_layers);
  mp_ui->keep_names_cbx->setChecked (options->keep_layer_names);
  mp_ui->merge_cbx->setChecked (options->merge);

  mp_ui->lib_path->set_values (options->lib_paths);
}

void 
MAGReaderOptionPage::commit (db::FormatSpecificReaderOptions *o, const db::Technology * /*tech*/)
{
  db::MAGReaderOptions *options = dynamic_cast<db::MAGReaderOptions *> (o);
  if (options) {

    tl::from_string_ext (tl::to_string (mp_ui->dbu_le->text ()), options->dbu);
    if (options->dbu > 1000.0 || options->dbu < 1e-9) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for database unit")));
    }

    tl::from_string_ext (tl::to_string (mp_ui->lambda_le->text ()), options->lambda);
    if (options->lambda > 10000000.0 || options->lambda < 1e-9) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for lambda")));
    }

    options->layer_map = mp_ui->layer_map->get_layer_map ();
    options->create_other_layers = mp_ui->read_all_cbx->isChecked ();
    options->keep_layer_names = mp_ui->keep_names_cbx->isChecked ();
    options->merge = mp_ui->merge_cbx->isChecked ();

    options->lib_paths = mp_ui->lib_path->get_values ();

  }
}

void
MAGReaderOptionPage::add_lib_path_clicked ()
{
  mp_ui->lib_path->add_value (tl::to_string (tr ("Enter your path here ...")));
}

void
MAGReaderOptionPage::add_lib_path_clicked_with_choose ()
{
  QString dir = QFileDialog::getExistingDirectory (this, QObject::tr ("Add library path"));
  if (! dir.isNull ()) {
    mp_ui->lib_path->add_value (tl::to_string (dir));
  }
}

// ---------------------------------------------------------------
//  MAGReaderPluginDeclaration definition and implementation

class MAGReaderPluginDeclaration
  : public StreamReaderPluginDeclaration
{
public:
  MAGReaderPluginDeclaration ()
    : StreamReaderPluginDeclaration (db::MAGReaderOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamReaderOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new MAGReaderOptionPage (parent);
  }

  db::FormatSpecificReaderOptions *create_specific_options () const
  {
    return new db::MAGReaderOptions ();
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::MAGReaderPluginDeclaration (), 10000, "MAGReader");

}

