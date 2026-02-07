
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#ifndef HDR_dbPLC
#define HDR_dbPLC

#include "dbCommon.h"
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

namespace plc
{

/**
 *  @brief A framework for piecewise linear curves
 *
 *  This framework implements classes for dealing with piecewise linear
 *  curves. It is the basis for triangulation and polygon decomposition
 *  algorithms.
 *
 *  The core class is the PLCGraph which is a collection of vertices,
 *  edges and edge loops (polygons). Vertices, edges and polygons form
 *  graphs.
 *
 *  A "vertex" (db::plc::Vertex) is a point. A point connects two or
 *  more edges.
 *  A vertex has some attributes:
 *  * 'precious': if set, the vertex is not removed during triangulation
 *                for example.
 *
 *  An "edge" (db::plc::Edge) is a line connecting two vertexes. The
 *  edge runs from vertex v1 to vertex v2. An edge separates two
 *  polygons (left and right, as seen in the run direction).
 *
 *  A "segment" as an edge that is part of an original polygon outline.
 *
 *  A "polygon" (db::plc::Polygon) is a loop of edges.
 */

class Polygon;
class Edge;
class Graph;

/**
 *  @brief A class representing a vertex in a Delaunay triangulation graph
 *
 *  The vertex carries information about the connected edges and
 *  an integer value that can be used in traversal algorithms
 *  ("level")
 */
class DB_PUBLIC Vertex
  : public db::DPoint
{
public:
  typedef std::list<Edge *> edges_type;
  typedef edges_type::const_iterator edges_iterator;
  typedef edges_type::iterator edges_iterator_non_const;

  Vertex (const Vertex &v);
  Vertex &operator= (const Vertex &v);

  /**
   *  @brief Gets a value indicating whether any of the attached edges is "outside"
   */
  bool is_outside () const;

  /**
   *  @brief Gets a value indicating whether is on the outline - i.e. one edge is a segment
   */
  bool is_on_outline () const;

  /**
   *  @brief Gets a list of polygons that are attached to this vertex
   */
  std::vector<Polygon *> polygons() const;

  /**
   *  @brief Gets the graph object this vertex belongs to
   */
  Graph *graph () const { return mp_graph; }

  /**
   *  @brief Iterates the edges on this vertex (begin)
   */
  edges_iterator begin_edges () const { return mp_edges.begin (); }

  /**
   *  @brief Iterates the edges on this vertex (end)
   */
  edges_iterator end_edges () const { return mp_edges.end (); }

  /**
   *  @brief Returns the number of edges attached to this vertex
   */
  size_t num_edges (int max_count = -1) const;

  /**
   *  @brief Returns a value indicating whether the given edge is attached to this vertex
   */
  bool has_edge (const Edge *edge) const;

  /**
   *  @brief Sets a value indicating whether the vertex is precious
   *
   *  "precious" vertexes are not removed during triangulation for example.
   */
  void set_is_precious (bool f, unsigned int id);

  /**
   *  @brief Gets a value indicating whether the vertex is precious
   */
  bool is_precious () const;

  /**
   *  @brief Gets the ID passed to "set_is_precious"
   *
   *  This ID can be used to identify the vertex in the context it came from (e.g.
   *  index in point vector).
   */
  const std::set<unsigned int> &ids () const;

  /**
   *  @brief Returns a string representation of the vertex
   */
  std::string to_string (bool with_id = false) const;

  /**
   *  @brief Returns 1 is the point is inside the circle, 0 if on the circle and -1 if outside
   */
  static int in_circle (const db::DPoint &point, const db::DPoint &center, double radius);

  /**
   *  @brief Returns 1 is this point is inside the circle, 0 if on the circle and -1 if outside
   */
  int in_circle (const db::DPoint &center, double radius) const
  {
    return in_circle (*this, center, radius);
  }

protected:
  Vertex (Graph *graph);
  Vertex (Graph *graph, const DPoint &p);
  Vertex (Graph *graph, const Vertex &v);
  Vertex (Graph *graph, db::DCoord x, db::DCoord y);
  ~Vertex ();

private:
  friend class Edge;
  friend class Graph;
  friend class tl::stable_vector<Vertex>;

  void remove_edge (const edges_iterator_non_const &ec)
  {
    mp_edges.erase (ec);
  }

  Graph *mp_graph;
  edges_type mp_edges;
  std::set<unsigned int> *mp_ids;
};

/**
 *  @brief A class representing an edge in the Delaunay triangulation graph
 */
class DB_PUBLIC Edge
{
public:
  class PolygonIterator
  {
  public:
    typedef Polygon value_type;
    typedef Polygon &reference;
    typedef Polygon *pointer;

    reference operator*() const
    {
      return *operator-> ();
    }

    pointer operator->() const
    {
      return m_index ? mp_edge->right () : mp_edge->left ();
    }

    bool operator== (const PolygonIterator &other) const
    {
      return m_index == other.m_index;
    }

    bool operator!= (const PolygonIterator &other) const
    {
      return !operator== (other);
    }

    PolygonIterator &operator++ ()
    {
      while (++m_index < 2 && operator-> () == 0)
        ;
      return *this;
    }

  private:
    friend class Edge;

    PolygonIterator (const Edge *edge)
      : mp_edge (edge), m_index (0)
    {
      if (! edge) {
        m_index = 2;
      } else {
        --m_index;
        operator++ ();
      }
    }

    const Edge *mp_edge;
    unsigned int m_index;
  };

  /**
   *  @brief Gets the first vertex ("from")
   */
  Vertex *v1 () const { return mp_v1; }

  /**
   *  @brief Gets the first vertex ("to")
   */
  Vertex *v2 () const { return mp_v2; }

  /**
   *  @brief Reverses the edge
   */
  void reverse ()
  {
    std::swap (mp_v1, mp_v2);
    std::swap (mp_left, mp_right);
  }

  /**
   *  @brief Gets the polygon on the left side (can be null)
   */
  Polygon *left  () const { return mp_left; }

  /**
   *  @brief Gets the polygon on the right side (can be null)
   */
  Polygon *right () const { return mp_right; }

  /**
   *  @brief Iterates the polygons (one or two, begin iterator)
   */
  PolygonIterator begin_polygons () const
  {
    return PolygonIterator (this);
  }

  /**
   *  @brief Iterates the polygons (end iterator)
   */
  PolygonIterator end_polygons () const
  {
    return PolygonIterator (0);
  }

  /**
   *  @brief Gets a value indicating whether the edge is a segment
   */
  bool is_segment () const { return m_is_segment; }

  /**
   *  @brief Gets the edge ID (a unique identifier)
   */
  size_t id () const { return m_id; }

  /**
   *  @brief Gets a string representation of the edge
   */
  std::string to_string (bool with_id = false) const;

  /**
   *  @brief Converts to a db::DEdge
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
  bool crosses (const Edge &other) const
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
  bool crosses_including (const Edge &other) const
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
  db::DPoint intersection_point (const Edge &other) const
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
  Polygon *other (const Polygon *) const;

  /**
   *  @brief Gets the other vertex for the given one
   */
  Vertex *other (const Vertex *) const;

  /**
   *  @brief Gets a value indicating whether the edge has the given vertex
   */
  bool has_vertex (const Vertex *) const;

  /**
   *  @brief Gets the common vertex of the other edge and this edge or null if there is no common vertex
   */
  Vertex *common_vertex (const Edge *other) const;

  /**
   *  @brief Returns a value indicating whether this edge can be flipped
   */
  bool can_flip () const;

  /**
   *  @brief Returns a value indicating whether the edge separates two triangles that can be joined into one (via the given vertex)
   */
  bool can_join_via (const Vertex *vertex) const;

  /**
   *  @brief Returns a value indicating whether this edge belongs to outside triangles
   */
  bool is_for_outside_triangles () const;

  /**
   *  @brief Returns a value indicating whether this edge is an outside edge (no other triangles)
   */
  bool is_outside () const;

  /**
   *  @brief Returns a value indicating whether t is attached to this edge
   */
  bool has_polygon (const Polygon *t) const;

protected:
  void unlink ();
  void link ();

  void set_level (size_t l) { m_level = l; }
  size_t level () const { return m_level; }

  void set_id (size_t id) { m_id = id; }
  void set_is_segment (bool is_seg) { m_is_segment = is_seg; }

  Edge (Graph *graph);
  Edge (Graph *graph, Vertex *v1, Vertex *v2);
  ~Edge ();

private:
  friend class Polygon;
  friend class Graph;
  friend class Triangulation;
  friend class ConvexDecomposition;
  friend class tl::stable_vector<Edge>;

  Graph *mp_graph;
  Vertex *mp_v1, *mp_v2;
  Polygon *mp_left, *mp_right;
  Vertex::edges_iterator_non_const m_ec_v1, m_ec_v2;
  size_t m_level;
  size_t m_id;
  bool m_is_segment;

  void set_left  (Polygon *t);
  void set_right (Polygon *t);
};

/**
 *  @brief A compare function that compares edges by ID
 *
 *  The ID acts as a more predicable unique ID for the object in sets and maps.
 */
struct EdgeLessFunc
{
  bool operator () (Edge *a, Edge *b) const
  {
    return a->id () < b->id ();
  }
};

/**
 *  @brief A class representing a polygon
 */
class DB_PUBLIC Polygon
  : public tl::list_node<Polygon>, public tl::Object
{
public:

  /**
   *  @brief Destructor
   *
   *  It is legal to delete a polygon object to remove it.
   */
  ~Polygon ();

  /**
   *  @brief Detaches a polygon object from the edges
   */
  void unlink ();

  /**
   *  @brief Gets the polygon's unique ID
   */
  size_t id () const { return m_id; }

  /**
   *  @brief Gets a value indicating whether the polygon is an outside polygon
   *
   *  Outside polygons are polygons that fill concave parts in a triangulation for example.
   */
  bool is_outside () const { return m_is_outside; }

  /**
   *  @brief Returns a string representation
   */
  std::string to_string (bool with_id = false) const;

  /**
   *  @brief Gets the number of vertexes
   */
  size_t size () const
  {
    return mp_e.size ();
  }

  /**
   *  @brief Gets the internal vertexes
   *
   *  Internal vertexes are special points inside the polygons.
   */
  size_t internal_vertexes () const
  {
    return mp_v.size () - mp_e.size ();
  }

  /**
   *  @brief Adds a vertex as an internal vertex
   */
  void add_internal_vertex (Vertex *v)
  {
    mp_v.push_back (v);
  }

  /**
   *  @brief Reserves for n internal vertexes
   */
  void reserve_internal_vertexes (size_t n)
  {
    mp_v.reserve (mp_v.size () + n);
  }

  /**
   *  @brief Gets the nth vertex (n wraps around and can be negative)
   *  The vertexes are oriented clockwise.
   */
  inline Vertex *vertex (int n) const
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
   *  @brief Gets the nth internal vertex
   */
  inline Vertex *internal_vertex (size_t n) const
  {
    return mp_v[mp_e.size () + n];
  }

  /**
   *  @brief Gets the nth edge (n wraps around and can be negative)
   */
  inline Edge *edge (int n) const
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

  /**
   *  @brief Returns a DPolygon object for this polygon
   */
  db::DPolygon polygon () const;

  /**
   *  @brief Gets the center point and radius of the circumcircle
   *  If ok is non-null, it will receive a boolean value indicating whether the circumcircle is valid.
   *  An invalid circumcircle is an indicator for a degenerated triangle with area 0 (or close to).
   *
   *  This method only applies to triangles.
   */
  std::pair<db::DPoint, double> circumcircle (bool *ok = 0) const;

  /**
   *  @brief Gets the vertex opposite of the given edge
   *
   *  This method only applies to triangles.
   */
  Vertex *opposite (const Edge *edge) const;

  /**
   *  @brief Gets the edge opposite of the given vertex
   *
   *  This method only applies to triangles.
   */
  Edge *opposite (const Vertex *vertex) const;

  /**
   *  @brief Gets the edge with the given vertexes
   */
  Edge *find_edge_with (const Vertex *v1, const Vertex *v2) const;

  /**
   *  @brief Finds the common edge for both polygons
   */
  Edge *common_edge (const Polygon *other) const;

  /**
   *  @brief Returns a value indicating whether the point is inside (1), on the polygon (0) or outside (-1)
   *
   *  This method only applies to triangles currently.
   */
  int contains (const db::DPoint &point) const;

  /**
   *  @brief Gets a value indicating whether the triangle has the given vertex
   */
  inline bool has_vertex (const Vertex *v) const
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
  inline bool has_edge (const Edge *e) const
  {
    for (auto i = mp_e.begin (); i != mp_e.end (); ++i) {
      if (*i == e) {
        return true;
      }
    }
    return false;
  }

  /**
   *  @brief Coming from an edge e and the vertex v, gets the next edge
   */
  Edge *next_edge (const Edge *e, const Vertex *v) const;

  /**
   *  @brief Returns the minimum edge length
   */
  double min_edge_length () const;

  /**
   *  @brief Returns the min edge length to circumcircle radius ratio
   *
   *  This method only applies to triangles currently.
   */
  double b () const;

  /**
   *  @brief Returns a value indicating whether the polygon borders to a segment
   */
  bool has_segment () const;

  /**
   *  @brief Returns the number of segments the polygon borders to
   */
  unsigned int num_segments () const;

protected:
  Polygon (Graph *graph);
  Polygon (Graph *graph, Edge *e1, Edge *e2, Edge *e3);

  template<class Iter>
  Polygon (Graph *graph, Iter from, Iter to)
    : mp_graph (graph), mp_e (from, to)
  {
    init ();
  }

  void set_outside (bool o) { m_is_outside = o; }
  void set_id (size_t id) { m_id = id; }

private:
  friend class Graph;
  friend class Triangulation;

  Graph *mp_graph;
  bool m_is_outside;
  std::vector<Edge *> mp_e;
  std::vector<Vertex *> mp_v;
  size_t m_id;

  void init ();

  //  no copying
  Polygon &operator= (const Polygon &);
  Polygon (const Polygon &);
};

/**
 *  @brief A compare function that compares polygons by ID
 *
 *  The ID acts as a more predicable unique ID for the object in sets and maps.
 */
struct PolygonLessFunc
{
  bool operator () (Polygon *a, Polygon *b) const
  {
    return a->id () < b->id ();
  }
};

/**
 *  @brief A class representing the polygon graph
 *
 *  A polygon graph is the main container, holding vertexes, edges and polygons.
 *  The graph can be of "triangles" type, in which case it is guaranteed to only
 *  hold triangles (polygons with 3 vertexes).
 */
class DB_PUBLIC Graph
  : public tl::Object
{
public:
  typedef tl::list<Polygon> polygons_type;
  typedef polygons_type::const_iterator polygon_iterator;

  Graph ();
  ~Graph ();

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

protected:
  Vertex *create_vertex (double x, double y);
  Vertex *create_vertex (const db::DPoint &pt);
  Edge *create_edge (Vertex *v1, Vertex *v2);

  template <class Iter>
  Polygon *
  create_polygon (Iter from, Iter to)
  {
    Polygon *res = new Polygon (this, from ,to);
    res->set_id (++m_id);
    mp_polygons.push_back (res);
    return res;
  }

  Polygon *create_triangle (Edge *e1, Edge *e2, Edge *e3);

  void remove_polygon (Polygon *p);

private:
  friend class Triangulation;
  friend class ConvexDecomposition;

  tl::list<Polygon> mp_polygons;
  tl::stable_vector<Edge> m_edges_heap;
  std::vector<Edge *> m_returned_edges;
  tl::stable_vector<Vertex> m_vertex_heap;
  size_t m_id;

  tl::list<Polygon> &polygons () { return mp_polygons; }
  tl::stable_vector<Edge> &edges () { return m_edges_heap; }
  tl::stable_vector<Vertex> &vertexes () { return m_vertex_heap; }
};

} //  namespace plc

} //  namespace db

#endif

