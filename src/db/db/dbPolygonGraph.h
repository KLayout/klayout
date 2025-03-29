
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

#ifndef HDR_dbPolygonGraph
#define HDR_dbPolygonGraph

#include "dbCommon.h"
#include "dbTriangle.h"
#include "dbBox.h"
#include "dbRegion.h"

#include "tlObjectCollection.h"
#include "tlStableVector.h"

#include <limits>
#include <list>
#include <vector>
#include <algorithm>

namespace db
{

class Layout;

class GPolygon;
class GPolygonEdge;

/**
 *  @brief A class representing a vertex in a Delaunay triangulation graph
 *
 *  The vertex carries information about the connected edges and
 *  an integer value that can be used in traversal algorithms
 *  ("level")
 */
class DB_PUBLIC GVertex
  : public db::DPoint
{
public:
  typedef std::list<GPolygonEdge *> edges_type;
  typedef edges_type::const_iterator edges_iterator;
  typedef edges_type::iterator edges_iterator_non_const;

  GVertex ();
  GVertex (const DPoint &p);
  GVertex (const GVertex &v);
  GVertex (db::DCoord x, db::DCoord y);

  GVertex &operator= (const GVertex &v);

#if 0 // @@@
  bool is_outside () const;
#endif
  std::vector<db::GPolygon *> polygons () const;

  edges_iterator begin_edges () const { return mp_edges.begin (); }
  edges_iterator end_edges () const { return mp_edges.end (); }
  size_t num_edges (int max_count = -1) const;

  bool has_edge (const GPolygonEdge *edge) const;

  void set_is_precious (bool f) { m_is_precious = f; }
  bool is_precious () const { return m_is_precious; }

  std::string to_string (bool with_id = false) const;

  /**
   *  @brief Returns 1 is the point is inside the circle, 0 if on the circle and -1 if outside
   *  TODO: Move to db::DPoint
   */
  static int in_circle (const db::DPoint &point, const db::DPoint &center, double radius);

  /**
   *  @brief Returns 1 is this point is inside the circle, 0 if on the circle and -1 if outside
   */
  int in_circle (const db::DPoint &center, double radius) const
  {
    return in_circle (*this, center, radius);
  }

private:
  friend class GPolygonEdge;

  void remove_edge (const edges_iterator_non_const &ec)
  {
    mp_edges.erase (ec);
  }

  edges_type mp_edges;
  bool m_is_precious;
};

/**
 *  @brief A class representing an edge in the Delaunay triangulation graph
 */
class DB_PUBLIC GPolygonEdge
{
public:
  class GPolygonIterator
  {
  public:
    typedef GPolygon value_type;
    typedef GPolygon &reference;
    typedef GPolygon *pointer;

    reference operator*() const
    {
      return *operator-> ();
    }

    pointer operator->() const
    {
      return m_index ? mp_edge->right () : mp_edge->left ();
    }

    bool operator== (const GPolygonIterator &other) const
    {
      return m_index == other.m_index;
    }

    bool operator!= (const GPolygonIterator &other) const
    {
      return !operator== (other);
    }

    GPolygonIterator &operator++ ()
    {
      while (++m_index < 2 && operator-> () == 0)
        ;
      return *this;
    }

  private:
    friend class GPolygonEdge;

    GPolygonIterator (const GPolygonEdge *edge)
      : mp_edge (edge), m_index (0)
    {
      if (! edge) {
        m_index = 2;
      } else {
        --m_index;
        operator++ ();
      }
    }

    const GPolygonEdge *mp_edge;
    unsigned int m_index;
  };

  GPolygonEdge ();
  GPolygonEdge (GVertex *v1, GVertex *v2);

  GVertex *v1 () const { return mp_v1; }
  GVertex *v2 () const { return mp_v2; }

  void reverse ()
  {
    std::swap (mp_v1, mp_v2);
    std::swap (mp_left, mp_right);
  }

  GPolygon *left  () const { return mp_left; }
  GPolygon *right () const { return mp_right; }

  GPolygonIterator begin_polygons () const
  {
    return GPolygonIterator (this);
  }

  GPolygonIterator end_polygons () const
  {
    return GPolygonIterator (0);
  }

  void set_level (size_t l) { m_level = l; }
  size_t level () const { return m_level; }

  void set_id (size_t id) { m_id = id; }
  size_t id () const { return m_id; }

  void set_is_segment (bool is_seg) { m_is_segment = is_seg; }
  bool is_segment () const { return m_is_segment; }

  std::string to_string (bool with_id = false) const;

  /**
   *  @brief Converts to an db::DEdge
   */
  db::DEdge edge () const
  {
    return db::DEdge (*mp_v1, *mp_v2);
  }

  /**
   *  @brief Returns the distance of the given point to the edge
   *
   *  The distance is the minimum distance of the point to one point from the edge.
   *  TODO: Move to db::DEdge
   */
  static double distance (const db::DEdge &e, const db::DPoint &p);

  /**
   *  @brief Returns the distance of the given point to the edge
   *
   *  The distance is the minimum distance of the point to one point from the edge.
   */
  double distance (const db::DPoint &p) const
  {
    return distance (edge (), p);
  }

  /**
   *  @brief Returns a value indicating whether this edge crosses the other one
   *
   *  "crosses" is true, if both edges share at least one point which is not an endpoint
   *  of one of the edges.
   *  TODO: Move to db::DEdge
   */
  static bool crosses (const db::DEdge &e, const db::DEdge &other);

  /**
   *  @brief Returns a value indicating whether this edge crosses the other one
   *
   *  "crosses" is true, if both edges share at least one point which is not an endpoint
   *  of one of the edges.
   */
  bool crosses (const db::DEdge &other) const
  {
    return crosses (edge (), other);
  }

  /**
   *  @brief Returns a value indicating whether this edge crosses the other one
   *
   *  "crosses" is true, if both edges share at least one point which is not an endpoint
   *  of one of the edges.
   */
  bool crosses (const db::GPolygonEdge &other) const
  {
    return crosses (edge (), other.edge ());
  }

  /**
   *  @brief Returns a value indicating whether this edge crosses the other one
   *  "crosses" is true, if both edges share at least one point.
   *  TODO: Move to db::DEdge
   */
  static bool crosses_including (const db::DEdge &e, const db::DEdge &other);

  /**
   *  @brief Returns a value indicating whether this edge crosses the other one
   *  "crosses" is true, if both edges share at least one point.
   */
  bool crosses_including (const db::DEdge &other) const
  {
    return crosses_including (edge (), other);
  }

  /**
   *  @brief Returns a value indicating whether this edge crosses the other one
   *  "crosses" is true, if both edges share at least one point.
   */
  bool crosses_including (const db::GPolygonEdge &other) const
  {
    return crosses_including (edge (), other.edge ());
  }

  /**
   *  @brief Gets the intersection point
   *  TODO: Move to db::DEdge
   */
  static db::DPoint intersection_point (const db::DEdge &e, const DEdge &other);

  /**
   *  @brief Gets the intersection point
   */
  db::DPoint intersection_point (const db::DEdge &other) const
  {
    return intersection_point (edge (), other);
  }

  /**
   *  @brief Gets the intersection point
   */
  db::DPoint intersection_point (const GPolygonEdge &other) const
  {
    return intersection_point (edge (), other.edge ());
  }

  /**
   *  @brief Returns a value indicating whether the point is on the edge
   *  TODO: Move to db::DEdge
   */
  static bool point_on (const db::DEdge &edge, const db::DPoint &point);

  /**
   *  @brief Returns a value indicating whether the point is on the edge
   */
  bool point_on (const db::DPoint &point) const
  {
    return point_on (edge (), point);
  }

  /**
   *  @brief Gets the side the point is on
   *
   *  -1 is for "left", 0 is "on" and +1 is "right"
   *  TODO: correct to same definition as db::Edge (negative)
   */
  static int side_of (const db::DEdge &e, const db::DPoint &point)
  {
    return -e.side_of (point);
  }

  /**
   *  @brief Gets the side the point is on
   *
   *  -1 is for "left", 0 is "on" and +1 is "right"
   *  TODO: correct to same definition as db::Edge (negative)
   */
  int side_of (const db::DPoint &p) const
  {
    return -edge ().side_of (p);
  }

  /**
   *  @brief Gets the distance vector
   */
  db::DVector d () const
  {
    return *mp_v2 - *mp_v1;
  }

  /**
   *  @brief Gets the other triangle for the given one
   */
  GPolygon *other (const GPolygon *) const;

  /**
   *  @brief Gets the other vertex for the given one
   */
  GVertex *other (const GVertex *) const;

  /**
   *  @brief Gets a value indicating whether the edge has the given vertex
   */
  bool has_vertex (const GVertex *) const;

  /**
   *  @brief Gets the common vertex of the other edge and this edge or null if there is no common vertex
   */
  GVertex *common_vertex (const GPolygonEdge *other) const;

#if 0 // @@@
  /**
   *  @brief Returns a value indicating whether this edge can be flipped
   */
  bool can_flip () const;

  /**
   *  @brief Returns a value indicating whether the edge separates two triangles that can be joined into one (via the given vertex)
   */
  bool can_join_via (const GVertex *vertex) const;

  /**
   *  @brief Returns a value indicating whether this edge is an outside edge (no other triangles)
   */
  bool is_outside () const;

  /**
   *  @brief Returns a value indicating whether this edge belongs to outside triangles
   */
  bool is_for_outside_triangles () const;
#endif // @@@

  /**
   *  @brief Returns a value indicating whether t is attached to this edge
   */
  bool has_polygon (const GPolygon *t) const;

protected:
  void unlink ();
  void link ();

private:
  friend class GPolygon;
  friend class PolygonGraph;

  GVertex *mp_v1, *mp_v2;
  GPolygon *mp_left, *mp_right;
  GVertex::edges_iterator_non_const m_ec_v1, m_ec_v2;
  size_t m_level;
  size_t m_id;
  bool m_is_segment;

  void set_left  (GPolygon *t);
  void set_right (GPolygon *t);
};

/**
 *  @brief A compare function that compares triangles by ID
 *
 *  The ID acts as a more predicable unique ID for the object in sets and maps.
 */
struct GPolygonEdgeLessFunc
{
  bool operator () (GPolygonEdge *a, GPolygonEdge *b) const
  {
    return a->id () < b->id ();
  }
};

/**
 *  @brief A class representing a triangle
 */
class DB_PUBLIC GPolygon
  : public tl::list_node<GPolygon>, public tl::Object
{
public:
  GPolygon ();

  template<class Iter>
  GPolygon (Iter from, Iter to)
    : mp_e (from, to)
  {
    init ();
  }

  ~GPolygon ();

  void unlink ();

  void set_id (size_t id) { m_id = id; }
  size_t id () const { return m_id; }

  // @@@bool is_outside () const { return m_is_outside; }
  // @@@void set_outside (bool o) { m_is_outside = o; }

  std::string to_string (bool with_id = false) const;

  /**
   *  @brief Gets the number of vertexes
   */
  size_t size () const
  {
    return mp_v.size ();
  }

  /**
   *  @brief Gets the nth vertex (n wraps around and can be negative)
   *  The vertexes are oriented clockwise.
   */
  inline GVertex *vertex (int n) const
  {
    size_t sz = size ();
    tl_assert (sz > 0);
    if (n >= 0 && size_t (n) < sz) {
      return mp_v[n];
    } else {
      return mp_v[(n + sz) % sz];
    }
  }

  /**
   *  @brief Gets the nth edge (n wraps around and can be negative)
   */
  inline GPolygonEdge *edge (int n) const
  {
    size_t sz = size ();
    tl_assert (sz > 0);
    if (n >= 0 && size_t (n) < sz) {
      return mp_e[n];
    } else {
      return mp_e[(n + sz) % sz];
    }
  }

  /**
   *  @brief Gets the area
   */
  double area () const;

  /**
   *  @brief Returns the bounding box of the triangle
   */
  db::DBox bbox () const;

#if 0 // @@@
  /**
   *  @brief Gets the center point and radius of the circumcircle
   *  If ok is non-null, it will receive a boolean value indicating whether the circumcircle is valid.
   *  An invalid circumcircle is an indicator for a degenerated triangle with area 0 (or close to).
   */
  std::pair<db::DPoint, double> circumcircle (bool *ok = 0) const;

  /**
   *  @brief Gets the vertex opposite of the given edge
   */
  GVertex *opposite (const GPolygonEdge *edge) const;

  /**
   *  @brief Gets the edge opposite of the given vertex
   */
  GPolygonEdge *opposite (const GVertex *vertex) const;
#endif

  /**
   *  @brief Gets the edge with the given vertexes
   */
  GPolygonEdge *find_edge_with (const GVertex *v1, const GVertex *v2) const;

  /**
   *  @brief Finds the common edge for both polygons
   */
  GPolygonEdge *common_edge (const GPolygon *other) const;

#if 0 // @@@
  /**
   *  @brief Returns a value indicating whether the point is inside (1), on the polygon (0) or outside (-1)
   */
  int contains (const db::DPoint &point) const;
#endif

  /**
   *  @brief Gets a value indicating whether the triangle has the given vertex
   */
  inline bool has_vertex (const db::GVertex *v) const
  {
    for (auto i = mp_v.begin (); i != mp_v.end (); ++i) {
      if (*i == v) {
        return true;
      }
    }
    return false;
  }

  /**
   *  @brief Gets a value indicating whether the triangle has the given edge
   */
  inline bool has_edge (const db::GPolygonEdge *e) const
  {
    for (auto i = mp_e.begin (); i != mp_e.end (); ++i) {
      if (*i == e) {
        return true;
      }
    }
    return false;
  }

  /**
   *  @brief Returns the minimum edge length
   */
  double min_edge_length () const;

#if 0 // @@@
  /**
   *  @brief Returns the min edge length to circumcircle radius ratio
   */
  double b () const;
#endif

  /**
   *  @brief Returns a value indicating whether the polygon borders to a segment
   */
  bool has_segment () const;

  /**
   *  @brief Returns the number of segments the polygon borders to
   */
  unsigned int num_segments () const;

private:
  // @@@ bool m_is_outside;
  std::vector<GPolygonEdge *> mp_e;
  std::vector<db::GVertex *> mp_v;
  size_t m_id;

  void init ();

  //  no copying
  GPolygon &operator= (const GPolygon &);
  GPolygon (const GPolygon &);
};

/**
 *  @brief A compare function that compares polygons by ID
 *
 *  The ID acts as a more predicable unique ID for the object in sets and maps.
 */
struct GPolygonLessFunc
{
  bool operator () (GPolygon *a, GPolygon *b) const
  {
    return a->id () < b->id ();
  }
};

class DB_PUBLIC PolygonGraph
{
public:
#if 0 // @@@
  struct TriangulateParameters
  {
    TriangulateParameters ()
      : min_b (1.0),
        min_length (0.0),
        max_area (0.0),
        max_area_border (0.0),
        max_iterations (std::numeric_limits<size_t>::max ()),
        base_verbosity (30),
        mark_triangles (false)
    { }

    /**
     *  @brief Min. readius-to-shortest edge ratio
     */
    double min_b;

    /**
     *  @brief Min. edge length
     *
     *  This parameter does not provide a guarantee about a minimume edge length, but
     *  helps avoiding ever-reducing triangle splits in acute corners of the input polygon.
     *  Splitting of edges stops when the edge is less than the min length.
     */
    double min_length;

    /**
     *  @brief Max area or zero for "no constraint"
     */
    double max_area;

    /**
     *  @brief Max area for border triangles or zero for "use max_area"
     */
    double max_area_border;

    /**
     *  @brief Max number of iterations
     */
    size_t max_iterations;

    /**
     *  @brief The verbosity level above which triangulation reports details
     */
    int base_verbosity;

    /**
     *  @brief If true, final triangles are marked using the "id" integer as a bit field
     *
     *  This provides information about the result quality.
     *
     *  Bit 0: skinny triangle
     *  Bit 1: bad-quality (skinny or area too large)
     *  Bit 2: non-Delaunay (in the strict sense)
     */
    bool mark_triangles;
  };
#endif

  typedef tl::list<db::GPolygon> polygons_type;
  typedef polygons_type::const_iterator polygon_iterator;

  PolygonGraph ();
  ~PolygonGraph ();

  /**
   *  @brief Inserts the given polygon
   */
  void insert_polygon (const db::DPolygon &box);

  /**
   *  @brief Returns a string representation of the polygon graph.
   */
  std::string to_string ();

  /**
   *  @brief Returns the bounding box of the polygon graph.
   */
  db::DBox bbox () const;

  /**
   *  @brief Iterates the polygons in the graph (begin iterator)
   */
  polygon_iterator begin () const { return mp_polygons.begin (); }

  /**
   *  @brief Iterates the polygons in the graph (end iterator)
   */
  polygon_iterator end () const { return mp_polygons.end (); }

  /**
   *  @brief Returns the number of polygons in the graph
   */
  size_t num_polygons () const { return mp_polygons.size (); }

  /**
   *  @brief Clears the polygon set
   */
  void clear ();

protected:
#if 0 // @@@
  /**
   *  @brief Checks the polygon graph for consistency
   *  This method is for testing purposes mainly.
   */
  bool check (bool check_delaunay = true) const;
#endif

  /**
   *  @brief Dumps the polygon graph to a GDS file at the given path
   *  This method is for testing purposes mainly.
   *
   *  "decompose_id" will map polygons to layer 20, 21 and 22.
   *  according to bit 0, 1 and 2 of the ID (useful with the 'mark_polygons'
   *  flat in TriangulateParameters).
   */
  void dump (const std::string &path, bool decompose_by_id = false) const;

  /**
   *  @brief Creates a new layout object representing the polygon graph
   *  This method is for testing purposes mainly.
   */
  db::Layout *to_layout (bool decompose_by_id = false) const;

private:
  tl::list<db::GPolygon> mp_polygons;
  tl::stable_vector<db::GPolygonEdge> m_edges_heap;
  std::vector<db::GPolygonEdge *> m_returned_edges;
  tl::stable_vector<db::GVertex> m_vertex_heap;
// @@@  bool m_is_constrained;
// @@@    size_t m_level;
  size_t m_id;
// @@@    size_t m_flips, m_hops;

  db::GVertex *create_vertex (double x, double y);
  db::GVertex *create_vertex (const db::DPoint &pt);
  db::GPolygonEdge *create_edge (db::GVertex *v1, db::GVertex *v2);

  template <class Iter>
  db::GPolygon *
  create_polygon (Iter from, Iter to)
  {
    db::GPolygon *res = new db::GPolygon (from ,to);
    res->set_id (++m_id);
    mp_polygons.push_back (res);
    return res;
  }

  void remove_polygon (db::GPolygon *tri);
  template<class Poly, class Trans> void make_contours (const Poly &poly, const Trans &trans, std::vector<std::vector<db::GVertex *> > &contours);
};

}

#endif

