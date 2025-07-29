
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

#ifndef HDR_gsiDeclDbMeasureHelpers
#define HDR_gsiDeclDbMeasureHelpers

#include "dbRegion.h"
#include "dbEdges.h"
#include "dbEdgePairs.h"
#include "dbTexts.h"
#include "dbRegionUtils.h"
#include "dbEdgesUtils.h"
#include "tlExpression.h"
#include "gsiClassBase.h"
#include "gsiDeclDbContainerHelpers.h"

namespace gsi
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

  void reset_shape () const;
  void set_shape (const db::Polygon *poly) const;
  void set_shape (const db::PolygonRef *poly) const;
  void set_shape (const db::Edge *edge) const;
  void set_shape (const db::EdgePair *edge_pair) const;
  void set_shape (const db::Text *text) const;
  void set_prop_id (db::properties_id_type prop_id) const;

  db::PropertiesSet &prop_set_out () const
  {
    return m_prop_set_out;
  }

protected:
  virtual void resolve_name (const std::string &name, const tl::EvalFunction *&function, const tl::Variant *&value, tl::Variant *&var);

private:
  friend class ShapeFunction;
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
  mutable db::PropertiesSet m_prop_set_out;
  double m_dbu;
  bool m_with_put;

  tl::Variant shape_func () const;
  tl::Variant value_func (db::property_names_id_type name_id) const;
  tl::Variant value_func (const tl::Variant &name) const;
  tl::Variant values_func (const tl::Variant &name) const;
  void put_func (const tl::Variant &name, const tl::Variant &value) const;
};

inline db::RecursiveShapeIterator
begin_iter (const db::Region *region)
{
  return region->merged_semantics () ? region->begin_merged_iter ().first : region->begin_iter ().first;
}

inline bool
is_merged (const db::Region *region)
{
  return region->merged_semantics ();
}

inline db::RecursiveShapeIterator
begin_iter (const db::Edges *edges)
{
  return edges->merged_semantics () ? edges->begin_merged_iter ().first : edges->begin_iter ().first;
}

inline bool
is_merged (const db::Edges *edges)
{
  return edges->merged_semantics ();
}

inline db::RecursiveShapeIterator
begin_iter (const db::EdgePairs *edge_pairs)
{
  return edge_pairs->begin_iter ().first;
}

inline bool
is_merged (const db::EdgePairs *)
{
  return false;
}

inline db::RecursiveShapeIterator
begin_iter (const db::Texts *texts)
{
  return texts->begin_iter ().first;
}

inline bool
is_merged (const db::Texts *)
{
  return false;
}

/**
 *  @brief A specialization of the shape processor
 *
 *  This class provides the evaluation of the expressions in the context of
 *  a specific shape and shape properties. It allows creating properties with
 *  a computed value.
 */
template <class ProcessorBase, class Container>
class property_computation_processor
  : public gsi::shape_processor_base<ProcessorBase>
{
public:
  typedef typename ProcessorBase::shape_type shape_type;
  typedef typename ProcessorBase::result_type result_type;

  property_computation_processor (const Container *container, const std::map<tl::Variant, std::string> &expressions, bool copy_properties, double dbu)
    : m_eval (dbu, true /*with_put*/), m_copy_properties (copy_properties), m_expression_strings (expressions)
  {
    if (container) {
      this->set_result_is_merged (is_merged (container));
    }

    m_eval.init ();

    //  compile the expressions
    for (auto e = m_expression_strings.begin (); e != m_expression_strings.end (); ++e) {
      m_expressions.push_back (std::make_pair (e->first.is_nil () ? db::property_names_id_type (0) : db::property_names_id (e->first), tl::Expression ()));
      tl::Extractor ex (e->second.c_str ());
      m_eval.parse (m_expressions.back ().second, ex);
    }
  }

  virtual void process (const db::object_with_properties<shape_type> &shape, std::vector<db::object_with_properties<shape_type> > &res) const
  {
    res.push_back (shape);

    m_eval.set_prop_id (shape.properties_id ());
    m_eval.set_shape (&shape);

    db::PropertiesSet &ps_out = m_eval.prop_set_out ();
    if (m_copy_properties) {
      ps_out = db::properties (shape.properties_id ());
      for (auto e = m_expressions.begin (); e != m_expressions.end (); ++e) {
        if (e->first != db::property_names_id_type (0)) {
          ps_out.erase (e->first);
        }
      }
    } else {
      ps_out.clear ();
    }

    for (auto e = m_expressions.begin (); e != m_expressions.end (); ++e) {
      if (e->first != db::property_names_id_type (0)) {
        ps_out.insert (e->first, e->second.execute ());
      }
    }

    for (auto e = m_expressions.begin (); e != m_expressions.end (); ++e) {
      if (e->first == db::property_names_id_type (0)) {
        e->second.execute ();
      }
    }

    res.back ().properties_id (db::properties_id (ps_out));
  }

public:
  MeasureEval m_eval;
  std::vector<std::pair<db::property_names_id_type, tl::Expression> > m_expressions;
  bool m_copy_properties;
  std::map<tl::Variant, std::string> m_expression_strings;
};

/**
 *  @brief A specialization of the shape processor
 *
 *  This class provides the evaluation of the expressions in the context of
 *  a specific shape and shape properties. It allows creating properties with
 *  a computed value.
 */
template <class FilterBase, class Container>
class expression_filter
  : public gsi::shape_filter_impl<FilterBase>
{
public:
  typedef typename FilterBase::shape_type shape_type;

  expression_filter (const std::string &expression, bool inverse, double dbu)
    : m_eval (dbu, false /*without put func*/), m_inverse (inverse), m_expression_string (expression)
  {
    m_eval.init ();

    //  compile the expression
    tl::Extractor ex (m_expression_string.c_str ());
    m_eval.parse (m_expression, ex);
  }

  virtual bool selected (const shape_type &shape, db::properties_id_type prop_id) const
  {
    m_eval.set_prop_id (prop_id);
    m_eval.set_shape (&shape);

    bool res = m_expression.execute ().to_bool ();
    return m_inverse ? !res : res;
  }

  //  only needed for PolygonFilterBase
  virtual bool selected (const db::PolygonRef &shape, db::properties_id_type prop_id) const
  {
    m_eval.set_prop_id (prop_id);
    m_eval.set_shape (&shape);

    bool res = m_expression.execute ().to_bool ();
    return m_inverse ? !res : res;
  }

public:
  MeasureEval m_eval;
  tl::Expression m_expression;
  bool m_inverse;
  std::string m_expression_string;
};

}

#endif
