
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


#ifndef HDR_dbHash
#define HDR_dbHash

#if defined(__GNUC__)
#   include <ext/hash_map>
#   include <ext/hash_set>
namespace std_ext = __gnu_cxx;
#  define DB_HASH_NAMESPACE __gnu_cxx
#else
#   include <hash_map>
#   include <hash_set>
namespace std_ext = std;
#  define DB_HASH_NAMESPACE std
#endif

#include "dbPoint.h"
#include "dbVector.h"
#include "dbBox.h"
#include "dbText.h"
#include "dbPath.h"
#include "dbPolygon.h"
#include "dbInstances.h"

/**
 *  This header defines some hash functions for various database objects 
 *  for use with std_ext::hash_map and std_ext::hash_set
 *
 *  It also provides namespace abstraction for the std_ext namespace
 */

namespace DB_HASH_NAMESPACE
{
#if !defined(__LP64__)
  /**
   *  @brief Specialization missing for size_t on WIN32
   */
  template<>
  struct hash<size_t>
  {
    size_t operator()(size_t __x) const
    {
      return __x;
    }
  };
#endif

  template <class T>
  inline size_t hfunc (const T &t)
  {
    hash <T> hf;
    return hf (t);
  }

  inline size_t hcombine (size_t h1, size_t h2)
  {
    return (h1 << 4) ^ (h1 >> 4) ^ h2;
  }

  template <class T>
  inline size_t hfunc (const T &t, size_t h)
  {
    hash <T> hf;
    return hcombine (h, hf (t));
  }

  /**
   *  @brief Hash value for a point
   */
  template <> 
  struct hash <db::Point>
  {
    size_t operator() (const db::Point &o) const
    {
      return hfunc (o.x (), hfunc (o.y ()));
    }
  };

  /**
   *  @brief Hash value for a point
   */
  template <>
  struct hash <db::Vector>
  {
    size_t operator() (const db::Vector &o) const
    {
      return hfunc (o.x (), hfunc (o.y ()));
    }
  };

  /**
   *  @brief Hash value for a box
   */
  template <> 
  struct hash <db::Box>
  {
    size_t operator() (const db::Box &o) const
    {
      return hfunc (o.p1 (), hfunc (o.p2 ()));
    }
  };

  /**
   *  @brief Hash value for an edge
   */
  template <> 
  struct hash <db::Edge>
  {
    size_t operator() (const db::Edge &o) const
    {
      return hfunc (o.p1 (), hfunc (o.p2 ()));
    }
  };

  /**
   *  @brief Hash value for a text object
   */
  template <> 
  struct hash <db::Text>
  {
    size_t operator() (const db::Text &o) const
    {
      size_t h = hfunc (int (o.halign ()));
      h = hfunc (int (o.valign ()), h);
      h = hfunc (o.trans ().rot (), h);
      h = hfunc (o.trans ().disp (), h);
      for (const char *cp = o.string (); *cp; ++cp) {
        h = hfunc (*cp, h);
      }
      return h;
    }
  };

  /**
   *  @brief Hash value for a path
   */
  template <> 
  struct hash <db::Path>
  {
    size_t operator() (const db::Path &o) const
    {
      size_t h = hfunc (int (o.round ()));
      h = hfunc (o.bgn_ext (), h);
      h = hfunc (o.end_ext (), h);
      h = hfunc (o.width (), h);
      for (db::Path::iterator p = o.begin (); p != o.end (); ++p) {
        h = hfunc (*p, h);
      }
      return h;
    }
  };

  /**
   *  @brief Hash value for a polygon
   */
  template <> 
  struct hash <db::Polygon>
  {
    size_t operator() (const db::Polygon &o) const
    {
      size_t h = o.hull ().hash ();
      for (size_t i = 0; i < o.holes (); ++i) {
        h = hfunc (o.hole (i).hash (), h);
      }
      return h;
    }
  };

  /**
   *  @brief Hash value for a simple polygon
   */
  template <> 
  struct hash <db::SimplePolygon>
  {
    size_t operator() (const db::SimplePolygon &o) const
    {
      return o.hull ().hash ();
    }
  };

  /**
   *  @brief A hash value for a db::CellInstArray
   */
  template <> 
  struct hash <db::CellInstArray>
  {
    size_t operator() (const db::CellInstArray &o) const
    {
      size_t h = hfunc (o.object ().cell_index ());
      db::Vector a, b;
      unsigned long na = 1, nb = 1;
      if (o.is_regular_array (a, b, na, nb)) {
        h = hfunc (a, h);
        h = hfunc (b, h);
        h = hfunc (na, h);
        h = hfunc (nb, h);
      }

      if (o.is_complex ()) {
        db::ICplxTrans t = o.complex_trans ();
        h = hfunc (int (0.5 + t.angle () * 1000000), h);
        h = hfunc (int (0.5 + t.mag () * 1000000), h);
        h = hfunc (int (t.is_mirror ()), h);
        h = hfunc (db::Vector (t.disp ()), h);
      } else {
        db::Trans t = o.front ();
        h = hfunc (int (t.rot ()), h);
        h = hfunc (t.disp (), h);
      }

      return h;
    }
  };

  /**
   *  @brief Hash value for an object with properties
   */
  template <class O> 
  struct hash <db::object_with_properties<O> >
  {
    size_t operator() (const db::object_with_properties<O> &o) const
    {
      return hfunc ((const O &) o, hfunc (o.properties_id ()));
    }
  };

  /**
   *  @brief Generic hash for a pair of objects
   */
  template <class T1, class T2>
  struct hash <std::pair <T1, T2> > 
  {
    size_t operator() (const std::pair<T1, T2> &p) const
    {
      hash <T1> hf1;
      hash <T2> hf2;
      size_t h = hf1 (p.first);
      return hfunc (hf2 (p.second), h);
    }
  };
}

#endif

