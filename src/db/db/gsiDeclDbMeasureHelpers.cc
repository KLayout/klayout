
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

#include "gsiDeclDbMeasureHelpers.h"

namespace gsi
{

class ShapeFunction
  : public tl::EvalFunction
{
public:
  ShapeFunction (MeasureEval *eval)
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
    : mp_eval (eval), m_name_id (db::property_names_id (name))
  {
    //  .. nothing yet ..
  }

  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
  {
    if (args.size () != 0) {
      throw tl::EvalError (tl::to_string (tr ("Property getter function does not take arguments")), context);
    }
    out = mp_eval->value_func (m_name_id);
  }

private:
  MeasureEval *mp_eval;
  db::property_names_id_type m_name_id;
};

// --------------------------------------------------------------------
//  MeasureEval implementation

MeasureEval::MeasureEval (double dbu)
  : m_shape_type (None), m_prop_id (0), m_dbu (dbu)
{
  mp_shape.any = 0;
}

void
MeasureEval::init ()
{
  define_function ("shape", new ShapeFunction (this));
  define_function ("value", new ValueFunction (this));
  define_function ("values", new ValuesFunction (this));
}

void
MeasureEval::reset_shape () const
{
  m_shape_type = None;
  mp_shape.any = 0;
  m_prop_id = 0;
}

void
MeasureEval::set_shape (const db::Polygon *poly) const
{
  m_shape_type = Polygon;
  mp_shape.poly = poly;
}

void
MeasureEval::set_shape (const db::PolygonRef *poly) const
{
  m_shape_type = PolygonRef;
  mp_shape.poly_ref = poly;
}

void
MeasureEval::set_shape (const db::Edge *edge) const
{
  m_shape_type = Edge;
  mp_shape.edge = edge;
}

void
MeasureEval::set_shape (const db::EdgePair *edge_pair) const
{
  m_shape_type = EdgePair;
  mp_shape.edge_pair = edge_pair;
}

void
MeasureEval::set_shape (const db::Text *text) const
{
  m_shape_type = Text;
  mp_shape.text = text;
}

void
MeasureEval::set_prop_id (db::properties_id_type prop_id) const
{
  m_prop_id = prop_id;
}

void
MeasureEval::resolve_name (const std::string &name, const tl::EvalFunction *&function, const tl::Variant *&value, tl::Variant *&var)
{
  tl::Eval::resolve_name (name, function, value, var);

  if (!function && !value && !var) {
    //  connect the name with a function getting the property value
    tl::EvalFunction *f = new PropertyFunction (this, name);
    define_function (name, f);
    function = f;
  }
}

tl::Variant
MeasureEval::shape_func () const
{
  if (m_dbu > 1e-10) {

    db::CplxTrans tr (m_dbu);

    switch (m_shape_type)
    {
    case None:
    default:
      return tl::Variant ();
    case Polygon:
      return tl::Variant (tr * *mp_shape.poly);
    case PolygonRef:
      {
        db::Polygon poly;
        mp_shape.poly_ref->instantiate (poly);
        return tl::Variant (tr * poly);
      }
    case Edge:
      return tl::Variant (tr * *mp_shape.edge);
    case EdgePair:
      return tl::Variant (tr * *mp_shape.edge_pair);
    case Text:
      return tl::Variant (tr * *mp_shape.text);
    }

  } else {

    switch (m_shape_type)
    {
    case None:
    default:
      return tl::Variant ();
    case Polygon:
      return tl::Variant (*mp_shape.poly);
    case PolygonRef:
      {
        db::Polygon poly;
        mp_shape.poly_ref->instantiate (poly);
        return tl::Variant (poly);
      }
    case Edge:
      return tl::Variant (*mp_shape.edge);
    case EdgePair:
      return tl::Variant (*mp_shape.edge_pair);
    case Text:
      return tl::Variant (*mp_shape.text);
    }

  }
}

tl::Variant
MeasureEval::value_func (db::property_names_id_type name_id) const
{
  const db::PropertiesSet &ps = db::properties (m_prop_id);
  for (auto i = ps.begin (); i != ps.end (); ++i) {
    if (i->first == name_id) {
      return db::property_value (i->second);
    }
  }

  return tl::Variant ();
}

tl::Variant
MeasureEval::value_func (const tl::Variant &name) const
{
  const db::PropertiesSet &ps = db::properties (m_prop_id);
  for (auto i = ps.begin (); i != ps.end (); ++i) {
    if (db::property_name (i->first) == name) {
      return db::property_value (i->second);
    }
  }

  return tl::Variant ();
}

tl::Variant
MeasureEval::values_func (const tl::Variant &name) const
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

}
