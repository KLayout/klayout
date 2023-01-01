
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
  inline size_t hcombine (size_t h1, size_t h2)
  {
    return (h1 << 4) ^ (h1 >> 4) ^ h2;
  }

  template <class T>
  inline size_t hfunc (const T &t)
  {
    hash <T> hf;
    return hf (t);
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
    return hcombine (h, hfunc_coord (d));
  }

  /**
   *  @brief Hash value for a point
   */

  template <class C>
  size_t hfunc (const db::point<C> &o, size_t h)
  {
    return hfunc_coord (o.x (), hfunc_coord (o.y (), h));
  }

  template <class C>
  size_t hfunc (const db::point<C> &o)
  {
    return hfunc_coord (o.x (), hfunc_coord (o.y ()));
  }

  template <class C>
  struct hash <db::point<C> >
  {
    size_t operator() (const db::point<C> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Hash value for a vector
   */

  template <class C>
  size_t hfunc (const db::vector<C> &o, size_t h)
  {
    return hfunc_coord (o.x (), hfunc_coord (o.y (), h));
  }

  template <class C>
  size_t hfunc (const db::vector<C> &o)
  {
    return hfunc_coord (o.x (), hfunc_coord (o.y ()));
  }

  template <class C>
  struct hash <db::vector<C> >
  {
    size_t operator() (const db::vector<C> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Hash value for a box
   */

  template <class C>
  size_t hfunc (const db::box<C> &o, size_t h)
  {
    return hfunc (o.p1 (), hfunc (o.p2 (), h));
  }

  template <class C>
  size_t hfunc (const db::box<C> &o)
  {
    return hfunc (o.p1 (), hfunc (o.p2 ()));
  }

  template <class C>
  struct hash <db::box<C> >
  {
    size_t operator() (const db::box<C> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Hash value for an edge
   */

  template <class C>
  size_t hfunc (const db::edge<C> &o, size_t h)
  {
    return hfunc (o.p1 (), hfunc (o.p2 (), h));
  }

  template <class C>
  size_t hfunc (const db::edge<C> &o)
  {
    return hfunc (o.p1 (), hfunc (o.p2 ()));
  }

  template <class C>
  struct hash <db::edge<C> >
  {
    size_t operator() (const db::edge<C> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Hash value for an edge pair
   */

  template <class C>
  size_t hfunc (const db::edge_pair<C> &o, size_t h)
  {
    return hfunc (o.lesser (), hfunc (o.greater (), hfunc (int (o.is_symmetric ()), h)));
  }

  template <class C>
  size_t hfunc (const db::edge_pair<C> &o)
  {
    return hfunc (o.lesser (), hfunc (o.greater (), hfunc (int (o.is_symmetric ()))));
  }

  template <class C>
  struct hash <db::edge_pair<C> >
  {
    size_t operator() (const db::edge_pair<C> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Hash value for a text object
   */

  template <class C>
  size_t hfunc (const db::text<C> &o, size_t h)
  {
    h = hfunc (int (o.halign ()), h);
    h = hfunc (int (o.valign ()), h);
    h = hfunc (o.trans ().rot (), h);
    h = hfunc (o.trans ().disp (), h);
    //  NOTE: using std::string for the value makes sure the default hasher doesn't use the pointer value
    h = hfunc (hfunc (std::string (o.string ())), h);
    return h;
  }

  template <class C>
  size_t hfunc (const db::text<C> &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class C>
  struct hash <db::text<C> >
  {
    size_t operator() (const db::text<C> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Hash value for a path
   */

  template <class C>
  size_t hfunc (const db::path<C> &o, size_t h)
  {
    h = hfunc (int (o.round ()), h);
    h = hfunc_coord (o.bgn_ext (), h);
    h = hfunc_coord (o.end_ext (), h);
    h = hfunc_coord (o.width (), h);
    //  NOTE: using too many points for the hash function just slows down the code.
    unsigned int n = 20;
    for (typename db::path<C>::iterator p = o.begin (); p != o.end (); ++p) {
      if (--n == 0) {
        h = hfunc (o.points (), h);
        break;
      } else {
        h = hfunc (*p, h);
      }
    }
    return h;
  }

  template <class C>
  size_t hfunc (const db::path<C> &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class C>
  struct hash <db::path<C> >
  {
    size_t operator() (const db::path<C> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Hash value for a polygon contour
   */

  template <class C>
  size_t hfunc (const db::polygon_contour<C> &o, size_t h)
  {
    //  NOTE: using too many points for the hash function just slows down the code.
    unsigned int n = 20;
    for (typename db::polygon_contour<C>::simple_iterator i = o.begin (); i != o.end (); ++i) {
      if (--n == 0) {
        h = hfunc (o.size (), h);
        break;
      } else {
        h = hfunc (*i, h);
      }
    }
    return h;
  }

  template <class C>
  size_t hfunc (const db::polygon_contour<C> &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class C>
  struct hash <db::polygon_contour<C> >
  {
    size_t operator() (const db::path<C> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Hash value for a polygon
   */

  template <class C>
  size_t hfunc (const db::polygon<C> &o, size_t h)
  {
    h = hfunc (o.hull (), h);
    //  NOTE: using too many points for the hash function just slows down the code.
    unsigned int n = 20;
    for (size_t i = 0; i < o.holes (); ++i) {
      if (--n == 0) {
        h = hfunc (o.holes (), h);
        break;
      } else {
        h = hfunc (o.hole (int (i)), h);
      }
    }
    return h;
  }

  template <class C>
  size_t hfunc (const db::polygon<C> &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class C>
  struct hash <db::polygon<C> >
  {
    size_t operator() (const db::polygon<C> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Hash value for a simple polygon
   */

  template <class C>
  size_t hfunc (const db::simple_polygon<C> &o, size_t h)
  {
    return hfunc (o.hull (), h);
  }

  template <class C>
  size_t hfunc (const db::simple_polygon<C> &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class C>
  struct hash <db::simple_polygon<C> >
  {
    size_t operator() (const db::simple_polygon<C> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Hash value for a simple transformation
   */

  template <class C>
  size_t hfunc (const db::simple_trans<C> &t, size_t h)
  {
    return hfunc (int (t.rot ()), hfunc (t.disp (), h));
  }

  template <class C>
  size_t hfunc (const db::simple_trans<C> &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class C>
  struct hash <db::simple_trans<C> >
  {
    size_t operator() (const db::simple_trans<C> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief A hash function for a displacement transformation
   */

  template <class C>
  size_t hfunc (const db::disp_trans<C> &t, size_t h)
  {
    return hfunc (t.disp (), h);
  }

  template <class C>
  size_t hfunc (const db::disp_trans<C> &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class C>
  struct hash <db::disp_trans<C> >
  {
    size_t operator() (const db::disp_trans<C> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Hash value for a complex transformation
   */

  template <class I, class F, class R>
  size_t hfunc (const db::complex_trans<I, F, R> &t, size_t h)
  {
    h = hfunc (int64_t (0.5 + t.angle () / db::epsilon), h);
    h = hfunc (int64_t (0.5 + t.mag () / db::epsilon), h);
    h = hfunc (int (t.is_mirror ()), h);
    h = hfunc (t.disp (), h);
    return h;
  }

  template <class I, class F, class R>
  size_t hfunc (const db::complex_trans<I, F, R> &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class I, class F, class R>
  struct hash <db::complex_trans<I, F, R> >
  {
    size_t operator() (const db::complex_trans<I, F, R> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief A hash value for a db::CellInstArray and db::DCellInstArray
   */

  template <class C>
  size_t hfunc (const db::array <db::CellInst, db::simple_trans<C> > &o, size_t h)
  {
    h = hfunc (o.object ().cell_index (), h);

    db::vector<C> a, b;
    unsigned long na = 1, nb = 1;
    if (o.is_regular_array (a, b, na, nb)) {
      h = hfunc (a, h);
      h = hfunc (b, h);
      h = hfunc (na, h);
      h = hfunc (nb, h);
    } else if (o.size () > 1) {
      //  iterated array
      typename db::array <db::CellInst, db::simple_trans<C> >::iterator i = o.begin ();
      while (! (++i).at_end ()) {
        h = hfunc (*i, h);
      }
    }

    if (o.is_complex ()) {
      h = hfunc (o.complex_trans (), h);
    } else {
      h = hfunc (o.front (), h);
    }

    return h;
  }

  template <class C>
  size_t hfunc (const db::array <db::CellInst, db::simple_trans<C> > &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class C>
  struct hash <db::array <db::CellInst, db::simple_trans<C> > >
  {
    size_t operator() (const db::array <db::CellInst, db::simple_trans<C> > &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Hash value for an object with properties
   */

  template <class O>
  size_t hfunc (const db::object_with_properties<O> &o, size_t h)
  {
    return hfunc ((const O &) o, hfunc (o.properties_id (), h));
  }

  template <class O>
  size_t hfunc (const db::object_with_properties<O> &o)
  {
    return hfunc ((const O &) o, hfunc (o.properties_id ()));
  }

  template <class O>
  struct hash <db::object_with_properties<O> >
  {
    size_t operator() (const db::object_with_properties<O> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief A hash function for a shape reference
   */

  template <class Shape, class Trans>
  size_t hfunc (const db::shape_ref<Shape, Trans> &o, size_t h)
  {
    return hfunc (*o.ptr (), hfunc (o.trans (), h));
  }

  template <class Shape, class Trans>
  size_t hfunc (const db::shape_ref<Shape, Trans> &o)
  {
    return hfunc (*o.ptr (), hfunc (o.trans ()));
  }

  template <class Shape, class Trans>
  struct hash <db::shape_ref<Shape, Trans> >
  {
    size_t operator() (const db::shape_ref<Shape, Trans> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief A hash function for a polygon reference
   */

  template <class Shape, class Trans>
  size_t hfunc (const db::polygon_ref<Shape, Trans> &o, size_t h)
  {
    return hfunc (*o.ptr (), hfunc (o.trans (), h));
  }

  template <class Shape, class Trans>
  size_t hfunc (const db::polygon_ref<Shape, Trans> &o)
  {
    return hfunc (*o.ptr (), hfunc (o.trans ()));
  }

  template <class Shape, class Trans>
  struct hash <db::polygon_ref<Shape, Trans> >
  {
    size_t operator() (const db::polygon_ref<Shape, Trans> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief A hash function for a path reference
   */

  template <class Shape, class Trans>
  size_t hfunc (const db::path_ref<Shape, Trans> &o, size_t h)
  {
    return hfunc (*o.ptr (), hfunc (o.trans (), h));
  }

  template <class Shape, class Trans>
  size_t hfunc (const db::path_ref<Shape, Trans> &o)
  {
    return hfunc (*o.ptr (), hfunc (o.trans ()));
  }

  template <class Shape, class Trans>
  struct hash <db::path_ref<Shape, Trans> >
  {
    size_t operator() (const db::path_ref<Shape, Trans> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief A hash function for a text reference
   */

  template <class Shape, class Trans>
  size_t hfunc (const db::text_ref<Shape, Trans> &o, size_t h)
  {
    return hfunc (*o.ptr (), hfunc (o.trans (), h));
  }

  template <class Shape, class Trans>
  size_t hfunc (const db::text_ref<Shape, Trans> &o)
  {
    return hfunc (*o.ptr (), hfunc (o.trans ()));
  }

  template <class Shape, class Trans>
  struct hash <db::text_ref<Shape, Trans> >
  {
    size_t operator() (const db::text_ref<Shape, Trans> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief A hash value for a db::LayerProperties object
   */

  inline size_t hfunc (const db::LayerProperties &o, size_t h)
  {
    if (o.is_named ()) {
      return hfunc (o.name, h);
    } else {
      h = hfunc (o.layer, h);
      h = hfunc (o.datatype, h);
      h = hfunc (o.name, h);
      return h;
    }
  }

  inline size_t hfunc (const db::LayerProperties &o)
  {
    return hfunc (o, size_t (0));
  }

  template <>
  struct hash <db::LayerProperties>
  {
    size_t operator() (const db::LayerProperties &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Generic hash for a pair of objects
   */

  template <class T1, class T2>
  size_t hfunc (const std::pair <T1, T2> &o, size_t h)
  {
    return hfunc (o.first, hfunc (o.second, h));
  }

  template <class T1, class T2>
  size_t hfunc (const std::pair <T1, T2> &o)
  {
    return hfunc (o.first, hfunc (o.second));
  }

  template <class T1, class T2>
  struct hash <std::pair <T1, T2> > 
  {
    size_t operator() (const std::pair<T1, T2> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Generic hash for an unordered set
   */

  template <class T>
  size_t hfunc (const std::unordered_set <T> &o, size_t h)
  {
    for (typename std::unordered_set<T>::const_iterator i = o.begin (); i != o.end (); ++i) {
      h = hfunc (*i, h);
    }
    return h;
  }

  template <class T>
  size_t hfunc (const std::unordered_set <T> &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class T>
  struct hash <std::unordered_set <T> >
  {
    size_t operator() (const std::unordered_set<T> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Generic hash for an ordered set
   */

  template <class T>
  size_t hfunc (const std::set <T> &o, size_t h)
  {
    for (typename std::set<T>::const_iterator i = o.begin (); i != o.end (); ++i) {
      h = hfunc (*i, h);
    }
    return h;
  }

  template <class T>
  size_t hfunc (const std::set <T> &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class T>
  struct hash <std::set <T> >
  {
    size_t operator() (const std::set<T> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Generic hash for an unordered map
   */

  template <class T1, class T2>
  size_t hfunc (const std::unordered_map<T1, T2> &o, size_t h)
  {
    for (typename std::unordered_map<T1, T2>::const_iterator i = o.begin (); i != o.end (); ++i) {
      h = hfunc (i->first, hfunc (i->second, h));
    }
    return h;
  }

  template <class T1, class T2>
  size_t hfunc (const std::unordered_map<T1, T2> &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class T1, class T2>
  struct hash <std::unordered_map<T1, T2> >
  {
    size_t operator() (const std::unordered_map<T1, T2> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Generic hash for an ordered map
   */

  template <class T1, class T2>
  size_t hfunc (const std::map<T1, T2> &o, size_t h)
  {
    for (typename std::map<T1, T2>::const_iterator i = o.begin (); i != o.end (); ++i) {
      h = hfunc (i->first, hfunc (i->second, h));
    }
    return h;
  }

  template <class T1, class T2>
  size_t hfunc (const std::map<T1, T2> &o)
  {
    return hfunc (o, size_t (0));
  }

  template <class T1, class T2>
  struct hash <std::map<T1, T2> >
  {
    size_t operator() (const std::map<T1, T2> &o) const
    {
      return hfunc (o);
    }
  };

  /**
   *  @brief Create a pointer hash from the pointer's value
   */
  template <class X>
  struct ptr_hash_from_value
  {
    size_t operator() (const X *ptr) const
    {
      return ptr ? hash<X> () (*ptr) : 0;
    }
  };

}

#endif
