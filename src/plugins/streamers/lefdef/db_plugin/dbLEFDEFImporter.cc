
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


#include "dbLEFDEFImporter.h"
#include "dbLayoutUtils.h"
#include "dbTechnology.h"
#include "dbShapeProcessor.h"

#include "tlStream.h"
#include "tlProgress.h"
#include "tlFileUtils.h"

#include <cctype>

namespace db
{

// -----------------------------------------------------------------------------------
//  Path resolution utility

std::string correct_path (const std::string &fn_in, const db::Layout &layout, const std::string &base_path)
{
  const db::Technology *tech = layout.technology ();

  //  Allow LEF reference through expressions, i.e.
  //    $(base_path) - path of the main file
  //    $(tech_dir)  - the location of the .lyt file if a technology is specified
  //    $(tech_name) - the name of the technology if one is specified
  //  In addition expressions are interpolated, e.g. "$(env('HOME'))".

  tl::Eval expr;
  expr.set_var ("base_path", base_path);
  if (tech) {
    expr.set_var ("tech_dir", tech->base_path ());
    expr.set_var ("tech_name", tech->name ());
  }

  std::string fn = expr.interpolate (fn_in);

  if (! tl::is_absolute (fn)) {

    //  if a technology is given and the file can be found in the technology's base path, take it
    //  from there.
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

static unsigned int hex_value (char c)
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

std::vector<unsigned int> string2masks (const std::string &s)
{
  std::vector<unsigned int> res;
  res.reserve (s.size ());

  for (const char *cp = s.c_str (); *cp; ++cp) {
    if (! is_hex_digit (*cp)) {
      throw tl::Exception ("Not a hex string: " + s);
    }
    res.push_back (hex_value (*cp));
  }

  std::reverse (res.begin (), res.end ());

  return res;
}

static unsigned int mask (const std::vector<unsigned int> &masks, unsigned int index)
{
  if (index < (unsigned int) masks.size ()) {
    return masks [index];
  } else {
    return 0;
  }
}

static std::string purpose_to_name (LayerPurpose purpose)
{
  switch (purpose) {
  case Outline:
    return "OUTLINE";
  case Regions:
    return "REGION";
  case RegionsGuide:
    return "REGIONGUIDE";
  case RegionsFence:
    return "REGIONFENCE";
  case RegionsNone:
    return "REGIONNONE";
  case PlacementBlockage:
    return "BLOCKAGE";
  case Routing:
    return "NET";
  case SpecialRouting:
    return "SPNET";
  case ViaGeometry:
    return "VIA";
  case Label:
    return "LABEL";
  case LEFLabel:
    return "LEFLABEL";
  case Pins:
    return "PIN";
  case Fills:
    return "FILL";
  case FillsOPC:
    return "FILLOPC";
  case LEFPins:
    return "LEFPIN";
  case Obstructions:
    return "LEFOBS";
  case Blockage:
    return "BLK";
  case All:
    return "ALL";
  default:
    return std::string ();
  }
}

static std::string
layer_spec_to_name (const std::string &layer_name, LayerPurpose purpose, unsigned int mask, const db::DVector &via_size)
{
  std::string ps = purpose_to_name (purpose);

  std::string n = layer_name;
  if (! n.empty ()) {
    n += ".";
  }
  n += ps;

  if (mask > 0) {
    n += ":";
    n += tl::to_string (mask);
  }

  if (via_size != db::DVector ()) {
    n += ":SIZE";
    n += tl::sprintf ("%.12gX%.12g", via_size.x (), via_size.y ());
  }

  return n;
}

// -----------------------------------------------------------------------------------
//  RuleBasedViaGenerator implementation

RuleBasedViaGenerator::RuleBasedViaGenerator ()
  : LEFDEFLayoutGenerator (), m_bottom_mask (0), m_cut_mask (0), m_top_mask (0), m_rows (1), m_columns (1)
{ }

void
RuleBasedViaGenerator::create_cell (LEFDEFReaderState &reader, Layout &layout, db::Cell &cell, const std::vector<std::string> *maskshift_layers, const std::vector<unsigned int> &masks, const LEFDEFNumberOfMasks *nm)
{
  //  will not be used with an external maskshift layer stack
  tl_assert (maskshift_layers == 0);

  unsigned int mask_bottom = mask (masks, 0), mask_cut = mask (masks, 1), mask_top = mask (masks, 2);

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

  std::set <unsigned int> dl;

  dl = reader.open_layer (layout, m_bottom_layer, ViaGeometry, mask_bottom);
  for (std::set<unsigned int>::const_iterator l = dl.begin (); l != dl.end (); ++l) {
    cell.shapes (*l).insert (db::Polygon (via_box.enlarged (m_be).moved (m_bo)));
  }

  dl = reader.open_layer (layout, m_top_layer, ViaGeometry, mask_top);
  for (std::set<unsigned int>::const_iterator l = dl.begin (); l != dl.end (); ++l) {
    cell.shapes (*l).insert (db::Polygon (via_box.enlarged (m_te).moved (m_to)));
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

          dl = reader.open_layer (layout, m_cut_layer, ViaGeometry, cm, vb);
          for (std::set<unsigned int>::const_iterator l = dl.begin (); l != dl.end (); ++l) {
            cell.shapes (*l).insert (db::Polygon (vb));
          }

        }

      }

    }

  }
}

// -----------------------------------------------------------------------------------
//  GeometryBasedViaGenerator implementation

GeometryBasedLayoutGenerator::GeometryBasedLayoutGenerator ()
  : LEFDEFLayoutGenerator (), m_fixedmask (false)
{
  //  .. nothing yet ..
}

unsigned int
GeometryBasedLayoutGenerator::get_maskshift (const std::string &ln, const std::vector<std::string> *msl, const std::vector<unsigned int> &masks)
{
  if (! msl) {
    msl = &m_maskshift_layers;
  }

  for (std::vector<std::string>::const_iterator l = msl->begin (); l != msl->end (); ++l) {
    if (! l->empty () && *l == ln) {
      return mask (masks, (unsigned int) (l - msl->begin ()));
    }
  }

  return 0;
}

unsigned int
GeometryBasedLayoutGenerator::mask_for (const std::string &ln, unsigned int m, unsigned int mshift, const LEFDEFNumberOfMasks *nm) const
{
  //  for FIXEDMASK we don't do any mask shifting
  if (m_fixedmask || mshift == 0) {
    return m;
  } else if (m == 0) {
    return mshift;
  } else {
    return (m + mshift - 2) % nm->number_of_masks (ln) + 1;
  }
}

unsigned int
GeometryBasedLayoutGenerator::combine_maskshifts (const std::string &ln, unsigned int mshift1, unsigned int mshift2, const LEFDEFNumberOfMasks *nm) const
{
  if (mshift1 == 0 || mshift2 == 0) {
    return mshift1 + mshift2;
  } else {
    return (mshift1 + mshift2 - 2) % nm->number_of_masks (ln) + 1;
  }
}

void
GeometryBasedLayoutGenerator::create_cell (LEFDEFReaderState &reader, Layout &layout, db::Cell &cell, const std::vector<std::string> *ext_msl, const std::vector<unsigned int> &masks, const LEFDEFNumberOfMasks *nm)
{
  for (std::map <std::pair<std::string, LayerDetailsKey>, db::Shapes>::const_iterator g = m_shapes.begin (); g != m_shapes.end (); ++g) {

    unsigned int mshift = get_maskshift (g->first.first, ext_msl, masks);
    unsigned int mask = mask_for (g->first.first, g->first.second.mask, mshift, nm);

    std::set <unsigned int> dl = reader.open_layer (layout, g->first.first, g->first.second.purpose, mask, g->first.second.via_size);
    for (std::set<unsigned int>::const_iterator l = dl.begin (); l != dl.end (); ++l) {
      cell.shapes (*l).insert (g->second);
    }

  }

  for (std::list<Via>::const_iterator v = m_vias.begin (); v != m_vias.end (); ++v) {

    LEFDEFLayoutGenerator *g = reader.via_generator (v->name, v->nondefaultrule);
    if (! g) {
      continue;
    }

    std::vector<std::string> msl = g->maskshift_layers ();
    msl.resize (3, std::string ());

    unsigned mshift_bottom = get_maskshift (msl [0], ext_msl, masks);
    unsigned mshift_cut = get_maskshift (msl [1], ext_msl, masks);
    unsigned mshift_top = get_maskshift (msl [2], ext_msl, masks);

    db::Cell *vc = reader.via_cell (v->name, v->nondefaultrule, layout,
                                    combine_maskshifts (msl [0], v->bottom_mask, mshift_bottom, nm),
                                    combine_maskshifts (msl [1], v->cut_mask, mshift_cut, nm),
                                    combine_maskshifts (msl [2], v->top_mask, mshift_top, nm),
                                    nm);

    if (vc) {
      cell.insert (db::CellInstArray (db::CellInst (vc->cell_index ()), v->trans));
    }

  }
}

template <class Shape>
static db::Shape insert_shape (db::Shapes &shapes, const Shape &shape, db::properties_id_type prop_id)
{
  if (prop_id == 0) {
    return shapes.insert (shape);
  } else {
    return shapes.insert (db::object_with_properties<Shape> (shape, prop_id));
  }
}

void
GeometryBasedLayoutGenerator::add_polygon (const std::string &ln, LayerPurpose purpose, const db::Polygon &poly, unsigned int mask, db::properties_id_type prop_id, const db::DVector &via_size)
{
  insert_shape (m_shapes [std::make_pair (ln, LayerDetailsKey (purpose, mask, via_size))], poly, prop_id);
}

void
GeometryBasedLayoutGenerator::add_box (const std::string &ln, LayerPurpose purpose, const db::Box &box, unsigned int mask, db::properties_id_type prop_id, const db::DVector &via_size)
{
  insert_shape (m_shapes [std::make_pair (ln, LayerDetailsKey (purpose, mask, via_size))], box, prop_id);
}

void
GeometryBasedLayoutGenerator::add_path (const std::string &ln, LayerPurpose purpose, const db::Path &path, unsigned int mask, db::properties_id_type prop_id, const db::DVector &via_size)
{
  insert_shape (m_shapes [std::make_pair (ln, LayerDetailsKey (purpose, mask, via_size))], path, prop_id);
}

void
GeometryBasedLayoutGenerator::add_text (const std::string &ln, LayerPurpose purpose, const db::Text &text, unsigned int mask, db::properties_id_type prop_id)
{
  insert_shape (m_shapes [std::make_pair (ln, LayerDetailsKey (purpose, mask))], text, prop_id);
}

void
GeometryBasedLayoutGenerator::add_via (const std::string &vn, const db::Trans &trans, unsigned int bottom_mask, unsigned int cut_mask, unsigned int top_mask)
{
  m_vias.push_back (Via ());
  m_vias.back ().name = vn;
  m_vias.back ().trans = trans;
  m_vias.back ().bottom_mask = bottom_mask;
  m_vias.back ().cut_mask = cut_mask;
  m_vias.back ().top_mask = top_mask;
}

void
GeometryBasedLayoutGenerator::subtract_overlap_from_outline (const std::set<std::string> &overlap_layers)
{
  db::Shapes all_overlaps;

  std::vector<std::map <std::pair<std::string, LayerDetailsKey>, db::Shapes>::iterator> to_remove;
  for (auto s = m_shapes.begin (); s != m_shapes.end (); ++s) {
    if (overlap_layers.find (s->first.first) != overlap_layers.end ()) {
      all_overlaps.insert (s->second);
      to_remove.push_back (s);
    }
  }

  for (auto i = to_remove.begin (); i != to_remove.end (); ++i) {
    m_shapes.erase (*i);
  }

  if (all_overlaps.empty ()) {
    return;
  }

  for (auto s = m_shapes.begin (); s != m_shapes.end (); ++s) {

    if (s->first.second.purpose != Outline) {
      continue;
    }

    db::ShapeProcessor proc;

    size_t pn = 0;
    for (auto sh = s->second.begin (db::ShapeIterator::All); ! sh.at_end (); ++sh) {
      proc.insert (*sh, pn);
      pn += 2;
    }

    pn = 1;
    for (auto sh = all_overlaps.begin (db::ShapeIterator::All); ! sh.at_end (); ++sh) {
      proc.insert (*sh, pn);
      pn += 2;
    }

    db::BooleanOp op (db::BooleanOp::And);
    db::ShapeGenerator sg (s->second, true /*clear shapes*/);
    db::PolygonGenerator out (sg, true, true);
    proc.process (out, op);

  }
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
    m_produce_fills (true),
    m_fills_suffix (".FILL"),
    m_fills_datatype (5),
    m_produce_obstructions (true),
    m_obstructions_suffix (".OBS"),
    m_obstructions_datatype (3),
    m_produce_blockages (true),
    m_blockages_suffix (".BLK"),
    m_blockages_datatype (4),
    m_produce_labels (true),
    m_labels_suffix (".LABEL"),
    m_labels_datatype (1),
    m_produce_lef_labels (true),
    m_lef_labels_suffix (".LABEL"),
    m_lef_labels_datatype (1),
    m_produce_routing (true),
    m_routing_suffix (""),
    m_routing_datatype (0),
    m_produce_special_routing (true),
    m_special_routing_suffix (""),
    m_special_routing_datatype (0),
    m_separate_groups (false),
    m_joined_paths (false),
    m_map_file (),
    m_macro_resolution_mode (0),
    m_read_lef_with_def (true),
    m_paths_relative_to_cwd (false)
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
    m_produce_fills = d.m_produce_fills;
    m_fills_suffix = d.m_fills_suffix;
    m_fills_suffixes = d.m_fills_suffixes;
    m_fills_datatype = d.m_fills_datatype;
    m_fills_datatypes = d.m_fills_datatypes;
    m_produce_obstructions = d.m_produce_obstructions;
    m_obstructions_suffix = d.m_obstructions_suffix;
    m_obstructions_datatype = d.m_obstructions_datatype;
    m_produce_blockages = d.m_produce_blockages;
    m_blockages_suffix = d.m_blockages_suffix;
    m_blockages_datatype = d.m_blockages_datatype;
    m_produce_labels = d.m_produce_labels;
    m_labels_suffix = d.m_labels_suffix;
    m_labels_datatype = d.m_labels_datatype;
    m_produce_lef_labels = d.m_produce_lef_labels;
    m_lef_labels_suffix = d.m_lef_labels_suffix;
    m_lef_labels_datatype = d.m_lef_labels_datatype;
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
    m_joined_paths = d.m_joined_paths;
    m_map_file = d.m_map_file;
    m_macro_resolution_mode = d.m_macro_resolution_mode;
    m_lef_files = d.m_lef_files;
    m_macro_layout_files = d.m_macro_layout_files;
    m_read_lef_with_def = d.m_read_lef_with_def;
    m_paths_relative_to_cwd = d.m_paths_relative_to_cwd;
    set_macro_layouts (d.macro_layouts ());
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
  (data->*set_datatype) (-1);

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
  (data->*set_suffix) (std::string ());

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
LEFDEFReaderOptions::set_fills_suffix_str (const std::string &s)
{
  set_suffixes (this, &LEFDEFReaderOptions::clear_fills_suffixes_per_mask, &LEFDEFReaderOptions::set_fills_suffix, &LEFDEFReaderOptions::set_fills_suffix_per_mask, s);
}

std::string
LEFDEFReaderOptions::fills_suffix_str () const
{
  return get_suffixes (this, &LEFDEFReaderOptions::fills_suffix, &LEFDEFReaderOptions::fills_suffix_per_mask, max_mask_number ());
}

void
LEFDEFReaderOptions::set_fills_datatype_str (const std::string &s)
{
  set_datatypes (this, &LEFDEFReaderOptions::clear_fills_datatypes_per_mask, &LEFDEFReaderOptions::set_fills_datatype, &LEFDEFReaderOptions::set_fills_datatype_per_mask, s);
}

std::string
LEFDEFReaderOptions::fills_datatype_str () const
{
  return get_datatypes (this, &LEFDEFReaderOptions::fills_datatype, &LEFDEFReaderOptions::fills_datatype_per_mask, max_mask_number ());
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
  : mp_importer (0), m_create_layers (true), m_has_explicit_layer_mapping (false), m_laynum (1), mp_tech_comp (tc)
{
  if (! tc) {

    //  use default options

  } else if (! tc->map_file ().empty ()) {

    read_map_file (tc->map_file (), layout, base_path);

  } else {

    m_layer_map = tc->layer_map ();
    m_create_layers = tc->read_all_layers ();

  }
}

LEFDEFReaderState::~LEFDEFReaderState ()
{
  for (std::map<std::pair<std::string, std::string>, LEFDEFLayoutGenerator *>::const_iterator i = m_via_generators.begin (); i != m_via_generators.end (); ++i) {
    delete i->second;
  }

  m_via_generators.clear ();

  for (std::map<std::string, LEFDEFLayoutGenerator *>::const_iterator i = m_macro_generators.begin (); i != m_macro_generators.end (); ++i) {
    delete i->second;
  }

  m_macro_generators.clear ();
}

void
LEFDEFReaderState::common_reader_error (const std::string &msg)
{
  if (mp_importer) {
    mp_importer->error (msg);
  }
}

void
LEFDEFReaderState::common_reader_warn (const std::string &msg, int warn_level)
{
  if (mp_importer) {
    mp_importer->warn (msg, warn_level);
  }
}

void
LEFDEFReaderState::register_layer (const std::string &ln)
{
  m_default_number.insert (std::make_pair (ln, m_laynum));
  ++m_laynum;
}

static bool try_read_layers (tl::Extractor &ex, std::vector<int> &layers)
{
  int l = 0;
  if (! ex.try_read (l)) {
    return false;
  }
  layers.push_back (l);

  if (ex.test (",")) {
    do {
      if (! ex.try_read (l)) {
        return false;
      }
      layers.push_back (l);
    } while (ex.test (","));
  }

  return true;
}

static std::string::size_type find_file_sep (const std::string &s, std::string::size_type from)
{
  std::string::size_type p1 = s.find ("+", from);
  std::string::size_type p2 = s.find (",", from);

  if (p1 == std::string::npos) {
    return p2;
  } else if (p2 == std::string::npos) {
    return p1;
  } else {
    return p1 < p2 ? p1 : p2;
  }
}

static std::vector<std::string> split_file_list (const std::string &infile)
{
  std::vector<std::string> files;

  size_t p = 0;
  for (size_t pp = 0; (pp = find_file_sep (infile, p)) != std::string::npos; p = pp + 1) {
    files.push_back (std::string (infile, p, pp - p));
  }
  files.push_back (std::string (infile, p));

  return files;
}

void
LEFDEFReaderState::read_map_file (const std::string &filename, db::Layout &layout, const std::string &base_path)
{
  m_has_explicit_layer_mapping = true;

  std::vector<std::string> paths = split_file_list (filename);

  std::map<std::pair<std::string, LayerDetailsKey>, std::vector<db::LayerProperties> > layer_map;

  for (std::vector<std::string>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
    read_single_map_file (correct_path (*p, layout, base_path), layer_map);
  }

  //  build an explicit layer mapping now.

  m_layers.clear ();
  m_layer_map.clear ();

  db::DirectLayerMapping lm (&layout);
  for (std::map<std::pair<std::string, LayerDetailsKey>, std::vector<db::LayerProperties> >::const_iterator i = layer_map.begin (); i != layer_map.end (); ++i) {
    for (std::vector<db::LayerProperties>::const_iterator j = i->second.begin (); j != i->second.end (); ++j) {
      unsigned int layer = lm.map_layer (*j).second;
      m_layers [i->first].insert (layer);
      m_layer_map.mmap (*j, layer);
    }
  }
}

void
LEFDEFReaderState::read_single_map_file (const std::string &path, std::map<std::pair<std::string, LayerDetailsKey>, std::vector<db::LayerProperties> > &layer_map)
{
  tl::InputFile file (path);
  tl::InputStream file_stream (file);
  tl::TextInputStream ts (file_stream);

  tl::log << tl::to_string (tr ("Reading LEF/DEF map file")) << " " << file_stream.absolute_path ();

  //  Purpose name to purpose code
  std::map<std::string, LayerPurpose> purpose_translation;
  purpose_translation ["LEFPIN"] = LEFPins;
  purpose_translation ["PIN"] = Pins;
  purpose_translation ["LEFPINNAME"] = LEFLabel;
  purpose_translation ["PINNAME"] = Label;
  purpose_translation ["FILL"] = Fills;
  purpose_translation ["FILLOPC"] = FillsOPC;
  purpose_translation ["LEFOBS"] = Obstructions;
  purpose_translation ["SPNET"] = SpecialRouting;
  purpose_translation ["NET"] = Routing;
  purpose_translation ["VIA"] = ViaGeometry;
  purpose_translation ["BLOCKAGE"] = Blockage;
  purpose_translation ["ALL"] = All;

  //  List of purposes corresponding to ALL
  LayerPurpose all_purposes[] = {
    LEFPins, Pins, Fills, FillsOPC, Obstructions, SpecialRouting, Routing, ViaGeometry
  };

  while (! ts.at_end ()) {

    const std::string &l = ts.get_line ();

    tl::Extractor ex (l.c_str ());
    if (ex.at_end () || ex.test ("#")) {

      //  ignore empty of comment lines

    } else {

      std::string w1, w2;
      std::vector<int> layers, datatypes;
      size_t max_purpose_str = 15;

      if (! ex.try_read_word (w1) || ! ex.try_read_word (w2, "._$,/:") || ! try_read_layers (ex, layers) || ! try_read_layers (ex, datatypes)) {
        tl::warn << tl::sprintf (tl::to_string (tr ("Reading layer map file %s, line %d not understood - skipped")), path, ts.line_number ());
        continue;
      }

      if (w1 == "DIEAREA") {

        for (std::vector<int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
          for (std::vector<int>::const_iterator d = datatypes.begin (); d != datatypes.end (); ++d) {
            layer_map [std::make_pair (std::string (), LayerDetailsKey (Outline))].push_back (db::LayerProperties (*l, *d, "OUTLINE"));
          }
        }

      } else if (w1 == "REGION") {

        std::string name = "REGIONS";
        LayerPurpose lp = Regions;
        if (w2 == "FENCE") {
          name = "REGIONS_FENCE";
          lp = RegionsFence;
        } else if (w2 == "GUIDE") {
          name = "REGIONS_GUIDE";
          lp = RegionsGuide;
        } else if (w2 == "NONE") {
          name = "REGIONS_NONE";
          lp = RegionsNone;
        } else if (w2 != "ALL") {
          tl::warn << tl::sprintf (tl::to_string (tr ("Reading layer map file %s, line %d - ignoring unknowns REGION purpose %s (use FENCE, GUIDE or ALL)")), path, ts.line_number (), w2);
        }

        for (std::vector<int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
          for (std::vector<int>::const_iterator d = datatypes.begin (); d != datatypes.end (); ++d) {
            layer_map [std::make_pair (std::string (), LayerDetailsKey (lp))].push_back (db::LayerProperties (*l, *d, name));
          }
        }

      } else if (w1 == "BLOCKAGE") {

        for (std::vector<int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
          for (std::vector<int>::const_iterator d = datatypes.begin (); d != datatypes.end (); ++d) {
            layer_map [std::make_pair (std::string (), LayerDetailsKey (PlacementBlockage))].push_back (db::LayerProperties (*l, *d, "PLACEMENT_BLK"));
          }
        }

      } else if (w1 == "NAME") {

        //  converts a line like
        //    "NAME M1/PINS,M2/PINS ..."
        //  into a canonical name mapping like
        //    "(M1/LABELS): M1.LABEL"
        //    "(M2/LABELS): M2.LABEL"
        //  supported purposes: PINS(->Label), LEFPINS(->LEFLabels)

        std::vector< std::pair<std::string, LayerPurpose> > layer_defs;

        std::vector<std::string> purposes = tl::split (w2, ",");
        for (std::vector<std::string>::const_iterator p = purposes.begin (); p != purposes.end (); ++p) {

          if (*p == "DIEAREA" || *p == "ALL" || *p == "COMP") {

            tl::warn << tl::sprintf (tl::to_string (tr ("Reading layer map file %s, line %d: NAME record ignored for entity: %s")), path, ts.line_number (), *p);

          } else {

            std::vector<std::string> lp = tl::split (*p, "/");

            if (lp.size () > 1) {

              LayerPurpose label_purpose = Pins;
              std::map<std::string, LayerPurpose>::const_iterator i = purpose_translation.find (lp[1]);
              if (i != purpose_translation.end ()) {
                label_purpose = i->second;
              }

              if (label_purpose == Pins || label_purpose == LEFPins) {
                layer_defs.push_back (std::make_pair (lp.front (), label_purpose == Pins ? Label : LEFLabel));
              } else {
                tl::warn << tl::sprintf (tl::to_string (tr ("Reading layer map file %s, line %d: NAME record ignored for purpose: %s")), path, ts.line_number (), purpose_to_name (label_purpose));
              }

            } else {

              layer_defs.push_back (std::make_pair (lp.front (), Label));
              layer_defs.push_back (std::make_pair (lp.front (), LEFLabel));

            }

          }

        }

        std::string final_name;
        for (std::vector< std::pair<std::string, LayerPurpose> >::const_iterator i = layer_defs.begin (); i != layer_defs.end (); ++i) {
          if (! final_name.empty ()) {
            final_name += "/";
          }
          final_name += i->first + "." + purpose_to_name (i->second);
        }

        for (std::vector< std::pair<std::string, LayerPurpose> >::const_iterator i = layer_defs.begin (); i != layer_defs.end (); ++i) {
          for (std::vector<int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
            for (std::vector<int>::const_iterator d = datatypes.begin (); d != datatypes.end (); ++d) {
              layer_map [std::make_pair (i->first, LayerDetailsKey (i->second))].push_back (db::LayerProperties (*l, *d, final_name));
            }
          }
        }

      } else if (w1 == "COMP") {

        //  ignore "COMP (ALL) ..."
        tl::warn << tl::sprintf (tl::to_string (tr ("Reading layer map file %s, line %d: COMP entry ignored")), path, ts.line_number ());

      } else {

        //  converts a line like
        //    "M1 SPNET,NET,PINS,LEFPINS ..."
        //  into a canonical name mapping like
        //    "(M1,NET):  M1.NET/PINS"
        //    "(M1,PINS): M1.NET/PINS"
        //  (separating, translating and recombing the purposes)

        std::set<LayerDetailsKey> translated_purposes;

        std::vector<std::string> purposes = tl::split (w2, ",");
        std::reverse (purposes.begin (), purposes.end ());

        unsigned int mask = 0;

        for (std::vector<std::string>::const_iterator p = purposes.begin (); p != purposes.end (); ++p) {

          std::string p_uc = tl::to_upper_case (*p);
          tl::Extractor ex (p_uc.c_str ());

          std::string ps;
          ex.read_word_or_quoted (ps);
          db::DVector via_size;

          std::map<std::string, LayerPurpose>::const_iterator i = purpose_translation.find (ps);
          if (i != purpose_translation.end ()) {

            if (i->second == Routing) {

              if (ex.test (":VOLTAGE:")) {
                double f = 0.0;
                ex.read (f);
                tl::warn << tl::sprintf (tl::to_string (tr ("Reading layer map file %s, line %d: NET voltage constraint ignored for layer %s")), path, ts.line_number (), w1);
              }

            } else if (i->second == ViaGeometry) {

              if (ex.test (":SIZE:")) {
                double sx = 0.0, sy = 0.0;
                ex.read (sx);
                ex.test("X");
                ex.read (sy);
                via_size = db::DVector (sx, sy);
              }

            }

          }

          if (ex.test (":MASK:")) {
            ex.read (mask);
          }

          if (i == purpose_translation.end ()) {

            tl::warn << tl::sprintf (tl::to_string (tr ("Reading layer map file %s, line %d: purpose %s ignored for layer %s")), path, ts.line_number (), ps, w1);

          } else if (i->second == All) {

            for (LayerPurpose *p = all_purposes; p != all_purposes + sizeof (all_purposes) / sizeof (all_purposes[0]); ++p) {
              translated_purposes.insert (LayerDetailsKey (*p, mask, via_size));
            }

          } else {

            translated_purposes.insert (LayerDetailsKey (i->second, mask, via_size));

          }

        }

        //  create a visual description string for the combined purposes
        std::string purpose_str;

        for (std::set<LayerDetailsKey>::const_iterator p = translated_purposes.begin (); p != translated_purposes.end (); ++p) {

          if (p != translated_purposes.begin ()) {
            purpose_str += "/";
          }

          std::string ps = layer_spec_to_name (std::string (), p->purpose, p->mask, p->via_size);

          if (p != translated_purposes.begin () && (purpose_str + ps).size () > max_purpose_str) {
            purpose_str += "...";
            break;
          } else {
            purpose_str += ps;
          }

        }

        std::string final_name = w1 + "." + purpose_str;

        for (std::set<LayerDetailsKey>::const_iterator p = translated_purposes.begin (); p != translated_purposes.end (); ++p) {
          for (std::vector<int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
            for (std::vector<int>::const_iterator d = datatypes.begin (); d != datatypes.end (); ++d) {
              layer_map [std::make_pair (w1, *p)].push_back (db::LayerProperties (*l, *d, final_name));
            }
          }
        }

      }

    }

  }
}

/**
 *  @brief Returns true, if the layer purpose has a fallback
 */
static bool has_fallback (LayerPurpose p)
{
  return p == RegionsFence || p == RegionsGuide || p == RegionsNone;
}

std::set <unsigned int>
LEFDEFReaderState::open_layer (db::Layout &layout, const std::string &n, LayerPurpose purpose, unsigned int mask, const db::DVector &via_size)
{
  std::map <std::pair<std::string, LayerDetailsKey>, std::set<unsigned int> >::const_iterator nl;
  nl = m_layers.find (std::make_pair (n, LayerDetailsKey (purpose, mask, via_size)));
  if (nl == m_layers.end ()) {
    nl = m_layers.find (std::make_pair (n, LayerDetailsKey (purpose, mask)));
  }
  if (nl == m_layers.end ()) {

    std::set <unsigned int> ll;

    if (! m_has_explicit_layer_mapping) {
      ll = open_layer_uncached (layout, n, purpose, mask);
    }

    m_layers.insert (std::make_pair (std::make_pair (n, LayerDetailsKey (purpose, mask)), ll));

    if (ll.empty () && ! has_fallback (purpose)) {
      if (n.empty ()) {
        tl::warn << tl::to_string (tr ("No mapping for purpose")) << " '" << purpose_to_name (purpose) << "'" << tl::noendl;
      } else {
        tl::warn << tl::to_string (tr ("No mapping for layer")) << " '" << n << "', purpose '" << purpose_to_name (purpose) << "'" << tl::noendl;
      }
      if (mask > 0) {
        tl::warn << tl::to_string (tr (" Mask ")) << mask << tl::noendl;
      }
//  not printing via size - too confusing?
#if 0
      if (via_size != db::DVector ()) {
        tl::warn << tl::to_string (tr (" Via size ")) << via_size.to_string () << tl::noendl;
      }
#endif
      tl::warn << tl::to_string (tr (" - layer is ignored"));
    }

    return ll;

  } else {
    return nl->second;
  }
}

/**
 *  @brief Implements implicit layer mapping
 *
 *  This is how Implicit layer mapping works:
 *
 *  1. For named layers (e.g. routing, pin, etc.
 *
 *  A decorated name is formed from the basic name and the purpose string (e.g. "M1" -> "M1.PIN").
 *  With the example of "M1" and purpose Pin (decorated name "M1.PIN") and with a tech component datatype specification
 *  of "5" for "Pin", the layer map entries have the following effect:
 *
 *     Layer map                     Result
 *
 *     (nothing)                     M1.PIN (default/5)          (only if "create_all_layers" is ON, "default" is a default number assigned by the reader)
 *     M1.PIN : 1/0                  M1.PIN (1/0)
 *     M1.PIN : 1/17                 M1.PIN (1/17)
 *     M1 : 1/0                      M1.PIN (1/5)
 *     M1 : 1/2                      M1.PIN (1/7)                (datatypes will add)
 *     M1                            M1.PIN (default/5)
 *     M1 : METAL1                   METAL1.PIN (default/5)      (name is taken from layer map and decorated)
 *     M1 : METAL1 (1/2)             METAL1.PIN (1/7)
 *     M1.PIN : METAL1_PIN           METAL1_PIN (default/5)      (specific name is used without decoration)
 *     M1.PIN : METAL1_PIN (1/17)    METAL1_PIN (1/17)           (full and specific mapping)
 *
 *  2. For general layers (e.g. outline)
 *
 *  By default, the name, layer and datatype are taken from the tech component's specification. The specification may
 *  lack the layer and datatype and even the name. If the name is missing, it is generated from the purpose.
 *
 *  Here are some examples for the mapping of "OUTLINE":
 *
 *    Tech component        Layer map          Result
 *
 *    (nothing)             (nothing)          OUTLINE            (only if "create_all_layers" is ON)
 *    OUTL                  (nothing)          OUTL (default/0)   ("default" is a default number assigned by the reader)
 *    OUTL (4/17)           (nothing)          OUTL (4/17)
 *    OUTL                  OUTL : 5/1         OUTL (5/1)
 *    OUTL (4/17)           OUTL : 4/11        OUTL 4/11
 *    OUTL (4/17)           4/17 : 4/11        OUTL 4/11
 *    4/17                  4/17 : 4/11        OUTLINE 4/11
 */

std::set<unsigned int> LEFDEFReaderState::open_layer_uncached(db::Layout &layout, const std::string &n, LayerPurpose purpose, unsigned int mask)
{
  if (n.empty ()) {

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
      return std::set<unsigned int> ();
    }

    db::LayerProperties lp;
    tl::Extractor ex (ld.c_str ());
    try {
      ex.read (lp);
    } catch (...) {
      lp.layer = 0;
      lp.datatype = 0;
    }

    //  if no name is given, derive one from the purpose
    if (lp.name.empty ()) {
      lp.name = purpose_to_name (purpose);
    }

    if (lp.layer < 0) {
      std::map<std::string, int>::const_iterator ldef = m_default_number.find (lp.name);
      if (ldef != m_default_number.end ()) {
        lp.layer = ldef->second;
        lp.datatype = 0;
      }
    }

    //  employ the layer map to find the target layer
    std::set<unsigned int> ll = m_layer_map.logical (lp, layout);
    if (ll.empty () && ! m_create_layers) {
      return std::set<unsigned int> ();
    }

    std::set<unsigned int> res;

    //  map the layers to targets from the layout
    //  (NOTE: the other readers will do this in advance, but LEF/DEF is too dynamic)

    bool at_least_once = true;
    for (std::set<unsigned int>::const_iterator l = ll.begin (); l != ll.end () || at_least_once; ++l) {

      at_least_once = false;

      //  If the layer map provides a target, use that one for the layer
      db::LayerProperties lp_new = lp;
      const db::LayerProperties *lpp = (l == ll.end () ? 0 : m_layer_map.target (*l));
      if (lpp) {
        if (! lpp->name.empty ()) {
          lp_new.name = lpp->name;
        }
        if (lpp->datatype >= 0) {
          lp_new.datatype = lpp->datatype;
        }
        if (lpp->layer >= 0) {
          lp_new.layer = lpp->layer;
        }
      }

      bool found = false;
      for (db::Layout::layer_iterator i = layout.begin_layers (); i != layout.end_layers () && ! found; ++i) {
        if ((*i).second->log_equal (lp_new)) {
          found = true;
          res.insert ((*i).first);
        }
      }

      if (! found) {
        res.insert (layout.insert_layer (lp_new));
      }

      if (l == ll.end ()) {
        break;
      }

    }

    return res;

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
      case LEFLabel:
        produce = mp_tech_comp->produce_lef_labels ();
        break;
      case Pins:
        produce = mp_tech_comp->produce_pins ();
        break;
      case Fills:
      case FillsOPC:
        produce = mp_tech_comp->produce_fills ();
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
        return std::set<unsigned int> ();
      }
    }

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
      case LEFLabel:
        name_suffix = mp_tech_comp->lef_labels_suffix ();
        dt = mp_tech_comp->lef_labels_datatype ();
        break;
      case Pins:
        name_suffix = mp_tech_comp->pins_suffix_per_mask (mask);
        dt = mp_tech_comp->pins_datatype_per_mask (mask);
        break;
      case Fills:
      case FillsOPC:
        name_suffix = mp_tech_comp->fills_suffix_per_mask (mask);
        dt = mp_tech_comp->fills_datatype_per_mask (mask);
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

    //  "name" is the decorated name as provided by the tech component's x_suffix specifications.
    std::string name = n + name_suffix;

    //  Assign a layer number (a default one for now) and the datatype from the tech component's x_datatype specification.
    db::LayerProperties lp (name);
    lp.datatype = dt;
    std::map<std::string, int>::const_iterator ldef = m_default_number.find (n);
    if (ldef != m_default_number.end ()) {
      lp.layer = ldef->second;
    }

    //  Route the layer through the layer map, first the decorated name and if there is no mapping, the
    //  undecorated one.
    std::set<unsigned int> ll = m_layer_map.logical (name, layout);
    bool generic_match = false;
    if (ll.empty ()) {
      ll = m_layer_map.logical (n, layout);
      generic_match = true;
    } else if (n == name) {
      //  no suffix defined in tech component -> treat as generic match and combine datatypes
      generic_match = true;
    }

    if (ll.empty () && ! m_create_layers) {
      return std::set<unsigned int> ();
    }

    std::set<unsigned int> res;

    bool at_least_once = true;
    for (std::set<unsigned int>::const_iterator l = ll.begin (); l != ll.end () || at_least_once; ++l) {

      at_least_once = false;

      //  If the layer map provides a target, use that one for the layer
      db::LayerProperties lp_new = lp;
      const db::LayerProperties *lpp = (l == ll.end () ? 0 : m_layer_map.target (*l));
      if (lpp) {
        lp_new = *lpp;
        if (lp_new.datatype < 0) {
          lp_new.datatype = dt;
        } else if (generic_match) {
          lp_new.datatype += dt;
        }
        if (lp_new.name.empty ()) {
          lp_new.name = name;
        } else if (generic_match) {
          lp_new.name += name_suffix;
        }
      }

      int lfound = -1;
      if (lp_new.layer >= 0 && lp_new.datatype >= 0) {
        for (db::Layout::layer_iterator i = layout.begin_layers (); i != layout.end_layers () && lfound < 0; ++i) {
          if ((*i).second->log_equal (lp_new)) {
            lfound = int ((*i).first);
          }
        }
      }

      if (lfound < 0) {
        res.insert (layout.insert_layer (lp_new));
      } else {
        res.insert ((unsigned int) lfound);
        db::LayerProperties lp_org = layout.get_properties ((unsigned int) lfound);
        join_layer_names (lp_org.name, name);
        layout.set_properties ((unsigned int) lfound, lp_org);
      }

      if (l == ll.end ()) {
        break;
      }

    }

    return res;

  }
}

void
LEFDEFReaderState::finish (db::Layout &layout)
{
  CommonReaderBase::finish (layout);

  int lnum = 0;

  std::set<int> used_numbers;
  for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
    if ((*l).second->layer >= 0) {
      used_numbers.insert ((*l).second->layer);
    }
  }

  std::map<std::string, int> number_for_name = m_default_number;

  for (std::map<std::string, int>::const_iterator ln = number_for_name.begin (); ln != number_for_name.end (); ++ln) {
    used_numbers.insert (ln->second);
  }

  //  Assign default numbers and generate a canonical mapping

  db::LayerMap lm;

  for (std::map <std::pair<std::string, LayerDetailsKey>, std::set<unsigned int> >::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {

    if (l->second.empty ()) {
      continue;
    }

    std::string n = layer_spec_to_name (l->first.first, l->first.second.purpose, l->first.second.mask, l->first.second.via_size);

    for (std::set<unsigned int>::const_iterator li = l->second.begin (); li != l->second.end (); ++li) {

      unsigned int layer_index = *li;
      db::LayerProperties lp = layout.get_properties (layer_index);

      if (lp.layer < 0) {

        std::map<std::string, int>::const_iterator n4n = number_for_name.end ();
        if (! l->first.first.empty ()) {
          n4n = number_for_name.find (l->first.first);
        }

        if (n4n == number_for_name.end ()) {
          do {
            ++lnum;
          } while (used_numbers.find (lnum) != used_numbers.end ());
          number_for_name.insert (std::make_pair (l->first.first, lnum));
          lp.layer = lnum;
        } else {
          lp.layer = n4n->second;
        }

      }

      if (lp.datatype < 0) {
        lp.datatype = 0;
      }

      layout.set_properties (layer_index, lp);

      lm.mmap (db::LayerProperties (n), layer_index, lp);

    }

  }

  //  On return we deliver the "canonical" map which lists the decorated name vs. the real ones.
  m_layer_map = lm;
}

void
LEFDEFReaderState::register_via_cell (const std::string &vn, const std::string &nondefaultrule, LEFDEFLayoutGenerator *generator)
{
  if (m_via_generators.find (std::make_pair (vn, nondefaultrule)) != m_via_generators.end ()) {
    delete m_via_generators [std::make_pair (vn, nondefaultrule)];
  }
  m_via_generators [std::make_pair (vn, nondefaultrule)] = generator;
}

LEFDEFLayoutGenerator *
LEFDEFReaderState::via_generator (const std::string &vn, const std::string &nondefaultrule)
{
  std::map<std::pair<std::string, std::string>, LEFDEFLayoutGenerator *>::const_iterator g = m_via_generators.find (std::make_pair (vn, nondefaultrule));
  if (g == m_via_generators.end () && ! nondefaultrule.empty ()) {
    //  default rule is fallback
    g = m_via_generators.find (std::make_pair (vn, std::string ()));
  }
  if (g != m_via_generators.end ()) {
    return g->second;
  } else {
    return 0;
  }
}

db::Cell *
LEFDEFReaderState::via_cell (const std::string &vn, const std::string &nondefaultrule, db::Layout &layout, unsigned int mask_bottom, unsigned int mask_cut, unsigned int mask_top, const LEFDEFNumberOfMasks *nm)
{
  ViaKey vk (vn, nondefaultrule, mask_bottom, mask_cut, mask_top);

  std::map<std::pair<std::string, std::string>, LEFDEFLayoutGenerator *>::const_iterator g = m_via_generators.find (std::make_pair (vn, nondefaultrule));

  if (g == m_via_generators.end () && ! vk.nondefaultrule.empty ()) {
    //  default rule is fallback
    g = m_via_generators.find (std::make_pair (vn, std::string ()));
    vk.nondefaultrule.clear ();
  }

  std::map<ViaKey, db::Cell *>::const_iterator i = m_via_cells.find (vk);
  if (i == m_via_cells.end ()) {

    db::Cell *cell = 0;

    if (g != m_via_generators.end ()) {

      LEFDEFLayoutGenerator *vg = g->second;

      std::string n = vn;

      if (! vk.nondefaultrule.empty ()) {
        n += "_";
        n += vk.nondefaultrule;
      }

      if (mask_bottom > 0 || mask_cut > 0 || mask_top > 0) {
        n += "_";
        n += tl::to_string (mask_bottom);
        n += "_";
        n += tl::to_string (mask_cut);
        n += "_";
        n += tl::to_string (mask_top);
      }

      std::string cn = mp_tech_comp->via_cellname_prefix () + n;
      cell = &layout.cell (make_cell (layout, cn.c_str ()));

      std::vector<unsigned int> masks;
      masks.reserve (3);
      masks.push_back (mask_bottom);
      masks.push_back (mask_cut);
      masks.push_back (mask_top);

      vg->create_cell (*this, layout, *cell, 0, masks, nm);

    }

    m_via_cells[vk] = cell;
    return cell;

  } else {
    tl_assert (! i->second || i->second->layout () == &layout);
    return i->second;
  }
}

void
LEFDEFReaderState::register_macro_cell (const std::string &mn, LEFDEFLayoutGenerator *generator)
{
  if (m_macro_generators.find (mn) != m_macro_generators.end ()) {
    delete m_macro_generators [mn];
  }
  m_macro_generators [mn] = generator;
}

LEFDEFLayoutGenerator *
LEFDEFReaderState::macro_generator (const std::string &mn)
{
  std::map<std::string, LEFDEFLayoutGenerator *>::const_iterator g = m_macro_generators.find (mn);
  if (g != m_macro_generators.end ()) {
    return g->second;
  } else {
    return 0;
  }
}

db::cell_index_type
LEFDEFReaderState::foreign_cell (Layout &layout, const std::string &name)
{
  std::map<std::string, db::cell_index_type>::const_iterator c = m_foreign_cells.find (name);
  if (c != m_foreign_cells.end ()) {
    return c->second;
  }

  std::pair<bool, db::cell_index_type> cc = layout.cell_by_name (name.c_str ());

  db::cell_index_type ci;
  if (cc.first) {
    ci = cc.second;
  } else {
    ci = make_cell (layout, name.c_str ());
    layout.cell (ci).set_ghost_cell (true);
  }

  m_foreign_cells.insert (std::make_pair (name, ci));
  return ci;
}

std::pair<db::Cell *, db::Trans>
LEFDEFReaderState::macro_cell (const std::string &mn, Layout &layout, const std::vector<std::string> &maskshift_layers, const std::vector<unsigned int> &masks, const MacroDesc &macro_desc, const LEFDEFNumberOfMasks *nm)
{
  std::map<std::string, LEFDEFLayoutGenerator *>::const_iterator g = m_macro_generators.find (mn);
  if (g == m_macro_generators.end ()) {
    return std::make_pair ((db::Cell *) 0, db::Trans ());
  }

  LEFDEFLayoutGenerator *mg = g->second;

  MacroKey mk;
  if (mg->is_fixedmask ()) {
    mk = MacroKey (mn, std::vector<unsigned int> ());
  } else {
    mk = MacroKey (mn, masks);
  }

  std::map<MacroKey, std::pair<db::Cell *, db::Trans> >::const_iterator i = m_macro_cells.find (mk);
  if (i != m_macro_cells.end ()) {
    tl_assert (! i->second.first || i->second.first->layout () == &layout);
    return i->second;
  }

  db::Cell *cell = 0;
  db::Trans tr;

  if (! macro_desc.foreign_name.empty ()) {

    db::cell_index_type ci = foreign_cell (layout, macro_desc.foreign_name);
    db::Cell *foreign_cell = &layout.cell (ci);

    if (macro_desc.foreign_name != mn) {

      //  create an indirection for renaming the cell
      cell = &layout.cell (make_cell (layout, mn.c_str ()));
      cell->insert (db::CellInstArray (db::CellInst (foreign_cell->cell_index ()), db::Trans (db::Point () - macro_desc.origin) * macro_desc.foreign_trans));

    } else {

      //  use FOREIGN cell instead of new one
      cell = foreign_cell;
      tr = db::Trans (db::Point () - macro_desc.origin) * macro_desc.foreign_trans;

    }

  } else if (tech_comp ()->macro_resolution_mode () == 2) {

    //  create a ghost cell always

    db::cell_index_type ci = foreign_cell (layout, mn);
    cell = &layout.cell (ci);

  } else {

    //  actually implement the real cell

    std::string mask_suffix;
    if (! mg->is_fixedmask ()) {
      for (std::vector<unsigned int>::const_iterator m = masks.begin (); m != masks.end (); ++m) {
        mask_suffix += "_";
        mask_suffix += tl::to_string (*m);
      }
    }

    std::string cn = mn + mask_suffix;

    cell = &layout.cell (make_cell (layout, cn.c_str ()));

    if (mg->is_fixedmask ()) {
      mg->create_cell (*this, layout, *cell, 0, std::vector<unsigned int> (), nm);
    } else {
      mg->create_cell (*this, layout, *cell, &maskshift_layers, masks, nm);
    }

  }

  m_macro_cells [mk] = std::make_pair (cell, tr);
  return std::make_pair (cell, tr);
}

// -----------------------------------------------------------------------------------
//  LEFDEFImporter implementation

LEFDEFImporter::LEFDEFImporter (int warn_level)
  : mp_progress (0), mp_stream (0), mp_reader_state (0),
    m_produce_net_props (false), m_net_prop_name_id (0),
    m_produce_inst_props (false), m_inst_prop_name_id (0),
    m_produce_pin_props (false), m_pin_prop_name_id (0),
    m_warn_level (warn_level)
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
  tl::log << tl::to_string (tr ("Reading LEF/DEF file")) << " " << stream.absolute_path ();

  m_fn = stream.filename ();

  tl::AbsoluteProgress progress (tl::to_string (tr ("Reading ")) + m_fn, 1000);
  progress.set_format (tl::to_string (tr ("%.0fk lines")));
  progress.set_format_unit (1000.0);
  progress.set_unit (10000.0);

  mp_reader_state = &state;
  mp_reader_state->attach_reader (this);

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

    mp_reader_state->attach_reader (0);
    delete mp_stream;
    mp_stream = 0;
    mp_progress = 0;

  } catch (...) {
    mp_reader_state->attach_reader (0);
    delete mp_stream;
    mp_stream = 0;
    mp_progress = 0;
    throw;
  }
}

void 
LEFDEFImporter::error (const std::string &msg)
{
  if (m_sections.empty ()) {
    throw LEFDEFReaderException (msg, int (mp_stream->line_number ()), m_cellname, m_fn);
  } else {
    throw LEFDEFReaderException (msg + tl::sprintf (tl::to_string (tr (" (inside %s)")), tl::join (m_sections, "/")), int (mp_stream->line_number ()), m_cellname, m_fn);
  }
}

void 
LEFDEFImporter::warn (const std::string &msg, int wl)
{
  if (m_warn_level < wl) {
    return;
  }

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

void
LEFDEFImporter::enter_section (const std::string &name)
{
  m_sections.push_back (name);
}

void
LEFDEFImporter::leave_section ()
{
  m_sections.pop_back ();
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

