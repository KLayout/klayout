
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

#ifndef HDR_pexRExtractor
#define HDR_pexRExtractor

#include "pexCommon.h"

#include "dbPolygon.h"
#include "dbPLC.h"
#include "dbPLCConvexDecomposition.h" // @@@
#include "dbPLCTriangulation.h"  // @@@
#include "tlList.h"

#include <string>
#include <list>

namespace pex
{

class RElement;
class RNode;

struct PEX_PUBLIC RNode
  : public tl::list_node<RNode>
{
public:
  enum node_type {
    INTERNAL,
    VERTEX_PORT,
    POLYGON_PORT
  };

  node_type type;
  db::DBox location;
  unsigned int port_index;
  mutable std::list<const RElement *> elements;

protected:
  friend class RNetwork;
  friend class tl::list_impl<RNode, false>;

  RNode (node_type _type, const db::DBox &_location, unsigned int _port_index)
    : type (_type), location (_location), port_index (_port_index)
  { }

  ~RNode () { }

private:
  RNode (const RNode &other);
  RNode &operator= (const RNode &other);
};

struct PEX_PUBLIC RElement
  : public tl::list_node<RElement>
{
  double conductivity;
  const RNode *a, *b;

  double resistance () const
  {
    return 1.0 / conductivity;
  }

protected:
  friend class RNetwork;
  friend class tl::list_impl<RElement, false>;

  RElement (double _conductivity, const RNode *_a, const RNode *_b)
    : conductivity (_conductivity), a (_a), b (_b)
  { }

  ~RElement ()
  {
    if (a) {
      a->elements.erase (m_ia);
    }
    if (b) {
      b->elements.erase (m_ib);
    }
    a = b = 0;
  }

  std::list<const RElement *>::iterator m_ia, m_ib;

private:
  RElement (const RElement &other);
  RElement &operator= (const RElement &other);
};

class PEX_PUBLIC RNetwork
{
public:
  RNetwork ();
  ~RNetwork ();

  RNode *create_node (RNode::node_type type, unsigned int port_index);
  RElement *create_element (double conductivity, RNode *a, RNode *b);
  void remove_element (RElement *element);
  void remove_node (RNode *node);
  void clear ();

private:
  tl::list<RNode, false> m_nodes;
  tl::list<RElement, false> m_elements;
  std::map<std::pair<RNode *, RNode *>, RElement *> m_elements_by_nodes;
  std::map<std::pair<RNode::node_type, unsigned int>, RNode *> m_nodes_by_type;

  RNetwork (const RNetwork &);
  RNetwork &operator= (const RNetwork &);
};


/**
 *  @brief A base class for an resistance extractor
 *
 *  The R extractor takes a polyon, a technology definition
 *  and port definitions and extracts a resistor network.
 *  
 *  Ports are points or polygons that define the connection
 *  points to the network.
 */
class PEX_PUBLIC RExtractor
{
public:
  RExtractor ();
  virtual ~RExtractor ();

  virtual void extract (const db::Polygon &polygon, const std::vector<db::Point> &vertex_ports, const std::vector<db::Polygon> &polygon_ports, RNetwork &rnetwork) = 0;
};

// @@@
class PEX_PUBLIC SquareCountingRExtractor
  : public RExtractor
{
public:
  SquareCountingRExtractor (double dbu);

  db::plc::ConvexDecompositionParameters &decomposition_parameters ()
  {
    return m_decomp_param;
  }

  void set_dbu (double dbu)
  {
    m_dbu = dbu;
  }

  double dbu () const
  {
    return m_dbu;
  }

  virtual void extract (const db::Polygon &polygon, const std::vector<db::Point> &vertex_ports, const std::vector<db::Polygon> &polygon_ports, RNetwork &rnetwork);

private:
  db::plc::ConvexDecompositionParameters m_decomp_param;
  double m_dbu;
};

}

#endif

