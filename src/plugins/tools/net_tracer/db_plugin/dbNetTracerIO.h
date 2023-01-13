
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

#ifndef HDR_dbNetTracerIO
#define HDR_dbNetTracerIO

#include "dbPluginCommon.h"
#include "dbNetTracer.h"
#include "dbLayerProperties.h"
#include "dbTechnology.h"
#include "tlColor.h"

namespace db
{

class NetTracerConnectivity;

DB_PLUGIN_PUBLIC std::string net_tracer_component_name ();

class DB_PLUGIN_PUBLIC NetTracerLayerExpressionInfo
{
public:
  NetTracerLayerExpressionInfo ();
  ~NetTracerLayerExpressionInfo ();
  NetTracerLayerExpressionInfo (const NetTracerLayerExpressionInfo &other);
  NetTracerLayerExpressionInfo &operator= (const NetTracerLayerExpressionInfo &other);

  bool operator== (const NetTracerLayerExpressionInfo &other) const
  {
    return m_expression == other.m_expression;
  }

  static NetTracerLayerExpressionInfo compile (const std::string &s);
  static NetTracerLayerExpressionInfo parse (tl::Extractor &ex);

  const std::string &to_string () const
  {
    return m_expression;
  }

  NetTracerLayerExpression *get (const db::Layout &layout, const NetTracerConnectivity &tech) const;

private:
  std::string m_expression;
  db::LayerProperties m_a, m_b;
  NetTracerLayerExpressionInfo *mp_a, *mp_b;
  NetTracerLayerExpression::Operator m_op;

  void merge (NetTracerLayerExpression::Operator op, const NetTracerLayerExpressionInfo &other);
  static NetTracerLayerExpressionInfo parse_add (tl::Extractor &ex);
  static NetTracerLayerExpressionInfo parse_mult (tl::Extractor &ex);
  static NetTracerLayerExpressionInfo parse_atomic (tl::Extractor &ex);

  NetTracerLayerExpression *get (const db::Layout &layout, const NetTracerConnectivity &tech, const std::set<std::string> &used_symbols) const;
  NetTracerLayerExpression *get_expr (const db::LayerProperties &lp, const db::Layout &layout, const NetTracerConnectivity &tech, const std::set<std::string> &used_symbols) const;
};

class DB_PLUGIN_PUBLIC NetTracerConnectionInfo
{
public:
  NetTracerConnectionInfo ();
  NetTracerConnectionInfo (const NetTracerLayerExpressionInfo &la, const NetTracerLayerExpressionInfo &lb);
  NetTracerConnectionInfo (const NetTracerLayerExpressionInfo &la, const NetTracerLayerExpressionInfo &via, const NetTracerLayerExpressionInfo &lb);

  NetTracerConnection get (const db::Layout &layout, const NetTracerConnectivity &tech, NetTracerData &data) const;

  std::string to_string () const;
  void parse (tl::Extractor &ex);

  const NetTracerLayerExpressionInfo &layer_a () const
  {
    return m_la;
  }

  void set_layer_a (const NetTracerLayerExpressionInfo &l)
  {
    m_la = l;
  }

  const NetTracerLayerExpressionInfo &via_layer () const
  {
    return m_via;
  }

  void set_via_layer (const NetTracerLayerExpressionInfo &l)
  {
    m_via = l;
  }

  const NetTracerLayerExpressionInfo &layer_b () const
  {
    return m_lb;
  }

  void set_layer_b (const NetTracerLayerExpressionInfo &l)
  {
    m_lb = l;
  }

private:
  NetTracerLayerExpressionInfo m_la, m_via, m_lb;
};

class DB_PLUGIN_PUBLIC NetTracerSymbolInfo
{
public:
  NetTracerSymbolInfo ();
  NetTracerSymbolInfo (const db::LayerProperties &symbol, const std::string &expression);

  std::string to_string () const;
  void parse (tl::Extractor &ex);

  const db::LayerProperties &symbol () const
  {
    return m_symbol;
  }

  void set_symbol (const db::LayerProperties &s)
  {
    m_symbol = s;
  }

  const std::string &expression () const
  {
    return m_expression;
  }

  void set_expression (const std::string &e)
  {
    m_expression = e;
  }

private:
  db::LayerProperties m_symbol;
  std::string m_expression;
};

class DB_PLUGIN_PUBLIC NetTracerNet
{
public:
  typedef std::vector <db::NetTracerShape>::const_iterator iterator;

  /**
   *  @brief Default constructor
   */
  NetTracerNet ();

  /**
   *  @brief Constructor
   */
  NetTracerNet (const db::NetTracer &tracer, const db::ICplxTrans &trans, const db::Layout &layout, db::cell_index_type cell_index, const std::string &layout_filename, const std::string &layout_name, const db::NetTracerData &data);

  /**
   *  @brief Iterate the shapes (begin)
   */
  iterator begin () const
  {
    return m_net_shapes.begin ();
  }

  /**
   *  @brief Iterate the shapes (end)
   */
  iterator end () const
  {
    return m_net_shapes.end ();
  }

  /**
   *  @brief Gets the number of shapes
   */
  size_t size () const
  {
    return m_net_shapes.size ();
  }

  /**
   *  @brief Gets the color in which the net is drawn
   */
  const tl::Color &color () const
  {
    return m_color;
  }

  /**
   *  @brief Sets the color in which the net is drawn
   */
  void set_color (const tl::Color &c)
  {
    m_color = c;
  }

  /**
   *  @brief Get a name for the net
   *
   *  The name can be empty if not specific net name could be determined (i.e. from a label or property)
   */
  std::string name () const
  {
    return m_name;
  }

  /**
   *  @brief Set a name for the net
   */
  void set_name (const std::string &n)
  {
    m_name = n;
  }

  /**
   *  @brief Gets the database unit
   */
  double dbu () const
  {
    return m_dbu;
  }

  /**
   *  @brief Returns true, if the net is incomplete
   *
   *  This flag is true if the extractor was aborted
   *  for example by the user.
   *  The shapes do not fully cover the net.
   */
  bool incomplete () const
  {
    return m_incomplete;
  }

  /**
   *  @brief Gets the file name
   */
  const std::string &layout_filename () const
  {
    return m_layout_filename;
  }

  /**
   *  @brief Gets the layout name
   */
  const std::string &layout_name () const
  {
    return m_layout_name;
  }

  /**
   *  @brief Gets the top cell name
   */
  const std::string &top_cell_name () const
  {
    return m_top_cell_name;
  }

  /**
   *  @brief Gets the cell name for a given cell index
   */
  const std::string &cell_name (db::cell_index_type cell_index) const;

  /**
   *  @brief Provides the
   */
  db::LayerProperties representative_layer_for (unsigned int log_layer) const;

  /**
   *  @brief Provides a mapping for logical layers to original ones
   */
  db::LayerProperties layer_for (unsigned int log_layer) const;

  /**
   *  @brief Export the net to another layout/cell
   */
  std::vector<unsigned int> export_net (db::Layout &layout, db::Cell &export_cell);

  /**
   *  @brief Set the start search box for redo
   */
  void set_start_search_box (const db::DBox &p)
  {
    m_start_search_box = p;
  }

  /**
   *  @brief Gets the start search box
   */
  const db::DBox &start_search_box () const
  {
    return m_start_search_box;
  }

  /**
   *  @brief Set the stop search box for redo
   */
  void set_stop_search_box (const db::DBox &p)
  {
    m_stop_search_box = p;
  }

  /**
   *  @brief Gets the stop search box
   */
  const db::DBox &stop_search_box () const
  {
    return m_stop_search_box;
  }

  /**
   *  @brief Sets the "trace path" flag for redo
   */
  void set_trace_path_flag (bool tp)
  {
    m_trace_path = tp;
  }

  /**
   *  @brief Gets the "trace path" flag
   */
  bool trace_path_flag () const
  {
    return m_trace_path;
  }

private:
  double m_dbu;
  std::string m_name;
  std::string m_layout_filename;
  std::string m_layout_name;
  std::string m_top_cell_name;
  bool m_incomplete;
  std::vector <db::NetTracerShape> m_net_shapes;
  db::Shapes m_shapes;
  std::map <unsigned int, std::pair <db::LayerProperties, db::LayerProperties> > m_layers;
  std::map <unsigned int, std::string> m_cell_names;
  tl::Color m_color;
  db::DBox m_start_search_box, m_stop_search_box;
  bool m_trace_path;

  void define_layer (unsigned int l, const db::LayerProperties &lp, const db::LayerProperties &lp_representative);
};

class DB_PLUGIN_PUBLIC NetTracerConnectivity
{
public:
  typedef std::vector<NetTracerConnectionInfo>::const_iterator const_iterator;
  typedef std::vector<NetTracerConnectionInfo>::iterator iterator;
  typedef std::vector<NetTracerSymbolInfo>::const_iterator const_symbol_iterator;
  typedef std::vector<NetTracerSymbolInfo>::iterator symbol_iterator;

  NetTracerConnectivity ();
  NetTracerConnectivity (const NetTracerConnectivity &d);
  NetTracerConnectivity &operator= (const NetTracerConnectivity &d);

  bool is_fallback_default () const
  {
    return m_is_fallback_default;
  }

  void set_fallback_default (bool f)
  {
    m_is_fallback_default = f;
  }

  const std::string &name () const
  {
    return m_name;
  }

  void set_name (const std::string &n)
  {
    m_name = n;
  }

  const std::string &description () const
  {
    return m_description;
  }

  void set_description (const std::string &d)
  {
    m_description = d;
  }

  const_iterator begin () const
  {
    return m_connections.begin ();
  }

  iterator begin ()
  {
    return m_connections.begin ();
  }

  const_iterator end () const
  {
    return m_connections.end ();
  }

  iterator end ()
  {
    return m_connections.end ();
  }

  const_symbol_iterator begin_symbols () const
  {
    return m_symbols.begin ();
  }

  symbol_iterator begin_symbols ()
  {
    return m_symbols.begin ();
  }

  const_symbol_iterator end_symbols () const
  {
    return m_symbols.end ();
  }

  symbol_iterator end_symbols ()
  {
    return m_symbols.end ();
  }

  void clear ()
  {
    m_connections.clear ();
    m_symbols.clear ();
  }

  void clear_connections ()
  {
    m_connections.clear ();
  }

  void clear_symbols ()
  {
    m_symbols.clear ();
  }

  void erase (iterator p)
  {
    m_connections.erase (p);
  }

  void insert (iterator p, const NetTracerConnectionInfo &info)
  {
    m_connections.insert (p, info);
  }

  void add (const NetTracerConnectionInfo &info)
  {
    m_connections.push_back (info);
  }

  void erase_symbol (symbol_iterator p)
  {
    m_symbols.erase (p);
  }

  void insert_symbol (symbol_iterator p, const NetTracerSymbolInfo &info)
  {
    m_symbols.insert (p, info);
  }

  void add_symbol (const NetTracerSymbolInfo &info)
  {
    m_symbols.push_back (info);
  }

  size_t size () const
  {
    return m_connections.size ();
  }

  size_t symbols () const
  {
    return m_symbols.size ();
  }

  NetTracerData get_tracer_data (const db::Layout &layout) const;

private:
  std::vector<NetTracerConnectionInfo> m_connections;
  std::vector<NetTracerSymbolInfo> m_symbols;
  std::string m_name, m_description;
  bool m_is_fallback_default;
};

class DB_PLUGIN_PUBLIC NetTracerTechnologyComponent
  : public db::TechnologyComponent
{
public:
  typedef std::vector<NetTracerConnectivity>::const_iterator const_iterator;
  typedef std::vector<NetTracerConnectivity>::iterator iterator;

  NetTracerTechnologyComponent ();

  size_t size () const
  {
    return m_connectivity.size ();
  }

  void push_back (const db::NetTracerConnectivity &c)
  {
    m_connectivity.push_back (c);
  }

  void clear ()
  {
    m_connectivity.clear ();
  }

  void erase (iterator i)
  {
    m_connectivity.erase (i);
  }

  void insert (iterator i, const db::NetTracerConnectivity &c)
  {
    m_connectivity.insert (i, c);
  }

  const_iterator begin () const
  {
    return m_connectivity.begin ();
  }

  const_iterator end () const
  {
    return m_connectivity.end ();
  }

  iterator begin ()
  {
    return m_connectivity.begin ();
  }

  iterator end ()
  {
    return m_connectivity.end ();
  }

  db::NetTracerTechnologyComponent *clone () const
  {
    return new NetTracerTechnologyComponent (*this);
  }

private:
  std::vector<NetTracerConnectivity> m_connectivity;
};

}

#endif
