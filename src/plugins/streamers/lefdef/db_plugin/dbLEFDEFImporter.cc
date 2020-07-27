
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


#include "dbLEFDEFImporter.h"
#include "dbLayoutUtils.h"
#include "dbTechnology.h"

#include "tlStream.h"
#include "tlProgress.h"
#include "tlFileUtils.h"

#include <cctype>

namespace db
{

// -----------------------------------------------------------------------------------
//  Path resolution utility

std::string correct_path (const std::string &fn, const db::Layout &layout, const std::string &base_path)
{
  if (! tl::is_absolute (fn)) {

    //  if a technology is given and the file can be found in the technology's base path, take it
    //  from there.
    std::string tn = layout.meta_info_value ("technology");
    const db::Technology *tech = 0;
    if (! tn.empty ()) {
      tech = db::Technologies::instance ()->technology_by_name (tn);
    }

    if (tech && ! tech->base_path ().empty ()) {
      std::string new_fn = tl::combine_path (tech->base_path (), fn);
      if (tl::file_exists (new_fn)) {
        return new_fn;
      }
    }

    if (! base_path.empty ()) {
      return tl::combine_path (base_path, fn);
    } else {
      return fn;
    }

  } else {
    return fn;
  }
}

// -----------------------------------------------------------------------------------
//  Utilities

static bool is_hex_digit (char c)
{
  char cup = toupper (c);
  return (cup >= 'A' && cup <= 'F') || (c >= '0' && c <= '9');
}

static int hex_value (char c)
{
  char cup = toupper (c);
  if (cup >= 'A' && cup <= 'F') {
    return (cup - 'A') + 10;
  } else if (c >= '0' && c <= '9') {
    return c - '0';
  } else {
    return 0;
  }
}

// -----------------------------------------------------------------------------------
//  RuleBasedViaGenerator implementation

RuleBasedViaGenerator::RuleBasedViaGenerator ()
  : LEFDEFViaGenerator (), m_bottom_mask (0), m_cut_mask (0), m_top_mask (0), m_rows (1), m_columns (1)
{ }

void
RuleBasedViaGenerator::create_cell (LEFDEFReaderState &reader, Layout &layout, db::Cell &cell, unsigned int mask_bottom, unsigned int mask_cut, unsigned int mask_top, const LEFDEFNumberOfMasks *nm)
{
  if (mask_bottom == 0) {
    mask_bottom = m_bottom_mask;
  }
  if (mask_cut == 0) {
    mask_cut = m_cut_mask;
  }
  if (mask_top == 0) {
    mask_top = m_top_mask;
  }

  unsigned int num_cut_masks = nm ? nm->number_of_masks (m_cut_layer) : 1;

  //  NOTE: missing cuts due to pattern holes don't change mask assignment

  db::Vector vs ((m_cutsize.x () * m_columns + m_cutspacing.x () * (m_columns - 1)) / 2, (m_cutsize.y () * m_rows + m_cutspacing.y () * (m_rows - 1)) / 2);
  db::Box via_box (m_offset - vs, m_offset + vs);

  std::pair <bool, unsigned int> dl (false, 0);

  dl = reader.open_layer (layout, m_bottom_layer, ViaGeometry, mask_bottom);
  if (dl.first) {
    cell.shapes (dl.second).insert (db::Polygon (via_box.enlarged (m_be).moved (m_bo)));
  }

  dl = reader.open_layer (layout, m_top_layer, ViaGeometry, mask_top);
  if (dl.first) {
    cell.shapes (dl.second).insert (db::Polygon (via_box.enlarged (m_te).moved (m_bo)));
  }

  const char *p = m_pattern.c_str ();
  int rp = m_pattern.empty () ? -1 : 0;
  const char *p0 = p, *p1 = p;

  for (int r = 0; r < m_rows; ++r) {

    if (rp == 0) {

      if (*p) {

        //  read a new row specification
        rp = 0;
        while (*p && is_hex_digit (*p)) {
          rp = (rp * 16) + hex_value (*p++);
        }
        if (*p == '_') {
          ++p;
        }

        p0 = p;
        if (*p) {
          while (*p && (is_hex_digit (*p) || toupper (*p) == 'R')) {
            ++p;
          }
        }
        p1 = p;
        if (*p == '_') {
          ++p;
        }

      }

    }

    if (rp != 0) {

      if (rp > 0) {
        --rp;
      }

      const char *pp = p0;
      unsigned int d = 0;
      int cp = (p == p0 ? -1 : 0);
      int bit = 0;

      for (int c = 0; c < m_columns; ++c) {

        if (cp == 0) {

          d = 0;
          cp = 4;
          bit = 0;

          if (*pp && pp < p1 && toupper (*pp) == 'R') {

            ++pp;
            if (*pp && pp < p1) {
              cp = 4 * hex_value (*pp++);
              if (*pp && pp < p1) {
                d = (unsigned int) hex_value (*pp++);
              }
            }

          } else if (*pp && pp < p1) {

            d = (unsigned int) hex_value (*pp++);

          }

          if (cp > 0) {
            --cp;
          }

        } else if (cp > 0) {

          --cp;

        } else {

          d = 0xf;

        }

        if ((d & (0x8 >> (bit++ % 4))) != 0) {

          db::Vector vbl ((m_cutsize + m_cutspacing).x () * c, (m_cutsize + m_cutspacing).y () * r);
          db::Box vb (via_box.lower_left () + vbl, via_box.lower_left () + vbl + m_cutsize);

          unsigned int cm = 0;
          if (mask_cut > 0) {
            //  This is the core algorithm for mask assignment in patterned vias
            cm = (mask_cut + r + c - 1) % num_cut_masks + 1;
          }

          dl = reader.open_layer (layout, m_cut_layer, ViaGeometry, cm);
          if (dl.first) {
            cell.shapes (dl.second).insert (db::Polygon (vb));
          }

        }

      }

    }

  }
}

// -----------------------------------------------------------------------------------
//  GeometryBasedViaGenerator implementation

GeometryBasedViaGenerator::GeometryBasedViaGenerator ()
  : LEFDEFViaGenerator ()
{ }

unsigned int
GeometryBasedViaGenerator::mask_for (const std::string &ln, unsigned int m, unsigned int mask_bottom, unsigned int mask_cut, unsigned int mask_top) const
{
  if (m == 0) {
    if (ln == m_bottom_layer) {
      return (mask_bottom);
    } else if (ln == m_cut_layer) {
      return (mask_cut);
    } else if (ln == m_top_layer) {
      return (mask_top);
    }
  }

  return m;
}

void
GeometryBasedViaGenerator::create_cell (LEFDEFReaderState &reader, Layout &layout, db::Cell &cell, unsigned int mask_bottom, unsigned int mask_cut, unsigned int mask_top, const LEFDEFNumberOfMasks * /*nm*/)
{
  for (std::map <std::string, std::list<std::pair<unsigned int, db::Polygon> > >::const_iterator g = m_polygons.begin (); g != m_polygons.end (); ++g) {
    for (std::list<std::pair<unsigned int, db::Polygon> >::const_iterator i = g->second.begin (); i != g->second.end (); ++i) {
      std::pair <bool, unsigned int> dl = reader.open_layer (layout, g->first, ViaGeometry, mask_for (g->first, i->first, mask_bottom, mask_cut, mask_top));
      if (dl.first) {
        cell.shapes (dl.second).insert (i->second);
      }
    }
  }

  for (std::map <std::string, std::list<std::pair<unsigned int, db::Box> > >::const_iterator g = m_boxes.begin (); g != m_boxes.end (); ++g) {
    for (std::list<std::pair<unsigned int, db::Box> >::const_iterator i = g->second.begin (); i != g->second.end (); ++i) {
      std::pair <bool, unsigned int> dl = reader.open_layer (layout, g->first, ViaGeometry, mask_for (g->first, i->first, mask_bottom, mask_cut, mask_top));
      if (dl.first) {
        cell.shapes (dl.second).insert (i->second);
      }
    }
  }
}

void
GeometryBasedViaGenerator::add_polygon (const std::string &ln, const db::Polygon &poly, unsigned int mask)
{
  m_polygons [ln].push_back (std::make_pair (mask, poly));
}

void
GeometryBasedViaGenerator::add_box (const std::string &ln, const db::Box &box, unsigned int mask)
{
  m_boxes [ln].push_back (std::make_pair (mask, box));
}

// -----------------------------------------------------------------------------------
//  LEFDEFTechnologyComponent implementation

LEFDEFReaderOptions::LEFDEFReaderOptions ()
  : m_read_all_layers (true),
    m_dbu (0.001), 
    m_produce_net_names (true),
    m_net_property_name (1),
    m_produce_inst_names (true),
    m_inst_property_name (1),
    m_produce_pin_names (false),
    m_pin_property_name (1),
    m_produce_cell_outlines (true),
    m_cell_outline_layer ("OUTLINE"),
    m_produce_placement_blockages (true),
    m_placement_blockage_layer ("PLACEMENT_BLK"),
    m_produce_regions (true),
    m_region_layer ("REGIONS"),
    m_produce_via_geometry (true),
    m_via_geometry_suffix (""),
    m_via_geometry_datatype (0),
    m_via_cellname_prefix ("VIA_"),
    m_produce_pins (true),
    m_pins_suffix (".PIN"),
    m_pins_datatype (2),
    m_produce_lef_pins (true),
    m_lef_pins_suffix (".PIN"),
    m_lef_pins_datatype (2),
    m_produce_obstructions (true),
    m_obstructions_suffix (".OBS"),
    m_obstructions_datatype (3),
    m_produce_blockages (true),
    m_blockages_suffix (".BLK"),
    m_blockages_datatype (4),
    m_produce_labels (true),
    m_labels_suffix (".LABEL"),
    m_labels_datatype (1),
    m_produce_routing (true),
    m_routing_suffix (""),
    m_routing_datatype (0),
    m_produce_special_routing (true),
    m_special_routing_suffix (""),
    m_special_routing_datatype (0),
    m_separate_groups (false),
    m_map_file (),
    m_macro_resolution_mode (0)
{
  //  .. nothing yet ..
}

LEFDEFReaderOptions::LEFDEFReaderOptions (const LEFDEFReaderOptions &d)
  : db::FormatSpecificReaderOptions ()
{
  operator= (d);
}

LEFDEFReaderOptions &LEFDEFReaderOptions::operator= (const LEFDEFReaderOptions &d)
{
  if (this != &d) {
    db::FormatSpecificReaderOptions::operator= (d);
    m_read_all_layers = d.m_read_all_layers;
    m_layer_map = d.m_layer_map;
    m_dbu = d.m_dbu;
    m_produce_net_names = d.m_produce_net_names;
    m_net_property_name = d.m_net_property_name;
    m_produce_inst_names = d.m_produce_inst_names;
    m_inst_property_name = d.m_inst_property_name;
    m_produce_pin_names = d.m_produce_pin_names;
    m_pin_property_name = d.m_pin_property_name;
    m_produce_cell_outlines = d.m_produce_cell_outlines;
    m_cell_outline_layer = d.m_cell_outline_layer;
    m_produce_placement_blockages = d.m_produce_placement_blockages;
    m_placement_blockage_layer = d.m_placement_blockage_layer;
    m_produce_regions = d.m_produce_regions;
    m_region_layer = d.m_region_layer;
    m_produce_via_geometry = d.m_produce_via_geometry;
    m_via_geometry_suffix = d.m_via_geometry_suffix;
    m_via_geometry_suffixes = d.m_via_geometry_suffixes;
    m_via_geometry_datatype = d.m_via_geometry_datatype;
    m_via_geometry_datatypes = d.m_via_geometry_datatypes;
    m_via_cellname_prefix = d.m_via_cellname_prefix;
    m_produce_pins = d.m_produce_pins;
    m_pins_suffix = d.m_pins_suffix;
    m_pins_suffixes = d.m_pins_suffixes;
    m_pins_datatype = d.m_pins_datatype;
    m_pins_datatypes = d.m_pins_datatypes;
    m_produce_lef_pins = d.m_produce_lef_pins;
    m_lef_pins_suffix = d.m_lef_pins_suffix;
    m_lef_pins_suffixes = d.m_lef_pins_suffixes;
    m_lef_pins_datatype = d.m_lef_pins_datatype;
    m_lef_pins_datatypes = d.m_lef_pins_datatypes;
    m_produce_obstructions = d.m_produce_obstructions;
    m_obstructions_suffix = d.m_obstructions_suffix;
    m_obstructions_datatype = d.m_obstructions_datatype;
    m_produce_blockages = d.m_produce_blockages;
    m_blockages_suffix = d.m_blockages_suffix;
    m_blockages_datatype = d.m_blockages_datatype;
    m_produce_labels = d.m_produce_labels;
    m_labels_suffix = d.m_labels_suffix;
    m_labels_datatype = d.m_labels_datatype;
    m_produce_routing = d.m_produce_routing;
    m_routing_suffix = d.m_routing_suffix;
    m_routing_suffixes = d.m_routing_suffixes;
    m_routing_datatype = d.m_routing_datatype;
    m_routing_datatypes = d.m_routing_datatypes;
    m_produce_special_routing = d.m_produce_special_routing;
    m_special_routing_suffix = d.m_special_routing_suffix;
    m_special_routing_suffixes = d.m_special_routing_suffixes;
    m_special_routing_datatype = d.m_special_routing_datatype;
    m_special_routing_datatypes = d.m_special_routing_datatypes;
    m_separate_groups = d.m_separate_groups;
    m_map_file = d.m_map_file;
    m_macro_resolution_mode = d.m_macro_resolution_mode;
    m_lef_files = d.m_lef_files;
  }
  return *this;
}

db::FormatSpecificReaderOptions *
LEFDEFReaderOptions::clone () const
{
  return new LEFDEFReaderOptions (*this);
}

const std::string &
LEFDEFReaderOptions::format_name () const
{
  static const std::string n ("LEFDEF");
  return n;
}

static void set_datatypes (db::LEFDEFReaderOptions *data, void (db::LEFDEFReaderOptions::*clear) (), void (db::LEFDEFReaderOptions::*set_datatype) (int datatype), void (db::LEFDEFReaderOptions::*set_datatype_per_mask) (unsigned int mask, int datatype), const std::string &s)
{
  (data->*clear) ();

  tl::Extractor ex (s.c_str ());

  while (! ex.at_end ()) {

    tl::Extractor ex_saved = ex;

    unsigned int mask = 0;
    if (ex.try_read (mask) && ex.test (":")) {
      int dt = 0;
      ex.read (dt);
      (data->*set_datatype_per_mask) (std::max ((unsigned int) 1, mask), dt);
    } else {
      ex = ex_saved;
      int dt = 0;
      ex.read (dt);
      (data->*set_datatype) (dt);
    }

    if (ex.at_end ()) {
      break;
    } else {
      ex.expect (",");
    }

  }
}

static void set_suffixes (db::LEFDEFReaderOptions *data, void (db::LEFDEFReaderOptions::*clear) (), void (db::LEFDEFReaderOptions::*set_suffix) (const std::string &suffix), void (db::LEFDEFReaderOptions::*set_suffix_per_mask) (unsigned int mask, const std::string &suffix), const std::string &s)
{
  (data->*clear) ();

  tl::Extractor ex (s.c_str ());

  while (! ex.at_end ()) {

    tl::Extractor ex_saved = ex;

    unsigned int mask = 0;
    if (ex.try_read (mask) && ex.test (":")) {
      std::string sfx;
      ex.read_word_or_quoted (sfx);
      (data->*set_suffix_per_mask) (std::max ((unsigned int) 1, mask), sfx);
    } else {
      ex = ex_saved;
      std::string sfx;
      ex.read_word_or_quoted (sfx);
      (data->*set_suffix) (sfx);
    }

    if (ex.at_end ()) {
      break;
    } else {
      ex.expect (",");
    }

  }
}

static std::string get_datatypes (const db::LEFDEFReaderOptions *data, int (db::LEFDEFReaderOptions::*get_datatype) () const, int (db::LEFDEFReaderOptions::*get_datatype_per_mask) (unsigned int mask) const, unsigned int max_mask)
{
  std::string res;
  int dt0 = (data->*get_datatype) ();
  if (dt0 >= 0) {
    res += tl::to_string (dt0);
  }

  for (unsigned int i = 0; i <= max_mask; ++i) {
    int dt = (data->*get_datatype_per_mask) (i);
    if (dt >= 0 && dt != dt0) {
      if (! res.empty ()) {
        res += ",";
      }
      res += tl::to_string (i);
      res += ":";
      res += tl::to_string (dt);
    }
  }

  return res;
}

static std::string get_suffixes (const db::LEFDEFReaderOptions *data, const std::string &(db::LEFDEFReaderOptions::*get_suffix) () const, const std::string &(db::LEFDEFReaderOptions::*get_suffix_per_mask) (unsigned int mask) const, unsigned int max_mask)
{
  std::string res;
  std::string sfx0 = (data->*get_suffix) ();
  if (! sfx0.empty ()) {
    res += tl::to_word_or_quoted_string (sfx0);
  }

  for (unsigned int i = 0; i <= max_mask; ++i) {
    std::string sfx = (data->*get_suffix_per_mask) (i);
    if (! sfx.empty () && sfx != sfx0) {
      if (! res.empty ()) {
        res += ",";
      }
      res += tl::to_string (i);
      res += ":";
      res += tl::to_word_or_quoted_string (sfx);
    }
  }

  return res;
}

void
LEFDEFReaderOptions::set_via_geometry_suffix_str (const std::string &s)
{
  set_suffixes (this, &LEFDEFReaderOptions::clear_via_geometry_suffixes_per_mask, &LEFDEFReaderOptions::set_via_geometry_suffix, &LEFDEFReaderOptions::set_via_geometry_suffix_per_mask, s);
}

std::string
LEFDEFReaderOptions::via_geometry_suffix_str () const
{
  return get_suffixes (this, &LEFDEFReaderOptions::via_geometry_suffix, &LEFDEFReaderOptions::via_geometry_suffix_per_mask, max_mask_number ());
}

void
LEFDEFReaderOptions::set_via_geometry_datatype_str (const std::string &s)
{
  set_datatypes (this, &LEFDEFReaderOptions::clear_via_geometry_datatypes_per_mask, &LEFDEFReaderOptions::set_via_geometry_datatype, &LEFDEFReaderOptions::set_via_geometry_datatype_per_mask, s);
}

std::string
LEFDEFReaderOptions::via_geometry_datatype_str () const
{
  return get_datatypes (this, &LEFDEFReaderOptions::via_geometry_datatype, &LEFDEFReaderOptions::via_geometry_datatype_per_mask, max_mask_number ());
}

void
LEFDEFReaderOptions::set_pins_suffix_str (const std::string &s)
{
  set_suffixes (this, &LEFDEFReaderOptions::clear_pins_suffixes_per_mask, &LEFDEFReaderOptions::set_pins_suffix, &LEFDEFReaderOptions::set_pins_suffix_per_mask, s);
}

std::string
LEFDEFReaderOptions::pins_suffix_str () const
{
  return get_suffixes (this, &LEFDEFReaderOptions::pins_suffix, &LEFDEFReaderOptions::pins_suffix_per_mask, max_mask_number ());
}

void
LEFDEFReaderOptions::set_pins_datatype_str (const std::string &s)
{
  set_datatypes (this, &LEFDEFReaderOptions::clear_pins_datatypes_per_mask, &LEFDEFReaderOptions::set_pins_datatype, &LEFDEFReaderOptions::set_pins_datatype_per_mask, s);
}

std::string
LEFDEFReaderOptions::pins_datatype_str () const
{
  return get_datatypes (this, &LEFDEFReaderOptions::pins_datatype, &LEFDEFReaderOptions::pins_datatype_per_mask, max_mask_number ());
}

void
LEFDEFReaderOptions::set_lef_pins_suffix_str (const std::string &s)
{
  set_suffixes (this, &LEFDEFReaderOptions::clear_lef_pins_suffixes_per_mask, &LEFDEFReaderOptions::set_lef_pins_suffix, &LEFDEFReaderOptions::set_lef_pins_suffix_per_mask, s);
}

std::string
LEFDEFReaderOptions::lef_pins_suffix_str () const
{
  return get_suffixes (this, &LEFDEFReaderOptions::lef_pins_suffix, &LEFDEFReaderOptions::lef_pins_suffix_per_mask, max_mask_number ());
}

void
LEFDEFReaderOptions::set_lef_pins_datatype_str (const std::string &s)
{
  set_datatypes (this, &LEFDEFReaderOptions::clear_lef_pins_datatypes_per_mask, &LEFDEFReaderOptions::set_lef_pins_datatype, &LEFDEFReaderOptions::set_lef_pins_datatype_per_mask, s);
}

std::string
LEFDEFReaderOptions::lef_pins_datatype_str () const
{
  return get_datatypes (this, &LEFDEFReaderOptions::lef_pins_datatype, &LEFDEFReaderOptions::lef_pins_datatype_per_mask, max_mask_number ());
}

void
LEFDEFReaderOptions::set_routing_suffix_str (const std::string &s)
{
  set_suffixes (this, &LEFDEFReaderOptions::clear_routing_suffixes_per_mask, &LEFDEFReaderOptions::set_routing_suffix, &LEFDEFReaderOptions::set_routing_suffix_per_mask, s);
}

std::string
LEFDEFReaderOptions::routing_suffix_str () const
{
  return get_suffixes (this, &LEFDEFReaderOptions::routing_suffix, &LEFDEFReaderOptions::routing_suffix_per_mask, max_mask_number ());
}

void
LEFDEFReaderOptions::set_routing_datatype_str (const std::string &s)
{
  set_datatypes (this, &LEFDEFReaderOptions::clear_routing_datatypes_per_mask, &LEFDEFReaderOptions::set_routing_datatype, &LEFDEFReaderOptions::set_routing_datatype_per_mask, s);
}

std::string
LEFDEFReaderOptions::routing_datatype_str () const
{
  return get_datatypes (this, &LEFDEFReaderOptions::routing_datatype, &LEFDEFReaderOptions::routing_datatype_per_mask, max_mask_number ());
}

void
LEFDEFReaderOptions::set_special_routing_suffix_str (const std::string &s)
{
  set_suffixes (this, &LEFDEFReaderOptions::clear_special_routing_suffixes_per_mask, &LEFDEFReaderOptions::set_special_routing_suffix, &LEFDEFReaderOptions::set_special_routing_suffix_per_mask, s);
}

std::string
LEFDEFReaderOptions::special_routing_suffix_str () const
{
  return get_suffixes (this, &LEFDEFReaderOptions::special_routing_suffix, &LEFDEFReaderOptions::special_routing_suffix_per_mask, max_mask_number ());
}

void
LEFDEFReaderOptions::set_special_routing_datatype_str (const std::string &s)
{
  set_datatypes (this, &LEFDEFReaderOptions::clear_special_routing_datatypes_per_mask, &LEFDEFReaderOptions::set_special_routing_datatype, &LEFDEFReaderOptions::set_special_routing_datatype_per_mask, s);
}

std::string
LEFDEFReaderOptions::special_routing_datatype_str () const
{
  return get_datatypes (this, &LEFDEFReaderOptions::special_routing_datatype, &LEFDEFReaderOptions::special_routing_datatype_per_mask, max_mask_number ());
}

// -----------------------------------------------------------------------------------
//  LEFDEFLayerDelegate implementation

LEFDEFReaderState::LEFDEFReaderState (const LEFDEFReaderOptions *tc, db::Layout &layout, const std::string &base_path)
  : m_create_layers (true), m_has_explicit_layer_mapping (false), m_laynum (1), mp_tech_comp (tc)
{
  if (! tc->map_file ().empty ()) {

    read_map_file (correct_path (tc->map_file (), layout, base_path), layout);

  } else {

    if (tc) {
      m_layer_map = tc->layer_map ();
      m_create_layers = tc->read_all_layers ();
    }

    m_layer_map.prepare (layout);

  }
}

LEFDEFReaderState::~LEFDEFReaderState ()
{
  for (std::map<std::string, LEFDEFViaGenerator *>::const_iterator i = m_via_generators.begin (); i != m_via_generators.end (); ++i) {
    delete i->second;
  }

  m_via_generators.clear ();
}

void
LEFDEFReaderState::register_layer (const std::string &ln)
{
  m_default_number.insert (std::make_pair (ln, m_laynum));
  ++m_laynum;
}

void
LEFDEFReaderState::map_layer_explicit (const std::string &n, LayerPurpose purpose, const db::LayerProperties &lp, unsigned int layer, unsigned int mask)
{
  tl_assert (m_has_explicit_layer_mapping);
  m_layers [std::make_pair (n, std::make_pair (purpose, mask))] = std::make_pair (true, layer);
  m_layer_map.map (lp, layer);
}

void
LEFDEFReaderState::read_map_file (const std::string &path, db::Layout &layout)
{
  m_has_explicit_layer_mapping = true;

  tl::log << tl::to_string (tr ("Reading LEF/DEF map file")) << " " << path;

  tl::InputFile file (path);
  tl::InputStream file_stream (file);
  tl::TextInputStream ts (file_stream);

  std::map<std::string, LayerPurpose> purpose_translation;
  purpose_translation ["LEFPIN"] = LEFPins;
  purpose_translation ["PIN"] = Pins;
  purpose_translation ["LEFOBS"] = Obstructions;
  purpose_translation ["SPNET"] = SpecialRouting;
  purpose_translation ["NET"] = Routing;
  purpose_translation ["VIA"] = ViaGeometry;
  purpose_translation ["BLOCKAGE"] = Blockage;

  std::map<std::pair<std::string, std::pair<LayerPurpose, unsigned int> >, db::LayerProperties> layer_map;

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

          layer_map [std::make_pair (std::string (), std::make_pair (Outline, (unsigned int) 0))] = db::LayerProperties (layer, datatype, "OUTLINE");

        } else if (w1 == "REGIONS") {

          layer_map [std::make_pair (std::string (), std::make_pair (Regions, (unsigned int) 0))] = db::LayerProperties (layer, datatype, "REGIONS");

        } else if (w1 == "BLOCKAGE") {

          layer_map [std::make_pair (std::string (), std::make_pair (PlacementBlockage, (unsigned int) 0))] = db::LayerProperties (layer, datatype, "PLACEMENT_BLK");

        } else if (w1 == "NAME") {

          //  converts a line like
          //    "NAME M1/PINS,M2/PINS ..."
          //  into a canonical name mapping like
          //    "(M1/LABELS): M1.LABEL"
          //    "(M2/LABELS): M2.LABEL"

          std::vector<std::string> layers;
          std::vector<std::string> purposes = tl::split (w2, ",");
          for (std::vector<std::string>::const_iterator p = purposes.begin (); p != purposes.end (); ++p) {
            layers.push_back (tl::split (*p, "/").front ());
          }

          std::string final_name = tl::join (layers, "/") + ".LABEL";
          for (std::vector<std::string>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
            layer_map [std::make_pair (*l, std::make_pair (Label, (unsigned int) 0))] = db::LayerProperties (layer, datatype, final_name);
          }

        } else {

          //  converts a line like
          //    "M1 SPNET,NET,PINS,LEFPINS ..."
          //  into a canonical name mapping like
          //    "(M1,NET):  M1.NET/PINS"
          //    "(M1,PINS): M1.NET/PINS"
          //  (separating, translating and recombing the purposes)

          std::set<std::pair<LayerPurpose, unsigned int> > translated_purposes;
          std::string purpose_str;

          std::vector<std::string> purposes = tl::split (w2, ",");
          for (std::vector<std::string>::const_iterator p = purposes.begin (); p != purposes.end (); ++p) {

            std::string p_uc = tl::to_upper_case (*p);
            tl::Extractor ex (p_uc.c_str ());

            std::string ps;
            ex.read (ps);

            unsigned int mask = 0;

            if (ex.test (":")) {
              if (ex.test ("MASK") && ex.test (":")) {
                ex.read (mask);
              }
            }

            std::map<std::string, LayerPurpose>::const_iterator i = purpose_translation.find (ps);
            if (i != purpose_translation.end ()) {

              translated_purposes.insert (std::make_pair (i->second, mask));

              if (! purpose_str.empty ()) {
                purpose_str += "/";
              }
              purpose_str += i->first;

            }

          }

          std::string final_name = w1 + "." + purpose_str;

          for (std::set<std::pair<LayerPurpose, unsigned int> >::const_iterator p = translated_purposes.begin (); p != translated_purposes.end (); ++p) {
            layer_map [std::make_pair (w1, *p)] = db::LayerProperties (layer, datatype, final_name);
          }

        }

      }

    }

  }

  db::DirectLayerMapping lm (&layout);
  for (std::map<std::pair<std::string, std::pair<LayerPurpose, unsigned int> >, db::LayerProperties>::const_iterator i = layer_map.begin (); i != layer_map.end (); ++i) {
    map_layer_explicit (i->first.first, i->first.second.first, i->second, lm.map_layer (i->second).second, i->first.second.second);
  }
}

std::pair <bool, unsigned int>
LEFDEFReaderState::open_layer (db::Layout &layout, const std::string &n, LayerPurpose purpose, unsigned int mask)
{
  std::map <std::pair<std::string, std::pair<LayerPurpose, unsigned int> >, std::pair<bool, unsigned int> >::const_iterator nl = m_layers.find (std::make_pair (n, std::make_pair (purpose, mask)));
  if (nl == m_layers.end ()) {

    std::pair <bool, unsigned int> ll (false, 0);

    if (n.empty () || ! m_has_explicit_layer_mapping) {
      ll = open_layer_uncached (layout, n, purpose, mask);
    }

    m_layers.insert (std::make_pair (std::make_pair (n, std::make_pair (purpose, mask)), ll));
    return ll;

  } else {
    return nl->second;
  }
}

std::pair <bool, unsigned int>
LEFDEFReaderState::open_layer_uncached (db::Layout &layout, const std::string &n, LayerPurpose purpose, unsigned int mask)
{
  if (n.empty ()) {

    //  NOTE: the canonical name is independent from the tech component's settings
    //  as is "(name)". It's used for implementing the automatic map file import
    //  feature.
    std::string ld;
    bool produce = false;

    if (purpose == Outline) {
      produce = mp_tech_comp->produce_cell_outlines ();
      ld = mp_tech_comp->cell_outline_layer ();
    } else if (purpose == Regions) {
      produce = mp_tech_comp->produce_regions ();
      ld = mp_tech_comp->region_layer ();
    } else if (purpose == PlacementBlockage) {
      produce = mp_tech_comp->produce_placement_blockages ();
      ld = mp_tech_comp->placement_blockage_layer ();
    }

    if (! produce) {
      return std::make_pair (false, 0);
    }

    db::LayerProperties lp;
    tl::Extractor ex (ld.c_str ());
    try {
      ex.read (lp);
    } catch (...) {
      lp.layer = 0;
      lp.datatype = 0;
    }

    std::pair<bool, unsigned int> ll = m_layer_map.logical (lp, layout);

    if (ll.first) {

      return ll;

    } else if (! m_create_layers) {

      return std::pair<bool, unsigned int> (false, 0);

    } else {

      unsigned int ll = layout.insert_layer (lp);
      m_layer_map.map (lp, ll);
      return std::pair<bool, unsigned int> (true, ll);

    }

  } else {

    if (mp_tech_comp) {
      bool produce = true;
      switch (purpose) {
      case Routing:
      default:
        produce = mp_tech_comp->produce_routing ();
        break;
      case SpecialRouting:
        produce = mp_tech_comp->produce_special_routing ();
        break;
      case ViaGeometry:
        produce = mp_tech_comp->produce_via_geometry ();
        break;
      case Label:
        produce = mp_tech_comp->produce_labels ();
        break;
      case Pins:
        produce = mp_tech_comp->produce_pins ();
        break;
      case LEFPins:
        produce = mp_tech_comp->produce_lef_pins ();
        break;
      case Obstructions:
        produce = mp_tech_comp->produce_obstructions ();
        break;
      case Blockage:
        produce = mp_tech_comp->produce_blockages ();
        break;
      }
      if (! produce) {
        return std::make_pair (false, 0);
      }
    }

    //  Note: "name" is the decorated name as provided by the tech component's
    //  x_suffix specifications.
    std::string name_suffix;
    int dt = 0;

    if (mp_tech_comp) {
      switch (purpose) {
      case Routing:
      default:
        name_suffix = mp_tech_comp->routing_suffix_per_mask (mask);
        dt = mp_tech_comp->routing_datatype_per_mask (mask);
        break;
      case SpecialRouting:
        name_suffix = mp_tech_comp->special_routing_suffix_per_mask (mask);
        dt = mp_tech_comp->special_routing_datatype_per_mask (mask);
        break;
      case ViaGeometry:
        name_suffix = mp_tech_comp->via_geometry_suffix_per_mask (mask);
        dt = mp_tech_comp->via_geometry_datatype_per_mask (mask);
        break;
      case Label:
        name_suffix = mp_tech_comp->labels_suffix ();
        dt = mp_tech_comp->labels_datatype ();
        break;
      case Pins:
        name_suffix = mp_tech_comp->pins_suffix_per_mask (mask);
        dt = mp_tech_comp->pins_datatype_per_mask (mask);
        break;
      case LEFPins:
        name_suffix = mp_tech_comp->lef_pins_suffix_per_mask (mask);
        dt = mp_tech_comp->lef_pins_datatype_per_mask (mask);
        break;
      case Obstructions:
        name_suffix = mp_tech_comp->obstructions_suffix ();
        dt = mp_tech_comp->obstructions_datatype ();
        break;
      case Blockage:
        name_suffix = mp_tech_comp->blockages_suffix ();
        dt = mp_tech_comp->blockages_datatype ();
        break;
      }
    }

    if (dt > 0 && name_suffix.empty ()) {
      name_suffix = "#" + tl::to_string (dt);
    }

    std::string name = n + name_suffix;

    std::pair<bool, unsigned int> ll = m_layer_map.logical (name, layout);

    if (ll.first) {

      return ll;

    } else {

      ll = m_layer_map.logical (n, layout);
      int ln = -1;

      if (ll.first && (ln = layout.get_properties (ll.second).layer) >= 0) {

        m_layer_map.map (db::LayerProperties (name), layout.layers (), db::LayerProperties (ln, dt, name));
        m_layer_map.prepare (layout);
        return m_layer_map.logical (name, layout);

      } else if (! m_create_layers) {

        return std::pair<bool, unsigned int> (false, 0);

      } else {

        std::map <std::pair<std::string, std::pair<LayerPurpose, unsigned int> >, unsigned int>::const_iterator l = m_unassigned_layers.find (std::make_pair (n, std::make_pair (purpose, mask)));
        if (l != m_unassigned_layers.end ()) {
          return std::pair<bool, unsigned int> (true, l->second);
        } else {
          unsigned int li = layout.insert_layer (db::LayerProperties (name));
          m_unassigned_layers.insert (std::make_pair (std::make_pair (n, std::make_pair (purpose, mask)), li));
          m_layer_map.map (db::LayerProperties (name), li);
          return std::pair<bool, unsigned int> (true, li);
        }

      }

    }

  }
}

void
LEFDEFReaderState::finish (db::Layout &layout)
{
  int lnum = 0;

  std::set<int> used_numbers;
  for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
    if ((*l).second->layer >= 0) {
      used_numbers.insert ((*l).second->layer);
    }
  }

  for (std::map<std::string, int>::const_iterator ln = m_default_number.begin (); ln != m_default_number.end (); ++ln) {
    used_numbers.insert (ln->second);
  }

  for (std::map<std::pair<std::string, std::pair<LayerPurpose, unsigned int> >, unsigned int>::const_iterator l = m_unassigned_layers.begin (); l != m_unassigned_layers.end (); ++l) {

    int dt = 0;
    switch (l->first.second.first) {
    case Routing:
    default:
      dt = mp_tech_comp->routing_datatype_per_mask (l->first.second.second);
      break;
    case SpecialRouting:
      dt = mp_tech_comp->special_routing_datatype_per_mask (l->first.second.second);
      break;
    case ViaGeometry:
      dt = mp_tech_comp->via_geometry_datatype_per_mask (l->first.second.second);
      break;
    case Label:
      dt = mp_tech_comp->labels_datatype ();
      break;
    case Pins:
      dt = mp_tech_comp->pins_datatype_per_mask (l->first.second.second);
      break;
    case LEFPins:
      dt = mp_tech_comp->lef_pins_datatype_per_mask (l->first.second.second);
      break;
    case Obstructions:
      dt = mp_tech_comp->obstructions_datatype ();
      break;
    case Blockage:
      dt = mp_tech_comp->blockages_datatype ();
      break;
    }

    int dl = 0;
    std::map<std::string, int>::const_iterator ln = m_default_number.find (l->first.first);
    if (ln != m_default_number.end ()) {
      dl = ln->second;
    } else {
      do {
        ++lnum;
      } while (used_numbers.find (lnum) != used_numbers.end ());
      m_default_number.insert (std::make_pair (l->first.first, lnum));
      dl = lnum;
    }

    db::LayerProperties lp = layout.get_properties (l->second);
    lp.layer = dl;
    lp.datatype = dt;
    layout.set_properties (l->second, lp);

  }
}

void
LEFDEFReaderState::register_via_cell (const std::string &vn, LEFDEFViaGenerator *generator)
{
  if (m_via_generators.find (vn) != m_via_generators.end ()) {
    delete m_via_generators [vn];
  }
  m_via_generators [vn] = generator;
}

db::Cell *
LEFDEFReaderState::via_cell (const std::string &vn, db::Layout &layout, unsigned int mask_bottom, unsigned int mask_cut, unsigned int mask_top, const LEFDEFNumberOfMasks *nm)
{
  ViaKey vk (vn, mask_bottom, mask_cut, mask_top);
  std::map<ViaKey, db::Cell *>::const_iterator i = m_via_cells.find (vk);
  if (i == m_via_cells.end ()) {

    db::Cell *cell = 0;

    std::map<std::string, LEFDEFViaGenerator *>::const_iterator g = m_via_generators.find (vn);
    if (g != m_via_generators.end ()) {

      LEFDEFViaGenerator *vg = g->second;

      std::string mask_suffix;
      if (mask_bottom > 0 || mask_cut > 0 || mask_top > 0) {
        mask_suffix += "_";
        mask_suffix += tl::to_string (mask_bottom);
        mask_suffix += "_";
        mask_suffix += tl::to_string (mask_cut);
        mask_suffix += "_";
        mask_suffix += tl::to_string (mask_top);
      }

      std::string cn = mp_tech_comp->via_cellname_prefix () + vn + mask_suffix;
      cell = &layout.cell (layout.add_cell (cn.c_str ()));

      vg->create_cell (*this, layout, *cell, mask_bottom, mask_cut, mask_top, nm);

    }

    m_via_cells[vk] = cell;
    return cell;

  } else {
    tl_assert (! i->second || i->second->layout () == &layout);
    return i->second;
  }
}

// -----------------------------------------------------------------------------------
//  LEFDEFImporter implementation

LEFDEFImporter::LEFDEFImporter ()
  : mp_progress (0), mp_stream (0), mp_reader_state (0),
    m_produce_net_props (false), m_net_prop_name_id (0),
    m_produce_inst_props (false), m_inst_prop_name_id (0),
    m_produce_pin_props (false), m_pin_prop_name_id (0)
{
  //  .. nothing yet ..
}

LEFDEFImporter::~LEFDEFImporter ()
{
  //  .. nothing yet ..
}

unsigned int
LEFDEFImporter::get_mask (long m)
{
  return (unsigned int) m;
}

void 
LEFDEFImporter::read (tl::InputStream &stream, db::Layout &layout, LEFDEFReaderState &state)
{
  m_fn = stream.filename ();

  tl::AbsoluteProgress progress (tl::to_string (tr ("Reading ")) + m_fn, 1000);
  progress.set_format (tl::to_string (tr ("%.0fk lines")));
  progress.set_format_unit (1000.0);
  progress.set_unit (10000.0);

  mp_reader_state = &state;

  if (state.tech_comp ()) {
    m_options = *state.tech_comp ();
  }

  m_produce_net_props = false;
  m_net_prop_name_id = 0;

  if (m_options.produce_net_names ()) {
    m_produce_net_props = true;
    m_net_prop_name_id = layout.properties_repository ().prop_name_id (m_options.net_property_name ());
  }

  m_produce_inst_props = false;
  m_inst_prop_name_id = 0;

  if (m_options.produce_inst_names ()) {
    m_produce_inst_props = true;
    m_inst_prop_name_id = layout.properties_repository ().prop_name_id (m_options.inst_property_name ());
  }

  m_produce_pin_props = false;
  m_pin_prop_name_id = 0;

  if (m_options.produce_pin_names ()) {
    m_produce_pin_props = true;
    m_pin_prop_name_id = layout.properties_repository ().prop_name_id (m_options.pin_property_name ());
  }

  try {

    mp_progress = &progress;
    mp_stream = new tl::TextInputStream (stream);

    do_read (layout); 

    delete mp_stream;
    mp_stream = 0;
    mp_progress = 0;

  } catch (...) {
    delete mp_stream;
    mp_stream = 0;
    mp_progress = 0;
    throw;
  }
}

void 
LEFDEFImporter::error (const std::string &msg)
{
  throw LEFDEFReaderException (msg, int (mp_stream->line_number ()), m_cellname, m_fn);
}

void 
LEFDEFImporter::warn (const std::string &msg)
{
  tl::warn << msg 
           << tl::to_string (tr (" (line=")) << mp_stream->line_number ()
           << tl::to_string (tr (", cell=")) << m_cellname
           << tl::to_string (tr (", file=")) << m_fn
           << ")";
}

bool
LEFDEFImporter::at_end ()
{
  if (m_last_token.empty ()) {
    if (next ().empty ()) {
      return true;
    }
  }
  return false;
}

bool  
LEFDEFImporter::peek (const std::string &token)
{
  if (m_last_token.empty ()) {
    if (next ().empty ()) {
      error ("Unexpected end of file");
    }
  }

  const char *a = m_last_token.c_str ();
  const char *b = token.c_str ();
  while (*a && *b) {
    if (std::toupper (*a) != std::toupper (*b)) {
      return false;
    }
    ++a, ++b;
  }
  return *a == *b;
}

bool  
LEFDEFImporter::test (const std::string &token)
{
  if (peek (token)) {
    //  consume when successful
    m_last_token.clear ();
    return true;
  } else {
    return false;
  }
}

void  
LEFDEFImporter::expect (const std::string &token)
{
  if (! test (token)) {
    error ("Expected token: " + token);
  }
}

void
LEFDEFImporter::expect (const std::string &token1, const std::string &token2)
{
  if (! test (token1) && ! test (token2)) {
    error ("Expected token: " + token1 + " or " + token2);
  }
}

void
LEFDEFImporter::expect (const std::string &token1, const std::string &token2, const std::string &token3)
{
  if (! test (token1) && ! test (token2) && ! test (token3)) {
    error ("Expected token: " + token1 + ", " + token2 + " or " + token3);
  }
}

double
LEFDEFImporter::get_double ()
{
  if (m_last_token.empty ()) {
    if (next ().empty ()) {
      error ("Unexpected end of file");
    }
  }

  double d = 0;
  try {
    tl::from_string (m_last_token, d);
  } catch (...) {
    error ("Not a floating-point value: " + m_last_token);
  }

  m_last_token.clear ();

  return d;
}

long  
LEFDEFImporter::get_long ()
{
  if (m_last_token.empty ()) {
    if (next ().empty ()) {
      error ("Unexpected end of file");
    }
  }

  long l = 0;
  try {
    tl::from_string (m_last_token, l);
  } catch (...) {
    error ("Not an integer value: " + m_last_token);
  }

  m_last_token.clear ();

  return l;
}

void
LEFDEFImporter::take ()
{
  if (m_last_token.empty ()) {
    if (next ().empty ()) {
      error ("Unexpected end of file");
    }
  }
  m_last_token.clear ();
}

std::string 
LEFDEFImporter::get ()
{
  if (m_last_token.empty ()) {
    if (next ().empty ()) {
      error ("Unexpected end of file");
    }
  }
  std::string r;
  r.swap (m_last_token);
  return r;
}

const std::string &
LEFDEFImporter::next ()
{
  unsigned int last_line = (unsigned int) mp_stream->line_number ();

  m_last_token.clear ();

  char c;

  do {

    while ((c = mp_stream->get_char ()) != 0 && isspace (c)) 
      ;

    if (c == '#') {

      while ((c = mp_stream->get_char ()) != 0 && (c != '\015' && c != '\012')) 
        ;

    } else if (c == '\'' || c == '"') {

      char quot = c;

      while ((c = mp_stream->get_char ()) != 0 && c != quot) {
        if (c == '\\') {
          c = mp_stream->get_char ();
        }
        if (c) {
          m_last_token += c;
        }
      }

      break;

    } else if (c) {

      m_last_token += c; 

      while ((c = mp_stream->get_char ()) != 0 && ! isspace (c)) {
        if (c == '\\') {
          c = mp_stream->get_char ();
        }
        if (c) {
          m_last_token += c;
        }
      }

      break;

    }

  } while (c);

  if (mp_stream->line_number () != last_line) {
    ++*mp_progress;
  }

  return m_last_token;
}

db::FTrans
LEFDEFImporter::get_orient (bool optional)
{
  if (test ("N")) {
    return db::FTrans (db::FTrans::r0);
  } else if (test ("S")) {
    return db::FTrans (db::FTrans::r180);
  } else if (test ("W")) {
    return db::FTrans (db::FTrans::r90);
  } else if (test ("E")) {
    return db::FTrans (db::FTrans::r270);
  } else if (test ("FN")) {
    return db::FTrans (db::FTrans::m90);
  } else if (test ("FS")) {
    return db::FTrans (db::FTrans::m0);
  } else if (test ("FW")) {
    return db::FTrans (db::FTrans::m45);
  } else if (test ("FE")) {
    return db::FTrans (db::FTrans::m135);
  } else if (optional) {
    return db::FTrans (db::FTrans::r0);
  } else {
    error (tl::to_string (tr ("Invalid orientation specification: ")) + get ());
    return db::FTrans (db::FTrans::r0);
  }
}

db::Point
LEFDEFImporter::get_point (double scale)
{
  double x = get_double ();
  double y = get_double ();
  return db::Point (db::DPoint (x * scale, y * scale));
}

db::Vector
LEFDEFImporter::get_vector (double scale)
{
  double x = get_double ();
  double y = get_double ();
  return db::Vector (db::DVector (x * scale, y * scale));
}

}

