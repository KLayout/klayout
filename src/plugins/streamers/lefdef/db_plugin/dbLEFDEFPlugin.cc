
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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
#include "dbLayoutUtils.h"
#include "dbTechnology.h"
#include "dbCellMapping.h"

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

// ---------------------------------------------------------------
//  LEFDEFReader implementation

LEFDEFReader::LEFDEFReader (tl::InputStream &s)
  : m_stream (s)
{
  //  .. nothing yet ..
}

const db::LayerMap &
LEFDEFReader::read (db::Layout &layout, const db::LoadLayoutOptions &options)
{
  return read_lefdef (layout, options, is_lef_format (m_stream.filename ()));
}

const db::LayerMap &
LEFDEFReader::read (db::Layout &layout)
{
  return read_lefdef (layout, db::LoadLayoutOptions (), is_lef_format (m_stream.filename ()));
}

const char *
LEFDEFReader::format () const
{
  return "LEFDEF";
}

const db::LayerMap &
LEFDEFReader::read_lefdef (db::Layout &layout, const db::LoadLayoutOptions &options, bool import_lef)
{
  init (options);

  const db::LEFDEFReaderOptions *lefdef_options = dynamic_cast<const db::LEFDEFReaderOptions *> (options.get_options (format ()));
  db::LEFDEFReaderOptions effective_options;
  if (lefdef_options) {
    effective_options = *lefdef_options;
  }

  std::string base_path;
  if (! effective_options.paths_relative_to_cwd ()) {
    base_path = tl::dirname (m_stream.absolute_path ());
  }

  db::LEFDEFReaderState state (&effective_options, layout, base_path);

  db::CommonReaderOptions common_options = options.get_options<db::CommonReaderOptions> ();
  state.set_conflict_resolution_mode (common_options.cell_conflict_resolution);

  layout.dbu (effective_options.dbu ());

  if (import_lef) {

    //  Always produce LEF geometry when reading LEF
    effective_options.set_macro_resolution_mode (1);

    tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Reading LEF file")));

    db::LEFImporter importer (warn_level ());

    for (std::vector<std::string>::const_iterator l = effective_options.begin_lef_files (); l != effective_options.end_lef_files (); ++l) {

      std::string lp = correct_path (*l, layout, base_path);

      tl::InputStream lef_stream (lp);
      tl::log << tl::to_string (tr ("Reading")) << " " << lp;
      importer.read (lef_stream, layout, state);

    }

    tl::log << tl::to_string (tr ("Reading")) << " " << m_stream.source ();
    importer.read (m_stream, layout, state);

    importer.finish_lef (layout);

  } else {

    tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Reading DEF file")));

    DEFImporter importer (warn_level ());

    for (std::vector<std::string>::const_iterator l = effective_options.begin_lef_files (); l != effective_options.end_lef_files (); ++l) {

      std::string lp = correct_path (*l, layout, base_path);

      tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Reading LEF file: ")) + lp);

      tl::InputStream lef_stream (lp);
      tl::log << tl::to_string (tr ("Reading")) << " " << lp;
      importer.read_lef (lef_stream, layout, state);

    }

    //  Additionally read all LEF files next to the DEF file

    if (effective_options.read_lef_with_def ()) {

      std::string input_dir = tl::absolute_path (m_stream.absolute_path ());

      if (tl::file_exists (input_dir)) {

        std::vector<std::string> entries = tl::dir_entries (input_dir, true, false, true);
        for (std::vector<std::string>::const_iterator e = entries.begin (); e != entries.end (); ++e) {

          if (is_lef_format (*e)) {

            std::string lp = tl::combine_path (input_dir, *e);

            tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Reading LEF file: ")) + lp);

            tl::InputStream lef_stream (lp);
            tl::log << tl::to_string (tr ("Reading")) << " " << lp;
            importer.read_lef (lef_stream, layout, state);

          }

        }

      }

    }

    tl::log << tl::to_string (tr ("Reading")) << " " << m_stream.source ();
    importer.read (m_stream, layout, state);

    //  Resolve unresolved COMPONENT cells

    std::map<std::string, db::cell_index_type> foreign_cells = state.foreign_cells ();
    db::cell_index_type seen = std::numeric_limits<db::cell_index_type>::max ();

    std::vector<db::Layout *> macro_layouts = effective_options.macro_layouts ();

    //  Additionally read the layouts from the given paths
    tl::shared_collection<db::Layout> macro_layout_object_holder;
    for (std::vector<std::string>::const_iterator l = effective_options.begin_macro_layout_files (); l != effective_options.end_macro_layout_files (); ++l) {

      std::string lp = correct_path (*l, layout, base_path);

      tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Reading LEF macro layout file: ")) + lp);

      tl::InputStream macro_layout_stream (lp);
      tl::log << tl::to_string (tr ("Reading")) << " " << lp;
      db::Layout *new_layout = new db::Layout (false);
      macro_layout_object_holder.push_back (new_layout);
      macro_layouts.push_back (new_layout);

      db::Reader reader (macro_layout_stream);
      reader.read (*new_layout, options);

    }

    for (std::vector<db::Layout *>::const_iterator m = macro_layouts.begin (); m != macro_layouts.end (); ++m) {

      std::vector<db::cell_index_type> target_cells, source_cells;

      //  collect the cells to pull in
      for (std::map<std::string, db::cell_index_type>::iterator f = foreign_cells.begin (); f != foreign_cells.end (); ++f) {
        if (f->second != seen) {
          std::pair<bool, db::cell_index_type> cp = (*m)->cell_by_name (f->first.c_str ());
          if (cp.first) {
            target_cells.push_back (f->second);
            source_cells.push_back (cp.second);
            layout.cell (f->second).set_ghost_cell (false);
            f->second = seen;
          }
        }
      }

      db::CellMapping cm;
      cm.create_multi_mapping_full (layout, target_cells, **m, source_cells);
      layout.copy_tree_shapes (**m, cm);

    }

  }

  state.finish (layout);

  m_layer_map = state.layer_map ();
  return m_layer_map;
}

// ---------------------------------------------------------------

namespace {

  struct MacroResolutionModeConverter
  {
  public:
    MacroResolutionModeConverter ()
    {
      m_values.push_back ("default");
      m_values.push_back ("always-lef");
      m_values.push_back ("always-cellref");
    }

    std::string to_string (unsigned int v) const
    {
      return v < m_values.size () ? m_values[v] : std::string ();
    }

    void from_string (const std::string &s, unsigned int &v) const
    {
      v = 0;
      for (unsigned int i = 0; i < (unsigned int) m_values.size (); ++i) {
        if (m_values [i] == s) {
          v = i;
        }
      }
    }

  private:
    std::vector<std::string> m_values;
  };

}

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
      //  for backward compatibility
      tl::make_member (&LEFDEFReaderOptions::set_via_geometry_suffix, "special-via_geometry-suffix") +
      tl::make_member (&LEFDEFReaderOptions::set_via_geometry_datatype, "special-via_geometry-datatype") +
      tl::make_member (&LEFDEFReaderOptions::set_via_geometry_suffix_str, "special-via_geometry-suffix-string") +
      tl::make_member (&LEFDEFReaderOptions::set_via_geometry_datatype_str, "special-via_geometry-datatype-string") +
      //  new:
      tl::make_member (&LEFDEFReaderOptions::via_geometry_suffix_str, &LEFDEFReaderOptions::set_via_geometry_suffix_str, "via_geometry-suffix-string") +
      tl::make_member (&LEFDEFReaderOptions::via_geometry_datatype_str, &LEFDEFReaderOptions::set_via_geometry_datatype_str, "via_geometry-datatype-string") +

      tl::make_member (&LEFDEFReaderOptions::produce_pins, &LEFDEFReaderOptions::set_produce_pins, "produce-pins") +
      //  for backward compatibility
      tl::make_member (&LEFDEFReaderOptions::set_pins_suffix, "special-pins-suffix") +
      tl::make_member (&LEFDEFReaderOptions::set_pins_datatype, "special-pins-datatype") +
      tl::make_member (&LEFDEFReaderOptions::set_pins_suffix_str, "special-pins-suffix-string") +
      tl::make_member (&LEFDEFReaderOptions::set_pins_datatype_str, "special-pins-datatype-string") +
      //  new:
      tl::make_member (&LEFDEFReaderOptions::pins_suffix_str, &LEFDEFReaderOptions::set_pins_suffix_str, "pins-suffix-string") +
      tl::make_member (&LEFDEFReaderOptions::pins_datatype_str, &LEFDEFReaderOptions::set_pins_datatype_str, "pins-datatype-string") +

      tl::make_member (&LEFDEFReaderOptions::produce_lef_pins, &LEFDEFReaderOptions::set_produce_lef_pins, "produce-lef-pins") +
      //  for backward compatibility
      tl::make_member (&LEFDEFReaderOptions::set_lef_pins_suffix, "special-lef_pins-suffix") +
      tl::make_member (&LEFDEFReaderOptions::set_lef_pins_datatype, "special-lef_pins-datatype") +
      tl::make_member (&LEFDEFReaderOptions::set_lef_pins_suffix_str, "special-lef_pins-suffix-string") +
      tl::make_member (&LEFDEFReaderOptions::set_lef_pins_datatype_str, "special-lef_pins-datatype-string") +
      //  new:
      tl::make_member (&LEFDEFReaderOptions::lef_pins_suffix_str, &LEFDEFReaderOptions::set_lef_pins_suffix_str, "lef_pins-suffix-string") +
      tl::make_member (&LEFDEFReaderOptions::lef_pins_datatype_str, &LEFDEFReaderOptions::set_lef_pins_datatype_str, "lef_pins-datatype-string") +

      tl::make_member (&LEFDEFReaderOptions::produce_fills, &LEFDEFReaderOptions::set_produce_fills, "produce-fills") +
      //  for backward compatibility
      tl::make_member (&LEFDEFReaderOptions::set_fills_suffix, "special-fills-suffix") +
      tl::make_member (&LEFDEFReaderOptions::set_fills_datatype, "special-fills-datatype") +
      tl::make_member (&LEFDEFReaderOptions::set_fills_suffix_str, "special-fills-suffix-string") +
      tl::make_member (&LEFDEFReaderOptions::set_fills_datatype_str, "special-fills-datatype-string") +
      //  new:
      tl::make_member (&LEFDEFReaderOptions::fills_suffix_str, &LEFDEFReaderOptions::set_fills_suffix_str, "fills-suffix-string") +
      tl::make_member (&LEFDEFReaderOptions::fills_datatype_str, &LEFDEFReaderOptions::set_fills_datatype_str, "fills-datatype-string") +

      tl::make_member (&LEFDEFReaderOptions::produce_obstructions, &LEFDEFReaderOptions::set_produce_obstructions, "produce-obstructions") +
      tl::make_member (&LEFDEFReaderOptions::obstructions_suffix, &LEFDEFReaderOptions::set_obstructions_suffix, "obstructions-suffix") +
      tl::make_member (&LEFDEFReaderOptions::obstructions_datatype, &LEFDEFReaderOptions::set_obstructions_datatype, "obstructions-datatype") +

      tl::make_member (&LEFDEFReaderOptions::produce_blockages, &LEFDEFReaderOptions::set_produce_blockages, "produce-blockages") +
      tl::make_member (&LEFDEFReaderOptions::blockages_suffix, &LEFDEFReaderOptions::set_blockages_suffix, "blockages-suffix") +
      tl::make_member (&LEFDEFReaderOptions::blockages_datatype, &LEFDEFReaderOptions::set_blockages_datatype, "blockages-datatype") +

      tl::make_member (&LEFDEFReaderOptions::produce_labels, &LEFDEFReaderOptions::set_produce_labels, "produce-labels") +
      tl::make_member (&LEFDEFReaderOptions::labels_suffix, &LEFDEFReaderOptions::set_labels_suffix, "labels-suffix") +
      tl::make_member (&LEFDEFReaderOptions::labels_datatype, &LEFDEFReaderOptions::set_labels_datatype, "labels-datatype") +
      tl::make_member (&LEFDEFReaderOptions::produce_lef_labels, &LEFDEFReaderOptions::set_produce_lef_labels, "produce-lef-labels") +
      tl::make_member (&LEFDEFReaderOptions::lef_labels_suffix, &LEFDEFReaderOptions::set_lef_labels_suffix, "lef-labels-suffix") +
      tl::make_member (&LEFDEFReaderOptions::lef_labels_datatype, &LEFDEFReaderOptions::set_lef_labels_datatype, "lef-labels-datatype") +

      tl::make_member (&LEFDEFReaderOptions::produce_routing, &LEFDEFReaderOptions::set_produce_routing, "produce-routing") +
      tl::make_member (&LEFDEFReaderOptions::routing_suffix_str, &LEFDEFReaderOptions::set_routing_suffix_str, "routing-suffix-string") +
      tl::make_member (&LEFDEFReaderOptions::routing_datatype_str, &LEFDEFReaderOptions::set_routing_datatype_str, "routing-datatype-string") +

      tl::make_member (&LEFDEFReaderOptions::produce_special_routing, &LEFDEFReaderOptions::set_produce_special_routing, "produce-special-routing") +
      //  for backward compatibility
      tl::make_member (&LEFDEFReaderOptions::set_special_routing_suffix, "special-routing-suffix") +
      tl::make_member (&LEFDEFReaderOptions::set_special_routing_datatype, "special-routing-datatype") +
      //  new:
      tl::make_member (&LEFDEFReaderOptions::special_routing_suffix_str, &LEFDEFReaderOptions::set_special_routing_suffix_str, "special-routing-suffix-string") +
      tl::make_member (&LEFDEFReaderOptions::special_routing_datatype_str, &LEFDEFReaderOptions::set_special_routing_datatype_str, "special-routing-datatype-string") +

      tl::make_member (&LEFDEFReaderOptions::via_cellname_prefix, &LEFDEFReaderOptions::set_via_cellname_prefix, "via-cellname-prefix") +
      tl::make_member (&LEFDEFReaderOptions::begin_lef_files, &LEFDEFReaderOptions::end_lef_files, &LEFDEFReaderOptions::push_lef_file, "lef-files") +
      tl::make_member (&LEFDEFReaderOptions::begin_macro_layout_files, &LEFDEFReaderOptions::end_macro_layout_files, &LEFDEFReaderOptions::push_macro_layout_file, "macro_layout-files") +
      tl::make_member (&LEFDEFReaderOptions::read_lef_with_def, &LEFDEFReaderOptions::set_read_lef_with_def, "read-lef-with-def") +
      tl::make_member (&LEFDEFReaderOptions::macro_resolution_mode, &LEFDEFReaderOptions::set_macro_resolution_mode, "macro-resolution-mode", MacroResolutionModeConverter ()) +
      tl::make_member (&LEFDEFReaderOptions::separate_groups, &LEFDEFReaderOptions::set_separate_groups, "separate-groups") +
      tl::make_member (&LEFDEFReaderOptions::joined_paths, &LEFDEFReaderOptions::set_joined_paths, "joined-paths") +
      tl::make_member (&LEFDEFReaderOptions::map_file, &LEFDEFReaderOptions::set_map_file, "map-file")
    );
  }
};

static tl::RegisteredClass<db::StreamFormatDeclaration> format_decl (new LEFDEFFormatDeclaration (), 500, "LEFDEF");

}

