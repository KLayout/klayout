
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

#ifndef HDR_dbHash
#define HDR_dbHash

#include <unordered_map>
#include <unordered_set>

#include "dbPoint.h"
#include "dbVector.h"
#include "dbBox.h"
#include "dbText.h"
#include "dbPath.h"
#include "dbPolygon.h"
#include "dbTrans.h"
#include "dbEdgePair.h"
#include "dbInstances.h"
#include "dbLayout.h"

#include <string>
#include <functional>
#include <stdint.h>

/**
 *  This header defines some hash functions for various database objects 
 *  for use with std::unordered_map and std::unordered_set
 *
 *  It also provides namespace abstraction for the std_ext namespace
 */

namespace std
{
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

  inline size_t hfunc_coord (db::DCoord d)
  {
    return hfunc (int64_t (floor (0.5 + d / db::coord_traits<db::DCoord>::prec ())));
  }

  inline size_t hfunc_coord (db::Coord d)
  {
    return hfunc (d);
  }

  template <class C>
  inline size_t hfunc_coord (C d, size_t h)
  {
    return hcombine (hfunc_coord (d), h);
  }

  /**
   *  @brief Hash value for a point
   */
  template <class C>
  struct hash <db::point<C> >
  {
    size_t operator() (const db::point<C> &o) const
    {
      return hfunc_coord (o.x (), hfunc_coord (o.y ()));
    }
  };

  /**
   *  @brief Hash value for a vector
   */
  template <class C>
  struct hash <db::vector<C> >
  {
    size_t operator() (const db::vector<C> &o) const
    {
      return hfunc_coord (o.x (), hfunc_coord (o.y ()));
    }
  };

  /**
   *  @brief Hash value for a box
   */
  template <class C>
  struct hash <db::box<C> >
  {
    size_t operator() (const db::box<C> &o) const
    {
      return hfunc (o.p1 (), hfunc (o.p2 ()));
    }
  };

  /**
   *  @brief Hash value for an edge
   */
  template <class C>
  struct hash <db::edge<C> >
  {
    size_t operator() (const db::edge<C> &o) const
    {
      return hfunc (o.p1 (), hfunc (o.p2 ()));
    }
  };

  /**
   *  @brief Hash value for an edge pair
   */
  template <class C>
  struct hash <db::edge_pair<C> >
  {
    size_t operator() (const db::edge_pair<C> &o) const
    {
      return hfunc (o.first (), hfunc (o.second ()));
    }
  };

  /**
   *  @brief Hash value for a text object
   */
  template <class C>
  struct hash <db::text<C> >
  {
    size_t operator() (const db::text<C> &o) const
    {
      size_t h = hfunc (int (o.halign ()));
      h = hfunc (int (o.valign ()), h);
      h = hfunc (o.trans ().rot (), h);
      h = hfunc (o.trans ().disp (), h);
      //  NOTE: using std::string for the value makes sure the default hasher doesn't use the pointer value
      h = hfunc (hfunc (std::string (o.string ())), h);
      return h;
    }
  };

  /**
   *  @brief Hash value for a path
   */
  template <class C>
  struct hash <db::path<C> >
  {
    size_t operator() (const db::path<C> &o) const
    {
      size_t h = hfunc (int (o.round ()));
      h = hfunc_coord (o.bgn_ext (), h);
      h = hfunc_coord (o.end_ext (), h);
      h = hfunc_coord (o.width (), h);
      for (typename db::path<C>::iterator p = o.begin (); p != o.end (); ++p) {
        h = hfunc (*p, h);
      }
      return h;
    }
  };

  /**
   *  @brief Hash value for a polygon contour
   */
  template <class C>
  struct hash <db::polygon_contour<C> >
  {
    size_t operator() (const db::polygon_contour<C> &o) const
    {
      size_t h = 0;
      for (typename db::polygon_contour<C>::simple_iterator i = o.begin (); i != o.end (); ++i) {
        h = hfunc (*i, h);
      }
      return h;
    }
  };

  /**
   *  @brief Hash value for a polygon
   */
  template <class C>
  struct hash <db::polygon<C> >
  {
    size_t operator() (const db::polygon<C> &o) const
    {
      size_t h = hfunc (o.hull ());
      for (size_t i = 0; i < o.holes (); ++i) {
        h = hfunc (o.hole (int (i)), h);
      }
      return h;
    }
  };

  /**
   *  @brief Hash value for a simple polygon
   */
  template <class C>
  struct hash <db::simple_polygon<C> >
  {
    size_t operator() (const db::simple_polygon<C> &o) const
    {
      return hfunc (o.hull ());
    }
  };

  /**
   *  @brief Hash value for a simple transformation
   */
  template <class C>
  struct hash <db::simple_trans<C> >
  {
    size_t operator() (const db::simple_trans<C> &t) const
    {
      return hfunc (int (t.rot ()), hfunc (t.disp ()));
    }
  };

  /**
   *  @brief A hash function for a displacement transformation
   */
  template <class C>
  struct hash <db::disp_trans<C> >
  {
    size_t operator() (const db::disp_trans<C> &t) const
    {
      return hfunc (t.disp ());
    }
  };

  /**
   *  @brief Hash value for a complex transformation
   */
  template <class I, class F, class R>
  struct hash <db::complex_trans<I, F, R> >
  {
    size_t operator() (const db::complex_trans<I, F, R> &t) const
    {
      size_t h = hfunc (int64_t (0.5 + t.angle () / db::epsilon));
      h = hfunc (int64_t (0.5 + t.mag () / db::epsilon), h);
      h = hfunc (int (t.is_mirror ()), h);
      h = hfunc (t.disp (), h);
      return h;
    }
  };

  /**
   *  @brief A hash value for a db::CellInstArray and db::DCellInstArray
   */
  template <class C>
  struct hash <db::array <db::CellInst, db::simple_trans<C> > >
  {
    size_t operator() (const db::array <db::CellInst, db::simple_trans<C> > &o) const
    {
      size_t h = hfunc (o.object ().cell_index ());

      db::vector<C> a, b;
      unsigned long na = 1, nb = 1;
      if (o.is_regular_array (a, b, na, nb)) {
        h = hfunc (a, h);
        h = hfunc (b, h);
        h = hfunc (na, h);
        h = hfunc (nb, h);
      }

      if (o.is_complex ()) {
        h = hfunc (o.complex_trans (), h);
      } else {
        h = hfunc (o.front (), h);
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
   *  @brief A hash function for a shape reference
   */
  template <class Shape, class Trans>
  struct hash<db::shape_ref<Shape, Trans> >
  {
    size_t operator() (const db::shape_ref<Shape, Trans> &o) const
    {
      return hfunc (std::hash<Shape> () (*o.ptr ()), std::hash<Trans> () (o.trans ()));
    }
  };

  /**
   *  @brief A hash function for a polygon reference
   */
  template <class Shape, class Trans>
  struct hash<db::polygon_ref<Shape, Trans> >
  {
    size_t operator() (const db::polygon_ref<Shape, Trans> &o) const
    {
      return std::hash<db::shape_ref<Shape, Trans> > () (o);
    }
  };

  /**
   *  @brief A hash function for a path reference
   */
  template <class Shape, class Trans>
  struct hash<db::path_ref<Shape, Trans> >
  {
    size_t operator() (const db::path_ref<Shape, Trans> &o) const
    {
      return std::hash<db::shape_ref<Shape, Trans> > () (o);
    }
  };

  /**
   *  @brief A hash function for a text reference
   */
  template <class Shape, class Trans>
  struct hash<db::text_ref<Shape, Trans> >
  {
    size_t operator() (const db::text_ref<Shape, Trans> &o) const
    {
      return std::hash<db::shape_ref<Shape, Trans> > () (o);
    }
  };

  /**
   *  @brief A hash value for a db::LayerProperties object
   */
  template <>
  struct hash <db::LayerProperties>
  {
    size_t operator() (const db::LayerProperties &o) const
    {
      if (o.is_named ()) {
        return hfunc (o.name);
      } else {
        size_t h = hfunc (o.layer);
        h = hfunc (o.datatype, h);
        h = hfunc (o.name, h);
        return h;
      }
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

  /**
   *  @brief Generic hash for an unordered set
   */
  template <class T>
  struct hash<std::unordered_set<T> >
  {
    size_t operator() (const std::unordered_set<T> &o) const
    {
      size_t hf = 0;
      for (typename std::unordered_set<T>::const_iterator i = o.begin (); i != o.end (); ++i) {
        hf = hfunc (hf, std::hash <T> () (*i));
      }
      return hf;
    }
  };

  /**
   *  @brief Generic hash for an ordered set
   */
  template <class T>
  struct hash<std::set<T> >
  {
    size_t operator() (const std::set<T> &o) const
    {
      size_t hf = 0;
      for (typename std::set<T>::const_iterator i = o.begin (); i != o.end (); ++i) {
        hf = hfunc (hf, std::hash <T> () (*i));
      }
      return hf;
    }
  };
}

#endif
