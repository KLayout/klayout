
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

#ifndef HDR_gsiDeclDbMeasureHelpers
#define HDR_gsiDeclDbMeasureHelpers

#include "dbMeasureEval.h"
#include "dbRegionUtils.h"
#include "dbEdgesUtils.h"
#include "gsiClassBase.h"
#include "gsiDeclDbContainerHelpers.h"

namespace gsi
{

// -------------------------------------------------------------------------------------
//  Some utilities

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

  property_computation_processor (const Container *container, const std::map<tl::Variant, std::string> &expressions, bool copy_properties, double dbu, const std::map<std::string, tl::Variant> &variables)
    : m_eval (dbu, true /*with_put*/), m_copy_properties (copy_properties), m_expression_strings (expressions)
  {
    if (container) {
      this->set_result_is_merged (is_merged (container));
    }

    m_eval.init ();

    for (auto v = variables.begin (); v != variables.end (); ++v) {
      m_eval.set_var (v->first, v->second);
    }

    //  compile the expressions
    for (auto e = m_expression_strings.begin (); e != m_expression_strings.end (); ++e) {
      m_expressions.push_back (std::make_pair (e->first.is_nil () ? db::property_names_id_type (0) : db::property_names_id (e->first), tl::Expression ()));
      tl::Extractor ex (e->second.c_str ());
      m_eval.parse (m_expressions.back ().second, ex);
    }
  }

  virtual void process (const db::object_with_properties<shape_type> &shape, std::vector<db::object_with_properties<shape_type> > &res) const
  {
    try {

      m_eval.reset (shape.properties_id ());
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

      if (! m_eval.skip ()) {
        res.push_back (shape);
        res.back ().properties_id (db::properties_id (ps_out));
      }

    } catch (tl::Exception &ex) {
      tl::warn << ex.msg ();
      res.clear ();
    }
  }

public:
  db::MeasureEval m_eval;
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
  : public FilterBase
{
public:
  typedef typename FilterBase::shape_type shape_type;

  expression_filter (const std::string &expression, bool inverse, double dbu, const std::map<std::string, tl::Variant> &variables)
    : m_eval (dbu, false /*without put func*/), m_inverse (inverse), m_expression_string (expression)
  {
    m_eval.init ();

    for (auto v = variables.begin (); v != variables.end (); ++v) {
      m_eval.set_var (v->first, v->second);
    }

    //  compile the expression
    tl::Extractor ex (m_expression_string.c_str ());
    m_eval.parse (m_expression, ex);
  }

  virtual bool selected (const shape_type &shape, db::properties_id_type prop_id) const
  {
    try {

      m_eval.reset (prop_id);
      m_eval.set_shape (&shape);

      bool res = m_expression.execute ().to_bool ();
      return m_inverse ? !res : res;

    } catch (tl::Exception &ex) {
      tl::warn << ex.msg ();
      return false;
    }
  }

  //  only needed for PolygonFilterBase
  virtual bool selected (const db::PolygonRef &shape, db::properties_id_type prop_id) const
  {
    try {

      m_eval.reset (prop_id);
      m_eval.set_shape (&shape);

      bool res = m_expression.execute ().to_bool ();
      return m_inverse ? !res : res;

    } catch (tl::Exception &ex) {
      tl::warn << ex.msg ();
      return false;
    }
  }

public:
  db::MeasureEval m_eval;
  tl::Expression m_expression;
  bool m_inverse;
  std::string m_expression_string;
};

}

#endif
