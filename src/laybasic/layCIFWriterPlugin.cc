
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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
#include "gsiDecl.h"

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
CIFWriterOptionPage::setup (const db::FormatSpecificWriterOptions *o, const lay::Technology * /*tech*/)
{
  const db::CIFWriterOptions *options = dynamic_cast<const db::CIFWriterOptions *> (o);
  if (options) {
    mp_ui->dummy_calls_cbx->setChecked (options->dummy_calls);
    mp_ui->blank_separator_cbx->setChecked (options->blank_separator);
  }
}

void 
CIFWriterOptionPage::commit (db::FormatSpecificWriterOptions *o, const lay::Technology * /*tech*/, bool /*gzip*/)
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

  virtual tl::XMLElementBase *xml_element () const
  {
    return new lay::WriterOptionsXMLElement<db::CIFWriterOptions> ("cif",
      tl::make_member (&db::CIFWriterOptions::dummy_calls, "dummy-calls") +
      tl::make_member (&db::CIFWriterOptions::blank_separator, "blank-separator")
    );
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::CIFWriterPluginDeclaration (), 10000, "CIFWriter");

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_cif_dummy_calls (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<db::CIFWriterOptions> ().dummy_calls = f;
}

static bool get_cif_dummy_calls (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::CIFWriterOptions> ().dummy_calls;
}

static void set_cif_blank_separator (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<db::CIFWriterOptions> ().blank_separator = f;
}

static bool get_cif_blank_separator (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::CIFWriterOptions> ().blank_separator;
}

//  extend lay::SaveLayoutOptions with the GDS2 options 
static
gsi::ClassExt<db::SaveLayoutOptions> cif_writer_options (
  gsi::method_ext ("cif_dummy_calls=", &set_cif_dummy_calls,
    "@brief Sets a flag indicating whether dummy calls shall be written\n"
    "If this property is set to true, dummy calls will be written in the top level entity "
    "of the CIF file calling every top cell.\n"
    "This option is useful for enhanced compatibility with other tools.\n"
    "\nThis property has been added in version 0.23.10.\n"
  ) +
  gsi::method_ext ("cif_dummy_calls", &get_cif_dummy_calls,
    "@brief Gets a flag indicating whether dummy calls shall be written\n"
    "See \\cif_dummy_calls= method for a description of that property."
    "\nThis property has been added in version 0.23.10.\n"
  ) +
  gsi::method_ext ("cif_blank_separator=", &set_cif_blank_separator,
    "@brief Sets a flag indicating whether blanks shall be used as x/y separator characters\n"
    "If this property is set to true, the x and y coordinates are separated with blank characters "
    "rather than comma characters."
    "\nThis property has been added in version 0.23.10.\n"
  ) +
  gsi::method_ext ("cif_blank_separator", &get_cif_blank_separator,
    "@brief Gets a flag indicating whether blanks shall be used as x/y separator characters\n"
    "See \\cif_blank_separator= method for a description of that property."
    "\nThis property has been added in version 0.23.10.\n"
  ),
  ""
);

}

