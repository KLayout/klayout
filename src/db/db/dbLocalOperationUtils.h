
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



#ifndef HDR_dbLocalOperationUtils
#define HDR_dbLocalOperationUtils

#include "dbCommon.h"

#include "dbLayout.h"
#include "dbPolygonGenerators.h"
#include "dbHash.h"

#include <unordered_set>

namespace db
{

template <class Trans>
class polygon_transformation_filter
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor specifying an external vector for storing the polygons
   */
  polygon_transformation_filter (PolygonSink *output, const Trans &tr)
    : mp_output (output), m_trans (tr)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Polygon &polygon)
  {
    mp_output->put (polygon.transformed (m_trans));
  }

private:
  db::PolygonSink *mp_output;
  const Trans m_trans;
};

class PolygonRefGenerator
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor
   */
  PolygonRefGenerator (db::Layout *layout, std::unordered_set<db::PolygonRef> &polyrefs);

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Polygon &polygon);

private:
  db::Layout *mp_layout;
  std::unordered_set<db::PolygonRef> *mp_polyrefs;
};

class EdgeToEdgeSetGenerator
  : public EdgeSink
{
public:
  /**
   *  @brief Constructor
   */
  EdgeToEdgeSetGenerator (std::unordered_set<db::Edge> &edges);

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Edge &edge);

private:
  std::unordered_set<db::Edge> *mp_edges;
};

class PolygonRefToShapesGenerator
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor specifying an external vector for storing the polygons
   */
  PolygonRefToShapesGenerator (db::Layout *layout, db::Shapes *shapes);

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Polygon &polygon);

private:
  db::Layout *mp_layout;
  db::Shapes *mp_shapes;
};

class PolygonSplitter
  : public PolygonSink
{
public:
  PolygonSplitter (PolygonSink &sink, double max_area_ratio, size_t max_vertex_count);

  virtual void put (const db::Polygon &poly);

  virtual void start () { mp_sink->start (); }
  virtual void flush () { mp_sink->flush (); }

private:
  PolygonSink *mp_sink;
  double m_max_area_ratio;
  size_t m_max_vertex_count;
};

}

#endif

