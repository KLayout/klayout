
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

#include "dbCIF.h"
#include "dbCIFReader.h"
#include "dbLoadLayoutOptions.h"
#include "layCIFReaderPlugin.h"
#include "ui_CIFReaderOptionPage.h"
#include "gsiDecl.h"

#include <QFrame>

namespace lay
{

// ---------------------------------------------------------------
//  CIFReaderOptionPage definition and implementation

CIFReaderOptionPage::CIFReaderOptionPage (QWidget *parent)
  : StreamReaderOptionsPage (parent)
{
  mp_ui = new Ui::CIFReaderOptionPage ();
  mp_ui->setupUi (this);
}

CIFReaderOptionPage::~CIFReaderOptionPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
CIFReaderOptionPage::setup (const db::FormatSpecificReaderOptions *o, const db::Technology * /*tech*/)
{
  static const db::CIFReaderOptions default_options;
  const db::CIFReaderOptions *options = dynamic_cast<const db::CIFReaderOptions *> (o);
  if (!options) {
    options = &default_options;
  }

  mp_ui->dbu_le->setText (tl::to_qstring (tl::to_string (options->dbu)));
  mp_ui->layer_map->set_layer_map (options->layer_map);
  mp_ui->read_all_cbx->setChecked (options->create_other_layers);
  mp_ui->keep_names_cbx->setChecked (options->keep_layer_names);
  mp_ui->wire_mode_cb->setCurrentIndex (options->wire_mode);
}

void 
CIFReaderOptionPage::commit (db::FormatSpecificReaderOptions *o, const db::Technology * /*tech*/)
{
  db::CIFReaderOptions *options = dynamic_cast<db::CIFReaderOptions *> (o);
  if (options) {
    tl::from_string_ext (tl::to_string (mp_ui->dbu_le->text ()), options->dbu);
    if (options->dbu > 1000.0 || options->dbu < 1e-9) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for database unit")));
    }
    options->wire_mode = mp_ui->wire_mode_cb->currentIndex ();
    options->layer_map = mp_ui->layer_map->get_layer_map ();
    options->create_other_layers = mp_ui->read_all_cbx->isChecked ();
    options->keep_layer_names = mp_ui->keep_names_cbx->isChecked ();
  }
}

// ---------------------------------------------------------------
//  CIFReaderPluginDeclaration definition and implementation

class CIFReaderPluginDeclaration
  : public StreamReaderPluginDeclaration
{
public:
  CIFReaderPluginDeclaration () 
    : StreamReaderPluginDeclaration (db::CIFReaderOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamReaderOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new CIFReaderOptionPage (parent);
  }

  db::FormatSpecificReaderOptions *create_specific_options () const
  {
    return new db::CIFReaderOptions ();
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::CIFReaderPluginDeclaration (), 10000, "CIFReader");

}





