
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



#include "dbCommonReader.h"
#include "dbLoadLayoutOptions.h"
#include "layCommonReaderPlugin.h"
#include "ui_CommonReaderOptionsPage.h"
#include "gsiDecl.h"

#include <QFrame>

namespace lay
{

// ---------------------------------------------------------------
//  CommonReaderOptionPage definition and implementation

CommonReaderOptionPage::CommonReaderOptionPage (QWidget *parent)
  : StreamReaderOptionsPage (parent)
{
  mp_ui = new Ui::CommonReaderOptionPage ();
  mp_ui->setupUi (this);
}

CommonReaderOptionPage::~CommonReaderOptionPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
CommonReaderOptionPage::setup (const db::FormatSpecificReaderOptions *o, const lay::Technology * /*tech*/)
{
  static const db::CommonReaderOptions default_options;
  const db::CommonReaderOptions *options = dynamic_cast<const db::CommonReaderOptions *> (o);
  if (!options) {
    options = &default_options;
  }

  mp_ui->enable_text_cbx->setChecked (options->enable_text_objects);
  mp_ui->enable_properties_cbx->setChecked (options->enable_properties);
  mp_ui->layer_map->set_layer_map (options->layer_map);
  mp_ui->read_all_cbx->setChecked (options->create_other_layers);
}

void 
CommonReaderOptionPage::commit (db::FormatSpecificReaderOptions *o, const lay::Technology * /*tech*/)
{
  db::CommonReaderOptions *options = dynamic_cast<db::CommonReaderOptions *> (o);
  if (options) {

    options->enable_text_objects = mp_ui->enable_text_cbx->isChecked ();
    options->enable_properties = mp_ui->enable_properties_cbx->isChecked ();
    options->layer_map = mp_ui->layer_map->get_layer_map ();
    options->create_other_layers = mp_ui->read_all_cbx->isChecked ();

  }
}

// ---------------------------------------------------------------
//  CommonReaderPluginDeclaration definition and implementation

class CommonReaderPluginDeclaration
  : public StreamReaderPluginDeclaration
{
public:
  CommonReaderPluginDeclaration () 
    : StreamReaderPluginDeclaration (db::CommonReaderOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamReaderOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new CommonReaderOptionPage (parent);
  }

  db::FormatSpecificReaderOptions *create_specific_options () const
  {
    return new db::CommonReaderOptions ();
  }

  virtual tl::XMLElementBase *xml_element () const
  {
    return new lay::ReaderOptionsXMLElement<db::CommonReaderOptions> ("common",
      tl::make_member (&db::CommonReaderOptions::create_other_layers, "create-other-layers") +
      tl::make_member (&db::CommonReaderOptions::layer_map, "layer-map") +
      tl::make_member (&db::CommonReaderOptions::enable_properties, "enable-properties") +
      tl::make_member (&db::CommonReaderOptions::enable_text_objects, "enable-text-objects")
    );
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::CommonReaderPluginDeclaration (), 10000, "CommonReader");

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_layer_map (db::LoadLayoutOptions *options, const db::LayerMap &lm, bool f)
{
  options->get_options<db::CommonReaderOptions> ().layer_map = lm;
  options->get_options<db::CommonReaderOptions> ().create_other_layers = f;
}

static db::LayerMap &get_layer_map (db::LoadLayoutOptions *options)
{
  return options->get_options<db::CommonReaderOptions> ().layer_map;
}

static void select_all_layers (db::LoadLayoutOptions *options)
{
  options->get_options<db::CommonReaderOptions> ().layer_map = db::LayerMap ();
  options->get_options<db::CommonReaderOptions> ().create_other_layers = true;
}

static bool create_other_layers (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CommonReaderOptions> ().create_other_layers;
}

static void set_create_other_layers (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::CommonReaderOptions> ().create_other_layers = l;
}

static bool get_text_enabled (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CommonReaderOptions> ().enable_text_objects;
}

static void set_text_enabled (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::CommonReaderOptions> ().enable_text_objects = l;
}

static bool get_properties_enabled (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CommonReaderOptions> ().enable_properties;
}

static void set_properties_enabled (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::CommonReaderOptions> ().enable_properties = l;
}

//  extend lay::LoadLayoutOptions with the Common options 
static
gsi::ClassExt<db::LoadLayoutOptions> common_reader_options (
  gsi::method_ext ("set_layer_map", &set_layer_map, gsi::arg ("map"), gsi::arg ("create_other_layers"),
    "@brief Sets the layer map\n"
    "This sets a layer mapping for the reader. The \"create_other_layers\" specifies whether to create layers that are not "
    "in the mapping and automatically assign layers to them.\n"
    "@param map The layer map to set."
    "@param create_other_layers The flag telling whether other layer should be created also. Set to false if just the layers in the mapping table should be read.\n"
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("select_all_layers", &select_all_layers,
    "@brief Selects all layers and disables the layer map\n"
    "\n"
    "This disables any layer map and enables reading of all layers.\n"
    "New layers will be created when required.\n"
    "\n"
    "Starting with version 0.25 this method only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("layer_map", &get_layer_map,
    "@brief Gets the layer map\n"
    "@return A reference to the layer map\n"
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("create_other_layers?", &create_other_layers,
    "@brief Gets a value indicating whether other layers shall be created\n"
    "@return True, if other layers should be created.\n"
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("create_other_layers=", &set_create_other_layers, gsi::arg ("create"),
    "@brief Specifies whether other layers shall be created\n"
    "@param create True, if other layers should be created.\n"
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("text_enabled?|#is_text_enabled?", &get_text_enabled,
    "@brief Gets a value indicating whether text objects shall be read\n"
    "@return True, if text objects should be read."
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("text_enabled=", &set_text_enabled, gsi::arg ("enabled"),
    "@brief Specifies whether text objects shall be read\n"
    "@param enabled True, if text objects should be read."
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("properties_enabled?|#is_properties_enabled?", &get_properties_enabled,
    "@brief Gets a value indicating whether properties shall be read\n"
    "@return True, if properties should be read."
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("properties_enabled=", &set_properties_enabled, gsi::arg ("enabled"),
    "@brief Specifies whether properties should be read\n"
    "@args enabled\n"
    "@param enabled True, if properties should be read."
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ),
  ""
);

}





