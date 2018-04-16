
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



#include "dbGDS2.h"
#include "dbGDS2Reader.h"
#include "dbLoadLayoutOptions.h"
#include "layGDS2ReaderPlugin.h"
#include "ui_GDS2ReaderOptionPage.h"
#include "gsiDecl.h"

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
GDS2ReaderOptionPage::setup (const db::FormatSpecificReaderOptions *o, const lay::Technology * /*tech*/)
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
GDS2ReaderOptionPage::commit (db::FormatSpecificReaderOptions *o, const lay::Technology * /*tech*/)
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

  virtual tl::XMLElementBase *xml_element () const
  {
    return new lay::ReaderOptionsXMLElement<db::GDS2ReaderOptions> ("gds2",
      tl::make_member (&db::GDS2ReaderOptions::box_mode, "box-mode") +
      tl::make_member (&db::GDS2ReaderOptions::allow_big_records, "allow-big-records") +
      tl::make_member (&db::GDS2ReaderOptions::allow_multi_xy_records, "allow-multi-xy-records")
    );
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::GDS2ReaderPluginDeclaration (), 10000, "GDS2Reader");

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_gds2_box_mode (db::LoadLayoutOptions *options, unsigned int n)
{
  options->get_options<db::GDS2ReaderOptions> ().box_mode = n;
}

static unsigned int get_gds2_box_mode (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::GDS2ReaderOptions> ().box_mode;
}

static void set_gds2_allow_multi_xy_records (db::LoadLayoutOptions *options, bool n)
{
  options->get_options<db::GDS2ReaderOptions> ().allow_multi_xy_records = n;
}

static bool get_gds2_allow_multi_xy_records (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::GDS2ReaderOptions> ().allow_multi_xy_records;
}

static void set_gds2_allow_big_records (db::LoadLayoutOptions *options, bool n)
{
  options->get_options<db::GDS2ReaderOptions> ().allow_big_records = n;
}

static bool get_gds2_allow_big_records (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::GDS2ReaderOptions> ().allow_big_records;
}

//  extend lay::LoadLayoutOptions with the GDS2 options 
static
gsi::ClassExt<db::LoadLayoutOptions> gds2_reader_options (
  gsi::method_ext ("gds2_box_mode=", &set_gds2_box_mode,
    "@brief Sets a value specifying how to treat BOX records\n"
    "This property specifies how BOX records are treated.\n"
    "Allowed values are 0 (ignore), 1 (treat as rectangles), 2 (treat as boundaries) or 3 (treat as errors). The default is 1.\n"
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_box_mode", &get_gds2_box_mode,
    "@brief Gets a value specifying how to treat BOX records\n"
    "See \\gds2_box_mode= method for a description of this mode."
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_allow_multi_xy_records=", &set_gds2_allow_multi_xy_records,
    "@brief Allows the use of multiple XY records in BOUNDARY elements for unlimited large polygons\n"
    "\n"
    "Setting this property to true allows big polygons that span over multiple XY records.\n"
    "For strict compatibility with the standard, this property should be set to false. The default is true.\n"
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_allow_multi_xy_records?|#gds2_allow_multi_xy_records", &get_gds2_allow_multi_xy_records,
    "@brief Gets a value specifying whether to allow big polygons with multiple XY records.\n"
    "See \\gds2_allow_multi_xy_records= method for a description of this property."
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_allow_big_records=", &set_gds2_allow_big_records,
    "@brief Allows big records with more than 32767 bytes\n"
    "\n"
    "Setting this property to true allows larger records by treating the record length as unsigned short, which for example "
    "allows larger polygons (~8000 points rather than ~4000 points) without using multiple XY records.\n"
    "For strict compatibility with the standard, this property should be set to false. The default is true.\n"
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_allow_big_records?|#gds2_allow_big_records", &get_gds2_allow_big_records,
    "@brief Gets a value specifying whether to allow big records with a length of 32768 to 65535 bytes.\n"
    "See \\gds2_allow_big_records= method for a description of this property."
    "\nThis property has been added in version 0.18.\n"
  ),
  ""
);

}





