
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
#include "dbDXFWriter.h"
#include "dbSaveLayoutOptions.h"
#include "layDXFWriterPlugin.h"
#include "ui_DXFWriterOptionPage.h"

#include <QFrame>

namespace lay
{

// ---------------------------------------------------------------
//  DXFWriterOptionPage definition and implementation

DXFWriterOptionPage::DXFWriterOptionPage (QWidget *parent)
  : StreamWriterOptionsPage (parent)
{
  mp_ui = new Ui::DXFWriterOptionPage ();
  mp_ui->setupUi (this);
}

DXFWriterOptionPage::~DXFWriterOptionPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
DXFWriterOptionPage::setup (const db::FormatSpecificWriterOptions *o, const db::Technology * /*tech*/)
{
  const db::DXFWriterOptions *options = dynamic_cast<const db::DXFWriterOptions *> (o);
  if (options) {
    mp_ui->polygon_mode_cbx->setCurrentIndex (options->polygon_mode);
  }
}

void 
DXFWriterOptionPage::commit (db::FormatSpecificWriterOptions *o, const db::Technology * /*tech*/, bool /*gzip*/)
{
  db::DXFWriterOptions *options = dynamic_cast<db::DXFWriterOptions *> (o);
  if (options) {
    options->polygon_mode = mp_ui->polygon_mode_cbx->currentIndex ();
  }
}

// ---------------------------------------------------------------
//  DXFWriterPluginDeclaration definition and implementation

class DXFWriterPluginDeclaration
  : public StreamWriterPluginDeclaration
{
public:
  DXFWriterPluginDeclaration ()
    : StreamWriterPluginDeclaration (db::DXFWriterOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamWriterOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new DXFWriterOptionPage (parent);
  }

  db::FormatSpecificWriterOptions *create_specific_options () const
  {
    return new db::DXFWriterOptions ();
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::DXFWriterPluginDeclaration (), 10000, "DXFWriter");

}

