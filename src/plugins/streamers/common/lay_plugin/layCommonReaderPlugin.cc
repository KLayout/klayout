
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



#include "dbCommonReader.h"
#include "dbLoadLayoutOptions.h"
#include "layCommonReaderPlugin.h"
#include "ui_CommonReaderOptionsPage.h"
#include "gsiDecl.h"

#include <QFrame>

namespace lay
{

// ---------------------------------------------------------------
//  CommonReaderOptionPage definition and implementation

CommonReaderOptionPage::CommonReaderOptionPage (QWidget *parent)
  : StreamReaderOptionsPage (parent)
{
  mp_ui = new Ui::CommonReaderOptionPage ();
  mp_ui->setupUi (this);
}

CommonReaderOptionPage::~CommonReaderOptionPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
CommonReaderOptionPage::setup (const db::FormatSpecificReaderOptions *o, const db::Technology * /*tech*/)
{
  static const db::CommonReaderOptions default_options;
  const db::CommonReaderOptions *options = dynamic_cast<const db::CommonReaderOptions *> (o);
  if (!options) {
    options = &default_options;
  }

  mp_ui->enable_text_cbx->setChecked (options->enable_text_objects);
  mp_ui->enable_properties_cbx->setChecked (options->enable_properties);
  mp_ui->layer_map->set_layer_map (options->layer_map);
  mp_ui->read_all_cbx->setChecked (options->create_other_layers);
}

void
CommonReaderOptionPage::commit (db::FormatSpecificReaderOptions *o, const db::Technology * /*tech*/)
{
  db::CommonReaderOptions *options = dynamic_cast<db::CommonReaderOptions *> (o);
  if (options) {

    options->enable_text_objects = mp_ui->enable_text_cbx->isChecked ();
    options->enable_properties = mp_ui->enable_properties_cbx->isChecked ();
    options->layer_map = mp_ui->layer_map->get_layer_map ();
    options->create_other_layers = mp_ui->read_all_cbx->isChecked ();

  }
}

// ---------------------------------------------------------------
//  CommonReaderPluginDeclaration definition and implementation

class CommonReaderPluginDeclaration
  : public StreamReaderPluginDeclaration
{
public:
  CommonReaderPluginDeclaration ()
    : StreamReaderPluginDeclaration (db::CommonReaderOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamReaderOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new CommonReaderOptionPage (parent);
  }

  db::FormatSpecificReaderOptions *create_specific_options () const
  {
    return new db::CommonReaderOptions ();
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::CommonReaderPluginDeclaration (), 10000, "CommonReader");

}
