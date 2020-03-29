
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
#include "tlFileUtils.h"

#include "dbReader.h"
#include "dbStream.h"
#include "dbLEFImporter.h"
#include "dbDEFImporter.h"
#include "dbLEFDEFImporter.h"

namespace db
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

/**
 *  @brief Reads a map file
 *
 *  NOTE: this is rather experimental ... no idea what is the specification of
 *  the map file.
 */
static void
read_map_file (const std::string &path, db::LEFDEFLayerDelegate &layers)
{
  tl::log << tl::to_string (tr ("Reading LEF/DEF map file")) << " " << path;

  db::LayerMap &lm = layers.layer_map ();
  unsigned int n = lm.next_index ();

  tl::InputFile file (path);
  tl::InputStream file_stream (file);
  tl::TextInputStream ts (file_stream);

  std::map<std::string, std::string> purpose_translation;
  purpose_translation ["LEFPIN"] = "PIN";
  purpose_translation ["LEFOBS"] = "OBS";
  purpose_translation ["SPNET"] = "NET";
  purpose_translation ["NET"] = "NET";
  purpose_translation ["VIA"] = "VIA";

  while (! ts.at_end ()) {

    const std::string &l = ts.get_line ();

    tl::Extractor ex (l.c_str ());
    if (ex.at_end () || ex.test ("#")) {

      //  ignore empty of comment lines

    } else {

      std::string w1, w2;
      int layer = 0, datatype = 0;

      if (ex.try_read_word (w1) && ex.try_read_word (w2, "._$,/:") && ex.try_read (layer) && ex.try_read (datatype)) {

        if (w1 == "DIEAREA") {

          std::string canonical_name = "(OUTLINE)";
          lm.map (db::LayerProperties (canonical_name), n++, db::LayerProperties (layer, datatype));

        } else if (w1 == "NAME") {

          std::vector<std::string> purposes = tl::split (w2, ",");
          for (std::vector<std::string>::const_iterator p = purposes.begin (); p != purposes.end (); ++p) {
            std::string canonical_name = std::string ("(") + tl::split (*p, "/").front () + ",LABEL)";
            lm.map (db::LayerProperties (canonical_name), n++, db::LayerProperties (layer, datatype));
          }

        } else {

          std::vector<std::string> purposes = tl::split (w2, ",");
          for (std::vector<std::string>::const_iterator p = purposes.begin (); p != purposes.end (); ++p) {
            std::map<std::string, std::string>::const_iterator i = purpose_translation.find (*p);
            if (i != purpose_translation.end ()) {
              std::string canonical_name = std::string ("(") + w1 + "," + i->second + ")";
              lm.map (db::LayerProperties (canonical_name), n++, db::LayerProperties (layer, datatype));
            }
          }

        }

      }

    }

  }
}

/**
 *  @brief Imports a .map file present next to the input files
 */
static void
import_map_file_heuristics (const std::string &main_path, db::LEFDEFLayerDelegate &layers)
{
  std::string input_dir = tl::absolute_path (main_path);
  if (! tl::file_exists (input_dir)) {
    return;
  }

  std::string bn = tl::basename (tl::filename (main_path));
  std::vector<std::string> map_files;
  std::string map_file_exact;

  std::vector<std::string> entries = tl::dir_entries (input_dir);
  for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {

    if (tl::to_lower_case (tl::extension (*e)) == "map") {

      if (tl::basename (*e) == bn) {
        map_file_exact = *e;
      } else {
        map_files.push_back (*e);
      }

    }

  }

  try {
    if (! map_file_exact.empty ()) {
      read_map_file (tl::combine_path (input_dir, map_file_exact), layers);
      tl::log << layers.layer_map ().to_string_file_format (); // @@@
    } else if (map_files.size () == 1) {
      read_map_file (tl::combine_path (input_dir, map_files.front ()), layers);
      tl::log << layers.layer_map ().to_string_file_format (); // @@@
    }
  } catch (tl::Exception &ex) {
    //  ignore read errors on map file (this is a heuristics!)
    tl::error << ex.msg ();
  }
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

  virtual const db::LayerMap &read (db::Layout &layout, const db::LoadLayoutOptions &options)
  {
    return read_lefdef (layout, options, is_lef_format (m_stream.filename ()));
  }

  virtual const db::LayerMap &read (db::Layout &layout)
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
    if (! tl::is_absolute (fn)) {
      return tl::combine_path (m_stream.absolute_path (), fn);
    } else {
      return fn;
    }
  }

  const db::LayerMap &read_lefdef (db::Layout &layout, const db::LoadLayoutOptions &options, bool import_lef)
  {
    const db::LEFDEFReaderOptions *lefdef_options = dynamic_cast<const db::LEFDEFReaderOptions *> (options.get_options (format ()));
    static db::LEFDEFReaderOptions default_options;
    if (! lefdef_options) {
      lefdef_options = &default_options;
    }

    //  Take the layer map and the "read all layers" flag from the reader options - hence we override the
    db::LEFDEFLayerDelegate layers (lefdef_options);

    import_map_file_heuristics (m_stream.absolute_path (), layers);

    layers.prepare (layout);
    layout.dbu (lefdef_options->dbu ());

    if (import_lef) {

      tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Reading LEF file")));

      db::LEFImporter importer;

      for (std::vector<std::string>::const_iterator l = lefdef_options->begin_lef_files (); l != lefdef_options->end_lef_files (); ++l) {

        std::string lp = correct_path (*l);

        tl::InputStream lef_stream (lp);
        tl::log << tl::to_string (tr ("Reading")) << " " << lp;
        importer.read (lef_stream, layout, layers);

      }

      tl::log << tl::to_string (tr ("Reading")) << " " << m_stream.source ();
      importer.read (m_stream, layout, layers);

    } else {

      tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Reading DEF file")));

      DEFImporter importer;

      for (std::vector<std::string>::const_iterator l = lefdef_options->begin_lef_files (); l != lefdef_options->end_lef_files (); ++l) {

        std::string lp = correct_path (*l);

        tl::InputStream lef_stream (lp);
        tl::log << tl::to_string (tr ("Reading")) << " " << lp;
        importer.read_lef (lef_stream, layout, layers);

      }

      //  Additionally read all LEF files next to the DEF file and if there is a single .map file
      //  or one with the same name than the input file with ".map" suffix, try to read this one too.

      std::string input_dir = tl::absolute_path (m_stream.absolute_path ());

      if (tl::file_exists (input_dir)) {

        std::vector<std::string> entries = tl::dir_entries (input_dir);
        for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {

          if (is_lef_format (*e)) {

            std::string lp = tl::combine_path (input_dir, *e);
            tl::InputStream lef_stream (lp);
            tl::log << tl::to_string (tr ("Reading")) << " " << lp;
            importer.read_lef (lef_stream, layout, layers);

          }

        }

      }

      tl::log << tl::to_string (tr ("Reading")) << " " << m_stream.source ();
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
    return new db::LEFDEFReader (s);
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

  virtual tl::XMLElementBase *xml_reader_options_element () const
  {
    return new db::ReaderOptionsXMLElement<LEFDEFReaderOptions> ("lefdef",
      tl::make_member (&LEFDEFReaderOptions::read_all_layers, &LEFDEFReaderOptions::set_read_all_layers, "read-all-layers") +
      tl::make_member (&LEFDEFReaderOptions::layer_map, &LEFDEFReaderOptions::set_layer_map, "layer-map") +
      tl::make_member (&LEFDEFReaderOptions::dbu, &LEFDEFReaderOptions::set_dbu, "dbu") +
      tl::make_member (&LEFDEFReaderOptions::produce_net_names, &LEFDEFReaderOptions::set_produce_net_names, "produce-net-names") +
      tl::make_member (&LEFDEFReaderOptions::net_property_name, &LEFDEFReaderOptions::set_net_property_name, "net-property-name") +
      tl::make_member (&LEFDEFReaderOptions::produce_inst_names, &LEFDEFReaderOptions::set_produce_inst_names, "produce-inst-names") +
      tl::make_member (&LEFDEFReaderOptions::inst_property_name, &LEFDEFReaderOptions::set_inst_property_name, "inst-property-name") +
      tl::make_member (&LEFDEFReaderOptions::produce_pin_names, &LEFDEFReaderOptions::set_produce_pin_names, "produce-pin-names") +
      tl::make_member (&LEFDEFReaderOptions::pin_property_name, &LEFDEFReaderOptions::set_pin_property_name, "pin-property-name") +
      tl::make_member (&LEFDEFReaderOptions::produce_cell_outlines, &LEFDEFReaderOptions::set_produce_cell_outlines, "produce-cell-outlines") +
      tl::make_member (&LEFDEFReaderOptions::cell_outline_layer, &LEFDEFReaderOptions::set_cell_outline_layer, "cell-outline-layer") +
      tl::make_member (&LEFDEFReaderOptions::produce_placement_blockages, &LEFDEFReaderOptions::set_produce_placement_blockages, "produce-placement-blockages") +
      tl::make_member (&LEFDEFReaderOptions::placement_blockage_layer, &LEFDEFReaderOptions::set_placement_blockage_layer, "placement-blockage-layer") +
      tl::make_member (&LEFDEFReaderOptions::produce_regions, &LEFDEFReaderOptions::set_produce_regions, "produce-regions") +
      tl::make_member (&LEFDEFReaderOptions::region_layer, &LEFDEFReaderOptions::set_region_layer, "region-layer") +
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

static tl::RegisteredClass<db::StreamFormatDeclaration> format_decl (new LEFDEFFormatDeclaration (), 500, "LEFDEF");

}

