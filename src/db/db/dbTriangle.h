
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



#ifndef HDR_dbTriangle
#define HDR_dbTriangle

#include "dbCommon.h"
#include "dbPoint.h"
#include "dbEdge.h"

#include "tlObjectCollection.h"
#include "tlList.h"

#include <vector>
#include <string>
#include <list>
#include <algorithm>

namespace db
{

class Triangle;
class TriangleEdge;

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
  typedef std::list<TriangleEdge *> edges_type;
  typedef edges_type::const_iterator edges_iterator;
  typedef edges_type::iterator edges_iterator_non_const;

  Vertex ();
  Vertex (const DPoint &p);
  Vertex (const Vertex &v);
  Vertex (db::DCoord x, db::DCoord y);

  Vertex &operator= (const Vertex &v);

  bool is_outside () const;
  std::vector<db::Triangle *> triangles () const;

  edges_iterator begin_edges () const { return mp_edges.begin (); }
  edges_iterator end_edges () const { return mp_edges.end (); }
  size_t num_edges (int max_count = -1) const;

  bool has_edge (const TriangleEdge *edge) const;

  size_t level () const { return m_level; }
  void set_level (size_t l) { m_level = l; }

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
  friend class TriangleEdge;

  void remove_edge (const edges_iterator_non_const &ec)
  {
    mp_edges.erase (ec);
  }

  edges_type mp_edges;
  size_t m_level;
};

/**
 *  @brief A class representing an edge in the Delaunay triangulation graph
 */
class DB_PUBLIC TriangleEdge
{
public:
  class TriangleIterator
  {
  public:
    typedef Triangle value_type;
    typedef Triangle &reference;
    typedef Triangle *pointer;

    reference operator*() const
    {
      return *operator-> ();
    }

    pointer operator->() const
    {
      return m_index ? mp_edge->right () : mp_edge->left ();
    }

    bool operator== (const TriangleIterator &other) const
    {
      return m_index == other.m_index;
    }

    bool operator!= (const TriangleIterator &other) const
    {
      return !operator== (other);
    }

    TriangleIterator &operator++ ()
    {
      while (++m_index < 2 && operator-> () == 0)
        ;
      return *this;
    }

  private:
    friend class TriangleEdge;

    TriangleIterator (const TriangleEdge *edge)
      : mp_edge (edge), m_index (0)
    {
      if (! edge) {
        m_index = 2;
      } else {
        --m_index;
        operator++ ();
      }
    }

    const TriangleEdge *mp_edge;
    unsigned int m_index;
  };

  TriangleEdge ();
  TriangleEdge (Vertex *v1, Vertex *v2);

  Vertex *v1 () const { return mp_v1; }
  Vertex *v2 () const { return mp_v2; }

  void reverse ()
  {
    std::swap (mp_v1, mp_v2);
    std::swap (mp_left, mp_right);
  }

  Triangle *left  () const { return mp_left; }
  Triangle *right () const { return mp_right; }

  TriangleIterator begin_triangles () const
  {
    return TriangleIterator (this);
  }

  TriangleIterator end_triangles () const
  {
    return TriangleIterator (0);
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
  bool crosses (const db::TriangleEdge &other) const
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
  bool crosses_including (const db::TriangleEdge &other) const
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
  db::DPoint intersection_point (const TriangleEdge &other) const
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
  Triangle *other (const Triangle *) const;

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
  Vertex *common_vertex (const TriangleEdge *other) const;

  /**
   *  @brief Returns a value indicating whether this edge can be flipped
   */
  bool can_flip () const;

  /**
   *  @brief Returns a value indicating whether the edge separates two triangles that can be joined into one (via the given vertex)
   */
  bool can_join_via (const Vertex *vertex) const;

  /**
   *  @brief Returns a value indicating whether this edge is an outside edge (no other triangles)
   */
  bool is_outside () const;

  /**
   *  @brief Returns a value indicating whether this edge belongs to outside triangles
   */
  bool is_for_outside_triangles () const;

  /**
   *  @brief Returns a value indicating whether t is attached to this edge
   */
  bool has_triangle (const Triangle *t) const;

protected:
  void unlink ();
  void link ();

private:
  friend class Triangle;
  friend class Triangles;

  Vertex *mp_v1, *mp_v2;
  Triangle *mp_left, *mp_right;
  Vertex::edges_iterator_non_const m_ec_v1, m_ec_v2;
  size_t m_level;
  size_t m_id;
  bool m_is_segment;

  void set_left  (Triangle *t);
  void set_right (Triangle *t);
};

/**
 *  @brief A compare function that compares triangles by ID
 *
 *  The ID acts as a more predicable unique ID for the object in sets and maps.
 */
struct TriangleEdgeLessFunc
{
  bool operator () (TriangleEdge *a, TriangleEdge *b) const
  {
    return a->id () < b->id ();
  }
};

/**
 *  @brief A class representing a triangle
 */
class DB_PUBLIC Triangle
  : public tl::list_node<Triangle>, public tl::Object
{
public:
  Triangle ();
  Triangle (TriangleEdge *e1, TriangleEdge *e2, TriangleEdge *e3);

  ~Triangle ();

  void unlink ();

  void set_id (size_t id) { m_id = id; }
  size_t id () const { return m_id; }

  bool is_outside () const { return m_is_outside; }
  void set_outside (bool o) { m_is_outside = o; }

  std::string to_string (bool with_id = false) const;

  /**
   *  @brief Gets the nth vertex (n wraps around and can be negative)
   *  The vertexes are oriented clockwise.
   */
  inline Vertex *vertex (int n) const
  {
    if (n >= 0 && n < 3) {
      return mp_v[n];
    } else {
      return mp_v[(n + 3) % 3];
    }
  }

  /**
   *  @brief Gets the nth edge (n wraps around and can be negative)
   */
  inline TriangleEdge *edge (int n) const
  {
    if (n >= 0 && n < 3) {
      return mp_e[n];
    } else {
      return mp_e[(n + 3) % 3];
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
   *  @brief Gets the center point and radius of the circumcircle
   */
  std::pair<db::DPoint, double> circumcircle () const;

  /**
   *  @brief Gets the vertex opposite of the given edge
   */
  Vertex *opposite (const TriangleEdge *edge) const;

  /**
   *  @brief Gets the edge opposite of the given vertex
   */
  TriangleEdge *opposite (const Vertex *vertex) const;

  /**
   *  @brief Gets the edge with the given vertexes
   */
  TriangleEdge *find_edge_with (const Vertex *v1, const Vertex *v2) const;

  /**
   *  @brief Finds the common edge for both triangles
   */
  TriangleEdge *common_edge (const Triangle *other) const;

  /**
   *  @brief Returns a value indicating whether the point is inside (1), on the triangle (0) or outside (-1)
   */
  int contains (const db::DPoint &point) const;

  /**
   *  @brief Gets a value indicating whether the triangle has the given vertex
   */
  inline bool has_vertex (const db::Vertex *v) const
  {
    return mp_v[0] == v || mp_v[1] == v || mp_v[2] == v;
  }

  /**
   *  @brief Gets a value indicating whether the triangle has the given edge
   */
  inline bool has_edge (const db::TriangleEdge *e) const
  {
    return mp_e[0] == e || mp_e[1] == e || mp_e[2] == e;
  }

  /**
   *  @brief Returns the minimum edge length
   */
  double min_edge_length () const;

  /**
   *  @brief Returns the min edge length to circumcircle radius ratio
   */
  double b () const;

  /**
   *  @brief Returns a value indicating whether the triangle borders to a segment
   */
  bool has_segment () const;

  /**
   *  @brief Returns the number of segments the triangle borders to
   */
  unsigned int num_segments () const;

private:
  bool m_is_outside;
  TriangleEdge *mp_e[3];
  db::Vertex *mp_v[3];
  size_t m_id;

  //  no copying
  Triangle &operator= (const Triangle &);
  Triangle (const Triangle &);
};

/**
 *  @brief A compare function that compares triangles by ID
 *
 *  The ID acts as a more predicable unique ID for the object in sets and maps.
 */
struct TriangleLessFunc
{
  bool operator () (Triangle *a, Triangle *b) const
  {
    return a->id () < b->id ();
  }
};

}

#endif

