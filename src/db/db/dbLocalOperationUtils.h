
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



#ifndef HDR_dbLocalOperationUtils
#define HDR_dbLocalOperationUtils

#include "dbCommon.h"

#include "dbLayout.h"
#include "dbPropertyConstraint.h"
#include "dbPolygonGenerators.h"
#include "dbLocalOperation.h"
#include "dbHash.h"
#include "tlThreads.h"

#include <unordered_set>

namespace db
{

class PropertyMapper;

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

template <class T>
class DB_PUBLIC polygon_ref_generator;

template <>
class DB_PUBLIC polygon_ref_generator<db::PolygonRef>
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor
   */
  polygon_ref_generator (db::Layout *layout, std::unordered_set<db::PolygonRef> &polyrefs)
    : PolygonSink (), mp_layout (layout), mp_polyrefs (&polyrefs)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  void put (const db::Polygon &polygon)
  {
    tl::MutexLocker locker (&mp_layout->lock ());
    mp_polyrefs->insert (db::PolygonRef (polygon, mp_layout->shape_repository ()));
  }

private:
  db::Layout *mp_layout;
  std::unordered_set<db::PolygonRef> *mp_polyrefs;
};

template <>
class DB_PUBLIC polygon_ref_generator<db::Polygon>
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor
   */
  polygon_ref_generator (db::Layout *, std::unordered_set<db::Polygon> &polygons)
    : mp_polygons (&polygons)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Polygon &polygon)
  {
    mp_polygons->insert (polygon);
  }

private:
  std::unordered_set<db::Polygon> *mp_polygons;
};

typedef polygon_ref_generator<db::PolygonRef> PolygonRefGenerator;

template <class T>
class DB_PUBLIC polygon_ref_generator_with_properties;

template <>
class DB_PUBLIC polygon_ref_generator_with_properties<db::PolygonRefWithProperties>
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor
   */
  polygon_ref_generator_with_properties (db::Layout *layout, std::unordered_set<db::PolygonRefWithProperties> &polyrefs, db::properties_id_type prop_id)
    : PolygonSink (), mp_layout (layout), mp_polyrefs (&polyrefs), m_prop_id (prop_id)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  void put (const db::Polygon &polygon)
  {
    tl::MutexLocker locker (&mp_layout->lock ());
    mp_polyrefs->insert (db::PolygonRefWithProperties (db::PolygonRef (polygon, mp_layout->shape_repository ()), m_prop_id));
  }

private:
  db::Layout *mp_layout;
  std::unordered_set<db::PolygonRefWithProperties> *mp_polyrefs;
  db::properties_id_type m_prop_id;
};

template <>
class DB_PUBLIC polygon_ref_generator_with_properties<db::PolygonWithProperties>
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor
   */
  polygon_ref_generator_with_properties (db::Layout *, std::unordered_set<db::PolygonWithProperties> &polygons, db::properties_id_type prop_id)
    : mp_polygons (&polygons), m_prop_id (prop_id)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Polygon &polygon)
  {
    mp_polygons->insert (db::PolygonWithProperties (polygon, m_prop_id));
  }

private:
  std::unordered_set<db::PolygonWithProperties> *mp_polygons;
  db::properties_id_type m_prop_id;
};

typedef polygon_ref_generator<db::PolygonRef> PolygonRefGenerator;

template <class Container>
class DB_PUBLIC edge_to_edge_set_generator
  : public EdgeSink
{
public:
  /**
   *  @brief Constructor
   */
  edge_to_edge_set_generator (Container &edges, int tag = 0, EdgeSink *chained = 0)
    : mp_edges (&edges), m_tag (tag), mp_chained (chained)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Edge &edge)
  {
    if (mp_edges) {
      mp_edges->insert (edge);
    }
    if (mp_chained) {
      mp_chained->put (edge);
    }
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Edge &edge, int tag)
  {
    if (m_tag == 0 || m_tag == tag) {
      if (mp_edges) {
        mp_edges->insert (edge);
      }
    }
    if (mp_chained) {
      mp_chained->put (edge, tag);
    }
  }

private:
  Container *mp_edges;
  int m_tag;
  EdgeSink *mp_chained;
};

typedef edge_to_edge_set_generator<std::unordered_set<db::Edge> > EdgeToEdgeSetGenerator;

class DB_PUBLIC PolygonRefToShapesGenerator
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor specifying an external vector for storing the polygons
   */
  PolygonRefToShapesGenerator (db::Layout *layout, db::Shapes *shapes, db::properties_id_type prop_id = 0);

  /**
   *  @brief Sets the property ID to be used for the next polygon
   */
  void set_prop_id (db::properties_id_type prop_id)
  {
    m_prop_id = prop_id;
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Polygon &polygon);

private:
  db::Layout *mp_layout;
  db::Shapes *mp_shapes;
  db::properties_id_type m_prop_id;
};

class DB_PUBLIC PolygonSplitter
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

template <class T, class Container>
class DB_PUBLIC property_injector
{
public:
  typedef typename Container::const_iterator const_iterator;

  property_injector (Container *container, db::properties_id_type prop_id)
    : mp_container (container), m_prop_id (prop_id)
  {
    //  .. nothing yet ..
  }

  const_iterator begin () const
  {
    return mp_container->begin ();
  }

  const_iterator end () const
  {
    return mp_container->end ();
  }

  void insert (const T &t)
  {
    mp_container->insert (db::object_with_properties<T> (t, m_prop_id));
  }

private:
  Container *mp_container;
  db::properties_id_type m_prop_id;
};

/**
 *  @brief Separates the interacting shapes by property relation
 *
 *  Returns a map of property ID, subject shapes and intruder shapes belonging to the subject shapes.
 *  Depending on the property constraint the intruders will either be ones with and properties (NoPropertyConstraint),
 *  the same properties than the subject (SamePropertiesConstraint) or different properties (DifferentPropertiesConstraint).
 */
template <class TS, class TI>
DB_PUBLIC_TEMPLATE
std::map<db::properties_id_type, std::pair<std::vector<const TS *>, std::set<const TI *> > >
separate_interactions_by_properties (const shape_interactions<db::object_with_properties<TS>, db::object_with_properties<TI> > &interactions, db::PropertyConstraint property_constraint, db::PropertyMapper &pms, db::PropertyMapper &pmi)
{
  std::map<db::properties_id_type, std::pair<std::vector<const TS *>, std::set<const TI *> > > by_prop_id;

  for (auto i = interactions.begin (); i != interactions.end (); ++i) {

    const db::object_with_properties<TS> &subject = interactions.subject_shape (i->first);

    db::properties_id_type prop_id = pms (subject.properties_id ());

    std::pair<std::vector<const TS *>, std::set<const TI *> > &s2p = by_prop_id [prop_id];
    s2p.first.push_back (&subject);

    for (auto ii = i->second.begin (); ii != i->second.end (); ++ii) {

      const std::pair<unsigned int, db::object_with_properties<TI> > &intruder = interactions.intruder_shape (*ii);

      if (pc_match (property_constraint, prop_id, pmi (intruder.second.properties_id ()))) {
        s2p.second.insert (&intruder.second);
      }

    }

  }

  return by_prop_id;
}

/**
 *  @brief Separates the interacting shapes by property relation
 *
 *  Returns a map of property ID, subject shapes and intruder shapes belonging to the subject shapes.
 *  Depending on the property constraint the intruders will either be ones with and properties (NoPropertyConstraint),
 *  the same properties than the subject (SamePropertiesConstraint) or different properties (DifferentPropertiesConstraint).
 */
template <class TS, class TI>
DB_PUBLIC_TEMPLATE
std::map<db::properties_id_type, db::shape_interactions<TS, TI> >
separate_interactions_to_interactions_by_properties (const shape_interactions<db::object_with_properties<TS>, db::object_with_properties<TI> > &interactions, db::PropertyConstraint property_constraint, db::PropertyMapper &pms, std::vector<db::PropertyMapper> &pmis)
{
  std::map<db::properties_id_type, db::shape_interactions<TS, TI> > by_prop_id;
  std::map<db::properties_id_type, std::set<unsigned int> > intruder_ids_by_prop_id;

  for (auto i = interactions.begin (); i != interactions.end (); ++i) {

    const db::object_with_properties<TS> &subject = interactions.subject_shape (i->first);
    db::properties_id_type prop_id = pms (subject.properties_id ());

    db::shape_interactions<TS, TI> &s2p = by_prop_id [prop_id];
    std::set<unsigned int> &intruder_ids = intruder_ids_by_prop_id [prop_id];
    s2p.add_subject (i->first, subject);

    for (auto ii = i->second.begin (); ii != i->second.end (); ++ii) {

      const std::pair<unsigned int, db::object_with_properties<TI> > &intruder = interactions.intruder_shape (*ii);
      tl_assert (intruder.first < (unsigned int) pmis.size ());

      if (pc_match (property_constraint, prop_id, pmis[intruder.first] (intruder.second.properties_id ()))) {
        s2p.add_interaction (i->first, *ii);
        intruder_ids.insert (*ii);
      }

    }

  }

  for (auto i = intruder_ids_by_prop_id.begin (); i != intruder_ids_by_prop_id.end (); ++i) {

    db::shape_interactions<TS, TI> &s2p = by_prop_id [i->first];
    const std::set<unsigned int> &intruder_ids = intruder_ids_by_prop_id [i->first];

    for (auto ii = intruder_ids.begin (); ii != intruder_ids.end (); ++ii) {
      auto is = interactions.intruder_shape (*ii);
      s2p.add_intruder_shape (*ii, is.first, is.second);
    }

  }

  return by_prop_id;
}

}

#endif

