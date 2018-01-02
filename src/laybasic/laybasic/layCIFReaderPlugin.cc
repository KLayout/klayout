
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
CIFReaderOptionPage::setup (const db::FormatSpecificReaderOptions *o, const lay::Technology * /*tech*/)
{
  static const db::CIFReaderOptions default_options;
  const db::CIFReaderOptions *options = dynamic_cast<const db::CIFReaderOptions *> (o);
  if (!options) {
    options = &default_options;
  }

  mp_ui->dbu_le->setText (tl::to_qstring (tl::to_string (options->dbu)));
  mp_ui->layer_map->set_layer_map (options->layer_map);
  mp_ui->read_all_cbx->setChecked (options->create_other_layers);
  mp_ui->wire_mode_cb->setCurrentIndex (options->wire_mode);
}

void 
CIFReaderOptionPage::commit (db::FormatSpecificReaderOptions *o, const lay::Technology * /*tech*/)
{
  db::CIFReaderOptions *options = dynamic_cast<db::CIFReaderOptions *> (o);
  if (options) {
    tl::from_string (tl::to_string (mp_ui->dbu_le->text ()), options->dbu);
    if (options->dbu > 1000.0 || options->dbu < 1e-9) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for database unit")));
    }
    options->wire_mode = mp_ui->wire_mode_cb->currentIndex ();
    options->layer_map = mp_ui->layer_map->get_layer_map ();
    options->create_other_layers = mp_ui->read_all_cbx->isChecked ();
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

  virtual tl::XMLElementBase *xml_element () const
  {
    return new lay::ReaderOptionsXMLElement<db::CIFReaderOptions> ("cif",
      tl::make_member (&db::CIFReaderOptions::wire_mode, "wire-mode") +
      tl::make_member (&db::CIFReaderOptions::dbu, "dbu") +
      tl::make_member (&db::CIFReaderOptions::layer_map, "layer-map") +
      tl::make_member (&db::CIFReaderOptions::create_other_layers, "create-other-layers")
    );
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::CIFReaderPluginDeclaration (), 10000, "CIFReader");

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_cif_wire_mode (db::LoadLayoutOptions *options, unsigned int n)
{
  options->get_options<db::CIFReaderOptions> ().wire_mode = n;
}

static unsigned int get_cif_wire_mode (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CIFReaderOptions> ().wire_mode;
}

static void set_cif_dbu (db::LoadLayoutOptions *options, double dbu)
{
  options->get_options<db::CIFReaderOptions> ().dbu = dbu;
}

static double get_cif_dbu (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CIFReaderOptions> ().dbu;
}

static void set_layer_map (db::LoadLayoutOptions *options, const db::LayerMap &lm, bool f)
{
  options->get_options<db::CIFReaderOptions> ().layer_map = lm;
  options->get_options<db::CIFReaderOptions> ().create_other_layers = f;
}

static db::LayerMap &get_layer_map (db::LoadLayoutOptions *options)
{
  return options->get_options<db::CIFReaderOptions> ().layer_map;
}

static void select_all_layers (db::LoadLayoutOptions *options)
{
  options->get_options<db::CIFReaderOptions> ().layer_map = db::LayerMap ();
  options->get_options<db::CIFReaderOptions> ().create_other_layers = true;
}

static bool create_other_layers (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CIFReaderOptions> ().create_other_layers;
}

static void set_create_other_layers (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::CIFReaderOptions> ().create_other_layers = l;
}

//  extend lay::LoadLayoutOptions with the CIF options
static
gsi::ClassExt<db::LoadLayoutOptions> cif_reader_options (
    gsi::method_ext ("cif_set_layer_map", &set_layer_map, gsi::arg ("map"), gsi::arg ("create_other_layers"),
      "@brief Sets the layer map\n"
      "This sets a layer mapping for the reader. The \"create_other_layers\" specifies whether to create layers that are not "
      "in the mapping and automatically assign layers to them.\n"
      "@param map The layer map to set."
      "@param create_other_layers The flag telling whether other layer should be created also. Set to false if just the layers in the mapping table should be read.\n"
      "\n"
      "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
      "in a format-specific fashion."
    ) +
    gsi::method_ext ("cif_select_all_layers", &select_all_layers,
      "@brief Selects all layers and disables the layer map\n"
      "\n"
      "This disables any layer map and enables reading of all layers.\n"
      "New layers will be created when required.\n"
      "\n"
      "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
      "in a format-specific fashion."
    ) +
    gsi::method_ext ("cif_layer_map", &get_layer_map,
      "@brief Gets the layer map\n"
      "@return A reference to the layer map\n"
      "\n"
      "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
      "in a format-specific fashion."
    ) +
    gsi::method_ext ("cif_create_other_layers?", &create_other_layers,
      "@brief Gets a value indicating whether other layers shall be created\n"
      "@return True, if other layers should be created.\n"
      "\n"
      "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
      "in a format-specific fashion."
    ) +
    gsi::method_ext ("cif_create_other_layers=", &set_create_other_layers, gsi::arg ("create"),
      "@brief Specifies whether other layers shall be created\n"
      "@param create True, if other layers should be created.\n"
      "\n"
      "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
      "in a format-specific fashion."
    ) +
    gsi::method_ext ("cif_wire_mode=", &set_cif_wire_mode,
    "@brief How to read 'W' objects\n"
    "\n"
    "This property specifies how to read 'W' (wire) objects.\n"
    "Allowed values are 0 (as square ended paths), 1 (as flush ended paths), 2 (as round paths)\n"
    "\nThis property has been added in version 0.21.\n"
  ) +
  gsi::method_ext ("cif_wire_mode", &get_cif_wire_mode,
    "@brief Specifies how to read 'W' objects\n"
    "See \\cif_wire_mode= method for a description of this mode."
    "\nThis property has been added in version 0.21 and was renamed to cif_wire_mode in 0.25.\n"
  ) +
  gsi::method_ext ("cif_dbu=", &set_cif_dbu,
    "@brief Specifies the database unit which the reader uses and produces\n"
    "\nThis property has been added in version 0.21.\n"
  ) +
  gsi::method_ext ("cif_dbu", &get_cif_dbu,
    "@brief Specifies the database unit which the reader uses and produces\n"
    "See \\cif_dbu= method for a description of this property."
    "\nThis property has been added in version 0.21.\n"
  ),
  ""
);

}





