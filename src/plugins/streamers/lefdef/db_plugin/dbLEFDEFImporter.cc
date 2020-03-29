
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

#include "tlStream.h"
#include "tlProgress.h"

#include <cctype>

namespace db
{

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
    m_produce_pins (true),
    m_pins_suffix (".PIN"),
    m_pins_datatype (2),
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
    m_routing_datatype (0)
{
  //  .. nothing yet ..
}

LEFDEFReaderOptions::LEFDEFReaderOptions (const LEFDEFReaderOptions &d)
  : db::FormatSpecificReaderOptions (d),
    m_read_all_layers (d.m_read_all_layers), m_layer_map (d.m_layer_map),
    m_dbu (d.m_dbu),
    m_produce_net_names (d.m_produce_net_names),
    m_net_property_name (d.m_net_property_name),
    m_produce_inst_names (d.m_produce_inst_names),
    m_inst_property_name (d.m_inst_property_name),
    m_produce_pin_names (d.m_produce_pin_names),
    m_pin_property_name (d.m_pin_property_name),
    m_produce_cell_outlines (d.m_produce_cell_outlines),
    m_cell_outline_layer (d.m_cell_outline_layer),
    m_produce_placement_blockages (d.m_produce_placement_blockages),
    m_placement_blockage_layer (d.m_placement_blockage_layer),
    m_produce_regions (d.m_produce_regions),
    m_region_layer (d.m_region_layer),
    m_produce_via_geometry (d.m_produce_via_geometry),
    m_via_geometry_suffix (d.m_via_geometry_suffix),
    m_via_geometry_datatype (d.m_via_geometry_datatype),
    m_produce_pins (d.m_produce_pins),
    m_pins_suffix (d.m_pins_suffix),
    m_pins_datatype (d.m_pins_datatype),
    m_produce_obstructions (d.m_produce_obstructions),
    m_obstructions_suffix (d.m_obstructions_suffix),
    m_obstructions_datatype (d.m_obstructions_datatype),
    m_produce_blockages (d.m_produce_blockages),
    m_blockages_suffix (d.m_blockages_suffix),
    m_blockages_datatype (d.m_blockages_datatype),
    m_produce_labels (d.m_produce_labels),
    m_labels_suffix (d.m_labels_suffix),
    m_labels_datatype (d.m_labels_datatype),
    m_produce_routing (d.m_produce_routing),
    m_routing_suffix (d.m_routing_suffix),
    m_routing_datatype (d.m_routing_datatype),
    m_lef_files (d.m_lef_files)
{
  //  .. nothing yet ..
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

// -----------------------------------------------------------------------------------
//  LEFDEFLayerDelegate implementation

LEFDEFLayerDelegate::LEFDEFLayerDelegate (const LEFDEFReaderOptions *tc)
  : m_create_layers (true), m_laynum (1), mp_tech_comp (tc)
{
  if (tc) {
    m_layer_map = tc->layer_map ();
    m_create_layers = tc->read_all_layers ();
  }
}

void
LEFDEFLayerDelegate::register_layer (const std::string &ln)
{
  m_default_number.insert (std::make_pair (ln, m_laynum));
  ++m_laynum;
}

std::pair <bool, unsigned int> 
LEFDEFLayerDelegate::open_layer (db::Layout &layout, const std::string &n, LayerPurpose purpose) 
{
  if (purpose == Outline || purpose == PlacementBlockage || purpose == Region) {

    //  NOTE: the canonical name is independent from the tech component's settings
    //  as is "(name)". It's used for implementing the automatic map file import
    //  feature.
    std::string ld;
    std::string canonical_name;
    bool produce;

    if (purpose == Outline) {
      produce = mp_tech_comp->produce_cell_outlines ();
      ld = mp_tech_comp->cell_outline_layer ();
      canonical_name = "(OUTLINE)";
    } else if (purpose == Region) {
      produce = mp_tech_comp->produce_regions ();
      ld = mp_tech_comp->region_layer ();
      canonical_name = "(REGION)";
    } else {
      produce = mp_tech_comp->produce_placement_blockages ();
      ld = mp_tech_comp->placement_blockage_layer ();
      canonical_name = "(BLK)";
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

    } else if ((ll = m_layer_map.logical (db::LayerProperties (canonical_name), layout)).first) {

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
      case ViaGeometry:
        produce = mp_tech_comp->produce_via_geometry ();
        break;
      case Label:
        produce = mp_tech_comp->produce_labels ();
        break;
      case Pins:
        produce = mp_tech_comp->produce_pins ();
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
    //  x_suffix specifications. As this is a variable entity, we also provide
    //  a canonical name of the form "(layer,purpose)" where purpose is a
    //  predefined suffix. The canonical name is the last fallback. Hence this
    //  allows importing layer mapping files as canonical name mapping.
    std::string name (n);
    int dt = 0;

    std::string canonical_purpose;

    if (mp_tech_comp) {
      switch (purpose) {
      case Routing:
      default:
        name += mp_tech_comp->routing_suffix ();
        canonical_purpose = "NET";
        dt += mp_tech_comp->routing_datatype ();
        break;
      case ViaGeometry:
        name += mp_tech_comp->via_geometry_suffix ();
        dt += mp_tech_comp->via_geometry_datatype ();
        canonical_purpose = "VIA";
        break;
      case Label:
        name += mp_tech_comp->labels_suffix ();
        dt += mp_tech_comp->labels_datatype ();
        canonical_purpose = "LABEL";
        break;
      case Pins:
        name += mp_tech_comp->pins_suffix ();
        dt += mp_tech_comp->pins_datatype ();
        canonical_purpose = "PIN";
        break;
      case Obstructions:
        name += mp_tech_comp->obstructions_suffix ();
        dt += mp_tech_comp->obstructions_datatype ();
        canonical_purpose = "OBS";
        break;
      case Blockage:
        name += mp_tech_comp->blockages_suffix ();
        dt += mp_tech_comp->blockages_datatype ();
        canonical_purpose = "BLK";
        break;
      }
    }

    std::string canonical_name = std::string ("(") + n + "," + canonical_purpose + ")";

    std::pair<bool, unsigned int> ll = m_layer_map.logical (name, layout);

    if (ll.first) {

      return ll;

    } else if ((ll = m_layer_map.logical (db::LayerProperties (canonical_name), layout)).first) {

      //  final fallback: try canonical name
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

        std::map <std::pair<std::string, LayerPurpose>, unsigned int>::const_iterator nl = m_layers.find (std::make_pair (n, purpose));
        if (nl == m_layers.end ()) {
          unsigned int li = layout.insert_layer (db::LayerProperties (name));
          m_layer_map.map (db::LayerProperties (name), li);
          m_layers.insert (std::make_pair (std::make_pair (n, purpose), li));
          return std::pair<bool, unsigned int> (true, li);
        } else {
          return std::pair<bool, unsigned int> (true, nl->second);
        }

      }

    }

  }
}

void
LEFDEFLayerDelegate::prepare (db::Layout &layout)
{
  m_layer_map.prepare (layout);
}

void
LEFDEFLayerDelegate::finish (db::Layout &layout)
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

  for (std::map<std::pair<std::string, LayerPurpose>, unsigned int>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {

    int dt = 0;
    switch (l->first.second) {
    case Routing:
    default:
      dt = mp_tech_comp->routing_datatype ();
      break;
    case ViaGeometry:
      dt = mp_tech_comp->via_geometry_datatype ();
      break;
    case Label:
      dt = mp_tech_comp->labels_datatype ();
      break;
    case Pins:
      dt = mp_tech_comp->pins_datatype ();
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

// -----------------------------------------------------------------------------------
//  LEFDEFImporter implementation

LEFDEFImporter::LEFDEFImporter ()
  : mp_progress (0), mp_stream (0), mp_layer_delegate (0),
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

void 
LEFDEFImporter::read (tl::InputStream &stream, db::Layout &layout, LEFDEFLayerDelegate &ld)
{
  m_fn = stream.filename ();

  tl::AbsoluteProgress progress (tl::to_string (tr ("Reading ")) + m_fn, 1000);
  progress.set_format (tl::to_string (tr ("%.0fk lines")));
  progress.set_format_unit (1000.0);
  progress.set_unit (10000.0);

  m_produce_net_props = false;
  m_net_prop_name_id = 0;

  if (ld.tech_comp () && ld.tech_comp ()->produce_net_names ()) {
    m_produce_net_props = true;
    m_net_prop_name_id = layout.properties_repository ().prop_name_id (ld.tech_comp ()->net_property_name ());
  }

  m_produce_inst_props = false;
  m_inst_prop_name_id = 0;

  if (ld.tech_comp () && ld.tech_comp ()->produce_inst_names ()) {
    m_produce_inst_props = true;
    m_inst_prop_name_id = layout.properties_repository ().prop_name_id (ld.tech_comp ()->inst_property_name ());
  }

  m_produce_pin_props = false;
  m_pin_prop_name_id = 0;

  if (ld.tech_comp () && ld.tech_comp ()->produce_pin_names ()) {
    m_produce_pin_props = true;
    m_pin_prop_name_id = layout.properties_repository ().prop_name_id (ld.tech_comp ()->pin_property_name ());
  }

  try {

    mp_progress = &progress;
    mp_layer_delegate = &ld;
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

void 
LEFDEFImporter::create_generated_via (std::vector<db::Polygon> &bottom,
                                      std::vector<db::Polygon> &cut,
                                      std::vector<db::Polygon> &top,
                                      const db::Vector &cutsize,
                                      const db::Vector &cutspacing,
                                      const db::Vector &be, const db::Vector &te,
                                      const db::Vector &bo, const db::Vector &to,
                                      const db::Point &o,
                                      int rows, int columns,
                                      const std::string &pattern)
{
  db::Vector vs ((cutsize.x () * columns + cutspacing.x () * (columns - 1)) / 2, (cutsize.y () * rows + cutspacing.y () * (rows - 1)) / 2);
  db::Box via_box (o - vs, o + vs);

  bottom.push_back (db::Polygon (via_box.enlarged (be).moved (bo)));
  top.push_back (db::Polygon (via_box.enlarged (te).moved (to)));

  const char *p = pattern.c_str ();
  int rp = pattern.empty () ? -1 : 0;
  const char *p0 = p, *p1 = p;

  for (int r = 0; r < rows; ++r) {

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

      for (int c = 0; c < columns; ++c) {

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

          db::Vector vbl ((cutsize + cutspacing).x () * c, (cutsize + cutspacing).y () * r);
          db::Box vb (via_box.lower_left () + vbl, via_box.lower_left () + vbl + cutsize);
          cut.push_back (db::Polygon (vb));

        }

      }

    }

  }

}

}

