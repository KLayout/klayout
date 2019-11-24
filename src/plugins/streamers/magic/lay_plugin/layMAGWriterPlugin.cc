
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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
#include "dbMAGWriter.h"
#include "dbSaveLayoutOptions.h"
#include "layMAGWriterPlugin.h"
#include "ui_MAGWriterOptionPage.h"

#include <QFrame>

namespace lay
{

// ---------------------------------------------------------------
//  MAGWriterOptionPage definition and implementation

MAGWriterOptionPage::MAGWriterOptionPage (QWidget *parent)
  : StreamWriterOptionsPage (parent)
{
  mp_ui = new Ui::MAGWriterOptionPage ();
  mp_ui->setupUi (this);

  // .. nothing yet ..
}

MAGWriterOptionPage::~MAGWriterOptionPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
MAGWriterOptionPage::setup (const db::FormatSpecificWriterOptions *o, const db::Technology * /*tech*/)
{
  const db::MAGWriterOptions *options = dynamic_cast<const db::MAGWriterOptions *> (o);
  if (options) {
    mp_ui->dummy_calls_cbx->setChecked (options->dummy_calls);
    mp_ui->blank_separator_cbx->setChecked (options->blank_separator);
  }
}

void 
MAGWriterOptionPage::commit (db::FormatSpecificWriterOptions *o, const db::Technology * /*tech*/, bool /*gzip*/)
{
  db::MAGWriterOptions *options = dynamic_cast<db::MAGWriterOptions *> (o);
  if (options) {
    options->dummy_calls = mp_ui->dummy_calls_cbx->isChecked ();
    options->blank_separator = mp_ui->blank_separator_cbx->isChecked ();
  }
}

// ---------------------------------------------------------------
//  MAGWriterPluginDeclaration definition and implementation

class MAGWriterPluginDeclaration
  : public StreamWriterPluginDeclaration
{
public:
  MAGWriterPluginDeclaration ()
    : StreamWriterPluginDeclaration (db::MAGWriterOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamWriterOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new MAGWriterOptionPage (parent);
  }

  db::FormatSpecificWriterOptions *create_specific_options () const
  {
    return new db::MAGWriterOptions ();
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::MAGWriterPluginDeclaration (), 10000, "MAGWriter");

}

