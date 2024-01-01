
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


#include "dbCIF.h"
#include "dbCIFWriter.h"
#include "dbSaveLayoutOptions.h"
#include "layCIFWriterPlugin.h"
#include "ui_CIFWriterOptionPage.h"

#include <QFrame>

namespace lay
{

// ---------------------------------------------------------------
//  CIFWriterOptionPage definition and implementation

CIFWriterOptionPage::CIFWriterOptionPage (QWidget *parent)
  : StreamWriterOptionsPage (parent)
{
  mp_ui = new Ui::CIFWriterOptionPage ();
  mp_ui->setupUi (this);

  // .. nothing yet ..
}

CIFWriterOptionPage::~CIFWriterOptionPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
CIFWriterOptionPage::setup (const db::FormatSpecificWriterOptions *o, const db::Technology * /*tech*/)
{
  const db::CIFWriterOptions *options = dynamic_cast<const db::CIFWriterOptions *> (o);
  if (options) {
    mp_ui->dummy_calls_cbx->setChecked (options->dummy_calls);
    mp_ui->blank_separator_cbx->setChecked (options->blank_separator);
  }
}

void 
CIFWriterOptionPage::commit (db::FormatSpecificWriterOptions *o, const db::Technology * /*tech*/, bool /*gzip*/)
{
  db::CIFWriterOptions *options = dynamic_cast<db::CIFWriterOptions *> (o);
  if (options) {
    options->dummy_calls = mp_ui->dummy_calls_cbx->isChecked ();
    options->blank_separator = mp_ui->blank_separator_cbx->isChecked ();
  }
}

// ---------------------------------------------------------------
//  CIFWriterPluginDeclaration definition and implementation

class CIFWriterPluginDeclaration
  : public StreamWriterPluginDeclaration
{
public:
  CIFWriterPluginDeclaration () 
    : StreamWriterPluginDeclaration (db::CIFWriterOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamWriterOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new CIFWriterOptionPage (parent);
  }

  db::FormatSpecificWriterOptions *create_specific_options () const
  {
    return new db::CIFWriterOptions ();
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::CIFWriterPluginDeclaration (), 10000, "CIFWriter");

}

