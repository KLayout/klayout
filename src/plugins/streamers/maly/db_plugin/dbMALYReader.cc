
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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



#include "dbMALYReader.h"
#include "dbStream.h"
#include "dbObjectWithProperties.h"
#include "dbArray.h"
#include "dbStatic.h"
#include "dbShapeProcessor.h"
#include "dbTechnology.h"

#include "tlException.h"
#include "tlString.h"
#include "tlClassRegistry.h"
#include "tlFileUtils.h"
#include "tlUri.h"

#include <cctype>
#include <string>

namespace db
{

// ---------------------------------------------------------------
//  Some helper structures to collect data

struct MALYReaderTitleSpec
{
  MALYReaderTitleSpec ()
    : enabled (false), width (1.0), height (1.0), pitch (1.0)
  { }

  bool enabled;
  db::DTrans trans;
  double width, height, pitch;
};

struct MALYReaderTitleData
{
  MALYReaderTitleData ()
  { }

  MALYReaderTitleSpec date_spec;
  MALYReaderTitleSpec serial_spec;
  std::list<std::pair<std::string, MALYReaderTitleSpec> > string_titles;
};

struct MALYReaderParametersData
{
  MALYReaderParametersData ()
    : base (Center), array_base (Center), masksize (0.0), maskmirror (false), font (MALYTitle::Standard)
  { }

  enum Base
  {
    Origin,
    Center,
    LowerLeft
  };

  Base base;
  Base array_base;
  double masksize;
  bool maskmirror;
  MALYTitle::Font font;
  std::list<std::pair<std::string, std::string> > roots;
};

struct MALYReaderStrRefData
{
  MALYReaderStrRefData ()
    : layer (-1), scale (1.0), nx (1), ny (1), dx (0.0), dy (0.0)
  { }

  std::string file;
  std::string name;
  int layer;
  db::DVector org;
  db::DBox size;
  double scale;
  int nx, ny;
  double dx, dy;
};

struct MALYReaderStrGroupData
{
  std::string name;
  std::list<MALYReaderStrRefData> refs;
};

struct MALYReaderMaskData
{
  MALYReaderParametersData parameters;
  MALYReaderTitleData title;
  std::list<MALYReaderStrGroupData> strgroups;
};


// ---------------------------------------------------------------
//  MALYReader

MALYReader::MALYReader (tl::InputStream &s)
  : m_stream (s),
    m_progress (tl::to_string (tr ("Reading MALY file")), 1000),
    m_dbu (0.001)
{
  m_progress.set_format (tl::to_string (tr ("%.0fk lines")));
  m_progress.set_format_unit (1000.0);
  m_progress.set_unit (100000.0);
}

MALYReader::~MALYReader ()
{
  //  .. nothing yet ..
}

bool
MALYReader::test ()
{
  try {

    tl::Extractor ex = read_record ();
    return ex.test ("BEGIN") && ex.test ("MALY");

  } catch (...) {
    return false;
  }
}

const LayerMap &
MALYReader::read (db::Layout &layout)
{
  return read (layout, db::LoadLayoutOptions ());
}

const LayerMap &
MALYReader::read (db::Layout &layout, const db::LoadLayoutOptions &options)
{
  init (options);

  prepare_layers (layout);

  const db::MALYReaderOptions &specific_options = options.get_options<db::MALYReaderOptions> ();
  m_dbu = specific_options.dbu;

  set_layer_map (specific_options.layer_map);
  set_create_layers (specific_options.create_other_layers);
  // @@@ set_keep_layer_names (specific_options.keep_layer_names);
  set_keep_layer_names (true);

  MALYData data = read_maly_file ();

  // @@@

  finish_layers (layout);
  return layer_map_out ();
}

void
MALYReader::unget_record ()
{
  m_record_returned = m_record;
}

tl::Extractor
MALYReader::read_record ()
{
  if (! m_record_returned.empty ()) {
    m_record = m_record_returned;
    return tl::Extractor (m_record.c_str ());
  }

  while (! m_stream.at_end ()) {
    m_record = read_record_internal ();
    tl::Extractor ex (m_record.c_str ());
    if (ex.test ("+")) {
      error (tl::to_string (tr ("'+' character past first column - did you mean to continue a line?")));
    } else if (! ex.at_end ()) {
      return ex;
    }
  }

  return tl::Extractor ();
}

std::string
MALYReader::read_record_internal ()
{
  std::string rec;

  while (! m_stream.at_end ()) {

    char c = m_stream.get_char ();

    //  skip comments
    if (c == '/') {
      char cc = m_stream.peek_char ();
      if (cc == '/') {
        while (! m_stream.at_end () && (c = m_stream.get_char ()) != '\n')
          ;
        if (m_stream.at_end ()) {
          break;
        }
      } else if (cc == '*') {
        m_stream.get_char ();  //  eat leading "*"
        while (! m_stream.at_end () && (m_stream.get_char () != '*' || m_stream.peek_char () != '/'))
          ;
        if (m_stream.at_end ()) {
          error (tl::to_string (tr ("/*...*/ comment not closed")));
        }
        m_stream.get_char ();  //  eat trailing "/"
        if (m_stream.at_end ()) {
          break;
        }
        c = m_stream.get_char ();
      }
    }

    if (c == '\n') {
      if (m_stream.peek_char () == '+') {
        //  continuation line
        m_stream.get_char ();  //  eat "+"
        if (m_stream.at_end ()) {
          break;
        }
        c = m_stream.get_char ();
      } else {
        break;
      }
    }

    if (c == '"' || c == '\'') {

      rec += c;

      //  skip quoted string
      char quote = c;
      while (! m_stream.at_end ()) {
        c = m_stream.get_char ();
        rec += c;
        if (c == quote) {
          quote = 0;
          break;
        } else if (c == '\\') {
          if (m_stream.at_end ()) {
            error (tl::to_string (tr ("Unexpected end of file inside quotee string")));
          }
          c = m_stream.get_char ();
          rec += c;
        } else if (c == '\n') {
          error (tl::to_string (tr ("Line break inside quoted string")));
        }
      }

      if (quote) {
        error (tl::to_string (tr ("Unexpected end of file inside quotee string")));
      }

    } else {
      rec += c;
    }

  }

  return rec;
}

MALYData
MALYReader::read_maly_file ()
{
  MALYData data;
  try {
    do_read_maly_file (data);
  } catch (tl::Exception &ex) {
    error (ex.msg ());
  }
  return data;
}

void
MALYReader::extract_title_trans (tl::Extractor &ex, MALYReaderTitleSpec &spec)
{
  double x = 0.0, y = 0.0;
  bool ymirror = false;
  int rot = 0;

  ex.read (x);
  ex.read (y);

  if (ex.test ("SIZE")) {
    ex.read (spec.width);
    ex.read (spec.height);
    ex.read (spec.pitch);
  }

  if (ex.test ("MIRROR")) {
    if (ex.test ("Y")) {
      ymirror = true;
    } else if (ex.test ("OFF")) { // @@@
      ymirror = false;
    } else {
      error (tl::to_string (tr ("Expected 'Y' or 'OFF' for MIRROR spec")));
    }
  }

  if (ex.test ("ROTATE")) {
    unsigned int a = 0;
    ex.read (a);
    rot = (a / 90) % 4;
  }

  spec.trans = db::DTrans (rot, ymirror, db::DVector (x, y));
}

static
MALYReaderParametersData::Base string_to_base (const std::string &string)
{
  if (string == "ORIGIN") {
    return MALYReaderParametersData::Origin;
  } else if (string == "LOWERLEFT") { // @@@?
    return MALYReaderParametersData::LowerLeft;
  } else if (string == "CENTER") {
    return MALYReaderParametersData::Center;
  } else {
    // @@@ error
    return MALYReaderParametersData::Center;
  }
}

bool
MALYReader::begin_section (tl::Extractor &ex, const std::string &name)
{
  tl::Extractor ex_saved = ex;

  if (ex.test ("BEGIN")) {
    if (name.empty ()) {
      m_sections.push_back (std::string ());
      ex.read_word (m_sections.back ());
      return true;
    } else if (ex.test (name.c_str ())) {
      m_sections.push_back (name);
      return true;
    }
  }

  ex = ex_saved;
  return false;
}

bool
MALYReader::end_section (tl::Extractor &ex)
{
  tl_assert (! m_sections.empty ());
  if (ex.at_end ()) {

    error (tl::to_string (tr ("Unexpected end of file during section")));
    return false;

  } else if (ex.test ("END")) {

    ex.expect (m_sections.back ().c_str ());
    m_sections.pop_back ();
    return true;

  } else {

    return false;

  }
}

void
MALYReader::skip_section ()
{
  while (true) {
    tl::Extractor ex = read_record ();
    if (begin_section (ex)) {
      skip_section ();
    } else if (end_section (ex)) {
      break;
    }
  }
}

void
MALYReader::read_parameter (MALYReaderParametersData &data)
{
  while (true) {

    tl::Extractor ex = read_record ();

    if (end_section (ex)) {
      break;
    } else if (ex.test ("MASKMIRROR")) {

      if (ex.test ("NONE")) {
        data.maskmirror = false;
      } else if (ex.test ("Y")) {
        data.maskmirror = true;
      } else {
        error (tl::to_string (tr ("Expected value Y or NONE for MASKMIRROR")));
      }

    } else if (ex.test ("MASKSIZE")) {

      data.masksize = 0.0;
      ex.read (data.masksize);

    } else if (ex.test ("FONT")) {

      if (ex.test ("STANDARD")) {
        data.font = MALYTitle::Standard;
      } else if (ex.test ("NATIVE")) {
        data.font = MALYTitle::Native;
      } else {
        error (tl::to_string (tr ("Expected value STANDARD or NATIVE for FONT")));
      }

    } else if (ex.test ("BASE")) {

      std::string base;
      ex.read_word (base);
      data.base = string_to_base (base);

    } else if (ex.test ("ARYBASE")) {

      std::string base;
      ex.read_word (base);
      data.array_base = string_to_base (base);

    } else if (ex.test ("REFERENCE")) {

      ex.expect ("TOOL");

      std::string para;
      ex.read_word_or_quoted (para);
      //  @@@ TODO: what to do with "para"

      ex.expect_end ();

    } else if (ex.test ("ROOT")) {

      std::string format, path;
      ex.read_word_or_quoted (format);
      ex.read_word_or_quoted (path, ".\\/+-");
      ex.expect_end ();

      data.roots.push_back (std::make_pair (format, path));

    } else {
      warn (tl::to_string (tr ("Unknown record ignored")));
    }

  }
}

void
MALYReader::read_title (MALYReaderTitleData &data)
{
  while (true) {

    tl::Extractor ex = read_record ();

    if (end_section (ex)) {
      break;
    } else if (ex.test ("DATE")) {

      if (ex.test ("OFF")) {
        data.date_spec.enabled = false;
      } else {
        data.date_spec.enabled = true;
        extract_title_trans (ex, data.date_spec);
        ex.expect_end ();
      }

    } else if (ex.test ("SERIAL")) {

      if (ex.test ("OFF")) {
        data.serial_spec.enabled = false;
      } else {
        data.serial_spec.enabled = true;
        extract_title_trans (ex, data.serial_spec);
        ex.expect_end ();
      }

    } else if (ex.test ("STRING")) {

      std::string text;
      ex.read_word_or_quoted (text);

      data.string_titles.push_back (std::make_pair (text, MALYReaderTitleSpec ()));
      data.string_titles.back ().second.enabled = true;
      extract_title_trans (ex, data.string_titles.back ().second);

      ex.expect_end ();

    } else {
      warn (tl::to_string (tr ("Unknown record ignored")));
    }

  }
}

void
MALYReader::read_strgroup (MALYReaderStrGroupData &data)
{
  while (true) {

    bool is_sref = false;

    tl::Extractor ex = read_record ();
    if (end_section (ex)) {
      break;
    } else if ((is_sref = ex.test ("SREF")) || ex.test ("AREF")) {

      data.refs.push_back (MALYReaderStrRefData ());
      MALYReaderStrRefData &ref = data.refs.back ();

      ex.read_word_or_quoted (ref.file);
      ex.read_word_or_quoted (ref.name);
      ex.read (ref.layer);

      if (ex.test ("ORG")) {
        double x = 0.0, y = 0.0;
        ex.read (x);
        ex.read (y);
        ref.org = db::DVector (x, y);
      }

      if (ex.test ("SIZE")) {
        double l = 0.0, b = 0.0, r = 0.0, t = 0.0;
        ex.read (l);
        ex.read (b);
        ex.read (r);
        ex.read (t);
        ref.size = db::DBox (l, b, r, t);
      }

      if (ex.test ("SCALE")) {
        ex.read (ref.scale);
      }

      if (! is_sref && ex.test ("ITERATION")) {
        ex.read (ref.nx);
        ex.read (ref.ny);
        ex.read (ref.dx);
        ex.read (ref.dy);
      }

      ex.expect_end ();

    } else {
      warn (tl::to_string (tr ("Unknown record ignored")));
    }

  }
}

void
MALYReader::read_mask (MALYReaderMaskData &mask)
{
  while (true) {

    tl::Extractor ex = read_record ();
    if (end_section (ex)) {
      break;
    } else if (begin_section (ex, "PARAMETER")) {

      ex.expect_end ();
      read_parameter (mask.parameters);

    } else if (begin_section (ex, "TITLE")) {

      ex.expect_end ();
      read_title (mask.title);

    } else if (begin_section (ex, "STRGROUP")) {

      mask.strgroups.push_back (MALYReaderStrGroupData ());

      ex.read_word_or_quoted (mask.strgroups.back ().name);
      ex.expect_end ();

      read_strgroup (mask.strgroups.back ());

    } else if (begin_section (ex)) {
      skip_section ();
    } else {
      warn (tl::to_string (tr ("Unknown record ignored")));
    }

  }
}

bool
MALYReader::read_maskset (MALYData &data)
{
  tl::Extractor ex = read_record ();

  if (! begin_section (ex, "MASKSET")) {
    unget_record ();
    return false;
  }

  MALYReaderMaskData cmask;
  std::list<MALYReaderMaskData> masks;

  while (true) {

    ex = read_record ();

    if (end_section (ex)) {

      ex.expect_end ();
      // @@@ create_masks (cmask, masks, data);
      return true;

    } else if (begin_section (ex, "MASK")) {

      ex.expect_end ();
      masks.push_back (MALYReaderMaskData ());
      read_mask (masks.back ());

    } else if (begin_section (ex, "CMASK")) {

      ex.expect_end ();
      read_mask (cmask);

    } else {
      warn (tl::to_string (tr ("Unknown record ignored")));
    }

  }
}

void
MALYReader::do_read_maly_file (MALYData &data)
{
  tl::Extractor ex = read_record ();
  if (! begin_section (ex, "MALY")) {
    error (tl::to_string (tr ("Header expected ('BEGIN MALY')")));
  }

  std::string version;
  ex.read (version, ".");
  //  @@@ TODO: what to do with version string?

  ex.expect_end ();

  while (read_maskset (data))
    ;

  ex = read_record ();
  if (! end_section (ex)) {
    error (tl::to_string (tr ("Terminator expected ('END MALY')")));
  }

  ex = read_record ();
  if (! ex.at_end ()) {
    error (tl::to_string (tr ("Records found past end of file")));
  }
}

void 
MALYReader::error (const std::string &msg)
{
  throw MALYReaderException (msg, m_stream.line_number (), m_stream.source ());
}

void 
MALYReader::warn (const std::string &msg, int wl)
{
  if (warn_level () < wl) {
    return;
  }

  if (first_warning ()) {
    tl::warn << tl::sprintf (tl::to_string (tr ("In file %s:")), m_stream.source ());
  }

  int ws = compress_warning (msg);
  if (ws < 0) {
    tl::warn << msg
             << tl::to_string (tr (" (line=")) << m_stream.line_number ()
             << tl::to_string (tr (", file=")) << m_stream.source ()
             << ")";
  } else if (ws == 0) {
    tl::warn << tl::to_string (tr ("... further warnings of this kind are not shown"));
  }
}

std::string
MALYReader::resolve_path (const std::string &path)
{
  tl::URI path_uri (path);

  if (tl::is_absolute (path_uri.path ())) {

    return path_uri.to_string ();

  } else {

    tl::URI source_uri (m_stream.source ());
    source_uri.set_path (tl::dirname (source_uri.path ()));
    return source_uri.resolved (tl::URI (path)).to_string ();

  }
}

void
MALYReader::do_read (db::Layout &layout, db::cell_index_type cell_index, tl::TextInputStream &stream)
{
  try {

    // @@@

  } catch (tl::Exception &ex) {
    error (ex.msg ());
  }
}

}

