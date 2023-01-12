
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


#ifndef HDR_dbShape
#define HDR_dbShape

#include "dbCommon.h"

#include "tlException.h"
#include "tlReuseVector.h"
#include "dbManager.h"
#include "dbPolygon.h"
#include "dbPath.h"
#include "dbEdge.h"
#include "dbEdgePair.h"
#include "dbText.h"
#include "dbBox.h"
#include "dbBoxConvert.h"
#include "dbUserObject.h"
#include "dbArray.h"
#include "dbObjectWithProperties.h"

#include <set>

namespace db 
{

class Shapes;

/**
 *  @brief A generic edge iterator 
 *
 *  This edge iterator is used to provide edge iterators for simple and
 *  complex polygons as well.
 *  The implementation is based on a union scheme. It assumes that the
 *  iterators represented by the generic instance do not require a special
 *  copy semantics but can be copied by bytewise copy simply.
 */
template <class C>
class generic_polygon_edge_iterator
{
public:
  typedef C coord_type;
  typedef db::edge<C> edge_type;
  typedef db::disp_trans<C> disp_type;
  typedef db::polygon<C> polygon_type;
  typedef typename polygon_type::polygon_edge_iterator polygon_edge_iterator_type;
  typedef db::simple_polygon<C> simple_polygon_type;
  typedef typename simple_polygon_type::polygon_edge_iterator simple_polygon_edge_iterator_type;
  typedef db::polygon_ref<polygon_type, disp_type> polygon_ref_type;
  typedef typename polygon_ref_type::polygon_edge_iterator polygon_ref_edge_iterator_type;
  typedef db::polygon_ref<simple_polygon_type, disp_type> simple_polygon_ref_type;
  typedef typename simple_polygon_ref_type::polygon_edge_iterator simple_polygon_ref_edge_iterator_type;
  typedef edge_type value_type;
  typedef void pointer;
  typedef value_type reference; 
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef void difference_type;

  enum iterator_type
  {
    None, SimplePolygon, SimplePolygonRef, Polygon, PolygonRef
  };

  /**
   *  @brief Default ctor
   */
  generic_polygon_edge_iterator ()
  {
    m_type = None;
  }
    
  /**
   *  @brief This constructor creates an edge iterator at "begin" for polygons
   * 
   *  The iterator stores begin and end and provides a "at_end" method to
   *  test if the sequence is at the end.
   *
   *  @param begin The start of the sequence
   */
  generic_polygon_edge_iterator (const polygon_edge_iterator_type &begin)
  {
    m_type = Polygon;
    new ((char *) m_d.iter) polygon_edge_iterator_type (begin);
  }
    
  /**
   *  @brief This constructor creates an edge iterator at "begin" for polygon references
   * 
   *  See former ctor.
   */
  generic_polygon_edge_iterator (const polygon_ref_edge_iterator_type &begin)
  {
    m_type = PolygonRef;
    new ((char *) m_d.iter) polygon_ref_edge_iterator_type (begin);
  }
    
  /**
   *  @brief This constructor creates an edge iterator at "begin" for simple polygons
   * 
   *  See former ctor.
   */
  generic_polygon_edge_iterator (const simple_polygon_edge_iterator_type &begin)
  {
    m_type = SimplePolygon;
    new ((char *) m_d.iter) simple_polygon_edge_iterator_type (begin);
  }
    
  /**
   *  @brief This constructor creates an edge iterator at "begin" for simple polygon references
   * 
   *  See former ctor.
   */
  generic_polygon_edge_iterator (const simple_polygon_ref_edge_iterator_type &begin)
  {
    m_type = SimplePolygonRef;
    new ((char *) m_d.iter) simple_polygon_ref_edge_iterator_type (begin);
  }
    
  /**
   *  @brief at_end predicate
   */
  bool at_end () const
  {
    return generic_const_f<bool, at_end_f> ();
  }

  /**
   *  @brief contour attribute
   */
  unsigned int contour () const
  {
    return generic_const_f<unsigned int, contour_f> ();
  }

  /**
   *  @brief Increment operator
   */
  generic_polygon_edge_iterator &operator++ () 
  {
    generic_f<bool, inc_f> ();
    return *this;
  }

  /**
   *  @brief Decrement operator
   */
  generic_polygon_edge_iterator &operator-- () 
  {
    generic_f<bool, dec_f> ();
    return *this;
  }

  /**
   *  @brief access operator
   */
  edge_type operator* () const
  {
    return generic_const_f<edge_type, deref_f> ();
  }
  
private:
  //  This union is there to compute the maximum size required to hold all
  //  kind of iterators 
  union iter_size {
    char sz1 [sizeof (polygon_edge_iterator_type)];
    char sz2 [sizeof (polygon_ref_edge_iterator_type)];
    char sz3 [sizeof (simple_polygon_edge_iterator_type)];
    char sz4 [sizeof (simple_polygon_ref_edge_iterator_type)];
  };

  //  This member must be first to guarantee alignment on 64bit systems:
  //  The strange construction and the local dummy class helps to guarantee alignment of the "iter" space
  union {
    class _align_helper { long l; } _ah;
    //  Hint: without the sizeof(size_t) offset, gcc 4.4.1 produces invalid iterators for the edge iterator.
    //  For example, these iterators do not correctly stop but produce more edges than are available.
    char iter [sizeof (iter_size) + sizeof (size_t)];
  } m_d;

  iterator_type m_type;

  struct contour_f {

    unsigned int operator() () const
    {
      return 0;
    }

    template <class Iter>
    unsigned int operator() (const Iter &iter) const
    {
      return iter.contour ();
    } 

  };

  struct at_end_f {

    bool operator() () const
    {
      return true;
    }

    template <class Iter>
    bool operator() (const Iter &iter) const
    {
      return iter.at_end ();
    } 

  };

  struct inc_f {

    bool operator() () const
    {
      return false;
    }

    template <class Iter>
    bool operator() (Iter &iter) const
    {
      iter.operator++ ();
      return false;
    }

  };

  struct dec_f {

    bool operator() () const
    {
      return false;
    }

    template <class Iter>
    bool operator() (Iter &iter) const
    {
      iter.operator-- ();
      return false;
    } 

  };

  struct deref_f {

    edge_type operator() () const
    {
      return edge_type ();
    }

    template <class Iter>
    edge_type operator() (const Iter &iter) const
    {
      return iter.operator* ();
    }

  };

  struct at_begin_f {

    bool operator() () const
    {
      return true;
    }

    template <class Iter>
    bool operator() (const Iter &iter) const
    {
      return iter.at_begin ();
    }

  };

  template <class Ret, class Func>
  Ret generic_const_f () const
  {
    Func f;
    if (m_type == Polygon) {
      return f (*((const polygon_edge_iterator_type *) m_d.iter));
    } else if (m_type == PolygonRef) {
      return f (*((const polygon_ref_edge_iterator_type *) m_d.iter));
    } else if (m_type == SimplePolygon) {
      return f (*((const simple_polygon_edge_iterator_type *) m_d.iter));
    } else if (m_type == SimplePolygonRef) {
      return f (*((const simple_polygon_ref_edge_iterator_type *) m_d.iter));
    } else {
      return f ();
    }
  }

  template <class Ret, class Func>
  Ret generic_f () 
  {
    Func f;
    if (m_type == Polygon) {
      return f (*((polygon_edge_iterator_type *) m_d.iter));
    } else if (m_type == PolygonRef) {
      return f (*((polygon_ref_edge_iterator_type *) m_d.iter));
    } else if (m_type == SimplePolygon) {
      return f (*((simple_polygon_edge_iterator_type *) m_d.iter));
    } else if (m_type == SimplePolygonRef) {
      return f (*((simple_polygon_ref_edge_iterator_type *) m_d.iter));
    } else {
      return f ();
    }
  }
};

/**
 *  @brief A generic point iterator 
 *
 *  This point iterator is used to provide point iterators for simple and
 *  complex polygons as well as paths.
 *  The implementation is based on a union scheme. It assumes that the
 *  iterators represented by the generic instance do not require a special
 *  copy semantics but can be copied by bytewise copy simply.
 */
template <class C>
class generic_point_iterator
{
public:
  typedef C coord_type;
  typedef db::point<C> point_type;
  typedef db::polygon<C> polygon_type;
  typedef db::disp_trans<C> disp_type;
  typedef typename polygon_type::polygon_contour_iterator polygon_point_iterator_type;
  typedef db::polygon_ref<polygon_type, disp_type> polygon_ref_type;
  typedef typename polygon_ref_type::polygon_contour_iterator polygon_ref_point_iterator_type;
  typedef db::path<C> path_type;
  typedef typename path_type::iterator path_point_iterator_type;
  typedef db::path_ref<path_type, disp_type> path_ref_type;
  typedef typename path_ref_type::iterator path_ref_point_iterator_type;
  typedef point_type value_type;
  typedef void pointer;
  typedef value_type reference; 
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef void difference_type;

  enum iterator_type
  {
    Polygon, PolygonRef, Path, PathRef
  };

  /**
   *  @brief This constructor creates an point iterator at "begin" for polygons
   * 
   *  The iterator stores begin and end and provides a "at_end" method to
   *  test if the sequence is at the end.
   *  This method also applies to simple polygon contours.
   *
   *  @param begin The start of the sequence
   */
  generic_point_iterator (const polygon_point_iterator_type &begin)
  {
    m_type = Polygon;
    new ((char *) m_d.iter) polygon_point_iterator_type (begin);
  }
    
  /**
   *  @brief This constructor creates an point iterator at "begin" for polygon references
   * 
   *  This method also applies to simple polygon references.
   *  See former ctor.
   */
  generic_point_iterator (const polygon_ref_point_iterator_type &begin)
  {
    m_type = PolygonRef;
    new ((char *) m_d.iter) polygon_ref_point_iterator_type (begin);
  }
    
  /**
   *  @brief This constructor creates an point iterator at "begin" for paths
   * 
   *  See former ctor.
   */
  generic_point_iterator (const path_point_iterator_type &begin)
  {
    m_type = Path;
    new ((char *) m_d.iter) path_point_iterator_type (begin);
  }
    
  /**
   *  @brief This constructor creates an point iterator at "begin" for path references
   * 
   *  See former ctor.
   */
  generic_point_iterator (const path_ref_point_iterator_type &begin)
  {
    m_type = PathRef;
    new ((char *) m_d.iter) path_ref_point_iterator_type (begin);
  }
    
  /**
   *  @brief at_end predicate
   */
  bool at_end () const
  {
    return generic_const_f<bool, at_end_f> ();
  }

  /**
   *  @brief Increment operator
   */
  generic_point_iterator &operator++ () 
  {
    generic_f<bool, inc_f> ();
    return *this;
  }

  /**
   *  @brief Decrement operator
   */
  generic_point_iterator &operator-- () 
  {
    generic_f<bool, dec_f> ();
    return *this;
  }

  /**
   *  @brief access operator
   */
  point_type operator* () const
  {
    return generic_const_f<point_type, deref_f> ();
  }

  /**
   *  @brief Sorting order 
   */
  bool operator< (const generic_point_iterator &d) const
  {
    if (m_type != d.m_type) {
      return m_type < d.m_type;
    }

    if (m_type == Polygon) {
      return *((const polygon_point_iterator_type *) m_d.iter) < *((const polygon_point_iterator_type *) d.m_d.iter);
    } else if (m_type == PolygonRef) {
      return *((const polygon_ref_point_iterator_type *) m_d.iter) < *((const polygon_ref_point_iterator_type *) d.m_d.iter);
    } else if (m_type == Path) {
      return *((const path_point_iterator_type *) m_d.iter) < *((const path_point_iterator_type *) d.m_d.iter);
    } else {
      return *((const path_ref_point_iterator_type *) m_d.iter) < *((const path_ref_point_iterator_type *) d.m_d.iter);
    }
  }

  /**
   *  @brief Equality 
   */
  bool operator== (const generic_point_iterator &d) const
  {
    tl_assert (m_type == d.m_type);

    if (m_type == Polygon) {
      return *((const polygon_point_iterator_type *) m_d.iter) == *((const polygon_point_iterator_type *) d.m_d.iter);
    } else if (m_type == PolygonRef) {
      return *((const polygon_ref_point_iterator_type *) m_d.iter) == *((const polygon_ref_point_iterator_type *) d.m_d.iter);
    } else if (m_type == Path) {
      return *((const path_point_iterator_type *) m_d.iter) == *((const path_point_iterator_type *) d.m_d.iter);
    } else {
      return *((const path_ref_point_iterator_type *) m_d.iter) == *((const path_ref_point_iterator_type *) d.m_d.iter);
    }
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const generic_point_iterator &d) const
  {
    return !operator== (d);
  }
  
private:
  //  This union is there to compute the maximum size required to hold all
  //  kind of iterators 
  union iter_size {
    char sz1 [sizeof (polygon_point_iterator_type)];
    char sz2 [sizeof (polygon_ref_point_iterator_type)];
    char sz3 [sizeof (path_point_iterator_type)];
    char sz4 [sizeof (path_ref_point_iterator_type)];
  };

  //  This member must be first to guarantee alignment on 64bit systems:
  //  The strange construction and the local dummy class helps to guarantee alignment of the "iter" space
  union {
    class _align_helper { long l; } _ah;
    //  Hint: without the sizeof(size_t) offset, gcc 4.4.1 produces invalid iterators for the edge iterator.
    //  For safety the offset is added here as well:
    char iter [sizeof (iter_size) + sizeof (size_t)];
  } m_d;

  iterator_type m_type;

  struct at_end_f {
    template <class Iter>
    bool operator() (const Iter &iter) const
    {
      return iter.at_end ();
    } 
  };

  struct inc_f {
    template <class Iter>
    bool operator() (Iter &iter) const
    {
      iter.operator++ ();
      return false;
    } 
  };

  struct dec_f {
    template <class Iter>
    bool operator() (Iter &iter) const
    {
      iter.operator-- ();
      return false;
    } 
  };

  struct deref_f {
    template <class Iter>
    point_type operator() (const Iter &iter) const
    {
      return iter.operator* ();
    } 
  };

  struct at_begin_f {
    template <class Iter>
    bool operator() (const Iter &iter) const
    {
      return iter.at_begin ();
    } 
  };

  template <class Ret, class Func>
  Ret generic_const_f () const
  {
    Func f;
    if (m_type == Polygon) {
      return f (*((const polygon_point_iterator_type *) m_d.iter));
    } else if (m_type == PolygonRef) {
      return f (*((const polygon_ref_point_iterator_type *) m_d.iter));
    } else if (m_type == Path) {
      return f (*((const path_point_iterator_type *) m_d.iter));
    } else {
      return f (*((const path_ref_point_iterator_type *) m_d.iter));
    }
  }

  template <class Ret, class Func>
  Ret generic_f () 
  {
    Func f;
    if (m_type == Polygon) {
      return f (*((polygon_point_iterator_type *) m_d.iter));
    } else if (m_type == PolygonRef) {
      return f (*((polygon_ref_point_iterator_type *) m_d.iter));
    } else if (m_type == Path) {
      return f (*((path_point_iterator_type *) m_d.iter));
    } else {
      return f (*((path_ref_point_iterator_type *) m_d.iter));
    }
  }
};

/**
 *  @brief A shape proxy 
 *
 *  The shape proxy is basically a pointer to a shape of different kinds 
 *  No copy of the shape is created: if the shape proxy is copied the copy still
 *  points to the original shape. If the original shape is modified or deleted,
 *  the shape proxy will also point to a modified or invalid shape.
 *  The proxy can be "null" which means a invalid reference.
 */ 
class DB_PUBLIC Shape
{
public:
  typedef db::Coord coord_type;
  typedef coord_traits<coord_type>::area_type area_type; 
  typedef coord_traits<coord_type>::distance_type distance_type; 
  typedef coord_traits<coord_type>::perimeter_type perimeter_type; 
  typedef db::simple_trans<coord_type> trans_type;
  typedef db::disp_trans<coord_type> disp_type;
  typedef db::unit_trans<coord_type> unit_trans_type;
  typedef db::polygon<coord_type> polygon_type;
  typedef db::simple_polygon<coord_type> simple_polygon_type;
  typedef db::polygon_ref<polygon_type, disp_type> polygon_ref_type;
  typedef db::polygon_ref<polygon_type, unit_trans_type> polygon_ptr_type;
  typedef db::array<polygon_ptr_type, disp_type> polygon_ptr_array_type;
  typedef db::polygon_ref<simple_polygon_type, disp_type> simple_polygon_ref_type;
  typedef db::polygon_ref<simple_polygon_type, unit_trans_type> simple_polygon_ptr_type;
  typedef db::array<simple_polygon_ptr_type, disp_type> simple_polygon_ptr_array_type;
  typedef db::path<coord_type> path_type;
  typedef db::path_ref<path_type, disp_type> path_ref_type;
  typedef db::path_ref<path_type, unit_trans_type> path_ptr_type;
  typedef db::array<path_ptr_type, disp_type> path_ptr_array_type;
  typedef db::edge<coord_type> edge_type;
  typedef db::edge_pair<coord_type> edge_pair_type;
  typedef db::text<coord_type> text_type;
  typedef db::text_ref<text_type, disp_type> text_ref_type;
  typedef db::text_ref<text_type, unit_trans_type> text_ptr_type;
  typedef db::array<text_ptr_type, disp_type> text_ptr_array_type;
  typedef db::user_object<coord_type> user_object_type;
  typedef db::box<coord_type> box_type;
  typedef db::array<box_type, unit_trans_type> box_array_type;
  typedef db::box<coord_type, coord_traits<coord_type>::short_coord_type> short_box_type;
  typedef db::array<short_box_type, unit_trans_type> short_box_array_type;
  typedef db::point<coord_type> point_type;
  typedef db::generic_polygon_edge_iterator<coord_type> polygon_edge_iterator;
  typedef db::generic_point_iterator<coord_type> point_iterator;

  typedef tl::reuse_vector<db::polygon<coord_type> >::const_iterator polygon_iter_type;
  typedef tl::reuse_vector<db::simple_polygon<coord_type> >::const_iterator simple_polygon_iter_type;
  typedef tl::reuse_vector<db::polygon_ref<polygon_type, disp_type> >::const_iterator polygon_ref_iter_type;
  typedef tl::reuse_vector<db::polygon_ref<polygon_type, unit_trans_type> >::const_iterator polygon_ptr_iter_type;
  typedef tl::reuse_vector<db::array<polygon_ptr_type, disp_type> >::const_iterator polygon_ptr_array_iter_type;
  typedef tl::reuse_vector<db::polygon_ref<simple_polygon_type, disp_type> >::const_iterator simple_polygon_ref_iter_type;
  typedef tl::reuse_vector<db::polygon_ref<simple_polygon_type, unit_trans_type> >::const_iterator simple_polygon_ptr_iter_type;
  typedef tl::reuse_vector<db::array<simple_polygon_ptr_type, disp_type> >::const_iterator simple_polygon_ptr_array_iter_type;
  typedef tl::reuse_vector<db::path<coord_type> >::const_iterator path_iter_type;
  typedef tl::reuse_vector<db::path_ref<path_type, disp_type> >::const_iterator path_ref_iter_type;
  typedef tl::reuse_vector<db::path_ref<path_type, unit_trans_type> >::const_iterator path_ptr_iter_type;
  typedef tl::reuse_vector<db::array<path_ptr_type, disp_type> >::const_iterator path_ptr_array_iter_type;
  typedef tl::reuse_vector<db::edge<coord_type> >::const_iterator edge_iter_type;
  typedef tl::reuse_vector<db::edge_pair<coord_type> >::const_iterator edge_pair_iter_type;
  typedef tl::reuse_vector<db::point<coord_type> >::const_iterator point_iter_type;
  typedef tl::reuse_vector<db::text<coord_type> >::const_iterator text_iter_type;
  typedef tl::reuse_vector<db::text_ref<text_type, disp_type> >::const_iterator text_ref_iter_type;
  typedef tl::reuse_vector<db::text_ref<text_type, unit_trans_type> >::const_iterator text_ptr_iter_type;
  typedef tl::reuse_vector<db::array<text_ptr_type, disp_type> >::const_iterator text_ptr_array_iter_type;
  typedef tl::reuse_vector<db::user_object<coord_type> >::const_iterator user_object_iter_type;
  typedef tl::reuse_vector<db::box<coord_type> >::const_iterator box_iter_type;
  typedef tl::reuse_vector<db::array<box_type, unit_trans_type> >::const_iterator box_array_iter_type;
  typedef tl::reuse_vector<db::box<coord_type, coord_traits<coord_type>::short_coord_type> >::const_iterator short_box_iter_type;
  typedef tl::reuse_vector<db::array<short_box_type, unit_trans_type> >::const_iterator short_box_array_iter_type;

  typedef tl::reuse_vector<db::object_with_properties<db::polygon<coord_type> > >::const_iterator ppolygon_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::simple_polygon<coord_type> > >::const_iterator psimple_polygon_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::polygon_ref<polygon_type, disp_type> > >::const_iterator ppolygon_ref_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::polygon_ref<polygon_type, unit_trans_type> > >::const_iterator ppolygon_ptr_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::array<polygon_ptr_type, disp_type> > >::const_iterator ppolygon_ptr_array_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::polygon_ref<simple_polygon_type, disp_type> > >::const_iterator psimple_polygon_ref_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::polygon_ref<simple_polygon_type, unit_trans_type> > >::const_iterator psimple_polygon_ptr_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::array<simple_polygon_ptr_type, disp_type> > >::const_iterator psimple_polygon_ptr_array_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::path<coord_type> > >::const_iterator ppath_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::path_ref<path_type, disp_type> > >::const_iterator ppath_ref_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::path_ref<path_type, unit_trans_type> > >::const_iterator ppath_ptr_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::array<path_ptr_type, disp_type> > >::const_iterator ppath_ptr_array_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::edge<coord_type> > >::const_iterator pedge_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::edge_pair<coord_type> > >::const_iterator pedge_pair_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::point<coord_type> > >::const_iterator ppoint_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::text<coord_type> > >::const_iterator ptext_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::text_ref<text_type, disp_type> > >::const_iterator ptext_ref_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::text_ref<text_type, unit_trans_type> > >::const_iterator ptext_ptr_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::array<text_ptr_type, disp_type> > >::const_iterator ptext_ptr_array_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::user_object<coord_type> > >::const_iterator puser_object_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::box<coord_type> > >::const_iterator pbox_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::array<box_type, unit_trans_type> > >::const_iterator pbox_array_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::box<coord_type, coord_traits<coord_type>::short_coord_type> > >::const_iterator pshort_box_iter_type;
  typedef tl::reuse_vector<db::object_with_properties<db::array<short_box_type, unit_trans_type> > >::const_iterator pshort_box_array_iter_type;

  //  Hint: in the following enum it is important that the ArrayMember types come *after*
  //  the non-member types. See db::shapes::erase with multi-shape erase capabilities.
  enum object_type {

    Null,

    Polygon,
    PolygonRef,
    PolygonPtrArray,
    PolygonPtrArrayMember,
    SimplePolygon,
    SimplePolygonRef,
    SimplePolygonPtrArray,
    SimplePolygonPtrArrayMember,
    Edge,
    EdgePair,
    Path,
    PathRef,
    PathPtrArray,
    PathPtrArrayMember,
    Box,
    BoxArray,
    BoxArrayMember,
    ShortBox,
    ShortBoxArray,
    ShortBoxArrayMember,
    Text,
    TextRef,
    TextPtrArray,
    TextPtrArrayMember,
    Point,
    UserObject
  };
    
  /** 
   *  @brief Construct a shape proxy as a null object
   */
  Shape ()
    : mp_shapes (0), m_with_props (false), m_stable (false), m_type (Null)
  {
    m_generic.any = 0;
  }

  /**
   *  @brief Construct as a proxy to a certain object
   */
  template <class Obj>
  Shape (const db::Shapes *shapes, const Obj &obj)
    : mp_shapes (const_cast<db::Shapes *> (shapes)), m_with_props (false), m_stable (false)
  {
    typename Obj::tag tag = typename Obj::tag ();
    init (tag);
    m_generic.any = &obj;
  }

  /**
   *  @brief Construct as a proxy to a certain object
   */
  template <class Obj>
  Shape (db::Shapes *shapes, const Obj &obj)
    : mp_shapes (shapes), m_with_props (false), m_stable (false)
  {
    typename Obj::tag tag = typename Obj::tag ();
    init (tag);
    m_generic.any = &obj;
  }

  /**
   *  @brief Construct as a proxy to a certain object
   *
   *  This form will copy the object itself. A temporary object may be passed to "obj".
   *  An implementation is provided only for the array types
   *
   *  Hint: there is no "const shape reference" currently and having one would mean
   *  we would probably create too much constness. Hence we use const_cast to convert it
   *  to a non-const one.
   */
  template <class Obj, class Trans>
  Shape (const db::Shapes *shapes, const Obj &obj, const Trans &trans)
    : mp_shapes (const_cast<db::Shapes *> (shapes)), m_with_props (false), m_stable (false)
  {
    typename Obj::tag tag = typename Obj::tag ();
    init_member (tag, trans_type (trans));
    m_generic.any = &obj;
  }

  /**
   *  @brief Construct as a proxy to a certain object
   *
   *  This form will copy the object itself. A temporary object may be passed to "obj".
   *  An implementation is provided only for the array types
   */
  template <class Obj, class Trans>
  Shape (db::Shapes *shapes, const Obj &obj, const Trans &trans)
    : mp_shapes (shapes), m_with_props (false), m_stable (false)
  {
    typename Obj::tag tag = typename Obj::tag ();
    init_member (tag, trans_type (trans));
    m_generic.any = &obj;
  }

  /**
   *  @brief Construct as a proxy to a certain object given by an iterator
   *
   *  Hint: there is no "const shape reference" currently and having one would mean
   *  we would probably create too much constness. Hence we use const_cast to convert it
   *  to a non-const one.
   */
  template <class Obj, bool R>
  Shape (const db::Shapes *shapes, const tl::reuse_vector_const_iterator<Obj, R> &tree_iter)
    : mp_shapes (const_cast<db::Shapes *> (shapes)), m_with_props (false), m_stable (true)
  {
    typename Obj::tag tag = typename Obj::tag ();
    //  this sets the type and potentially m_with_props:
    init (tag);
    //  HINT: we assume that we do not need an destructor for that iterator ..
    new ((char *) m_generic.iter) typename tl::reuse_vector<Obj>::const_iterator (tree_iter);
  }

  /**
   *  @brief Construct as a proxy to a certain object given by an iterator
   */
  template <class Obj, bool R>
  Shape (db::Shapes *shapes, const tl::reuse_vector_const_iterator<Obj, R> &tree_iter)
    : mp_shapes (shapes), m_with_props (false), m_stable (true)
  {
    typename Obj::tag tag = typename Obj::tag ();
    //  this sets the type and potentially m_with_props:
    init (tag);
    //  HINT: we assume that we do not need an destructor for that iterator ..
    new ((char *) m_generic.iter) typename tl::reuse_vector<Obj>::const_iterator (tree_iter);
  }

  /**
   *  @brief Construct as a proxy to a certain object given by an iterator
   *
   *  Hint: there is no "const shape reference" currently and having one would mean
   *  we would probably create too much constness. Hence we use const_cast to convert it
   *  to a non-const one.
   */
  template <class Obj, bool R, class Trans>
  Shape (const db::Shapes *shapes, const tl::reuse_vector_const_iterator<Obj, R> &tree_iter, const Trans &trans)
    : mp_shapes (const_cast<db::Shapes *> (shapes)), m_with_props (false), m_stable (true)
  {
    typename Obj::tag tag = typename Obj::tag ();
    //  this sets the type and potentially m_with_props:
    init_member (tag, trans_type (trans));
    //  HINT: we assume that we do not need an destructor for that iterator ..
    new ((char *) m_generic.iter) typename tl::reuse_vector<Obj>::const_iterator (tree_iter);
  }

  /**
   *  @brief Construct as a proxy to a certain object given by an iterator
   */
  template <class Obj, bool R, class Trans>
  Shape (db::Shapes *shapes, const tl::reuse_vector_const_iterator<Obj, R> &tree_iter, const Trans &trans)
    : mp_shapes (shapes), m_with_props (false), m_stable (true)
  {
    typename Obj::tag tag = typename Obj::tag ();
    //  this sets the type and potentially m_with_props:
    init_member (tag, trans_type (trans));
    //  HINT: we assume that we do not need an destructor for that iterator ..
    new ((char *) m_generic.iter) typename tl::reuse_vector<Obj>::const_iterator (tree_iter);
  }

  /**
   *  @brief Returns the shape container this reference points into
   *
   *  The returned value can be 0 indicating that there is no container.
   */
  db::Shapes *shapes () const
  {
    return mp_shapes;
  }

  /** 
   *  @brief Construct a shape proxy as a shape with properties
   *
   *  This forwards the initialisation to the basic shape type.
   */
  template <class Sh>
  void init (db::object_tag< db::object_with_properties<Sh> >)
  {
    typename Sh::tag basic_tag = typename Sh::tag ();
    init (basic_tag);
    m_with_props = true;
  }

  /** 
   *  @brief Construct a shape proxy as a shape with properties for an array member
   *
   *  This form will copy the object itself. A temporary object may be passed to "obj".
   *  An implementation is provided only for a few types.
   *  This forwards the initialisation to the basic shape type.
   */
  template <class Sh, class Trans>
  void init_member (db::object_tag< db::object_with_properties<Sh> >, const Trans &trans)
  {
    typename Sh::tag tag = typename Sh::tag ();
    init_member (tag, trans_type (trans));
    m_with_props = true;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a polygon
   */
  void init (polygon_type::tag)
  {
    m_type = Polygon;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a polygon reference
   */
  void init (polygon_ref_type::tag)
  {
    m_type = PolygonRef;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a polygon array member
   */
  void init_member (polygon_ptr_array_type::tag, const trans_type &trans)
  {
    m_trans = trans;
    m_type = PolygonPtrArrayMember;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a polygon reference array
   */
  void init (polygon_ptr_array_type::tag)
  {
    m_type = PolygonPtrArray;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a simple polygon
   */
  void init (simple_polygon_type::tag)
  {
    m_type = SimplePolygon;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a polygon reference
   */
  void init (simple_polygon_ref_type::tag)
  {
    m_type = SimplePolygonRef;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a simple polygon array member
   */
  void init_member (simple_polygon_ptr_array_type::tag, const trans_type &trans)
  {
    m_trans = trans;
    m_type = SimplePolygonPtrArrayMember;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a simple polygon reference array
   */
  void init (simple_polygon_ptr_array_type::tag)
  {
    m_type = SimplePolygonPtrArray;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a path
   */
  void init (path_type::tag)
  {
    m_type = Path;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a path reference
   */
  void init (path_ref_type::tag)
  {
    m_type = PathRef;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a path array member
   */
  void init_member (path_ptr_array_type::tag, const trans_type &trans)
  {
    m_trans = trans;
    m_type = PathPtrArrayMember;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a path reference array
   */
  void init (path_ptr_array_type::tag)
  {
    m_type = PathPtrArray;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a text
   */
  void init (text_type::tag)
  {
    m_type = Text;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a text reference
   */
  void init (text_ref_type::tag)
  {
    m_type = TextRef;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a path array member
   */
  void init_member (text_ptr_array_type::tag, const trans_type &trans)
  {
    m_trans = trans;
    m_type = TextPtrArrayMember;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a text reference array
   */
  void init (text_ptr_array_type::tag)
  {
    m_type = TextPtrArray;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a edge
   */
  void init (edge_type::tag)
  {
    m_type = Edge;
  }

  /**
   *  @brief Construct a shape proxy as a reference to a edge pair
   */
  void init (edge_pair_type::tag)
  {
    m_type = EdgePair;
  }

  /**
   *  @brief Construct a shape proxy as a reference to a point
   */
  void init (point_type::tag)
  {
    m_type = Point;
  }

  /**
   *  @brief Construct a shape proxy as a reference to a box
   */
  void init (box_type::tag)
  {
    m_type = Box;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a box array member
   */
  void init_member (box_array_type::tag, const trans_type &trans)
  {
    m_trans = trans;
    m_type = BoxArrayMember;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a box array
   */
  void init (box_array_type::tag)
  {
    m_type = BoxArray;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a small box
   */
  void init (short_box_type::tag)
  {
    m_type = ShortBox;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a box array member
   */
  void init_member (short_box_array_type::tag, const trans_type &trans)
  {
    m_trans = trans;
    m_type = ShortBoxArrayMember;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a box array
   */
  void init (short_box_array_type::tag)
  {
    m_type = ShortBoxArray;
  }

  /** 
   *  @brief Construct a shape proxy as a reference to a user object
   */
  void init (user_object_type::tag)
  {
    m_type = UserObject;
  }

  /**
   *  @brief Determine, if the shape is of "object_with_properties" type.
   *
   *  Usually, the properties id (see prop_id()) should be used.
   *
   *  @return true, if the shape is of "object_with_properties<Sh> type".
   */
  bool with_props () const
  {
    return m_with_props;
  }

  /**
   *  @brief Get the properties Id associated with the shape
   */
  db::properties_id_type prop_id () const;

  /**
   *  @brief Check, if the shape is associated with a properties Id
   */
  bool has_prop_id () const
  {
    return m_with_props;
  }

  /**
   *  @brief Create the "begin" point iterator
   *
   *  This method applies to paths.
   */
  point_iterator begin_point () const;

  /**
   *  @brief Create the "end" point iterator
   *
   *  This method applies to paths.
   */
  point_iterator end_point () const;

  /**
   *  @brief Create the "begin" hull point iterator
   *
   *  This method applies to polygons.
   */
  point_iterator begin_hull () const;

  /**
   *  @brief Create the "end" hull point iterator
   *
   *  This method applies to polygons.
   */
  point_iterator end_hull () const;

  /**
   *  @brief Create the "begin" hole point iterator
   *
   *  This method applies to polygons.
   *  Simple polygons deliver an empty sequence.
   *
   *  @param hole The hole index (see holes () method)
   */
  point_iterator begin_hole (unsigned int hole) const;

  /**
   *  @brief Create the "end" hole point iterator
   *
   *  This method applies to polygons.
   *  Simple polygons deliver an empty sequence.
   *
   *  @param hole The hole index (see holes () method)
   */
  point_iterator end_hole (unsigned int hole) const;

  /**
   *  @brief Return the number of holes
   *
   *  This method applies to polygons.
   *  Simple polygons deliver a zero value.
   */
  unsigned int holes () const;

  /**
   *  @brief Create and return an edge iterator
   */
  polygon_edge_iterator begin_edge () const;

  /**
   *  @brief Create and return an edge iterator for the given contour
   *  If the object is a polygon, a contour of 0 will deliver the 
   *  edges of the hull and higher contours will deliver the edges of holes.
   */
  polygon_edge_iterator begin_edge (unsigned int c) const;

  /** 
   *  @brief Return the type of the shape reference
   */
  object_type type () const
  {
    return m_type;
  }

  /** 
   *  @brief Test if the shape proxy is a null object
   */
  bool is_null () const
  {
    return m_type == Null;
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const polygon_type *basic_ptr (polygon_type::tag) const
  {
    tl_assert (m_type == Polygon);
    if (m_stable) {
      return m_with_props ? &**(((ppolygon_iter_type *) m_generic.iter)) : &**(((polygon_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.ppolygon : m_generic.polygon;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const simple_polygon_type *basic_ptr (simple_polygon_type::tag) const
  {
    tl_assert (m_type == SimplePolygon);
    if (m_stable) {
      return m_with_props ? &**(((psimple_polygon_iter_type *) m_generic.iter)) : &**(((simple_polygon_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.psimple_polygon : m_generic.simple_polygon;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const polygon_ref_type *basic_ptr (polygon_ref_type::tag) const
  {
    tl_assert (m_type == PolygonRef);
    if (m_stable) {
      return m_with_props ? &**(((ppolygon_ref_iter_type *) m_generic.iter)) : &**(((polygon_ref_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.ppolygon_ref : m_generic.polygon_ref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const polygon_ptr_array_type *basic_ptr (polygon_ptr_array_type::tag) const
  {
    tl_assert (m_type == PolygonPtrArray || m_type == PolygonPtrArrayMember);
    if (m_stable) {
      return m_with_props ? &**(((ppolygon_ptr_array_iter_type *) m_generic.iter)) : &**(((polygon_ptr_array_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.ppolygon_aref : m_generic.polygon_aref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const simple_polygon_ref_type *basic_ptr (simple_polygon_ref_type::tag) const
  {
    tl_assert (m_type == SimplePolygonRef);
    if (m_stable) {
      return m_with_props ? &**(((psimple_polygon_ref_iter_type *) m_generic.iter)) : &**(((simple_polygon_ref_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.psimple_polygon_ref : m_generic.simple_polygon_ref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const simple_polygon_ptr_array_type *basic_ptr (simple_polygon_ptr_array_type::tag) const
  {
    tl_assert (m_type == SimplePolygonPtrArray || m_type == SimplePolygonPtrArrayMember);
    if (m_stable) {
      return m_with_props ? &**(((psimple_polygon_ptr_array_iter_type *) m_generic.iter)) : &**(((simple_polygon_ptr_array_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.psimple_polygon_aref : m_generic.simple_polygon_aref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const path_type *basic_ptr (path_type::tag) const
  {
    tl_assert (m_type == Path);
    if (m_stable) {
      return m_with_props ? &**(((ppath_iter_type *) m_generic.iter)) : &**(((path_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.ppath : m_generic.path;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const path_ref_type *basic_ptr (path_ref_type::tag) const
  {
    tl_assert (m_type == PathRef);
    if (m_stable) {
      return m_with_props ? &**(((ppath_ref_iter_type *) m_generic.iter)) : &**(((path_ref_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.ppath_ref : m_generic.path_ref;
    }
  }


  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const path_ptr_array_type *basic_ptr (path_ptr_array_type::tag) const
  {
    tl_assert (m_type == PathPtrArray || m_type == PathPtrArrayMember);
    if (m_stable) {
      return m_with_props ? &**(((ppath_ptr_array_iter_type *) m_generic.iter)) : &**(((path_ptr_array_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.ppath_aref : m_generic.path_aref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const edge_type *basic_ptr (edge_type::tag) const
  {
    tl_assert (m_type == Edge);
    if (m_stable) {
      return m_with_props ? &**(((pedge_iter_type *) m_generic.iter)) : &**(((edge_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.pedge : m_generic.edge;
    }
  }

  /**
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const edge_pair_type *basic_ptr (edge_pair_type::tag) const
  {
    tl_assert (m_type == EdgePair);
    if (m_stable) {
      return m_with_props ? &**(((pedge_pair_iter_type *) m_generic.iter)) : &**(((edge_pair_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.pedge_pair : m_generic.edge_pair;
    }
  }

  /**
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const point_type *basic_ptr (point_type::tag) const
  {
    tl_assert (m_type == Point);
    if (m_stable) {
      return m_with_props ? &**(((ppoint_iter_type *) m_generic.iter)) : &**(((point_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.ppoint : m_generic.point;
    }
  }

  /**
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const text_type *basic_ptr (text_type::tag) const
  {
    tl_assert (m_type == Text);
    if (m_stable) {
      return m_with_props ? &**(((ptext_iter_type *) m_generic.iter)) : &**(((text_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.ptext : m_generic.text;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const text_ref_type *basic_ptr (text_ref_type::tag) const
  {
    tl_assert (m_type == TextRef);
    if (m_stable) {
      return m_with_props ? &**(((ptext_ref_iter_type *) m_generic.iter)) : &**(((text_ref_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.ptext_ref : m_generic.text_ref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const text_ptr_array_type *basic_ptr (text_ptr_array_type::tag) const
  {
    tl_assert (m_type == TextPtrArray || m_type == TextPtrArrayMember);
    if (m_stable) {
      return m_with_props ? &**(((ptext_ptr_array_iter_type *) m_generic.iter)) : &**(((text_ptr_array_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.ptext_aref : m_generic.text_aref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const user_object_type *basic_ptr (user_object_type::tag) const
  {
    tl_assert (m_type == UserObject);
    if (m_stable) {
      return m_with_props ? &**(((puser_object_iter_type *) m_generic.iter)) : &**(((user_object_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.puser_object : m_generic.user_object;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const box_type *basic_ptr (box_type::tag) const
  {
    tl_assert (m_type == Box);
    if (m_stable) {
      return m_with_props ? &**(((pbox_iter_type *) m_generic.iter)) : &**(((box_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.pbox : m_generic.box;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const box_array_type *basic_ptr (box_array_type::tag) const
  {
    tl_assert (m_type == BoxArray || m_type == BoxArrayMember);
    if (m_stable) {
      return m_with_props ? &**(((pbox_array_iter_type *) m_generic.iter)) : &**(((box_array_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.pbox_array : m_generic.box_array;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const short_box_type *basic_ptr (short_box_type::tag) const
  {
    tl_assert (m_type == ShortBox);
    if (m_stable) {
      return m_with_props ? &**(((pshort_box_iter_type *) m_generic.iter)) : &**(((short_box_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.pshort_box : m_generic.short_box;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const short_box_array_type *basic_ptr (short_box_array_type::tag) const
  {
    tl_assert (m_type == ShortBoxArray || m_type == ShortBoxArrayMember);
    if (m_stable) {
      return m_with_props ? &**(((pshort_box_array_iter_type *) m_generic.iter)) : &**(((short_box_array_iter_type *) m_generic.iter));
    } else {
      return m_with_props ? m_generic.pshort_box_array : m_generic.short_box_array;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<polygon_type> *basic_ptr (db::object_with_properties<polygon_type>::tag) const
  {
    tl_assert (m_type == Polygon);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((ppolygon_iter_type *) m_generic.iter));
    } else {
      return m_generic.ppolygon;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<simple_polygon_type> *basic_ptr (db::object_with_properties<simple_polygon_type>::tag) const
  {
    tl_assert (m_type == SimplePolygon);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((psimple_polygon_iter_type *) m_generic.iter));
    } else {
      return m_generic.psimple_polygon;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<polygon_ref_type> *basic_ptr (db::object_with_properties<polygon_ref_type>::tag) const
  {
    tl_assert (m_type == PolygonRef);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((ppolygon_ref_iter_type *) m_generic.iter));
    } else {
      return m_generic.ppolygon_ref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<polygon_ptr_array_type> *basic_ptr (db::object_with_properties<polygon_ptr_array_type>::tag) const
  {
    tl_assert (m_type == PolygonPtrArray || m_type == PolygonPtrArrayMember);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((ppolygon_ptr_array_iter_type *) m_generic.iter));
    } else {
      return m_generic.ppolygon_aref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<simple_polygon_ref_type> *basic_ptr (db::object_with_properties<simple_polygon_ref_type>::tag) const
  {
    tl_assert (m_type == SimplePolygonRef);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((psimple_polygon_ref_iter_type *) m_generic.iter));
    } else {
      return m_generic.psimple_polygon_ref;
    }
  }

  const db::object_with_properties<simple_polygon_ptr_array_type> *basic_ptr (db::object_with_properties<simple_polygon_ptr_array_type>::tag) const
  {
    tl_assert (m_type == SimplePolygonPtrArray || m_type == SimplePolygonPtrArrayMember);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((psimple_polygon_ptr_array_iter_type *) m_generic.iter));
    } else {
      return m_generic.psimple_polygon_aref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<path_type> *basic_ptr (db::object_with_properties<path_type>::tag) const
  {
    tl_assert (m_type == Path);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((ppath_iter_type *) m_generic.iter));
    } else {
      return m_generic.ppath;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<path_ref_type> *basic_ptr (db::object_with_properties<path_ref_type>::tag) const
  {
    tl_assert (m_type == PathRef);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((ppath_ref_iter_type *) m_generic.iter));
    } else {
      return m_generic.ppath_ref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<path_ptr_array_type> *basic_ptr (db::object_with_properties<path_ptr_array_type>::tag) const
  {
    tl_assert (m_type == PathPtrArray || m_type == PathPtrArrayMember);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((ppath_ptr_array_iter_type *) m_generic.iter));
    } else {
      return m_generic.ppath_aref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<edge_type> *basic_ptr (db::object_with_properties<edge_type>::tag) const
  {
    tl_assert (m_type == Edge);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((pedge_iter_type *) m_generic.iter));
    } else {
      return m_generic.pedge;
    }
  }

  /**
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<edge_pair_type> *basic_ptr (db::object_with_properties<edge_pair_type>::tag) const
  {
    tl_assert (m_type == EdgePair);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((pedge_pair_iter_type *) m_generic.iter));
    } else {
      return m_generic.pedge_pair;
    }
  }

  /**
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<point_type> *basic_ptr (db::object_with_properties<point_type>::tag) const
  {
    tl_assert (m_type == Point);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((ppoint_iter_type *) m_generic.iter));
    } else {
      return m_generic.ppoint;
    }
  }

  /**
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<text_type> *basic_ptr (db::object_with_properties<text_type>::tag) const
  {
    tl_assert (m_type == Text);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((ptext_iter_type *) m_generic.iter));
    } else {
      return m_generic.ptext;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<text_ref_type> *basic_ptr (db::object_with_properties<text_ref_type>::tag) const
  {
    tl_assert (m_type == TextRef);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((ptext_ref_iter_type *) m_generic.iter));
    } else {
      return m_generic.ptext_ref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<text_ptr_array_type> *basic_ptr (db::object_with_properties<text_ptr_array_type>::tag) const
  {
    tl_assert (m_type == TextPtrArray || m_type == TextPtrArrayMember);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((ptext_ptr_array_iter_type *) m_generic.iter));
    } else {
      return m_generic.ptext_aref;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<user_object_type> *basic_ptr (db::object_with_properties<user_object_type>::tag) const
  {
    tl_assert (m_type == UserObject);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((puser_object_iter_type *) m_generic.iter));
    } else {
      return m_generic.puser_object;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<box_type> *basic_ptr (db::object_with_properties<box_type>::tag) const
  {
    tl_assert (m_type == Box);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((pbox_iter_type *) m_generic.iter));
    } else {
      return m_generic.pbox;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<box_array_type> *basic_ptr (db::object_with_properties<box_array_type>::tag) const
  {
    tl_assert (m_type == BoxArray || m_type == BoxArrayMember);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((pbox_array_iter_type *) m_generic.iter));
    } else {
      return m_generic.pbox_array;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<short_box_type> *basic_ptr (db::object_with_properties<short_box_type>::tag) const
  {
    tl_assert (m_type == ShortBox);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((pshort_box_iter_type *) m_generic.iter));
    } else {
      return m_generic.pshort_box;
    }
  }

  /** 
   *  @brief Return the actual object that this shape reference is pointing to for objects with properties
   *
   *  This is a generalization of the polygon (), etc. methods using a tag to identify the
   *  target object.
   */
  const db::object_with_properties<short_box_array_type> *basic_ptr (db::object_with_properties<short_box_array_type>::tag) const
  {
    tl_assert (m_type == ShortBoxArray || m_type == ShortBoxArrayMember);
    tl_assert (m_with_props);
    if (m_stable) {
      return &**(((pshort_box_array_iter_type *) m_generic.iter));
    } else {
      return m_generic.pshort_box_array;
    }
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  polygon_iter_type basic_iter (polygon_type::tag) const
  {
    tl_assert (m_type == Polygon && ! m_with_props);
    return *(((polygon_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  simple_polygon_iter_type basic_iter (simple_polygon_type::tag) const
  {
    tl_assert (m_type == SimplePolygon && ! m_with_props);
    return *(((simple_polygon_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  polygon_ref_iter_type basic_iter (polygon_ref_type::tag) const
  {
    tl_assert (m_type == PolygonRef && ! m_with_props);
    return *(((polygon_ref_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  polygon_ptr_array_iter_type basic_iter (polygon_ptr_array_type::tag) const
  {
    tl_assert ((m_type == PolygonPtrArray || m_type == PolygonPtrArrayMember) && ! m_with_props);
    return *(((polygon_ptr_array_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  simple_polygon_ref_iter_type basic_iter (simple_polygon_ref_type::tag) const
  {
    tl_assert (m_type == SimplePolygonRef && ! m_with_props);
    return *(((simple_polygon_ref_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  simple_polygon_ptr_array_iter_type basic_iter (simple_polygon_ptr_array_type::tag) const
  {
    tl_assert ((m_type == SimplePolygonPtrArray || m_type == SimplePolygonPtrArrayMember) && ! m_with_props);
    return *(((simple_polygon_ptr_array_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  path_iter_type basic_iter (path_type::tag) const
  {
    tl_assert (m_type == Path && ! m_with_props);
    return *(((path_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  path_ref_iter_type basic_iter (path_ref_type::tag) const
  {
    tl_assert (m_type == PathRef && ! m_with_props);
    return *(((path_ref_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  path_ptr_array_iter_type basic_iter (path_ptr_array_type::tag) const
  {
    tl_assert ((m_type == PathPtrArray || m_type == PathPtrArrayMember) && ! m_with_props);
    return *(((path_ptr_array_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  edge_iter_type basic_iter (edge_type::tag) const
  {
    tl_assert (m_type == Edge && ! m_with_props);
    return *(((edge_iter_type *) m_generic.iter));
  }

  /**
   *  @brief Return the iterator (in stable reference mode) by tag
   */
  edge_pair_iter_type basic_iter (edge_pair_type::tag) const
  {
    tl_assert (m_type == EdgePair && ! m_with_props);
    return *(((edge_pair_iter_type *) m_generic.iter));
  }

  /**
   *  @brief Return the iterator (in stable reference mode) by tag
   */
  point_iter_type basic_iter (point_type::tag) const
  {
    tl_assert (m_type == Point && ! m_with_props);
    return *(((point_iter_type *) m_generic.iter));
  }

  /**
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  text_iter_type basic_iter (text_type::tag) const
  {
    tl_assert (m_type == Text && ! m_with_props);
    return *(((text_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  text_ref_iter_type basic_iter (text_ref_type::tag) const
  {
    tl_assert (m_type == TextRef && ! m_with_props);
    return *(((text_ref_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  text_ptr_array_iter_type basic_iter (text_ptr_array_type::tag) const
  {
    tl_assert ((m_type == TextPtrArray || m_type == TextPtrArrayMember) && ! m_with_props);
    return *(((text_ptr_array_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  user_object_iter_type basic_iter (user_object_type::tag) const
  {
    tl_assert (m_type == UserObject && ! m_with_props);
    return *(((user_object_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  box_iter_type basic_iter (box_type::tag) const
  {
    tl_assert (m_type == Box && ! m_with_props);
    return *(((box_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  box_array_iter_type basic_iter (box_array_type::tag) const
  {
    tl_assert ((m_type == BoxArray || m_type == BoxArrayMember) && ! m_with_props);
    return *(((box_array_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  short_box_iter_type basic_iter (short_box_type::tag) const
  {
    tl_assert (m_type == ShortBox && ! m_with_props);
    return *(((short_box_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag 
   */
  short_box_array_iter_type basic_iter (short_box_array_type::tag) const
  {
    tl_assert ((m_type == ShortBoxArray || m_type == ShortBoxArrayMember) && ! m_with_props);
    return *(((short_box_array_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  ppolygon_iter_type basic_iter (db::object_with_properties<polygon_type>::tag) const
  {
    tl_assert (m_type == Polygon && m_with_props);
    return *(((ppolygon_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  psimple_polygon_iter_type basic_iter (db::object_with_properties<simple_polygon_type>::tag) const
  {
    tl_assert (m_type == SimplePolygon && m_with_props);
    return *(((psimple_polygon_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  ppolygon_ref_iter_type basic_iter (db::object_with_properties<polygon_ref_type>::tag) const
  {
    tl_assert (m_type == PolygonRef && m_with_props);
    return *(((ppolygon_ref_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  ppolygon_ptr_array_iter_type basic_iter (db::object_with_properties<polygon_ptr_array_type>::tag) const
  {
    tl_assert ((m_type == PolygonPtrArray || m_type == PolygonPtrArrayMember) && m_with_props);
    return *(((ppolygon_ptr_array_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  psimple_polygon_ref_iter_type basic_iter (db::object_with_properties<simple_polygon_ref_type>::tag) const
  {
    tl_assert (m_type == SimplePolygonRef && m_with_props);
    return *(((psimple_polygon_ref_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  psimple_polygon_ptr_array_iter_type basic_iter (db::object_with_properties<simple_polygon_ptr_array_type>::tag) const
  {
    tl_assert ((m_type == SimplePolygonPtrArray || m_type == SimplePolygonPtrArrayMember) && m_with_props);
    return *(((psimple_polygon_ptr_array_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  ppath_iter_type basic_iter (db::object_with_properties<path_type>::tag) const
  {
    tl_assert (m_type == Path && m_with_props);
    return *(((ppath_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  ppath_ref_iter_type basic_iter (db::object_with_properties<path_ref_type>::tag) const
  {
    tl_assert (m_type == PathRef && m_with_props);
    return *(((ppath_ref_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  ppath_ptr_array_iter_type basic_iter (db::object_with_properties<path_ptr_array_type>::tag) const
  {
    tl_assert ((m_type == PathPtrArray || m_type == PathPtrArrayMember) && m_with_props);
    return *(((ppath_ptr_array_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  pedge_iter_type basic_iter (db::object_with_properties<edge_type>::tag) const
  {
    tl_assert (m_type == Edge && m_with_props);
    return *(((pedge_iter_type *) m_generic.iter));
  }

  /**
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  pedge_pair_iter_type basic_iter (db::object_with_properties<edge_pair_type>::tag) const
  {
    tl_assert (m_type == EdgePair && m_with_props);
    return *(((pedge_pair_iter_type *) m_generic.iter));
  }

  /**
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  ppoint_iter_type basic_iter (db::object_with_properties<point_type>::tag) const
  {
    tl_assert (m_type == Point && m_with_props);
    return *(((ppoint_iter_type *) m_generic.iter));
  }

  /**
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  ptext_iter_type basic_iter (db::object_with_properties<text_type>::tag) const
  {
    tl_assert (m_type == Text && m_with_props);
    return *(((ptext_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  ptext_ref_iter_type basic_iter (db::object_with_properties<text_ref_type>::tag) const
  {
    tl_assert (m_type == TextRef && m_with_props);
    return *(((ptext_ref_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  ptext_ptr_array_iter_type basic_iter (db::object_with_properties<text_ptr_array_type>::tag) const
  {
    tl_assert ((m_type == TextPtrArray || m_type == TextPtrArrayMember) && m_with_props);
    return *(((ptext_ptr_array_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  puser_object_iter_type basic_iter (db::object_with_properties<user_object_type>::tag) const
  {
    tl_assert (m_type == UserObject && m_with_props);
    return *(((puser_object_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  pbox_iter_type basic_iter (db::object_with_properties<box_type>::tag) const
  {
    tl_assert (m_type == Box && m_with_props);
    return *(((pbox_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  pbox_array_iter_type basic_iter (db::object_with_properties<box_array_type>::tag) const
  {
    tl_assert ((m_type == BoxArray || m_type == BoxArrayMember) && m_with_props);
    return *(((pbox_array_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  pshort_box_iter_type basic_iter (db::object_with_properties<short_box_type>::tag) const
  {
    tl_assert (m_type == ShortBox && m_with_props);
    return *(((pshort_box_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return the iterator (in stable reference mode) by tag for objects with properties
   */
  pshort_box_array_iter_type basic_iter (db::object_with_properties<short_box_array_type>::tag) const
  {
    tl_assert ((m_type == ShortBoxArray || m_type == ShortBoxArrayMember) && m_with_props);
    return *(((pshort_box_array_iter_type *) m_generic.iter));
  }

  /** 
   *  @brief Return a reference to the polygon if one is referenced
   */
  const polygon_type &polygon () const
  {
    return *basic_ptr (polygon_type::tag ());
  }

  /** 
   *  @brief Return a polygon reference if one is referenced
   */
  polygon_ref_type polygon_ref () const;

  /** 
   *  @brief Return a reference to the simple polygon if one is referenced
   */
  const simple_polygon_type &simple_polygon () const
  {
    return *basic_ptr (simple_polygon_type::tag ());
  }

  /** 
   *  @brief Return a simple polygon reference if one is referenced
   */
  simple_polygon_ref_type simple_polygon_ref () const;

  /** 
   *  @brief Test if the shape proxy points to a polygon
   */
  bool is_polygon () const
  {
    return (m_type == Polygon || m_type == PolygonRef || m_type == PolygonPtrArrayMember || 
            m_type == SimplePolygon || m_type == SimplePolygonRef || m_type == SimplePolygonPtrArrayMember);
  }

  /** 
   *  @brief Instantiate the polygon object
   * 
   *  If a polygon is referenced, this object is instantiated
   *  by this method. Paths are converted to polygons. Boxes are also converted.
   *  Returns true, if the conversion was successful.
   */
  bool polygon (polygon_type &p) const;

  /** 
   *  @brief Alias for polymorphic expansion
   *  Returns true, if the conversion was successful.
   */
  bool instantiate (polygon_type &p) const
  {
    return polygon (p);
  }

  /** 
   *  @brief Test if the shape proxy points to a simple polygon
   */
  bool is_simple_polygon () const
  {
    return (m_type == SimplePolygon || m_type == SimplePolygonRef || m_type == SimplePolygonPtrArrayMember);
  }

  /** 
   *  @brief Instantiate the simple polygon object
   * 
   *  If a simple polygon is referenced, this object is instantiated
   *  by this method. Paths are converted to polygons. Boxes are also converted.
   *  Returns true, if the conversion was successful.
   */
  bool simple_polygon (simple_polygon_type &p) const;

  /** 
   *  @brief Alias for polymorphic expansion
   *  Returns true, if the conversion was successful.
   */
  bool instantiate (simple_polygon_type &p) const
  {
    return simple_polygon (p);
  }

  /** 
   *  @brief Return a reference to the path if one is referenced
   */
  const path_type &path () const
  {
    return *basic_ptr (path_type::tag ());
  }

  /** 
   *  @brief Return a path reference if one is referenced
   */
  path_ref_type path_ref () const;

  /** 
   *  @brief Test if the shape proxy points to a path
   */
  bool is_path () const
  {
    return (m_type == Path || m_type == PathRef || m_type == PathPtrArrayMember);
  }

  /**
   *  @brief Obtain the path length
   *
   *  Applies to paths only. Will assert if not a path.
   */
  distance_type path_length () const;
  
  /**
   *  @brief Obtain the path width
   *
   *  Applies to paths only. Will assert if not a path.
   */
  coord_type path_width () const;
  
  /**
   *  @brief Obtain the path extensions
   *
   *  Applies to paths only. Will assert if not a path.
   *
   *  @return A pair consisting of the begin and end extensions.
   */
  std::pair<coord_type, coord_type> path_extensions () const;
  
  /**
   *  @brief Obtain the round path flag
   *
   *  Applies to paths only. Will assert if not a path.
   *
   *  @return True, if the path has round ends
   */
  bool round_path () const;
  
  /** 
   *  @brief Instantiate the path object
   * 
   *  If a path is referenced, this object is instantiated
   *  by this method.
   *  Returns true, if the conversion was successful.
   */
  bool path (path_type &p) const;

  /** 
   *  @brief Alias for polymorphic expansion
   */
  bool instantiate (path_type &p) const
  {
    return path (p);
  }

  /** 
   *  @brief Return a reference to the edge if one is referenced
   */
  const edge_type &edge () const
  {
    tl_assert (m_type == Edge);
    return *basic_ptr (edge_type::tag ());
  }

  /** 
   *  @brief Test if the shape proxy points to a edge
   */
  bool is_edge () const
  {
    return (m_type == Edge);
  }

  /** 
   *  @brief Instantiate the edge object
   * 
   *  If a edge is referenced, this object is instantiated
   *  by this method.
   *  Returns true, if the conversion was successful.
   */
  bool edge (edge_type &e) const
  {
    if (is_edge ()) {
      e = edge ();
      return true;
    } else {
      return false;
    }
  }

  /** 
   *  @brief Alias for polymorphic expansion
   *  Returns true, if the conversion was successful.
   */
  bool instantiate (edge_type &p) const
  {
    return edge (p);
  }

  /**
   *  @brief Return a reference to the edge pair if one is referenced
   */
  const edge_pair_type &edge_pair () const
  {
    tl_assert (m_type == EdgePair);
    return *basic_ptr (edge_pair_type::tag ());
  }

  /**
   *  @brief Test if the shape proxy points to a edge pair
   */
  bool is_edge_pair () const
  {
    return (m_type == EdgePair);
  }

  /**
   *  @brief Instantiate the edge pair object
   *
   *  If an edge pair is referenced, this object is instantiated
   *  by this method.
   *  Returns true, if the conversion was successful.
   */
  bool edge_pair (edge_pair_type &e) const
  {
    if (is_edge_pair ()) {
      e = edge_pair ();
      return true;
    } else {
      return false;
    }
  }

  /**
   *  @brief Alias for polymorphic expansion
   *  Returns true, if the conversion was successful.
   */
  bool instantiate (edge_pair_type &p) const
  {
    return edge_pair (p);
  }

  /**
   *  @brief Return a reference to the point if one is referenced
   */
  const point_type &point () const
  {
    tl_assert (m_type == Point);
    return *basic_ptr (point_type::tag ());
  }

  /**
   *  @brief Test if the shape proxy points to a point
   */
  bool is_point () const
  {
    return (m_type == Point);
  }

  /**
   *  @brief Instantiate the edge object
   *
   *  If a edge is referenced, this object is instantiated
   *  by this method.
   *  Returns true, if the conversion was successful.
   */
  bool point (point_type &p) const
  {
    if (is_point ()) {
      p = point ();
      return true;
    } else {
      return false;
    }
  }

  /**
   *  @brief Alias for polymorphic expansion
   *  Returns true, if the conversion was successful.
   */
  bool instantiate (point_type &p) const
  {
    return point (p);
  }

  /**
   *  @brief Return a reference to the text if one is referenced
   */
  const text_type &text () const
  {
    return *basic_ptr (text_type::tag ());
  }

  /** 
   *  @brief Return a text reference if one is referenced
   */
  text_ref_type text_ref () const;

  /** 
   *  @brief Test if the shape proxy points to a text
   */
  bool is_text () const
  {
    return (m_type == Text || m_type == TextRef || m_type == TextPtrArrayMember);
  }

  /** 
   *  @brief Instantiate the text object
   * 
   *  If a text is referenced, this object is instantiated
   *  by this method.
   *  Returns true, if the conversion was successful
   */
  bool text (text_type &t) const;

  /** 
   *  @brief Alias for polymorphic expansion
   *  Returns true, if the conversion was successful
   */
  bool instantiate (text_type &p) const
  {
    return text (p);
  }

  /**
   *  @brief Obtain the text string
   *
   *  Applies to texts only. Will assert if not a text.
   */
  const char *text_string () const;
  
  /**
   *  @brief Obtain the text transformation
   *
   *  Applies to texts only. Will assert if not a text.
   */
  text_type::trans_type text_trans () const;

  /**
   *  @brief Obtain the text size
   *
   *  Applies to texts only. Will assert if not a text.
   */
  coord_type text_size () const;

  /**
   *  @brief Obtain the text's font
   *
   *  Applies to texts only. Will assert if not a text.
   */
  db::Font text_font () const;

  /**
   *  @brief Obtain the text's horizontal alignment
   *
   *  Applies to texts only. Will assert if not a text.
   */
  db::HAlign text_halign () const;

  /**
   *  @brief Obtain the text's vertical alignment
   *
   *  Applies to texts only. Will assert if not a text.
   */
  db::VAlign text_valign () const;

  /** 
   *  @brief Return a box if one is referenced
   */
  box_type box () const;

  /** 
   *  @brief Test if the shape proxy points to a box
   */
  bool is_box () const
  {
    return (m_type == Box || m_type == ShortBox || m_type == BoxArrayMember || m_type == ShortBoxArrayMember);
  }

  /** 
   *  @brief Instantiate the box object
   * 
   *  If a box is referenced, this object is instantiated
   *  by this method.
   *  Returns true, if the conversion was successful
   */
  bool box (box_type &b) const
  {
    if (is_box ()) {
      b = box ();
      return true;
    } else {
      return false;
    }
  }

  /** 
   *  @brief Alias for polymorphic expansion
   */
  bool instantiate (box_type &p) const
  {
    return box (p);
  }

  /** 
   *  @brief Return a reference to the user object if one is referenced
   */
  const user_object_type &user_object () const
  {
    return *basic_ptr (user_object_type::tag ());
  }

  /** 
   *  @brief Test if the shape proxy points to a user object
   */
  bool is_user_object () const
  {
    return (m_type == UserObject);
  }

  /** 
   *  @brief Instantiate the user object
   * 
   *  If a user object is referenced, this object is instantiated
   *  by this method.
   */
  void user_object (user_object_type &u) const
  {
    u = *basic_ptr (user_object_type::tag ());
  }

  /** 
   *  @brief Compute the bounding box of the shape the is referenced
   */
  box_type bbox () const;

  /** 
   *  @brief Compute the area of the shape
   */
  area_type area () const;

  /** 
   *  @brief Compute the perimeter of the shape
   */
  perimeter_type perimeter () const;

  /**
   *  @brief Attribute that tells if the shape is member of an array
   *
   *  In this case the shape is not a freestanding shape but one instance of an
   *  array shape.
   */
  bool is_array_member () const
  {
    return m_type == PolygonPtrArrayMember ||
           m_type == SimplePolygonPtrArrayMember ||
           m_type == PathPtrArrayMember ||
           m_type == BoxArrayMember ||
           m_type == ShortBoxArrayMember ||
           m_type == TextPtrArrayMember;
  }

  /**
   *  @brief Returns the array size if the shape references an array
   */
  size_t array_size () const;

  /**
   *  @brief Array instance member transformation
   *
   *  This attribute is valid only if is_array_member is true.
   *  The transformation returned describes the relative transformation of the 
   *  array member addressed.
   */
  const trans_type &array_trans () const
  {
    return m_trans;
  }

  /**
   *  @brief inequality 
   */
  bool operator!= (const Shape &d) const
  {
    return ! operator== (d);
  }

  /**
   *  @brief equality 
   *
   *  Equality of shapes is not specified by the identity of the objects but by the
   *  identity of the pointers - both shapes must reference the same object.
   */
  bool operator== (const Shape &d) const
  {
    if (m_type != d.m_type) {
      return false;
    }
    if (m_stable) {
      //  do a byte-by-byte compare of the iterators:
      //  there are supposed to have the same size for all types, thus all bytes are
      //  supposed to be meaningful
      for (unsigned int i = 0; i < sizeof (tl::reuse_vector<box_type>::const_iterator); ++i) {
        if (m_generic.iter[i] != d.m_generic.iter[i]) {
          return false;
        }
      }
    } else {
      if (m_generic.any != d.m_generic.any) {
        return false;
      }
    }
    return m_trans == d.m_trans;
  }

  /**
   *  @brief less-than operator 
   *
   *  Ordering of shapes is not specified by the identity of the objects but by the
   *  order of the pointers.
   *  Since pointers are volatile objects, the ordering is not strictly reproducible!
   */
  bool operator< (const Shape &d) const
  {
    if (m_type != d.m_type) {
      return m_type < d.m_type;
    }
    if (m_stable) {
      //  do a byte-by-byte compare of the iterators:
      //  there are supposed to have the same size for all types, thus all bytes are
      //  supposed to be meaningful
      for (unsigned int i = 0; i < sizeof (tl::reuse_vector<box_type>::const_iterator); ++i) {
        if (m_generic.iter[i] != d.m_generic.iter[i]) {
          return m_generic.iter[i] < d.m_generic.iter[i];
        }
      }
    } else {
      if (m_generic.any != d.m_generic.any) {
        return m_generic.any < d.m_generic.any;
      }
    }
    return m_trans < d.m_trans;
  }

  /** 
   *  @brief Convert to a string
   */
  std::string to_string () const;

public:
  friend class db::Shapes;
 
  union generic {

    const polygon_type *polygon;
    const polygon_ref_type *polygon_ref;
    const polygon_ptr_array_type *polygon_aref;
    const simple_polygon_type *simple_polygon;
    const simple_polygon_ref_type *simple_polygon_ref;
    const simple_polygon_ptr_array_type *simple_polygon_aref;
    const text_type *text;
    const text_ref_type *text_ref;
    const text_ptr_array_type *text_aref;
    const edge_type *edge;
    const edge_pair_type *edge_pair;
    const point_type *point;
    const path_type *path;
    const path_ref_type *path_ref;
    const path_ptr_array_type *path_aref;
    const box_type *box;
    const box_array_type *box_array;
    const short_box_type *short_box;
    const short_box_array_type *short_box_array;
    const user_object_type *user_object;

    const db::object_with_properties<polygon_type> *ppolygon;
    const db::object_with_properties<polygon_ref_type> *ppolygon_ref;
    const db::object_with_properties<polygon_ptr_array_type> *ppolygon_aref;
    const db::object_with_properties<simple_polygon_type> *psimple_polygon;
    const db::object_with_properties<simple_polygon_ref_type> *psimple_polygon_ref;
    const db::object_with_properties<simple_polygon_ptr_array_type> *psimple_polygon_aref;
    const db::object_with_properties<text_type> *ptext;
    const db::object_with_properties<text_ref_type> *ptext_ref;
    const db::object_with_properties<text_ptr_array_type> *ptext_aref;
    const db::object_with_properties<edge_type> *pedge;
    const db::object_with_properties<edge_pair_type> *pedge_pair;
    const db::object_with_properties<point_type> *ppoint;
    const db::object_with_properties<path_type> *ppath;
    const db::object_with_properties<path_ref_type> *ppath_ref;
    const db::object_with_properties<path_ptr_array_type> *ppath_aref;
    const db::object_with_properties<box_type> *pbox;
    const db::object_with_properties<box_array_type> *pbox_array;
    const db::object_with_properties<short_box_type> *pshort_box;
    const db::object_with_properties<short_box_array_type> *pshort_box_array;
    const db::object_with_properties<user_object_type> *puser_object;

    const void *any;

    //  all iterators have the same size - there is just one space for that
    char iter [sizeof (tl::reuse_vector<box_type>::const_iterator)];

  };

  db::Shapes *mp_shapes;
  generic m_generic;
  trans_type m_trans;
  bool m_with_props : 8;
  bool m_stable : 8;
  object_type m_type : 16;
};

}  // namespace db
  
#endif

