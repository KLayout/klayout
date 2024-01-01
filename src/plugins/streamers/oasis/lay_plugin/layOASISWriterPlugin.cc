
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


#include "dbOASIS.h"
#include "dbOASISWriter.h"
#include "dbSaveLayoutOptions.h"
#include "layOASISWriterPlugin.h"

#include <QFrame>

namespace lay
{

// ---------------------------------------------------------------
//  OASISWriterOptionPage definition and implementation

OASISWriterOptionPage::OASISWriterOptionPage (QWidget *parent)
  : StreamWriterOptionsPage (parent)
{
  mp_ui = new Ui::OASISWriterOptionPage ();
  mp_ui->setupUi (this);

  connect (mp_ui->write_cblocks, SIGNAL (clicked(bool)), this, SLOT (flags_changed()));
  connect (mp_ui->strict_mode, SIGNAL (clicked(bool)), this, SLOT (flags_changed()));
}

OASISWriterOptionPage::~OASISWriterOptionPage ()
{
  delete mp_ui;
}

void 
OASISWriterOptionPage::setup (const db::FormatSpecificWriterOptions *o, const db::Technology * /*tech*/)
{
  const db::OASISWriterOptions *options = dynamic_cast<const db::OASISWriterOptions *> (o);
  if (options) {
    mp_ui->compression_slider->setValue (options->compression_level);
    mp_ui->write_cblocks->setChecked (options->write_cblocks);
    mp_ui->cblock_warning_frame->setEnabled (! options->write_cblocks);
    mp_ui->strict_mode->setChecked (options->strict_mode);
    mp_ui->strict_mode_warning_frame->setEnabled (! options->strict_mode);
    mp_ui->std_prop_mode->setCurrentIndex (options->write_std_properties);
    mp_ui->subst_char->setText (tl::to_qstring (options->subst_char));
    mp_ui->permissive->setChecked (options->permissive);
  }
}

void
OASISWriterOptionPage::flags_changed ()
{
  mp_ui->cblock_warning_frame->setEnabled (! mp_ui->write_cblocks->isChecked ());
  mp_ui->strict_mode_warning_frame->setEnabled (! mp_ui->strict_mode->isChecked ());
}

void 
OASISWriterOptionPage::commit (db::FormatSpecificWriterOptions *o, const db::Technology * /*tech*/, bool gzip)
{
  if (gzip && mp_ui->write_cblocks->isChecked ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("gzip compression cannot be used with CBLOCK compression")));
  }

  if (mp_ui->subst_char->text ().size () > 1) {
    throw tl::Exception (tl::to_string (QObject::tr ("Substitution character must be either empty or exactly one character")));
  }

  db::OASISWriterOptions *options = dynamic_cast<db::OASISWriterOptions *> (o);
  if (options) {
    options->compression_level = mp_ui->compression_slider->value ();
    options->write_cblocks = mp_ui->write_cblocks->isChecked ();
    options->strict_mode = mp_ui->strict_mode->isChecked ();
    options->write_std_properties = mp_ui->std_prop_mode->currentIndex ();
    options->subst_char = tl::to_string (mp_ui->subst_char->text ());
    options->permissive = mp_ui->permissive->isChecked ();
  }
}

// ---------------------------------------------------------------
//  OASISWriterPluginDeclaration definition and implementation

class OASISWriterPluginDeclaration
  : public StreamWriterPluginDeclaration
{
public:
  OASISWriterPluginDeclaration () 
    : StreamWriterPluginDeclaration (db::OASISWriterOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamWriterOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new OASISWriterOptionPage (parent);
  }

  db::FormatSpecificWriterOptions *create_specific_options () const
  {
    return new db::OASISWriterOptions ();
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::OASISWriterPluginDeclaration (), 10000, "OASISWriter");

}




