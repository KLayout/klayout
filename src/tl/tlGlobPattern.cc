
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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
do_match (const char *p, const char *s, std::vector<std::string> *o, std::vector<std::pair<unsigned int, const char *> > &bstart)
{
  while (*p) {

    if (*p == '\\') {

      ++p;
      if (!*s || *s != *p) {
        return false;
      }
      if (*p) {
        ++p;
      }
      ++s;

    } else if (*p == '?') {

      ++p;
      if (! *s) {
        return false;
      }
      ++s;

    } else if (*p == '*') {

      ++p;

      //  a trailing '*' always matches
      if (!*p) {
        return true;
      }

      std::vector<std::pair<unsigned int, const char *> > bs = bstart;
      size_t no = o ? o->size () : 0;

      while (*s) {
        if (do_match (p, s, o, bstart)) {
          return true;
        }
        bstart = bs;
        if (o && o->begin () + no < o->end ()) {
          o->erase (o->begin () + no, o->end ());
        }
        ++s;
      }

    } else if (*p == '[') {

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
          if (*s >= c1 && *s <= c2) {
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

    } else if (*p == '{') {

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
              } else if (*p != *s) {
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

    } else if (*p == ')') {

      ++p;

      if (! bstart.empty ()) {
        if (o) {
          (*o)[bstart.back ().first] = std::string (bstart.back ().second, s - bstart.back ().second);
        }
        bstart.pop_back ();
      }

    } else if (*p == '(') {

      ++p;
      if (o) {
        bstart.push_back (std::make_pair ((unsigned int) o->size (), s));
        o->push_back (std::string ());
      }

    } else if (*s != *p) {
      return false;
    } else {
      ++s;
      ++p;
    }

  } 

  return (*s == 0);
}

GlobPattern::GlobPattern ()
{
  //  .. nothing yet ..
}

GlobPattern::GlobPattern (const std::string &p)
  : m_p (p)
{
  //  .. nothing yet ..
}

bool GlobPattern::match (const char *s) const
{
  std::vector<std::pair<unsigned int, const char *> > bstart;
  return do_match (m_p.c_str (), s, 0, bstart);
}

bool GlobPattern::match (const char *s, std::vector<std::string> &e) const
{
  if (! e.empty ()) {
    e.clear ();
  }
  std::vector<std::pair<unsigned int, const char *> > bstart;
  return do_match (m_p.c_str (), s, &e, bstart);
}

bool GlobPattern::match (const std::string &s) const
{
  std::vector<std::pair<unsigned int, const char *> > bstart;
  return do_match (m_p.c_str (), s.c_str (), 0, bstart);
}

bool GlobPattern::match (const std::string &s, std::vector<std::string> &e) const
{
  if (! e.empty ()) {
    e.clear ();
  }
  std::vector<std::pair<unsigned int, const char *> > bstart;
  return do_match (m_p.c_str (), s.c_str (), &e, bstart);
}

}
