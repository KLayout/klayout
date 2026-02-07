
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


#include "dbSaveLayoutOptions.h"
#include "lstrWriter.h"
#include "lstrFormat.h"
#include "layLStreamWriterPlugin.h"

#include <QFrame>

namespace lay
{

// ---------------------------------------------------------------
//  LStreamWriterOptionPage definition and implementation

LStreamWriterOptionPage::LStreamWriterOptionPage (QWidget *parent)
  : StreamWriterOptionsPage (parent)
{
  mp_ui = new Ui::LStreamWriterOptionPage ();
  mp_ui->setupUi (this);
}

LStreamWriterOptionPage::~LStreamWriterOptionPage ()
{
  delete mp_ui;
}

void 
LStreamWriterOptionPage::setup (const db::FormatSpecificWriterOptions *o, const db::Technology * /*tech*/)
{
  const lstr::WriterOptions *options = dynamic_cast<const lstr::WriterOptions *> (o);
  if (options) {
    mp_ui->compression_slider->setValue (options->compression_level);
    mp_ui->permissive->setChecked (options->permissive);
  }
}

void 
LStreamWriterOptionPage::commit (db::FormatSpecificWriterOptions *o, const db::Technology * /*tech*/, bool gzip)
{
  lstr::WriterOptions *options = dynamic_cast<lstr::WriterOptions *> (o);
  if (options) {
    options->compression_level = mp_ui->compression_slider->value ();
    options->permissive = mp_ui->permissive->isChecked ();
  }
}

// ---------------------------------------------------------------
//  LStreamWriterPluginDeclaration definition and implementation

class LStreamWriterPluginDeclaration
  : public StreamWriterPluginDeclaration
{
public:
  LStreamWriterPluginDeclaration ()
    : StreamWriterPluginDeclaration (lstr::WriterOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamWriterOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new LStreamWriterOptionPage (parent);
  }

  db::FormatSpecificWriterOptions *create_specific_options () const
  {
    return new lstr::WriterOptions ();
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::LStreamWriterPluginDeclaration (), 10002, "LStreamWriter");

}




