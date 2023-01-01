
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


#include "dbGlyphs.h"
#include "dbLayout.h"
#include "dbEdgeProcessor.h"
#include "dbReader.h"
#include "tlStream.h"
#include "tlFileUtils.h"

#if defined(HAVE_QT)
#  include <QResource>
#  include <QByteArray>
#  include <QFileInfo>
#endif

#include <cctype>

//  compiled with "scripts/compile_glyphs.rb":
#include "glyphs.cc_gen"

namespace db
{

TextGenerator::TextGenerator ()
  : m_width (1000), m_height (1000), m_line_width (100), m_design_grid (10),
    m_dbu (0.001), m_lowercase_supported (false)
{
  //  .. nothing yet ..
}

const std::vector<db::Polygon> &
TextGenerator::glyph (char c) const
{
  std::map<char, std::vector<db::Polygon> >::const_iterator dc = m_data.find (m_lowercase_supported ? c : toupper(c));
  if (dc != m_data.end ()) {
    return dc->second;
  } else {
    static std::vector<db::Polygon> empty_polygons;
    return empty_polygons;
  }
}

db::Region
TextGenerator::glyph_as_region (char c) const
{
  db::Region region;
  std::map<char, std::vector<db::Polygon> >::const_iterator dc = m_data.find (m_lowercase_supported ? c : toupper(c));
  if (dc != m_data.end ()) {
    for (std::vector<db::Polygon>::const_iterator p = dc->second.begin (); p != dc->second.end (); ++p) {
      region.insert (*p);
    }
  }
  return region;
}

void
TextGenerator::text (const std::string &t, double target_dbu, double mag, bool inv, double bias, double char_spacing, double line_spacing, std::vector <db::Polygon> &data) const
{
  data.clear ();
  db::EdgeProcessor ep;

  double m = mag * dbu () / target_dbu;
  db::Coord b = db::coord_traits<db::Coord>::rounded (bias / target_dbu);

  db::Coord x = 0, y = 0;
  db::Coord dx = db::coord_traits<db::Coord>::rounded (m * width () + char_spacing / target_dbu);
  db::Coord dy = db::coord_traits<db::Coord>::rounded (m * height () + line_spacing / target_dbu);

  db::Box bb;

  for (const char *cp = t.c_str (); *cp; ++cp) {

    char c = *cp;
    if (c == '\\' && cp [1]) {
      if (cp [1] == 'n') {
        ++cp;
        y -= dy;
        x = 0;
        c = 0;
      } else {
        ++cp;
        c = *cp;
      }
    }

    if (c) {

      db::ICplxTrans trans (m, 0.0, false, db::Vector (x, y));

      const std::vector<db::Polygon> &g = glyph (c);
      for (std::vector<db::Polygon>::const_iterator d = g.begin (); d != g.end (); ++d) {
        data.push_back (d->transformed (trans));
      }

      bb += background ().transformed (trans);

      x += dx;

    }

  }

  if (b != 0) {
    std::vector<db::Polygon> sized_data;
    ep.size (data, b, b, sized_data);
    data.swap (sized_data);
  }

  if (inv && ! bb.empty ()) {
    std::vector<db::Polygon> bg, in;
    bg.push_back (db::Polygon (bb));
    data.swap (in);
    ep.boolean (bg, in, data, db::BooleanOp::ANotB, true, true);
  }
}

db::Region
TextGenerator::text_as_region (const std::string &t, double target_dbu, double mag, bool inv, double bias, double char_spacing, double line_spacing) const
{
  std::vector<db::Polygon> poly;
  text (t, target_dbu, mag, inv, bias, char_spacing, line_spacing, poly);
  db::Region region;
  for (std::vector<db::Polygon>::const_iterator p = poly.begin (); p != poly.end (); ++p) {
    region.insert (*p);
  }
  return region;
}

void
TextGenerator::load_from_resource (const std::string &name)
{
  load_from_file (name);
}

void
TextGenerator::load_from_data (const char *data, size_t ndata, const std::string &name, const std::string &description)
{
  db::Layout layout;
  tl::InputMemoryStream memory_stream (data, ndata);
  tl::InputStream stream (memory_stream);
  db::Reader reader (stream);
  db::LayerMap map = reader.read (layout);

  m_description = description;
  m_name = name;

  std::pair<bool, unsigned int> l1 = map.first_logical (db::LDPair (1, 0));
  std::pair<bool, unsigned int> l2 = map.first_logical (db::LDPair (2, 0));
  std::pair<bool, unsigned int> l3 = map.first_logical (db::LDPair (3, 0));

  if (l1.first && l2.first) {
    read_from_layout (layout, l1.second, l2.second, l3.second);
  }
}

void
TextGenerator::load_from_file (const std::string &filename)
{
  db::Layout layout;
  tl::InputStream stream (filename);
  db::Reader reader (stream);
  db::LayerMap map = reader.read (layout);

  m_description = filename;

  std::pair<bool, unsigned int> l1 = map.first_logical (db::LDPair (1, 0));
  std::pair<bool, unsigned int> l2 = map.first_logical (db::LDPair (2, 0));
  std::pair<bool, unsigned int> l3 = map.first_logical (db::LDPair (3, 0));

  if (l1.first && l2.first) {
    read_from_layout (layout, l1.second, l2.second, l3.second);
  }

  m_name = tl::basename (filename);
}

void
TextGenerator::read_from_layout (const db::Layout &layout, unsigned int l1, unsigned int l2, unsigned int l3)
{
  m_dbu = layout.dbu ();

  //  try to read the comment
  std::pair<bool, db::cell_index_type> cn = layout.cell_by_name ("COMMENT");
  if (cn.first) {

    db::Shapes::shape_iterator sh = layout.cell (cn.second).shapes (l1).begin (db::ShapeIterator::All);
    while (! sh.at_end ()) {

      if (sh->is_text ()) {

        std::string s = sh->text_string ();
        tl::Extractor ex (s.c_str ());

        if (ex.test ("line_width")) {

          ex.test ("=");
          m_line_width = 0;
          ex.try_read (m_line_width);

        } else if (ex.test ("design_grid")) {

          ex.test ("=");
          m_design_grid = 0;
          ex.try_read (m_design_grid);

        } else {
          m_description = sh->text_string ();
        }

      }

      ++sh;

    }

  }

  m_lowercase_supported = layout.cell_by_name ("a").first || layout.cell_by_name ("065").first;

  db::Box bbox, bg;

  //  read the data and determine the bounding box
  for (int ch = 32; ch < 128; ++ch) {

    char n[32];
    n[0] = char (ch);
    n[1] = 0;

    std::pair<bool, db::cell_index_type> cn = layout.cell_by_name (n);
    if (! cn.first) {
      sprintf (n, "%03d", ch);
      cn = layout.cell_by_name (n);
    }

    if (cn.first) {

      std::vector<db::Polygon> &data = m_data.insert (std::make_pair (char (ch), std::vector<db::Polygon> ())).first->second;

      bbox += layout.cell (cn.second).bbox (l2);

      bg += layout.cell (cn.second).bbox (l2);
      bg += layout.cell (cn.second).bbox (l3);

      db::Shapes::shape_iterator sh = layout.cell (cn.second).shapes (l1).begin (db::ShapeIterator::All);
      while (! sh.at_end ()) {
        if (sh->is_box () || sh->is_path () || sh->is_polygon ()) {
          data.push_back (db::Polygon ());
          sh->instantiate (data.back ());
        }
        ++sh;
      }

    }

  }

  if (! bbox.empty ()) {
    m_width = bbox.width ();
    m_height = bbox.height ();
  }

  m_background = bg;
}

const TextGenerator *
TextGenerator::generator_by_name (const std::string &name)
{
  const std::vector<TextGenerator> &fonts = generators ();
  for (std::vector<TextGenerator>::const_iterator f = fonts.begin (); f != fonts.end (); ++f) {
    if (f->name () == name) {
      return f.operator-> ();
    }
  }
  return 0;
}

const TextGenerator *
TextGenerator::default_generator ()
{
  const std::vector<TextGenerator> &fonts = generators ();
  return fonts.empty () ? 0 : &fonts [0];
}


static std::vector<std::string> s_font_paths;
static std::vector<TextGenerator> s_fonts;
static bool s_fonts_loaded = false;

void
TextGenerator::set_font_paths (const std::vector<std::string> &paths)
{
  s_font_paths = paths;
  s_fonts.clear ();
  s_fonts_loaded = false;
}

std::vector<std::string>
TextGenerator::font_paths ()
{
  return s_font_paths;
}

const std::vector<TextGenerator> &
TextGenerator::generators ()
{
  if (! s_fonts_loaded) {

    s_fonts.clear ();

    //  load the compiled-in glyphs
    load_glyphs (s_fonts);

    //  scan for font files
    for (std::vector<std::string>::const_iterator p = s_font_paths.begin (); p != s_font_paths.end (); ++p) {

      if (tl::file_exists (*p)) {

        std::vector<std::string> font_files = tl::dir_entries (*p, true, false, true);
        for (std::vector<std::string>::const_iterator ff = font_files.begin (); ff != font_files.end (); ++ff) {

          try {
            std::string ffp = tl::combine_path (*p, *ff);
            tl::log << "Loading font from " << ffp << " ..";
            s_fonts.push_back (TextGenerator ());
            s_fonts.back ().load_from_file (ffp);
          } catch (tl::Exception &ex) {
            tl::error << ex.msg ();
            s_fonts.pop_back ();
          }

        }

      }

    }

    s_fonts_loaded = true;

  }

  return s_fonts;
}

}
