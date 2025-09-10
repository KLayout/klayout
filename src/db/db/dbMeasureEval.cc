
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

#include "dbMeasureEval.h"
#include "gsiClass.h"

namespace db
{

// -----------------------------------------------------------------------------
//  MeasureEval implementation

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

class SkipFunction
  : public tl::EvalFunction
{
public:
  SkipFunction (MeasureEval *eval)
    : mp_eval (eval)
  {
    //  .. nothing yet ..
  }

  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant & /*out*/, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
  {
    if (args.size () != 1) {
      throw tl::EvalError (tl::to_string (tr ("'skip' function takes one argument (flag)")), context);
    }
    mp_eval->skip_func (args [0].to_bool ());
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

class PutFunction
  : public tl::EvalFunction
{
public:
  PutFunction (MeasureEval *eval)
    : mp_eval (eval)
  {
    //  .. nothing yet ..
  }

  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant & /*out*/, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
  {
    if (args.size () != 2) {
      throw tl::EvalError (tl::to_string (tr ("'put' function takes two arguments (name, value)")), context);
    }
    mp_eval->put_func (args [0], args [1]);
  }

private:
  MeasureEval *mp_eval;
};

// --------------------------------------------------------------------
//  MeasureEval implementation

MeasureEval::MeasureEval (double dbu, bool with_put)
  : m_shape_type (None), m_prop_id (0), m_skip (false), m_dbu (dbu), m_with_put (with_put)
{
  mp_shape.any = 0;
}

void
MeasureEval::init ()
{
  if (m_with_put) {
    define_function ("put", new PutFunction (this));
    define_function ("skip", new SkipFunction (this));
  }

  define_function ("shape", new ShapeFunction (this));
  define_function ("value", new ValueFunction (this));
  define_function ("values", new ValuesFunction (this));
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
MeasureEval::reset (db::properties_id_type prop_id) const
{
  m_prop_id = prop_id;
  m_skip = false;
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

void
MeasureEval::skip_func (bool f) const
{
  m_skip = f;
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

void
MeasureEval::put_func (const tl::Variant &name, const tl::Variant &value) const
{
  auto prop_name_id = db::property_names_id (name);
  m_prop_set_out.erase (prop_name_id);
  m_prop_set_out.insert (prop_name_id, value);
}

// -----------------------------------------------------------------------------
//  MeasureNetEval implementation

class NetPutFunction
  : public tl::EvalFunction
{
public:
  NetPutFunction (MeasureNetEval *eval)
    : mp_eval (eval)
  {
    //  .. nothing yet ..
  }

  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant & /*out*/, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
  {
    if (args.size () != 2) {
      throw tl::EvalError (tl::to_string (tr ("'put' function takes two arguments (name, value)")), context);
    }
    mp_eval->put_func (args [0], args [1]);
  }

private:
  MeasureNetEval *mp_eval;
};

class NetSkipFunction
  : public tl::EvalFunction
{
public:
  NetSkipFunction (MeasureNetEval *eval)
    : mp_eval (eval)
  {
    //  .. nothing yet ..
  }

  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant & /*out*/, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
  {
    if (args.size () != 1) {
      throw tl::EvalError (tl::to_string (tr ("'skip' function takes one argument (flag)")), context);
    }
    mp_eval->skip_func (args [0].to_bool ());
  }

private:
  MeasureNetEval *mp_eval;
};

class NetFunction
  : public tl::EvalFunction
{
public:
  NetFunction (MeasureNetEval *eval)
    : mp_eval (eval)
  {
    //  .. nothing yet ..
  }

  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
  {
    if (args.size () != 0) {
      throw tl::EvalError (tl::to_string (tr ("'net' function does not take any argument")), context);
    }
    out = mp_eval->net_func ();
  }

private:
  MeasureNetEval *mp_eval;
};

class NetAreaFunction
  : public tl::EvalFunction
{
public:
  NetAreaFunction (MeasureNetEval *eval)
    : mp_eval (eval)
  {
    //  .. nothing yet ..
  }

  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
  {
    if (args.size () > 1) {
      throw tl::EvalError (tl::to_string (tr ("'area' function takes one optional argument (layer symbol)")), context);
    }
    out = mp_eval->area_func (args.size () == 0 ? 0 : args [0].to_int ());
  }

private:
  MeasureNetEval *mp_eval;
};

class NetPerimeterFunction
  : public tl::EvalFunction
{
public:
  NetPerimeterFunction (MeasureNetEval *eval)
    : mp_eval (eval)
  {
    //  .. nothing yet ..
  }

  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
  {
    if (args.size () > 1) {
      throw tl::EvalError (tl::to_string (tr ("'perimeter' function takes one optional argument (layer symbol)")), context);
    }
    out = mp_eval->perimeter_func (args.size () == 0 ? 0 : args [0].to_int ());
  }

private:
  MeasureNetEval *mp_eval;
};

MeasureNetEval::MeasureNetEval (const db::LayoutToNetlist *l2n, double dbu)
  : tl::Eval (), mp_l2n (l2n), m_dbu (dbu)
{
  //  .. nothing yet ..
}

void
MeasureNetEval::set_primary_layer (unsigned int layer_index)
{
  tl_assert (m_layers.empty ());
  m_layers.push_back (layer_index);
}

void
MeasureNetEval::set_secondary_layer (const std::string &name, unsigned int layer_index)
{
  set_var (name, tl::Variant (int (m_layers.size ())));
  m_layers.push_back (layer_index);
}

void
MeasureNetEval::init ()
{
  define_function ("put", new NetPutFunction (this));
  define_function ("skip", new NetSkipFunction (this));
  define_function ("area", new NetAreaFunction (this));
  define_function ("perimeter", new NetPerimeterFunction (this));
  define_function ("net", new NetFunction (this));
}

void
MeasureNetEval::reset (db::cell_index_type cell_index, size_t cluster_id) const
{
  m_skip = false;
  m_cell_index = cell_index;
  m_cluster_id = cluster_id;
  m_area_and_perimeter_cache.clear ();
}

void
MeasureNetEval::put_func (const tl::Variant &name, const tl::Variant &value) const
{
  auto prop_name_id = db::property_names_id (name);
  m_prop_set_out.erase (prop_name_id);
  m_prop_set_out.insert (prop_name_id, value);
}

MeasureNetEval::AreaAndPerimeter
MeasureNetEval::compute_area_and_perimeter (int layer_index) const
{
  if (layer_index < 0 || layer_index >= (int) m_layers.size ()) {
    return AreaAndPerimeter ();
  }

  unsigned int layer = m_layers [layer_index];
  db::Polygon::area_type area = 0;
  db::Polygon::perimeter_type perimeter = 0;
  mp_l2n->compute_area_and_perimeter_of_net_shapes (m_cell_index, m_cluster_id, layer, area, perimeter);

  AreaAndPerimeter ap;
  if (m_dbu > 0.0) {
    ap.area = m_dbu * m_dbu * area;
    ap.perimeter = m_dbu * perimeter;
  } else {
    ap.area = area;
    ap.perimeter = perimeter;
  }

  return ap;
}

tl::Variant
MeasureNetEval::area_func (int layer_index) const
{
  auto ap = m_area_and_perimeter_cache.find (layer_index);
  if (ap == m_area_and_perimeter_cache.end ()) {
    ap = m_area_and_perimeter_cache.insert (std::make_pair (layer_index, compute_area_and_perimeter (layer_index))).first;
  }
  return ap->second.area;
}

tl::Variant
MeasureNetEval::perimeter_func (int layer_index) const
{
  auto ap = m_area_and_perimeter_cache.find (layer_index);
  if (ap == m_area_and_perimeter_cache.end ()) {
    ap = m_area_and_perimeter_cache.insert (std::make_pair (layer_index, compute_area_and_perimeter (layer_index))).first;
  }
  return ap->second.perimeter;
}

void
MeasureNetEval::skip_func (bool f) const
{
  m_skip = f;
}

tl::Variant
MeasureNetEval::net_func () const
{
  const db::Netlist *nl = mp_l2n->netlist ();
  if (! nl) {
    return tl::Variant ();
  }

  //  build a lookup table of nets vs. cell_index+cluster_id
  if (! m_nets_per_cell_and_cluster_id.get ()) {
    m_nets_per_cell_and_cluster_id.reset (new std::map<std::pair<db::cell_index_type, size_t>, const db::Net *> ());
    for (auto c = nl->begin_circuits (); c != nl->end_circuits (); ++c) {
      auto ci = c->cell_index ();
      for (auto n = c->begin_nets (); n != c->end_nets (); ++n) {
        auto cid = n->cluster_id ();
        m_nets_per_cell_and_cluster_id->insert (std::make_pair (std::make_pair (ci, cid), n.operator-> ()));
      }
    }
  }

  auto n = m_nets_per_cell_and_cluster_id->find (std::make_pair (m_cell_index, m_cluster_id));
  if (n != m_nets_per_cell_and_cluster_id->end ()) {
    return tl::Variant::make_variant_ref (n->second);
  } else {
    return tl::Variant ();
  }
}

}
