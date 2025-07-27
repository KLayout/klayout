
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
class MeasureEval
  : public tl::Eval
{
public:
  MeasureEval ()
    : m_shape_type (None), m_prop_id (0)
  {
    mp_shape.any = 0;
  }

  void init ()
  {
    define_function ("shape", new ShapesFunction (this));
    define_function ("value", new ValueFunction (this));
    define_function ("values", new ValuesFunction (this));
  }

  void reset_shape () const
  {
    m_shape_type = None;
    mp_shape.any = 0;
    m_prop_id = 0;
  }

  void set_shape (const db::Polygon *poly) const
  {
    m_shape_type = Polygon;
    mp_shape.poly = poly;
  }

  void set_shape (const db::PolygonRef *poly) const
  {
    m_shape_type = PolygonRef;
    mp_shape.poly_ref = poly;
  }

  void set_shape (const db::Edge *edge) const
  {
    m_shape_type = Edge;
    mp_shape.edge = edge;
  }

  void set_shape (const db::EdgePair *edge_pair) const
  {
    m_shape_type = EdgePair;
    mp_shape.edge_pair = edge_pair;
  }

  void set_shape (const db::Text *text) const
  {
    m_shape_type = Text;
    mp_shape.text = text;
  }

  void set_prop_id (db::properties_id_type prop_id) const
  {
    m_prop_id = prop_id;
  }

protected:
  virtual void resolve_name (const std::string &name, const tl::EvalFunction *&function, const tl::Variant *&value, tl::Variant *&var)
  {
    tl::Eval::resolve_name (name, function, value, var);

    if (!function && !value && !var) {
      //  connect the name with a function getting the property value
      tl::EvalFunction *f = new PropertyFunction (this, name);
      define_function (name, f);
      function = f;
    }
  }

private:
  class ShapesFunction
    : public tl::EvalFunction
  {
  public:
    ShapesFunction (MeasureEval *eval)
      : mp_eval (eval)
    {
      //  .. nothing yet ..
    }

    virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
    {
      if (args.size () != 0) {
        throw tl::EvalError (tl::to_string (tr ("'shape' function does not take arguments")), context);
      }
      out = mp_eval->shape_func ();
    }

  private:
    MeasureEval *mp_eval;
  };

  class ValueFunction
    : public tl::EvalFunction
  {
  public:
    ValueFunction (MeasureEval *eval)
      : mp_eval (eval)
    {
      //  .. nothing yet ..
    }

    virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
    {
      if (args.size () != 1) {
        throw tl::EvalError (tl::to_string (tr ("'value' function takes one argument")), context);
      }
      out = mp_eval->value_func (args [0]);
    }

  private:
    MeasureEval *mp_eval;
  };

  class ValuesFunction
    : public tl::EvalFunction
  {
  public:
    ValuesFunction (MeasureEval *eval)
      : mp_eval (eval)
    {
      //  .. nothing yet ..
    }

    virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
    {
      if (args.size () != 1) {
        throw tl::EvalError (tl::to_string (tr ("'values' function takes one argument")), context);
      }
      out = mp_eval->values_func (args [0]);
    }

  private:
    MeasureEval *mp_eval;
  };

  class PropertyFunction
    : public tl::EvalFunction
  {
  public:
    PropertyFunction (MeasureEval *eval, const tl::Variant &name)
      : mp_eval (eval), m_name (name)
    {
      //  .. nothing yet ..
    }

    virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
    {
      if (args.size () != 0) {
        throw tl::EvalError (tl::to_string (tr ("Property getter function does not take arguments")), context);
      }
      out = mp_eval->value_func (m_name);
    }

  private:
    MeasureEval *mp_eval;
    tl::Variant m_name;
  };

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

  tl::Variant shape_func () const
  {
    switch (m_shape_type)
    {
    case None:
    default:
      return tl::Variant ();
    case Polygon:
      return tl::Variant (mp_shape.poly);
    case PolygonRef:
      return tl::Variant (mp_shape.poly_ref);
    case Edge:
      return tl::Variant (mp_shape.edge);
    case EdgePair:
      return tl::Variant (mp_shape.edge_pair);
    case Text:
      return tl::Variant (mp_shape.text);
    }
  }

  tl::Variant value_func (const tl::Variant &name) const
  {
    const db::PropertiesSet &ps = db::properties (m_prop_id);
    for (auto i = ps.begin (); i != ps.end (); ++i) {
      if (db::property_name (i->first) == name) {
        return db::property_value (i->second);
      }
    }

    return tl::Variant ();
  }

  tl::Variant values_func (const tl::Variant &name) const
  {
    tl::Variant res = tl::Variant::empty_list ();

    const db::PropertiesSet &ps = db::properties (m_prop_id);
    for (auto i = ps.begin (); i != ps.end (); ++i) {
      if (db::property_name (i->first) == name) {
        res.push (db::property_value (i->second));
      }
    }

    return res;
  }
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

  property_computation_processor (const Container *container, const std::map<tl::Variant, std::string> &expressions, bool copy_properties)
    : m_eval (), m_copy_properties (copy_properties)
  {
    if (container) {
      this->set_result_is_merged (is_merged (container));
    }

    m_eval.init ();

    //  compile the expressions
    for (auto e = expressions.begin (); e != expressions.end (); ++e) {
      m_expressions.push_back (std::make_pair (db::property_names_id (e->first), tl::Expression ()));
      tl::Extractor ex (e->second.c_str ());
      m_eval.parse (m_expressions.back ().second, ex);
    }
  }

  virtual void process (const db::object_with_properties<shape_type> &shape, std::vector<db::object_with_properties<shape_type> > &res) const
  {
    res.push_back (shape);

    m_eval.set_prop_id (shape.properties_id ());
    m_eval.set_shape (&shape);

    db::PropertiesSet ps;
    if (m_copy_properties) {
      ps = db::properties (shape.properties_id ());
    }

    for (auto e = m_expressions.begin (); e != m_expressions.end (); ++e) {
      ps.insert (e->first, e->second.execute ());
    }

    res.back ().properties_id (db::properties_id (ps));
  }

public:
  MeasureEval m_eval;
  std::vector<std::pair<db::property_names_id_type, tl::Expression> > m_expressions;
  bool m_copy_properties;
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

  expression_filter (const std::string &expression, bool inverse)
    : m_eval (), m_inverse (inverse)
  {
    m_eval.init ();

    //  compile the expression
    tl::Extractor ex (expression.c_str ());
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
};

}

#endif
