
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


#include "tlGlobPattern.h"

namespace tl
{

static bool 
do_match (const char *p, const char *s, bool cs, bool exact, bool hm, std::vector<std::string> *o, std::vector<std::pair<unsigned int, const char *> > &bstart)
{
  while (*p) {

    if (!exact && *p == '\\') {

      ++p;
      if (!*s || *s != *p) {
        return false;
      }
      if (*p) {
        ++p;
      }
      ++s;

    } else if (!exact && *p == '?') {

      ++p;
      if (! *s) {
        return false;
      }
      ++s;

    } else if (!exact && *p == '*') {

      ++p;

      //  a trailing '*' always matches
      if (!*p) {
        return true;
      }

      std::vector<std::pair<unsigned int, const char *> > bs = bstart;
      size_t no = o ? o->size () : 0;

      while (*s) {
        if (do_match (p, s, cs, exact, hm, o, bstart)) {
          return true;
        }
        bstart = bs;
        if (o && o->begin () + no < o->end ()) {
          o->erase (o->begin () + no, o->end ());
        }
        ++s;
      }

    } else if (!exact && *p == '[') {

      if (! *s) {
        return false;
      }

      bool negate = false;
      ++p;
      if (*p && *p == '^') {
        ++p;
        negate = true;
      }

      bool hit = false;

      while (*p != ']' && *p) {

        char c1 = *p;
        if (c1 == '\\') {
          c1 = *++p;
        } 
        if (*p) {
          ++p;
        }

        char c2 = c1;
        if (*p == '-') {
          ++p;
          c2 = *p;
          if (c2 == '\\') {
            c2 = *++p;
          }
          if (*p) {
            ++p;
          }
        }

        if (! hit) {
          if (cs && *s >= c1 && *s <= c2) {
            hit = true;
          //  TODO: implement UTF-8 support
          } else if (!cs && tolower (*s) >= tolower (c1) && tolower (*s) <= tolower (c2)) {
            hit = true;
          }
        }

      }

      if (negate == hit) {
        return false;
      }

      ++s;
      if (*p) {
        ++p;
      }

    } else if (!exact && *p == '{') {

      ++p;

      bool hit = false;
      const char *s0 = s;

      while (*p) {

        if (hit) {

          while (*p && *p != ',' && *p != '}') {
            if (*p == '\\') {
              ++p;
            }
            if (*p) {
              ++p;
            }
          }

        } else {

          s = s0;
          hit = true;
          while (*p && *p != ',' && *p != '}') {
            if (*p == '\\') {
              ++p;
            }
            if (hit) {
              if (! *s) {
                hit = false;
              } else if (cs && *p != *s) {
                hit = false;
              //  TODO: implement UTF-8 support
              } else if (!cs && tolower (*p) != tolower (*s)) {
                hit = false;
              } else {
                ++s;
              }
            }
            if (*p) {
              ++p;
            }
          }

        }

        if (*p == ',') {
          ++p;
        } else if (*p == '}') {
          ++p;
          break;
        }

      }

      if (! hit) {
        return false;
      }

    } else if (!exact && *p == ')') {

      ++p;

      if (! bstart.empty ()) {
        if (o) {
          (*o)[bstart.back ().first] = std::string (bstart.back ().second, s - bstart.back ().second);
        }
        bstart.pop_back ();
      }

    } else if (!exact && *p == '(') {

      ++p;
      if (o) {
        bstart.push_back (std::make_pair ((unsigned int) o->size (), s));
        o->push_back (std::string ());
      }

    } else {

      if (cs) {
        if (*s != *p) {
          return false;
        } else {
          ++s;
          ++p;
        }
      } else {
        //  TODO: implement UTF-8 support
        if (tolower (*s) != tolower (*p)) {
          return false;
        } else {
          ++s;
          ++p;
        }
      }

    }

  } 

  return (hm || *s == 0);
}

GlobPattern::GlobPattern ()
  : m_case_sensitive (true), m_exact (false), m_header_match (false)
{
  //  .. nothing yet ..
}

GlobPattern::GlobPattern (const std::string &p)
  : m_p (p), m_case_sensitive (true), m_exact (false), m_header_match (false)
{
  //  .. nothing yet ..
}

void GlobPattern::set_case_sensitive (bool f)
{
  m_case_sensitive = f;
}

bool GlobPattern::case_sensitive () const
{
  return m_case_sensitive;
}

void GlobPattern::set_exact (bool f)
{
  m_exact = f;
}

bool GlobPattern::exact () const
{
  return m_exact;
}

void GlobPattern::set_header_match (bool f)
{
  m_header_match = f;
}

bool GlobPattern::header_match () const
{
  return m_header_match;
}

bool GlobPattern::match (const char *s) const
{
  std::vector<std::pair<unsigned int, const char *> > bstart;
  return do_match (m_p.c_str (), s, m_case_sensitive, m_exact, m_header_match, 0, bstart);
}

bool GlobPattern::match (const char *s, std::vector<std::string> &e) const
{
  if (! e.empty ()) {
    e.clear ();
  }
  std::vector<std::pair<unsigned int, const char *> > bstart;
  return do_match (m_p.c_str (), s, m_case_sensitive, m_exact, m_header_match, &e, bstart);
}

bool GlobPattern::match (const std::string &s) const
{
  std::vector<std::pair<unsigned int, const char *> > bstart;
  return do_match (m_p.c_str (), s.c_str (), m_case_sensitive, m_exact, m_header_match, 0, bstart);
}

bool GlobPattern::match (const std::string &s, std::vector<std::string> &e) const
{
  if (! e.empty ()) {
    e.clear ();
  }
  std::vector<std::pair<unsigned int, const char *> > bstart;
  return do_match (m_p.c_str (), s.c_str (), m_case_sensitive, m_exact, m_header_match, &e, bstart);
}

}
