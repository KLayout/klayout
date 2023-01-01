
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


#include "layParsedLayerSource.h"
#include "layLayoutViewBase.h"
#include "tlString.h"
#include "tlGlobPattern.h"

#include <limits>

namespace lay
{

static PropertySelectorBase *extract_top (tl::Extractor &ex);

// --------------------------------------------------------------------------
//  PropertySelectorBase definition & implementation

/**
 *  @brief A base class for the expression graph nodes
 */
class PropertySelectorBase
{
public:
  PropertySelectorBase () { }
  virtual ~PropertySelectorBase () { }

  virtual std::string to_string (bool inner, size_t max_len) const = 0;
  virtual PropertySelectorBase *clone () const = 0;
  virtual int compare (const PropertySelectorBase *b) const = 0;
  virtual unsigned int type_id () const = 0;
  virtual bool check (const db::PropertiesRepository &rep, const db::PropertiesRepository::properties_set &set) const = 0;
  virtual bool selection (const db::PropertiesRepository &rep, std::set<db::properties_id_type> &ids) const = 0;
};

/**
 *  @brief A expression graph node combining n arguments with either a "and" or "or" operation
 */
class PropertySelectorOp 
  : public PropertySelectorBase
{
public:
  enum op_type_enum { And, Or };

  PropertySelectorOp (op_type_enum op, const PropertySelectorBase *arg)
    : m_op (op)
  {
    m_args.push_back (arg);
  }

  PropertySelectorOp (const PropertySelectorOp &d)
    : m_op (d.m_op)
  {
    m_args.reserve (d.m_args.size ());
    for (std::vector<const PropertySelectorBase *>::const_iterator b = d.m_args.begin (); b != d.m_args.end (); ++b) {
      m_args.push_back ((*b)->clone ());
    }
  }

  ~PropertySelectorOp ()
  {
    for (std::vector<const PropertySelectorBase *>::iterator b = m_args.begin (); b != m_args.end (); ++b) {
      delete const_cast<PropertySelectorBase *> (*b);
    }
    m_args.clear ();
  }

  virtual std::string to_string (bool inner, size_t max_len) const
  {
    std::string s;
    if (inner) {
      s += "(";
    }
    s += m_args [0]->to_string (true, max_len);
    for (std::vector<const PropertySelectorBase *>::const_iterator b = m_args.begin () + 1; b != m_args.end (); ++b) {
      s += (m_op == And) ? "&&" : "||";
      if (s.size () > max_len) {
        s += "...";
        break;
      } 
      s += (*b)->to_string (true, max_len);
    }
    if (inner) {
      s += ")";
    }
    return s;
  }

  void add_arg (const PropertySelectorBase *arg)
  {
    m_args.push_back (arg);
  }

  PropertySelectorBase *clone () const 
  {
    return new PropertySelectorOp (*this);
  }

  void join (const PropertySelectorBase *other)
  {
    const PropertySelectorOp *op = dynamic_cast<const PropertySelectorOp *> (other);
    if (op && op->op_type () == And) {
      m_args.reserve (m_args.size () + op->m_args.size ());
      for (std::vector<const PropertySelectorBase *>::const_iterator b = op->m_args.begin (); b != op->m_args.end (); ++b) {
        m_args.push_back ((*b)->clone ());
      }
    } else {
      m_args.push_back (other);
    }
  }

  op_type_enum op_type () const
  {
    return m_op;
  }

  bool check (const db::PropertiesRepository &rep, const db::PropertiesRepository::properties_set &set) const 
  {
    if (m_op == And) {
      for (std::vector<const PropertySelectorBase *>::const_iterator b = m_args.begin (); b != m_args.end (); ++b) {
        if (! (*b)->check (rep, set)) {
          return false;
        }
      }
      return true;
    } else {
      for (std::vector<const PropertySelectorBase *>::const_iterator b = m_args.begin (); b != m_args.end (); ++b) {
        if ((*b)->check (rep, set)) {
          return true;
        }
      }
      return false;
    }
  }

  bool selection (const db::PropertiesRepository &rep, std::set<db::properties_id_type> &ids) const 
  {
    //  this algorithm computes the "or" of two sets by using this relationship: a or b or c or .. = !((!a) and (!b) and (!c) and ..)

    //  get the selection of the first operand into ids
    std::vector<const PropertySelectorBase *>::const_iterator b = m_args.begin ();
    bool inv = (*b)->selection (rep, ids);
    if (m_op == Or) {
      inv = !inv;
    }

    for (++b; b != m_args.end () && !(ids.empty () && !inv); ++b) {

      //  get the selection of the next operand into ids2
      std::set<db::properties_id_type> ids2;
      bool inv2 = (*b)->selection (rep, ids2);
      if (m_op == Or) {
        inv2 = !inv2;
      }

      //  compute the intersection of ids and ids2 in place int ids
      if (ids2.empty () && !inv2) {
        //  shortcut: if the second operand is empty, just clear and terminate the loop then
        ids.clear ();
        inv = false;
      } else if (!inv && !inv2) {
        for (std::set<db::properties_id_type>::iterator id = ids.begin (); id != ids.end (); ) {
          std::set<db::properties_id_type>::iterator i = id;
          ++id;
          if (ids2.find (*i) == ids2.end ()) {
            ids.erase (i);
          }
        }
      } else if (inv && inv2) {
        for (std::set<db::properties_id_type>::iterator id = ids2.begin (); id != ids2.end (); ++id) {
          ids.insert (*id);
        }
      } else {
        //  swap current and new ids such that inv==false
        if (inv) {
          std::swap (inv, inv2);
          ids.swap (ids2);
        }
        //  from ids subtract all ids that are in ids2 (inv2==true!)
        for (std::set<db::properties_id_type>::iterator id = ids.begin (); id != ids.end (); ) {
          std::set<db::properties_id_type>::iterator i = id;
          ++id;
          if (ids2.find (*i) != ids2.end ()) {
            ids.erase (i);
          }
        }
      }

    }

    return m_op == Or ? !inv : inv;
  }

  unsigned int type_id () const 
  {
    return m_op == And ? 1 : 2; 
  }

  int compare (const PropertySelectorBase *b) const 
  {
    if (type_id () != b->type_id ()) {
      return type_id () < b->type_id () ? -1 : 1;
    }
    const PropertySelectorOp *bb = dynamic_cast<const PropertySelectorOp *> (b);
    if (bb) {
      if (m_args.size () != bb->m_args.size ()) {
        return (m_args.size () < bb->m_args.size ()) ? -1 : 1;
      }
      for (size_t n = 0; n < m_args.size (); ++n) {
        int cmp = m_args[n]->compare (bb->m_args[n]);
        if (cmp != 0) {
          return cmp;
        }
      }
    }
    return 0;
  }

private:
  op_type_enum m_op;
  std::vector<const PropertySelectorBase *> m_args;
};

/**
 *  @brief A expression graph node forming the inverse of one argument
 */
class PropertySelectorNot 
  : public PropertySelectorBase
{
public:
  PropertySelectorNot (PropertySelectorBase *arg)
    : mp_arg (arg)
  {
    //  .. nothing yet ..
  }

  ~PropertySelectorNot ()
  {
    delete mp_arg;
    mp_arg = 0;
  }

  virtual std::string to_string (bool /*inner*/, size_t max_len) const
  {
    return "!(" + mp_arg->to_string (false, max_len) + ")";
  }

  PropertySelectorBase *clone () const 
  {
    return new PropertySelectorNot (mp_arg->clone ());
  }

  bool check (const db::PropertiesRepository &rep, const db::PropertiesRepository::properties_set &set) const 
  {
    return ! mp_arg->check (rep, set);
  }

  bool selection (const db::PropertiesRepository &rep, std::set<db::properties_id_type> &ids) const 
  {
    return ! mp_arg->selection (rep, ids);
  }

  unsigned int type_id () const 
  {
    return 10;
  }

  int compare (const PropertySelectorBase *b) const 
  {
    if (type_id () != b->type_id ()) {
      return type_id () < b->type_id () ? -1 : 1;
    }
    const PropertySelectorNot *bb = dynamic_cast<const PropertySelectorNot *> (b);
    return bb ? mp_arg->compare (bb->mp_arg) : 0;
  }

private:
  PropertySelectorBase *mp_arg;
};

/**
 *  @brief A expression graph leaf node: a comparison operation
 */
class PropertySelectorEqual 
  : public PropertySelectorBase
{
public:
  PropertySelectorEqual (const tl::Variant &name, const tl::Variant &value, bool equal)
    : m_name (name), m_value (value), m_equal (equal)
  {
    //  .. nothing yet ..
  }

  ~PropertySelectorEqual ()
  {
    //  .. nothing yet ..
  }

  virtual std::string to_string (bool /*inner*/, size_t /*max_len*/) const
  {
    std::string s = m_name.to_parsable_string ();
    if (m_equal) {
      s += "==";
    } else {
      s += "!=";
    }
    s += m_value.to_parsable_string ();
    return s;
  }

  PropertySelectorBase *clone () const 
  {
    return new PropertySelectorEqual (m_name, m_value, m_equal);
  }

  bool check (const db::PropertiesRepository &rep, const db::PropertiesRepository::properties_set &set) const 
  {
    std::pair<bool, db::property_names_id_type> p = rep.get_id_of_name (m_name);
    if (! p.first) {
      //  name is not known at all.
      return false;
    }

    db::PropertiesRepository::properties_set::const_iterator i = set.find (p.second);
    if (i == set.end ()) {
      //  name is not present in the property set
      return false;
    } else {
      //  check value
      if (m_equal && i->second == m_value) {
        return true;
      } else if (! m_equal && i->second != m_value) {
        return true;
      } else {
        return false;
      }
    }
  }

  bool selection (const db::PropertiesRepository &rep, std::set<db::properties_id_type> &ids) const 
  {
    std::pair<bool, db::property_names_id_type> p = rep.get_id_of_name (m_name);
    if (! p.first) {
      //  name is not known at all.
      return false;
    }

    const db::PropertiesRepository::properties_id_vector &idv = rep.properties_ids_by_name_value (std::make_pair (p.second, m_value));
    for (db::PropertiesRepository::properties_id_vector::const_iterator id = idv.begin (); id != idv.end (); ++id) {
      ids.insert (*id);
    }

    return ! m_equal;
  }

  unsigned int type_id () const 
  {
    return m_equal ? 20 : 21;
  }

  int compare (const PropertySelectorBase *b) const 
  {
    if (type_id () != b->type_id ()) {
      return type_id () < b->type_id () ? -1 : 1;
    }
    const PropertySelectorEqual *bb = dynamic_cast<const PropertySelectorEqual *> (b);
    if (bb) {
      if (m_name != bb->m_name) {
        return m_name < bb->m_name ? -1 : 1;
      }
      if (m_value != bb->m_value) {
        return m_value < bb->m_value ? -1 : 1;
      }
    }
    return 0;
  }

private:
  tl::Variant m_name, m_value;
  bool m_equal;
};


/**
 *  @brief Expression parser: extract one comparison operation
 */
static PropertySelectorBase *
extract_base (tl::Extractor &ex)
{
  bool eq = true;
  tl::Variant n, v;
  ex.read (n);
  if (ex.test ("==")) {
    eq = true;
  } else if (ex.test ("!=")) {
    eq = false;
  } else {
    ex.error (tl::to_string (tr ("'==' or '!=' operator expected")));
  }
  ex.read (v);
  return new PropertySelectorEqual (n, v, eq);
}

/**
 *  @brief Expression parser: parse complex elements (bracketed expressions)
 */
static PropertySelectorBase *
extract_element (tl::Extractor &ex)
{
  if (ex.test ("(")) {
    PropertySelectorBase *expr = extract_top (ex);
    ex.expect (")");
    return expr;
  } else {
    return extract_base (ex);
  }
}

/**
 *  @brief Expression parser: parse unary operations with one argument
 */
static PropertySelectorBase *
extract_unary (tl::Extractor &ex)
{
  if (ex.test ("!")) {
    return new PropertySelectorNot (extract_unary (ex));
  } else {
    return extract_element (ex);
  }
}

/**
 *  @brief Expression parser: parse binary operations at level 2
 */
static PropertySelectorBase *
extract_or_seq (tl::Extractor &ex)
{
  PropertySelectorBase *expr = extract_unary (ex);
  if (ex.test ("||")) {
    PropertySelectorOp *op = new PropertySelectorOp (PropertySelectorOp::Or, expr);
    expr = op;
    do {
      op->add_arg (extract_unary (ex));
    } while (ex.test ("||"));
  }
  return expr;
}

/**
 *  @brief Expression parser: parse binary operations at level 1
 */
static PropertySelectorBase *
extract_and_seq (tl::Extractor &ex)
{
  PropertySelectorBase *expr = extract_or_seq (ex);
  if (ex.test ("&&")) {
    PropertySelectorOp *op = new PropertySelectorOp (PropertySelectorOp::And, expr);
    expr = op;
    do {
      op->add_arg (extract_or_seq (ex));
    } while (ex.test ("&&"));
  }
  return expr;
}

/**
 *  @brief Expression parser: parse top level expressions
 */
static PropertySelectorBase *
extract_top (tl::Extractor &ex)
{
  return extract_and_seq (ex);
}

// --------------------------------------------------------------------------
//  PropertySelector implementation

PropertySelector::PropertySelector ()
  : mp_base (0)
{
  //  .. nothing yet ..
}

PropertySelector::PropertySelector (const PropertySelector &sel)
  : mp_base (0)
{
  operator= (sel);
}

PropertySelector &
PropertySelector::operator= (const PropertySelector &sel)
{
  if (this != &sel) {
    if (mp_base) {
      delete mp_base;
      mp_base = 0;
    }
    if (sel.mp_base) {
      mp_base = sel.mp_base->clone ();
    }
  }
  return *this;
}

bool 
PropertySelector::operator== (const PropertySelector &sel) const
{
  if (mp_base == 0 && sel.mp_base == 0) {
    return true;
  } else if (mp_base && sel.mp_base) {
    return mp_base->compare (sel.mp_base) == 0;
  } else {
    return false;
  }
}

bool 
PropertySelector::operator< (const PropertySelector &sel) const
{
  if (mp_base == 0 && sel.mp_base == 0) {
    return false;
  } else if (mp_base && sel.mp_base) {
    return mp_base->compare (sel.mp_base) < 0;
  } else {
    return mp_base == 0;
  }
}

PropertySelector::~PropertySelector ()
{
  if (mp_base) {
    delete mp_base;
  }
  mp_base = 0;
}

void 
PropertySelector::extract (tl::Extractor &ex)
{
  if (mp_base) {
    delete mp_base;
  }
  mp_base = extract_top (ex);
}

std::string 
PropertySelector::to_string (size_t max_len) const
{
  if (mp_base) {
    return mp_base->to_string (false, max_len);
  } else {
    return std::string ();
  }
}

void
PropertySelector::join (const PropertySelector &d)
{
  //  Create a combined "and" operator of both property selectors
  if (d.mp_base) {
    if (! mp_base) {
      mp_base = d.mp_base->clone ();
    } else {
      PropertySelectorOp *op = dynamic_cast<PropertySelectorOp *> (mp_base);
      if (op && op->op_type () == PropertySelectorOp::And) {
        op->join (d.mp_base);
      } else {
        op = new PropertySelectorOp (PropertySelectorOp::And, mp_base);
        op->add_arg (d.mp_base->clone ());
        mp_base = op;
      }
    }
  }
}

bool 
PropertySelector::check (const db::PropertiesRepository &rep, db::properties_id_type id) const
{
  if (is_null ()) {
    return true;
  } else {
    return mp_base->check (rep, rep.properties (id));
  }
}

bool 
PropertySelector::matching (const db::PropertiesRepository &rep, std::set<db::properties_id_type> &ids) const
{
  if (is_null ()) {
    return true;
  } else {
    return mp_base->selection (rep, ids);
  }
}

// --------------------------------------------------------------------------
//  PartialTreeSelector implementation

PartialTreeSelector::PartialTreeSelector ()
  : mp_layout (0), m_state (0), m_selected (false)
{
  //  .. nothing yet ..
}

PartialTreeSelector::PartialTreeSelector (const db::Layout &layout, bool initially_selected)
  : mp_layout (&layout), m_state (0), m_selected (initially_selected)
{
  //  .. nothing yet ..
}

PartialTreeSelector::PartialTreeSelector (const PartialTreeSelector &d)
  : mp_layout (d.mp_layout), 
    m_state (d.m_state),
    m_selected (d.m_selected),
    m_state_stack (d.m_state_stack),
    m_selected_stack (d.m_selected_stack),
    m_state_machine (d.m_state_machine)
{
  //  .. nothing yet ..
}

PartialTreeSelector &PartialTreeSelector::operator= (const PartialTreeSelector &d)
{
  if (this != &d) {
    mp_layout = d.mp_layout;
    m_state = d.m_state;
    m_selected = d.m_selected;
    m_state_stack = d.m_state_stack;
    m_selected_stack = d.m_selected_stack;
    m_state_machine = d.m_state_machine;
  }
  return *this;
}

int PartialTreeSelector::is_child_selected (db::cell_index_type child) const
{
  if (m_state >= 0 && m_state < int (m_state_machine.size ())) {

    const std::map <db::cell_index_type, std::pair<int, int> > &m = m_state_machine [m_state];

    std::map <db::cell_index_type, std::pair<int, int> >::const_iterator i = m.find (child);
    if (i == m.end ()) {
      i = m.find (db::cell_index_type (-1));
    }

    if (i != m.end ()) {
      bool sel = i->second.second >= 0 ? i->second.second : m_selected;
      if (i->second.first < 0 || i->second.first >= int (m_state_machine.size ())) {
        return sel ? 1 : 0;
      } else {
        return sel ? 1 : -1;
      }
    }

  }

  return m_selected ? 1 : 0;
}

void PartialTreeSelector::descend (db::cell_index_type child)
{
  if (m_state_machine.empty ()) {
    return;
  }

  m_state_stack.push_back (m_state);
  m_selected_stack.push_back (m_selected);

  if (m_state >= 0 && m_state < int (m_state_machine.size ())) {

    const std::map <db::cell_index_type, std::pair<int, int> > &m = m_state_machine [m_state];

    std::map <db::cell_index_type, std::pair<int, int> >::const_iterator i = m.find (child);
    if (i == m.end ()) {
      i = m.find (db::cell_index_type (-1));
    }

    if (i != m.end ()) {
      m_state = i->second.first;
      if (i->second.second >= 0) {
        m_selected = i->second.second;
      }
    }

  }
}

void PartialTreeSelector::ascend ()
{
  if (m_state_machine.empty ()) {
    return;
  }

  if (! m_state_stack.empty ()) {
    m_state = m_state_stack.back ();
    m_state_stack.pop_back ();
    m_selected = m_selected_stack.back ();
    m_selected_stack.pop_back ();
  }
}

void PartialTreeSelector::add_state_transition (int initial_state, db::cell_index_type cell_index, int target_state, int selected)
{
  if (initial_state >= 0) {
    while (int (m_state_machine.size ()) <= initial_state) {
      m_state_machine.push_back (std::map <db::cell_index_type, std::pair<int, int> > ());
    }
    m_state_machine [initial_state][cell_index] = std::make_pair (target_state, selected);
  }
}

void PartialTreeSelector::add_state_transition (int initial_state, int target_state, int selected)
{
  if (initial_state >= 0) {
    while (int (m_state_machine.size ()) <= initial_state) {
      m_state_machine.push_back (std::map <db::cell_index_type, std::pair<int, int> > ());
    }
    m_state_machine [initial_state].clear ();
    m_state_machine [initial_state][db::cell_index_type (-1)] = std::make_pair (target_state, selected);
  }
}

// --------------------------------------------------------------------------
//  CellSelector implementation

CellSelector::CellSelector ()
{
  //  .. nothing yet ..
}

CellSelector::CellSelector (const CellSelector &d)
  : m_selectors (d.m_selectors)
{
  //  .. nothing yet ..
}

CellSelector &CellSelector::operator= (const CellSelector &d)
{
  if (this != &d) {
    m_selectors = d.m_selectors;
  }
  return *this;
}

bool CellSelector::operator== (const CellSelector &d) const
{
  return m_selectors == d.m_selectors;
}

bool CellSelector::operator< (const CellSelector &d) const
{
  return m_selectors < d.m_selectors;
}

static std::pair <bool, std::string> parse_part (tl::Extractor &ex)
{
  bool sel = true;
  if (ex.test ("-")) {
    sel = false;
  } else if (ex.test ("+")) {
    sel = true;
  }

  std::string nf;
  if (ex.try_read_word_or_quoted (nf, "_.$*?[]")) {
    return std::make_pair (sel, nf);
  } else {
    return std::pair <bool, std::string> ();
  }
}

static std::vector <std::pair <bool, std::string> > parse_list (tl::Extractor &ex)
{
  std::vector <std::pair <bool, std::string> > list;

  if (ex.test ("(")) {

    while (! ex.test (")")) {
      list.push_back (parse_part (ex));
      if (list.back () == std::pair <bool, std::string> ()) {
        list.pop_back ();
        ex.expect (")");
        break;
      }
    }

  } else {
    list.push_back (parse_part (ex));
    if (list.back () == std::pair <bool, std::string> ()) {
      list.pop_back ();
    }
  }

  return list;
}

void CellSelector::parse (tl::Extractor &ex)
{
  m_selectors.clear ();

  while (! ex.at_end ()) {
    m_selectors.push_back (parse_list (ex));
    if (m_selectors.back ().empty ()) {
      m_selectors.pop_back ();
      break;
    }
  }
}

std::string CellSelector::to_string () const
{
  std::string r;

  for (std::vector <std::vector <std::pair <bool, std::string> > >::const_iterator i = m_selectors.begin (); i != m_selectors.end (); ++i) {

    if (! r.empty ()) {
      r += " ";
    }

    if (i->size () > 1) {
      r += "(";
    }

    for (std::vector <std::pair <bool, std::string> >::const_iterator j = i->begin (); j != i->end (); ++j) {
      if (j != i->begin ()) {
        r += " ";
      }
      r += j->first ? "+" : "-";
      r += tl::to_word_or_quoted_string (j->second, "_.$*?[]");
    }

    if (i->size () > 1) {
      r += ")";
    }

  }
  
  return r;
}

PartialTreeSelector CellSelector::create_tree_selector (const db::Layout &layout, db::cell_index_type initial_cell) const
{
  //  start in deselected state if the front selector is selecting ("+ABC")
  bool initial_sel = true;
  if (! m_selectors.empty () && ! m_selectors.front ().empty () && m_selectors.front ().front ().first) {
    initial_sel = false;
  }

  //  if the first level matches the initial cell, use the selection state to enable this cell
  bool consume_first = false;
  if (! m_selectors.empty () && layout.is_valid_cell_index (initial_cell)) {
    for (std::vector <std::pair <bool, std::string> >::const_iterator j = m_selectors.front ().begin (); j != m_selectors.front ().end (); ++j) {
      tl::GlobPattern pat (j->second);
      if (pat.match (layout.cell_name (initial_cell))) {
        initial_sel = j->first;
        consume_first = true;
      }
    }
  }

  PartialTreeSelector pts (layout, initial_sel);

  int state = 0;

  for (std::vector <std::vector <std::pair <bool, std::string> > >::const_iterator i = m_selectors.begin (); i != m_selectors.end (); ++i) {

    //  The first level is consumed by the initial cell
    if (i == m_selectors.begin () && consume_first) {
      continue;
    }

    //  default loop for any other cell
    pts.add_state_transition (state, state, -1);

    for (std::vector <std::pair <bool, std::string> >::const_iterator j = i->begin (); j != i->end (); ++j) {

      if (j->second == "*") {

        //  global select/deselect
        pts.add_state_transition (state, state + 1, j->first ? 1 : 0);

      } else {

        //  named select/deselect
        tl::GlobPattern pat (j->second);
        for (db::cell_index_type ci = 0; ci < layout.cells (); ++ci) {
          if (layout.is_valid_cell_index (ci) && pat.match (layout.cell_name (ci))) {
            pts.add_state_transition (state, ci, state + 1, j->first ? 1 : 0);
          }
        }

      }

    }

    ++state;

  }

  return pts;
}

// --------------------------------------------------------------------------
//  ParsedLayerSource implementation

ParsedLayerSource::ParsedLayerSource (const std::string &src)
  : m_has_name (false), m_special_purpose (SP_None), m_layer_index (-1), m_layer (-1), m_datatype (-1), m_cv_index (-1)
{
  parse_from_string (src.c_str ());
}

ParsedLayerSource::ParsedLayerSource (const ParsedLayerSource &d)
  : m_has_name (false), m_special_purpose (SP_None), m_layer_index (-1), m_layer (-1), m_datatype (-1), m_cv_index (-1)
{
  operator= (d);
}

ParsedLayerSource::ParsedLayerSource (const db::LayerProperties &lp, int cv_index)
  : m_has_name (! lp.name.empty ()), m_special_purpose (SP_None), m_layer_index (-1), m_layer (lp.layer), m_datatype (lp.datatype), m_name (lp.name), m_cv_index (cv_index)
{
  m_trans.push_back (db::DCplxTrans ());
}

ParsedLayerSource::ParsedLayerSource (int layer, int datatype, int cv_index)
  : m_has_name (false), m_special_purpose (SP_None), m_layer_index (-1), m_layer (layer), m_datatype (datatype), m_cv_index (cv_index)
{
  m_trans.push_back (db::DCplxTrans ());
}

ParsedLayerSource::ParsedLayerSource (int layer_index, int cv_index)
  : m_has_name (false), m_special_purpose (SP_None), m_layer_index (layer_index), m_layer (-1), m_datatype (-1), m_cv_index (cv_index)
{
  m_trans.push_back (db::DCplxTrans ());
}

ParsedLayerSource::ParsedLayerSource (const std::string &name, int cv_index)
  : m_has_name (true), m_special_purpose (SP_None), m_layer_index (-1), m_layer (-1), m_datatype (-1), m_name (name), m_cv_index (cv_index)
{
  m_trans.push_back (db::DCplxTrans ());
}

ParsedLayerSource::ParsedLayerSource ()
  : m_has_name (false), m_special_purpose (SP_None), m_layer_index (-1), m_layer (-1), m_datatype (-1), m_cv_index (-1)
{
  m_trans.push_back (db::DCplxTrans ());
}

ParsedLayerSource &
ParsedLayerSource::operator= (const ParsedLayerSource &d)
{
  if (this != &d) {
    m_has_name          = d.m_has_name;
    m_special_purpose   = d.m_special_purpose;
    m_layer_index       = d.m_layer_index;
    m_layer             = d.m_layer;
    m_datatype          = d.m_datatype;
    m_name              = d.m_name;
    m_cv_index          = d.m_cv_index;
    m_trans             = d.m_trans;
    m_property_sel      = d.m_property_sel;
    m_cell_sel          = d.m_cell_sel;
    m_hier_levels       = d.m_hier_levels;
  }
  return *this;
}

ParsedLayerSource &
ParsedLayerSource::operator+= (const ParsedLayerSource &d)
{
  if (m_layer_index < 0) {
    m_layer_index = d.m_layer_index;
  }

  //  attempt a mixture of ours and the other's properties
  if (m_special_purpose == SP_None) {
    m_special_purpose = d.m_special_purpose;
  }

  if (m_layer < 0) {
    m_layer = d.m_layer;
  }
  if (m_datatype < 0) {
    m_datatype = d.m_datatype;
  }
  if (! m_has_name) {
    m_name = d.m_name;
    m_has_name = d.m_has_name;
  }

  if (m_cv_index < 0) {
    m_cv_index = d.m_cv_index;
  }

  if (m_cell_sel.is_empty ()) {
    m_cell_sel = d.m_cell_sel;
  }

  m_property_sel.join (d.m_property_sel);

  std::vector<db::DCplxTrans> tr;
  tr.reserve (m_trans.size () * d.m_trans.size ());
  for (std::vector<db::DCplxTrans>::const_iterator a = m_trans.begin (); a != m_trans.end (); ++a) {
    for (std::vector<db::DCplxTrans>::const_iterator b = d.m_trans.begin (); b != d.m_trans.end (); ++b) {
      tr.push_back (*a * *b);
    }
  }
  m_trans.swap (tr);

  m_hier_levels = m_hier_levels.combine (d.m_hier_levels);

  return *this;
}

static std::string 
hier_levels_to_string (const HierarchyLevelSelection &hier_levels)
{
  std::string r;

  if (hier_levels.has_from_level ()) {
    std::string m;
    if (hier_levels.from_level_mode () == HierarchyLevelSelection::minimum) {
      m = "<";
    } else if (hier_levels.from_level_mode () == HierarchyLevelSelection::maximum) {
      m = ">";
    }
    if (hier_levels.from_level_relative ()) {
      r += "(" + m + tl::to_string (hier_levels.from_level ()) + ")";
    } else {
      r += m + tl::to_string (hier_levels.from_level ());
    }
  }
  r += "..";
  if (hier_levels.has_to_level ()) {
    std::string m;
    if (hier_levels.to_level_mode () == HierarchyLevelSelection::minimum) {
      m = "<";
    } else if (hier_levels.to_level_mode () == HierarchyLevelSelection::maximum) {
      m = ">";
    }
    if (hier_levels.to_level () == std::numeric_limits<int>::max ()) {
      r += m + "*";
    } else if (hier_levels.to_level_relative ()) {
      r += "(" + m + tl::to_string (hier_levels.to_level ()) + ")";
    } else {
      r += m + tl::to_string (hier_levels.to_level ());
    }
  }

  return r;
}

std::string 
ParsedLayerSource::to_string () const
{
  std::string r;
  
  if (m_layer_index >= 0) {
    if (! r.empty ()) {
      r += " ";
    }
    r += tl::sprintf ("%%%d", m_layer_index);
  } else {
    //  the normal source specification is either
    //   <name>          - name specification only
    //   <l>/<d>         - layer/datatype
    //   <name> <l>/<d>  - name plus layer/datatype
    if (m_has_name) {
      if (! r.empty ()) {
        r += " ";
      }
      r += tl::to_word_or_quoted_string (m_name);
    } else if (m_layer < 0 && m_datatype < 0 && m_special_purpose == SP_None) {
      if (! r.empty ()) {
        r += " ";
      }
      r += "*/*";
    }
    if (m_layer >= 0 || m_datatype >= 0) {
      if (! r.empty ()) {
        r += " ";
      }
      if (m_layer < 0) {
        r += tl::sprintf ("*/%d", m_datatype);
      } else if (m_datatype < 0) {
        r += tl::sprintf ("%d/*", m_layer);
      } else {
        r += tl::sprintf ("%d/%d", m_layer, m_datatype);
      }
    }
  }
  if (m_cv_index >= 0) {
    r += tl::sprintf ("@%d", m_cv_index + 1);
  } else {
    r += "@*";
  }

  switch (m_special_purpose) {
  case SP_None:
    break;
  case SP_CellFrame:
    if (! r.empty ()) {
      r += " ";
    }
    r += "!CellFrame";
    break;
  }

  if (! m_cell_sel.is_empty ()) {
    if (! r.empty ()) {
      r += " ";
    }
    r += "{";
    r += m_cell_sel.to_string ();
    r += "}";
  }

  if (! m_trans.empty () && (m_trans.size () > 1 || m_trans[0] != db::DCplxTrans ())) {
    for (std::vector<db::DCplxTrans>::const_iterator t = m_trans.begin (); t != m_trans.end (); ++t) {
      if (! r.empty ()) {
        r += " ";
      }
      r += "(";
      r += t->to_string ();
      r += ")";
    }
  }

  if (! m_property_sel.is_null ()) {
    if (! r.empty ()) {
      r += " ";
    }
    r += "[" + m_property_sel.to_string (std::numeric_limits<size_t>::max ()) + "]";
  }

  if (m_hier_levels.has_from_level () || m_hier_levels.has_to_level ()) {
    if (! r.empty ()) {
      r += " ";
    }
    r += "#" + hier_levels_to_string (m_hier_levels);
  }

  return r;
}

std::string
ParsedLayerSource::display_string (const lay::LayoutViewBase *view) const
{
  std::string r;

  if (m_layer_index >= 0) {

    if (!view || m_cv_index < 0 || m_cv_index >= int (view->cellviews ()) || ! view->cellview (m_cv_index)->layout ().is_valid_layer (m_layer_index)) {
      r = tl::sprintf ("%%%d", m_layer_index);
    } else {
      const db::LayerProperties &lp = view->cellview (m_cv_index)->layout ().get_properties (m_layer_index);
      if (! lp.name.empty ()) {
        r = lp.name;
        if (lp.layer >= 0 && lp.datatype >= 0 && view->always_show_ld ()) {
          r += tl::sprintf (" %d/%d", lp.layer, lp.datatype);
        }
      } else {
        if (lp.layer < 0 && lp.datatype < 0) {
          r = tl::sprintf ("%%%d", m_layer_index);
        } else if (lp.layer < 0) {
          r = tl::sprintf ("*/%d", lp.datatype);
        } else if (lp.datatype < 0) {
          r = tl::sprintf ("%d/*", lp.layer);
        } else {
          r = tl::sprintf ("%d/%d", lp.layer, lp.datatype);
        }
      }
    }

  } else if (m_has_name) {
    r = m_name;
    if (m_layer >= 0 && m_datatype >= 0 && (!view || view->always_show_ld ())) {
      r += tl::sprintf (" %d/%d", m_layer, m_datatype);
    }
  } else {
    if (m_layer < 0 && m_datatype < 0) {
      r = "";
    } else if (m_layer < 0) {
      r = tl::sprintf ("*/%d", m_datatype);
    } else if (m_datatype < 0) {
      r = tl::sprintf ("%d/*", m_layer);
    } else {
      r = tl::sprintf ("%d/%d", m_layer, m_datatype);
    }
  }

  if (m_cv_index >= 0 && (!view || view->always_show_layout_index () || m_cv_index > 0 || view->cellviews () > 1)) {
    r += tl::sprintf ("@%d", m_cv_index + 1);
  }

  switch (m_special_purpose) {
  case SP_None:
    break;
  case SP_CellFrame:
    if (! r.empty ()) {
      r += " ";
    }
    r += "!CellFrame";
    break;
  }

  if (! m_cell_sel.is_empty ()) {
    if (! r.empty ()) {
      r += " ";
    }
    r += m_cell_sel.to_string ();
  }

  if (! m_trans.empty () && (m_trans.size () > 1 || m_trans[0] != db::DCplxTrans ())) {
    for (std::vector<db::DCplxTrans>::const_iterator t = m_trans.begin (); t != m_trans.end (); ++t) {
      if (! r.empty ()) {
        r += " ";
      }
      r += "(";
      r += t->to_string ();
      r += ")";
    }
  }

  if (! m_property_sel.is_null ()) {
    if (! r.empty ()) {
      r += " ";
    }
    r += "[" + m_property_sel.to_string (32) + "]";
  }

  if (m_hier_levels.has_from_level () || m_hier_levels.has_to_level ()) {
    if (! r.empty ()) {
      r += " ";
    }
    r += "#" + hier_levels_to_string (m_hier_levels);
  }

  return r;
}

bool 
ParsedLayerSource::operator== (const ParsedLayerSource &d) const
{
  if (m_trans != d.m_trans) {
    return false;
  }
  if (m_cell_sel != d.m_cell_sel) {
    return false;
  }
  if (m_property_sel != d.m_property_sel) {
    return false;
  }
  if (m_cv_index != d.m_cv_index) {
    return false;
  }
  if (m_hier_levels != d.m_hier_levels) {
    return false;
  }
  if (m_has_name != d.m_has_name) {
    return false;
  }
  if (m_special_purpose != d.m_special_purpose) {
    return false;
  }
  if (m_layer_index != d.m_layer_index) {
    return false;
  }
  if ((m_layer < 0) != (d.m_layer < 0)) {
    return false;
  }
  if (m_layer >= 0 && m_layer != d.m_layer) {
    return false;
  }
  if ((m_datatype < 0) != (d.m_datatype < 0)) {
    return false;
  }
  if (m_datatype >= 0 && m_datatype != d.m_datatype) {
    return false;
  }
  if (m_has_name && m_name != d.m_name) {
    return false;
  } 
  return true;
}

bool 
ParsedLayerSource::operator< (const ParsedLayerSource &d) const
{
  if (m_trans != d.m_trans) {
    return m_trans < d.m_trans;
  }
  if (m_cell_sel != d.m_cell_sel) {
    return m_cell_sel < d.m_cell_sel;
  }
  if (m_property_sel != d.m_property_sel) {
    return m_property_sel < d.m_property_sel;
  }
  if (m_cv_index != d.m_cv_index) {
    return m_cv_index < d.m_cv_index;
  }
  if (m_hier_levels != d.m_hier_levels) {
    return m_hier_levels < d.m_hier_levels;
  }
  if (m_special_purpose != d.m_special_purpose) {
    return m_special_purpose < d.m_special_purpose;
  }
  if (m_layer_index != d.m_layer_index) {
    return m_layer_index < d.m_layer_index;
  }
  bool is_named = (m_layer < 0 && m_datatype < 0);
  bool d_is_named = (d.m_layer < 0 && d.m_datatype < 0);
  if (is_named != d_is_named) {
    return is_named < d_is_named;
  }
  if (is_named) {
    if (m_has_name != d.m_has_name) {
      return m_has_name < d.m_has_name;
    }
    if (m_has_name) {
      return m_name < d.m_name;
    } else {
      return false;
    }
  } else {
    if (m_layer != d.m_layer) {
      return m_layer < d.m_layer;
    }
    return m_datatype < d.m_datatype;
  }
}

void 
ParsedLayerSource::parse_from_string (const char *cp) 
{
  m_layer_index = -1;
  m_special_purpose = SP_None;
  m_layer = -1;
  m_datatype = -1;
  m_name = "";
  m_cv_index = 0;
  m_has_name = false;
  m_trans.clear ();
  m_cell_sel = CellSelector ();

  unsigned int v = 0;

  tl::Extractor x (cp);

  while (! x.at_end ()) {

    if (x.test ("!")) {

      std::string sp;
      x.read (sp);

      if (sp == "CellFrame" || sp == "cellframe" || sp == "CF" || sp == "cell-frame") {
        m_special_purpose = SP_CellFrame;
      } else {
        throw tl::Exception (tl::to_string (tr ("Invalid special purpose '%s'")), sp);
      }

    } else if (x.test ("(")) {

      db::DCplxTrans t;
      x.read (t);
      x.expect (")");

      m_trans.push_back (t);

    } else if (x.test ("[")) {

      m_property_sel.extract (x);
      x.expect ("]");

    } else if (x.test ("%")) {

      unsigned int n;
      x.read (n);

      m_layer_index = int (n);

    } else if (x.test ("#")) {

      //  The hierarchy level selection format is that (C=number of context levels, above current cell)
      //   #n           -> = #0..n
      //   #(n)         -> = #(0)..(n)
      //   #..n         -> = #0..n
      //   #..(n)       -> = #0..(n)
      //   #m..n        -> from=m, to=n
      //   #(m)..(n)    -> from=C+m, to=C+n
      //   #m..(n)      -> from=m, to=C+n
      //   #(m)..n      -> from=C+m, to=n
      //   m and n can be "<l" or ">l" to indicate minimum or maximum mode respectively

      m_hier_levels = HierarchyLevelSelection ();
      if (x.test ("*")) {

        m_hier_levels.set_from_level (0, false, HierarchyLevelSelection::absolute);
        m_hier_levels.set_to_level (std::numeric_limits<int>::max (), false, HierarchyLevelSelection::absolute);

      } else if (x.test ("<*")) {

        m_hier_levels.set_from_level (0, false, HierarchyLevelSelection::absolute);
        m_hier_levels.set_to_level (std::numeric_limits<int>::max (), false, HierarchyLevelSelection::minimum);

      } else if (x.test ("(*)")) {

        m_hier_levels.set_from_level (0, true, HierarchyLevelSelection::absolute);
        m_hier_levels.set_to_level (std::numeric_limits<int>::max (), false, HierarchyLevelSelection::absolute);

      } else if (x.test ("(<*)")) {

        m_hier_levels.set_from_level (0, true, HierarchyLevelSelection::absolute);
        m_hier_levels.set_to_level (std::numeric_limits<int>::max (), false, HierarchyLevelSelection::minimum);

      } else {

        HierarchyLevelSelection::level_mode_type m;

        int f = 0;
        m = HierarchyLevelSelection::absolute;
        if (x.test ("(")) {
          if (x.test ("<")) {
            m = HierarchyLevelSelection::minimum;
          } else if (x.test (">")) {
            m = HierarchyLevelSelection::maximum;
          }
          x.read (f);
          x.expect(")");
          m_hier_levels.set_from_level (f, true, m);
        } else {
          if (x.test ("<")) {
            m = HierarchyLevelSelection::minimum;
          } else if (x.test (">")) {
            m = HierarchyLevelSelection::maximum;
          }
          if (x.try_read (f)) {
            m_hier_levels.set_from_level (f, false, m);
          }
        }

        if (x.test ("..")) {

          int t;
          m = HierarchyLevelSelection::absolute;
          if (x.test ("*") || x.test ("(*)")) {
            m_hier_levels.set_to_level (std::numeric_limits<int>::max (), false, HierarchyLevelSelection::absolute);
          } else if (x.test ("<*") || x.test ("(<*)")) {
            m_hier_levels.set_to_level (std::numeric_limits<int>::max (), false, HierarchyLevelSelection::minimum);
          } else if (x.test ("(")) {
            if (x.test ("<")) {
              m = HierarchyLevelSelection::minimum;
            } else if (x.test (">")) {
              m = HierarchyLevelSelection::maximum;
            }
            x.read (t);
            x.expect(")");
            m_hier_levels.set_to_level (t, true, m);
          } else {
            if (x.test ("<")) {
              m = HierarchyLevelSelection::minimum;
            } else if (x.test (">")) {
              m = HierarchyLevelSelection::maximum;
            }
            if (x.try_read (t)) {
              m_hier_levels.set_to_level (t, false, m);
            }
          }

        } else if (m_hier_levels.has_from_level ()) {

          //  No explicit "to" spec: use "from" as "to" and put "0" into "from" place.
          bool fr = m_hier_levels.from_level_relative ();
          int f = m_hier_levels.from_level ();
          HierarchyLevelSelection::level_mode_type m = m_hier_levels.from_level_mode ();
          m_hier_levels.set_to_level (f, fr, m);
          m_hier_levels.set_from_level (0, fr, HierarchyLevelSelection::absolute);

        }

      }

    } else if (x.test ("/")) {

      if (x.test ("*")) {
        m_datatype = -1;
      } else {
        x.read (v);
        m_datatype = int (v);
      }

    } else if (x.test ("{")) {

      m_cell_sel.parse (x);
      x.expect ("}");

    } else if (x.test ("*")) {

      m_layer = -1;

    } else if (x.try_read (v)) {

      m_layer = int (v);
      m_datatype = 0;

    } else if (x.test ("@")) {

      unsigned int ui = 0;
      if (x.test ("*")) {
        //  .. @* -> cv_index=-1
      } else {
        x.read (ui);
      }
      m_cv_index = int (ui) - 1;

    } else {
      
      x.skip ();
      if (*x == '\'' || *x == '"') {
        x.read_quoted (m_name);
      } else {
        x.read (m_name, "@([/*#%");
      }

      m_has_name = true;

    }
  }
  
  if (m_trans.empty ()) {
    m_trans.push_back (db::DCplxTrans ());
  }
}

db::LayerProperties 
ParsedLayerSource::layer_props () const
{
  db::LayerProperties lp;
  if (has_name ()) {
    lp.name = name ();
  }
  if (m_layer >= 0) {
    lp.layer = m_layer;
  }
  if (m_datatype >= 0) {
    lp.datatype = m_datatype;
  }
  return lp;
}

unsigned int 
ParsedLayerSource::color_index () const
{
  if (layer () >= 0) {

    //  by default use the GDS layer number
    return (unsigned int) layer ();

  } else if (has_name ()) {

    //  if there is no layer, use a simple hash value derived from the name.
    unsigned int ln = 0;
    const std::string &n = name ();
    for (const char *cp = n.c_str (); *cp; ++cp) {
      ln = ln * 37 + (unsigned int)*cp;
    }

    return ln;

  } else {

    //  no specific ordering: no ordering
    return 0;

  }
}

bool 
ParsedLayerSource::is_wildcard_layer () const
{
  return (m_special_purpose == SP_None && ! has_name () && m_layer < 0 && m_datatype < 0 && m_layer_index < 0);
}

bool
ParsedLayerSource::match (const db::LayerProperties &lp) const
{
  return layer_props ().log_equal (lp);
}

}

