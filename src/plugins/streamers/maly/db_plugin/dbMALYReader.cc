
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
//  MALYReader

MALYReader::MALYReader (tl::InputStream &s)
  : m_stream (s),
    m_progress (tl::to_string (tr ("Reading MALY file")), 1000),
    m_dbu (0.001),
    m_last_record_line (0)
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
  std::cout << data.to_string () << std::endl;
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
    m_record_returned.clear ();

    return tl::Extractor (m_record.c_str ());

  }

  while (! m_stream.at_end ()) {

    m_last_record_line = m_stream.line_number ();
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
    } else if (ex.test ("NONE")) {
      ymirror = false;
    } else {
      error (tl::to_string (tr ("Expected 'Y' or 'NONE' for MIRROR spec")));
    }
  }

  if (ex.test ("ROTATE")) {
    unsigned int a = 0;
    ex.read (a);
    rot = (a / 90) % 4;
  }

  spec.trans = db::DTrans (rot, ymirror, db::DVector (x, y));
}

MALYReader::MALYReaderParametersData::Base
MALYReader::string_to_base (const std::string &string)
{
  if (string == "ORIGIN") {
    return MALYReaderParametersData::Origin;
  } else if (string == "LOWERLEFT") {
    return MALYReaderParametersData::LowerLeft;
  } else if (string == "CENTER") {
    return MALYReaderParametersData::Center;
  } else {
    throw tl::Exception (tl::to_string (tr ("Unknown base specification: ")) + string);
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
      //  TODO: what to do with "para"

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

    } else if (ex.test ("PROPERTY")) {

      if (data.refs.empty ()) {
        error (tl::to_string (tr ("PROPERTY entry without a preceeding SREF or AREF")));
      }

      while (! ex.at_end ()) {
        if (ex.test ("DNAME")) {
          ex.read_word_or_quoted (data.refs.back ().dname);
        } else if (ex.test ("ENAME")) {
          ex.read_word_or_quoted (data.refs.back ().ename);
        } else if (ex.test ("MNAME")) {
          ex.read_word_or_quoted (data.refs.back ().mname);
        } else {
          error (tl::to_string (tr ("Unknown PROPERTY item")));
        }
      }

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
      create_masks (cmask, masks, data);
      return true;

    } else if (begin_section (ex, "MASK")) {

      masks.push_back (MALYReaderMaskData ());
      ex.read (masks.back ().name);

      ex.expect_end ();
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
MALYReader::create_masks (const MALYReaderMaskData &cmask, const std::list<MALYReaderMaskData> &masks, MALYData &data)
{
  for (auto i = masks.begin (); i != masks.end (); ++i) {

    data.masks.push_back (MALYMask ());
    MALYMask &m = data.masks.back ();

    m.name = i->name;

    m.size_um = i->parameters.masksize * 25400.0;
    if (m.size_um < db::epsilon) {
      m.size_um = cmask.parameters.masksize * 25400.0;
    }
    if (m.size_um < db::epsilon) {
      m.size_um = 7.0 * 25400.0;
      warn (tl::to_string (tr ("No mask size given for - using default of 7 inch for mask: ")) + m.name);
    }

    MALYTitle::Font font = i->parameters.font;
    if (font == MALYTitle::FontNotSet) {
      font = cmask.parameters.font;
    }
    if (font == MALYTitle::FontNotSet) {
      font = MALYTitle::Standard;
    }

    const MALYReaderTitleSpec *date_spec = 0;
    if (i->title.date_spec.enabled) {
      date_spec = &i->title.date_spec;
    } else if (cmask.title.date_spec.enabled) {
      date_spec = &cmask.title.date_spec;
    }
    if (date_spec) {
      m.titles.push_back (create_title (MALYTitle::Date, *date_spec, font, std::string ("<SERIAL>")));
    }

    const MALYReaderTitleSpec *serial_spec = 0;
    if (i->title.serial_spec.enabled) {
      serial_spec = &i->title.serial_spec;
    } else if (cmask.title.serial_spec.enabled) {
      serial_spec = &cmask.title.serial_spec;
    }
    if (date_spec) {
      m.titles.push_back (create_title (MALYTitle::Serial, *serial_spec, font, std::string ("<DATE>")));
    }

    for (auto t = i->title.string_titles.begin (); t != i->title.string_titles.end (); ++t) {
      m.titles.push_back (create_title (MALYTitle::String, t->second, font, t->first));
    }
    for (auto t = cmask.title.string_titles.begin (); t != cmask.title.string_titles.end (); ++t) {
      m.titles.push_back (create_title (MALYTitle::String, t->second, font, t->first));
    }

    MALYReaderParametersData::Base base = i->parameters.base;
    if (base == MALYReaderParametersData::BaseNotSet) {
      base = cmask.parameters.base;
    }
    if (base == MALYReaderParametersData::BaseNotSet) {
      base = MALYReaderParametersData::Center;
      warn (tl::to_string (tr ("No structure placement given - using 'center' for mask: ")) + m.name);
    }

    MALYReaderParametersData::Base array_base = MALYReaderParametersData::BaseNotSet;
    if (array_base == MALYReaderParametersData::BaseNotSet) {
      array_base = cmask.parameters.base;
    }
    if (array_base == MALYReaderParametersData::BaseNotSet) {
      array_base = MALYReaderParametersData::Center;
      warn (tl::to_string (tr ("No array structure placement given - using 'center' for mask: ")) + m.name);
    }

    for (auto sg = cmask.strgroups.begin (); sg != cmask.strgroups.end (); ++sg) {
      for (auto s = sg->refs.begin (); s != sg->refs.end (); ++s) {
        m.structures.push_back (create_structure (i->parameters, cmask.parameters, *s, sg->name, base, array_base));
      }
    }
    for (auto sg = i->strgroups.begin (); sg != i->strgroups.end (); ++sg) {
      for (auto s = sg->refs.begin (); s != sg->refs.end (); ++s) {
        m.structures.push_back (create_structure (i->parameters, cmask.parameters, *s, sg->name, base, array_base));
      }
    }

  }
}

MALYTitle
MALYReader::create_title (MALYTitle::Type type, const MALYReaderTitleSpec &data, MALYTitle::Font font, const std::string &string)
{
  MALYTitle title;

  title.transformation = data.trans;
  title.width = data.width;
  title.height = data.height;
  title.pitch = data.pitch;
  title.type = type;
  title.font = font;
  title.string = string;

  return title;
}

MALYStructure
MALYReader::create_structure (const MALYReaderParametersData &mparam, const MALYReaderParametersData &cparam, const MALYReaderStrRefData &data, const std::string & /*strgroup_name*/, MALYReaderParametersData::Base base, MALYReaderParametersData::Base array_base)
{
  MALYStructure str;

  str.size = data.size;
  str.dname = data.dname;
  str.ename = data.ename;
  str.mname = data.mname;
  str.topcell = data.name;
  str.nx = std::max (1, data.nx);
  str.ny = std::max (1, data.ny);
  str.dx = data.dx;
  str.dy = data.dy;
  str.layer = data.layer;

  str.path = resolve_path (mparam, data.file);
  if (str.path.empty ()) {
    str.path = resolve_path (cparam, data.file);
  }
  if (str.path.empty ()) {
    //  try any fail later ...
    str.path = data.file;
  }

  MALYReaderParametersData::Base eff_base = (data.nx > 1 || data.ny > 1) ? array_base : base;

  db::DPoint rp;
  switch (eff_base) {
  case MALYReaderParametersData::LowerLeft:
    rp = data.size.p1 ();
    break;
  case MALYReaderParametersData::Center:
  default:
    //  NOTE: the center implies the whole array's center in case of an AREF
    rp = (data.size + data.size.moved (db::DVector (str.dx * (str.nx - 1), str.dy * (str.ny - 1)))).center ();
    break;
  case MALYReaderParametersData::Origin:
    break;
  }

  db::DCplxTrans mirr (mparam.maskmirror != cparam.maskmirror ? db::DFTrans::m90 : db::DFTrans::r0);
  str.transformation = mirr * db::DCplxTrans (data.scale, 0.0, false, data.org + (db::DPoint () - rp));

  return str;
}

std::string
MALYReader::resolve_path (const MALYReaderParametersData &param, const std::string &path)
{
  if (tl::is_absolute (path)) {

    return path;

  } else {

    //  NOTE: we don't differentiate by file type here. Each root is used in the
    //  same way to find the actual file.
    //  Relative paths are always resolved relative to the MALY file.

    for (auto r = param.roots.begin (); r != param.roots.end (); ++r) {

      std::string p = tl::combine_path (r->second, path);
      if (! tl::is_absolute (p)) {
        p = tl::combine_path (tl::dirname (m_stream.source ()), p);
      }

      if (tl::file_exists (p)) {
        return p;
      }

    }

  }

  return std::string ();
}

void
MALYReader::do_read_maly_file (MALYData &data)
{
  tl::Extractor ex = read_record ();
  if (! begin_section (ex, "MALY")) {
    error (tl::to_string (tr ("Header expected ('BEGIN MALY')")));
  }

  std::string version;
  ex.read_word (version, ".");
  //  TODO: what to do with version string?

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
  throw MALYReaderException (msg, m_last_record_line, m_stream.source ());
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
             << tl::to_string (tr (" (line=")) << m_last_record_line
             << tl::to_string (tr (", file=")) << m_stream.source ()
             << ")";
  } else if (ws == 0) {
    tl::warn << tl::to_string (tr ("... further warnings of this kind are not shown"));
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

