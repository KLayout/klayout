
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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
#include "gsiDecl.h"

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
DXFWriterOptionPage::setup (const db::FormatSpecificWriterOptions *o, const lay::Technology * /*tech*/)
{
  const db::DXFWriterOptions *options = dynamic_cast<const db::DXFWriterOptions *> (o);
  if (options) {
    mp_ui->polygon_mode_cbx->setCurrentIndex (options->polygon_mode);
  }
}

void 
DXFWriterOptionPage::commit (db::FormatSpecificWriterOptions *o, const lay::Technology * /*tech*/, bool /*gzip*/)
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

  virtual tl::XMLElementBase *xml_element () const
  {
    return new lay::WriterOptionsXMLElement<db::DXFWriterOptions> ("cif",
      tl::make_member (&db::DXFWriterOptions::polygon_mode, "polygon-mode")
    );
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::DXFWriterPluginDeclaration (), 10000, "DXFWriter");

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_dxf_polygon_mode (db::SaveLayoutOptions *options, int mode)
{
  if (mode < 0 || mode > 4) {
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid polygon mode")));
  }

  options->get_options<db::DXFWriterOptions> ().polygon_mode = mode;
}

static int get_dxf_polygon_mode (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::DXFWriterOptions> ().polygon_mode;
}

//  extend lay::SaveLayoutOptions with the DXF options 
static
gsi::ClassExt<db::SaveLayoutOptions> dxf_writer_options (
  gsi::method_ext ("dxf_polygon_mode=", &set_dxf_polygon_mode,
    "@brief Specifies how to write polygons.\n"
    "@args mode\n"
    "The mode is 0 (write POLYLINE entities), 1 (write LWPOLYLINE entities), 2 (decompose into SOLID entities), "
    "3 (write HATCH entities), or 4 (write LINE entities).\n"
    "\nThis property has been added in version 0.21.3. '4', in version 0.25.6.\n"
  ) +
  gsi::method_ext ("dxf_polygon_mode", &get_dxf_polygon_mode,
    "@brief Specifies how to write polygons.\n"
    "See \\dxf_polygon_mode= for a description of this property.\n"
    "\nThis property has been added in version 0.21.3.\n"
  ),
  ""
);

}





