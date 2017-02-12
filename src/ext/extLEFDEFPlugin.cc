
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


#include "tlTimer.h"
#include "tlStream.h"
#include "dbReader.h"
#include "dbStream.h"
#include "layPlugin.h"
#include "layStream.h"
#include "gsiDecl.h"

#include "extLEFDEFImportDialogs.h"
#include "extLEFImporter.h"
#include "extDEFImporter.h"
#include "extLEFDEFImporter.h"

#include <QFileInfo>
#include <QDir>

namespace ext
{

// ---------------------------------------------------------------
//  Plugin for the stream reader

/**
 *  @brief Determines the format of the given stream
 *  Returns true, if the stream has LEF format
 */
static bool is_lef_format (const std::string &fn)
{
  static const char *suffixes[] = { ".lef", ".LEF", ".lef.gz", ".LEF.gz" };

  //  NOTE: there is no reliable way of (easily) detecting the format. Hence we use the file
  //  name's suffix for the format hint.
  for (size_t i = 0; i < sizeof (suffixes) / sizeof (suffixes[0]); ++i) {
    std::string suffix = suffixes [i];
    if (fn.size () > suffix.size () && fn.find (suffix) == fn.size () - suffix.size ()) {
      return true;
    }
  }

  return false;
}

/**
 *  @brief Determines the format of the given stream
 *  Returns true, if the stream has DEF format
 */
static bool is_def_format (const std::string &fn)
{
  static const char *suffixes[] = { ".def", ".DEF", ".def.gz", ".DEF.gz" };

  //  NOTE: there is no reliable way of (easily) detecting the format. Hence we use the file
  //  name's suffix for the format hint.
  for (size_t i = 0; i < sizeof (suffixes) / sizeof (suffixes[0]); ++i) {
    std::string suffix = suffixes [i];
    if (fn.size () > suffix.size () && fn.find (suffix) == fn.size () - suffix.size ()) {
      return true;
    }
  }

  return false;
}

class LEFDEFReader
  : public db::ReaderBase
{
public:

  LEFDEFReader (tl::InputStream &s)
    : m_stream (s)
  {
    //  .. nothing yet ..
  }

  virtual const db::LayerMap &read (db::Layout &layout, const db::LoadLayoutOptions &options) throw (tl::Exception)
  {
    return read_lefdef (layout, options, is_lef_format (m_stream.filename ()));
  }

  virtual const db::LayerMap &read (db::Layout &layout) throw (tl::Exception)
  {
    return read_lefdef (layout, db::LoadLayoutOptions (), is_lef_format (m_stream.filename ()));
  }

  virtual const char *format () const
  {
    return "LEFDEF";
  }
private:
  tl::InputStream &m_stream;
  db::LayerMap m_layer_map;

  std::string correct_path (const std::string &fn)
  {
    QFileInfo fi (tl::to_qstring (fn));
    if (! fi.isAbsolute ()) {
      QDir input_dir (QFileInfo (tl::to_qstring (m_stream.absolute_path ())).dir ());
      return tl::to_string (input_dir.filePath (fi.filePath ()));
    } else {
      return fn;
    }
  }

  const db::LayerMap &read_lefdef (db::Layout &layout, const db::LoadLayoutOptions &options, bool import_lef) throw (tl::Exception)
  {
    const ext::LEFDEFReaderOptions *lefdef_options = dynamic_cast<const ext::LEFDEFReaderOptions *> (options.get_options (format ()));

    //  Take the layer map and the "read all layers" flag from the reader options - hence we override the
    ext::LEFDEFLayerDelegate layers (lefdef_options);
    layers.prepare (layout);
    layout.dbu (lefdef_options->dbu ());

    if (import_lef) {

      tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Reading LEF file")));

      ext::LEFImporter importer;

      for (std::vector<std::string>::const_iterator l = lefdef_options->begin_lef_files (); l != lefdef_options->end_lef_files (); ++l) {

        std::string lp = correct_path (*l);

        tl::InputStream lef_stream (lp);
        tl::log << tl::to_string (QObject::tr ("Reading")) << " " << lp;
        importer.read (lef_stream, layout, layers);

      }

      tl::log << tl::to_string (QObject::tr ("Reading")) << " " << m_stream.source ();
      importer.read (m_stream, layout, layers);

    } else {

      tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (QObject::tr ("Reading DEF file")));

      DEFImporter importer;

      for (std::vector<std::string>::const_iterator l = lefdef_options->begin_lef_files (); l != lefdef_options->end_lef_files (); ++l) {

        std::string lp = correct_path (*l);

        tl::InputStream lef_stream (lp);
        tl::log << tl::to_string (QObject::tr ("Reading")) << " " << lp;
        importer.read_lef (lef_stream, layout, layers);

      }

      //  Additionally read all LEF files next to the DEF file

      QDir input_dir (QFileInfo (tl::to_qstring (m_stream.absolute_path ())).dir ());
      if (input_dir.exists () && input_dir.isReadable ()) {

        QStringList entries = input_dir.entryList ();
        for (QStringList::const_iterator e = entries.begin (); e != entries.end (); ++e) {

          if (is_lef_format (tl::to_string (*e))) {

            std::string lp = tl::to_string (input_dir.filePath (*e));
            tl::InputStream lef_stream (lp);
            tl::log << tl::to_string (QObject::tr ("Reading")) << " " << lp;
            importer.read_lef (lef_stream, layout, layers);

          }

        }

      }

      tl::log << tl::to_string (QObject::tr ("Reading")) << " " << m_stream.source ();
      importer.read (m_stream, layout, layers);

    }

    layers.finish (layout);

    m_layer_map = layers.layer_map ();
    return m_layer_map;
  }
};

class LEFDEFFormatDeclaration
  : public db::StreamFormatDeclaration
{
  virtual std::string format_name () const { return "LEFDEF"; }
  virtual std::string format_desc () const { return "LEF/DEF"; }
  virtual std::string format_title () const { return "LEF/DEF (unified reader)"; }
  virtual std::string file_format () const { return "LEF/DEF files (*.lef *.LEF *.lef.gz *.LEF.gz *.def *.DEF *.def.gz *.DEF.gz)"; }

  virtual bool detect (tl::InputStream &stream) const
  {
    return is_lef_format (stream.filename ()) || is_def_format (stream.filename ());
  }

  virtual db::ReaderBase *create_reader (tl::InputStream &s) const
  {
    return new LEFDEFReader (s);
  }

  virtual db::WriterBase *create_writer () const
  {
    return 0;
  }

  virtual bool can_read () const
  {
    return true;
  }

  virtual bool can_write () const
  {
    return false;
  }
};

static tl::RegisteredClass<db::StreamFormatDeclaration> format_decl (new LEFDEFFormatDeclaration (), 500, "LEFDEF");

// ---------------------------------------------------------------
//  LEFDEFPluginDeclaration definition and implementation

class LEFDEFPluginDeclaration
  : public lay::StreamReaderPluginDeclaration
{
public:
  LEFDEFPluginDeclaration ()
    : lay::StreamReaderPluginDeclaration (LEFDEFReaderOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  lay::StreamReaderOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new LEFDEFReaderOptionsEditor (parent);
  }

  db::FormatSpecificReaderOptions *create_specific_options () const
  {
    return new LEFDEFReaderOptions ();
  }

  virtual tl::XMLElementBase *xml_element () const
  {
    return new lay::ReaderOptionsXMLElement<LEFDEFReaderOptions> ("lefdef",
      tl::make_member (&LEFDEFReaderOptions::read_all_layers, &LEFDEFReaderOptions::set_read_all_layers, "read-all-layers") +
      tl::make_member (&LEFDEFReaderOptions::layer_map, &LEFDEFReaderOptions::set_layer_map, "layer-map") +
      tl::make_member (&LEFDEFReaderOptions::dbu, &LEFDEFReaderOptions::set_dbu, "dbu") +
      tl::make_member (&LEFDEFReaderOptions::produce_net_names, &LEFDEFReaderOptions::set_produce_net_names, "produce-net-names") +
      tl::make_member (&LEFDEFReaderOptions::net_property_name, &LEFDEFReaderOptions::set_net_property_name, "net-property-name") +
      tl::make_member (&LEFDEFReaderOptions::produce_cell_outlines, &LEFDEFReaderOptions::set_produce_cell_outlines, "produce-cell-outlines") +
      tl::make_member (&LEFDEFReaderOptions::cell_outline_layer, &LEFDEFReaderOptions::set_cell_outline_layer, "cell-outline-layer") +
      tl::make_member (&LEFDEFReaderOptions::produce_placement_blockages, &LEFDEFReaderOptions::set_produce_placement_blockages, "produce-placement-blockages") +
      tl::make_member (&LEFDEFReaderOptions::placement_blockage_layer, &LEFDEFReaderOptions::set_placement_blockage_layer, "placement-blockage-layer") +
      tl::make_member (&LEFDEFReaderOptions::produce_via_geometry, &LEFDEFReaderOptions::set_produce_via_geometry, "produce-via-geometry") +
      tl::make_member (&LEFDEFReaderOptions::via_geometry_suffix, &LEFDEFReaderOptions::set_via_geometry_suffix, "via-geometry-suffix") +
      tl::make_member (&LEFDEFReaderOptions::via_geometry_datatype, &LEFDEFReaderOptions::set_via_geometry_datatype, "via-geometry-datatype") +
      tl::make_member (&LEFDEFReaderOptions::produce_pins, &LEFDEFReaderOptions::set_produce_pins, "produce-pins") +
      tl::make_member (&LEFDEFReaderOptions::pins_suffix, &LEFDEFReaderOptions::set_pins_suffix, "pins-suffix") +
      tl::make_member (&LEFDEFReaderOptions::pins_datatype, &LEFDEFReaderOptions::set_pins_datatype, "pins-datatype") +
      tl::make_member (&LEFDEFReaderOptions::produce_obstructions, &LEFDEFReaderOptions::set_produce_obstructions, "produce-obstructions") +
      tl::make_member (&LEFDEFReaderOptions::obstructions_suffix, &LEFDEFReaderOptions::set_obstructions_suffix, "obstructions-suffix") +
      tl::make_member (&LEFDEFReaderOptions::obstructions_datatype, &LEFDEFReaderOptions::set_obstructions_datatype, "obstructions-datatype") +
      tl::make_member (&LEFDEFReaderOptions::produce_blockages, &LEFDEFReaderOptions::set_produce_blockages, "produce-blockages") +
      tl::make_member (&LEFDEFReaderOptions::blockages_suffix, &LEFDEFReaderOptions::set_blockages_suffix, "blockages-suffix") +
      tl::make_member (&LEFDEFReaderOptions::blockages_datatype, &LEFDEFReaderOptions::set_blockages_datatype, "blockages-datatype") +
      tl::make_member (&LEFDEFReaderOptions::produce_labels, &LEFDEFReaderOptions::set_produce_labels, "produce-labels") +
      tl::make_member (&LEFDEFReaderOptions::labels_suffix, &LEFDEFReaderOptions::set_labels_suffix, "labels-suffix") +
      tl::make_member (&LEFDEFReaderOptions::labels_datatype, &LEFDEFReaderOptions::set_labels_datatype, "labels-datatype") +
      tl::make_member (&LEFDEFReaderOptions::produce_routing, &LEFDEFReaderOptions::set_produce_routing, "produce-routing") +
      tl::make_member (&LEFDEFReaderOptions::routing_suffix, &LEFDEFReaderOptions::set_routing_suffix, "routing-suffix") +
      tl::make_member (&LEFDEFReaderOptions::routing_datatype, &LEFDEFReaderOptions::set_routing_datatype, "routing-datatype") +
      tl::make_member (&LEFDEFReaderOptions::begin_lef_files, &LEFDEFReaderOptions::end_lef_files, &LEFDEFReaderOptions::push_lef_file, "lef-files")
    );
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new LEFDEFPluginDeclaration (), 10001, "LEFDEFReader");

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static LEFDEFReaderOptions &get_lefdef_config (db::LoadLayoutOptions *options)
{
  return options->get_options<LEFDEFReaderOptions> ();
}

static void set_lefdef_config (db::LoadLayoutOptions *options, const LEFDEFReaderOptions &config)
{
  options->set_options (config);
}

//  extend lay::LoadLayoutOptions with the GDS2 options
static
gsi::ClassExt<db::LoadLayoutOptions> decl_ext_lefdef_reader_options (
  gsi::method_ext ("lefdef_config", &get_lefdef_config,
    "@brief Gets a copy of the LEF/DEF reader configuration\n"
    "The LEF/DEF reader configuration is wrapped in a separate object of class \\LEFDEFReaderConfiguration. See there for details.\n"
    "This method will return a copy of the reader configuration. To modify the configuration, modify the copy and set the modified "
    "configuration with \\lefdef_config=.\n"
    "\n"
    "\nThis method has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("lefdef_config=", &set_lefdef_config, gsi::arg ("config"),
    "@brief Sets the LEF/DEF reader configuration\n"
    "\n"
    "\nThis method has been added in version 0.25.\n"
  )
);

static tl::Variant get_net_property_name (const LEFDEFReaderOptions *config)
{
  if (config->produce_net_names ()) {
    return config->net_property_name ();
  } else {
    return tl::Variant ();
  }
}

static void set_net_property_name (LEFDEFReaderOptions *config, const tl::Variant &name)
{
  config->set_produce_net_names (! name.is_nil ());
  config->set_net_property_name (name);
}

static
gsi::Class<LEFDEFReaderOptions> decl_lefdef_config ("LEFDEFReaderConfiguration",
  gsi::method ("layer_map", (db::LayerMap &(LEFDEFReaderOptions::*) ()) &LEFDEFReaderOptions::layer_map,
    "@brief Gets the layer map to be used for the LEF/DEF reader\n"
    "@return A reference to the layer map\n"
    "Because LEF/DEF layer mapping is substantially different than for normal layout files, the LEF/DEF reader "
    "employs a separate layer mapping table. The LEF/DEF specific layer mapping is stored within the "
    "LEF/DEF reader's configuration and can be accessed with this attribute. The layer mapping table of "
    "\\LoadLayoutOptions will be ignored for the LEF/DEF reader.\n"
    "\n"
    "The setter is \\layer_map=. \\create_other_layers= is available to control whether layers "
    "not specified in the layer mapping table shall be created automatically."
  ) +
  gsi::method ("layer_map=", &LEFDEFReaderOptions::set_layer_map,
    "@brief Sets the layer map to be used for the LEF/DEF reader\n"
    "See \\layer_map for details."
  ) +
  gsi::method ("create_other_layers", &LEFDEFReaderOptions::read_all_layers,
    "@brief Gets a value indicating whether layers not mapped in the layer map shall be created too\n"
    "See \\layer_map for details."
  ) +
  gsi::method ("create_other_layers=", &LEFDEFReaderOptions::set_read_all_layers,
    "@brief Sets a value indicating whether layers not mapped in the layer map shall be created too\n"
    "See \\layer_map for details."
  ) +
  gsi::method ("dbu", &LEFDEFReaderOptions::dbu,
    "@brief Gets the database unit to use for producing the layout.\n"
    "This value specifies the database to be used for the layout that is read. When a DEF file is specified with "
    "a different database unit, the layout is translated into this database unit.\n"
  ) +
  gsi::method ("dbu=", &LEFDEFReaderOptions::set_dbu, gsi::arg ("dbu"),
    "@brief Sets the database unit to use for producing the layout.\n"
    "See \\dbu for details."
  ) +
  gsi::method_ext ("net_property_name", &get_net_property_name,
    "@brief Gets a value indicating whether and how to produce net names as properties.\n"
    "If set to a value not nil, net names will be attached to the shapes and instances generated as user properties.\n"
    "This attribute then specifies the user property name to be used for attaching the net names.\n"
    "If set to nil, no net names will be produced.\n"
    "\n"
    "The corresponding setter is \\net_property_name=."
  ) +
  gsi::method_ext ("net_property_name=", &set_net_property_name, gsi::arg ("name"),
    "@brief Sets a value indicating whether and how to produce net names as properties.\n"
    "See \\net_property_name for details."
  ) +
  gsi::method ("produce_cell_outlines", &LEFDEFReaderOptions::produce_cell_outlines,
    "@brief Gets a value indicating whether to produce cell outlines.\n"
    "If set to true, cell outlines will be produced on the layer given by \\cell_outline_layer. "
  ) +
  gsi::method ("produce_cell_outlines=", &LEFDEFReaderOptions::set_produce_cell_outlines, gsi::arg ("produce"),
    "@brief Sets a value indicating whether to produce cell outlines.\n"
    "See \\produce_cell_outlines for details.\n"
  ) +
  gsi::method ("cell_outline_layer", &LEFDEFReaderOptions::cell_outline_layer,
    "@brief Gets the layer on which to produce the cell outline.\n"
    "This attribute is a string correspondig to the string representation of \\LayerInfo. "
    "This string can be either a layer number, a layer/datatype pair, a name or a combination of both. See \\LayerInfo for details.\n"
    "The setter for this attribute is \\cell_outline_layer=. See also \\produce_cell_outlines."
  ) +
  gsi::method ("cell_outline_layer=", &LEFDEFReaderOptions::set_cell_outline_layer, gsi::arg ("spec"),
    "@brief Sets the layer on which to produce the cell outline.\n"
    "See \\cell_outline_layer for details.\n"
  ) +
  gsi::method ("produce_placement_blockages", &LEFDEFReaderOptions::produce_placement_blockages,
    "@brief Gets a value indicating whether to produce placement blockage regions.\n"
    "If set to true, polygons will be produced representing the placement blockage region on the layer given by \\placement_blockage_layer. "
  ) +
  gsi::method ("produce_placement_blockages=", &LEFDEFReaderOptions::set_produce_placement_blockages, gsi::arg ("produce"),
    "@brief Sets a value indicating whether to produce placement blockage regions.\n"
    "See \\produce_placement_blockages for details.\n"
  ) +
  gsi::method ("placement_blockage_layer", &LEFDEFReaderOptions::placement_blockage_layer,
    "@brief Gets the layer on which to produce the placement blockage.\n"
    "This attribute is a string correspondig to the string representation of \\LayerInfo. "
    "This string can be either a layer number, a layer/datatype pair, a name or a combination of both. See \\LayerInfo for details."
    "The setter for this attribute is \\placement_blockage_layer=. See also \\produce_placement_blockages."
  ) +
  gsi::method ("placement_blockage_layer=", &LEFDEFReaderOptions::set_placement_blockage_layer,
    "@brief Sets the layer on which to produce the placement blockage.\n"
    "See \\placement_blockage_layer for details.\n"
  ) +
  gsi::method ("produce_via_geometry", &LEFDEFReaderOptions::produce_via_geometry,
    "@brief Sets a value indicating whether via geometries shall be produced.\n"
    "\n"
    "If set to true, shapes will be produced for each via. The layer to be produced will be determined from the "
    "via layer's name using the suffix provided by \\via_geometry_suffix. If there is a specific mapping in the "
    "layer mapping table for the via layer including the suffix, the layer/datatype will be taken from the layer "
    "mapping table. If there is a mapping to the undecorated via layer, the datatype will be substituted with "
    "the \\via_geometry_datatype value. If no mapping is defined, a unique number will be assigned to the layer "
    "number and the datatype will be taken from the \\via_geometry_datatype value.\n"
    "\n"
    "For example: the via layer is 'V1', \\via_geometry_suffix is 'GEO' and \\via_geometry_datatype is 1. Then:\n"
    "\n"
    "@li\n"
    "@ul If there is a mapping for 'V1.GEO', the layer and datatype will be taken from there. @/ul\n"
    "@ul If there is a mapping for 'V1', the layer will be taken from there and the datatype will be taken from \\via_geometry_datatype. "
    "    The name of the produced layer will be 'V1.GEO'. @/ul\n"
    "@ul If there is no mapping for both, the layer number will be a unique value, the datatype will be taken from \\via_geometry_datatype "
    "    and the layer name will be 'V1.GEO'. @/ul"
    "@/li\n"
  ) +
  gsi::method ("produce_via_geometry=", &LEFDEFReaderOptions::set_produce_via_geometry, gsi::arg ("produce"),
    "@brief Sets a value indicating whether via geometries shall be produced.\n"
    "See \\produce_via_geometry for details.\n"
  ) +
  gsi::method ("via_geometry_suffix", &LEFDEFReaderOptions::via_geometry_suffix,
    "@brief Gets the via geometry layer name suffix.\n"
    "See \\produce_via_geometry for details about this property.\n"
  ) +
  gsi::method ("via_geometry_suffix=", &LEFDEFReaderOptions::set_via_geometry_suffix, gsi::arg ("suffix"),
    "@brief Sets the via geometry layer name suffix.\n"
    "See \\produce_via_geometry for details about this property.\n"
  ) +
  gsi::method ("via_geometry_datatype", &LEFDEFReaderOptions::via_geometry_datatype,
    "@brief Gets the via geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about this property.\n"
  ) +
  gsi::method ("via_geometry_datatype=", &LEFDEFReaderOptions::set_via_geometry_datatype, gsi::arg ("datatype"),
    "@brief Sets the via geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about this property.\n"
  ) +
  gsi::method ("produce_pins", &LEFDEFReaderOptions::produce_pins,
    "@brief Gets a value indicating whether pin geometries shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_pins=", &LEFDEFReaderOptions::set_produce_pins, gsi::arg ("produce"),
    "@brief Sets a value indicating whether pin geometries shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("pins_suffix", &LEFDEFReaderOptions::pins_suffix,
    "@brief Gets the pin geometry layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("pins_suffix=", &LEFDEFReaderOptions::set_pins_suffix, gsi::arg ("suffix"),
    "@brief Sets the pin geometry layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("pins_datatype", &LEFDEFReaderOptions::pins_datatype,
    "@brief Gets the pin geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("pins_datatype=", &LEFDEFReaderOptions::set_pins_datatype, gsi::arg ("datatype"),
    "@brief Sets the pin geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_obstructions", &LEFDEFReaderOptions::produce_obstructions,
    "@brief Gets a value indicating whether obstruction markers shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_obstructions=", &LEFDEFReaderOptions::set_produce_obstructions, gsi::arg ("produce"),
    "@brief Sets a value indicating whether obstruction markers shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("obstructions_suffix", &LEFDEFReaderOptions::obstructions_suffix,
    "@brief Gets the obstruction marker layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("obstructions_suffix=", &LEFDEFReaderOptions::set_obstructions_suffix, gsi::arg ("suffix"),
    "@brief Sets the obstruction marker layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("obstructions_datatype", &LEFDEFReaderOptions::obstructions_datatype,
    "@brief Gets the obstruction marker layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("obstructions_datatype=", &LEFDEFReaderOptions::set_obstructions_datatype, gsi::arg ("datatype"),
    "@brief Sets the obstruction marker layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_blockages", &LEFDEFReaderOptions::produce_blockages,
    "@brief Gets a value indicating whether routing blockage markers shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_blockages=", &LEFDEFReaderOptions::set_produce_blockages, gsi::arg ("produce"),
    "@brief Sets a value indicating whether routing blockage markers shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("blockages_suffix", &LEFDEFReaderOptions::blockages_suffix,
    "@brief Gets the blockage marker layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("blockages_suffix=", &LEFDEFReaderOptions::set_blockages_suffix, gsi::arg ("suffix"),
    "@brief Sets the blockage marker layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("blockages_datatype", &LEFDEFReaderOptions::blockages_datatype,
    "@brief Gets the blockage marker layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("blockages_datatype=", &LEFDEFReaderOptions::set_blockages_datatype, gsi::arg ("datatype"),
    "@brief Sets the blockage marker layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_labels", &LEFDEFReaderOptions::produce_labels,
    "@brief Gets a value indicating whether labels shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_labels=", &LEFDEFReaderOptions::set_produce_labels, gsi::arg ("produce"),
    "@brief Sets a value indicating whether labels shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("labels_suffix", &LEFDEFReaderOptions::labels_suffix,
    "@brief Gets the label layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("labels_suffix=", &LEFDEFReaderOptions::set_labels_suffix, gsi::arg ("suffix"),
    "@brief Sets the label layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("labels_datatype", &LEFDEFReaderOptions::labels_datatype,
    "@brief Gets the labels layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("labels_datatype=", &LEFDEFReaderOptions::set_labels_datatype, gsi::arg ("datatype"),
    "@brief Sets the labels layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_routing", &LEFDEFReaderOptions::produce_routing,
    "@brief Gets a value indicating whether routing geometry shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_routing=", &LEFDEFReaderOptions::set_produce_routing, gsi::arg ("produce"),
    "@brief Sets a value indicating whether routing geometry shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("routing_suffix", &LEFDEFReaderOptions::routing_suffix,
    "@brief Gets the routing layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("routing_suffix=", &LEFDEFReaderOptions::set_routing_suffix, gsi::arg ("suffix"),
    "@brief Sets the routing layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("routing_datatype", &LEFDEFReaderOptions::routing_datatype,
    "@brief Gets the routing layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("routing_datatype=", &LEFDEFReaderOptions::set_routing_datatype, gsi::arg ("datatype"),
    "@brief Sets the routing layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("lef_files", &LEFDEFReaderOptions::lef_files,
    "@brief Gets the list technology LEF files to additionally import\n"
    "Returns a list of path names for technology LEF files to read in addition to the primary file. "
    "Relative paths are resolved relative to the file to read.\n"
    "\n"
    "The setter for this property is \\lef_files=."
  ) +
  gsi::method ("lef_files=", &LEFDEFReaderOptions::set_lef_files,
    "@brief Sets the list technology LEF files to additionally import\n"
    "See \\lef_files for details."
  ),
  "@brief Detailed LEF/DEF reader options\n"
  "This class is a aggregate belonging to the \\LoadLayoutOptions class. It provides options for the LEF/DEF reader. "
  "These options have been placed into a separate class to account for their complexity."
);

}

