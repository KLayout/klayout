
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


#include "dbNetTracer.h"

#include "dbRecursiveShapeIterator.h"
#include "dbPolygonTools.h"
#include "dbShapeProcessor.h"
#include "dbLayoutToNetlist.h"
#include "tlLog.h"

//  -O3 appears not to work properly for gcc 4.4.7 (RHEL 6)
//  In that case, the net tracer function crashes.
#if defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR == 4 && defined(__OPTIMIZE__)
#  pragma GCC optimize("O2")
#endif

namespace db
{

// -----------------------------------------------------------------------------------
//  Two helper functions that help determining interactions

static bool
interacts (const db::Box &box, const NetTracerShape &net_shape)
{
  if (net_shape.shape ().is_text ()) {

    return box.touches (net_shape.bbox ());

  } else if (net_shape.shape ().is_box ()) {

    if (net_shape.trans ().is_ortho ()) {

      return box.touches (net_shape.bbox ());

    } else {

      db::Polygon box_poly (net_shape.shape ().box ());
      box_poly.transform (db::ICplxTrans (net_shape.trans ()));

      return (db::interact (box_poly, box));

    }

  } else if (net_shape.shape ().is_polygon () || net_shape.shape ().is_path ()) {

    db::Polygon polygon;
    net_shape.shape ().polygon (polygon);
    polygon.transform (db::ICplxTrans (net_shape.trans ()));

    return db::interact (polygon, box);

  } else {

    return false;

  }
}

static bool
interacts (const db::Polygon &polygon, const NetTracerShape &net_shape)
{
  if (net_shape.shape ().is_text ()) {

    return db::interact (polygon, net_shape.bbox ());

  } else if (net_shape.shape ().is_box ()) {

    if (net_shape.trans ().is_ortho ()) {

      return db::interact (polygon, net_shape.bbox ());

    } else {

      db::Polygon box_poly (net_shape.shape ().box ());
      box_poly.transform (db::ICplxTrans (net_shape.trans ()));

      return db::interact (polygon, box_poly);

    }

  } else if (net_shape.shape ().is_polygon () || net_shape.shape ().is_path ()) {

    db::Polygon p;
    net_shape.shape ().polygon (p);
    p.transform (db::ICplxTrans (net_shape.trans ()));

    return db::interact (p, polygon);

  } else {

    return false;

  }
}

// -----------------------------------------------------------------------------------
//  NetTracerShapeHeap implementation

NetTracerShapeHeap::NetTracerShapeHeap ()
{
  //  .. nothing yet ..
}

db::Shape 
NetTracerShapeHeap::insert (const db::Polygon &p)
{
  std::map<db::Polygon, db::Shape>::const_iterator c = m_cache.find (p);
  if (c == m_cache.end ()) {
    c = m_cache.insert (std::make_pair (p, m_container.insert (p))).first;
  }
  return c->second;
}

void
NetTracerShapeHeap::clear ()
{
  m_container.clear ();
  m_cache.clear ();
}

// -----------------------------------------------------------------------------------
//  NetTracerData implementation

NetTracerData::NetTracerData ()
  : m_next_log_layer (1000000000)
{
  // .. nothing yet ..
}

NetTracerData::NetTracerData (const NetTracerData &other)
  : m_next_log_layer (0)
{
  operator= (other);
}

NetTracerData &
NetTracerData::operator= (const NetTracerData &other)
{
  if (this != &other) {

    for (std::map <unsigned int, NetTracerLayerExpression *>::const_iterator l = m_log_layers.begin (); l != m_log_layers.end (); ++l) {
      delete l->second;
    }
    m_log_layers.clear ();

    for (std::map <unsigned int, NetTracerLayerExpression *>::const_iterator l = other.m_log_layers.begin (); l != other.m_log_layers.end (); ++l) {
      m_log_layers.insert (std::make_pair (l->first, new NetTracerLayerExpression (*l->second)));
    }

    m_next_log_layer = other.m_next_log_layer;
    m_connections = other.m_connections;
    m_original_layers = other.m_original_layers;
    m_connection_graph = other.m_connection_graph;
    m_log_connection_graph = other.m_log_connection_graph;
    m_requires_booleans = other.m_requires_booleans;
    m_symbols = other.m_symbols;

  }

  return *this;
}

NetTracerData::~NetTracerData ()
{
  for (std::map <unsigned int, NetTracerLayerExpression *>::const_iterator l = m_log_layers.begin (); l != m_log_layers.end (); ++l) {
    delete l->second;
  }
  m_log_layers.clear ();

  clean_l2n_regions ();
}

void 
NetTracerData::add_connection (const NetTracerConnection &connection)
{
  if (connection.layer_a () >= 0 && connection.layer_b () >= 0 && (! connection.has_via_layer () || connection.via_layer () >= 0)) {
    m_connections.push_back (connection);
  }
  if (connection.has_via_layer ()) {
    if (connection.layer_a () >= 0) {
      add_layer_pair (connection.layer_a (), connection.via_layer ());
    }
    if (connection.layer_b () >= 0) {
      add_layer_pair (connection.layer_b (), connection.via_layer ());
    }
  } else if (connection.layer_a () >= 0 && connection.layer_b () >= 0) {
    add_layer_pair (connection.layer_a (), connection.layer_b ());
  }
}

void 
NetTracerData::add_layer_pair (unsigned int a, unsigned int b)
{
  add_layers (a, b);
  add_layers (b, a);
}

void 
NetTracerData::add_layers (unsigned int a, unsigned int b)
{
  if (m_log_connection_graph.find (a) == m_log_connection_graph.end ()) {
    m_log_connection_graph.insert (std::make_pair (a, std::set <unsigned int> ())).first->second.insert (a);
  }
  m_log_connection_graph.insert (std::make_pair (a, std::set <unsigned int> ())).first->second.insert (b);

  if (m_connection_graph.find (a) == m_connection_graph.end ()) {
    std::set <unsigned int> aa = expression (a).original_layers ();
    m_connection_graph.insert (std::make_pair (a, std::set <unsigned int> ())).first->second.insert (aa.begin (), aa.end ());
    m_original_layers.insert (std::make_pair (a, aa));
  }
  std::set <unsigned int> bb = expression (b).original_layers ();
  m_connection_graph.insert (std::make_pair (a, std::set <unsigned int> ())).first->second.insert (bb.begin (), bb.end ());
}

const std::set<unsigned int> &
NetTracerData::connections (unsigned int from_layer) const
{
  std::map <unsigned int, std::set <unsigned int> >::const_iterator g = m_connection_graph.find (from_layer);
  if (g != m_connection_graph.end ()) {
    return g->second;
  } else {
    static std::set<unsigned int> empty;
    return empty;
  }
} 

std::set<unsigned int> 
NetTracerData::log_layers_for (unsigned int original_layer) const
{
  std::set <unsigned int> log_layers;
  for (std::map <unsigned int, std::set <unsigned int> >::const_iterator g = m_original_layers.begin (); g != m_original_layers.end (); ++g) {
    if (g->second.find (original_layer) != g->second.end ()) {
      log_layers.insert (g->first);
    }
  }
  return log_layers;
}

const std::set<unsigned int> &
NetTracerData::log_connections (unsigned int from_layer) const
{
  std::map <unsigned int, std::set <unsigned int> >::const_iterator g = m_log_connection_graph.find (from_layer);
  if (g != m_log_connection_graph.end ()) {
    return g->second;
  } else {
    static std::set<unsigned int> empty;
    return empty;
  }
}

int 
NetTracerData::find_symbol (const std::string &symbol) const
{
  std::map<std::string, unsigned int>::const_iterator s = m_symbols.find (symbol);
  if (s == m_symbols.end ()) {
    return -1;
  } else {
    return int (s->second);
  }
}

unsigned int 
NetTracerData::register_logical_layer (NetTracerLayerExpression *expr, const char *symbol)
{
  unsigned int l = ++m_next_log_layer;
  m_log_layers.insert (std::make_pair (l, expr));

  if (symbol) {
    m_symbols.insert (std::make_pair (std::string (symbol), l));
  }

  return l;
}

const NetTracerLayerExpression & 
NetTracerData::expression (unsigned int ll) const
{
  std::map <unsigned int, NetTracerLayerExpression *>::iterator l = m_log_layers.find (ll);
  if (l == m_log_layers.end ()) {
    l = m_log_layers.insert (std::make_pair (ll, new NetTracerLayerExpression (ll))).first;
  }
  return *l->second;
}

/**
 *  @brief Returns the connected original layers split into the ones requiring booleans and the ones which don't
 *  The result pair will contain the ones which do not require booleans in the first element, and the ones which
 *  do in the second.
 */
const std::pair <std::set <unsigned int>, std::set <unsigned int> > &
NetTracerData::requires_booleans (unsigned int from_layer) const
{
  std::map <unsigned int, std::pair <std::set <unsigned int>, std::set <unsigned int> > >::iterator r = m_requires_booleans.find (from_layer);
  if (r == m_requires_booleans.end ()) {

    std::set <unsigned int> layers_without_booleans = connections (from_layer);
    std::set <unsigned int> layers_with_booleans;

    std::set <unsigned int> ll = log_connections (from_layer);
    for (std::set <unsigned int>::const_iterator i = ll.begin (); i != ll.end (); ++i) {
      if (! expression (*i).is_alias ()) {
        std::map <unsigned int, std::set <unsigned int> >::const_iterator ol = m_original_layers.find (*i);
        tl_assert (ol != m_original_layers.end ());
        layers_with_booleans.insert (ol->second.begin (), ol->second.end ());
        for (std::set <unsigned int>::const_iterator j = ol->second.begin (); j != ol->second.end (); ++j) {
          layers_without_booleans.erase (*j);
        }
      }
    }

    r = m_requires_booleans.insert (std::make_pair (from_layer, std::make_pair (layers_without_booleans, layers_with_booleans))).first;

  } 

  return r->second;
}

void
NetTracerData::clean_l2n_regions ()
{
  m_l2n_regions.clear ();
}

void
NetTracerData::configure_l2n (db::LayoutToNetlist &l2n)
{
  clean_l2n_regions ();

  //  take names from symbols
  std::map<unsigned int, std::string> layer_to_symbol;
  for (std::map<std::string, unsigned int>::const_iterator s = m_symbols.begin (); s != m_symbols.end (); ++s) {
    layer_to_symbol.insert (std::make_pair (s->second, s->first));
  }

  std::map <unsigned int, tl::shared_ptr<NetTracerLayerExpression::RegionHolder> > regions_per_org_layer;

  tl::RelativeProgress progress (tl::to_string (tr ("Computing input layers")), m_log_layers.size ());

  //  first fetch all the alias expressions
  for (std::map <unsigned int, NetTracerLayerExpression *>::const_iterator l = m_log_layers.begin (); l != m_log_layers.end (); ++l) {
    if (l->second->is_alias ()) {
      tl::shared_ptr<NetTracerLayerExpression::RegionHolder> rh = l->second->make_l2n_region (l2n, regions_per_org_layer, layer_to_symbol [l->first]);
      m_l2n_regions [l->first] = rh;
      ++progress;
    }
  }

  //  then compute all the symbolic expressions
  for (std::map <unsigned int, NetTracerLayerExpression *>::const_iterator l = m_log_layers.begin (); l != m_log_layers.end (); ++l) {
    if (! l->second->is_alias ()) {
      tl::shared_ptr<NetTracerLayerExpression::RegionHolder> rh = l->second->make_l2n_region (l2n, regions_per_org_layer, layer_to_symbol [l->first]);
      m_l2n_regions [l->first] = rh;
      ++progress;
    }
  }

  //  make all connections (intra and inter-layer)
  for (std::map<unsigned int, tl::shared_ptr<NetTracerLayerExpression::RegionHolder> >::const_iterator r = m_l2n_regions.begin (); r != m_l2n_regions.end (); ++r) {
    l2n.connect (*r->second->get ());
    const std::set<unsigned int> &connections_to = log_connections (r->first);
    for (std::set<unsigned int>::const_iterator c = connections_to.begin (); c != connections_to.end (); ++c) {
      std::map<unsigned int, tl::shared_ptr<NetTracerLayerExpression::RegionHolder> >::const_iterator rc = m_l2n_regions.find (*c);
      if (rc != m_l2n_regions.end ()) {
        l2n.connect (*r->second->get (), *rc->second->get ());
      }
    }
  }
}

// -----------------------------------------------------------------------------------
//  NetTracerLayerExpression implementation

NetTracerLayerExpression::NetTracerLayerExpression ()
  : m_a (0), m_b (0), mp_a (0), mp_b (0), m_op (OPNone)
{
  //  .. nothing yet ..
}

NetTracerLayerExpression::NetTracerLayerExpression (int l)
  : m_a (l), m_b (0), mp_a (0), mp_b (0), m_op (OPNone)
{
  //  .. nothing yet ..
}

NetTracerLayerExpression::NetTracerLayerExpression (const NetTracerLayerExpression &other)
  : m_a (other.m_a), m_b (other.m_b), mp_a (0), mp_b (0), m_op (other.m_op)
{
  if (other.mp_a) {
    mp_a = new NetTracerLayerExpression (*other.mp_a);
  }
  if (other.mp_b) {
    mp_b = new NetTracerLayerExpression (*other.mp_b);
  }
}

NetTracerLayerExpression &
NetTracerLayerExpression::operator= (const NetTracerLayerExpression &other)
{
  if (this != &other) {

    m_a = other.m_a;
    m_b = other.m_b;
    m_op = other.m_op;

    if (mp_a) {
      delete mp_a;
      mp_a = 0;
    }
    if (other.mp_a) {
      mp_a = new NetTracerLayerExpression (*other.mp_a);
    }

    if (mp_b) {
      delete mp_b;
      mp_b = 0;
    }
    if (other.mp_b) {
      mp_b = new NetTracerLayerExpression (*other.mp_b);
    }

  }

  return *this;
}

NetTracerLayerExpression::~NetTracerLayerExpression ()
{
  if (mp_a) {
    delete mp_a;
    mp_a = 0;
  }
  if (mp_b) {
    delete mp_b;
    mp_b = 0;
  }
}

void 
NetTracerLayerExpression::merge (Operator op, NetTracerLayerExpression *other)
{
  if (m_op != OPNone) {
    NetTracerLayerExpression *e = new NetTracerLayerExpression (*this);
    *this = NetTracerLayerExpression ();
    mp_a = e;
  }

  m_op = op;

  if (other->m_op == OPNone) {
    if (other->mp_a) {
      mp_b = new NetTracerLayerExpression (*other->mp_a);
    } else {
      m_b = other->m_a;
    }
    delete other;
  } else {
    mp_b = other;
  }
}

void 
NetTracerLayerExpression::collect_original_layers (std::set<unsigned int> &l) const
{
  if (! mp_a) {
    if (m_a >= 0) {
      l.insert ((unsigned int) m_a);
    }
  } else {
    mp_a->collect_original_layers (l);
  }
  if (m_op != OPNone) {
    if (! mp_b) {
      if (m_b >= 0) {
        l.insert ((unsigned int) m_b);
      }
    } else {
      mp_b->collect_original_layers (l);
    }
  }
}


class PartialShapeDetection
  : public db::EdgeEvaluatorBase 
{
public:
  PartialShapeDetection ()
  {
    //  .. nothing yet ..
  }

  virtual void reset ()
  {
    m_wcv.clear ();
    m_inside.clear ();
  }

  virtual void reserve (size_t n)
  {
    m_wcv.clear ();
    m_inside.clear ();
    m_wcv.resize (n, 0);
  }

  virtual int edge (bool north, bool enter, db::EdgeEvaluatorBase::property_type p)
  {
    if (! north) {
      return 0;
    } else {

      tl_assert (p < m_wcv.size ());

      int *wcv = &m_wcv [p];

      bool inside_before = (*wcv != 0);
      *wcv += (enter ? 1 : -1);
      bool inside_after = (*wcv != 0);

      if (inside_after && ! inside_before) {
        m_inside.insert (p);
      } else if (! inside_after && inside_before) {
        m_inside.erase (p);
      }

      return 1;

    }
  }

  virtual int compare_ns () const
  {
    if (m_inside.find (0) == m_inside.end ()) {
      m_outside.insert (m_inside.begin (), m_inside.end ());
    }
    return 0;
  }

  virtual bool is_reset () const 
  { 
    return m_inside.empty ();
  }

  bool is_outside (size_t n) const
  {
    return m_outside.find (n) != m_outside.end ();
  }

private:
  std::vector <int> m_wcv;
  mutable std::set <db::EdgeEvaluatorBase::property_type> m_inside;
  mutable std::set <db::EdgeEvaluatorBase::property_type> m_outside;
};

void 
NetTracerLayerExpression::compute_results (unsigned int layer, db::cell_index_type cell_index, const std::vector<db::Polygon> *mask, const std::set <std::pair<NetTracerShape, const NetTracerShape *> > &input, const HitTestDataBoxTree *seeds_tree, NetTracerShapeHeap &shape_heap, std::set <std::pair<NetTracerShape, const NetTracerShape *> > &output, const NetTracerData &data, db::EdgeProcessor &ep) const
{
  std::set <std::pair<NetTracerShape, const NetTracerShape *> >::const_iterator i1, i2;
  std::set <std::pair<NetTracerShape, const NetTracerShape *> > shapes, shapes2;
  NetTracerShapeHeap local_shape_heap;
  std::vector<db::Polygon> output_polygons;
  std::vector<const NetTracerShape *> input_shapes;

  if (mp_a) {
    mp_a->compute_results (layer, cell_index, 0, input, 0, local_shape_heap, shapes, data, ep);
    i1 = shapes.begin ();
    i2 = shapes.end ();
  } else {
    i1 = input.end (), i2 = input.end ();
    if (m_a >= 0) {
      for (i1 = input.begin (); i1 != input.end () && i1->first.layer () != (unsigned int) m_a; ++i1) { }
      for (i2 = i1; i2 != input.end () && i2->first.layer () == (unsigned int) m_a; ++i2) { }
    }
  }

  std::vector<db::Polygon> input_a;
  for (std::set <std::pair<NetTracerShape, const NetTracerShape *> >::const_iterator i = i1; i != i2; ++i) {
    if (i->first.shape ().is_box () || i->first.shape ().is_polygon () || i->first.shape ().is_path ()) {
      input_a.push_back (db::Polygon ());
      i->first.shape ().polygon (input_a.back ());
      input_a.back ().transform (db::ICplxTrans (i->first.trans ()));
      input_shapes.push_back (&i->first);
    }
  }

  if (m_op == OPNone) {

    output_polygons.swap (input_a);

  } else {

    if (mp_b) {
      mp_b->compute_results (layer, cell_index, 0, input, 0, local_shape_heap, shapes2, data, ep);
      i1 = shapes2.begin ();
      i2 = shapes2.end ();
    } else {
      i1 = input.end (), i2 = input.end ();
      if (m_b >= 0) {
        for (i1 = input.begin (); i1 != input.end () && i1->first.layer () != (unsigned int) m_b; ++i1) { }
        for (i2 = i1; i2 != input.end () && i2->first.layer () == (unsigned int) m_b; ++i2) { }
      }
    }

    if (m_op == OPOr) {

      output_polygons.swap (input_a);

      for (std::set <std::pair<NetTracerShape, const NetTracerShape *> >::const_iterator i = i1; i != i2; ++i) {
        if (i->first.shape ().is_box () || i->first.shape ().is_polygon () || i->first.shape ().is_path ()) {
          output_polygons.push_back (db::Polygon ());
          i->first.shape ().polygon (output_polygons.back ());
          output_polygons.back ().transform (db::ICplxTrans (i->first.trans ()));
          input_shapes.push_back (&i->first);
        }
      }

    } else {

      std::vector<db::Polygon> input_b;
      for (std::set <std::pair<NetTracerShape, const NetTracerShape *> >::const_iterator i = i1; i != i2; ++i) {
        if (i->first.shape ().is_box () || i->first.shape ().is_polygon () || i->first.shape ().is_path ()) {
          input_b.push_back (db::Polygon ());
          i->first.shape ().polygon (input_b.back ());
          input_b.back ().transform (db::ICplxTrans (i->first.trans ()));
        }
      }

      if (m_op == OPAnd) {
        ep.boolean (input_a, input_b, output_polygons, db::BooleanOp::And);
      } else if (m_op == OPNot) {
        ep.boolean (input_a, input_b, output_polygons, db::BooleanOp::ANotB);
      } else if (m_op == OPXor) {
        ep.boolean (input_a, input_b, output_polygons, db::BooleanOp::Xor);
      }

    }

  }

  //  Apply the mask
  if (mask) {
    input_a.clear ();
    input_a.swap (output_polygons);
    ep.boolean (input_a, *mask, output_polygons, db::BooleanOp::And);
  }

  std::vector <db::Polygon> full_shapes;
  std::vector <db::Polygon> partial_shapes;

  //  Determine what shapes are outside the masked output region
  db::ShapeProcessor sp;

  for (std::vector<db::Polygon>::const_iterator p = output_polygons.begin (); p != output_polygons.end (); ++p) {
    sp.insert_native (*p, 0);
  }

  for (std::vector<const NetTracerShape *>::const_iterator i = input_shapes.begin (); i != input_shapes.end (); ++i) {
    sp.insert ((*i)->shape (), (*i)->trans (), std::distance (std::vector<const NetTracerShape *>::const_iterator (input_shapes.begin ()), i) + 1);
  }

  PartialShapeDetection psd;
  db::EdgeSink es;
  sp.process (es, psd);

  //  Determine all input shapes fully inside the masked delivery. Assign these shapes to seeds by
  //  looking them up in the seed tree. Shapes partially inside the masked deliver are treated later.
  for (std::vector<const NetTracerShape *>::const_iterator i = input_shapes.begin (); i != input_shapes.end (); ++i) {

    db::Polygon ip;
    (*i)->shape ().polygon (ip);
    ip.transform (db::ICplxTrans ((*i)->trans ()));

    if (! psd.is_outside (std::distance (std::vector<const NetTracerShape *>::const_iterator (input_shapes.begin ()), i) + 1)) {

      full_shapes.push_back (ip);

#if 0
      if (seeds_tree) {
        for (HitTestDataBoxTree::touching_iterator s = seeds_tree->begin_touching (ip.box (), HitTestDataBoxConverter ()); ! s.at_end (); ++s) {
          if (interacts (ip, **s)) {
            output.insert (std::make_pair (**i, *s));
          }
        }
      } else {
        output.insert (std::make_pair (**i, (const NetTracerShape *) 0));
      }
#else
      db::Shape os = shape_heap.insert (ip);

      if (seeds_tree) {
        for (HitTestDataBoxTree::touching_iterator s = seeds_tree->begin_touching (ip.box (), HitTestDataBoxConverter ()); ! s.at_end (); ++s) {
          if (interacts (ip, **s)) {
            output.insert (std::make_pair (NetTracerShape (db::ICplxTrans (), os, layer, cell_index), *s));
          }
        }
      } else {
        output.insert (std::make_pair (NetTracerShape (db::ICplxTrans (), os, layer, cell_index), (const NetTracerShape *) 0));
      }
#endif

    } else {
      partial_shapes.push_back (ip);
    }

  }

  if (! partial_shapes.empty ()) {

    input_a.clear ();
    input_a.swap (output_polygons);
    ep.boolean (input_a, full_shapes, output_polygons, db::BooleanOp::ANotB);

    input_a.clear ();
    input_a.swap (output_polygons);
    ep.boolean (input_a, partial_shapes, output_polygons, db::BooleanOp::And);

    //  Assign output to seeds by looking into the seed tree
    for (std::vector<db::Polygon>::const_iterator o = output_polygons.begin (); o != output_polygons.end (); ++o) {

      db::Shape os = shape_heap.insert (*o);

      if (seeds_tree) {
        for (HitTestDataBoxTree::touching_iterator s = seeds_tree->begin_touching (o->box (), HitTestDataBoxConverter ()); ! s.at_end (); ++s) {
          if (interacts (*o, **s)) {
            output.insert (std::make_pair (NetTracerShape (db::ICplxTrans (), os, layer, cell_index), *s));
          }
        }
      } else {
        output.insert (std::make_pair (NetTracerShape (db::ICplxTrans (), os, layer, cell_index), (const NetTracerShape *) 0));
      }

    }

  }
}

std::string
NetTracerLayerExpression::to_string () const
{
  std::string s;
  if (mp_a) {
    s += "(" + mp_a->to_string () + ")";
  } else {
    s += "#" + tl::to_string (m_a);
  }
  if (m_op != OPNone) {
    if (m_op == OPOr) {
      s += "+";
    } else if (m_op == OPAnd) {
      s += "*";
    } else if (m_op == OPXor) {
      s += "^";
    } else if (m_op == OPNot) {
      s += "-";
    }
    if (mp_b) {
      s += "(" + mp_b->to_string () + ")";
    } else {
      s += "#" + tl::to_string (m_b);
    }
  }
  return s;
}

tl::shared_ptr<NetTracerLayerExpression::RegionHolder>
NetTracerLayerExpression::make_l2n_region_for_org (db::LayoutToNetlist &l2n, std::map <unsigned int, tl::shared_ptr<NetTracerLayerExpression::RegionHolder> > &region_cache, int org_index, const std::string &name)
{
  std::map <unsigned int, tl::shared_ptr<NetTracerLayerExpression::RegionHolder> >::iterator r = region_cache.find ((unsigned int) org_index);
  if (r != region_cache.end ()) {
    return r->second;
  } else {
    tl::shared_ptr<NetTracerLayerExpression::RegionHolder> rh (new NetTracerLayerExpression::RegionHolder (l2n.make_layer ((unsigned int) org_index, name)));
    region_cache.insert (std::make_pair ((unsigned int) org_index, rh));
    return rh;
  }
}

tl::shared_ptr<NetTracerLayerExpression::RegionHolder>
NetTracerLayerExpression::make_l2n_region (db::LayoutToNetlist &l2n, std::map <unsigned int, tl::shared_ptr<NetTracerLayerExpression::RegionHolder> > &region_cache, const std::string &name)
{
  tl::shared_ptr<NetTracerLayerExpression::RegionHolder> rha;
  if (mp_a) {
    rha = mp_a->make_l2n_region (l2n, region_cache, m_op == OPNone ? name : std::string ());
  } else {
    rha = make_l2n_region_for_org (l2n, region_cache, m_a, m_op == OPNone ? name : std::string ());
  }

  if (m_op == OPNone) {
    return rha;
  }

  tl::shared_ptr<NetTracerLayerExpression::RegionHolder> rhb;
  if (mp_b) {
    rhb = mp_b->make_l2n_region (l2n, region_cache, std::string ());
  } else {
    rhb = make_l2n_region_for_org (l2n, region_cache, m_b, std::string ());
  }

  std::unique_ptr<db::Region> res (new db::Region (*rha->get ()));

  if (m_op == OPAnd) {
    *res &= *rhb->get ();
  } else if (m_op == OPXor) {
    *res ^= *rhb->get ();
  } else if (m_op == OPOr) {
    *res += *rhb->get ();
  } else if (m_op == OPNot) {
    *res -= *rhb->get ();
  }

  l2n.register_layer (*res, name);

  return tl::shared_ptr<NetTracerLayerExpression::RegionHolder> (new NetTracerLayerExpression::RegionHolder (res.release ()));
}

// -----------------------------------------------------------------------------------
//  NetTracer implementation

NetTracer::NetTracer ()
  : mp_layout (0), mp_cell (0), mp_progress (0), m_name_hier_depth (-1), m_incomplete (false), m_trace_depth (0)
{
  //  .. nothing yet ..
}

void 
NetTracer::clear ()
{
  m_shapes_graph.clear ();
  m_shapes_found.clear ();
  m_shape_heap.clear ();
}

std::string
NetTracer::name () const
{
  return m_name;
}

void 
NetTracer::set_name (const std::string &n)
{
  m_name = n;
}

void 
NetTracer::trace (const db::Layout &layout, const db::Cell &cell, const db::Point &pt_start, unsigned int l_start, const NetTracerData &data)
{
  db::Shape s_start = m_shape_heap.insert (db::Polygon (db::Box (pt_start - db::Vector (1, 1), pt_start + db::Vector (1, 1))));

  NetTracerShape start (db::ICplxTrans (), s_start, l_start, cell.cell_index (), true);
  trace (layout, cell, start, data);

  //  remove the artificial point-like seed from the shape list
  for (std::set <NetTracerShape>::iterator s = m_shapes_found.begin (); s != m_shapes_found.end (); ) {
    std::set <NetTracerShape>::iterator s1 = s;
    ++s;
    if (s1->shape () == s_start) {
      m_shapes_found.erase (s1);
    }
  }

  m_shapes_graph.clear ();
}

void
NetTracer::trace (const db::Layout &layout, const db::Cell &cell, const NetTracerShape &start, const NetTracerData &data)
{
  trace (layout, cell, start, NetTracerShape (), data);
}

void 
NetTracer::trace (const db::Layout &layout, const db::Cell &cell, const db::Point &pt_start, unsigned int l_start, const db::Point &pt_stop, unsigned int l_stop, const NetTracerData &data)
{
  db::Shape s_start = m_shape_heap.insert (db::Polygon (db::Box (pt_start - db::Vector (1, 1), pt_start + db::Vector (1, 1))));
  db::Shape s_stop = m_shape_heap.insert (db::Polygon (db::Box (pt_stop - db::Vector (1, 1), pt_stop + db::Vector (1, 1))));

  NetTracerShape start (db::ICplxTrans (), s_start, l_start, cell.cell_index (), true);
  NetTracerShape stop (db::ICplxTrans (), s_stop, l_stop, cell.cell_index (), true);
  trace (layout, cell, start, stop, data);

  //  remove the artificial point-like seeds from the shape list
  for (std::set <NetTracerShape>::iterator s = m_shapes_found.begin (); s != m_shapes_found.end (); ) {
    std::set <NetTracerShape>::iterator s1 = s;
    ++s;
    if (s1->shape () == s_start || s1->shape () == s_stop) {
      m_shapes_found.erase (s1);
    }
  }

  m_shapes_graph.clear ();
}

void
NetTracer::compute_results_for_next_iteration (const std::vector <const NetTracerShape *> &new_seeds, unsigned int seed_layer, const std::set<unsigned int> &output_layers, std::set <std::pair<NetTracerShape, const NetTracerShape *> > &current, std::set <std::pair<NetTracerShape, const NetTracerShape *> > &output, const NetTracerData &data)
{
  //  Compute the seed hull used to collect all interacting shapes and also to mask them out.
  db::Box secondary_box;
  std::vector <db::Polygon> secondary_seed_polygons;
  secondary_seed_polygons.reserve (current.size ());
  for (std::set <std::pair<NetTracerShape, const NetTracerShape *> >::const_iterator s = current.begin (); s != current.end (); ++s) {

    if (s->first.shape ().is_polygon () || s->first.shape ().is_path () || s->first.shape ().is_box ()) {

      secondary_seed_polygons.push_back (db::Polygon ());
      s->first.shape ().polygon (secondary_seed_polygons.back ());
      secondary_seed_polygons.back ().transform(s->first.trans ());
      secondary_box += secondary_seed_polygons.back ().box ();

    }

  }

  std::vector <db::Polygon> secondary_seed_hull;
  m_ep.simple_merge (secondary_seed_polygons, secondary_seed_hull, false);

  const std::set<unsigned int> &connected_layers = data.connections (seed_layer);

  //  collect all shapes related to that seed hull
  for (std::vector <db::Polygon>::const_iterator s = secondary_seed_hull.begin (); s != secondary_seed_hull.end (); ++s) {
    determine_interactions (*s, 0, connected_layers, current);
  }

#if 0 
TODO: optimization idea:
  //  build a layer map: holds the starting elements for each layer of the delivery.
  //  This exploits the fact that the items in the delivery set are sorted by layer first.
  std::vector<std::set <std::pair<NetTracerShape, const NetTracerShape *> >::const_iterator> layer_chunks;
  for (std::set<std::pair<NetTracerShape, const NetTracerShape *> >::const_iterator c = delivery->begin (); c != delivery->end (); ++c) {
    if (layer_chunks.empty () || layer_chunks.back ()->first.layer != c->first.layer) {
      layer_chunks.push_back (c);
    }
  }
#endif

  HitTestDataBoxTree seed_tree;
  for (std::vector<const NetTracerShape *>::const_iterator seed = new_seeds.begin (); seed != new_seeds.end (); ++seed) {
    seed_tree.insert (*seed);
  }
  seed_tree.sort (HitTestDataBoxConverter ());

  for (std::set<unsigned int>::const_iterator c = output_layers.begin (); c != output_layers.end (); ++c) {

    //  From new_entries compute the results of this operation, use only results interacting with the 
    //  quad tree entries and store them in the output. Use seed_tree to assign the pieces back to the seeds for
    //  building the shape graph. 
    data.expression (*c).compute_results (*c, cell ().cell_index (), &secondary_seed_hull, current, &seed_tree, m_shape_heap, output, data, m_ep);

  }
}

void
NetTracer::trace (const db::Layout &layout, const db::Cell &cell, const NetTracerShape &start, const NetTracerShape &stop, const NetTracerData &data)
{
  mp_layout = &layout;
  mp_cell = &cell;

  m_shapes_graph.clear ();
  m_shapes_found.clear ();

  try {

    tl::AbsoluteProgress progress (tl::to_string (tr ("Tracing Net")), 1);
    progress.set_format (tl::to_string (tr ("%.0f shapes")));
    progress.set_unit (100);
    progress.set_format_unit (1);

    mp_progress = &progress;

    tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Net Tracing")));

    m_stop_shape = stop;
    m_start_shape = start;

    m_hit_test_queue.clear ();

    //  Required in order to provide a connection point for the start shape:
    const NetTracerShape *start_shape = deliver_shape (m_start_shape, (const NetTracerShape *) 0);

    if (! (m_start_shape == m_stop_shape)) {

      //  Use the appropriate logical layer for the start shape
      std::set<unsigned int> ll = data.log_layers_for (start.layer ());
      for (std::set<unsigned int>::const_iterator l = ll.begin (); l != ll.end (); ++l) {

        std::vector <const NetTracerShape *> new_seeds;
        new_seeds.push_back (start_shape);

        std::set <std::pair<NetTracerShape, const NetTracerShape *> > new_entries;
        new_entries.insert (std::make_pair (m_start_shape, (const NetTracerShape *) 0));

        std::set<unsigned int> cl;
        cl.insert (*l);
        compute_results_for_next_iteration (new_seeds, *l, cl, new_entries, m_hit_test_queue, data);

      }

      if (m_stop_shape.is_valid ()) {

        //  Required in order to provide a connection point for the stop shape:
        const NetTracerShape *stop_shape = deliver_shape (m_stop_shape, (const NetTracerShape *) 0);
        if (stop_shape) {

          //  Use the appropriate logical layer for the stop shape
          std::set<unsigned int> ll = data.log_layers_for (stop.layer ());
          for (std::set<unsigned int>::const_iterator l = ll.begin (); l != ll.end (); ++l) {

            std::vector <const NetTracerShape *> new_seeds;
            new_seeds.push_back (stop_shape);

            std::set <std::pair<NetTracerShape, const NetTracerShape *> > new_entries;
            new_entries.insert (std::make_pair (m_stop_shape, (const NetTracerShape *) 0));

            std::set<unsigned int> cl;
            cl.insert (*l);
            compute_results_for_next_iteration (new_seeds, *l, cl, new_entries, m_hit_test_queue, data);

          }

        }

      }

    }

    while (! m_hit_test_queue.empty ()) {

      std::set<std::pair<NetTracerShape, const NetTracerShape *> >::iterator new_seed = m_hit_test_queue.end ();
      --new_seed;

      db::Box combined_box = new_seed->first.bbox ();
      double asum = new_seed->first.shape ().area ();

      unsigned int seed_layer = new_seed->first.layer ();

      std::set<std::pair<NetTracerShape, const NetTracerShape *> >::iterator c;
      size_t n = 1;
      for (c = new_seed; c != m_hit_test_queue.begin (); ) {

        --c;
        ++n;

        if (c->first.layer () == seed_layer) {

          db::Box b = c->first.bbox ();
          double a = c->first.shape ().area ();

          //  The ratio threshold of 20 for box/shape area was determined empirically
          if ((combined_box + b).area () > (asum + a) * 20.0) {
            ++c;
            --n;
            break;
          }

          combined_box += b;
          asum += a;

        } else {

          //  because we sort the set primarily by layer, we can stop now!
          ++c;
          --n;
          break;

        }

      }

      //  Take out the new seeds and deliver them
      std::vector <const NetTracerShape *> new_seeds;
      new_seeds.reserve (n);

      for (std::set <std::pair<NetTracerShape, const NetTracerShape *> >::const_iterator td = c; td != m_hit_test_queue.end (); ++td) {
        const NetTracerShape *shape = deliver_shape (td->first, td->second);
        if (shape) {
          new_seeds.push_back (shape);
        }
      }

      m_hit_test_queue.erase (c, m_hit_test_queue.end ());

      const std::pair <std::set <unsigned int>, std::set <unsigned int> > &bb = data.requires_booleans (seed_layer);
      const std::set<unsigned int> &connected_layers_with_booleans = bb.second;
      const std::set<unsigned int> &connected_layers_without_booleans = bb.first;

      if (! connected_layers_with_booleans.empty ()) {

        //  In the boolean case, we do a collection step first. Then we determine the 
        //  next generation interactions to get all the involved shapes, compute the 
        //  results of the boolean operations and do a shape-to-seed assignment later
        std::set <std::pair<NetTracerShape, const NetTracerShape *> > new_entries;

        //  Determine next-generation interactions
        if (new_seeds.size () == 1) {

          const NetTracerShape *c = new_seeds.back ();

          if (c->shape ().is_box ()) {

            if (c->trans ().is_ortho ()) {
              determine_interactions (c->bbox (), c /*do not do seed assignment*/, connected_layers_with_booleans, new_entries);
            } else {
              db::Polygon box_poly (c->shape ().box ());
              box_poly.transform (db::ICplxTrans (c->trans ()));
              determine_interactions (box_poly, c /*do not do seed assignment*/, connected_layers_with_booleans, new_entries);
            }

          } else if (c->shape ().is_polygon () || c->shape ().is_path ()) {
            db::Polygon p;
            c->shape ().polygon (p);
            p.transform (db::ICplxTrans (c->trans ()));
            determine_interactions (p, c /*do not do seed assignment*/, connected_layers_with_booleans, new_entries);
          }

        } else if (! new_seeds.empty ()) {
          determine_interactions (new_seeds, combined_box, connected_layers_with_booleans, new_entries, true /*do not do seed assignment*/);
        }

        std::set <unsigned int> computed_layers;
        std::set <unsigned int> all_connected = data.log_connections (seed_layer);
        std::set <unsigned int> involved = data.log_connections (seed_layer);
        for (std::set <unsigned int>::const_iterator i = connected_layers_with_booleans.begin (); i != connected_layers_with_booleans.end (); ++i) {
          std::set <unsigned int> ll = data.log_layers_for (*i);
          std::set_intersection (all_connected.begin (), all_connected.end (), ll.begin (), ll.end (), std::inserter (computed_layers, computed_layers.begin ()));
        }

        compute_results_for_next_iteration (new_seeds, seed_layer, computed_layers, new_entries, m_hit_test_queue, data);

      } 

      if (! connected_layers_without_booleans.empty ()) {

        //  If no boolean step is required afterwards, we simply collect the interacting shapes for the new seeds
        //  results. Later we will run the booleans on these results.
        if (new_seeds.size () == 1) {

          const NetTracerShape *c = new_seeds.back ();

          if (c->shape ().is_box ()) {

            if (c->trans ().is_ortho ()) {
              determine_interactions (c->bbox (), c, connected_layers_without_booleans, m_hit_test_queue);
            } else {
              db::Polygon box_poly (c->shape ().box ());
              box_poly.transform (db::ICplxTrans (c->trans ()));
              determine_interactions (box_poly, c, connected_layers_without_booleans, m_hit_test_queue);
            }

          } else if (c->shape ().is_polygon () || c->shape ().is_path ()) {
            db::Polygon p;
            c->shape ().polygon (p);
            p.transform (db::ICplxTrans (c->trans ()));
            determine_interactions (p, c, connected_layers_without_booleans, m_hit_test_queue);
          }

        } else if (! new_seeds.empty ()) {
          determine_interactions (new_seeds, combined_box, connected_layers_without_booleans, m_hit_test_queue, true);
        }

      }

    }

    m_hit_test_queue.clear ();
    m_incomplete = false;
    mp_progress = 0;

  } catch (tl::BreakException &) {

    m_shapes_graph.clear ();

    m_hit_test_queue.clear ();
    m_incomplete = true;
    mp_progress = 0;

    //  on user break or depth exhausted just keep the shapes
    return;

  } catch (...) {

    m_shapes_graph.clear ();

    m_hit_test_queue.clear ();
    m_incomplete = true;
    mp_progress = 0;

    throw;

  }

  try {

    if (m_stop_shape.is_valid ()) {

      tl::AbsoluteProgress search_progress (tl::to_string (tr ("Finding Path")), 100);
      search_progress.set_format (tl::to_string (tr ("Iteration %.0f00")));
      search_progress.set_unit (100);

      const NetTracerShape *stop = &m_shapes_graph.find (m_stop_shape)->first;
      const NetTracerShape *start = &m_shapes_graph.find (m_start_shape)->first;

      //  find the shortest path with Dijkstra's algorithm

      std::map<const NetTracerShape *, const NetTracerShape *> previous;
      std::map<const NetTracerShape *, size_t> cost;
      cost.insert (std::make_pair (stop, 0));
      std::set<const NetTracerShape *> visited;

      bool found = false;

      while (! cost.empty ()) {
        
        ++search_progress;

        size_t min_cost = std::numeric_limits <size_t>::max ();
        for (std::map<const NetTracerShape *, size_t>::const_iterator ac = cost.begin (); ac != cost.end (); ++ac) {
          if (ac->second < min_cost) {
            min_cost = ac->second;
          }
        }

        const NetTracerShape *current = 0;
        for (std::map<const NetTracerShape *, size_t>::iterator ac = cost.begin (); ac != cost.end (); ++ac) {
          if (ac->second == min_cost) {
            current = ac->first;
            visited.insert (current);
            cost.erase (ac);
            break;
          }
        }

        if (! current) {
          break; 
        }

        const std::vector <const NetTracerShape *> &adj = m_shapes_graph[*current];
        for (std::vector <const NetTracerShape *>::const_iterator a = adj.begin (); a != adj.end (); ++a) {
          if (visited.find (*a) == visited.end ()) {
            std::map<const NetTracerShape *, size_t>::iterator ac = cost.find (*a);
            if (ac == cost.end ()) {
              cost.insert (std::make_pair (*a, min_cost + 1));
              previous.insert (std::make_pair (*a, current));
            } else if (min_cost + 1 < ac->second) {
              ac->second = min_cost + 1;
              previous.insert (std::make_pair (*a, current));
            }
          }
        }

        if (previous.find (start) != previous.end ()) {
          found = true;
          break;
        }

      }

      m_shapes_found.clear ();

      if (! found) {
        throw tl::Exception (tl::to_string (tr ("Nets are not connected")));
      }

      const NetTracerShape *s = start;
      while (s) {
        m_shapes_found.insert (*s);
        std::map<const NetTracerShape *, const NetTracerShape *>::const_iterator p = previous.find (s);
        if (p == previous.end ()) {
          s = 0;
        } else {
          s = p->second;
        }
      }

      m_shapes_graph.clear ();

    }

  } catch (...) {
    m_shapes_found.clear ();
    m_shapes_graph.clear ();
    throw;
  }
}

void 
NetTracer::evaluate_text (const db::RecursiveShapeIterator &iter)
{
  if (iter.shape ().is_text ()) {
    if (m_name.empty () || m_name_hier_depth < 0 || m_name_hier_depth > int (iter.depth ())) {
      m_name = iter.shape ().text_string ();
      m_name_hier_depth = int (iter.depth ());
    }
  }
}

const NetTracerShape *
NetTracer::deliver_shape (const NetTracerShape &net_shape, const NetTracerShape *adjacent)
{
  const NetTracerShape *ret = 0;

  if (! m_stop_shape.is_valid ()) {

    if (m_trace_depth > 0 && m_shapes_found.size () >= m_trace_depth) {
      throw tl::BreakException ();
    }

    std::pair<std::set <NetTracerShape>::iterator, bool> f = m_shapes_found.insert (net_shape);
    if (f.second) {
      if (mp_progress) {
        ++(*mp_progress);
      }
      ret = &*f.first;
    } else if (f.first->is_pseudo ()) {
      ret = &*f.first;
    }

  } else {
    
    std::map <NetTracerShape, std::vector<const NetTracerShape *> >::iterator n = m_shapes_graph.find (net_shape);
    if (n == m_shapes_graph.end ()) {

      if (m_trace_depth > 0 && m_shapes_graph.size () >= m_trace_depth) {
        throw tl::BreakException ();
      }

      n = m_shapes_graph.insert (std::make_pair (net_shape, std::vector<const NetTracerShape *> ())).first;

      if (mp_progress) {
        ++(*mp_progress);
      }

      ret = &n->first;

    } else if (n->first.is_pseudo ()) {
      ret = &n->first;
    }

    if (adjacent) {

      n->second.push_back (adjacent);

      //  Record the reverse interaction
      std::map <NetTracerShape, std::vector<const NetTracerShape *> >::iterator m = m_shapes_graph.find (*adjacent);
      m->second.push_back (&n->first);

    }

  } 

  return ret;
}

void
NetTracer::determine_interactions (const std::vector<const NetTracerShape *> &seeds, const db::Box &combined_box, const std::set<unsigned int> &layers, std::set <std::pair<NetTracerShape, const NetTracerShape *> > &delivery, bool do_seed_assignment)
{
  bool extract_full_graph = m_stop_shape.is_valid ();

  HitTestDataBoxTree seed_tree;
  for (std::vector<const NetTracerShape *>::const_iterator seed = seeds.begin (); seed != seeds.end (); ++seed) {
    seed_tree.insert (*seed);
  }
  seed_tree.sort (HitTestDataBoxConverter ());

  db::RecursiveShapeIterator net_shapes (layout (), cell (), layers, combined_box);
  while (! net_shapes.at_end ()) {

    NetTracerShape net_shape (net_shapes.trans (), net_shapes.shape (), net_shapes.layer (), net_shapes.cell_index ());

    for (HitTestDataBoxTree::touching_iterator s = seed_tree.begin_touching (net_shape.bbox (), HitTestDataBoxConverter ()); ! s.at_end (); ++s) {

      const NetTracerShape *seed = *s;

      evaluate_text (net_shapes);

      bool interact = false;

      if (seed->shape ().is_box ()) {

        if (seed->trans ().is_ortho ()) {
          interact = interacts (seed->bbox (), net_shape);
        } else  {
          db::Polygon box_poly (seed->shape ().box ());
          box_poly.transform (db::ICplxTrans (seed->trans ()));
          interact = interacts (box_poly, net_shape);
        }

      } else if (seed->shape ().is_polygon () || seed->shape ().is_path ()) {
        db::Polygon p;
        seed->shape ().polygon (p);
        p.transform (db::ICplxTrans (seed->trans ()));
        interact = interacts (p, net_shape);
      }

      if (interact) {
        delivery.insert (std::make_pair (net_shape, do_seed_assignment ? seed : (const NetTracerShape *) 0));
        if (! extract_full_graph) {
          break;
        }
      }

    }

    ++net_shapes;

  }
}

void
NetTracer::determine_interactions (const db::Box &seed, const NetTracerShape *shape, const std::set<unsigned int> &layers, std::set <std::pair<NetTracerShape, const NetTracerShape *> > &delivery)
{
  db::RecursiveShapeIterator net_shapes (layout (), cell (), layers, seed);
  while (! net_shapes.at_end ()) {

    NetTracerShape net_shape (net_shapes.trans (), net_shapes.shape (), net_shapes.layer (), net_shapes.cell_index ());

    evaluate_text (net_shapes);

    if (interacts (seed, net_shape)) {
      delivery.insert (std::make_pair (net_shape, shape));
    }

    ++net_shapes;

  }
}

void
NetTracer::determine_interactions (const db::Polygon &seed, const NetTracerShape *shape, const std::set<unsigned int> &layers, std::set <std::pair<NetTracerShape, const NetTracerShape *> > &delivery)
{
  int area_ratio = 2;

  db::Polygon::area_type poly_area = seed.area ();
  db::Polygon::area_type box_area = seed.box ().area ();

  if (poly_area == box_area && seed.vertices () == 4) {

    //  The polygon is a box
    determine_interactions (seed.box (), shape, layers, delivery);

  } else if (poly_area + 1 >= box_area / area_ratio) {

    //  The polygon is sufficiently "dense", so it can be used as it is.
    db::RecursiveShapeIterator net_shapes (layout (), cell (), layers, seed.box ());
    while (! net_shapes.at_end ()) {

      NetTracerShape net_shape (net_shapes.trans (), net_shapes.shape (), net_shapes.layer (), net_shapes.cell_index ());

      evaluate_text (net_shapes);

      if (interacts (seed, net_shape)) {
        delivery.insert (std::make_pair (net_shape, shape));
      }

      ++net_shapes;

    }

  } else {

    //  otherwise split polygon and recursively treat these parts ...
    std::vector <db::Polygon> polygons;
    db::split_polygon (seed, polygons);

    for (std::vector<db::Polygon>::const_iterator p = polygons.begin (); p != polygons.end (); ++p) {
      determine_interactions (*p, shape, layers, delivery);
    }

  }
}

}

