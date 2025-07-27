
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

#include "dbMeasure.h"
#include "dbRegion.h"
#include "dbEdges.h"
#include "dbEdgePairs.h"
#include "dbTexts.h"
#include "tlExpression.h"
#include "gsiClassBase.h"
#include "gsiDeclDbContainerHelpers.h"

namespace db
{

// -------------------------------------------------------------------------------------
//  Some utilities

namespace
{

/**
 *  @brief A class collecting the properties names from the shapes delivered by a RecursiveShapeIterator
 *
 *  This class implements the "RecursiveShapeReceiver" interface and will collect all property names
 *  present in the shapes delivered by a RecursiveShapeIterator. Use this class as a target for
 *  the RecursiveShapeIterator's "push" method. After this, "names" will give you a set with the
 *  property names found.
 */
class PropertyNamesCollector
  : public db::RecursiveShapeReceiver
{
public:
  PropertyNamesCollector ()
    : m_names (), m_name_ids ()
  {
    //  .. nothing yet ..
  }

  const std::set<tl::Variant> &names () const
  {
    return m_names;
  }

  virtual void enter_cell (const db::RecursiveShapeIterator * /*iter*/, const db::Cell *cell, const db::Box & /*region*/, const box_tree_type * /*complex_region*/)
  {
    m_cell_ids.insert (cell->cell_index ());
  }

  virtual new_inst_mode new_inst (const db::RecursiveShapeIterator * /*iter*/, const db::CellInstArray &inst, const db::ICplxTrans & /*always_apply*/, const db::Box & /*region*/, const box_tree_type * /*complex_region*/, bool /*all*/, bool /*skip_shapes*/)
  {
    if (m_cell_ids.find (inst.object ().cell_index ()) != m_cell_ids.end ()) {
      return NI_skip;
    } else {
      return NI_single;
    }
  }

  virtual void shape (const db::RecursiveShapeIterator * /*iter*/, const db::Shape &shape, const db::ICplxTrans & /*always_apply*/, const db::ICplxTrans & /*trans*/, const db::Box & /*region*/, const box_tree_type * /*complex_region*/)
  {
    auto pid = shape.prop_id ();
    if (pid != 0 && m_pids.find (pid) == m_pids.end ()) {
      m_pids.insert (pid);
      const db::PropertiesSet &ps = db::properties (pid);
      for (auto i = ps.begin (); i != ps.end (); ++i) {
        if (m_name_ids.find (i->first) == m_name_ids.end ()) {
          m_name_ids.insert (i->first);
          m_names.insert (db::property_name (i->first));
        }
      }
    }
  }

private:
  std::set<tl::Variant> m_names;
  std::unordered_set<db::property_names_id_type> m_name_ids;
  std::unordered_set<db::properties_id_type> m_pids;
  std::set<db::cell_index_type> m_cell_ids;
};

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

  void init (const std::set<tl::Variant> &names)
  {
    define_function ("shape", new ShapesFunction (this));
    define_function ("value", new ValueFunction (this));
    define_function ("values", new ValuesFunction (this));

    for (auto n = names.begin (); n != names.end (); ++n) {
      if (n->is_a_string ()) {
        //  TODO: should check, if the name is a word
        define_function (n->to_string (), new PropertyFunction (this, n->to_string ()));
      }
    }
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

static db::RecursiveShapeIterator
begin_iter (db::Region *region)
{
  return region->merged_semantics () ? region->begin_merged_iter ().first : region->begin_iter ().first;
}

static bool
is_merged (db::Region *region)
{
  return region->merged_semantics ();
}

static db::RecursiveShapeIterator
begin_iter (db::Edges *edges)
{
  return edges->merged_semantics () ? edges->begin_merged_iter ().first : edges->begin_iter ().first;
}

static bool
is_merged (db::Edges *edges)
{
  return edges->merged_semantics ();
}

static db::RecursiveShapeIterator
begin_iter (db::EdgePairs *edge_pairs)
{
  return edge_pairs->begin_iter ().first;
}

static bool
is_merged (db::EdgePairs *)
{
  return false;
}

static db::RecursiveShapeIterator
begin_iter (db::Texts *texts)
{
  return texts->begin_iter ().first;
}

static bool
is_merged (db::Texts *)
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

  property_computation_processor (Container *container, const std::map<tl::Variant, std::string> &expressions, bool copy_properties)
    : m_eval (), m_copy_properties (copy_properties)
  {
    PropertyNamesCollector names_collector;
    if (container) {

      db::RecursiveShapeIterator iter = begin_iter (container);
      iter.push (&names_collector);

      this->set_result_is_merged (is_merged (container));

    }

    m_eval.init (names_collector.names ());

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

}

// -------------------------------------------------------------------------------------

/**
 *  @brief Provides methods to handle measurement functions on various containers
 */
template <class Container, class ProcessorBase>
struct measure_methods
{
  /**
   *  @brief Computes one or many properties from expressions
   *
   *  This method will use the shapes from the "input" container and compute properties from them using the
   *  given expression from "expressions". This map specifies the name of the target property, the value
   *  specifies the expression to execute.
   *
   *  The expressions can make use of the following variables and functions:
   *  * "shape": the shape which is currently seen
   *  * "<prop-name>": an existing property from the shape currently seen (or nil, if no such property is present).
   *    This is a shortcut, only for properties with string names that are compatible with variable names
   *  * "value(<name>)": the value of the property with the given name - if multiple properties with that
   *    name are present, one value is returned
   *  * "values(<name>)": a list of values for all properties with the given name
   *
   *  Returns the new container with the computed properties attached.
   */
  Container computed_properties (Container *input, const std::map<tl::Variant, std::string> &expressions, bool clear_properties);

  /**
   *  @brief Computes one or many properties from expressions
   *
   *  Like "computed_properties", this method computes properties, but attaches them to the existing shapes.
   *  As a side effect, the shapes may be merged if "merged_semantics" applies. If "clear_properties" is true,
   *  any existing properties will be removed. If not, the new properties are added to the existing ones.
   */
  void compute_properties_in_place (Container *container, const std::map<tl::Variant, std::string> &expressions, bool clear_properties);

  /**
   *  @brief Selects all shapes for which the condition expression renders true (or the inverse)
   *
   *  The condition expression can use the features as described for "computed_properties".
   *  If inverse is false, all shapes are selected for which the condition renders true. If
   *  inverse is true, all shapes are selected for which the condition renders false.
   */
  Container selected_if (const Container &container, const std::string &condition_expression, bool inverse);

  /**
   *  @brief In-place version of "selected_if"
   */
  void select_if (const Container &container, const std::string &condition_expression, bool inverse);

  /**
   *  @brief Splits the container into one for which is the condition is true and one with the other shapes
   */
  std::pair<Container, Container> split_if (const Container &container, const std::string &condition_expression);
};

template <class Container, class ProcessorBase>
Container
measure_methods<Container, ProcessorBase>::computed_properties (Container *container, const std::map<tl::Variant, std::string> &expressions, bool clear_properties)
{
  property_computation_processor<ProcessorBase, Container> proc (container, expressions, !clear_properties);
  return container->processed (proc);
}

template <class Container, class ProcessorBase>
void
measure_methods<Container, ProcessorBase>::compute_properties_in_place (Container *container, const std::map<tl::Variant, std::string> &expressions, bool clear_properties)
{
  property_computation_processor<ProcessorBase, Container> proc (container, expressions, !clear_properties);
  container->process (proc);
}

template <class Container, class ProcessorBase>
Container
measure_methods<Container, ProcessorBase>::selected_if (const Container &container, const std::string &condition_expression, bool inverse)
{
  //  - collect property names
  //  - define tl::Eval functions for property names (properties_id -> value)
  //  - define tl::Eval function for shape
  //  - compile expression
  //  - launch filter (Region::filtered)
}

template <class Container, class ProcessorBase>
void
measure_methods<Container, ProcessorBase>::select_if (const Container &container, const std::string &condition_expression, bool inverse)
{
  //  - collect property names
  //  - define tl::Eval functions for property names (properties_id -> value)
  //  - define tl::Eval function for shape
  //  - compile expression
  //  - launch filter (Region::filtered)
}

template <class Container, class ProcessorBase>
std::pair<Container, Container>
measure_methods<Container, ProcessorBase>::split_if (const Container &container, const std::string &condition_expression)
{
  //  - collect property names
  //  - define tl::Eval functions for property names (properties_id -> value)
  //  - define tl::Eval function for shape
  //  - compile expression
  //  - launch filter (Region::filtered)
}

//  explicit instantiations
template struct measure_methods<db::Region, db::shape_collection_processor<db::Polygon, db::Polygon> >;
template struct measure_methods<db::Edges, db::shape_collection_processor<db::Edge, db::Edge> >;
template struct measure_methods<db::EdgePairs, db::shape_collection_processor<db::EdgePair, db::EdgePair> >;
template struct measure_methods<db::Texts, db::shape_collection_processor<db::Text, db::Text> >;

}
