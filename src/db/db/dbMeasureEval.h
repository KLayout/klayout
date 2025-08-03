
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

#ifndef HDR_dbMeasureEval
#define HDR_dbMeasureEval

#include "dbRegion.h"
#include "dbEdges.h"
#include "dbEdgePairs.h"
#include "dbTexts.h"
#include "dbLayoutToNetlist.h"
#include "tlExpression.h"

namespace db
{

// -------------------------------------------------------------------------------------
//  Some utilities

/**
 *  @brief An evaluation context for the expressions
 *
 *  This class provides the methods, functions and variables for the expressions.
 */
class DB_PUBLIC MeasureEval
  : public tl::Eval
{
public:
  MeasureEval (double dbu, bool with_put);

  void init ();

  void reset (db::properties_id_type prop_id) const;
  void set_shape (const db::Polygon *poly) const;
  void set_shape (const db::PolygonRef *poly) const;
  void set_shape (const db::Edge *edge) const;
  void set_shape (const db::EdgePair *edge_pair) const;
  void set_shape (const db::Text *text) const;

  bool skip () const
  {
    return m_skip;
  }

  db::PropertiesSet &prop_set_out () const
  {
    return m_prop_set_out;
  }

protected:
  virtual void resolve_name (const std::string &name, const tl::EvalFunction *&function, const tl::Variant *&value, tl::Variant *&var);

private:
  friend class ShapeFunction;
  friend class SkipFunction;
  friend class ValueFunction;
  friend class ValuesFunction;
  friend class PropertyFunction;
  friend class PutFunction;

  union ShapeRef
  {
    const db::Polygon *poly;
    const db::PolygonRef *poly_ref;
    const db::Edge *edge;
    const db::EdgePair *edge_pair;
    const db::Text *text;
    void *any;
  };

  enum ShapeType
  {
    None,
    Polygon,
    PolygonRef,
    Edge,
    EdgePair,
    Text
  };

  mutable ShapeType m_shape_type;
  mutable ShapeRef mp_shape;
  mutable db::properties_id_type m_prop_id;
  mutable bool m_skip;
  mutable db::PropertiesSet m_prop_set_out;
  double m_dbu;
  bool m_with_put;

  tl::Variant shape_func () const;
  tl::Variant value_func (db::property_names_id_type name_id) const;
  tl::Variant value_func (const tl::Variant &name) const;
  tl::Variant values_func (const tl::Variant &name) const;
  void put_func (const tl::Variant &name, const tl::Variant &value) const;
  void skip_func (bool f) const;
};

/**
 *  @brief An evaluation context for net expressions
 *
 *  This class provides the methods, functions and variables for the expressions.
 */
class DB_PUBLIC MeasureNetEval
  : public tl::Eval
{
public:
  MeasureNetEval (const db::LayoutToNetlist *l2n, double dbu);

  void set_primary_layer (unsigned int layer_index);
  void set_secondary_layer (const std::string &name, unsigned int layer_index);
  void init ();

  void reset (db::cell_index_type cell_index, size_t cluster_id) const;

  bool skip () const
  {
    return m_skip;
  }

  db::PropertiesSet &prop_set_out () const
  {
    return m_prop_set_out;
  }

private:
  friend class NetPutFunction;
  friend class NetAreaFunction;
  friend class NetPerimeterFunction;
  friend class NetFunction;
  friend class NetSkipFunction;

  struct AreaAndPerimeter
  {
    AreaAndPerimeter () : area (0.0), perimeter (0.0) { }
    double area, perimeter;
  };

  const db::LayoutToNetlist *mp_l2n;
  double m_dbu;
  std::vector<unsigned int> m_layers;
  mutable bool m_skip;
  mutable db::PropertiesSet m_prop_set_out;
  mutable db::cell_index_type m_cell_index;
  mutable size_t m_cluster_id;
  mutable std::map<unsigned int, AreaAndPerimeter> m_area_and_perimeter_cache;
  mutable std::unique_ptr<std::map<std::pair<db::cell_index_type, size_t>, const db::Net *> > m_nets_per_cell_and_cluster_id;

  AreaAndPerimeter compute_area_and_perimeter (int layer_index) const;

  void put_func (const tl::Variant &name, const tl::Variant &value) const;
  tl::Variant area_func (int layer_index) const;
  tl::Variant perimeter_func (int layer_index) const;
  void skip_func (bool f) const;
  tl::Variant net_func () const;
};

}

#endif
