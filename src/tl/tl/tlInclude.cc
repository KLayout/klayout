
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

#include "tlInclude.h"

#include "tlAssert.h"
#include "tlString.h"
#include "tlFileUtils.h"
#include "tlStream.h"
#include "tlExpression.h"
#include "tlUri.h"

namespace tl
{

// -----------------------------------------------------------------------------------------------------
//  IncludeExpander implementation

static const char *valid_fn_chars = "@_:,.\\/-+";

IncludeExpander::IncludeExpander ()
{
  //  .. nothing yet ..
}

IncludeExpander
IncludeExpander::expand (const std::string &path, std::string &expanded_text, const IncludeFileResolver *resolver)
{
  IncludeExpander ie;
  int lc = 1;
  tl::InputStream is (path);
  ie.read (path, is, expanded_text, lc, resolver);
  return ie;
}

IncludeExpander
IncludeExpander::expand (const std::string &path, const std::string &original_text, std::string &expanded_text, const IncludeFileResolver *resolver)
{
  IncludeExpander ie;
  int lc = 1;
  tl::InputMemoryStream ms (original_text.c_str (), original_text.size ());
  tl::InputStream is (ms);
  ie.read (path, is, expanded_text, lc, resolver);
  return ie;
}

void
IncludeExpander::read (const std::string &path, tl::InputStream &is, std::string &expanded_text, int &line_counter, const IncludeFileResolver *resolver)
{
  m_sections [line_counter] = std::make_pair (path, 1 - line_counter);

  tl::TextInputStream text (is);

  int lnum = 0;
  bool emit_section = false;

  while (! text.at_end ()) {

    std::string l = text.get_line ();
    ++lnum;

    tl::Extractor ex (l.c_str ());
    if (ex.test ("#") && ex.test ("%include")) {

      std::string include_path;
      if (*ex.skip () == '"' || *ex.skip () == '\'') {
        ex.read_quoted (include_path);
        ex.expect_end ();
      } else {
        include_path = tl::trim (ex.skip ());
      }

      //  allow some customization by employing expression interpolation
      include_path = tl::Eval ().interpolate (include_path);

      //  NOTE: by using URI's we can basically read from HTTP etc.
      tl::URI current_uri (path);
      tl::URI new_uri (include_path);
      if (current_uri.scheme ().empty () && new_uri.scheme ().empty ()) {
        if (! tl::is_absolute (include_path)) {
          include_path = tl::combine_path (tl::dirname (path), include_path);
        }
      } else {
        include_path = current_uri.resolved (new_uri).to_abstract_path ();
      }

      std::string include_text;
      if (resolver) {
        include_text = resolver->get_text (include_path);
      } else {
        tl::InputStream iis (include_path);
        include_text = iis.read_all ();
      }

      tl::InputMemoryStream ms (include_text.c_str (), include_text.size ());
      tl::InputStream is (ms);
      read (include_path, is, expanded_text, line_counter, resolver);

      emit_section = true;

    } else {

      if (emit_section) {
        emit_section = false;
        m_sections [line_counter] = std::make_pair (path, lnum - line_counter);
      }

      expanded_text += l;
      expanded_text += "\n";
      ++line_counter;

    }

  }
}

std::string
IncludeExpander::to_string () const
{
  if (m_sections.empty ()) {

    return std::string ();

  } else if (m_sections.size () == 1) {

    tl_assert (m_sections.begin ()->first == 1);
    tl_assert (m_sections.begin ()->second.second == 0);

    std::string fn = m_sections.begin ()->second.first;
    if (! fn.empty () && fn.front () == '@') {
      return tl::to_quoted_string (fn);
    } else {
      return fn;
    }

  } else {

    //  "@" indicates a mapping table
    std::string res ("@");

    for (std::map<int, std::pair<std::string, int> >::const_iterator m = m_sections.begin (); m != m_sections.end (); ++m) {
      res += tl::to_string (m->first);
      res += "*";
      res += tl::to_word_or_quoted_string (m->second.first, valid_fn_chars);
      res += "*";
      res += tl::to_string (m->second.second);
      res += ";";
    }

    return res;

  }
}

IncludeExpander
IncludeExpander::from_string (const std::string &s)
{
  IncludeExpander ie;

  tl::Extractor ex (s.c_str ());

  if (*ex == '"' || *ex == '\'') {

    ex.read_quoted (ie.m_sections [1].first);

  } else if (*ex == '@') {

    ++ex;

    while (! ex.at_end ()) {

      int ln = 0;
      ex.read (ln);

      std::pair<std::string, int> &si = ie.m_sections [ln];

      ex.expect ("*");
      ex.read_word_or_quoted (si.first, valid_fn_chars);
      ex.expect ("*");
      ex.read (si.second);

      ex.test (";");

    }

  } else {

    ie.m_sections [1].first = s;

  }

  return ie;
}

std::pair<std::string, int>
IncludeExpander::translate_to_original (int line_number)
{
  std::map<int, std::pair<std::string, int> >::const_iterator m = m_sections.lower_bound (line_number);
  if (m != m_sections.begin () && (m == m_sections.end () || m->first > line_number)) {
    --m;
  }
  if (m == m_sections.end ()) {
    return std::make_pair (std::string (), 0);
  } else {
    return std::make_pair (m->second.first, line_number + m->second.second);
  }
}

}
