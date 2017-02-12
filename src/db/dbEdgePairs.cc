
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


#include "dbCommon.h"

#include "dbEdgePairs.h"
#include "dbEdges.h"
#include "dbRegion.h"

#include "tlVariant.h"

#include <sstream>

namespace db
{

void 
EdgePairs::insert (const db::Edge &e1, const db::Edge &e2)
{
  m_edge_pairs.push_back (db::EdgePair (e1, e2));
  m_bbox_valid = false;
}

void 
EdgePairs::insert (const edge_pair_type &ep)
{
  m_edge_pairs.push_back (ep);
  m_bbox_valid = false;
}

bool  
EdgePairs::operator== (const db::EdgePairs &other) const
{
  if (empty () != other.empty ()) {
    return false;
  }
  if (size () != other.size ()) {
    return false;
  }
  db::EdgePairs::const_iterator o1 = begin ();
  db::EdgePairs::const_iterator o2 = other.begin ();
  while (o1 != end () && o2 != other.end ()) {
    if (*o1 != *o2) {
      return false;
    }
    ++o1;
    ++o2;
  }
  return true;
}

bool  
EdgePairs::operator< (const db::EdgePairs &other) const
{
  if (empty () != other.empty ()) {
    return empty () < other.empty ();
  }
  if (size () != other.size ()) {
    return (size () < other.size ());
  }
  db::EdgePairs::const_iterator o1 = begin ();
  db::EdgePairs::const_iterator o2 = other.begin ();
  while (o1 != end () && o2 != other.end ()) {
    if (*o1 != *o2) {
      return *o1 < *o2;
    }
    ++o1;
    ++o2;
  }
  return false;
}

db::EdgePairs &
EdgePairs::operator+= (const db::EdgePairs &other)
{
  if (! other.empty ()) {
    m_edge_pairs.insert (m_edge_pairs.end (), other.begin (), other.end ());
    m_bbox_valid = false;
  }
  return *this;
}

std::string 
EdgePairs::to_string (size_t nmax) const
{
  std::ostringstream os;
  const_iterator ep;
  for (ep = begin (); ep != end () && nmax != 0; ++ep, --nmax) {
    if (ep != begin ()) {
      os << ";";
    }
    os << ep->to_string ();
  }
  if (ep != end ()) {
    os << "...";
  }
  return os.str ();
}

void 
EdgePairs::clear ()
{
  m_edge_pairs.clear ();
  m_bbox = db::Box ();
  m_bbox_valid = true;
}

void 
EdgePairs::polygons (Region &output, db::Coord e) const
{
  for (const_iterator ep = begin (); ep != end (); ++ep) {
    db::Polygon poly = ep->normalized ().to_polygon (e);
    if (poly.vertices () >= 3) {
      output.insert (poly);
    }
  }
}

void 
EdgePairs::edges (Edges &output) const
{
  for (const_iterator ep = begin (); ep != end (); ++ep) {
    output.insert (ep->first ());
    output.insert (ep->second ());
  }
}

void 
EdgePairs::first_edges (Edges &output) const
{
  for (const_iterator ep = begin (); ep != end (); ++ep) {
    output.insert (ep->first ());
  }
}

void 
EdgePairs::second_edges (Edges &output) const
{
  for (const_iterator ep = begin (); ep != end (); ++ep) {
    output.insert (ep->second ());
  }
}

void 
EdgePairs::init ()
{
  m_bbox_valid = false;
  m_report_progress = false;
}

void 
EdgePairs::ensure_bbox_valid () const
{
  if (! m_bbox_valid) {
    m_bbox = db::Box ();
    for (const_iterator ep = begin (); ep != end (); ++ep) {
      m_bbox += db::Box (ep->first ().p1 (), ep->first ().p2 ());
      m_bbox += db::Box (ep->second ().p1 (), ep->second ().p2 ());
    }
    m_bbox_valid = true;
  }
}

void 
EdgePairs::disable_progress ()
{
  m_report_progress = false;
}

void 
EdgePairs::enable_progress (const std::string &progress_desc)
{
  m_report_progress = true;
  m_progress_desc = progress_desc;
}

}

namespace tl
{
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::EdgePairs &b)
  {
    db::EdgePair ep;

    if (! ex.try_read (ep)) {
      return false;
    }
    b.insert (ep);

    while (ex.test (";")) {
      ex.read (ep);
      b.insert (ep);
    } 

    return true;
  }

  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::EdgePairs &b)
  {
    if (! test_extractor_impl (ex, b)) {
      ex.error (tl::to_string (QObject::tr ("Expected an edge pair collection specification")));
    }
  }
}

