
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

#ifndef HDR_lstrCompressor
#define HDR_lstrCompressor

#include "dbVector.h"
#include "dbBox.h"
#include "dbPath.h"
#include "dbEdge.h"
#include "dbEdgePair.h"
#include "dbPolygon.h"
#include "dbText.h"
#include "dbInstances.h"
#include "dbObjectWithProperties.h"

#include <unordered_map>
#include <vector>

namespace lstr
{

const unsigned int max_lstreame_compression_level = 10;

/**
 *  @brief Compare operator for points, distinct x clustered (with same y)
 */
struct vector_cmp_x
{
  bool operator() (const db::Vector &a, const db::Vector &b) const
  {
    if (a.y () != b.y ()) {
      return a.y () < b.y ();
    } else {
      return a.x () < b.x ();
    }
  }
};

/**
 *  @brief Compare operator for points, distinct y clustered (with same x)
 */
struct vector_cmp_y
{
  bool operator() (const db::Vector &a, const db::Vector &b) const
  {
    if (a.x () != b.x ()) {
      return a.x () < b.x ();
    } else {
      return a.y () < b.y ();
    }
  }
};

/**
 *  @brief Returns the cost value of a coordinate difference (or coordinate)
 *  The cost is used to estimate the size cost of a coordinate difference 
 *  in the OASIS output.
 *  The cost is roughly the number of bytes required to represent the 
 *  number. It does not consider gdelta compression, actual byte count or similar.
 * 
 *  TODO: this heuristics is taken from OASIS. I should be adapted to LStream.
 */
inline double cost_of (double d)
{
  int exp = 0;
  frexp (d, &exp);
  return double ((exp + 7) / 8);
}

/**
 *  @brief A predicate describing whether an object is empty
 * 
 *  An object is "empty", if it does not have at least one reference point.
 *  For example, an empty box is empty. Such objects cannot be written and are
 *  stripped.
 */
template <class Object>
static inline bool object_is_empty (const Object &) { return false; }
static inline bool object_is_empty (const db::Box &object) { return object.empty (); }
static inline bool object_is_empty (const db::Polygon &object) { return object.hull ().begin () == object.hull ().end (); }
static inline bool object_is_empty (const db::SimplePolygon &object) { return object.hull ().begin () == object.hull ().end (); }
static inline bool object_is_empty (const db::Path &object) { return object.begin () == object.end (); }

/**
 *  @brief Normalization of the position of an object
 */
template <class Object>
static inline db::Vector reduce_object (Object &object)
{
  db::Disp tr;
  object.reduce (tr);
  return tr.disp ();
}

/**
 *  @brief Specialization for EdgePair which currently does not have "reduce"
 */
static inline db::Vector reduce_object_edge_pair (db::EdgePair &ep)
{
  db::EdgePair::vector_type d (ep.first ().p1 ());
  ep.move (-d);
  return d;
}

static inline db::Vector reduce_object (db::EdgePair &ep)
{
  return reduce_object_edge_pair (ep);
}

static inline db::Vector reduce_object (db::EdgePairWithProperties &ep)
{
  return reduce_object_edge_pair (ep);
}

/**
 *  @brief Specialization for EdgePair which currently does not have "reduce"
 */
static inline db::Vector reduce_object_point (db::Point &pt)
{
  db::Vector d = db::Vector (pt);
  pt = db::Point ();
  return d;
}

static inline db::Vector reduce_object (db::Point &pt)
{
  return reduce_object_point (pt);
}

static inline db::Vector reduce_object (db::PointWithProperties &pt)
{
  return reduce_object_point (pt);
}

/**
 *  @brief Specialization for CellInstArray
 */
static inline db::Vector reduce_object_cell_inst_array (db::CellInstArray &ci)
{
  db::Vector d = ci.front ().disp ();
  ci.move (-d);
  return d;
}

static inline db::Vector reduce_object (db::CellInstArray &ci)
{
  return reduce_object_cell_inst_array (ci);
}

static inline db::Vector reduce_object (db::CellInstArrayWithProperties &ci)
{
  return reduce_object_cell_inst_array (ci);
}

/**
 *  @brief Compare operator for points/abstract repetition pair with configurable point compare operator
 */
template <class PC>
struct rep_vector_cmp
{
  bool operator () (const std::pair <db::Vector, std::pair <db::Coord, int> > &a, const std::pair <db::Vector, std::pair <db::Coord, int> > &b)
  {
    if (a.second != b.second) {
      return a.second < b.second;
    }
    PC pc;
    return pc (a.first, b.first);
  }
};

/**
 *  @brief Represents a regular array
 * 
 *  A regular array is a set of displacements given by the
 *  formula
 * 
 *     d = ia*a + ib*b
 * 
 *  where "ia" is an integer running from 0 to na-1, "ib" is an integer
 *  running from 0 to nb-1 and "a" and "b" are two arbitrary vectors.
 * 
 *  The axes "a" and "b" do not need to be orthogonal in the general
 *  case, but they should not be collinear.
 * 
 *  "na" and "nb" are the dimensions of the array.
 * 
 *  An array can be "null", which means it does not represent any
 *  placements.
 */
class RegularArray
{
public:
  /**
   *  @brief Creates a null array
   */
  RegularArray ()
    : m_na (0), m_nb (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Creates an array with the given axes and dimensions
   * 
   *  @param a The "a" axis
   *  @param b The "b" axis
   *  @param na The "a" dimension
   *  @param nb The "b" dimension
   */
  RegularArray (const db::Vector &a, const db::Vector &b, size_t na, size_t nb)
    : m_a (a), m_b (b), m_na (na), m_nb (nb)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns a value indicating whether the array is a null array
   */
  bool is_null () const
  {
    return m_na == 0 || m_nb == 0;
  }

  /**
   *  @brief Gets the a axis
   */
  const db::Vector &a () const { return m_a; }

  /**
   *  @brief Gets the b axis
   */
  const db::Vector &b () const { return m_b; }

  /**
   *  @brief Gets the a dimension
   */
  size_t na () const { return m_na; }

  /**
   *  @brief Gets the b dimension
   */
  size_t nb () const { return m_nb; }

  /**
   *  @brief The equality operator
   */
  bool operator== (const RegularArray &other) const
  {
    return m_a == other.m_a && 
           m_b == other.m_b && 
           m_na == other.m_na && 
           m_nb == other.m_nb;
  }

  /**
   *  @brief The less operator
   * 
   *  This operator is provided for strict weak ordering
   *  for us of the array as a key in std::map or std::set.
   */
  bool operator< (const RegularArray &other) const
  {
    if (m_a != other.m_a) {
      return m_a < other.m_a;
    }
    if (m_b != other.m_b) {
      return m_b < other.m_b;
    }
    if (m_na != other.m_na) {
      return m_na < other.m_na;
    }
    if (m_nb != other.m_nb) {
      return m_nb < other.m_nb;
    }
    return false;
  }

private:
  db::Vector m_a, m_b;
  size_t m_na, m_nb;
};

/**
 *  @brief An interface by which the compressor delivers the results of the compression
 * 
 *  Note that we're lacking virtual templates, hence the large number of
 *  methods for every object type.
 */
class CompressorDelivery
{
public:
  typedef std::vector<db::Vector> disp_vector;

  virtual ~CompressorDelivery () { }

  virtual void write (const db::Point &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::PointWithProperties &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::Box &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::BoxWithProperties &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::Edge &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::EdgeWithProperties &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::EdgePair &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::EdgePairWithProperties &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::Polygon &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::PolygonWithProperties &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::SimplePolygon &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::SimplePolygonWithProperties &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::Path &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::PathWithProperties &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::Text &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::TextWithProperties &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::CellInstArray &obj, const RegularArray &array, const disp_vector &irregular) = 0;
  virtual void write (const db::CellInstArrayWithProperties &obj, const RegularArray &array, const disp_vector &irregular) = 0;
};

/**
 *  @brief The compressor object
 * 
 *  The task of the compressor object is to accept a serial stream
 *  of individual objects and arranging them into arrays as far as possible.
 * 
 *  Arrays can be regular (RegularArray) or enumerated (lists of placements).
 * 
 *  Individual objects are fed using the "add" method. Once all objects
 *  are fed "flush" can be used to deliver the compressed arrays to a
 *  "CompressorDelivery" object.
 * 
 *  Note that once "flush" is called, "add" should no longer be used.
 *  For compressing new objects, construct a fresh Compressor object.
 */
template <class Obj>
class Compressor 
{
public:
  /** 
   *  @brief Constructor
   *
   *  @param level The compression level 
   *
   *  Allowed levels are:
   *    0   - simple
   *    1   - form simple arrays
   *    2++ - search for 2nd, 3rd ... order neighbors
   */
  Compressor (unsigned int level)
    : m_level (level)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Adds a new object with the given displacement
   * 
   *  The object is supposed to be reduced (positioned at 0,0)
   *  already and the displacement specifies where the object
   *  was sitting originally.
   */
  void add (const Obj &obj, const db::Vector &disp)
  {
    if (! object_is_empty (obj)) {
      m_normalized [obj].push_back (disp);
    }
  }

  /**
   *  @brief Adds an object with reduction
   * 
   *  The object added can sit anywhere. Before it is added,
   *  it is reduced (positioned at 0,0) and the displacement
   *  is recorded for array formation.
   */
  void add (const Obj &obj)
  {
    if (! object_is_empty (obj)) {
      Obj red (obj);
      auto disp = reduce_object (red);
      m_normalized [red].push_back (disp);
    }
  }

  /**
   *  @brief Generates arrays and delivers when to the delivery interface
   * 
   *  This method will can "deliver->write (Object, ...)" as many times as
   *  needed.
   * 
   *  Note that single objects may be delivered as well. These are encoded
   *  as null regular arrays and empty irregular placement lists.
   */
  void flush (CompressorDelivery *delivery);

private:
  typedef std::vector<db::Vector> disp_vector;
  
  std::unordered_map <Obj, disp_vector> m_normalized;
  unsigned int m_level;
};

}

#endif

