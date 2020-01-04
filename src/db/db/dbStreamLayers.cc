
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "dbStreamLayers.h"
#include "dbLayoutUtils.h"

#include "tlException.h"
#include "tlString.h"

#include <stdio.h>
#include <set>
#include <sstream>

namespace db
{

// ---------------------------------------------------------------
//  LayerMap

/// A helper class to join two datatype map members
struct LmapJoinOp1
{
  void operator() (unsigned int &a, unsigned int b)
  {
    //  TODO: could also do?: throw an exception ..
    a = b;
  }
};

/// A helper class to join two layer map members 
/// (this implementation basically merged the datatype maps)  
struct LmapJoinOp2
{
  void operator() (LayerMap::datatype_map &a, const LayerMap::datatype_map &b)
  {
    LmapJoinOp1 op1;
    a.add (b.begin (), b.end (), op1);
  }
};

/// Utility typedefs for the expression parser  
typedef std::pair<ld_type, ld_type> ld_interval;

/// Utility typedefs for the expression parser  
typedef std::vector<ld_interval> ld_interval_vector;


LayerMap::LayerMap ()
  : m_ld_map (), m_next_index (0)
{
  //  .. nothing yet ..
}

std::pair<bool, unsigned int> 
LayerMap::logical (const LDPair &p) const
{
  const datatype_map *dm = m_ld_map.mapped (p.layer);
  if (dm) {
    const unsigned int *l = dm->mapped (p.datatype);
    if (l) {
      return std::make_pair (true, *l);
    }
  }
  return std::make_pair (false, 0);
}

std::pair<bool, unsigned int> 
LayerMap::logical (const std::string &n) const
{
  std::map<std::string, unsigned int>::const_iterator m = m_name_map.find (n);
  if (m != m_name_map.end ()) {
    return std::make_pair (true, m->second);
  } else {
    return std::make_pair (false, 0);
  }
}

std::pair<bool, unsigned int> 
LayerMap::logical (const db::LayerProperties &p) const
{
  if (p.layer >= 0 && p.datatype >= 0) {
    std::pair<bool, unsigned int> m = logical (db::LDPair (p.layer, p.datatype));
    if (m.first) {
      return m;
    }
  }
  if (! p.name.empty ()) {
    std::pair<bool, unsigned int> m = logical (p.name);
    if (m.first) {
      return m;
    }
  }
  return std::make_pair (false, 0);
}

std::string
LayerMap::mapping_str (unsigned int ll) const
{
  std::string s;
  bool f1 = true;

  for (ld_map::const_iterator l = m_ld_map.begin (); l != m_ld_map.end (); ++l) {

    bool f2 = true;
    for (datatype_map::const_iterator d = l->second.begin (); d != l->second.end (); ++d) {
      if (d->second == ll) {

        //  create a string representation
        if (!f2) {
          s += ",";
        } else {

          if (!f1) {
            s += ";";
          }
          f1 = false;

          s += tl::to_string (l->first.first);
          if (l->first.first < l->first.second - 1) {
            s += "-";
            s += tl::to_string (l->first.second - 1);
          }
          s += "/";

        }
        f2 = false;

        s += tl::to_string (d->first.first);
        if (d->first.first < d->first.second - 1) {
          s += "-";
          s += tl::to_string (d->first.second - 1);
        }

      }

    }
    
  }

  for (std::map <std::string, unsigned int>::const_iterator l = m_name_map.begin (); l != m_name_map.end (); ++l) {
    if (l->second == ll) {

      if (!f1) {
        s += ";";
      }
      f1 = false;

      s += tl::to_word_or_quoted_string (l->first);

    }
  }

  std::map<unsigned int, LayerProperties>::const_iterator t = m_target_layers.find (ll);
  if (t != m_target_layers.end ()) {
    s += " : ";
    s += t->second.to_string ();
  }

  return s;
}

void
LayerMap::prepare (db::Layout &layout)
{
  std::map<unsigned int, unsigned int> real_layers;
  std::set<unsigned int> mapped_layers;

  //  determine the mapping of existing layers to real layers and create layers if required
  DirectLayerMapping layer_mapping (&layout);

  std::vector<unsigned int> old_layers = get_layers ();
  for (std::vector<unsigned int>::const_iterator l = old_layers.begin (); l != old_layers.end (); ++l) {
    if (layout.is_valid_layer (*l)) {
      real_layers.insert (std::make_pair (*l, *l));
      mapped_layers.insert (*l);
    } else {
      std::pair <bool, unsigned int> lm = layer_mapping.map_layer (mapping (*l));
      if (lm.first) {
        real_layers.insert (std::make_pair (*l, lm.second));
        mapped_layers.insert (lm.second);
      }
    }
  }

  //  Now remap the indexes
  for (ld_map::iterator l = m_ld_map.begin (); l != m_ld_map.end (); ++l) {
    for (datatype_map::iterator d = l->second.begin (); d != l->second.end (); ++d) {
      d->second = real_layers [d->second];
    }
  }
  for (std::map<std::string, unsigned int>::iterator n = m_name_map.begin (); n != m_name_map.end (); ++n) {
    n->second = real_layers [n->second];
  }

  std::map<unsigned int, LayerProperties> old_target_layers;
  old_target_layers.swap (m_target_layers);

  for (std::map<unsigned int, LayerProperties>::const_iterator tl = old_target_layers.begin (); tl != old_target_layers.end (); ++tl) {
    m_target_layers[real_layers [tl->first]] = tl->second;
  }

  //  In addition, map other existing layers as well, so merging of layout is somewhat better supported
  for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
    if (mapped_layers.find ((*l).first) == mapped_layers.end ()) {
      map (*(*l).second, (*l).first);
    }
  }
}

std::vector<unsigned int> 
LayerMap::get_layers () const
{
  std::set<unsigned int> layers;

  for (ld_map::const_iterator l = m_ld_map.begin (); l != m_ld_map.end (); ++l) {
    for (datatype_map::const_iterator d = l->second.begin (); d != l->second.end (); ++d) {
      layers.insert (d->second);
    }
  }
  for (const_iterator_names n = m_name_map.begin (); n != m_name_map.end (); ++n) {
    layers.insert(n->second);
  }

  return std::vector<unsigned int> (layers.begin (), layers.end ());
}

LayerProperties 
LayerMap::mapping (unsigned int ll) const
{
  //  try to combine source and target spec into a final specification (if for example, just a name is specified and
  //  layer/datatype are given in the source spec).
  db::LayerProperties p;

  std::map<unsigned int, LayerProperties>::const_iterator t = m_target_layers.find (ll);
  if (t != m_target_layers.end ()) {
    p = t->second;
  }

  if (p.layer < 0 || p.datatype < 0) {
    for (ld_map::const_iterator l = m_ld_map.begin (); l != m_ld_map.end (); ++l) {
      for (datatype_map::const_iterator d = l->second.begin (); d != l->second.end (); ++d) {
        if (d->second == ll) {
          p.layer = l->first.first;
          p.datatype = d->first.first;
          break;
        }
      }
    }
  }

  if (p.name.empty ()) {
    for (std::map <std::string, unsigned int>::const_iterator l = m_name_map.begin (); l != m_name_map.end (); ++l) {
      if (l->second == ll) {
        p.name = l->first;
        break;
      }
    }
  }

  //  no layer found
  return p;
}

void 
LayerMap::map (const LDPair &p, unsigned int l)
{
  insert (p, p, l, LayerProperties ());
}

void 
LayerMap::map (const std::string &name, unsigned int l)
{
  insert (name, l, LayerProperties ());
}

void 
LayerMap::map (const LayerProperties &f, unsigned int l)
{
  if (f.layer >= 0 && f.datatype >= 0) {
    map (db::LDPair (f.layer, f.datatype), l);
  } 
  if (! f.name.empty ()) {
    map (f.name, l);
  }
}

void 
LayerMap::map (const LDPair &p, unsigned int l, const LayerProperties &t)
{
  insert (p, p, l, t);
}

void 
LayerMap::map (const std::string &name, unsigned int l, const LayerProperties &t)
{
  insert (name, l, t);
}

void 
LayerMap::map (const LayerProperties &f, unsigned int l, const LayerProperties &t)
{
  if (f.layer >= 0 && f.datatype >= 0) {
    map (db::LDPair (f.layer, f.datatype), l, t);
  } 
  if (! f.name.empty ()) {
    map (f.name, l, t);
  }
}

void 
LayerMap::map (const LDPair &p1, const LDPair &p2, unsigned int l)
{
  insert (p1, p2, l, LayerProperties ());
}

void 
LayerMap::map (const LDPair &p1, const LDPair &p2, unsigned int l, const LayerProperties &lp)
{
  insert (p1, p2, l, lp);
}

/// Utility function for the expression parser:  
/// Parse an interval
void
parse_interval (tl::Extractor &ex, ld_interval &p)
{
  ld_type n1 = 0, n2 = 0;

  ex.try_read (n1);
  if (ex.test ("-")) {
    ex.try_read (n2);
  } else {
    n2 = n1;
  }

  p.first = n1;
  p.second = n2;
}

/// Utility function for the expression parser:  
/// Parse an interval list
void
parse_intervals (tl::Extractor &ex, ld_interval_vector &v)
{
  do {
    v.push_back (ld_interval (0, 0));
    parse_interval (ex, v.back ());
  } while (ex.test (","));
}

void 
LayerMap::map_expr (const std::string &expr, unsigned int l)
{
  tl::Extractor ex (expr.c_str ());
  map_expr (ex, l);
  ex.expect_end ();
}

void
LayerMap::map_expr (tl::Extractor &ex, unsigned int l)
{
  try {

    do {

      tl::Extractor ex_saved = ex;

      std::string name;
      ld_type n;
      if (! ex.try_read (n) && ex.try_read_word_or_quoted (name)) {

        m_name_map.insert (std::make_pair (name, l));

      } else {

        ex = ex_saved;
        ld_interval_vector vl, vd;

        parse_intervals (ex, vl);

        if (ex.test ("/")) {
          parse_intervals (ex, vd);
        } else {
          vd.push_back (ld_interval (0, 0));
        }

        datatype_map dm;
        for (ld_interval_vector::const_iterator di = vd.begin (); di != vd.end (); ++di) {
          LmapJoinOp1 op1;
          dm.add (di->first, di->second + 1, l, op1);
        }
        for (ld_interval_vector::const_iterator li = vl.begin (); li != vl.end (); ++li) {
          LmapJoinOp2 op2;
          m_ld_map.add (li->first, li->second + 1, dm, op2);
        }

      }

    } while (ex.test (";") || ex.test (","));

    if (ex.test (":")) {
      LayerProperties lp;
      lp.read (ex); 
      m_target_layers[l] = lp;
    } 

  } catch (...) {
    throw LayerSpecFormatException (ex.skip ());
  }

  if (l >= m_next_index) {
    m_next_index = l + 1;
  }
}

void
LayerMap::insert (const std::string &name, unsigned int l, const LayerProperties &target) 
{
  if (! (target == LayerProperties ())) {
    m_target_layers[l] = target;
  }

  m_name_map.insert (std::make_pair (name, l));

  if (l >= m_next_index) {
    m_next_index = l + 1;
  }
}

void
LayerMap::insert (const LDPair &p1, const LDPair &p2, unsigned int l, const LayerProperties &target) 
{
  if (! (target == LayerProperties ())) {
    m_target_layers[l] = target;
  }

  //  create a single-interval list for the datatype range
  LayerMap::datatype_map dt;
  LmapJoinOp1 op1;
  dt.add (p1.datatype, p2.datatype + 1, l, op1);

  //  add this to the layers using the special join operator that
  //  combines the datatype intervals
  LmapJoinOp2 op2;
  m_ld_map.add (p1.layer, p2.layer + 1, dt, op2);

  if (l >= m_next_index) {
    m_next_index = l + 1;
  }
}

void
LayerMap::clear ()
{
  m_ld_map.clear ();
  m_name_map.clear ();
  m_target_layers.clear ();
  m_next_index = 0;
}

bool
LayerMap::is_empty () const
{
  return m_name_map.empty () && m_ld_map.begin () == m_ld_map.end ();
}

std::string 
LayerMap::to_string () const
{ 
  std::vector<unsigned int> layers = get_layers ();
  std::ostringstream os;
  os << "layer_map(";
  for (std::vector<unsigned int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
    if (l != layers.begin ()) {
      os << ";";
    }
    os << tl::to_quoted_string (mapping_str (*l));
  }
  os << ")";
  return os.str ();
}

std::string 
LayerMap::to_string_file_format () const
{ 
  std::vector<unsigned int> layers = get_layers ();
  std::ostringstream os;
  for (std::vector<unsigned int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
    os << mapping_str (*l);
    os << "\n";
  }
  return os.str ();
}

db::LayerMap 
LayerMap::from_string_file_format (const std::string &s)
{
  db::LayerMap lm;
  unsigned int l = 0;

  int lnr = 0;

  try {

    std::vector<std::string> lines = tl::split (s, "\n");

    for (std::vector<std::string>::const_iterator line = lines.begin (); line != lines.end (); ++line) {

      ++lnr;

      tl::Extractor ex (line->c_str ());
      if (ex.test ("#") || ex.test ("//")) {
        //  ignore comments
      } else {

        if (! ex.at_end ()) {
          lm.map_expr (ex, l);
          if (ex.test ("#") || ex.test ("//")) {
            //  ignore comments
          } else {
            ex.expect_end ();
          }
          ++l;
        }

      }

    }

  } catch (tl::Exception &ex) {
    throw tl::Exception (ex.msg () + tl::to_string (tr (" in line ")) + tl::to_string (lnr));
  }

  return lm;
}

}

namespace tl
{
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::LayerMap &t)
  {
    t = db::LayerMap ();

    ex.test("layer_map");
    ex.test("(");

    unsigned int l = 0;
    while (! ex.test (")") && ! ex.at_end ()) {
      std::string m;
      ex.read_word_or_quoted (m);
      t.map_expr (m, l);
      ++l;
      ex.test (";");
    }
  }

  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::LayerMap &t)
  {
    t = db::LayerMap ();

    if (!ex.test("layer_map")) {
      return false;
    }

    ex.test("(");

    unsigned int l = 0;
    while (! ex.test (")") && ! ex.at_end ()) {
      std::string m;
      ex.read_word_or_quoted (m);
      t.map_expr (m, l);
      ++l;
      ex.test (";");
    }

    return true;
  }
}

