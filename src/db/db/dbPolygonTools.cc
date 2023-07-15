
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


#include "dbCommon.h"

#include "dbPolygonTools.h"
#include "dbPolygonGenerators.h"
#include "tlLog.h"
#include "tlInt128Support.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace db
{

// -------------------------------------------------------------------------
//  Implementation of inside_poly_test

//  internal: compare edges by higher y coordinate
template <class E>
struct inside_poly_test_edge_max_compare_f
{
  bool operator() (const E &e1, const E &e2)
  {
    return std::max (e1.p1 ().y (), e1.p2 ().y ()) < std::max (e2.p1 ().y (), e2.p2 ().y ());
  }
};

template<class P> 
inside_poly_test<P>::inside_poly_test (const P &polygon)
{
  m_edges.reserve (polygon.vertices ());
  for (typename P::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
    m_edges.push_back (*e);
  }
  std::sort (m_edges.begin (), m_edges.end (), inside_poly_test_edge_max_compare_f<edge_type> ());
}

template<class P> 
int inside_poly_test<P>::operator() (const typename inside_poly_test<P>::point_type &pt) const
{
  int wrapcount_left = 0;

  typename std::vector<edge_type>::const_iterator e = std::lower_bound (m_edges.begin (), m_edges.end (), edge_type (pt, pt), inside_poly_test_edge_max_compare_f<edge_type> ());
  while (e != m_edges.end () && pt.y () <= std::max (e->p1 ().y (), e->p2 ().y ())) {

    if ((*e).p1 ().y () <= pt.y () && (*e).p2 ().y () > pt.y ()) {
      int side = (*e).side_of (pt);
      if (side < 0) { 
        ++wrapcount_left;
      } else if (side == 0) {
        //  "on" the line is excluded in the predicate
        return 0;
      }
    } else if ((*e).p2 ().y () <= pt.y () && (*e).p1 ().y () > pt.y ()) {
      int side = (*e).side_of (pt);
      if (side > 0) { 
        --wrapcount_left;
      } else if (side == 0) {
        //  "on" the line is excluded in the predicate
        return 0;
      }
    } else if ((*e).p1 ().y () == pt.y () && (*e).p2 ().y () == pt.y () &&
               (((*e).p1 ().x () <= pt.x () && (*e).p2 ().x () >= pt.x ()) ||
                ((*e).p2 ().x () <= pt.x () && (*e).p1 ().x () >= pt.x ()))) {
      //  "on" the horizontal line is excluded in the predicate
      return 0;
    }

    ++e;

  }

  return (wrapcount_left != 0) ? 1 : -1;
}

template class DB_PUBLIC inside_poly_test<db::SimplePolygon>;
template class DB_PUBLIC inside_poly_test<db::Polygon>;
template class DB_PUBLIC inside_poly_test<db::DSimplePolygon>;
template class DB_PUBLIC inside_poly_test<db::DPolygon>;

// -------------------------------------------------------------------------
//  Implementation of cut_polygon

/**
 *  @brief A helper structure describing an edge cutting the cut line.
 */
template <class PointType>
struct cut_polygon_edge
{
  typedef typename PointType::coord_type coord_type;
  typedef typename db::edge<coord_type> edge_type;
  typedef double projection_type;

  cut_polygon_edge ()
    : contour (-1), index (0), projected (0), point (), last_point ()
  { }

  cut_polygon_edge (int c, unsigned int n, projection_type p, const PointType &pt, const PointType &lpt) 
    : contour (c), index (n), projected (p), point (pt), last_point (lpt)
  { }

  edge_type edge () const
  {
    return edge_type (last_point, point);
  }

  int contour;
  unsigned int index;
  projection_type projected;
  PointType point;
  PointType last_point;
};

template <class CuttingEdgeType>
struct cut_polygon_segment
{
  typedef typename CuttingEdgeType::edge_type edge_type;
  typedef typename CuttingEdgeType::projection_type projection_type;

  CuttingEdgeType leave;
  CuttingEdgeType enter;
  int segment;

  cut_polygon_segment ()
    : leave (), enter (), segment (-1)
  {
    // .. nothing yet ..
  }
};

template <class CuttingEdgeType>
struct loose_end_struct
{
public:
  typedef typename CuttingEdgeType::edge_type edge_type;
  typedef typename CuttingEdgeType::projection_type projection_type;

  loose_end_struct (bool _enter, typename std::vector<cut_polygon_segment<CuttingEdgeType> >::const_iterator _iter)
    : enter (_enter), iter (_iter)
  {
    //  .. nothing yet ..
  }

  bool enter;
  typename std::vector<cut_polygon_segment<CuttingEdgeType> >::const_iterator iter;

  edge_type edge () const
  {
    return enter ? iter->enter.edge () : iter->leave.edge ();
  }

  projection_type proj () const
  {
    return enter ? iter->enter.projected : iter->leave.projected;
  }

  bool operator< (const loose_end_struct<CuttingEdgeType> &other) const
  {
    if (! db::coord_traits<double>::equal (proj (), other.proj ())) {
      return proj () < other.proj ();
    } else {
      return db::vprod_sign (edge (), other.edge ()) > 0;
    }
  }

  bool operator== (const loose_end_struct<CuttingEdgeType> &other) const
  {
    if (! db::coord_traits<double>::equal (proj (), other.proj ())) {
      return false;
    } else {
      return db::vprod_sign (edge (), other.edge ()) == 0;
    }
  }
};

template <class PolygonType, class Edge>
static bool _cut_polygon_internal (const PolygonType &input, const Edge &line, cut_polygon_receiver_base<PolygonType> *right_of_line)
{
  typedef typename PolygonType::point_type point_type;
  typedef typename PolygonType::coord_type coord_type;
  typedef typename PolygonType::contour_type contour_type;
  typedef db::edge<coord_type> edge_type;
  typedef cut_polygon_edge<point_type> cut_polygon_edge_type;
  typedef cut_polygon_segment<cut_polygon_edge_type> cutting_segment_type;

  bool do_hole_assignment = (input.holes () > 0);
  std::vector <PolygonType> hull_polygons;
  std::vector <PolygonType> hole_polygons;

  std::vector<cutting_segment_type> cutting_segments;
  double line_length = line.double_length ();

  for (unsigned int nc = 0; nc < input.holes () + 1; ++nc) {

    const contour_type &contour = input.contour (nc);
    if (contour.size () == 0) {
      continue;
    }

    bool any = false;

    unsigned int nn = (unsigned int) contour.size () - 1;
    int sc = -1;
    size_t nfirst = cutting_segments.size ();

    point_type last_pt = contour[nn];
    for (unsigned int n = 0; n < contour.size (); ++n) {

      edge_type e (last_pt, contour[n]);
      last_pt = e.p2 ();

      std::pair <bool, point_type> ip = line.crossed_by_point (e);
      if (ip.first) {

        int s1 = line.side_of (e.p1 ());
        int s2 = line.side_of (e.p2 ());

        double p = line_length * double (db::vprod (e.p1 () - line.p1 (), e.d ())) / double (db::vprod (line.d (), e.d ()));

        if (s1 < 0 && s2 >= 0) {
          // right -> left or on edge
          if (cutting_segments.size () == nfirst) {
            cutting_segments.push_back (cutting_segment_type ());
          }
          cutting_segments.back ().leave = cut_polygon_edge_type (int (nc), nn, p, ip.second, e.p1 ());
          any = true;
        }

        if (s1 >= 0 && s2 < 0) {
          // left or on edge -> right 
          ++sc;
          cutting_segments.push_back (cutting_segment_type ());
          cutting_segments.back ().enter = cut_polygon_edge_type (int (nc), nn, p, ip.second, e.p2 ());
          cutting_segments.back ().segment = sc;
        }

      }

      nn = n;

    }

    if (any) {

      //  tie together last and first partial segments.
      if (cutting_segments[nfirst].segment < 0) {
        cutting_segments[nfirst].enter = cutting_segments.back ().enter;
        cutting_segments[nfirst].segment = cutting_segments.back ().segment;
        cutting_segments.pop_back ();
      }

    } else if (line.side_of (contour[0]) < 0) {

      if (do_hole_assignment) {

        if (nc == 0) {
          //  the hull is fully on the right side -> just output the input polygon and that's it.
          right_of_line->put (input);
          return true;
        } else {
          //  remember hole contours for later assignment
          hole_polygons.push_back (PolygonType ());
          hole_polygons.back ().assign_hull (contour.begin (), contour.end ());
        }

      } else {
        PolygonType poly;
        poly.assign_hull (contour.begin (), contour.end ());
        right_of_line->put (poly);
      }
    }

  }

  //  build a table of the loose ends

  std::vector<loose_end_struct<cut_polygon_edge_type> > loose_ends;
  loose_ends.reserve (cutting_segments.size () * 2);

  for (typename std::vector<cutting_segment_type>::const_iterator i = cutting_segments.begin (); i != cutting_segments.end (); ++i) {
    loose_ends.push_back (loose_end_struct<cut_polygon_edge_type> (true, i));
    loose_ends.push_back (loose_end_struct<cut_polygon_edge_type> (false, i));
  }

  std::stable_sort (loose_ends.begin (), loose_ends.end ());

  //  we allow single pairs of collinear entry/leave edges (cut lines) and bring them in the right order

  bool enter = false;
  for (typename std::vector<loose_end_struct<cut_polygon_edge_type> >::iterator i = loose_ends.begin (); i != loose_ends.end (); ++i) {
    if (i + 1 != loose_ends.end () && i[1] == i[0]) {
      if (i + 2 != loose_ends.end () && i[2] == i[0]) {
        //  triple collinear
        return false;
      }
      if (i[0].enter != enter && i[1].enter == enter) {
        std::swap (i[0], i[1]);
      }
    }
    enter = !enter;
  }

  //  the points now have to be in strict enter/leave order - otherwise fallback to merge

  enter = false;
  for (typename std::vector<loose_end_struct<cut_polygon_edge_type> >::iterator i = loose_ends.begin (); i != loose_ends.end (); ++i) {
    if (i->enter != enter) {
      return false;
    }
    enter = !enter;
  }

  //  connect the segments a pair each

  std::vector<cut_polygon_edge_type> cutting_edges;
  cutting_edges.reserve (loose_ends.size ());

  for (typename std::vector<loose_end_struct<cut_polygon_edge_type> >::iterator i = loose_ends.begin (); i != loose_ends.end (); ++i) {
    cutting_edges.push_back (i->enter ? i->iter->enter : i->iter->leave);
  }

  std::vector<point_type> contour_points;
  typedef std::map <std::pair<int, int>, std::pair<typename std::vector<cut_polygon_edge_type>::iterator, typename std::vector<cut_polygon_edge_type>::iterator> > cut_point_map_type;
  cut_point_map_type cut_points;

  for (typename std::vector<cut_polygon_edge_type>::iterator c = cutting_edges.begin (); c != cutting_edges.end (); c += 2) {
    cut_points.insert (std::make_pair (std::make_pair (c[0].contour, c[0].index), std::make_pair (c, c + 1)));
    cut_points.insert (std::make_pair (std::make_pair (c[1].contour, c[1].index), std::make_pair (c + 1, c)));
  }

  for (typename std::vector<cut_polygon_edge_type>::iterator c = cutting_edges.begin (); c != cutting_edges.end (); c += 2) {

    if (c->contour >= 0) {

      typename std::vector<cut_polygon_edge_type>::iterator c1 = c;
      typename std::vector<cut_polygon_edge_type>::iterator c2 = c + 1;

      contour_points.clear ();
      bool is_hull = false;

      do {

        tl_assert (c1->contour >= 0 && c2->contour >= 0);

        contour_points.push_back (c1->point);
        contour_points.push_back (c2->point);

        int n = c2->index;
        int n0 = n;
        int nc = c2->contour;
        const contour_type &contour = input.contour (nc);

        if (nc == 0) {
          is_hull = true;
        }

        c1->contour = -1;
        c2->contour = -1;

        ++n;
        if (n == int (contour.size ())) {
          n = 0;
        }

        while (n != n0) {

          contour_points.push_back (contour[n]);

          typename cut_point_map_type::iterator cp = cut_points.find (std::make_pair (nc, n));
          if (cp != cut_points.end ()) {
            c1 = cp->second.first;
            c2 = cp->second.second;
            break;
          }

          ++n;
          if (n == int (contour.size ())) {
            n = 0;
          }

        }

        tl_assert (n != n0);

      } while (c1 != c && c2 != c);

      //  Hint: the algorithm tends to create spikes for hole insertion edges.
      //  Therefore we used "remove reflected" on the assignment.
      if (do_hole_assignment) {

        if (is_hull) {
          hull_polygons.push_back (PolygonType ());
          hull_polygons.back ().assign_hull (contour_points.begin (), contour_points.end (), true, true /*remove reflected*/);
        } else {
          hole_polygons.push_back (PolygonType ());
          hole_polygons.back ().assign_hull (contour_points.begin (), contour_points.end (), true, true /*remove reflected*/);
        }

      } else {
        PolygonType poly;
        poly.assign_hull (contour_points.begin (), contour_points.end (), true, true /*remove reflected*/);
        //  it might happen in some cases, that cut pieces may vanish (i.e. all points on a line). Thus we check, if that
        //  is the case and do not produce a polygon then.
        if (poly.vertices () > 0) {
          right_of_line->put (poly);
        }
      }

    }

  }

  //  do hole assignment
  for (typename std::vector<PolygonType>::iterator hull = hull_polygons.begin (); hull != hull_polygons.end (); ++hull) {

    //  it might happen in some cases, that cut pieces may vanish (i.e. all points on a line). Thus we check, if that
    //  is the case and do not produce a polygon then.
    if (hull->vertices () > 0) {

      db::inside_poly_test<PolygonType> inside_hull (*hull);
      for (typename std::vector<PolygonType>::iterator hole = hole_polygons.begin (); hole != hole_polygons.end (); ++hole) {
        size_t n = hole->hull ().size ();
        if (n > 0) {
          //  look for one point "really" inside ...
          int inside = 0;
          for (size_t i = 0; i < n && inside == 0; ++i) {
            inside = inside_hull (hole->hull ()[i]);
          }
          if (inside >= 0) {
            hull->insert_hole (hole->hull ().begin (), hole->hull ().end ());
            *hole = PolygonType ();
          }
        }
      }

      hull->sort_holes ();

      right_of_line->put (*hull);

    }

  }

  // use non-assigned hole (parts) as hulls
  // TODO: precisely, this is possible only if the orientation is clockwise. Since we loose the orientation because
  // we assign to a PolygonType, this check is not possible.
  for (typename std::vector<PolygonType>::iterator hole = hole_polygons.begin (); hole != hole_polygons.end (); ++hole) {
    if (hole->vertices () > 0) {
      right_of_line->put (*hole);
    }
  }

  return true;
}

namespace
{

  template <class PolygonType>
  struct get_sink_type;

  template <>
  struct get_sink_type<db::Polygon> { typedef db::PolygonSink result; };

  template <>
  struct get_sink_type<db::SimplePolygon> { typedef db::SimplePolygonSink result; };

  /**
   *  @brief A polygon sink for the edge processor that feeds the polygon into the cut algorithm
   */
  template <class Sink, class PolygonType>
  struct cut_polygon_bool_sink
    : public Sink
  {
    cut_polygon_bool_sink (cut_polygon_receiver_base<PolygonType> *_right_of_line)
      : right_of_line (_right_of_line)
    {
      //  .. nothing yet ..
    }

    virtual void put (const PolygonType &poly)
    {
      right_of_line->put (poly);
    }

    cut_polygon_receiver_base<PolygonType> *right_of_line;
  };

  /**
   *  @brief The cut algorithm driver with fallback to polygon merging when the cut fails
   *  TODO: this is kind of inefficient as we will first merge all and the cut just a half.
   *  This means for producing both parts we do this twice. But remember, this is just a
   *  fallback.
   */
  template <class PolygonType, class Edge>
  void cut_polygon_internal_int (const PolygonType &input, const Edge &line, cut_polygon_receiver_base<PolygonType> *right_of_line)
  {
    bool ok = _cut_polygon_internal (input, line, right_of_line);
    if (! ok) {

      //  If the fast cut operation fails, use boolean AND to perform the cut operation

      PolygonType clip (input.box ());
      std::vector<PolygonType> mask;
      cut_polygon (clip, line, std::back_inserter (mask));

      if (! mask.empty ()) {

        db::EdgeProcessor ep;
        ep.insert_sequence (input.begin_edge (), 0);
        ep.insert_sequence (mask.begin (), mask.end (), 1);

        db::BooleanOp op (BooleanOp::And);

        cut_polygon_bool_sink<typename get_sink_type<PolygonType>::result, PolygonType> sink (right_of_line);
        db::PolygonGenerator pg (sink);
        ep.process (pg, op);


      }

    }

  }

  /**
   *  A transforming receiver that is put between a int cut algorithm and the double output receiver
   */
  template <class PolygonType, class IPolygonType>
  class cut_polygon_receiver_double_impl
    : public cut_polygon_receiver_base<IPolygonType>
  {
  public:
    cut_polygon_receiver_double_impl ()
      : mp_next (0)
    { }

    void set_next (cut_polygon_receiver_base<PolygonType> *next)
    {
      mp_next = next;
    }

    void set_trans (const db::CplxTrans &tr)
    {
      m_tr = tr;
    }

    virtual void put (const IPolygonType &p)
    {
      PolygonType pp = p.transformed (m_tr, false);
      mp_next->put (pp);
    }

  private:
    cut_polygon_receiver_base<PolygonType> *mp_next;
    db::CplxTrans m_tr;
  };

  template <class PolygonType>
  class cut_polygon_receiver_double;

  //  template specializations for the double types
  template <> class cut_polygon_receiver_double<db::DPolygon>       : public cut_polygon_receiver_double_impl<db::DPolygon, db::Polygon> { };
  template <> class cut_polygon_receiver_double<db::DSimplePolygon> : public cut_polygon_receiver_double_impl<db::DSimplePolygon, db::SimplePolygon> { };

  /**
   *  @brief The cut algorithm driver for double types
   *  This driver will first try to guess a database unit (based on the bounding box) and then
   *  transform the polygon to int. On output, the polygon is transformed back to double.
   */
  template <class PolygonType, class Edge>
  void cut_polygon_internal_double (const PolygonType &input, const Edge &line, cut_polygon_receiver_base<PolygonType> *right_of_line)
  {
    db::DBox bbox = input.box ();
    bbox += db::DBox (0, 0, 0, 0);
    bbox += line.bbox ();

    //  guess DBU
    double dbu = std::max (1e-10, std::max (bbox.width (), bbox.height ()) / (std::numeric_limits<db::Coord>::max () / 2));
    dbu = pow (10.0, ceil (log10 (dbu)));

    db::CplxTrans tr (dbu);
    cut_polygon_receiver_double<PolygonType> rec;
    rec.set_trans (tr);
    rec.set_next (right_of_line);

    cut_polygon_internal_int (input.transformed (tr.inverted (), false), line.transformed (tr.inverted ()), &rec);
  }

}

template<> DB_PUBLIC void cut_polygon_internal (const db::Polygon &polygon, const db::Polygon::edge_type &line, cut_polygon_receiver_base<db::Polygon> *right_of_line)
{
  cut_polygon_internal_int (polygon, line, right_of_line);
}

template<> DB_PUBLIC void cut_polygon_internal (const db::SimplePolygon &polygon, const db::SimplePolygon::edge_type &line, cut_polygon_receiver_base<db::SimplePolygon> *right_of_line)
{
  cut_polygon_internal_int (polygon, line, right_of_line);
}

template<> DB_PUBLIC void cut_polygon_internal (const db::DPolygon &polygon, const db::DPolygon::edge_type &line, cut_polygon_receiver_base<db::DPolygon> *right_of_line)
{
  cut_polygon_internal_double (polygon, line, right_of_line);
}

template<> DB_PUBLIC void cut_polygon_internal (const db::DSimplePolygon &polygon, const db::DSimplePolygon::edge_type &line, cut_polygon_receiver_base<db::DSimplePolygon> *right_of_line)
{
  cut_polygon_internal_double (polygon, line, right_of_line);
}


// -------------------------------------------------------------------------
//  Implementation of split_polygon

template <class PolygonType>
void
split_polygon (const PolygonType &polygon, std::vector<PolygonType> &output)
{
  typedef typename PolygonType::coord_type coord_type;
  typedef typename PolygonType::point_type point_type;
  typedef typename PolygonType::box_type box_type;
  typedef db::edge<coord_type> edge_type;

  box_type bbox = polygon.box ();
  box_type b1, b2;

  coord_type x = bbox.center ().x ();
  coord_type xx = x;
  bool xx_set = false;

  coord_type y = bbox.center ().y ();
  coord_type yy = y;
  bool yy_set = false;

  for (typename PolygonType::polygon_contour_iterator e = polygon.begin_hull (); e != polygon.end_hull (); ++e) {
    if ((*e).x () != bbox.left () && (*e).x () != bbox.right () && (std::abs ((*e).x () - x) < std::abs (xx - x) || ! xx_set)) {
      xx = (*e).x ();
      xx_set = true;
    }
    if ((*e).y () != bbox.top () && (*e).y () != bbox.bottom () && (std::abs ((*e).y () - y) < std::abs (yy - y) || ! yy_set)) {
      yy = (*e).y ();
      yy_set = true;
    }
  }

  if (! xx_set && ! yy_set) {
    if (bbox.width () > bbox.height ()) { 
      xx_set = true;
    } else {
      yy_set = true;
    }
  } else if (xx_set && yy_set) {
    //  an empiric threshold for splitting polygons in one direction: don't split along the long
    //  axis for polygons with a aspect ratio (of the bounding box) of larger than 3
    if (bbox.width () > 3 * bbox.height ()) {
      yy_set = false;
    } else if (bbox.height () > 3 * bbox.width ()) {
      xx_set = false;
    }
  }

  std::vector <PolygonType> xx_polygons;
  size_t xx_n = std::numeric_limits<size_t>::max ();
  if (xx_set) {

    db::cut_polygon (polygon, edge_type (point_type (xx, 0), point_type (xx, 1)), std::back_inserter (xx_polygons));
    db::cut_polygon (polygon, edge_type (point_type (xx, 1), point_type (xx, 0)), std::back_inserter (xx_polygons));
    
    xx_n = 0;
    for (typename std::vector <PolygonType>::const_iterator p = xx_polygons.begin (); p != xx_polygons.end (); ++p) {
      xx_n += p->vertices ();
    }

  }

  std::vector <PolygonType> yy_polygons;
  size_t yy_n = std::numeric_limits<size_t>::max ();
  if (yy_set) {

    db::cut_polygon (polygon, edge_type (point_type (0, yy), point_type (1, yy)), std::back_inserter (yy_polygons));
    db::cut_polygon (polygon, edge_type (point_type (1, yy), point_type (0, yy)), std::back_inserter (yy_polygons));
    
    yy_n = 0;
    for (typename std::vector <PolygonType>::const_iterator p = yy_polygons.begin (); p != yy_polygons.end (); ++p) {
      yy_n += p->vertices ();
    }

  }

  if (xx_n < yy_n) {
    output.swap (xx_polygons);
  } else {
    output.swap (yy_polygons);
  }
}

template DB_PUBLIC void split_polygon<> (const db::Polygon &polygon, std::vector<db::Polygon> &output);
template DB_PUBLIC void split_polygon<> (const db::SimplePolygon &polygon, std::vector<db::SimplePolygon> &output);
template DB_PUBLIC void split_polygon<> (const db::DPolygon &polygon, std::vector<db::DPolygon> &output);
template DB_PUBLIC void split_polygon<> (const db::DSimplePolygon &polygon, std::vector<db::DSimplePolygon> &output);

// -------------------------------------------------------------------------
//  Smoothing tools

void 
smooth_contour (db::Polygon::polygon_contour_iterator from, db::Polygon::polygon_contour_iterator to, std::vector <db::Point> &points, db::Coord d, bool keep_hv)
{
  points.clear ();
  points.reserve (std::distance (from, to));

  std::vector<db::Point> org_points;
  std::vector<size_t> point_indexes;
  point_indexes.reserve (std::distance (from, to));

  //  collect the points into a vector

  size_t pi = 0;
  for (db::Polygon::polygon_contour_iterator p = from; p != to; ++p, ++pi) {
    points.push_back (*p);
    point_indexes.push_back (pi);
  }

  org_points = points;

  //  proceed until there is nothing to do
  
  bool even = false;
  int cont = 2;
  while (points.size () >= 3 && cont > 0) {

    std::vector<db::Point> new_points;
    new_points.reserve (points.size ());
    std::vector<size_t> new_point_indexes;
    new_point_indexes.reserve (points.size ());

    bool any = false;

    size_t i;
    bool first_point_deleted = false;
    for (i = (even ? 0 : 1); i < points.size (); i += 2) {

      if (i == points.size () - 1 && first_point_deleted) {
        break;
      }

      db::Point pm1 = points [(i + points.size () - 2) % points.size ()];
      db::Point p0 = points [(i + points.size () - 1) % points.size ()];
      db::Point p1 = points [i];
      db::Point p2 = points [(i + 1) % points.size ()];

      size_t pi0 = point_indexes [(i + points.size () - 1) % points.size ()];
      size_t pi1 = point_indexes [i];
      size_t pi2 = point_indexes [(i + 1) % points.size ()];

      if (i > 0) {
        new_points.push_back (p0);
        new_point_indexes.push_back (pi0);
      }

      bool can_drop = false;

      if (keep_hv && (p1.x () == p0.x () || p1.y () == p0.y () || p2.x () == p1.x () || p2.y () == p1.y ())) {
        //  keep points which participate in either a vertical or horizontal edge
      } else if (db::Coord (p1.distance(p0)) <= d && db::sprod_sign (p2 - p1, p0 - pm1) > 0 && std::abs (db::vprod (p2 - p1, p0 - pm1)) < 0.8 * p2.distance (p1) * p0.distance (pm1)) {
        //  jog configurations with small edges are candidates
        can_drop = true;
      } else if (db::vprod_sign (p2 - p1, p1 - p0) < 0) {
        //  concave corners are always candidates
        can_drop = true;
      } else {
        //  convex corners enclosing a little more than 45 degree are candidates too
        can_drop = (db::sprod_sign (p2 - p1, p1 - p0) > 0 && std::abs (db::vprod (p2 - p1, p1 - p0)) < 0.8 * p2.distance (p1) * p1.distance (p0));
      }

      for (size_t j = pi0; can_drop; ) {
        if (std::abs (db::Edge (p0, p2).distance (org_points [j])) > d) {
          can_drop = false;
        }
        if (j == pi2) {
          break;
        }
        ++j;
        if (j == org_points.size ()) {
          j = 0;
        }
      }

      if (can_drop) {
        //  drop this point
        any = true;
        if (i == 0) {
          first_point_deleted = true;
        }
      } else {
        new_points.push_back (p1);
        new_point_indexes.push_back (pi1);
      }

    }

    if (any) {
      cont = 2;
    } else {
      cont -= 1;
    }

    while (i <= points.size ()) {
      new_points.push_back (points [i - 1]);
      new_point_indexes.push_back (point_indexes [i - 1]);
      ++i;
    }

    points.swap (new_points);
    point_indexes.swap (new_point_indexes);

    even = !even;

  }
}

db::Polygon 
smooth (const db::Polygon &polygon, db::Coord d, bool keep_hv)
{
  db::Polygon new_poly;
  std::vector <db::Point> new_pts;

  smooth_contour (polygon.begin_hull (), polygon.end_hull (), new_pts, d, keep_hv);
  if (new_pts.size () >= 3) {

    new_poly.assign_hull (new_pts.begin (), new_pts.end (), false /*don't compress*/);

    for (unsigned int h = 0; h < polygon.holes (); ++h) {
      new_pts.clear ();
      smooth_contour (polygon.begin_hole (h), polygon.end_hole (h), new_pts, d, keep_hv);
      if (new_pts.size () >= 3) {
        new_poly.insert_hole (new_pts.begin (), new_pts.end (), false /*don't compress*/);
      }
    }

    new_poly.sort_holes ();

  }

  return new_poly;
}

// -------------------------------------------------------------------------
//  Strange polygons

namespace
{

/**
 *  @brief A helper class to implement the strange polygon detector
 */
struct StrangePolygonInsideFunc
{
  inline bool operator() (int wc) const
  {
    return wc < 0 || wc > 1;
  }
};

/**
 *  @brief A helper class to implement the non-orientable polygon detector
 */
struct NonOrientablePolygonFunc
{
  inline bool operator() (int wc) const
  {
    //  As polygon contours are normalized by default to positive wrap count a negative wrap count
    //  indicates non-orientability
    return wc < 0;
  }
};

/**
 *  @brief An exception type indicating a strange polygon
 */
struct OddPolygonException
{
  OddPolygonException () { }
};

/**
 *  @brief An edge processor catching the error
 */
class ErrorCatchingEdgeSink
  : public db::EdgeSink
{
  //  TODO: we should not use exceptions to indicate a condition, but right now, there is no good alternative
  //  and this is considered an error anyway.
  virtual void put (const db::Edge &) { throw OddPolygonException (); }
  virtual void put (const db::Edge &, int) { }
  virtual void crossing_edge (const db::Edge &) { throw OddPolygonException (); }
};

}

template <class F>
bool
check_wrapcount (const db::Polygon &poly, std::vector<db::Polygon> *error_parts)
{
  size_t vn = poly.vertices ();
  if (vn < 4 || (vn == 4 && poly.is_box ())) {
    return false;
  }

  EdgeProcessor ep;
  ep.insert (poly);

  F inside;
  db::GenericMerge<F> op (inside);

  if (error_parts) {

    db::PolygonContainer pc (*error_parts, false);
    db::PolygonGenerator pg (pc, false, false);
    ep.process (pg, op);

    return ! error_parts->empty ();

  } else {

    try {
      ErrorCatchingEdgeSink es;
      ep.process (es, op);
    } catch (OddPolygonException &) {
      return true;
    }

    return false;

  }
}

bool
is_strange_polygon (const db::Polygon &poly, std::vector<db::Polygon> *strange_parts)
{
  return check_wrapcount<StrangePolygonInsideFunc> (poly, strange_parts);
}

bool
is_non_orientable_polygon (const db::Polygon &poly, std::vector<db::Polygon> *strange_parts)
{
  return check_wrapcount<NonOrientablePolygonFunc> (poly, strange_parts);
}

// -------------------------------------------------------------------------
//  Rounding tools

template <class C>
static bool
do_extract_rad_from_contour (typename db::polygon<C>::polygon_contour_iterator from, typename db::polygon<C>::polygon_contour_iterator to, double &rinner, double &router, unsigned int &n, std::vector <db::point<C> > *new_pts, bool fallback)
{
  if (from == to) {
    return false;
  }

  typename db::polygon<C>::polygon_contour_iterator p0 = from;

  typename db::polygon<C>::polygon_contour_iterator p1 = p0;
  ++p1;
  if (p1 == to) {
    p1 = from;
  }
  typename db::polygon<C>::polygon_contour_iterator p2 = p1;

  const double cos_thr = 0.8;
  const double acute_cos_thr = -0.8;
  const double circle_segment_thr = 2.5;

  //  search for the first circle segment (where cos(a) > cos_thr) 
  double ls_inner = 0.0, ls_outer = 0.0;
  unsigned long n_ls_inner = 0, n_ls_outer = 0;

  if (! fallback) {

    do {

      ++p2;
      if (p2 == to) {
        p2 = from;
      }

      db::edge<C> ep (*p0, *p1);
      db::edge<C> e (*p1, *p2);

      bool inner = (db::vprod_sign (ep, e) > 0);
      double &ls = inner ? ls_inner : ls_outer;
      unsigned long &n_ls = inner ? n_ls_inner : n_ls_outer;

      if (db::sprod (ep, e) > cos_thr * e.double_length () * ep.double_length ()) {
        ls += std::min (e.double_length (), ep.double_length ());
        ++n_ls;
      }

      p0 = p1;
      p1 = p2;

    } while (p0 != from);

    if (n_ls_inner > 0) {
      ls_inner /= n_ls_inner;
    }
    if (n_ls_outer > 0) {
      ls_outer /= n_ls_outer;
    }

  }

  bool found = false;

  //  search for the first circle segment (where cos(a) > cos_thr) 
  //  or a long segment is followed by a short one or the curvature changes.

  typename db::polygon<C>::polygon_contour_iterator pm1 = from;

  p0 = pm1;
  ++p0;
  if (p0 == to) {
    p0 = from;
  }

  p1 = p0;
  ++p1;
  if (p1 == to) {
    p1 = from;
  }

  p2 = p1;
  ++p2;
  if (p2 == to) {
    p2 = from;
  }

  typename db::polygon<C>::polygon_contour_iterator p3 = p2;

  do {

    ++p3;
    if (p3 == to) {
      p3 = from;
    }

    db::edge<C> em (*pm1, *p0);
    db::edge<C> ep (*p0, *p1);
    db::edge<C> e (*p1, *p2);
    db::edge<C> en (*p2, *p3);

    bool first_or_last = fallback || (e.double_length () > circle_segment_thr * ep.double_length () || ep.double_length () > circle_segment_thr * e.double_length ()) 
                                  || (db::vprod_sign (em, ep) * db::vprod_sign (ep, e) < 0 || db::vprod_sign (ep, e) * db::vprod_sign (e, en) < 0);

    if (first_or_last && db::sprod (ep, e) > cos_thr * e.double_length () * ep.double_length ()) {
      double ls = (db::vprod_sign (ep, e) > 0 ? ls_inner : ls_outer);
      if (! fallback && ((e.double_length () < circle_segment_thr * ls && ep.double_length () > circle_segment_thr * ls) 
                         || db::vprod_sign (em, ep) * db::vprod_sign (ep, e) < 0)) {
        found = true;
        break;
      } else if (fallback && (ep.dx () == 0 || ep.dy () == 0)) {
        found = true;
        break;
      }
    }

    pm1 = p0;
    p0 = p1;
    p1 = p2;
    p2 = p3;

  } while (pm1 != from);

  if (! found) {
    return false;
  }

  //  create a list of new points without the rounded corners and compute rounding radii

  if (new_pts) {
    new_pts->clear ();
  }

  typename db::polygon<C>::polygon_contour_iterator pfirst = p0;
  bool in_corner = false;
  double ls_corner = 0.0;
  db::edge<C> elast;
  typename db::polygon<C>::polygon_contour_iterator plast;
  double asum = 0.0;
  unsigned int nseg = 0;

  double rxi_sum = 0.0;
  double rxo_sum = 0.0;
  double da_sum = 0.0;
  int n_corners = 0;
  int ni_corners = 0;
  int no_corners = 0;

  p3 = p2;

  do {

    ++p3;
    if (p3 == to) {
      p3 = from;
    }

    db::edge<C> em (*pm1, *p0);
    db::edge<C> ep (*p0, *p1);
    db::edge<C> e (*p1, *p2);
    db::edge<C> en (*p2, *p3);

    //  Heuristic detection of a new circle segment: 
    //  In fallback mode vertical or horizontal edges separate circle segments.
    //  In non-fallback mode either a long edge followed by a short one indicates the beginning of a new
    //  circle segment or a circle segment is detected when the curvature changes.
    //  The latter case detects situations where two circle segments directly attach to each other
    //  with different bending direction.

    bool first_or_last = fallback || (e.double_length () > circle_segment_thr * ep.double_length () || ep.double_length () > circle_segment_thr * e.double_length ())
                                  || (db::vprod_sign (em, ep) * db::vprod_sign (ep, e) < 0 || db::vprod_sign (ep, e) * db::vprod_sign (e, en) < 0);

    if (db::sprod (ep, e) > cos_thr * e.double_length () * ep.double_length ()) { 

      double ls = db::vprod_sign (ep, e) > 0 ? ls_inner : ls_outer;

      if ((! fallback && first_or_last && ((e.double_length () < circle_segment_thr * ls && ep.double_length () > circle_segment_thr * ls) || db::vprod_sign (em, ep) * db::vprod_sign (ep, e) < 0)) ||
          (fallback && (ep.dx () == 0 || ep.dy () == 0))) {

        if (! in_corner) {
          elast = ep;
          plast = p1;
          asum = db::vprod (*p1 - db::point<C> (), *p2 - db::point<C> ());
          nseg = 1;
          ls_corner = ls;
        } 
        in_corner = true;

      } else if ((!fallback && first_or_last && ((e.double_length () > circle_segment_thr * ls_corner && ep.double_length () < circle_segment_thr * ls_corner) || db::vprod_sign (ep, e) * db::vprod_sign (e, en) < 0)) ||
                 (fallback && (e.dx () == 0 || e.dy () == 0))) {

        if (in_corner) {

          std::pair<bool, db::point<C> > cp = elast.cut_point (e);
          if (! cp.first || db::sprod (elast, e) < acute_cos_thr * elast.double_length () * e.double_length ()) {

            //  We have a full 180 degree bend without a stop (actually two corners).
            //  Use the segment in between that is perpendicular to the start and end segment as stop edge.
            typename db::polygon<C>::polygon_contour_iterator pp1 = plast;
            typename db::polygon<C>::polygon_contour_iterator pp2 = pp1;
            double asum_part = 0.0;
            unsigned int nseg_part = 0;

            while (pp1 != p1) {

              ++pp2;
              if (pp2 == to) {
                pp2 = from;
              }

              e = db::edge<C> (*pp1, *pp2);
              if (db::sprod_sign (elast, e) <= 0) {
                break;
              }

              asum_part += db::vprod (*pp1 - db::point<C> (), *pp2 - db::point<C> ());
              ++nseg_part;

              pp1 = pp2;

            }

            ++nseg_part;

            if (nseg_part >= nseg) {
              //  not a valid rounded bend - skip this solution
              return false;
            }

            cp = elast.cut_point (e);
            if (! cp.first) {
              return false;
            }

            if (new_pts) {
              new_pts->push_back (cp.second);
            }

            asum -= asum_part;
            asum -= db::vprod (e.p1 () - db::point<C> (), e.p2 () - db::point<C> ());
            nseg -= nseg_part;

            asum_part += db::vprod (cp.second - db::point<C> (), elast.p2 () - db::point<C> ());
            asum_part += db::vprod (*pp1 - db::point<C> (), cp.second - db::point<C> ());

            double sin_atot = db::vprod (elast, e);
            double cos_atot = db::sprod (elast, e);
            double atot = fabs (atan2 (sin_atot, cos_atot));

            double rx = sqrt (fabs (asum_part) * 0.5 / (tan (atot * 0.5) - tan (atot * 0.5 / nseg_part) * nseg_part));
            double da = atot / nseg_part;

            if (sin_atot > 0.0) {
              rxi_sum += rx;
              ++ni_corners;
            } else {
              rxo_sum += rx;
              ++no_corners;
            }

            da_sum += da;
            ++n_corners;

            elast = e;

            e = db::edge<C> (*p1, *p2);
            cp = elast.cut_point (e);
            if (! cp.first) {
              return false;
            }

          }

          if (new_pts) {
            new_pts->push_back (cp.second);
          }

          asum += db::vprod (cp.second - db::point<C> (), elast.p2 () - db::point<C> ());
          asum += db::vprod (*p1 - db::point<C> (), cp.second - db::point<C> ());

          ++nseg;

          double sin_atot = db::vprod (elast, e);
          double cos_atot = db::sprod (elast, e);
          double atot = fabs (atan2 (sin_atot, cos_atot));

          double rx = sqrt (fabs (asum) * 0.5 / (tan (atot * 0.5) - tan (atot * 0.5 / nseg) * nseg));
          double da = atot / nseg;

          if (sin_atot > 0.0) {
            rxi_sum += rx;
            ++ni_corners;
          } else {
            rxo_sum += rx;
            ++no_corners;
          }

          da_sum += da;
          ++n_corners;

        }
        in_corner = false;

      } else if (in_corner) {

        asum += db::vprod (*p1 - db::point<C> (), *p2 - db::point<C> ());
        ++nseg;

      } else {
        if (new_pts) {
          new_pts->push_back (*p1);
        }
      }

    } else {
      if (new_pts) {
        new_pts->push_back (*p1);
      }
    }

    pm1 = p0;
    p0 = p1;
    p1 = p2;
    p2 = p3;

  } while (p0 != pfirst);

  if (n_corners < 2) {

    return false;

  } else {
    n = (unsigned int) floor (2.0 * M_PI / (da_sum / n_corners) + 0.5);
    if (ni_corners > 0) {
      rinner = floor ((rxi_sum / ni_corners * 0.5) + 0.5) * 2;
    } 
    if (no_corners > 0) {
      router = floor ((rxo_sum / no_corners * 0.5) + 0.5) * 2;
    }
    return true;
  }
}

bool
extract_rad_from_contour (db::Polygon::polygon_contour_iterator from, db::Polygon::polygon_contour_iterator to, double &rinner, double &router, unsigned int &n, std::vector <db::Point> *new_pts, bool fallback)
{
  return do_extract_rad_from_contour (from, to, rinner, router, n, new_pts, fallback);
}

bool
extract_rad_from_contour (db::DPolygon::polygon_contour_iterator from, db::DPolygon::polygon_contour_iterator to, double &rinner, double &router, unsigned int &n, std::vector <db::DPoint> *new_pts, bool fallback)
{
  return do_extract_rad_from_contour (from, to, rinner, router, n, new_pts, fallback);
}

template <class C>
static bool
do_extract_rad (const db::polygon<C> &polygon, double &rinner, double &router, unsigned int &n, db::polygon<C> *new_polygon)
{
  if (new_polygon) {

    std::vector <db::point<C> > new_pts;

    if (! do_extract_rad_from_contour (polygon.begin_hull (), polygon.end_hull (), rinner, router, n, &new_pts, false) &&
        ! do_extract_rad_from_contour (polygon.begin_hull (), polygon.end_hull (), rinner, router, n, &new_pts, true)) {
      //  no radius found
      return false;
    } else {
      new_polygon->assign_hull (new_pts.begin (), new_pts.end ());
    }

    for (unsigned int h = 0; h < polygon.holes (); ++h) {

      new_pts.clear ();
      if (! do_extract_rad_from_contour (polygon.begin_hole (h), polygon.end_hole (h), rinner, router, n, &new_pts, false) &&
          ! do_extract_rad_from_contour (polygon.begin_hole (h), polygon.end_hole (h), rinner, router, n, &new_pts, true)) {
        //  no radius found
        return false;
      } else {
        new_polygon->insert_hole (new_pts.begin (), new_pts.end ());
      }

    }

    new_polygon->sort_holes ();

  } else {

    if (! do_extract_rad_from_contour (polygon.begin_hull (), polygon.end_hull (), rinner, router, n, (std::vector<db::point<C> > *) 0, false)) {
      if (! do_extract_rad_from_contour (polygon.begin_hull (), polygon.end_hull (), rinner, router, n, (std::vector<db::point<C> > *) 0, true)) {
        return false;
      }
    }

    for (unsigned int h = 0; h < polygon.holes (); ++h) {
      if (! do_extract_rad_from_contour (polygon.begin_hole (h), polygon.end_hole (h), rinner, router, n, (std::vector<db::point<C> > *) 0, false)) {
        if (! do_extract_rad_from_contour (polygon.begin_hole (h), polygon.end_hole (h), rinner, router, n, (std::vector<db::point<C> > *) 0, true)) {
          return false;
        }
      }
    }

  }

  return true;

}

bool
extract_rad (const db::Polygon &polygon, double &rinner, double &router, unsigned int &n, db::Polygon *new_polygon)
{
  return do_extract_rad (polygon, rinner, router, n, new_polygon);
}

bool
extract_rad (const db::DPolygon &polygon, double &rinner, double &router, unsigned int &n, db::DPolygon *new_polygon)
{
  return do_extract_rad (polygon, rinner, router, n, new_polygon);
}


template <class C>
static void
do_compute_rounded_contour (typename db::polygon<C>::polygon_contour_iterator from, typename db::polygon<C>::polygon_contour_iterator to, std::vector <db::point<C> > &new_pts, double rinner, double router, unsigned int n)
{
  std::vector<db::point<C> > points;

  //  collect the points into a vector

  if (from != to) {

    typename db::polygon<C>::polygon_contour_iterator p0 = from;
    typename db::polygon<C>::polygon_contour_iterator p1 = p0;
    ++p1;
    if (p1 == to) {
      p1 = from;
    }
    typename db::polygon<C>::polygon_contour_iterator p2 = p1;

    do {

      ++p2;
      if (p2 == to) {
        p2 = from;
      }

      if (! db::edge<C> (*p0, *p1).parallel (db::edge<C> (*p1, *p2))) {
        points.push_back (*p1);
      }

      p0 = p1;
      p1 = p2;

    } while (p0 != from);

  }

  //  compute the radii and segment length 

  std::vector<double> rad (points.size (), 0.0);
  std::vector<double> seg (points.size (), 0.0);

  for (size_t i = 0; i < points.size (); ++i) {

    db::point<C>  p0 = points [(i + points.size () - 1) % points.size ()];
    db::point<C>  p1 = points [i];
    db::point<C>  p2 = points [(i + points.size () + 1) % points.size ()];

    db::DVector e1 = (db::DPoint (p1) - db::DPoint (p0)) * (1.0 / p0.double_distance (p1));
    db::DVector e2 = (db::DPoint (p2) - db::DPoint (p1)) * (1.0 / p1.double_distance (p2));

    double sin_a = db::vprod (e1, e2);
    double cos_a = db::sprod (e1, e2);
    double a = fabs (atan2 (sin_a, cos_a));

    double r = sin_a > 0.0 ? rinner : router;
    double s = r * fabs (sin (a * 0.5) / cos (a * 0.5));

    rad [i] = r;
    seg [i] = s;

  }

  //  compute the rounded points

  for (size_t i = 0; i < points.size (); ++i) {

    db::point<C>  p0 = points [(i + points.size () - 1) % points.size ()];
    db::point<C>  p1 = points [i];
    db::point<C>  p2 = points [(i + points.size () + 1) % points.size ()];

    db::DVector e1 = (db::DPoint (p1) - db::DPoint (p0)) * (1.0 / p0.double_distance (p1));
    db::DVector e2 = (db::DPoint (p2) - db::DPoint (p1)) * (1.0 / p1.double_distance (p2));

    double sin_a = db::vprod (e1, e2);
    double cos_a = db::sprod (e1, e2);
    double a = fabs (atan2 (sin_a, cos_a));

    double s0 = seg [(i + points.size () - 1) % points.size ()];
    double s1 = seg [i];
    double s2 = seg [(i + points.size () + 1) % points.size ()];

    double f0 = std::min (1.0, p0.double_distance (p1) / (s0 + s1));
    double f1 = std::min (1.0, p1.double_distance (p2) / (s1 + s2));
    double r = std::min (f0, f1) * rad [i];

    if (r > 0.0) {

      db::DPoint q0 = db::DPoint (p1) - e1 * (tan (a * 0.5) * r);
      db::DVector n1;
      if (sin_a > 0) {
        n1 = db::DVector (e1.y (), -e1.x ());
      } else {
        n1 = db::DVector (-e1.y (), e1.x ());
      }
      db::DPoint pr = q0 - n1 * r;

      double ares = (2.0 * M_PI) / double (n);
      unsigned int nseg = (unsigned int) floor (a / ares + 0.5);
      if (nseg == 0) {
        new_pts.push_back (p1);
      } else {

        double da = a / floor (a / ares + 0.5);
        for (double aa = 0.0; aa < a - 1e-6; aa += da) {

          db::DPoint q1 = pr + n1 * (r * cos (aa + da)) + e1 * (r * sin (aa + da));

          //  do a interpolation by computing the crossing point of the tangents of the
          //  circle at a and a+da. This scheme guarantees a low distortion of the original
          //  polygon and enables reverting back to the original polygon to some degree.
          db::DPoint qm = q0 + (q1 - q0) * 0.5;
          db::DPoint q = qm + (qm - pr) * (q0.sq_distance (qm) / pr.sq_distance (qm));

          new_pts.push_back (db::point<C> (q));

          q0 = q1;

        }

      }

    } else {
      new_pts.push_back (p1);
    }

  } 
}

void
compute_rounded_contour (db::Polygon::polygon_contour_iterator from, db::Polygon::polygon_contour_iterator to, std::vector <db::Point> &new_pts, double rinner, double router, unsigned int n)
{
  do_compute_rounded_contour (from, to, new_pts, rinner, router, n);
}

void
compute_rounded_contour (db::DPolygon::polygon_contour_iterator from, db::DPolygon::polygon_contour_iterator to, std::vector <db::DPoint> &new_pts, double rinner, double router, unsigned int n)
{
  do_compute_rounded_contour (from, to, new_pts, rinner, router, n);
}

template <class C>
static db::polygon<C>
do_compute_rounded (const db::polygon<C> &polygon, double rinner, double router, unsigned int n)
{
  db::polygon<C> new_poly;
  std::vector <db::point<C> > new_pts;

  compute_rounded_contour (polygon.begin_hull (), polygon.end_hull (), new_pts, rinner, router, n);
  new_poly.assign_hull (new_pts.begin (), new_pts.end (), false /*don't compress*/);

  for (unsigned int h = 0; h < polygon.holes (); ++h) {
    new_pts.clear ();
    compute_rounded_contour (polygon.begin_hole (h), polygon.end_hole (h), new_pts, rinner, router, n);
    new_poly.insert_hole (new_pts.begin (), new_pts.end (), false /*don't compress*/);
  }

  new_poly.sort_holes ();

  return new_poly;
}

db::Polygon
compute_rounded (const db::Polygon &polygon, double rinner, double router, unsigned int n)
{
  return do_compute_rounded (polygon, rinner, router, n);
}

db::DPolygon
compute_rounded (const db::DPolygon &polygon, double rinner, double router, unsigned int n)
{
  return do_compute_rounded (polygon, rinner, router, n);
}

// -------------------------------------------------------------------------
//  Implementation of AreaMap

AreaMap::AreaMap ()
  : m_nx (0), m_ny (0)
{
  mp_av = 0;
}

AreaMap::AreaMap (const AreaMap &other)
  : m_nx (0), m_ny (0)
{
  mp_av = 0;
  operator= (other);
}

AreaMap &
AreaMap::operator= (const AreaMap &other)
{
  if (this != &other) {
    //  TODO: this could be copy on write
    reinitialize (other.p0 (), other.d (), other.p (), other.nx (), other.ny ());
    if (other.mp_av) {
      memcpy (mp_av, other.mp_av, m_nx * m_ny * sizeof (*mp_av));
    }
  }
  return *this;
}

AreaMap::AreaMap (const db::Point &p0, const db::Vector &d, size_t nx, size_t ny)
  : m_p0 (p0), m_d (d), m_p (d), m_nx (nx), m_ny (ny)
{
  mp_av = new area_type [nx * ny];
  clear ();
}

AreaMap::AreaMap (const db::Point &p0, const db::Vector &d, const db::Vector &p, size_t nx, size_t ny)
  : m_p0 (p0), m_d (d), m_p (std::min (d.x (), p.x ()), std::min (d.y (), p.y ())), m_nx (nx), m_ny (ny)
{
  mp_av = new area_type [nx * ny];
  clear ();
}

AreaMap::~AreaMap ()
{
  if (mp_av) {
    delete[] mp_av;
  }
  mp_av = 0;
}

void
AreaMap::reinitialize (const db::Point &p0, const db::Vector &d, size_t nx, size_t ny)
{
  reinitialize (p0, d, d, nx, ny);
}

void
AreaMap::reinitialize (const db::Point &p0, const db::Vector &d, const db::Vector &p, size_t nx, size_t ny)
{
  m_p0 = p0;
  m_d = d;
  m_p = db::Vector (std::min (d.x (), p.x ()), std::min (d.y (), p.y ()));

  if (nx != m_nx || ny != m_ny) {

    m_nx = nx;
    m_ny = ny;

    if (mp_av) {
      delete[] mp_av;
    }

    mp_av = new area_type [nx * ny];

  }

  clear ();
}

void
AreaMap::clear ()
{
  if (mp_av) {
    area_type *a = mp_av;
    for (size_t n = m_nx * m_ny; n > 0; --n) {
      *a++ = 0;
    }
  }
}

void
AreaMap::swap (AreaMap &other)
{
  std::swap (m_p0, other.m_p0);
  std::swap (m_d, other.m_d);
  std::swap (m_p, other.m_p);
  std::swap (m_nx, other.m_nx);
  std::swap (m_ny, other.m_ny);
  std::swap (mp_av, other.mp_av);
}

AreaMap::area_type 
AreaMap::total_area () const
{
  area_type asum = 0;
  if (mp_av) {
    const area_type *a = mp_av;
    for (size_t n = m_nx * m_ny; n > 0; --n) {
      asum += *a++;
    }
  }
  return asum;
}

db::Box
AreaMap::bbox () const
{
  if (m_nx == 0 || m_ny == 0) {
    return db::Box ();
  } else {
    return db::Box (m_p0, m_p0 + db::Vector (db::Coord (m_nx - 1) * m_d.x () + m_p.x (), db::Coord (m_ny - 1) * m_d.y () + m_p.y ()));
  }
}

// -------------------------------------------------------------------------
//  Implementation of rasterize

static bool edge_is_partially_left_of (const db::Edge &e, const db::Edge &e_original, db::Coord x)
{
  Coord xmin = db::edge_xmin (e);
  if (xmin < x) {
    return true;
  } else if (xmin == x && e_original.dx () != 0) {
    //  the skew edge is cut partially rendering a straight vertical line (due to rounding)
    //  which we will count as "left of"
    return true;
  } else {
    return false;
  }
}

bool
rasterize (const db::Polygon &polygon, db::AreaMap &am)
{
  typedef db::AreaMap::area_type area_type;
  db::Box box = am.bbox ();
  db::Box pbox = polygon.box ();

  //  check if the polygon overlaps the rasterization area. Otherwise, we simply do nothing.
  if (! pbox.overlaps (box)) {
    return false;
  }

  db::Coord ymin = box.bottom (), ymax = box.top ();
  db::Coord dy = am.d ().y (), dx = am.d ().x ();
  db::Coord py = am.p ().y (), px = am.p ().x ();
  db::Coord y0 = am.p0 ().y (), x0 = am.p0 ().x ();
  size_t ny = am.ny (), nx = am.nx ();

  size_t iy0 = std::min (ny, size_t (std::max (db::Coord (0), (pbox.bottom () - am.p0 ().y ()) / am.d ().y ())));
  size_t iy1 = std::min (ny, size_t (std::max (db::Coord (0), (pbox.top () - am.p0 ().y () + am.d ().y () - 1) / am.d ().y ())));

  size_t ix0 = std::min (nx, size_t (std::max (db::Coord (0), (pbox.left () - am.p0 ().x ()) / am.d ().x ())));
  size_t ix1 = std::min (nx, size_t (std::max (db::Coord (0), (pbox.right () - am.p0 ().x () + am.d ().x () - 1) / am.d ().x ())));

  //  no scanning required (i.e. degenerated polygon) -> do nothing 
  if (iy0 == iy1 || ix0 == ix1) {
    return false;
  }

  //  collect edges 
  size_t n = 0;
  for (db::Polygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
    if ((*e).dy () != 0 && db::edge_ymax (*e) > ymin && db::edge_ymin (*e) < ymax) {
      ++n;
    }
  }

  std::vector <db::Edge> edges;
  edges.reserve (n);
  for (db::Polygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
    if ((*e).dy () != 0 && db::edge_ymax (*e) > ymin && db::edge_ymin (*e) < ymax) {
      edges.push_back (*e);
    }
  }

  //  sort edges
  std::sort (edges.begin (), edges.end (), db::edge_ymin_compare<db::Coord> ());

  std::vector <db::Edge>::iterator c = edges.begin ();

  db::Coord y = y0 + dy * db::Coord (iy0);

  while (c != edges.end () && db::edge_ymax (*c) <= y) {
    ++c;
  }

  if (c == edges.end ()) {
    return false;
  }

  std::vector <db::Edge>::iterator f = c;

  for (size_t iy = iy0; iy < iy1; ++iy) {

    db::Coord yy = y + py;
    while (f != edges.end () && db::edge_ymin (*f) < yy) {
      ++f;
    }

    std::sort (c, f, db::edge_xmin_compare <db::Coord> ());

    db::Coord x = x0 + dx * db::Coord (ix0);
    db::Coord xl = pbox.left ();
    area_type a = 0;

    std::vector <db::Edge>::iterator cc = c;

    while (cc != edges.end () && cc != f && db::edge_xmax (*cc) <= x) {
      db::Coord y1 = std::max (y, std::min (yy, cc->p1 ().y ()));
      db::Coord y2 = std::max (y, std::min (yy, cc->p2 ().y ()));
      a += area_type (px) * area_type (y2 - y1);
      ++cc;
    }

    std::vector <db::Edge>::iterator ff = cc;

    for (size_t ix = ix0; ix < ix1; ++ix) {

      db::Coord xx = x + px;
      db::Coord xxx = x + dx;

      // TODO: edge_xmin_at_interval(y, yy) and edge_xmax.. would be more efficient in the
      // all-angle case. However, it is crucial that the edge clipping produces 
      // connected edge segments and it is questionable whether the at_interval 
      // functions produce a sorting/filter criterion compatible with the clip.

      while (ff != f && db::edge_xmin (*ff) < xx) {
        ++ff;
      }

      std::vector <db::Edge>::iterator fff = ff;

      if (xx < xxx) {
        while (fff != f && db::edge_xmin (*fff) < xxx) {
          ++fff;
        }
      }

      if (xl < x) {

        //  consider all edges or parts of those left of the first cell
        db::Box left (xl, y, x, yy);

        for (std::vector <db::Edge>::iterator e = cc; e != ff; ++e) {

          std::pair<bool, db::Edge> ec = e->clipped (left);
          if (ec.first && edge_is_partially_left_of (ec.second, *e, x)) {
            a += area_type (ec.second.dy ()) * area_type (px);
          }

        }

      }

      area_type aa = a;

      if (dx == py) {

        db::Box cell (x, y, xx, yy);

        for (std::vector <db::Edge>::iterator e = cc; e != ff; ++e) {

          std::pair<bool, db::Edge> ec = e->clipped (cell);
          if (ec.first && edge_is_partially_left_of (ec.second, *e, xx)) {
            aa += (area_type (ec.second.dy ()) * area_type (2 * xx - (ec.second.p2 ().x () + ec.second.p1 ().x ()))) / 2;
            a += area_type (ec.second.dy ()) * area_type (px);
          }

        }

      } else {

        db::Box cell (x, y, xx, yy);

        for (std::vector <db::Edge>::iterator e = cc; e != ff; ++e) {

          std::pair<bool, db::Edge> ec = e->clipped (cell);
          if (ec.first && edge_is_partially_left_of (ec.second, *e, xx)) {
            aa += (area_type (ec.second.dy ()) * area_type (2 * xx - (ec.second.p2 ().x () + ec.second.p1 ().x ()))) / 2;
          }

        }

        db::Box wide_cell (x, y, x + dx, yy);

        for (std::vector <db::Edge>::iterator e = cc; e != fff; ++e) {

          std::pair<bool, db::Edge> wide_ec = e->clipped (wide_cell);
          if (wide_ec.first && edge_is_partially_left_of (wide_ec.second, *e, x + dx)) {
            a += area_type (wide_ec.second.dy ()) * area_type (px);
          }

        }

      }

      am.get (ix, iy) += aa;

      x += dx;
      xl = x;

      ff = fff;

      for (std::vector <db::Edge>::iterator ccx = cc; ccx != ff; ++ccx) {
        if (db::edge_xmax (*ccx) <= x) {
          std::swap (*ccx, *cc);
          ++cc;
        }
      }

    }

    if (yy < y + dy) {
      yy = y + dy;
      while (f != edges.end () && db::edge_ymin (*f) < yy) {
        ++f;
      }
    }

    y = yy;

    for (std::vector <db::Edge>::iterator cx = c; cx != f; ++cx) {
      if (db::edge_ymax (*cx) <= y) {
        std::swap (*cx, *c);
        ++c;
      }
    }

  }

  return true;
}

// -------------------------------------------------------------------------
//  Implementation of Minkowski sum

/**
 *  @brief A helper class that produces edges into an EdgeProcessor from a sequence of points
 */
class EdgeInputIterator
{
public:
  EdgeInputIterator (db::EdgeProcessor &ep, bool inverse = false)
    : m_last_set (false), m_last (), mp_ep (&ep), m_inverse (inverse)
  { }

  ~EdgeInputIterator ()
  {
    //  close the polygon
    if (m_last_set && m_last != m_first) {
      if (!m_inverse) {
        mp_ep->insert (db::Edge (m_last, m_first));
      } else {
        mp_ep->insert (db::Edge (m_first, m_last));
      }
      m_last_set = false;
    }

    mp_ep = 0;
  }

  void operator+= (const db::Point &p)
  {
    if (m_last_set) {
      if (!m_inverse) {
        mp_ep->insert (db::Edge (m_last, p));
      } else {
        mp_ep->insert (db::Edge (p, m_last));
      }
    } else {
      m_first = p;
    }

    m_last = p;
    m_last_set = true;
  }

private:
  bool m_last_set;
  db::Point m_last, m_first;
  db::EdgeProcessor *mp_ep;
  bool m_inverse;
};

/**
 *  @brief Produce edges for the partial Minkowski sum of an edge with an input polygon
 */
static void
ms_production (const db::Polygon &a, const db::Point &p1, const db::Point &p2, db::EdgeProcessor &ep)
{
  double d12 = p2.double_distance (p1); 
  db::DPoint d (-double (p2.y () - p1.y ()) / d12, double (p2.x () - p1.x ()) / d12);

  db::EdgeInputIterator e (ep);

  db::Polygon::polygon_contour_iterator ci = a.begin_hull ();
  db::Polygon::polygon_contour_iterator cf = a.end_hull ();

  db::Polygon::polygon_contour_iterator cmin = cf, cmax = cf;
  double pmin = 0.0, pmax = 0.0;

  //  Look for the points in the contour bounding the partial sum perpendicular to the edge
  for (db::Polygon::polygon_contour_iterator c = ci; c != cf; ++c) {

    double p = (*c).x() * d.x () + (*c).y () * d.y ();

    if (cmin == cf || pmin > p) {
      pmin = p;
      cmin = c;
    }

    if (cmax == cf || pmax < p) {
      pmax = p;
      cmax = c;
    }

  }

  tl_assert (cmin != cf && cmax != cf);

  db::Polygon::polygon_contour_iterator c = cmin;
  db::Polygon::polygon_contour_iterator cc = cf;
  db::Polygon::polygon_contour_iterator cl = cf;

  bool pcc_set = false;
  double pcc = 0.0;

  while (true) {

    double pc = (*c).x() * d.x () + (*c).y () * d.y ();

    // detect inversion due to a convex pattern and create a cover polygon for that case
    if (pcc_set) {

      cc = c;
      if (cc == ci) {
        cc = cf;
      }
      --cc;

      if (pcc > pc + 1e-6) {

        if (cl == cf) {
          cl = cc;
        }

      } else if (cl != cf) {

        EdgeInputIterator ee (ep, true);

        // create the cover polygon
        db::Polygon::polygon_contour_iterator k = cl;

        while (k != cc) {
          ee += p1 + (*k - db::Point ());
          if (++k == cf) {
            k = ci;
          }
        }

        ee += p1 + (*k - db::Point ());

        while (k != cl) {
          ee += p2 + (*k - db::Point ());
          if (k == ci) {
            k = cf;
          }
          --k;
        }

        ee += p2 + (*cl - db::Point ());

        cl = cf;

      }

    }

    // produce a new edge
    e += p1 + (*c - db::Point ());

    if (c == cmax) {
      break;
    }

    if (++c == cf) {
      c = ci;
    }

    pcc = pc;
    pcc_set = true;

  }

  cl = cf;
  pcc_set = false;

  while (true) {

    double pc = (*c).x() * d.x () + (*c).y () * d.y ();

    // detect inversion due to a convex pattern and create a cover polygon for that case
    if (pcc_set) {

      cc = c;
      if (cc == ci) {
        cc = cf;
      }
      --cc;

      pcc = (*cc).x() * d.x () + (*cc).y () * d.y ();

      if (pcc < pc - 1e-6) {

        if (cl == cf) {
          cl = cc;
        }

      } else if (cl != cf) {

        EdgeInputIterator ee (ep, true);

        // create the cover polygon
        db::Polygon::polygon_contour_iterator k = cl;

        // create the cover polygon
        while (k != cc) {
          ee += p2 + (*k - db::Point ());
          if (++k == cf) {
            k = ci;
          }
        }

        ee += p2 + (*cc - db::Point ());

        while (k != cl) {
          ee += p1 + (*k - db::Point ());
          if (k == ci) {
            k = cf;
          }
          --k;
        }

        ee += p1 + (*cl - db::Point ());

        cl = cf;

      }

    }

    e += p2 + (*c - db::Point ());

    if (c == cmin) {
      break;
    }

    if (++c == cf) {
      c = ci;
    }

    pcc = pc;
    pcc_set = true;

  }

}

static db::Polygon 
ms_extraction (db::EdgeProcessor &ep, bool resolve_holes)
{
  db::SimpleMerge op (-1);
  std::vector <db::Polygon> polygons;
  db::PolygonContainer pc (polygons);
  db::PolygonGenerator out (pc, resolve_holes, false);
  ep.process (out, op);

  if (polygons.empty ()) {
    return db::Polygon ();
  } else {
    tl_assert (polygons.size () == 1);
    return polygons [0];
  }
}

static db::Polygon 
do_minkowski_sum (const db::Polygon &a, const db::Edge &b, bool resolve_holes)
{
  if (a.begin_hull () == a.end_hull ()) {
    return db::Polygon ();
  }

  db::EdgeProcessor ep;
  db::ms_production (a, b.p1 (), b.p2 (), ep);
  return db::ms_extraction (ep, resolve_holes);
}

db::Polygon 
minkowski_sum (const db::Polygon &a, const db::Edge &b, bool rh)
{
  if (a.holes () > 0) {
    return do_minkowski_sum (db::resolve_holes (a), b, rh);
  } else {
    return do_minkowski_sum (a, b, rh);
  }
}

static db::Polygon 
do_minkowski_sum (const db::Polygon &a, const db::Polygon &b, bool resolve_holes)
{
  if (a.begin_hull () == a.end_hull () || b.begin_hull () == b.end_hull ()) {
    return db::Polygon ();
  }

  db::Vector p0 = *a.begin_hull () - db::Point ();

  db::EdgeProcessor ep;
  for (db::Polygon::polygon_edge_iterator e = b.begin_edge (); ! e.at_end (); ++e) {
    ep.insert (db::Edge ((*e).p1 () + p0, (*e).p2 () + p0));
    db::ms_production (a, (*e).p1 (), (*e).p2 (), ep);
  }

  return db::ms_extraction (ep, resolve_holes);
}

db::Polygon 
minkowski_sum (const db::Polygon &a, const db::Polygon &b, bool rh)
{
  if (a.holes () > 0) {
    return do_minkowski_sum (db::resolve_holes (a), b, rh);
  } else {
    return do_minkowski_sum (a, b, rh);
  }
}

static db::Polygon 
do_minkowski_sum (const db::Polygon &a, const db::Box &b, bool resolve_holes)
{
  return minkowski_sum (a, db::Polygon (b), resolve_holes);
}

db::Polygon 
minkowski_sum (const db::Polygon &a, const db::Box &b, bool rh)
{
  if (a.holes () > 0) {
    return do_minkowski_sum (db::resolve_holes (a), b, rh);
  } else {
    return do_minkowski_sum (a, b, rh);
  }
}

static db::Polygon 
do_minkowski_sum (const db::Polygon &a, const std::vector<db::Point> &c, bool resolve_holes)
{
  db::EdgeProcessor ep;
  for (size_t i = 1; i < c.size (); ++i) {
    db::ms_production (a, c[i - 1], c[i], ep);
  }

  return db::ms_extraction (ep, resolve_holes);
}

db::Polygon 
minkowski_sum (const db::Polygon &a, const std::vector<db::Point> &c, bool rh)
{
  if (a.holes () > 0) {
    return do_minkowski_sum (db::resolve_holes (a), c, rh);
  } else {
    return do_minkowski_sum (a, c, rh);
  }
}

// -------------------------------------------------------------------------
//  Implementation of hole resolution and polygon to simple polygon conversion

db::Polygon 
resolve_holes (const db::Polygon &p)
{
  db::EdgeProcessor ep;
  ep.insert_sequence (p.begin_edge ());

  std::vector<db::Polygon> polygons;
  db::PolygonContainer pc (polygons);
  db::PolygonGenerator out (pc, true /*resolve holes*/, false /*max coherence to get one polygon*/);

  db::SimpleMerge op;
  ep.process (out, op);

  if (polygons.empty ()) {
    return db::Polygon ();
  } else {
    tl_assert (polygons.size () == 1);
    return polygons [0];
  }
}

db::Polygon 
simple_polygon_to_polygon (const db::SimplePolygon &sp)
{
  db::Polygon p;
  p.assign_hull (sp.begin_hull (), sp.end_hull ());
  return p;
}

db::SimplePolygon 
polygon_to_simple_polygon (const db::Polygon &p)
{
  if (p.holes () > 0) {
    db::Polygon pp = resolve_holes (p);
    db::SimplePolygon sp;
    sp.assign_hull (pp.begin_hull (), pp.end_hull ());
    return sp;
  } else {
    db::SimplePolygon sp;
    sp.assign_hull (p.begin_hull (), p.end_hull ());
    return sp;
  }
}

static void decompose_convex_helper (int depth, PreferredOrientation po, const db::SimplePolygon &sp, SimplePolygonSink &sink)
{
  size_t n = sp.hull ().size ();
  if (n < 4 || depth <= 0) {
    if (n > 2) {
      sink.put (sp);
    }
    return;
  }

  db::Box bbox = sp.box ();
  db::coord_traits<db::Coord>::area_type atot = 0;
  db::coord_traits<db::Coord>::distance_type min_edge = std::numeric_limits<db::coord_traits<db::Coord>::distance_type>::max ();
  for (size_t i = 0; i < n; ++i) {
    db::Edge ep (sp.hull ()[(i + n - 1) % n], sp.hull ()[i]);
    atot += db::vprod (ep.p2 () - db::Point (), ep.p1 () - db::Point ());
    min_edge = std::min (min_edge, ep.length ());
  }

  std::set<db::Point> skipped;

  while (true) {

    //  Look for the convex corner closed to the median
    size_t imed = std::numeric_limits<size_t>::max ();
    db::Coord dmin = 0;
    for (size_t i = 0; i < n; ++i) {

      db::Edge ep (sp.hull ()[(i + n - 1) % n], sp.hull ()[i]);
      db::Edge ec (sp.hull ()[i], sp.hull ()[(i + 1) % n]);

      if (db::vprod_sign (ep, ec) > 0 && skipped.find (ep.p2 ()) == skipped.end ()) {

        db::Vector v = sp.hull ()[i] - bbox.center ();
        db::Coord d = std::min (std::abs (v.x ()), std::abs (v.y ()));
        if (imed == std::numeric_limits<size_t>::max () || d < dmin) {
          imed = i;
          dmin = d;
        }

      }

    }

    //  is convex already
    if (imed == std::numeric_limits<size_t>::max ()) {
      if (! skipped.empty ()) {
        tl::warn << "sp=" << sp << tl::endl << "po=" << int(po);
        tl_assert (false);
      }
      sink.put (sp);
      return;
    }

    db::Point p (sp.hull ()[imed]);
    db::Edge ep (sp.hull ()[(imed + n - 1) % n], p);
    db::Edge ec (p, sp.hull ()[(imed + 1) % n]);

    //  convex corner

    std::set<db::Vector> cuts;

    db::Vector pv = db::Vector (ep.dy (), -ep.dx ()) + db::Vector (ec.dy (), -ec.dx ());
    bool ortho = ((ep.dx () == 0 || ep.dy () == 0) && (ec.dx () == 0 || ec.dy () == 0));

    if (po == PO_any || po == PO_horizontal || po == PO_htrapezoids || (po == PO_vtrapezoids && ortho)) {
      if (pv.x () >= 0) {
        cuts.insert (db::Vector (1, 0));
      }
      if (pv.x () <= 0) {
        cuts.insert (db::Vector (-1, 0));
      }
    }
    if (po == PO_any || po == PO_vertical || po == PO_vtrapezoids || (po == PO_htrapezoids && ortho)) {
      if (pv.y () >= 0) {
        cuts.insert (db::Vector (0, 1));
      }
      if (pv.y () <= 0) {
        cuts.insert (db::Vector (0, -1));
      }
    }

    int cut_rating = 0;
    size_t jmin = 0;
    db::Point xmin;
    db::coord_traits<db::Coord>::area_type acutoff = 0;

    for (std::set<db::Vector>::const_iterator c = cuts.begin (); c != cuts.end (); ++c) {

      db::Vector nv = *c;

      int cut_rating_inner = 0;
      size_t jmin_inner = 0;
      db::Point xmin_inner;
      db::coord_traits<db::Coord>::area_type acutoff_inner = 0;

      db::coord_traits<db::Coord>::area_type asum = 0;
      db::coord_traits<db::Coord>::area_type min_dist = std::numeric_limits<db::coord_traits<db::Coord>::area_type>::max ();

      for (size_t j = 1; j != n - 1; ++j) {

        db::Edge efc (sp.hull ()[(imed + j) % n], sp.hull ()[(imed + j + 1) % n]);
        db::Edge efp (sp.hull ()[(imed + j + n - 1) % n], sp.hull ()[(imed + j) % n]);

        asum += db::vprod (efp.p2 () - db::Point (), efp.p1 () - db::Point ());

        std::pair <bool, db::Point> x;

        if (db::vprod_sign (nv, efc.d ()) == 0) {

          if (efc.side_of (p) == 0) {

            db::coord_traits<db::Coord>::area_type d1 = db::sprod (efc.p1 () - p, nv);
            db::coord_traits<db::Coord>::area_type d2 = db::sprod (efc.p2 () - p, nv);

            if (d1 <= 0 && d2 >= 0) {
              x = std::make_pair (true, p);
            } else if (d2 <= 0 && d1 >= 0) {
              x = std::make_pair (true, p);
            } else if (d1 >= 0 && d2 >= 0) {
              if (d1 < d2) {
                x = std::make_pair (true, efc.p1 ());
              } else {
                x = std::make_pair (true, efc.p2 ());
              }
            }

          }

        } else {
          x = db::Edge (p, nv).crossed_by_point (efc);
        }

        if (x.first && x.second != efc.p2 ()) {

          db::coord_traits<db::Coord>::area_type dist = db::sprod (x.second - p, nv);
          if (dist >= 0 && dist < min_dist) {

            //  a will be the area of the half we cut off
            db::coord_traits<db::Coord>::area_type a = asum +
              db::vprod (x.second - db::Point (), efp.p2 () - db::Point ()) +
              db::vprod (p - db::Point (), x.second - db::Point ());

            //  due to rounding, the new cut point will modify the total
            //  area. We compute the new total area now:
            db::coord_traits<db::Coord>::area_type atot_eff = atot +
              db::vprod (efc.p2 () - db::Point (), x.second - db::Point ()) +
              db::vprod (x.second - db::Point (), efc.p1 () - db::Point ()) +
              db::vprod (efc.p1 () - db::Point (), efc.p2 () - db::Point ());

            db::coord_traits<db::Coord>::area_type ac;
            if (a > atot_eff / 2) {
              ac = a - atot_eff / 2;
            } else {
              ac = atot_eff / 2 - a;
            }

            if (db::vprod_sign (nv, efc.d ()) <= 0 && a >= 0 && a <= atot_eff) {

              //  compute rating
              int cr = 0;
              if (x.second == efc.p1 ()) {
                if (db::vprod (efc, efp) < 0) {
                  cr = 3;   //  cut terminates at another concave corner
                } else {
                  cr = 2;   //  cut terminates at a convex corner
                }
              } else {
                db::coord_traits<db::Coord>::distance_type el = std::min (x.second.distance (efc.p1 ()), x.second.distance (efc.p2 ()));
                if (el >= min_edge) {
                  cr = 1;   //  does not induce shorter edge than we have so far
                }
              }

              jmin_inner = j;
              cut_rating_inner = cr;
              xmin_inner = x.second;
              acutoff_inner = ac;

              min_dist = dist;

            } else if (db::vprod_sign (nv, efc.d ()) < 0) {

              min_dist = dist;
              jmin_inner = 0;

            }

          }

        }

      }

      if (jmin_inner > 0 && (!jmin || cut_rating_inner > cut_rating || (cut_rating_inner == cut_rating && acutoff_inner < acutoff))) {

        jmin = jmin_inner;
        cut_rating = cut_rating_inner;
        xmin = xmin_inner;
        acutoff = acutoff_inner;

      }

    }

    if (jmin > 0) {

      std::vector<db::Point> pts;
      pts.reserve (n);
      db::SimplePolygon sp_out;

      for (size_t i = imed; i <= imed + jmin; ++i) {
        pts.push_back (sp.hull ()[i % n]);
      }
      if (pts.back () != xmin) {
        pts.push_back (xmin);
      }
      sp_out.assign_hull (pts.begin (), pts.end (), true, true);
      decompose_convex_helper (depth - 1, po, sp_out, sink);

      pts.clear ();

      for (size_t i = imed + jmin + 1; i <= imed + n; ++i) {
        pts.push_back (sp.hull ()[i % n]);
      }
      if (pts.front () != xmin) {
        pts.push_back (xmin);
      }
      sp_out.assign_hull (pts.begin (), pts.end (), true, true);
      decompose_convex_helper (depth - 1, po, sp_out, sink);

      break;

    } else {
      //  no decomposition found -> next try
      skipped.insert (p);
    }

  }
}

class ConvexDecompositionFilter
  : public db::SimplePolygonSink
{
public:
  ConvexDecompositionFilter (PreferredOrientation po, bool swap_xy, db::SimplePolygonSink *out)
    : mp_out (out), m_po (po), m_swap_xy (swap_xy)
  {
    //  .. nothing yet ..
  }

  virtual void put (const db::SimplePolygon &polygon)
  {
    if (m_swap_xy) {
      db::SimplePolygon p (polygon);
      p.transform (db::FTrans (db::FTrans::m45));
      decompose_convex_helper (std::numeric_limits<int>::max (), m_po, p, *mp_out);
    } else {
      decompose_convex_helper (std::numeric_limits<int>::max (), m_po, polygon, *mp_out);
    }
  }

private:
  db::SimplePolygonSink *mp_out;
  PreferredOrientation m_po;
  bool m_swap_xy;
};

void
decompose_convex (const db::Polygon &p, PreferredOrientation po, db::SimplePolygonSink &sink)
{
  if (p.is_box ()) {

    sink.put (db::SimplePolygon (p.box ()));

  } else {

    //  Because the hole resolution strategy favours horizontal lines we need to swap x and y
    //  for the PO_vertical and PO_vtrapezoids case
    bool swap_xy = (po == PO_vertical || po == PO_vtrapezoids);

    ConvexDecompositionFilter cd (po, swap_xy, &sink);

    db::PolygonGenerator pg (cd, true /*min coherence*/);
    //  Does some pre-decomposition and avoids self-interacting polygons:
    pg.open_contours (true);

    db::EdgeProcessor ep;

    if (swap_xy) {
      for (db::Polygon::polygon_edge_iterator e = p.begin_edge (); ! e.at_end (); ++e) {
        ep.insert ((*e).transformed (db::FTrans (db::FTrans::m45)));
      }
    } else {
      ep.insert_sequence (p.begin_edge ());
    }

    db::SimpleMerge op;
    ep.process (pg, op);

  }
}

void
decompose_convex (const db::SimplePolygon &sp, PreferredOrientation po, db::SimplePolygonSink &sink)
{
  if (sp.is_box ()) {
    sink.put (sp);
  } else {
    decompose_convex_helper (std::numeric_limits<int>::max (), po, sp, sink);
  }
}

template <class P>
bool
is_convex_helper (const P &p)
{
  size_t n = p.hull ().size ();
  if (n < 4) {
    return true;
  }

  for (size_t i = 0; i < n; ++i) {
    db::Edge ep (p.hull ()[(i + n - 1) % n], p.hull ()[i]);
    db::Edge ec (p.hull ()[i], p.hull ()[(i + 1) % n]);
    if (db::vprod_sign (ep, ec) > 0) {
      return false;
    }
  }

  return true;
}

bool
is_convex (const db::SimplePolygon &p)
{
  return is_convex_helper (p);
}

bool
is_convex (const db::Polygon &p)
{
  if (p.holes () > 0) {
    return false;
  } else {
    return is_convex_helper (p);
  }
}

static void
decompose_convex_to_trapezoids (const db::SimplePolygon &sp, bool horizontal, db::SimplePolygonSink &sink)
{
  if (sp.hull ().size () < 3) {
    return;
  }

  std::vector<db::Edge> edges;
  edges.reserve (sp.hull ().size ());
  for (db::SimplePolygon::polygon_edge_iterator e = sp.begin_edge (); ! e.at_end (); ++e) {
    db::Edge ee = *e;
    if (! horizontal) {
      ee.transform (db::FTrans (db::FTrans::m45));
    }
    if (ee.dy () != 0) {
      edges.push_back (ee);
    }
  }

  std::sort (edges.begin (), edges.end (), db::edge_ymin_compare<db::Coord> ());

  db::Coord y = db::edge_ymin (edges.front ());

  std::vector<db::Edge>::iterator c = edges.begin ();
  while (c != edges.end ()) {

    std::vector<db::Edge>::iterator cc = c;
    while (cc != edges.end () && db::edge_ymin (*cc) <= y) {
      ++cc;
    }

    //  this condition will be fulfilled always if the input polygon is convex
    tl_assert (cc - c == 2);

    db::Coord x1 = db::edge_xaty (c[0], y);
    db::Coord x2 = db::edge_xaty (c[1], y);
    if (x1 > x2) {
      std::swap (x1, x2);
    }

    db::Coord yy;
    if (cc == edges.end ()) {
      yy = db::edge_ymax (*c);
      tl_assert (db::edge_ymax (c[1]) == db::edge_ymax (*c));
    } else {
      yy = db::edge_ymin (*cc);
    }

    db::Coord xx1 = db::edge_xaty (c[0], yy);
    db::Coord xx2 = db::edge_xaty (c[1], yy);
    if (xx1 > xx2) {
      std::swap (xx1, xx2);
    }

    db::SimplePolygon sp;

    if (x1 == x2) {

      db::Point pts[] = {
        db::Point (x1, y),
        db::Point (xx1, yy),
        db::Point (xx2, yy)
      };

      sp.assign_hull (&pts[0], &pts[sizeof (pts) / sizeof (pts[0])]);

    } else if (xx1 == xx2) {

      db::Point pts[] = {
        db::Point (x1, y),
        db::Point (xx1, yy),
        db::Point (x2, y)
      };

      sp.assign_hull (&pts[0], &pts[sizeof (pts) / sizeof (pts[0])]);

    } else {

      db::Point pts[] = {
        db::Point (x1, y),
        db::Point (xx1, yy),
        db::Point (xx2, yy),
        db::Point (x2, y)
      };

      sp.assign_hull (&pts[0], &pts[sizeof (pts) / sizeof (pts[0])]);

    }

    if (! horizontal) {
      sp.transform (db::FTrans (db::FTrans::m45));
    }

    sink.put (sp);

    std::vector<db::Edge>::iterator c0 = c;

    for (std::vector<db::Edge>::iterator i = c; i != cc; ++i) {
      if (db::edge_ymax (*i) <= yy) {
        if (c != i) {
          std::swap (*c, *i);
        }
        ++c;
      }
    }

    y = yy;
    tl_assert (c0 != c);

  }
}

namespace {

  struct TrapezoidConverter
    : public db::SimplePolygonSink
  {
    TrapezoidConverter (bool horizontal, db::SimplePolygonSink *target)
      : m_horizontal (horizontal), mp_target (target)
    {
      //  .. nothing yet ..
    }

    virtual void put (const db::SimplePolygon &polygon)
    {
      decompose_convex_to_trapezoids (polygon, m_horizontal, *mp_target);
    }

  public:
    bool m_horizontal;
    db::SimplePolygonSink *mp_target;
  };
}

void
decompose_trapezoids (const db::Polygon &p, TrapezoidDecompositionMode mode, db::SimplePolygonSink &sink)
{
  if (mode == TD_htrapezoids || mode == TD_vtrapezoids) {

    //  Implementation uses convex decomposition and trapezoid decomposition
    if (p.is_box ()) {
      sink.put (db::SimplePolygon (p.box ()));
    } else {

      bool swap_xy = (mode == TD_vtrapezoids);

      TrapezoidConverter trap_maker (mode == TD_htrapezoids, &sink);
      ConvexDecompositionFilter cd (mode == TD_htrapezoids ? PO_htrapezoids : PO_vtrapezoids, swap_xy, &trap_maker);

      db::PolygonGenerator pg (cd, true /*min coherence*/);
      //  Does some pre-decomposition and avoids self-interacting polygons:
      pg.open_contours (true);

      db::EdgeProcessor ep;

      if (swap_xy) {
        for (db::Polygon::polygon_edge_iterator e = p.begin_edge (); ! e.at_end (); ++e) {
          ep.insert ((*e).transformed (db::FTrans (db::FTrans::m45)));
        }
      } else {
        ep.insert_sequence (p.begin_edge ());
      }

      db::SimpleMerge op;
      ep.process (pg, op);

    }

  } else {

    //  Implementation uses trapezoid generator
    if (p.is_box ()) {
      sink.put (db::SimplePolygon (p.box ()));
    } else {

      db::TrapezoidGenerator pg (sink);

      db::EdgeProcessor ep;
      db::SimpleMerge op;
      ep.insert_sequence (p.begin_edge ());
      ep.process (pg, op);

    }

  }
}

void
decompose_trapezoids (const db::SimplePolygon &sp, TrapezoidDecompositionMode mode, db::SimplePolygonSink &sink)
{
  if (mode == TD_htrapezoids || mode == TD_vtrapezoids) {

    if (sp.is_box ()) {
      sink.put (sp);
    } else {
      TrapezoidConverter trap_maker (mode == TD_htrapezoids, &sink);
      decompose_convex_helper (std::numeric_limits<int>::max (), mode == TD_htrapezoids ? PO_htrapezoids : PO_vtrapezoids, sp, trap_maker);
    }

  } else {

    //  This implementation uses trapezoid generator
    if (sp.is_box ()) {
      sink.put (db::SimplePolygon (sp.box ()));
    } else {

      db::TrapezoidGenerator pg (sink);

      db::EdgeProcessor ep;
      db::SimpleMerge op;
      ep.insert_sequence (sp.begin_edge ());
      ep.process (pg, op);

    }

  }
}

// -------------------------------------------------------------------------------------
//  Polygon snapping

db::Polygon
snapped_polygon (const db::Polygon &poly, db::Coord gx, db::Coord gy, std::vector<db::Point> &heap)
{
  db::Polygon pnew;

  for (size_t i = 0; i < poly.holes () + 1; ++i) {

    heap.clear ();

    db::Polygon::polygon_contour_iterator b, e;

    if (i == 0) {
      b = poly.begin_hull ();
      e = poly.end_hull ();
    } else {
      b = poly.begin_hole ((unsigned int)  (i - 1));
      e = poly.end_hole ((unsigned int)  (i - 1));
    }

    for (db::Polygon::polygon_contour_iterator pt = b; pt != e; ++pt) {
      heap.push_back (db::Point (snap_to_grid ((*pt).x (), gx), snap_to_grid ((*pt).y (), gy)));
    }

    if (i == 0) {
      pnew.assign_hull (heap.begin (), heap.end ());
    } else {
      pnew.insert_hole (heap.begin (), heap.end ());
    }

  }

  pnew.sort_holes ();

  return pnew;
}

db::Polygon
scaled_and_snapped_polygon (const db::Polygon &poly, db::Coord gx, db::Coord mx, db::Coord dx, db::Coord ox, db::Coord gy, db::Coord my, db::Coord dy, db::Coord oy, std::vector<db::Point> &heap)
{
  db::Polygon pnew;

  int64_t dgx = int64_t (gx) * int64_t (dx);
  int64_t dgy = int64_t (gy) * int64_t (dy);

  for (size_t i = 0; i < poly.holes () + 1; ++i) {

    heap.clear ();

    db::Polygon::polygon_contour_iterator b, e;

    if (i == 0) {
      b = poly.begin_hull ();
      e = poly.end_hull ();
    } else {
      b = poly.begin_hole ((unsigned int)  (i - 1));
      e = poly.end_hole ((unsigned int)  (i - 1));
    }

    for (db::Polygon::polygon_contour_iterator pt = b; pt != e; ++pt) {
      int64_t x = snap_to_grid (int64_t ((*pt).x ()) * mx + int64_t (ox), dgx) / int64_t (dx);
      int64_t y = snap_to_grid (int64_t ((*pt).y ()) * my + int64_t (oy), dgy) / int64_t (dy);
      heap.push_back (db::Point (db::Coord (x), db::Coord (y)));
    }

    if (i == 0) {
      pnew.assign_hull (heap.begin (), heap.end ());
    } else {
      pnew.insert_hole (heap.begin (), heap.end ());
    }

  }

  pnew.sort_holes ();

  return pnew;
}

db::Vector
scaled_and_snapped_vector (const db::Vector &v, db::Coord gx, db::Coord mx, db::Coord dx, db::Coord ox, db::Coord gy, db::Coord my, db::Coord dy, db::Coord oy)
{
  int64_t dgx = int64_t (gx) * int64_t (dx);
  int64_t dgy = int64_t (gy) * int64_t (dy);

  int64_t x = snap_to_grid (int64_t (v.x ()) * mx + int64_t (ox), dgx) / int64_t (dx);
  int64_t y = snap_to_grid (int64_t (v.y ()) * my + int64_t (oy), dgy) / int64_t (dy);

  return db::Vector (db::Coord (x), db::Coord (y));
}

}

