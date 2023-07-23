
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


#include "dbGerberImporter.h"
#include "dbGerberImportData.h"
#include "dbGerberDrillFileReader.h"
#include "dbRS274XReader.h"
#include "dbReader.h"
#include "dbStream.h"

#include "tlStream.h"
#include "tlString.h"
#include "tlString.h"
#include "tlLog.h"
#include "tlFileUtils.h"
#include "dbShapeProcessor.h"

#include <cmath>
#include <cctype>

namespace db
{

// ---------------------------------------------------------------------------------------
//  Some utilities

static void parse_format (const std::string &format, int &l, int &t, bool &tz)
{
  tl::Extractor ex (format.c_str ());

  try {

    l = -1; t = -1;
    tz = true;

    if (! ex.at_end ()) {

      if (! ex.test ("*")) {
        ex.try_read (l);
      }
      ex.expect (":");
      if (! ex.test ("*")) {
        ex.try_read (t);
      }
      
      if (ex.test ("T") || ex.test ("t")) {
        tz = true;
      } else if (ex.test ("L") || ex.test ("l")) {
        tz = false;
      } else {
        if (t < 0) {
          tz = false;
        } 
      }

      ex.expect_end ();

    }

  } catch (tl::Exception &ex) {
    throw tl::Exception (tl::to_string (tr ("Gerber format specification '%s' is invalid: %s")), format, ex.msg ());
  }
}

static std::string format_to_string (int l, int t, bool tz)
{
  std::string r;

  if (l >= 0) {
    r += tl::to_string (l);
  } else {
    r += "*";
  }

  r += ":";

  if (t >= 0) {
    r += tl::to_string (t);
  } else {
    r += "*";
  }

  r += (tz ? "T" : "L");

  return r;
}

// ---------------------------------------------------------------------------------------
//  Implementation of GerberFile

GerberFileReader::GerberFileReader (int warn_level)
  : m_circle_points (64), m_digits_before (-1), m_digits_after (-1), m_omit_leading_zeroes (true),
    m_merge (false), m_inverse (false),
    m_dbu (0.001), m_unit (1000.0),
    m_rot (0.0), m_s (1.0), m_ox (0.0), m_oy (0.0),
    m_mx (false), m_my (false),
    m_orot (0.0), m_os (1.0), m_omx (false), m_omy (false),
    m_ep (true /*report progress*/),
    mp_layout (0), mp_top_cell (0), mp_stream (0),
    m_progress (tl::to_string (tr ("Reading Gerber file")), 10000),
    m_warn_level (warn_level)
{
  m_progress.set_format (tl::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);
}

bool 
GerberFileReader::accepts (tl::TextInputStream &stream)
{
  mp_stream = &stream;
  bool result = does_accept ();
  mp_stream = 0;
  return result;
}

GerberMetaData
GerberFileReader::scan (tl::TextInputStream &stream)
{
  mp_stream = &stream;
  mp_layout = 0;
  mp_top_cell = 0;
  m_target_layers.clear ();

  GerberMetaData meta_data;

  try {
    meta_data = do_scan();
  } catch (tl::Exception &ex) {
    throw tl::Exception (ex.msg () + tl::to_string (tr (" in line ")) + tl::to_string (stream.line_number ()));
  }

  mp_stream = 0;

  return meta_data;
}

void
GerberFileReader::read (tl::TextInputStream &stream, db::Layout &layout, db::Cell &cell, const std::vector <unsigned int> &targets)
{
  GraphicsState state;
  state.global_trans = m_global_trans;
  swap_graphics_state (state);

  mp_stream = &stream;
  mp_layout = &layout;
  mp_top_cell = &cell;
  m_target_layers = targets;

  try {
    do_read ();
  } catch (tl::BreakException &) {
    throw;
  } catch (tl::Exception &ex) {
    throw tl::Exception (ex.msg () + tl::to_string (tr (" in line ")) + tl::to_string (stream.line_number ()));
  }

  flush ();

  mp_stream = 0;
  m_target_layers.clear ();
}

void 
GerberFileReader::set_format_string (const std::string &format)
{
  int l = -1, t = -1;
  bool tz = true;
  parse_format (format, l, t, tz);

  set_format (l, t, tz);
}

std::string 
GerberFileReader::format_string () const
{
  return format_to_string (m_digits_before, m_digits_after, m_omit_leading_zeroes);
}

void 
GerberFileReader::warn (const std::string &warning, int wl)
{
  if (m_warn_level < wl) {
    return;
  }

  tl::warn << warning << tl::to_string (tr (" in line ")) << mp_stream->line_number () << tl::to_string (tr (" (file ")) << mp_stream->source () << ")";
}

void 
GerberFileReader::error (const std::string &error)
{
  tl::error << error << tl::to_string (tr (" in line ")) << mp_stream->line_number () << tl::to_string (tr (" (file ")) << mp_stream->source () << ")";
}

void 
GerberFileReader::fatal (const std::string &error)
{
  throw tl::Exception (error);
}

double
GerberFileReader::accuracy() const
{
  return m_unit / pow (10.0, m_digits_after);
}

double 
GerberFileReader::read_coord (tl::Extractor &ex) 
{
  ex.skip ();
  int sign = 1;
  if (*ex == '+') {
    ++ex;
  } else if (*ex == '-') {
    sign = -1;
    ++ex;
  }

  double number = 0.0;
  int ndigits = 0;
  bool has_dot = false;
  while (! ex.at_end () && (isdigit (*ex) || *ex == '.')) {
    if (*ex == '.') {
      has_dot = true;
      ndigits = 0;
    } else {
      number = number * 10.0 + (*ex - '0');
      ++ndigits;
    }
    ++ex;
  }

  if (has_dot) {
    number /= pow (10.0, ndigits);
  } else if (m_omit_leading_zeroes) {
    if (m_digits_after < 0) {
      error (tl::to_string (tr ("Undefined number of digits - format missing")));
    }
    number /= pow (10.0, m_digits_after);
  } else {
    if (m_digits_before < 0) {
      error (tl::to_string (tr ("Undefined number of digits - format missing")));
    }
    number /= pow (10.0, ndigits - m_digits_before);
  }

  return number * m_unit * sign;
}

void
GerberFileReader::swap_graphics_state (GraphicsState &state)
{
  std::swap (m_inverse, state.inverse);
  std::swap (m_global_trans, state.global_trans);
  std::swap (m_s, state.m_s);
  std::swap (m_mx, state.m_mx);
  std::swap (m_my, state.m_my);
  std::swap (m_ox, state.m_ox);
  std::swap (m_oy, state.m_oy);
  std::swap (m_rot, state.m_rot);
  std::swap (m_os, state.m_os);
  std::swap (m_omx, state.m_omx);
  std::swap (m_omy, state.m_omy);
  std::swap (m_orot, state.m_orot);
  std::swap (m_lines, state.lines);
  std::swap (m_polygons, state.polygons);
  std::swap (m_clear_polygons, state.clear_polygons);
  std::swap (m_displacements, state.displacements);
}

void
GerberFileReader::push_state (const std::string &token)
{
  m_graphics_stack.push_back (GraphicsState ());
  swap_graphics_state (m_graphics_stack.back ());
  m_graphics_stack.back ().token = token;
}

std::string
GerberFileReader::pop_state ()
{
  std::string token;

  if (! m_graphics_stack.empty ()) {
    swap_graphics_state (m_graphics_stack.back ());
    token = m_graphics_stack.back ().token;
  }

  m_graphics_stack.pop_back ();
  return token;
}

bool
GerberFileReader::graphics_stack_empty () const
{
  return m_graphics_stack.empty ();
}

db::DCplxTrans
GerberFileReader::local_trans () const
{
  // TODO: is this order correct?
  db::DCplxTrans lt = db::DCplxTrans (m_s, m_rot, false, db::DVector (m_ox, m_oy));
  if (m_mx) {
    lt *= db::DCplxTrans (db::DTrans (db::FTrans::m0));
  }
  if (m_my) {
    lt *= db::DCplxTrans (db::DTrans (db::FTrans::m90));
  }

  return lt;
}

db::DCplxTrans
GerberFileReader::object_trans () const
{
  db::DCplxTrans ot = db::DCplxTrans (m_os, m_orot, false, db::DVector ());
  if (m_omx) {
    ot *= db::DCplxTrans (db::DTrans (db::FTrans::m0));
  }
  if (m_omy) {
    ot *= db::DCplxTrans (db::DTrans (db::FTrans::m90));
  }

  return ot;
}

void
GerberFileReader::produce_line (const db::DPath &p, bool clear)
{
  db::DCplxTrans t = global_trans () * db::DCplxTrans (1.0 / dbu ()) * local_trans ();

  //  Ignore clear paths for now - they cannot be subtracted from anything.
  //  Clear is just provided for completeness.
  if (clear) {
    return;
  }

  process_clear_polygons ();

  for (std::vector<db::DVector>::const_iterator d = m_displacements.begin (); d != m_displacements.end (); ++d) {
    m_lines.push_back (db::Path ());
    m_lines.back() = db::Path (p.transformed (t * db::DCplxTrans (*d)));
  }
}

void 
GerberFileReader::produce_polygon (const db::DPolygon &p, bool clear)
{
  db::DCplxTrans t = global_trans () * db::DCplxTrans (1.0 / dbu ()) * local_trans ();

  if (! clear) {
    process_clear_polygons ();
  }

  for (std::vector<db::DVector>::const_iterator d = m_displacements.begin (); d != m_displacements.end (); ++d) {
    db::Polygon *poly = 0;
    if (clear) {
      m_clear_polygons.push_back (db::Polygon ());
      poly = &m_clear_polygons.back ();
    } else {
      m_polygons.push_back (db::Polygon ());
      poly = &m_polygons.back ();
    }
    *poly = db::Polygon (p.transformed (t * db::DCplxTrans (*d)));
  }
}

void
GerberFileReader::process_clear_polygons ()
{
  if (! m_clear_polygons.empty ()) {
    std::vector<db::Polygon> input;
    m_polygons.swap (input);
    m_ep.boolean (input, m_clear_polygons, m_polygons, db::BooleanOp::ANotB, false, true);
    m_clear_polygons.clear ();
  }
}

void
GerberFileReader::collect (db::Region &region)
{
  process_clear_polygons ();

  if (m_merge) {
    std::vector<db::Polygon> merged_polygons;
    m_ep.merge (m_polygons, merged_polygons, 0, false /*don't resolve holes*/);
    m_polygons.swap (merged_polygons);
  }

  for (std::vector<db::Polygon>::const_iterator p = m_polygons.begin (); p != m_polygons.end (); ++p) {
    region.insert (*p);
  }
  for (std::vector<db::Path>::const_iterator p = m_lines.begin (); p != m_lines.end (); ++p) {
    region.insert (*p);
  }

  m_polygons.clear ();
  m_lines.clear ();
}

void
GerberFileReader::flush (const std::string &net_name)
{
  process_clear_polygons ();

  if (m_merge) {
    std::vector<db::Polygon> merged_polygons;
    m_ep.merge (m_polygons, merged_polygons, 0, false /*don't resolve holes*/); 
    m_polygons.swap (merged_polygons);
  }

  std::string nn = net_name;
  for (std::vector <unsigned int>::const_iterator t = m_target_layers.begin (); t != m_target_layers.end (); ++t) {

    db::Shapes &shapes = mp_top_cell->shapes (*t);

    for (std::vector<db::Polygon>::const_iterator p = m_polygons.begin (); p != m_polygons.end (); ++p) {

      shapes.insert (*p);

      if (! nn.empty () && p->hull ().begin () != p->hull ().end ()) {
        db::Point pt = *p->hull ().begin ();
        shapes.insert (db::Text (nn, db::Trans (pt - db::Point ())));
        nn.clear ();
      }

    }

    for (std::vector<db::Path>::const_iterator p = m_lines.begin (); p != m_lines.end (); ++p) {

      shapes.insert (*p);

      if (! nn.empty () && p->begin () != p->end ()) {
        db::Point pt = *p->begin ();
        shapes.insert (db::Text (nn, db::Trans (pt - db::Point ())));
        nn.clear ();
      }

    }

  }

  m_polygons.clear ();
  m_lines.clear ();
}

void 
GerberFileReader::progress_checkpoint () 
{
  if (mp_stream) {
    m_progress.set (mp_stream->raw_stream ().pos ());
  }
}

void  
GerberFileReader::step_and_repeat (const std::vector <db::DVector> &displacements)
{
  reset_step_and_repeat ();
  if (! displacements.empty ()) {
    m_displacements = displacements;
  }
}

void  
GerberFileReader::reset_step_and_repeat ()
{
  m_displacements.clear ();
  m_displacements.push_back (db::DVector ());
}
  
// ---------------------------------------------------------------------------------------
//  Implementation of GerberFile

GerberFile::GerberFile ()
  : m_circle_points (-1), m_merge_mode (-1), m_digits_before (-1), m_digits_after (-1), m_omit_leading_zeroes (true)
{
  // .. nothing yet ..
}

void 
GerberFile::set_format_string (const std::string &format)
{
  int l = -1, t = -1;
  bool tz = true;
  parse_format (format, l, t, tz);

  set_format (l, t, tz);
}

std::string 
GerberFile::format_string () const
{
  return format_to_string (digits_before (), digits_after (), omit_leading_zeroes ());
}

void 
GerberFile::set_layers_string (const std::string &layers)
{
  tl::Extractor ex (layers.c_str ());

  while (! ex.at_end ()) {
    db::LayerProperties lp;
    lp.read (ex);
    add_layer_spec (lp);
    ex.test (",");
  }
}

std::string 
GerberFile::layers_string () const
{
  std::string r;

  for (std::vector<db::LayerProperties>::const_iterator ls = m_layer_specs.begin (); ls != m_layer_specs.end (); ++ls) {
    if (! r.empty ()) {
      r += ",";
    }
    r += ls->to_string ();
  }

  return r;
}

// ---------------------------------------------------------------------------------------
//  Implementation of GerberImporter

//  TODO: generalize this:
std::vector <tl::shared_ptr<db::GerberFileReader> > get_readers (int warn_level)
{
  std::vector <tl::shared_ptr<db::GerberFileReader> > readers;
  readers.push_back (new db::GerberDrillFileReader (warn_level));
  readers.push_back (new db::RS274XReader (warn_level));
  return readers;
}

GerberImporter::GerberImporter (int warn_level)
  : m_cell_name ("PCB"), m_dbu (0.001), m_merge (false), 
    m_invert_negative_layers (false), m_border (5000), 
    m_circle_points (64), m_warn_level (warn_level)
{
  // .. nothing yet ..
}

void 
GerberImporter::load_project (const std::string &fn)
{
  //  use the file's absolute path as the base directory
  m_dir = tl::absolute_file_path (fn);

  tl::InputStream stream (fn);
  tl::TextInputStream text_stream (stream);

  load_project (text_stream);
}

void 
GerberImporter::load_project (tl::TextInputStream &stream)
{
  try {
    do_load_project (stream);
  } catch (tl::Exception &ex) {
    throw tl::Exception (ex.msg () + tl::to_string (tr (" in line ")) + tl::to_string (stream.line_number ()));
  }
}

GerberMetaData
GerberImporter::scan (const std::string &fn)
{
  tl::InputStream stream (fn);
  tl::TextInputStream text_stream (stream);

  return scan (text_stream);
}

GerberMetaData
GerberImporter::scan (tl::TextInputStream &stream)
{
  try {

    std::vector <tl::shared_ptr<db::GerberFileReader> > readers = get_readers (0);

    //  determine the reader to use:
    for (std::vector <tl::shared_ptr<db::GerberFileReader> >::iterator r = readers.begin (); r != readers.end (); ++r) {
      stream.reset ();
      if ((*r)->accepts (stream)) {
        return (*r)->scan (stream);
      }
    }

  } catch (tl::Exception &ex) {
    tl::warn << ex.msg ();
  }

  return GerberMetaData ();
}

static void read_ref_point_spec (tl::Extractor &l, std::vector<std::pair<db::DBox, db::DBox> > &ref_points, size_t n, bool pcb)
{
  while (ref_points.size () < n + 1) {
    ref_points.push_back (std::pair<db::DBox, db::DBox> ());
  }

  l.expect ("=");
  double x, y;
  l.read (x);
  l.test (",");
  l.read (y);

  if (pcb) {
    ref_points[n].first = db::DBox (x, y, x, y);
  } else {
    ref_points[n].second = db::DBox (x, y, x, y);
  }
}

void 
GerberImporter::do_load_project (tl::TextInputStream &stream)
{
  std::vector<std::pair<db::DBox, db::DBox> > ref_points; // Reference points PCB/Layout: boxes can be "empty" ..

  while (! stream.at_end ()) {

    std::string lstr = stream.get_line ();
    tl::Extractor l (lstr.c_str ());

    //  skip empty or pure comment lines
    if (l.at_end () || l.test ("#")) {
      continue;
    }

    if (l.test ("dbu")) {

      l.expect ("=");
      l.read (m_dbu);

      if (m_dbu < 1e-6) {
        throw tl::Exception (tl::to_string (tr ("Invalid database unit %g")), m_dbu);
      }

    } else if (l.test ("cell-name")) {

      l.expect ("=");
      l.read_word_or_quoted (m_cell_name);

    } else if (l.test ("layer-styles")) {

      l.expect ("=");
      l.read_word_or_quoted (m_layer_styles, "%!.:/\\+-,=_$");

    } else if (l.test ("circle-points")) {

      l.expect ("=");
      l.read (m_circle_points);

      if (m_circle_points < 4) {
        throw tl::Exception (tl::to_string (tr ("Invalid number of points for full circle (%d)")), m_circle_points);
      }

    } else if (l.test ("keep-path")) {

      //  ignored currently, kept for compatibility with prototype
      l.expect ("=");
      int d; l.read (d);

    } else if (l.test ("border")) {

      l.expect ("=");
      double d; 
      l.read (d);
      m_border = d;

    } else if (l.test ("invert-negative-layers")) {

      l.expect ("=");
      int d; 
      l.read (d);
      m_invert_negative_layers = d;

    } else if (l.test ("merge")) {

      l.expect ("=");
      int d; 
      l.read (d);
      m_merge = d;

    //  provided for compatibility with prototype, use ref-point instead
    } else if (l.test ("p1-pcb")) {
      read_ref_point_spec (l, ref_points, 0, true);
    } else if (l.test ("p2-pcb")) {
      read_ref_point_spec (l, ref_points, 1, true);
    } else if (l.test ("p3-pcb")) {
      read_ref_point_spec (l, ref_points, 2, true);
    } else if (l.test ("p1-gds")) {
      read_ref_point_spec (l, ref_points, 0, false);
    } else if (l.test ("p2-gds")) {
      read_ref_point_spec (l, ref_points, 1, false);
    } else if (l.test ("p3-gds")) {
      read_ref_point_spec (l, ref_points, 2, false);
    } else if (l.test ("ref-point")) {

      l.expect ("=");

      l.expect ("(");
      double x_pcb, y_pcb;
      l.read (x_pcb);
      l.expect (",");
      l.read (y_pcb);
      l.expect (")");

      l.expect (",");

      l.expect ("(");
      double x_ly, y_ly;
      l.read (x_ly);
      l.expect (",");
      l.read (y_ly);
      l.expect (")");

      ref_points.push_back (std::make_pair (db::DBox (x_pcb, y_pcb, x_pcb, y_pcb), db::DBox (x_ly, y_ly, x_ly, y_ly)));

    } else if (l.test ("transformation")) {

      l.expect ("=");
      db::DCplxTrans gt;
      l.read (gt);
      m_global_trans = gt * m_global_trans;

    } else if (l.test ("mirror")) {

      l.expect ("=");
      int d; 
      l.read (d);
      if (d != 0) {
        m_global_trans = db::DCplxTrans (db::DFTrans::m0) * m_global_trans;
      }

    } else if (l.test ("dir")) {

      l.expect ("=");
      l.read_word_or_quoted (m_dir, "%!.:/\\+-,=_$");

    } else if (l.test ("format")) {

      l.expect ("=");
      l.read_word_or_quoted (m_format_string, "*:");

      //  check the string ...
      int l = 0, t = 0;
      bool tz = false;
      parse_format (m_format_string, l, t, tz);

    } else {

      l.test ("file");

      db::GerberFile file;

      std::string fn;
      l.read_word_or_quoted (fn, "%!.:/\\+-,=_$");
      file.set_filename (fn);

      while (! l.at_end () && ! l.test ("#")) {

        if (l.test ("circle-points")) {

          l.expect ("=");

          int cp = -1;
          l.read (cp);

          if (cp < 4) {
            throw tl::Exception (tl::to_string (tr ("Invalid number of points for full circle (%d)")), m_circle_points);
          }

          file.set_circle_points (cp);

        } else if (l.test ("format")) {

          l.expect ("=");

          std::string f;
          l.read_word_or_quoted (f, "*:");

          file.set_format_string (f);

        } else if (l.test ("merge")) {

          l.expect ("=");

          int m = 0;
          l.read (m);

          file.set_merge_mode (m != 0 ? 1 : 0);

        } else {

          db::LayerProperties lp;
          lp.read (l);
          file.add_layer_spec (lp);

        }

      }

      add_file (file);

    }

    if (! l.test ("#")) {
      l.expect_end ();
    }

  }

  //  transfer the reference points
  m_reference_points.clear ();

  if (ref_points.size () > 3) {
    throw tl::Exception (tl::to_string (tr ("Not more than three reference points can be specified")));
  }

  for (unsigned int i = 0; i < ref_points.size (); ++i) {
    if (ref_points [i].first.empty () || ref_points [i].second.empty ()) {
      throw tl::Exception (tl::to_string (tr ("Reference point #%d is not fully specified (either PCB or layout coordinate is missing)")), int (i + 1));
    }
    m_reference_points.push_back (std::make_pair (ref_points [i].first.center (), ref_points [i].second.center ()));
  }
}

void 
GerberImporter::save_project (std::ostream &stream)
{
  stream << "# Gerber PCB import project" << std::endl;
  stream << "# Created by KLayout" << std::endl;
  stream << "dir=" << tl::to_quoted_string (m_dir) << std::endl;
  stream << "cell=" << tl::to_quoted_string (m_cell_name) << std::endl;
  stream << "dbu=" << tl::to_string (m_dbu) << std::endl;
  stream << "circle-points=" << tl::to_string (m_circle_points) << std::endl;
  stream << "transformation=" << tl::to_quoted_string (m_global_trans.to_string ()) << std::endl;
  for (std::vector <std::pair<db::DPoint, db::DPoint> >::const_iterator r = m_reference_points.begin (); r != m_reference_points.end (); ++r) {
    stream << "ref-point=(" << tl::to_string (r->first.x ()) << "," << tl::to_string (r->first.y ()) << "),(" << tl::to_string (r->second.x ()) << "," << tl::to_string (r->second.y ()) << ")" << std::endl;
  }
  stream << "merge=" << (m_merge ? 1 : 0) << std::endl;
  stream << "invert-negative-layers=" << (m_invert_negative_layers ? 1 : 0) << std::endl;
  stream << "border=" << tl::to_string (m_border) << std::endl;
  if (! m_layer_styles.empty ()) {
    stream << "layer-styles=" << tl::to_quoted_string (m_layer_styles) << std::endl;
  }

  for (std::vector<db::GerberFile>::iterator file = m_files.begin (); file != m_files.end (); ++file) {

    stream << "file " << tl::to_quoted_string (file->filename ());
    for (std::vector <db::LayerProperties>::const_iterator ls = file->layer_specs ().begin (); ls != file->layer_specs ().end (); ++ls) {
      stream << " " << tl::to_quoted_string (ls->to_string ());
    }

    if (file->circle_points () > 0) {
      stream << " circle-points=" << tl::to_string (file->circle_points ());
    }
    if (file->has_format ()) {
      stream << " format=" << tl::to_quoted_string (file->format_string ());
    }
    if (file->merge_mode () >= 0) {
      stream << " merge=" << tl::to_string (file->merge_mode ());
    }

    stream << std::endl;

  }
}

void 
GerberImporter::read (db::Layout &layout, db::cell_index_type cell_index)
{
  m_cell_name = layout.cell_name (cell_index);
  m_dbu = layout.dbu ();

  do_read (layout, cell_index);
}

db::cell_index_type 
GerberImporter::read (db::Layout &layout)
{
  db::cell_index_type ci = layout.add_cell (m_cell_name.c_str ());
  layout.dbu (m_dbu);

  do_read (layout, ci);

  return ci;
}

void 
GerberImporter::do_read (db::Layout &layout, db::cell_index_type cell_index)
{
  tl::log << tl::to_string (tr ("Importing PCB data"));

  std::set<unsigned int> inverse_layers;

  {
    tl::RelativeProgress progress (tl::to_string (tr ("Importing PCB data")), m_files.size (), 1);

    //  derive the actual global transformation from the reference points
    db::DCplxTrans global_trans (m_global_trans);
    if (! m_reference_points.empty ()) {

      db::DPoint p1_pcb = m_reference_points[0].first;
      db::DPoint p1_ly = m_reference_points[0].second;

      if (m_reference_points.size () > 1) {

        db::DPoint p2_pcb = m_reference_points[1].first;
        db::DPoint p2_ly = m_reference_points[1].second;

        db::DVector d12_pcb = (p2_pcb - p1_pcb) * (1.0 / p2_pcb.distance (p1_pcb));
        db::DVector d12_ly = (p2_ly - p1_ly) * (1.0 / p2_ly.distance (p1_ly));

        int ru = -1;
        int rm = -1;
        for (int f = 0; f < 8; ++f) {
          db::DVector d12 = db::DTrans (f) * d12_pcb;
          if ((d12 - d12_ly).length () < 0.1) {
            if (f < 4) {
              ru = f;
            } else {
              rm = f;
            }
          }
        }

        if (ru < 0 || rm < 0) {
          throw tl::Exception (tl::to_string (tr ("Unable to deduce rotation from reference points p1 and p2 (PCB and layout)")));
        }

        if (m_reference_points.size () > 2) {

          db::DPoint p3_pcb = m_reference_points[2].first;
          db::DPoint p3_ly = m_reference_points[2].second;

          db::DVector d13_pcb = (p3_pcb - p1_pcb) * (1.0 / p3_pcb.distance (p1_pcb));
          db::DVector d13_ly = (p3_ly - p1_ly) * (1.0 / p3_ly.distance (p1_ly));

          double vp_pcb = d13_pcb.x () * d12_pcb.y () - d13_pcb.y () * d12_pcb.x ();
          double vp_gds = d13_ly.x () * d12_ly.y () - d13_ly.y () * d12_ly.x ();

          if (vp_pcb * vp_gds < 0.0) {
            global_trans = db::DCplxTrans (db::DFTrans (rm));
          } else {
            global_trans = db::DCplxTrans (db::DFTrans (ru));
          }

        } else {

          if (global_trans.is_mirror ()) {
            global_trans = db::DCplxTrans (db::DFTrans (rm));
          } else {
            global_trans = db::DCplxTrans (db::DFTrans (ru));
          }

        }

      }

      global_trans = db::DCplxTrans (p1_ly - (db::DPoint () + global_trans.disp ())) * global_trans * db::DCplxTrans (db::DPoint () - p1_pcb);

    }

    std::string format (m_format_string);

    for (std::vector<db::GerberFile>::iterator file = m_files.begin (); file != m_files.end (); ++file) {

      ++progress;

      std::vector <unsigned int> targets;

      for (std::vector <db::LayerProperties>::const_iterator ls = file->layer_specs ().begin (); ls != file->layer_specs ().end (); ++ls) {

        int layer_index = -1;
        for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
          if (layout.get_properties ((*l).first).log_equal (*ls)) {
            layer_index = int ((*l).first);
            break;
          }
        }

        if (layer_index < 0) {
          layer_index = int (layout.insert_layer (*ls));
        }

        targets.push_back (layer_index);

      }

      std::string fp = tl::combine_path (tl::absolute_file_path (m_dir), file->filename ());
      tl::InputStream input_file (fp);
      tl::TextInputStream stream (input_file);

      std::vector <tl::shared_ptr<db::GerberFileReader> > readers = get_readers (m_warn_level);

      //  determine the reader to use:
      db::GerberFileReader *reader = 0;
      for (std::vector <tl::shared_ptr<db::GerberFileReader> >::iterator r = readers.begin (); r != readers.end (); ++r) {
        stream.reset ();
        if ((*r)->accepts (stream)) {
          reader = r->operator-> ();
          break;
        }
      }

      if (! reader) {
        throw tl::Exception (tl::to_string (tr ("Unable to determine format for file '%s'")), fp);
      }
      
      stream.reset ();

      //  set up the reader
      reader->set_dbu (m_dbu);
      reader->set_global_trans (db::DCplxTrans (1.0 / m_dbu) * global_trans * db::DCplxTrans (m_dbu));
      reader->set_format_string (file->format_string ());
      if (! reader->has_format ()) {
        reader->set_format_string (format);
      }
      reader->set_merge (file->merge_mode () >= 0 ? (file->merge_mode () != 0) : m_merge);
      reader->set_circle_points (file->circle_points () >= 0 ? file->circle_points () : m_circle_points);

      //  actually read
      try {
        tl::log << "Reading PCB file '" << file->filename () << "' with format '" << file->format_string () << "'";
        reader->read (stream, layout, layout.cell (cell_index), targets);
      } catch (tl::BreakException &) {
        throw;
      } catch (tl::Exception &ex) {
        throw tl::Exception (ex.msg () + ", reading file " + file->filename ());
      }

      //  use the current format as further default
      format = reader->format_string ();

      if (reader->is_inverse ()) {
        inverse_layers.insert (targets.begin (), targets.end ());
      }

    }

  }

  //  Invert the negative layers if requested
  if (m_invert_negative_layers && ! inverse_layers.empty ()) {

    db::Cell &cell = layout.cell (cell_index);

    double e = m_border / layout.dbu ();
    db::Box bbox = cell.bbox ().enlarged (db::Vector (db::DVector (e, e)));
   
    int bbox_layer = layout.insert_layer ();
    cell.shapes (bbox_layer).insert (bbox);
    
    db::ShapeProcessor sp;

    int n = 0;
    for (std::set<unsigned int>::const_iterator l = inverse_layers.begin (); l != inverse_layers.end (); ++l) {

      tl::log << "Inverting layer " << layout.get_properties (*l).to_string ();

      sp.enable_progress (tl::to_string (tr ("Inverting layer")) + " " + tl::to_string (n + 1) + " " + tl::to_string (tr ("of")) + " " + tl::to_string (inverse_layers.size ()));
      sp.boolean (layout, cell, *l, layout, cell, bbox_layer, cell.shapes (*l), db::BooleanOp::BNotA, true);

      //  clear the result layer for all called cells (if there are some)
      std::set<db::cell_index_type> called_cells;
      cell.collect_called_cells (called_cells);
      for (std::set<db::cell_index_type>::const_iterator c = called_cells.begin (); c != called_cells.end (); ++c) {
        layout.cell (*c).shapes (*l).clear ();
      }

    }

    layout.delete_layer (bbox_layer);

  }

}

// ---------------------------------------------------------------
//  Plugin for the stream reader


class GerberReader
  : public db::ReaderBase
{
public:

  GerberReader (tl::InputStream &s)
    : m_stream (s)
  {
    //  .. nothing yet ..
  }

  ~GerberReader ()
  {
    //  .. nothing yet ..
  }

  virtual const db::LayerMap &read (db::Layout &layout)
  {
    return read (layout, db::LoadLayoutOptions ());
  }

  virtual const db::LayerMap &read (db::Layout &layout, const db::LoadLayoutOptions &options)
  {
    init (options);

    //  TODO: too simple, should provide at least a layer filtering.
    db::GerberImportData data;

    std::string fn (m_stream.source ());
    if (! fn.empty ()) {
      data.base_dir = tl::absolute_path (fn);
    }

    data.load (m_stream);

    db::GerberImporter importer (warn_level ());
    data.setup_importer (&importer);

    importer.read (layout);

    std::string lyr_file = data.get_layer_properties_file ();
    if (! lyr_file.empty ()) {
      layout.add_meta_info ("layer-properties-file", db::MetaInfo ("Layer Properties File", lyr_file));
    }

    return m_layers;
  }

  virtual const char *format () const
  {
    return "GerberPCB";
  }

private:
  tl::InputStream &m_stream;
  db::LayerMap m_layers;
};

class GerberFormatDeclaration
  : public db::StreamFormatDeclaration
{
  virtual std::string format_name () const { return "GerberPCB"; }
  virtual std::string format_desc () const { return "Gerber PCB"; }
  virtual std::string format_title () const { return "Gerber PCB (project files)"; }
  virtual std::string file_format () const { return "Gerber PCB project files (*.pcb *.PCB)"; }

  virtual bool detect (tl::InputStream &stream) const
  {
    //  The test is that somewhere within the first 1000 bytes, a <pcb-project> XML tag appears.
    //  1000 bytes are within the initial block that the stream reader reads and hence
    //  this does not trigger any reread which is not available on some sources.
    //  TODO: this is pretty simple test. A more elaborate test would be in place here.
    std::string h = stream.read_all (1000);
    //  HINT: this assumes UTF8 or ISO encoding ...
    if (h.find ("<pcb-project>") != std::string::npos) {
      return true;
    } else {
      return false;
    }
  }

  virtual db::ReaderBase *create_reader (tl::InputStream &s) const
  {
    return new GerberReader (s);
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

static tl::RegisteredClass<db::StreamFormatDeclaration> format_decl (new GerberFormatDeclaration (), 1000, "GerberPCB");

}

