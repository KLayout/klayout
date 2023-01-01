
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



#ifndef HDR_dbPolygonGenerators
#define HDR_dbPolygonGenerators

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbEdge.h"
#include "dbEdgeProcessor.h"
#include "dbPolygon.h"

#include <vector>

namespace db
{

class PGPolyContour;
class PGContourList;
struct PGPoint;
class PolygonSink;
class SimplePolygonSink;

/**
 *  @brief Forms polygons from a edge set
 *
 *  This class implements EdgeSink. It builds polygons from the edges delivered to it 
 *  and outputs the polygons to another receiver (PolygonSink). 
 *  The way how touching corners are resolved can be specified (minimum and maximum coherence).
 *  In addition, it can be specified if the resulting polygons contain holes are whether the 
 *  holes are attached to the hull contour by stich lines.
 */
class DB_PUBLIC PolygonGenerator
  : public EdgeSink
{
public:
  typedef std::list <PGPoint> open_map_type;
  typedef open_map_type::iterator open_map_iterator_type;

  /**
   *  @brief Constructor
   *
   *  This constructor takes the polygon receiver (of which is keeps a reference).
   *  It allows one to specify how holes are resolved and how touching corners are resolved.
   *
   *  @param psink The polygon receiver
   *  @param resolve_holes true, if holes should be resolved into the hull using stich lines
   *  @param min_coherence true, if the resulting polygons should be minimized (less holes, more polygons)
   */
  PolygonGenerator (PolygonSink &psink, bool resolve_holes = true, bool min_coherence = true); 

  /**
   *  @brief Constructor
   *
   *  This constructor takes the simple polygon receiver (of which is keeps a reference).
   *  It allows one to specify how touching corners are resolved. Holes are always resolved since this is
   *  the only way to create a simple polygon.
   *
   *  @param spsink The simple polygon receiver
   *  @param resolve_holes true, if holes should be resolved into the hull using stich lines
   *  @param min_coherence true, if the resulting polygons should be minimized (less holes, more polygons)
   */
  PolygonGenerator (SimplePolygonSink &spsink, bool min_coherence = true); 

  /**
   *  @brief Destructor
   */
  ~PolygonGenerator ();

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void start ();

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void flush ();

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void begin_scanline (db::Coord y);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void end_scanline (db::Coord y);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void crossing_edge (const db::Edge &e);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void skip_n (size_t n);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void put (const db::Edge &e);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void put (const db::Edge & /*e*/, int /*tag*/) { }

  /**
   *  @brief Sets the way how holes are resolved dynamically
   *
   *  This property should not be changed why polygons are created (between start and flush)
   */
  void resolve_holes (bool f) { m_resolve_holes = f; }

  /**
   *  @brief Enables open contours for hole resolution
   *
   *  With this property set to false (the default), holes are resolved with a single stitch
   *  line. This will create self-touching polygons finally. By setting this property to true,
   *  a different hole resolution strategy is chosen which resolves holes by inserting a new
   *  contour at the left of the hole.
   *
   *  Using is feature will result in a larger number but less complex polygons on output.
   */
  void open_contours (bool f) { m_open_contours = f; }

  /**
   *  @brief Sets the way how touching corners are resolved dynamically
   *
   *  This property should not be changed while polygons are created (between start and flush)
   */
  void min_coherence (bool f) { m_min_coherence = f; }

  /**
   *  @brief Disables or enable compression for polygon contours
   *
   *  If compression is disabled, no vertices will be dropped.
   */
  void enable_compression (bool enable) { m_compress = enable; }

  /**
   *  @brief Disables or enable compression for polygon contours
   *
   *  This method switches the global flag and is intended for regression test purposes only!
   */
  static void enable_compression_global (bool enable) { ms_compress = enable; }

private:
  PGContourList *mp_contours;
  open_map_type m_open;
  db::Coord m_y;
  open_map_iterator_type m_open_pos;
  PolygonSink *mp_psink;
  SimplePolygonSink *mp_spsink;
  bool m_resolve_holes;
  bool m_open_contours;
  bool m_min_coherence;
  db::Polygon m_poly;
  db::SimplePolygon m_spoly;
  static bool ms_compress;
  bool m_compress;

  void join_contours (db::Coord x);
  void produce_poly (const PGPolyContour &c);
  void eliminate_hole ();

  PolygonGenerator &operator= (const PolygonGenerator &);
  PolygonGenerator (const PolygonGenerator &);
};

/**
 *  @brief Forms trapezoids from an edge set
 *
 *  This class implements EdgeSink. It builds simple polygons from the edges delivered to it
 *  and outputs the polygons to another receiver (PolygonSink or SimplePolygonSink). The
 *  polygons created form a horizontal trapezoid decomposition of the full polygon.
 */
class DB_PUBLIC TrapezoidGenerator
  : public EdgeSink
{
public:
  typedef std::vector <std::pair<db::Edge, db::Edge> > edge_map_type;
  typedef edge_map_type::iterator edge_map_type_iterator;

  /**
   *  @brief Constructor
   *
   *  This constructor takes the polygon receiver (of which is keeps a reference).
   *  The trapezoids will be delivered to this sink.
   *
   *  @param psink The polygon receiver
   */
  TrapezoidGenerator (PolygonSink &psink);

  /**
   *  @brief Constructor
   *
   *  This constructor takes the polygon receiver (of which is keeps a reference).
   *  The trapezoids will be delivered to this sink.
   *
   *  @param spsink The simple polygon receiver
   */
  TrapezoidGenerator (SimplePolygonSink &spsink);

  /**
   *  @brief Destructor
   */
  ~TrapezoidGenerator ();

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void start ();

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void flush ();

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void begin_scanline (db::Coord y);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void end_scanline (db::Coord y);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void crossing_edge (const db::Edge &e);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void skip_n (size_t n);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void put (const db::Edge &e);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void put (const db::Edge & /*e*/, int /*tag*/) { }

private:
  db::Coord m_y;
  PolygonSink *mp_psink;
  SimplePolygonSink *mp_spsink;
  db::Polygon m_poly;
  db::SimplePolygon m_spoly;
  edge_map_type m_edges, m_new_edges;
  edge_map_type_iterator m_current_edge;
  std::vector<size_t> m_new_edge_refs;

  TrapezoidGenerator &operator= (const TrapezoidGenerator &);
  TrapezoidGenerator (const TrapezoidGenerator &);

  void make_trap (const db::Point (&pts)[4]);
};

/**
 *  @brief Declaration of the simple polygon sink interface
 */
class DB_PUBLIC SimplePolygonSink
{
public:
  /**
   *  @brief Constructor
   */
  SimplePolygonSink () { }

  /**
   *  @brief Destructor
   */
  virtual ~SimplePolygonSink () { }

  /**
   *  @brief Deliver a simple polygon
   *
   *  This method is called to deliver a new polygon
   */
  virtual void put (const db::SimplePolygon & /*polygon*/) { }

  /**
   *  @brief Start event
   *
   *  This method is called before the first polygon is delivered. 
   *  The PolygonGenerator will simply forward the EdgeSink's start method to the
   *  polygon sink.
   */
  virtual void start () { }

  /**
   *  @brief End event
   *
   *  This method is called after the last polygon was delivered. 
   *  The PolygonGenerator will deliver all remaining polygons and call flush then.
   */
  virtual void flush () { }
};

/**
 *  @brief A simple polygon receiver collecting the simple polygons in a vector
 *
 *  This class implements the SimplePolygonSink interface.
 *  Like EdgeContainer, this receiver collects the objects either in an external
 *  or an internal vector of polygons.
 */
class DB_PUBLIC SimplePolygonContainer
  : public SimplePolygonSink
{
public:
  /**
   *  @brief Constructor specifying an external vector for storing the polygons
   */
  SimplePolygonContainer (std::vector<db::SimplePolygon> &polygons, bool clear = false) 
    : SimplePolygonSink (), mp_polygons (&polygons), m_clear (clear) 
  { }

  /**
   *  @brief Constructor which tells the container to use the internal vector for storing the polygons
   */
  SimplePolygonContainer () 
    : SimplePolygonSink (), mp_polygons (&m_polygons), m_clear (false) 
  { }

  /**
   *  @brief Start the sequence
   */
  virtual void start ()
  {
    if (m_clear) {
      mp_polygons->clear ();
      //  The single-shot scheme is a easy way to overcome problems with multiple start/flush brackets (i.e. on size filter)
      m_clear = false;
    }
  }

  /**
   *  @brief The polygons collected so far (const version)
   */
  const std::vector<db::SimplePolygon> &polygons () const
  { 
    return *mp_polygons; 
  }

  /**
   *  @brief The polygons collected so far (non-const version)
   */
  std::vector<db::SimplePolygon> &polygons () 
  { 
    return *mp_polygons; 
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::SimplePolygon &polygon) 
  {
    mp_polygons->push_back (polygon);
  }

private:
  std::vector<db::SimplePolygon> m_polygons;
  std::vector<db::SimplePolygon> *mp_polygons;
  bool m_clear;
};

/**
 *  @brief Declaration of the polygon sink interface
 */
class DB_PUBLIC PolygonSink
{
public:
  /**
   *  @brief Constructor
   */
  PolygonSink () { }

  /**
   *  @brief Destructor
   */
  virtual ~PolygonSink () { }

  /**
   *  @brief Deliver a polygons
   *
   *  This method is called to deliver a new polygon
   */
  virtual void put (const db::Polygon &) { }

  /**
   *  @brief Start event
   *
   *  This method is called before the first polygon is delivered. 
   *  The PolygonGenerator will simply forward the EdgeSink's start method to the
   *  polygon sink.
   */
  virtual void start () { }

  /**
   *  @brief End event
   *
   *  This method is called after the last polygon was delivered. 
   *  The PolygonGenerator will deliver all remaining polygons and call flush then.
   */
  virtual void flush () { }
};

/**
 *  @brief A polygon receiver collecting the polygons in a vector
 *
 *  This class implements the PolygonSink interface.
 *  Like EdgeContainer, this receiver collects the objects either in an external
 *  or an internal vector of polygons.
 */
class DB_PUBLIC PolygonContainer
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor specifying an external vector for storing the polygons
   */
  PolygonContainer (std::vector<db::Polygon> &polygons, bool clear = false) 
    : PolygonSink (), mp_polygons (&polygons), m_clear (clear)
  { }

  /**
   *  @brief Constructor which tells the container to use the internal vector for storing the polygons
   */
  PolygonContainer () 
    : PolygonSink (), mp_polygons (&m_polygons), m_clear (false) 
  { }

  /**
   *  @brief The polygons collected so far (const version)
   */
  const std::vector<db::Polygon> &polygons () const
  { 
    return *mp_polygons; 
  }

  /**
   *  @brief The polygons collected so far (non-const version)
   */
  std::vector<db::Polygon> &polygons () 
  { 
    return *mp_polygons; 
  }

  /**
   *  @brief Start the sequence
   */
  virtual void start ()
  {
    if (m_clear) {
      mp_polygons->clear ();
      //  The single-shot scheme is a easy way to overcome problems with multiple start/flush brackets (i.e. on size filter)
      m_clear = false;
    }
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Polygon &polygon) 
  {
    mp_polygons->push_back (polygon);
  }

private:
  std::vector<db::Polygon> m_polygons;
  std::vector<db::Polygon> *mp_polygons;
  bool m_clear;
};

/**
 *  @brief A polygon filter that sizes the polygons 
 *
 *  This class implements the PolygonSink interface and delivers the sized polygons to an EdgeSink.
 */
class DB_PUBLIC SizingPolygonFilter
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor 
   */
  SizingPolygonFilter (EdgeSink &output, Coord dx, Coord dy, unsigned int mode)
    : PolygonSink (), mp_output (&output), m_dx (dx), m_dy (dy), m_mode (mode)
  { }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Polygon &polygon);

private:
  EdgeProcessor m_sizing_processor;
  EdgeSink *mp_output;
  Coord m_dx, m_dy;
  unsigned int m_mode;
};

}

#endif


