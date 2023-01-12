
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

#include "dbNetTracerIO.h"
#include "dbTechnology.h"
#include "tlClassRegistry.h"

namespace db
{

std::string net_tracer_component_name ()
{
  return std::string ("connectivity");
}

// -----------------------------------------------------------------------------------------
//  NetTracerLayerExpressionInfo implementation

NetTracerLayerExpressionInfo::NetTracerLayerExpressionInfo ()
  : mp_a (0), mp_b (0), m_op (NetTracerLayerExpression::OPNone)
{
  //  .. nothing yet ..
}

NetTracerLayerExpressionInfo::~NetTracerLayerExpressionInfo ()
{
  delete mp_a;
  mp_a = 0;
  delete mp_b;
  mp_b = 0;
}

NetTracerLayerExpressionInfo::NetTracerLayerExpressionInfo (const NetTracerLayerExpressionInfo &other)
  : m_expression (other.m_expression), m_a (other.m_a), m_b (other.m_b), mp_a (0), mp_b (0), m_op (other.m_op)
{
  if (other.mp_a) {
    mp_a = new NetTracerLayerExpressionInfo (*other.mp_a);
  }
  if (other.mp_b) {
    mp_b = new NetTracerLayerExpressionInfo (*other.mp_b);
  }
}

NetTracerLayerExpressionInfo &
NetTracerLayerExpressionInfo::operator= (const NetTracerLayerExpressionInfo &other)
{
  if (this != &other) {

    m_expression = other.m_expression;

    delete mp_a;
    mp_a = 0;
    delete mp_b;
    mp_b = 0;

    m_a = other.m_a;
    m_b = other.m_b;
    m_op = other.m_op;

    if (other.mp_a) {
      mp_a = new NetTracerLayerExpressionInfo (*other.mp_a);
    }
    if (other.mp_b) {
      mp_b = new NetTracerLayerExpressionInfo (*other.mp_b);
    }

  }

  return *this;
}

void
NetTracerLayerExpressionInfo::merge (NetTracerLayerExpression::Operator op, const NetTracerLayerExpressionInfo &other)
{
  if (m_op != NetTracerLayerExpression::OPNone) {
    NetTracerLayerExpressionInfo *e = new NetTracerLayerExpressionInfo (*this);
    *this = NetTracerLayerExpressionInfo ();
    mp_a = e;
  }

  m_op = op;

  if (other.m_op == NetTracerLayerExpression::OPNone) {
    if (other.mp_a) {
      mp_b = new NetTracerLayerExpressionInfo (*other.mp_a);
    } else {
      m_b = other.m_a;
    }
  } else {
    mp_b = new NetTracerLayerExpressionInfo (other);
  }
}

NetTracerLayerExpressionInfo
NetTracerLayerExpressionInfo::parse_add (tl::Extractor &ex)
{
  NetTracerLayerExpressionInfo e = parse_mult (ex);
  while (true) {
    if (ex.test ("+")) {
      NetTracerLayerExpressionInfo ee = parse_mult (ex);
      e.merge (NetTracerLayerExpression::OPOr, ee);
    } else if (ex.test ("-")) {
      NetTracerLayerExpressionInfo ee = parse_mult (ex);
      e.merge (NetTracerLayerExpression::OPNot, ee);
    } else {
      break;
    }
  }

  return e;
}

NetTracerLayerExpressionInfo
NetTracerLayerExpressionInfo::parse_mult (tl::Extractor &ex)
{
  NetTracerLayerExpressionInfo e = parse_atomic (ex);
  while (true) {
    if (ex.test ("*")) {
      NetTracerLayerExpressionInfo ee = parse_atomic (ex);
      e.merge (NetTracerLayerExpression::OPAnd, ee);
    } else if (ex.test ("^")) {
      NetTracerLayerExpressionInfo ee = parse_atomic (ex);
      e.merge (NetTracerLayerExpression::OPXor, ee);
    } else {
      break;
    }
  }

  return e;
}

NetTracerLayerExpressionInfo
NetTracerLayerExpressionInfo::parse_atomic (tl::Extractor &ex)
{
  NetTracerLayerExpressionInfo e;
  if (ex.test ("(")) {
    e = parse_add (ex);
    ex.expect (")");
  } else {
    e.m_a.read (ex);
  }
  return e;
}

NetTracerLayerExpressionInfo
NetTracerLayerExpressionInfo::parse (tl::Extractor &ex)
{
  const char *start = ex.skip ();
  NetTracerLayerExpressionInfo e = parse_add (ex);
  e.m_expression = std::string (start, ex.get () - start);
  return e;
}

NetTracerLayerExpressionInfo
NetTracerLayerExpressionInfo::compile (const std::string &s)
{
  tl::Extractor ex (s.c_str ());
  const char *start = ex.skip ();
  NetTracerLayerExpressionInfo e = parse_add (ex);
  e.m_expression = std::string (start, ex.get () - start);
  ex.expect_end ();
  return e;
}

NetTracerLayerExpression *
NetTracerLayerExpressionInfo::get_expr (const db::LayerProperties &lp, const db::Layout &layout, const NetTracerConnectivity &tech, const std::set<std::string> &used_symbols) const
{
  for (NetTracerConnectivity::const_symbol_iterator s = tech.begin_symbols (); s != tech.end_symbols (); ++s) {
    if (s->symbol ().log_equal (lp)) {
      std::set<std::string> us = used_symbols;
      if (! us.insert (s->symbol ().to_string ()).second) {
        throw tl::Exception (tl::to_string (tr ("Recursive expression through symbol %s")), s->symbol ());
      }
      return NetTracerLayerExpressionInfo::compile (s->expression ()).get (layout, tech, us);
    }
  }

  for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
    if ((*l).second->log_equal (lp)) {
      return new NetTracerLayerExpression ((*l).first);
    }
  }

  return new NetTracerLayerExpression (-1);
}

NetTracerLayerExpression *
NetTracerLayerExpressionInfo::get (const db::Layout &layout, const NetTracerConnectivity &tech) const
{
  std::set<std::string> us;
  return get (layout, tech, us);
}

NetTracerLayerExpression *
NetTracerLayerExpressionInfo::get (const db::Layout &layout, const NetTracerConnectivity &tech, const std::set<std::string> &used_symbols) const
{
  NetTracerLayerExpression *e = 0;

  if (mp_a) {
    e = mp_a->get (layout, tech, used_symbols);
  } else {
    e = get_expr (m_a, layout, tech, used_symbols);
  }

  if (m_op != NetTracerLayerExpression::OPNone) {
    if (mp_b) {
      e->merge (m_op, mp_b->get (layout, tech, used_symbols));
    } else {
      e->merge (m_op, get_expr (m_b, layout, tech, used_symbols));
    }
  }

  return e;
}

// -----------------------------------------------------------------------------------
//  NetTracerConnectionInfo implementation

NetTracerConnectionInfo::NetTracerConnectionInfo ()
{
  // .. nothing yet ..
}

NetTracerConnectionInfo::NetTracerConnectionInfo (const NetTracerLayerExpressionInfo &la, const NetTracerLayerExpressionInfo &lb)
  : m_la (la), m_via (), m_lb (lb)
{
  // .. nothing yet ..
}

NetTracerConnectionInfo::NetTracerConnectionInfo (const NetTracerLayerExpressionInfo &la, const NetTracerLayerExpressionInfo &via, const NetTracerLayerExpressionInfo &lb)
  : m_la (la), m_via (via), m_lb (lb)
{
  // .. nothing yet ..
}

static int get_layer_id (const NetTracerLayerExpressionInfo &e, const db::Layout &layout, const NetTracerConnectivity &tech, NetTracerData *data)
{
  std::unique_ptr<NetTracerLayerExpression> expr_in (NetTracerLayerExpressionInfo::compile (e.to_string ()).get (layout, tech));
  int l = expr_in->alias_for ();
  if (l < 0 && data) {
    l = data->find_symbol (e.to_string ());
    if (l < 0) {
      return int (data->register_logical_layer (expr_in.release (), 0));
    }
  }
  return l;
}

NetTracerConnection
NetTracerConnectionInfo::get (const db::Layout &layout, const NetTracerConnectivity &tech, NetTracerData &data) const
{
  int la = get_layer_id (m_la, layout, tech, &data);
  int lb = get_layer_id (m_lb, layout, tech, &data);

  if (! m_via.to_string ().empty ()) {
    int via = get_layer_id (m_via, layout, tech, &data);
    return NetTracerConnection (la, via, lb);
  } else {
    return NetTracerConnection (la, lb);
  }
}

std::string
NetTracerConnectionInfo::to_string () const
{
  std::string res;
  res += m_la.to_string ();
  res += ",";
  res += m_via.to_string ();
  res += ",";
  res += m_lb.to_string ();

  return res;
}

void
NetTracerConnectionInfo::parse (tl::Extractor &ex)
{
  m_la = NetTracerLayerExpressionInfo::parse (ex);
  ex.expect (",");
  m_via = NetTracerLayerExpressionInfo::parse (ex);
  ex.expect (",");
  m_lb = NetTracerLayerExpressionInfo::parse (ex);
}

// -----------------------------------------------------------------------------------
//  NetTracerSymbolInfo implementation

NetTracerSymbolInfo::NetTracerSymbolInfo ()
{
  // .. nothing yet ..
}

NetTracerSymbolInfo::NetTracerSymbolInfo (const db::LayerProperties &symbol, const std::string &expression)
  : m_symbol (symbol), m_expression (expression)
{
  // .. nothing yet ..
}

std::string
NetTracerSymbolInfo::to_string () const
{
  std::string res;
  res += m_symbol.to_string ();
  res += "=";
  res += tl::to_quoted_string(m_expression);

  return res;
}

void
NetTracerSymbolInfo::parse (tl::Extractor &ex)
{
  m_symbol.read (ex);
  ex.expect ("=");
  ex.read_word_or_quoted (m_expression);
}

// -----------------------------------------------------------------------------------
//  Net implementation

NetTracerNet::NetTracerNet ()
  : m_dbu (0.001), m_incomplete (true), m_color (), m_trace_path (false)
{
  //  .. nothing yet ..
}

NetTracerNet::NetTracerNet (const NetTracer &tracer, const db::ICplxTrans &trans, const db::Layout &layout, db::cell_index_type cell_index, const std::string &layout_filename, const std::string &layout_name, const NetTracerData &data)
  : m_name (tracer.name ()), m_incomplete (tracer.incomplete ()), m_color (), m_trace_path (false)
{
  m_dbu = layout.dbu ();
  m_top_cell_name = layout.cell_name (cell_index);
  m_layout_filename = layout_filename;
  m_layout_name = layout_name;

  size_t n = 0;
  for (NetTracer::iterator s = tracer.begin (); s != tracer.end (); ++s) {
    ++n;
  }
  m_net_shapes.reserve (n);

  for (NetTracer::iterator s = tracer.begin (); s != tracer.end (); ++s) {

    //  TODO: should reset property ID:
    tl::ident_map<db::properties_id_type> pm;
    db::Shape new_shape = m_shapes.insert (s->shape (), trans, pm);
    m_net_shapes.push_back (*s);
    m_net_shapes.back ().shape (new_shape);

    if (m_cell_names.find (s->cell_index ()) == m_cell_names.end ()) {
      m_cell_names.insert (std::make_pair (s->cell_index (), layout.cell_name (s->cell_index ())));
    }

    if (m_layers.find (s->layer ()) == m_layers.end ()) {

      unsigned int l = s->layer ();
      db::LayerProperties lp;
      db::LayerProperties lprep;

      if (layout.is_valid_layer (l)) {

        lp = layout.get_properties (l);
        lprep = lp;

      } else {

        int lrep = data.expression (l).representative_layer ();
        if (layout.is_valid_layer (lrep)) {
          lprep = layout.get_properties (lrep);
        }

        for (std::map<std::string, unsigned int>::const_iterator sy = data.symbols ().begin (); sy != data.symbols ().end (); ++sy) {
          if (sy->second == l) {
            tl::Extractor ex (sy->first.c_str ());
            lp.read (ex);
            break;
          }
        }

      }

      define_layer (l, lp, lprep);

    }

  }
}

std::vector<unsigned int>
NetTracerNet::export_net (db::Layout &layout, db::Cell &export_cell)
{
  std::vector<unsigned int> new_layers;
  std::map<unsigned int, unsigned int> layer_map;

  for (iterator net_shape = begin (); net_shape != end (); ++net_shape) {

    if (net_shape->is_pseudo ()) {
      continue;
    }

    std::map<unsigned int, unsigned int>::const_iterator lm = layer_map.find (net_shape->layer ());
    if (lm == layer_map.end ()) {

      int layer_index = -1;
      for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
        if ((*l).second->log_equal (representative_layer_for (net_shape->layer ()))) {
          layer_index = int ((*l).first);
          break;
        }
      }

      if (layer_index < 0) {
        layer_index = int (layout.insert_layer (representative_layer_for (net_shape->layer ())));
        new_layers.push_back (layer_index);
      }

      lm = layer_map.insert (std::make_pair (net_shape->layer (), (unsigned int)layer_index)).first;

    }

    tl::ident_map<db::properties_id_type> pm;
    export_cell.shapes (lm->second).insert (net_shape->shape (), db::ICplxTrans (net_shape->trans ()), pm);

  }

  return new_layers;
}

const std::string &
NetTracerNet::cell_name (db::cell_index_type cell_index) const
{
  std::map <unsigned int, std::string>::const_iterator cn = m_cell_names.find (cell_index);
  if (cn != m_cell_names.end ()) {
    return cn->second;
  } else {
    static std::string n;
    return n;
  }
}

db::LayerProperties
NetTracerNet::representative_layer_for (unsigned int log_layer) const
{
  std::map <unsigned int, std::pair <db::LayerProperties, db::LayerProperties> >::const_iterator l = m_layers.find (log_layer);
  if (l != m_layers.end ()) {
    return l->second.second;
  } else {
    return db::LayerProperties ();
  }
}

db::LayerProperties
NetTracerNet::layer_for (unsigned int log_layer) const
{
  std::map <unsigned int, std::pair <db::LayerProperties, db::LayerProperties> >::const_iterator l = m_layers.find (log_layer);
  if (l != m_layers.end ()) {
    return l->second.first;
  } else {
    return db::LayerProperties ();
  }
}

void
NetTracerNet::define_layer (unsigned int l, const db::LayerProperties &lp, const db::LayerProperties &lp_representative)
{
  m_layers.insert (std::make_pair (l, std::make_pair (lp, lp_representative)));
}

// -----------------------------------------------------------------------------------
//  NetTracerTechnologyComponent implementation

NetTracerTechnologyComponent::NetTracerTechnologyComponent ()
  : db::TechnologyComponent (net_tracer_component_name (), tl::to_string (tr ("Connectivity")))
{
  //  .. nothing yet ..
}

// -----------------------------------------------------------------------------------
//  NetTracerConnectivity implementation

NetTracerConnectivity::NetTracerConnectivity ()
  : m_is_fallback_default (false)
{
  // .. nothing yet ..
}

NetTracerConnectivity::NetTracerConnectivity (const NetTracerConnectivity &d)
{
  operator= (d);
}

NetTracerConnectivity &NetTracerConnectivity::operator= (const NetTracerConnectivity &d)
{
  if (this != &d) {
    m_is_fallback_default = d.m_is_fallback_default;
    m_connections = d.m_connections;
    m_symbols = d.m_symbols;
    m_name = d.m_name;
    m_description = d.m_description;
  }
  return *this;
}

NetTracerData
NetTracerConnectivity::get_tracer_data (const db::Layout &layout) const
{
  //  test run on the expressions to verify their syntax
  int n = 1;
  for (NetTracerConnectivity::const_iterator c = begin (); c != end (); ++c, ++n) {
    if (c->layer_a ().to_string ().empty ()) {
      throw tl::Exception (tl::to_string (tr ("Missing first layer specification on connectivity specification #%d")), n);
    }
    if (c->layer_b ().to_string ().empty ()) {
      throw tl::Exception (tl::to_string (tr ("Missing second layer specification on connectivity specification #%d")), n);
    }
  }

  n = 1;
  for (NetTracerConnectivity::const_symbol_iterator s = begin_symbols (); s != end_symbols (); ++s, ++n) {
    if (s->symbol ().to_string ().empty ()) {
      throw tl::Exception (tl::to_string (tr ("Missing symbol name on symbol specification #%d")), n);
    }
    if (s->expression ().empty ()) {
      throw tl::Exception (tl::to_string (tr ("Missing expression on symbol specification #%d")), n);
    }
    try {
      std::unique_ptr<NetTracerLayerExpression> expr_in (NetTracerLayerExpressionInfo::compile (s->expression ()).get (layout, *this));
    } catch (tl::Exception &ex) {
      throw tl::Exception (tl::to_string (tr ("Error compiling expression '%s' (symbol #%d): %s")), s->expression (), n, ex.msg ());
    }
  }

  NetTracerData data;

  //  register a logical layer for each original one as alias and one for each expression with a new ID
  for (db::NetTracerConnectivity::const_symbol_iterator s = begin_symbols (); s != end_symbols (); ++s) {
    db::NetTracerLayerExpression *expr = db::NetTracerLayerExpressionInfo::compile (s->expression ()).get (layout, *this);
    data.register_logical_layer (expr, s->symbol ().to_string ().c_str ());
  }

  for (db::NetTracerConnectivity::const_iterator c = begin (); c != end (); ++c) {
    data.add_connection (c->get (layout, *this, data));
  }

  return data;
}

}
