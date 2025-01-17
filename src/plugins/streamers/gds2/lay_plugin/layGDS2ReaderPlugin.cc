
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



#include "dbGDS2.h"
#include "dbGDS2Reader.h"
#include "dbLoadLayoutOptions.h"
#include "layGDS2ReaderPlugin.h"
#include "ui_GDS2ReaderOptionPage.h"

#include <QFrame>

namespace lay
{

// ---------------------------------------------------------------
//  GDS2ReaderOptionPage definition and implementation

GDS2ReaderOptionPage::GDS2ReaderOptionPage (QWidget *parent)
  : StreamReaderOptionsPage (parent)
{
  mp_ui = new Ui::GDS2ReaderOptionPage ();
  mp_ui->setupUi (this);
}

GDS2ReaderOptionPage::~GDS2ReaderOptionPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
GDS2ReaderOptionPage::setup (const db::FormatSpecificReaderOptions *o, const db::Technology * /*tech*/)
{
  static const db::GDS2ReaderOptions default_options;
  const db::GDS2ReaderOptions *options = dynamic_cast<const db::GDS2ReaderOptions *> (o);
  if (!options) {
    options = &default_options;
  }

  mp_ui->big_records_cbx->setChecked (! options->allow_big_records);
  mp_ui->big_poly_cbx->setChecked (! options->allow_multi_xy_records);
  mp_ui->box_mode_cb->setCurrentIndex (options->box_mode);
}

void 
GDS2ReaderOptionPage::commit (db::FormatSpecificReaderOptions *o, const db::Technology * /*tech*/)
{
  db::GDS2ReaderOptions *options = dynamic_cast<db::GDS2ReaderOptions *> (o);
  if (options) {

    options->allow_big_records = ! mp_ui->big_records_cbx->isChecked ();
    options->allow_multi_xy_records = ! mp_ui->big_poly_cbx->isChecked ();
    options->box_mode = mp_ui->box_mode_cb->currentIndex ();

  }
}

// ---------------------------------------------------------------
//  GDS2ReaderPluginDeclaration definition and implementation

class GDS2ReaderPluginDeclaration
  : public StreamReaderPluginDeclaration
{
public:
  GDS2ReaderPluginDeclaration () 
    : StreamReaderPluginDeclaration (db::GDS2ReaderOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamReaderOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new GDS2ReaderOptionPage (parent);
  }

  db::FormatSpecificReaderOptions *create_specific_options () const
  {
    return new db::GDS2ReaderOptions ();
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::GDS2ReaderPluginDeclaration (), 10000, "GDS2Reader");

}
