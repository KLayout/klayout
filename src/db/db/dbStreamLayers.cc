
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
  void operator() (std::set<unsigned int> &a, const std::set<unsigned int> &b)
  {
    a.insert (b.begin (), b.end ());
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

/// A helper class to implement the unmap operation
struct LmapEraseDatatypeInterval
{
  LmapEraseDatatypeInterval (unsigned int dfrom, unsigned int dto)
    : m_dfrom (dfrom), m_dto (dto)
  { }

  void operator() (LayerMap::datatype_map &a, const LayerMap::datatype_map &)
  {
    if (is_static_ld (m_dfrom) && is_static_ld (m_dto)) {
      a.erase (m_dfrom, m_dto + 1);
    } else {
      a.clear ();
    }
  }

private:
  unsigned int m_dfrom, m_dto;
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

bool
LayerMap::is_mapped (const LDPair &p) const
{
  const datatype_map *dm = m_ld_map.mapped (p.layer);
  if (!dm) {
    return false;
  }
  const std::set<unsigned int> *l = dm->mapped (p.datatype);
  return (l && ! l->empty ());
}

bool
LayerMap::is_mapped (const std::string &n) const
{
  std::map<std::string, std::set<unsigned int> >::const_iterator m = m_name_map.find (n);
  return m != m_name_map.end () && ! m->second.empty ();
}

bool
LayerMap::is_mapped (const db::LayerProperties &p) const
{
  std::set<unsigned int> m;
  if (p.layer >= 0 && p.datatype >= 0) {
    if (is_mapped (db::LDPair (p.layer, p.datatype))) {
      return true;
    }
  }
  if (! p.name.empty ()) {
    return is_mapped (p.name);
  } else {
    return false;
  }
}

std::set<unsigned int>
LayerMap::logical (const LDPair &p) const
{
  return logical_internal (p, false);
}

std::set<unsigned int>
LayerMap::logical (const std::string &n) const
{
  return logical_internal (n, false);
}

std::set<unsigned int>
LayerMap::logical (const db::LayerProperties &p) const
{
  return logical_internal (p, false);
}

std::set<unsigned int>
LayerMap::logical_internal (const LDPair &p, bool allow_placeholder) const
{
  const datatype_map *dm = m_ld_map.mapped (p.layer);
  if (dm) {
    const std::set<unsigned int> *l = dm->mapped (p.datatype);
    if (l && (allow_placeholder || ! is_placeholder (*l))) {
      return *l;
    }
  }
  return std::set<unsigned int> ();
}

std::set<unsigned int>
LayerMap::logical_internal (const std::string &n, bool allow_placeholder) const
{
  std::map<std::string, std::set<unsigned int> >::const_iterator m = m_name_map.find (n);
  if (m != m_name_map.end () && (allow_placeholder || ! is_placeholder (m->second))) {
    return m->second;
  } else {
    return std::set<unsigned int> ();
  }
}

std::set<unsigned int>
LayerMap::logical_internal (const db::LayerProperties &p, bool allow_placeholder) const
{
  std::set<unsigned int> m;
  if (p.layer >= 0 && p.datatype >= 0) {
    m = logical_internal (db::LDPair (p.layer, p.datatype), allow_placeholder);
  }
  if (m.empty () && ! p.name.empty ()) {
    m = logical_internal (p.name, allow_placeholder);
  }
  return m;
}

bool
LayerMap::is_placeholder (const std::set<unsigned int> &m) const
{
  for (std::set<unsigned int>::const_iterator i = m.begin (); i != m.end (); ++i) {
    if (m_placeholders.size () > std::numeric_limits<unsigned int>::max () - *i) {
      return true;
    }
  }
  return false;
}

const db::LayerProperties *
LayerMap::target (unsigned int l) const
{
  std::map<unsigned int, LayerProperties>::const_iterator i = m_target_layers.find (l);
  if (i != m_target_layers.end ()) {
    return & i->second;
  } else {
    return 0;
  }
}

std::set<unsigned int>
LayerMap::logical (const db::LayerProperties &p, db::Layout &layout) const
{
  std::set<unsigned int> l = logical_internal (p, true);
  if (is_placeholder (l)) {
    return const_cast<LayerMap *> (this)->substitute_placeholder (p, l, layout);
  } else {
    return l;
  }
}

std::set<unsigned int>
LayerMap::logical (const db::LDPair &p, db::Layout &layout) const
{
  std::set<unsigned int> l = logical_internal (p, true);
  if (is_placeholder (l)) {
    return const_cast<LayerMap *> (this)->substitute_placeholder (db::LayerProperties (p.layer, p.datatype), l, layout);
  } else {
    return l;
  }
}

std::set<unsigned int>
LayerMap::substitute_placeholder (const db::LayerProperties &p, const std::set<unsigned int> &m, db::Layout &layout)
{
  std::set<unsigned int> res;
  for (std::set<unsigned int>::const_iterator i = m.begin (); i != m.end (); ++i) {

    if (m_placeholders.size () > std::numeric_limits<unsigned int>::max () - *i) {

      const db::LayerProperties &lp_ph = m_placeholders [std::numeric_limits<unsigned int>::max () - *i];
      db::LayerProperties lp_new = p;
      lp_new.layer = db::ld_combine (p.layer, lp_ph.layer);
      lp_new.datatype = db::ld_combine (p.datatype, lp_ph.datatype);

      unsigned int l_new = layout.insert_layer (lp_new);
      map (p, l_new, lp_new);
      res.insert (l_new);

    } else {
      res.insert (*i);
    }

  }

  return res;
}

static std::string format_interval (ld_type l1, ld_type l2)
{
  if (l1 == 0 && l2 == std::numeric_limits<ld_type>::max ()) {
    return "*";
  } else if (l2 == std::numeric_limits<ld_type>::max ()) {
    return tl::to_string (l1) + "-*";
  } else if (l1 + 1 < l2) {
    return tl::to_string (l1) + "-" + tl::to_string (l2 - 1);
  } else {
    return tl::to_string (l1);
  }
}

static std::vector<std::pair<ld_type, ld_type> >
extract_dt_intervals (const LayerMap::datatype_map &dt_map, int ll, bool &has_others)
{
  std::vector<std::pair<ld_type, ld_type> > res;

  for (LayerMap::datatype_map::const_iterator d = dt_map.begin (); d != dt_map.end (); ) {

    if (d->second.find (ll) != d->second.end ()) {

      std::pair<ld_type, ld_type> dpi = d->first;

      if (d->second.size () > 1) {
        has_others = true;
      }

      LayerMap::datatype_map::const_iterator dd = d;
      ++dd;
      while (dd != dt_map.end () && dd->first.first == dpi.second && dd->second.find (ll) != dd->second.end ()) {
        if (dd->second.size () > 1) {
          has_others = true;
        }
        dpi.second = dd->first.second;
        ++dd;
      }

      d = dd;

      res.push_back (dpi);

    } else {
      ++d;
    }

  }
  return res;
}

std::string
LayerMap::mapping_str (unsigned int ll) const
{
  std::string s;
  bool f1 = true;
  bool is_mmap = false;

  for (ld_map::const_iterator l = m_ld_map.begin (); l != m_ld_map.end (); ) {

    std::pair<ld_type, ld_type> lti = l->first;

    std::vector<std::pair<ld_type, ld_type> > dti = extract_dt_intervals (l->second, ll, is_mmap);
    ++l;
    while (l != m_ld_map.end () && lti.second == l->first.first && extract_dt_intervals (l->second, ll, is_mmap) == dti) {
      lti.second = l->first.second;
      ++l;
    }

    bool f2 = true;
    for (std::vector<std::pair<ld_type, ld_type> >::const_iterator d = dti.begin (); d != dti.end (); ++d) {

      //  create a string representation
      if (!f2) {
        s += ",";
      } else {

        if (!f1) {
          s += ";";
        }
        f1 = false;

        s += format_interval (lti.first, lti.second);
        s += "/";

      }
      f2 = false;

      s += format_interval (d->first, d->second);

    }
    
  }

  for (std::map <std::string, std::set<unsigned int> >::const_iterator l = m_name_map.begin (); l != m_name_map.end (); ++l) {

    if (l->second.find (ll) != l->second.end ()) {

      if (l->second.size () > 1) {
        is_mmap = true;
      }

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
    s += t->second.to_string (true);
  }

  if (is_mmap) {
    return "+" + s;
  } else {
    return s;
  }
}

void
LayerMap::prepare (db::Layout &layout)
{
  m_placeholders.clear ();
  unsigned int ph = std::numeric_limits<unsigned int>::max ();

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

      db::LayerProperties lp = mapping (*l);
      if (lp.is_named () || (db::is_static_ld (lp.layer) && db::is_static_ld (lp.datatype))) {

        std::pair <bool, unsigned int> lm = layer_mapping.map_layer (lp);
        if (lm.first) {
          real_layers.insert (std::make_pair (*l, lm.second));
          mapped_layers.insert (lm.second);
        }

      } else {

        //  install a placeholder index
        m_placeholders.push_back (lp);
        real_layers.insert (std::make_pair (*l, ph--));

      }

    }

  }

  //  Now remap the indexes
  for (ld_map::iterator l = m_ld_map.begin (); l != m_ld_map.end (); ++l) {
    for (datatype_map::iterator d = l->second.begin (); d != l->second.end (); ++d) {
      std::set<unsigned int> dn;
      for (std::set<unsigned int>::const_iterator i = d->second.begin (); i != d->second.end (); ++i) {
        dn.insert (real_layers [*i]);
      }
      d->second = dn;
    }
  }
  for (std::map<std::string, std::set<unsigned int> >::iterator n = m_name_map.begin (); n != m_name_map.end (); ++n) {
    std::set<unsigned int> dn;
    for (std::set<unsigned int>::const_iterator i = n->second.begin (); i != n->second.end (); ++i) {
      dn.insert (real_layers [*i]);
    }
    n->second = dn;
  }

  std::map<unsigned int, LayerProperties> old_target_layers;
  old_target_layers.swap (m_target_layers);

  for (std::map<unsigned int, LayerProperties>::const_iterator tl = old_target_layers.begin (); tl != old_target_layers.end (); ++tl) {
    m_target_layers[real_layers [tl->first]] = tl->second;
  }

  //  In addition, map other existing layers as well, so merging of layout is somewhat better supported
  for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
    if (! (*l).second->is_null () && mapped_layers.find ((*l).first) == mapped_layers.end ()) {
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
      layers.insert (d->second.begin (), d->second.end ());
    }
  }
  for (const_iterator_names n = m_name_map.begin (); n != m_name_map.end (); ++n) {
    layers.insert(n->second.begin (), n->second.end ());
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

    //  a mapping is given. Use it.
    p = t->second;

    //  special case: if it is a name mapping, add the layer mapping (for backward compatibility)
    if (p.is_named ()) {

      //  no mapping is given. Use the lowest layer and datatype
      for (ld_map::const_iterator l = m_ld_map.begin (); l != m_ld_map.end (); ++l) {
        for (datatype_map::const_iterator d = l->second.begin (); d != l->second.end (); ++d) {
          if (d->second.find (ll) != d->second.end ()) {
            p.layer = l->first.first;
            p.datatype = d->first.first;
            break;
          }
        }
      }

    }

  } else {

    //  no mapping is given. Use the lowest layer and datatype
    for (ld_map::const_iterator l = m_ld_map.begin (); l != m_ld_map.end (); ++l) {
      for (datatype_map::const_iterator d = l->second.begin (); d != l->second.end (); ++d) {
        if (d->second.find (ll) != d->second.end ()) {
          p.layer = l->first.first;
          p.datatype = d->first.first;
          break;
        }
      }
    }

  }

  if (p.name.empty ()) {
    for (std::map <std::string, std::set<unsigned int> >::const_iterator l = m_name_map.begin (); l != m_name_map.end (); ++l) {
      if (l->second.find (ll) != l->second.end ()) {
        p.name = l->first;
        break;
      }
    }
  }

  //  no layer found
  return p;
}

void 
LayerMap::mmap (const LDPair &p, unsigned int l)
{
  insert (p, p, l, (const LayerProperties *) 0);
}

void 
LayerMap::mmap (const std::string &name, unsigned int l)
{
  insert (name, l, (const LayerProperties *) 0);
}

void 
LayerMap::mmap (const LayerProperties &f, unsigned int l)
{
  if (f.name.empty () || is_static_ld (f.layer) || is_static_ld (f.datatype)) {
    mmap (db::LDPair (f.layer, f.datatype), l);
  } 
  if (! f.name.empty ()) {
    mmap (f.name, l);
  }
}

void 
LayerMap::mmap (const LDPair &p, unsigned int l, const LayerProperties &t)
{
  insert (p, p, l, &t);
}

void 
LayerMap::mmap (const std::string &name, unsigned int l, const LayerProperties &t)
{
  insert (name, l, &t);
}

void 
LayerMap::mmap (const LayerProperties &f, unsigned int l, const LayerProperties &t)
{
  if (f.name.empty () || is_static_ld (f.layer) || is_static_ld (f.datatype)) {
    mmap (db::LDPair (f.layer, f.datatype), l, t);
  } 
  if (! f.name.empty ()) {
    mmap (f.name, l, t);
  }
}

void 
LayerMap::mmap (const LDPair &p1, const LDPair &p2, unsigned int l)
{
  insert (p1, p2, l, (const LayerProperties *) 0);
}

void 
LayerMap::mmap (const LDPair &p1, const LDPair &p2, unsigned int l, const LayerProperties &lp)
{
  insert (p1, p2, l, &lp);
}

/// Utility function for the expression parser:  
/// Parse an interval
void
parse_interval (tl::Extractor &ex, ld_interval &p)
{
  ld_type n1 = 0, n2 = 0;

  if (ex.test ("*")) {
    n1 = 0;
    //  NOTE: as the upper limit is stored as n2 + 1, this will map to max():
    n2 = std::numeric_limits<ld_type>::max () - 1;
  } else {
    ex.try_read (n1);
    if (ex.test ("-")) {
      if (ex.test ("*")) {
        //  NOTE: as the upper limit is stored as n2 + 1, this will map to max():
        n2 = std::numeric_limits<ld_type>::max () - 1;
      } else {
        ex.try_read (n2);
      }
    } else {
      n2 = n1;
    }
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
LayerMap::mmap_expr (const std::string &expr, unsigned int l)
{
  tl::Extractor ex (expr.c_str ());
  mmap_expr (ex, l);
  ex.expect_end ();
}

void
LayerMap::mmap_expr (tl::Extractor &ex, unsigned int l)
{
  try {

    bool round_bracket = false, square_bracket = false;
    if (ex.test ("(")) {
      round_bracket = true;
    } else if (ex.test ("[")) {
      square_bracket = true;
    }

    do {

      tl::Extractor ex_saved = ex;

      std::string name;
      ld_type n;
      if (! ex.try_read (n) && ex.try_read_word_or_quoted (name)) {

        m_name_map [name].insert (l);

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
          std::set<unsigned int> single;
          single.insert (l);
          dm.add (di->first, di->second + 1, single, op1);
        }
        for (ld_interval_vector::const_iterator li = vl.begin (); li != vl.end (); ++li) {
          LmapJoinOp2 op2;
          m_ld_map.add (li->first, li->second + 1, dm, op2);
        }

      }

    } while (ex.test (";") || ex.test (","));

    if (ex.test (":")) {
      LayerProperties lp;
      lp.read (ex, true);
      m_target_layers[l] = lp;
    } else if (square_bracket) {
      m_target_layers[l] = LayerProperties (db::any_ld (), db::any_ld ());
    }

    if (round_bracket) {
      ex.expect (")");
    } else if (square_bracket) {
      ex.expect ("]");
    }

  } catch (...) {
    throw LayerSpecFormatException (ex.skip ());
  }

  if (l >= m_next_index) {
    m_next_index = l + 1;
  }
}

void
LayerMap::insert (const std::string &name, unsigned int l, const LayerProperties *target)
{
  if (target) {
    m_target_layers[l] = *target;
  }

  m_name_map [name].insert (l);

  if (l >= m_next_index) {
    m_next_index = l + 1;
  }
}

void
LayerMap::insert (const LDPair &p1, const LDPair &p2, unsigned int l, const LayerProperties *target)
{
  if (target) {
    m_target_layers[l] = *target;
  }

  std::set<unsigned int> single;
  single.insert (l);

  //  create a single-interval list for the datatype range
  LayerMap::datatype_map dt;
  LmapJoinOp1 op1;
  if (db::is_static_ld (p1.datatype) && db::is_static_ld (p2.datatype)) {
    dt.add (p1.datatype, p2.datatype + 1, single, op1);
  } else {
    dt.add (0, std::numeric_limits<ld_type>::max (), single, op1);
  }

  //  add this to the layers using the special join operator that
  //  combines the datatype intervals
  LmapJoinOp2 op2;
  if (db::is_static_ld (p1.layer) && db::is_static_ld (p2.layer)) {
    m_ld_map.add (p1.layer, p2.layer + 1, dt, op2);
  } else {
    m_ld_map.add (0, std::numeric_limits<ld_type>::max (), dt, op2);
  }

  if (l >= m_next_index) {
    m_next_index = l + 1;
  }
}

void
LayerMap::unmap (const LDPair &f)
{
  unmap (f, f);
}

void
LayerMap::unmap (const std::string &name)
{
  m_name_map.erase (name);
}

void
LayerMap::unmap (const LayerProperties &f)
{
  if (f.name.empty () || is_static_ld (f.layer) || is_static_ld (f.datatype)) {
    unmap (db::LDPair (f.layer, f.datatype));
  }
  if (! f.name.empty ()) {
    unmap (f.name);
  }
}



void
LayerMap::unmap (const LDPair &p1, const LDPair &p2)
{
  if (m_ld_map.begin () == m_ld_map.end ()) {
    return;
  }

  LmapEraseDatatypeInterval op (p1.datatype, p2.datatype);
  if (db::is_static_ld (p1.layer) && db::is_static_ld (p2.layer)) {
    m_ld_map.add (p1.layer, p2.layer + 1, LayerMap::datatype_map (), op);
  } else {
    m_ld_map.add (m_ld_map.begin ()->first.first, (--m_ld_map.end ())->first.second, LayerMap::datatype_map (), op);
  }
}


void
LayerMap::unmap_expr (const std::string &expr)
{
  tl::Extractor ex (expr.c_str ());
  unmap_expr (ex);
  ex.expect_end ();
}

void
LayerMap::unmap_expr (tl::Extractor &ex)
{
  try {

    bool round_bracket = false, square_bracket = false;
    if (ex.test ("(")) {
      round_bracket = true;
    } else if (ex.test ("[")) {
      square_bracket = true;
    }

    do {

      tl::Extractor ex_saved = ex;

      std::string name;
      ld_type n;
      if (! ex.try_read (n) && ex.try_read_word_or_quoted (name)) {

        m_name_map.erase (name);

      } else {

        ex = ex_saved;
        ld_interval_vector vl, vd;

        parse_intervals (ex, vl);

        if (ex.test ("/")) {
          parse_intervals (ex, vd);
        } else {
          vd.push_back (ld_interval (0, 0));
        }

        for (ld_interval_vector::const_iterator li = vl.begin (); li != vl.end (); ++li) {
          for (ld_interval_vector::const_iterator di = vd.begin (); di != vd.end (); ++di) {
            unmap (LDPair (li->first, di->first), LDPair (li->second, di->second));
          }
        }

      }

    } while (ex.test (";") || ex.test (","));

    if (ex.test (":")) {
      //  ignore target layers
      LayerProperties lp;
      lp.read (ex, true);
    }

    if (round_bracket) {
      ex.expect (")");
    } else if (square_bracket) {
      ex.expect ("]");
    }

  } catch (...) {
    throw LayerSpecFormatException (ex.skip ());
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

void
LayerMap::add_expr (const std::string &expr, unsigned int l)
{
  tl::Extractor ex (expr.c_str ());
  add_expr (ex, l);
  ex.expect_end ();
}

void
LayerMap::add_expr (tl::Extractor &ex, unsigned int l)
{
  if (ex.test ("+")) {
    mmap_expr (ex, l);
  } else if (ex.test ("-")) {
    unmap_expr (ex);
  } else {
    map_expr (ex, l);
  }
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
          lm.add_expr (ex, l);
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
      t.add_expr (m, l);
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
      t.add_expr (m, l);
      ++l;
      ex.test (";");
    }

    return true;
  }
}

