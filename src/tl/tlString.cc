
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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


#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <cctype>
#include <limits>
#include <vector>
#include <sstream>

#include "tlString.h"
#include "tlExpression.h"
#include "tlInternational.h"

static std::locale c_locale ("C");

// -------------------------------------------------------------------------
//  Utility: a strtod version that is independent of the locale

static std::string micron_format ("%.5f");
static std::string dbu_format ("%.2f");

void tl::set_micron_resolution (unsigned int ndigits)
{
  micron_format = "%." + tl::to_string (ndigits) + "f";
}

void tl::set_db_resolution (unsigned int ndigits)
{
  dbu_format = "%." + tl::to_string (ndigits) + "f";
}

std::string tl::micron_to_string (double d) 
{
  return tl::sprintf (micron_format.c_str (), d);
}

std::string tl::db_to_string (double d) 
{
  return tl::sprintf (dbu_format.c_str (), d);
}

std::string tl::to_upper_case (const std::string &s)
{
  return tl::to_string (tl::to_qstring (s).toUpper ());
}

std::string tl::to_lower_case (const std::string &s)
{
  return tl::to_string (tl::to_qstring (s).toLower ());
}

// -------------------------------------------------------------------------
//  Utility: a strtod version that is independent of the locale

static double local_strtod (const char *cp, const char *&cp_new)
{
  const char *cp0 = cp;

  //  Extract sign
  double s = 1.0;
  if (*cp == '-') {
    s = -1.0;
    ++cp;
  /*
  } else if (*cp == '+') {
    ++cp;
  */
  }

  //  Extract upper digits
  int exponent = 0;
  double mant = 0.0;
  while (isdigit (*cp)) {
    mant = mant * 10.0 + double (*cp - '0');
    ++cp;
  }

  //  Extract lower digits
  if (*cp == '.') {
    ++cp;
    while (isdigit (*cp)) {
      mant = mant * 10.0 + double (*cp - '0');
      ++cp;
      --exponent;
    }
  }

  //  Extract exponent (unless we're at the beginning)
  if (cp != cp0 && (*cp == 'e' || *cp == 'E')) {
    ++cp;
    bool epos = true;
    if (*cp == '-') {
      epos = false;
      ++cp;
    } else if (*cp == '+') {
      ++cp;
    }
    int en = 0;
    while (isdigit (*cp)) {
      en = en * 10 + int (*cp - '0');
      ++cp;
    }
    if (! epos) {
      en = -en;
    }
    exponent += en;
  }

  cp_new = cp;

  return s * mant * pow(10.0, exponent);
}

// -------------------------------------------------------------------------
//  Implementation

std::string 
tl::to_string (double d, int prec)
{
  //  For small values less than 1e-(prec) simply return "0" to avoid ugly values like "1.2321716e-14".
  if (fabs (d) < pow (10.0, -prec)) {
    return "0";
  }

  std::ostringstream os;
  os.imbue (c_locale);
  os.precision (prec);
  os.setf (std::ios_base::fmtflags (0), std::ios::basefield);
  os.setf (std::ios_base::fmtflags (0), std::ios::floatfield);
  os << d;
  return os.str ();
}

std::string 
tl::to_string (float d, int prec)
{
  //  For small values less than 1e-(prec) simply return "0" to avoid ugly values like "1.2321716e-14".
  if (fabs (d) < pow (10.0, -prec)) {
    return "0";
  }

  std::ostringstream os;
  os.imbue (c_locale);
  os.precision (prec);
  os.setf (std::ios_base::fmtflags (0), std::ios::basefield);
  os.setf (std::ios_base::fmtflags (0), std::ios::floatfield);
  os << d;
  return os.str ();
}

template <>
std::string 
tl::to_string (const int &d)
{
  std::ostringstream os;
  os.imbue (c_locale);
  os << d;
  return os.str ();
}

template <>
std::string 
tl::to_string (const unsigned int &d)
{
  std::ostringstream os;
  os.imbue (c_locale);
  os << d;
  return os.str ();
}

template <>
std::string 
tl::to_string (const long &d)
{
  std::ostringstream os;
  os.imbue (c_locale);
  os << d;
  return os.str ();
}

template <>
std::string 
tl::to_string (const long long &d)
{
  std::ostringstream os;
  os.imbue (c_locale);
  os << d;
  return os.str ();
}

template <>
std::string 
tl::to_string (const unsigned long &d)
{
  std::ostringstream os;
  os.imbue (c_locale);
  os << d;
  return os.str ();
}

template <>
std::string 
tl::to_string (const unsigned long long &d)
{
  std::ostringstream os;
  os.imbue (c_locale);
  os << d;
  return os.str ();
}

template <>
std::string
tl::to_string (char * const &cp)
{
  return std::string (cp);
}

template <>
std::string
tl::to_string (const char * const &cp)
{
  return std::string (cp);
}

template <>
std::string
tl::to_string (unsigned char * const &cp)
{
  return std::string ((const char *) cp);
}

template <>
std::string
tl::to_string (const unsigned char * const &cp)
{
  return std::string ((const char *) cp);
}

std::string
tl::to_string (const char *cp, int length)
{
  return std::string (cp, length);
}

std::string
tl::to_string (const unsigned char *cp, int length)
{
  return std::string ((const char *) cp, length);
}

template <>
std::string
tl::to_string (const bool &b)
{
  return b ? "true" : "false";
}

int 
tl::edit_distance (const std::string &a, const std::string &b)
{
  std::vector<int> row0, row1;
  row0.resize (a.size () + 1, 0);
  row1.resize (a.size () + 1, 0);

  for (int i = 0; i <= int (a.size ()); ++i) {
    row0[i] = i;
  }

  for (int i = 0; i < int (b.size ()); ++i) {

    row1[0] = i + 1;

    for (int j = 0; j < int (a.size ()); ++j) {
      int cost = (b[i] == a[j] ? 0 : 1);
      row1[j + 1] = std::min (row0[j] + cost, std::min (row0[j + 1], row1[j]) + 1);
    }

    row0.swap (row1);

  }

  return row0 [a.size ()];
}

std::string
tl::to_quoted_string (const std::string &s)
{
  std::string r;
  r.reserve (s.size () + 2);
  r += '\'';
  for (const char *c = s.c_str (); *c; ++c) {
    if (*c == '\'' || *c == '\\') {
      r += '\\';
      r += *c;
    } else if (*c == '\n') {
      r += "\\n";
    } else if (*c == '\r') {
      r += "\\r";
    } else if (*c == '\t') {
      r += "\\t";
    } else if (! isprint (*c)) {
      char b [20];
      ::sprintf (b, "\\%03o", int ((unsigned char) *c));
      r += b;
    } else {
      r += *c;
    }
  }
  r += '\'';
  return r;
}

std::string 
tl::escape_string (const std::string &s)
{
  std::string r;
  for (const char *c = s.c_str (); *c; ++c) {
    if (*c == '\\') {
      r += '\\';
      r += *c;
    } else if (*c == '\n') {
      r += "\\n";
    } else if (*c == '\r') {
      r += "\\r";
    } else if (*c == '\t') {
      r += "\\t";
    } else if (! isprint (*c)) {
      char b [20];
      ::sprintf (b, "\\%03o", int ((unsigned char) *c));
      r += b;
    } else {
      r += *c;
    }
  }
  return r;
}

inline char unescape_char (const char * &cp)
{
  if (isdigit (*cp)) {
    int c = 0;
    while (*cp && isdigit (*cp)) {
      c = c * 8 + int (*cp - '0');
      ++cp;
    }
    --cp;
    return char (c);
  } else if (*cp == 'r') {
    return '\r';
  } else if (*cp == 'n') {
    return '\n';
  } else if (*cp == 't') {
    return '\t';
  } else {
    return *cp;
  }
}

std::string 
tl::unescape_string (const std::string &value)
{
  std::string r;
  for (const char *cp = value.c_str (); *cp; ++cp) {
    if (*cp == '\\' && cp[1]) {
      ++cp;
      r += unescape_char (cp);
    } else {
      r += *cp;
    }
  }
  return r;
}

std::string
tl::to_word_or_quoted_string (const std::string &s, const char *non_term)
{
  //  If the string does not contain non_term characters, we may simply keep it. 
  //  Otherwise we need to quote it.
  const char *cp = s.c_str ();
  if (*cp && (isalpha (*cp) || strchr (non_term, *cp) != NULL)) {
    ++cp;
    for ( ; *cp && (isalnum (*cp) || strchr (non_term, *cp) != NULL); ++cp) {
      ;
    }
  }
  if (*cp || s.empty ()) {
    return to_quoted_string (s);
  } else {
    return s;
  }
}

void
tl::escape_to_html (std::string &out, const std::string &in, bool replace_newlines)
{
  for (const char *cp = in.c_str (); *cp; ++cp) {
    if (*cp == '<') {
      out += "&lt;";
    } else if (*cp == '>') {
      out += "&gt;";
    } else if (*cp == '&') {
      out += "&amp;";
    } else if (replace_newlines && *cp == '\n') {
      out += "<br/>";
    } else {
      out += *cp;
    }
  }
}

std::string
tl::escaped_to_html (const std::string &in, bool replace_newlines)
{
  std::string s;
  escape_to_html (s, in, replace_newlines);
  return s;
}

void
tl::from_string (const std::string &s, const char * &result)
{
  result = s.c_str ();
}

void
tl::from_string (const std::string &s, const unsigned char * &result)
{
  result = (unsigned char *) s.c_str ();
}

void 
tl::from_string (const std::string &s, double &v) throw (tl::Exception)
{
  const char *cp = s.c_str ();
  while (*cp && isspace (*cp)) {
    ++cp;
  }
  if (! *cp) {
    throw tl::Exception (tl::to_string (QObject::tr ("Got empty string where a real number was expected")));
  }
  const char *cp_end = cp;
  v = local_strtod (cp, cp_end);
  while (*cp_end && isspace (*cp_end)) {
    ++cp_end;
  }
  if (*cp_end) {
    //  try using an expression
    v = tl::Eval ().parse (s).execute ().to_double ();
  }
}

template <class T>
void 
convert_string_to_int (const std::string &s, T &v) throw (tl::Exception)
{
  double x;
  // HACK: this should be some real string-to-int conversion
  tl::from_string (s, x);
  if (x < std::numeric_limits <T>::min ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Range underflow: ")) + s);
  }
  if (x > std::numeric_limits <T>::max ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Range overflow: ")) + s);
  }
  v = T (x);
  if (x != v) {
    throw tl::Exception (tl::to_string (QObject::tr ("Number cannot be represented precisely: ")) + s);
  }
}

void 
tl::from_string (const std::string &s, int &v) throw (tl::Exception)
{
  convert_string_to_int (s, v);
}

void 
tl::from_string (const std::string &s, long &v) throw (tl::Exception)
{
  convert_string_to_int (s, v);
}

void 
tl::from_string (const std::string &s, long long &v) throw (tl::Exception)
{
  convert_string_to_int (s, v);
}

void 
tl::from_string (const std::string &s, unsigned int &v) throw (tl::Exception)
{
  convert_string_to_int (s, v);
}

void 
tl::from_string (const std::string &s, unsigned long &v) throw (tl::Exception)
{
  convert_string_to_int (s, v);
}

void 
tl::from_string (const std::string &s, unsigned long long &v) throw (tl::Exception)
{
  convert_string_to_int (s, v);
}

void
tl::from_string (const std::string &s, bool &b) throw (tl::Exception)
{
  std::string t (tl::trim (s));
  if (t == "true") {
    b = true;
  } else if (t == "false") {
    b = false;
  } else if (t == "1") {
    b = true;
  } else if (t == "0") {
    b = false;
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid boolean value: ")) + s);
  }
}

std::string
tl::join (const std::vector <std::string> &vv, const std::string &s)
{
  std::ostringstream r;

  bool first = true;
  for (std::vector <std::string>::const_iterator i = vv.begin (); i != vv.end (); ++i) {
    if (!first) {
      r << s;
    }
    first = false;
    r << *i;
  }

  return r.str ();
}

std::vector<std::string> 
tl::split (const std::string &t, const std::string &s)
{
  std::vector<std::string> r;

  size_t p = 0;
  for (size_t pp = 0; (pp = t.find (s, p)) != std::string::npos; p = pp + s.size ()) {
    r.push_back (std::string (t, p, pp - p));
  }

  r.push_back (std::string (t, p));

  return r;
}

std::string 
tl::trim (const std::string &s)
{
  const char *cp = s.c_str ();
  while (isspace (*cp) && *cp) {
    ++cp;
  }

  const char *cq = s.c_str () + s.size ();
  while (cq > cp && isspace (cq [-1])) {
    --cq;
  }

  return std::string (cp, cq - cp);
}

// -------------------------------------------------------------------
//  tl::Extractor implementation

tl::Extractor::Extractor (const char *s)
  : m_cp (s)
{
  //  .. nothing yet ..
}

tl::Extractor::Extractor (const std::string &str)
  : m_str (str)
{
  m_cp = m_str.c_str ();
}

tl::Extractor &
tl::Extractor::read (unsigned int &value)
{
  if (! try_read (value)) {
    error (tl::to_string (QObject::tr ("Expected an unsigned integer value")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::read (unsigned long &value)
{
  if (! try_read (value)) {
    error (tl::to_string (QObject::tr ("Expected an unsigned long integer value")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::read (unsigned long long &value)
{
  if (! try_read (value)) {
    error (tl::to_string (QObject::tr ("Expected an unsigned long integer value")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::read (double &value)
{
  if (! try_read (value)) {
    error (tl::to_string (QObject::tr ("Expected a real number")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::read (int &value)
{
  if (! try_read (value)) {
    error (tl::to_string (QObject::tr ("Expected a integer value")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::read (long &value)
{
  if (! try_read (value)) {
    error (tl::to_string (QObject::tr ("Expected a long integer value")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::read (long long &value)
{
  if (! try_read (value)) {
    error (tl::to_string (QObject::tr ("Expected a long integer value")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::read (bool &value)
{
  if (! try_read (value)) {
    error (tl::to_string (QObject::tr ("Expected a boolean value ('true', 'false')")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::read (std::string &value, const char *term)
{
  if (! try_read (value, term)) {
    error (tl::to_string (QObject::tr ("Expected a string")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::read_word (std::string &value, const char *non_term)
{
  if (! try_read_word (value, non_term)) {
    error (tl::to_string (QObject::tr ("Expected a word string")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::read_word_or_quoted (std::string &value, const char *non_term)
{
  if (! try_read_word (value, non_term) && ! try_read_quoted (value)) {
    error (tl::to_string (QObject::tr ("Expected a word or quoted string")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::read_quoted (std::string &value)
{
  if (! try_read_quoted (value)) {
    error (tl::to_string (QObject::tr ("Expected a quoted string")));
  }
  return *this;
}

bool 
tl::Extractor::try_read (unsigned int &value)
{
  if (! *skip ()) {
    return false;
  } 

  if (! isdigit (*m_cp)) {
    return false;
  } 

  value = 0;
  while (isdigit (*m_cp)) {
    if ((value * 10) / 10 != value) {
      throw tl::Exception (tl::to_string (QObject::tr ("Range overflow on unsigned integer")));
    }
    value *= 10;
    value += (*m_cp - '0');
    ++m_cp;
  }

  return true;
}

bool 
tl::Extractor::try_read (int &value)
{
  if (! *skip ()) {
    return false;
  } 

  bool minus = false;
  if (*m_cp == '-') {
    minus = true;
    ++m_cp;
  } else if (*m_cp == '+') {
    ++m_cp;
  }

  if (! isdigit (*m_cp)) {
    return false;
  } 

  value = 0;
  while (isdigit (*m_cp)) {
    if ((value * 10) / 10 != value) {
      throw tl::Exception (tl::to_string (QObject::tr ("Range overflow on integer")));
    }
    value *= 10;
    value += (*m_cp - '0');
    ++m_cp;
  }

  if (minus) {
    value = -value;
  }

  return true;
}

bool 
tl::Extractor::try_read (unsigned long &value)
{
  if (! *skip ()) {
    return false;
  } 

  if (! isdigit (*m_cp)) {
    return false;
  } 

  value = 0;
  while (isdigit (*m_cp)) {
    if ((value * 10) / 10 != value) {
      throw tl::Exception (tl::to_string (QObject::tr ("Range overflow on unsigned long integer")));
    }
    value *= 10;
    value += (*m_cp - '0');
    ++m_cp;
  }

  return true;
}

bool 
tl::Extractor::try_read (unsigned long long &value)
{
  if (! *skip ()) {
    return false;
  } 

  if (! isdigit (*m_cp)) {
    return false;
  } 

  value = 0;
  while (isdigit (*m_cp)) {
    if ((value * 10) / 10 != value) {
      throw tl::Exception (tl::to_string (QObject::tr ("Range overflow on unsigned long long integer")));
    }
    value *= 10;
    value += (*m_cp - '0');
    ++m_cp;
  }

  return true;
}

bool 
tl::Extractor::try_read (long &value)
{
  if (! *skip ()) {
    return false;
  } 

  bool minus = false;
  if (*m_cp == '-') {
    minus = true;
    ++m_cp;
  } else if (*m_cp == '+') {
    ++m_cp;
  }

  if (! isdigit (*m_cp)) {
    return false;
  } 

  value = 0;
  while (isdigit (*m_cp)) {
    if ((value * 10) / 10 != value) {
      throw tl::Exception (tl::to_string (QObject::tr ("Range overflow on long integer")));
    }
    value *= 10;
    value += (*m_cp - '0');
    ++m_cp;
  }

  if (minus) {
    value = -value;
  }

  return true;
}

bool 
tl::Extractor::try_read (long long &value)
{
  if (! *skip ()) {
    return false;
  } 

  bool minus = false;
  if (*m_cp == '-') {
    minus = true;
    ++m_cp;
  } else if (*m_cp == '+') {
    ++m_cp;
  }

  if (! isdigit (*m_cp)) {
    return false;
  } 

  value = 0;
  while (isdigit (*m_cp)) {
    if ((value * 10) / 10 != value) {
      throw tl::Exception (tl::to_string (QObject::tr ("Range overflow on long long integer")));
    }
    value *= 10;
    value += (*m_cp - '0');
    ++m_cp;
  }

  if (minus) {
    value = -value;
  }

  return true;
}

bool 
tl::Extractor::try_read (double &value)
{
  if (! *skip ()) {
    return false;
  } 

  const char *cp_end = m_cp;
  value = local_strtod (m_cp, cp_end);

  if (cp_end == m_cp) {
    return false;
  } else {
    m_cp = cp_end;
    return true;
  }
}

bool 
tl::Extractor::try_read (bool &value)
{
  if (test ("0") || test ("false")) {
    value = false;
    return true;
  }
  if (test ("1") || test ("true")) {
    value = true;
    return true;
  }
  return false;
}

bool 
tl::Extractor::try_read_word (std::string &string, const char *non_term)
{
  if (! *skip ()) {
    return false;
  } 

  string.clear ();
  while (*m_cp && (isalnum (*m_cp) || strchr (non_term, *m_cp) != NULL)) {
    string += *m_cp;
    ++m_cp;
  }
  return ! string.empty ();
}

bool 
tl::Extractor::try_read_word_or_quoted (std::string &string, const char *non_term)
{
  return try_read_word (string, non_term) || try_read_quoted (string);
}

bool 
tl::Extractor::try_read_quoted (std::string &string)
{
  char q = *skip ();

  if (q != '\'' && q != '\"') {
    return false;
  }

  ++m_cp;
  string.clear ();
  while (*m_cp && *m_cp != q) {
    if (*m_cp == '\\' && m_cp[1]) {
      ++m_cp;
      string += unescape_char (m_cp);
    } else {
      string += *m_cp;
    }
    ++m_cp;
  }
  if (*m_cp == q) {
    ++m_cp;
  }
  return true;
}

bool 
tl::Extractor::try_read (std::string &string, const char *term)
{
  //  if the terminating characters contain line feed for blank, we must not skip over them
  if (strchr (term, '\n') || strchr (term, ' ')) {
    while (isspace (*m_cp) && strchr (term, *m_cp) == 0 && *m_cp) {
      ++m_cp;
    }
    if (! *m_cp) {
      return false;
    }
  } else if (! *skip ()) {
    return false;
  } 

  bool term_is_space = false;
  for (const char *t = term; *t && ! term_is_space; ++t) {
    term_is_space = isspace (*t);
  }

  string.clear ();
  while (*m_cp && (term_is_space || ! isspace (*m_cp)) && strchr (term, *m_cp) == NULL) {
    string += *m_cp;
    ++m_cp;
  }
  return true;
}

tl::Extractor & 
tl::Extractor::expect_end ()
{
  if (! at_end ()) {
    error (tl::to_string (QObject::tr ("Expected end of text")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::expect_more ()
{
  if (at_end ()) {
    error (tl::to_string (QObject::tr ("Expected more text")));
  }
  return *this;
}

tl::Extractor & 
tl::Extractor::expect (const char *token)
{
  if (! test (token)) {
    error (tl::sprintf (tl::to_string (QObject::tr ("Expected '%s'")).c_str (), token));
  }
  return *this;
}

bool 
tl::Extractor::test (const char *token)
{
  skip ();

  const char *cp = m_cp;
  while (*cp && *token) {
    if (*cp != *token) {
      return false;
    }
    ++cp;
    ++token;
  }
  
  if (! *token) {
    m_cp = cp;
    return true;
  } else {
    return false;
  }
}

const char *
tl::Extractor::skip ()
{
  while (isspace (*m_cp) && *m_cp) {
    ++m_cp;
  }
  return m_cp;
}

void
tl::Extractor::error (const std::string &msg)
{
  std::string m (msg);

  if (at_end ()) {
    m += tl::to_string (QObject::tr (", but text ended"));
  } else {
    m += tl::to_string (QObject::tr (" here: "));
    const char *cp = m_cp;
    for (unsigned int i = 0; i < 10 && *cp; ++i, ++cp) {
      m += *cp;
    }
    if (*cp) {
      m += " ..";
    }
  }

  throw tl::Exception (m);
}

// -------------------------------------------------------------------
//  tl::string implementation

tl::string::string (const char *c)
{
  if (c && *c) {
    m_capacity = m_size = strlen (c);
    allocator_t alloc;
    mp_rep = alloc.allocate (m_capacity + 1);
    strcpy (mp_rep, c);
  } else {
    mp_rep = 0;
    m_capacity = m_size = 0;
  }
}

tl::string::string (const char *c, size_t from, size_t to)
{
  m_capacity = m_size = to - from;
  if (m_size > 0) {
    allocator_t alloc;
    mp_rep = alloc.allocate (m_capacity + 1);
    strncpy (mp_rep, c + from, m_size);
    mp_rep [m_size] = 0;
  } else {
    mp_rep = 0;
  }
}

tl::string::string (const tl::string &s)
{
  m_capacity = m_size = s.size ();
  if (m_size > 0) {
    allocator_t alloc;
    mp_rep = alloc.allocate (m_capacity + 1);
    strncpy (mp_rep, s.c_str (), m_size);
    mp_rep [m_size] = 0;
  } else {
    mp_rep = 0;
  }
}

tl::string::string (const tl::string &s, size_t from, size_t to)
{
  m_capacity = m_size = to - from;
  if (m_size > 0) {
    allocator_t alloc;
    mp_rep = alloc.allocate (m_capacity + 1);
    strncpy (mp_rep, s.c_str () + from, m_size);
    mp_rep [m_size] = 0;
  } else {
    mp_rep = 0;
  }
}

tl::string::string (const std::string &s)
{
  m_capacity = m_size = s.size ();
  if (m_size > 0) {
    allocator_t alloc;
    mp_rep = alloc.allocate (m_capacity + 1);
    strncpy (mp_rep, s.c_str (), m_size);
    mp_rep [m_size] = 0;
  } else {
    mp_rep = 0;
  }
}

tl::string::string (const std::string &s, size_t from, size_t to)
{
  m_capacity = m_size = to - from;
  if (m_size > 0) {
    allocator_t alloc;
    mp_rep = alloc.allocate (m_capacity + 1);
    strncpy (mp_rep, s.c_str () + from, m_size);
    mp_rep [m_size] = 0;
  } else {
    mp_rep = 0;
  }
}

tl::string::~string ()
{
  if (mp_rep) {
    allocator_t alloc;
    alloc.deallocate (mp_rep, m_capacity + 1);
  }
  mp_rep = 0;
}

tl::string &
tl::string::operator= (const char *c)
{
  if (c && *c) {
    assign (c, 0, strlen (c));
  } else {
    m_size = 0;
    if (mp_rep) {
      mp_rep [0] = 0;
    }
  }
  return *this;
}

void 
tl::string::assign (const char *c, size_t from, size_t to)
{
  m_size = to - from;
  if (m_size > 0) {
    if (m_capacity < m_size) {
      allocator_t alloc;
      if (mp_rep) {
        alloc.deallocate (mp_rep, m_capacity + 1);
      }
      mp_rep = alloc.allocate (m_size + 1);
      m_capacity = m_size;
    }
    strncpy (mp_rep, c + from, m_size);
    mp_rep [m_size] = 0;
  } else {
    if (mp_rep) {
      mp_rep [0] = 0;
    }
  }
}

tl::string &
tl::string::operator= (const tl::string &s)
{
  if (&s != this) {
    m_size = s.size ();
    if (m_size > 0) {
      if (m_capacity < m_size) {
        allocator_t alloc;
        if (mp_rep) {
          alloc.deallocate (mp_rep, m_capacity + 1);
        }
        mp_rep = alloc.allocate (m_size + 1);
        m_capacity = m_size;
      }
      strncpy (mp_rep, s.mp_rep, m_size);
      mp_rep [m_size] = 0;
    } else {
      if (mp_rep) {
        mp_rep [0] = 0;
      }
    }
  }
  return *this;
}

void 
tl::string::assign (const tl::string &s, size_t from, size_t to)
{
  if (&s != this) {
    assign (s.c_str (), from, to);
  } else if (from != 0 || to != m_size) {
    tl::string substr (s, from, to);
    swap (substr);
  }
}

tl::string &
tl::string::operator= (const std::string &s)
{
  assign (s.c_str (), 0, s.size ());
  return *this;
}

void 
tl::string::assign (const std::string &s, size_t from, size_t to)
{
  assign (s.c_str (), from, to);
}

void
tl::string::clear ()
{
  if (mp_rep) {
    allocator_t alloc;
    alloc.deallocate (mp_rep, m_capacity + 1);
    mp_rep = 0;
  }
  m_size = 0;
  m_capacity = 0;
}

void 
tl::string::reserve (size_t n) 
{
  if (m_capacity < n) {
    allocator_t alloc;
    char *nrep = alloc.allocate (n + 1);
    strncpy (nrep, mp_rep, m_size);
    if (mp_rep) {
      alloc.deallocate (mp_rep, m_capacity + 1);
    }
    mp_rep = nrep;
    m_capacity = n; 
  }
}

bool 
tl::string::operator== (const char *c) const
{
  return (c[0] == c_str()[0] && strcmp (c, c_str()) == 0);
}

bool 
tl::string::operator== (const tl::string &s) const
{
  return (c_str()[0] == s.c_str()[0] && strcmp (c_str(), s.c_str()) == 0);
}

bool 
tl::string::operator!= (const char *c) const
{
  return (c[0] != c_str()[0] || strcmp (c, c_str()) != 0);
}

bool 
tl::string::operator!= (const tl::string &s) const
{
  return (c_str()[0] != s.c_str()[0] || strcmp (c_str(), s.c_str()) != 0);
}

bool 
tl::string::operator< (const char *c) const
{
  return strcmp (c_str(), c) < 0;
}
 
bool 
tl::string::operator< (const tl::string &s) const
{
  return strcmp (c_str(), s.c_str()) < 0;
}

bool 
tl::string::operator<= (const char *c) const
{
  return strcmp (c_str(), c) <= 0;
}
 
bool 
tl::string::operator<= (const tl::string &s) const
{
  return strcmp (c_str(), s.c_str()) <= 0;
}

bool 
tl::string::operator> (const char *c) const
{
  return strcmp (c_str(), c) > 0;
}
 
bool 
tl::string::operator> (const tl::string &s) const
{
  return strcmp (c_str(), s.c_str()) > 0;
}

bool 
tl::string::operator>= (const char *c) const
{
  return strcmp (c_str(), c) >= 0;
}
 
bool 
tl::string::operator>= (const tl::string &s) const
{
  return strcmp (c_str(), s.c_str()) >= 0;
}

// -------------------------------------------------------------------
//  tl::sprintf implementation

#if defined(_STLPORT_VERSION) && _STLPORT_VERSION == 0x521 && defined(_MSC_VER)
/**
 *  @brief Workaround for STLPort 5.2.1 bug with scientific formatting 
 *  In that version, the scientific formatting produces on digit less precision
 *  and replaces uses 0 for the last digit.
 *  To work around that problem we first create one digit too much and delete the
 *  trailing '0' in front of 'E' or 'e'.
 */
std::string format_sci_stlport_fix (double f, int prec, unsigned int flags)
{
  std::ostringstream os;
  os.setf (flags);
  os.precision (prec + 1);
  os << f;
  std::string res;
  res.reserve (os.str ().size ());
  for (const char *cp = os.str ().c_str (); *cp; ++cp) {
	if (*cp == '0' && (cp[1] == 'e' || cp[1] == 'E')) {
	  ++cp;
	}
	res += *cp;
  }
  return res;
};
#endif

std::string
tl::sprintf (const char *f, const std::vector <tl::Variant> &vv, unsigned int a0)
{
  std::ostringstream os;
  os.imbue (c_locale);

  int def_prec = os.precision();

  unsigned int a = a0;

  for (const char *cp = f; *cp; ) {

    if (*cp == '%' && cp[1] == '%') {
      os << '%';
      cp += 2;
    } else if (*cp == '%') {

      ++cp;
      if (*cp == '-') {
        ++cp;
        os << std::left;
      } else {
        os << std::right;
      }

      if (*cp == '0') {
        ++cp;
        os.fill('0');
      } else {
        os.fill(' ');
      }

      unsigned int width = 0;
      while (isdigit (*cp) && *cp) {
        width = (width * 10) + (unsigned int)(*cp - '0');
        ++cp;
      }
      os.width(width);

      if (*cp == '.') {
        ++cp;
        unsigned int prec = 0;
        while (isdigit (*cp) && *cp) {
          prec = (prec * 10) + (unsigned int)(*cp - '0');
          ++cp;
        }
        os.precision(prec);
      } else {
        os.precision(def_prec);
      }

      //  allow up to two 'l' for compatibility
      if (*cp == 'l') {
        ++cp;
        if (*cp == 'l') {
          ++cp;
        }
      }

      if (*cp == 'c' || *cp == 'C') {
        if (a < vv.size ()) {
          os << char (vv [a].to_long ());
        }
      } else if (*cp == 'x' || *cp == 'X') {
        os.setf (std::ios::hex, std::ios::basefield | std::ios::uppercase);
        if (*cp == 'X') {
          os.setf (std::ios::uppercase);
        }
        if (a < vv.size ()) {
          os << vv [a].to_ulong ();
        }
      } else if (*cp == 'u' || *cp == 'U') {
        os.setf (std::ios_base::fmtflags (0), std::ios::basefield);
        if (a < vv.size ()) {
          os << vv [a].to_ulong ();
        }
      } else if (*cp == 'd' || *cp == 'D') {
        os.setf (std::ios_base::fmtflags (0), std::ios::basefield);
        if (a < vv.size ()) {
          os << vv [a].to_long ();
        }
      } else if (*cp == 's' || *cp == 'S') {
        os.setf (std::ios_base::fmtflags (0), std::ios::basefield);
        if (a < vv.size ()) {
          os << vv [a].to_string ();
        }
      } else if (*cp == 'g' || *cp == 'G') {
        os.setf (std::ios_base::fmtflags (0), std::ios::floatfield | std::ios::basefield | std::ios::uppercase);
        if (*cp == 'G') {
          os.setf (std::ios::uppercase);
        }
        if (a < vv.size ()) {
          os << vv [a].to_double ();
        }
      } else if (*cp == 'e' || *cp == 'E') {
        os.setf (std::ios::scientific, std::ios::floatfield | std::ios::basefield | std::ios::uppercase);
        if (*cp == 'E') {
          os.setf (std::ios::uppercase);
        }
        if (a < vv.size ()) {
#if defined(_STLPORT_VERSION) && _STLPORT_VERSION == 0x521 && defined(_MSC_VER)
	      os << format_sci_stlport_fix (vv [a].to_double (), os.precision (), os.flags ()).c_str ();
#else
          os << vv [a].to_double ();
#endif
        }
      } else if (*cp == 'f' || *cp == 'F') {
        os.setf (std::ios::fixed, std::ios::floatfield | std::ios::basefield);
        if (a < vv.size ()) {
          os << vv [a].to_double ();
        }
      }

      if (*cp) {
        ++cp;
      }

      ++a;

    } else {
      os << *cp;
      ++cp;
    }

  }

  return os.str ();
}

std::string
tl::sprintf (const std::string &f, const std::vector <tl::Variant> &vv, unsigned int a0)
{
  return tl::sprintf (f.c_str (), vv, a0);
}

