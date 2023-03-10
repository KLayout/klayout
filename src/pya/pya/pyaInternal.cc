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

#include <Python.h>

#include "pya.h"
#include "pyaInternal.h"
#include "pyaModule.h"
#include "pyaObject.h"

#include "tlLog.h"

#include <cctype>

namespace pya
{

// -------------------------------------------------------------------
//  MethodTableEntry implementation

MethodTableEntry::MethodTableEntry (const std::string &name, bool st, bool prot)
  : m_name (name), m_is_static (st), m_is_protected (prot), m_is_enabled (true), m_is_init (false), m_fallback_not_implemented (false)
{ }

const std::string &
MethodTableEntry::name () const
{
  return m_name;
}

void
MethodTableEntry::set_name (const std::string &n)
{
  m_name = n;
}

void
MethodTableEntry::set_enabled (bool en)
{
  m_is_enabled = en;
}

bool
MethodTableEntry::is_enabled () const
{
  return m_is_enabled;
}

void
MethodTableEntry::set_fallback_not_implemented (bool f)
{
  m_fallback_not_implemented = f;
}

bool
MethodTableEntry::fallback_not_implemented () const
{
  return m_fallback_not_implemented;
}

void
MethodTableEntry::set_init (bool f)
{
  m_is_init = f;
}

bool
MethodTableEntry::is_init () const
{
  return m_is_init;
}

bool
MethodTableEntry::is_static () const
{
  return m_is_static;
}

bool
MethodTableEntry::is_protected () const
{
  return m_is_protected;
}

void
MethodTableEntry::add (const gsi::MethodBase *m)
{
  m_methods.push_back (m);
}

void
MethodTableEntry::finish ()
{
  //  remove duplicate entries in the method list
  std::vector<const gsi::MethodBase *> m = m_methods;
  std::sort(m.begin (), m.end ());
  m_methods.assign (m.begin (), std::unique (m.begin (), m.end ()));
}

MethodTableEntry::method_iterator
MethodTableEntry::begin () const
{
  return m_methods.begin ();
}

MethodTableEntry::method_iterator
MethodTableEntry::end () const
{
  return m_methods.end ();
}

// -------------------------------------------------------------------
//  MethodTable implementation

MethodTable::MethodTable (const gsi::ClassBase *cls_decl, PythonModule *module)
  : m_method_offset (0), m_property_offset (0), mp_cls_decl (cls_decl), mp_module (module)
{
  if (cls_decl->base ()) {
    const MethodTable *base_mt = method_table_by_class (cls_decl->base ());
    tl_assert (base_mt);
    m_method_offset = base_mt->top_mid ();
    m_property_offset = base_mt->top_property_mid ();
  }

  //  signals are translated into the setters and getters
  for (gsi::ClassBase::method_iterator m = cls_decl->begin_methods (); m != cls_decl->end_methods (); ++m) {
    if ((*m)->is_signal ()) {
      for (gsi::MethodBase::synonym_iterator syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms (); ++syn) {
        add_getter (syn->name, *m);
        add_setter (syn->name, *m);
      }
    }
  }

  //  first add getters and setters
  for (gsi::ClassBase::method_iterator m = cls_decl->begin_methods (); m != cls_decl->end_methods (); ++m) {
    if (! (*m)->is_callback ()) {
      for (gsi::MethodBase::synonym_iterator syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms (); ++syn) {
        if (syn->is_getter) {
          add_getter (syn->name, *m);
        } else if (syn->is_setter) {
          add_setter (syn->name, *m);
        }
      }
    }
  }

  //  then add normal methods - on name clash with properties make them a getter
  for (gsi::ClassBase::method_iterator m = cls_decl->begin_methods (); m != cls_decl->end_methods (); ++m) {

    if (! (*m)->is_callback () && ! (*m)->is_signal ()) {

      bool st = (*m)->is_static ();
      bool no_args = ((*m)->end_arguments () == (*m)->begin_arguments ());

      for (gsi::MethodBase::synonym_iterator syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms (); ++syn) {
        if (! syn->is_getter && ! syn->is_setter) {
          if (no_args && is_property_setter (st, syn->name) && ! is_property_getter (st, syn->name)) {
            add_getter (syn->name, *m);
          } else if (st && no_args && (isupper (syn->name [0]) || (*m)->is_const ())) {
            //  static methods without arguments which start with a capital letter are treated as constants
            add_getter (syn->name, *m);
          } else {
            add_method (syn->name, *m);
          }
        }
      }

    }
  }

  //  synthesize a getter from is_...? predicates (e.g. is_empty? -> empty getter)
  for (gsi::ClassBase::method_iterator m = cls_decl->begin_methods (); m != cls_decl->end_methods (); ++m) {

    if (! (*m)->is_callback () && ! (*m)->is_signal ()) {

      bool st = (*m)->is_static ();
      bool no_args = ((*m)->end_arguments () == (*m)->begin_arguments ());

      for (gsi::MethodBase::synonym_iterator syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms (); ++syn) {
        if (no_args && ! syn->is_getter && ! syn->is_setter && syn->is_predicate && std::string (syn->name, 0, 3) == "is_") {
          std::string n = std::string (syn->name, 3, std::string::npos);
          if (is_property_setter (st, n) && ! is_property_getter (st, n)) {
            add_getter (n, *m);
          }
        }
      }

    }

  }
}

size_t
MethodTable::bottom_mid () const
{
  return m_method_offset;
}

size_t
MethodTable::top_mid () const
{
  return m_method_offset + m_table.size ();
}

size_t
MethodTable::bottom_property_mid () const
{
  return m_property_offset;
}

size_t
MethodTable::top_property_mid () const
{
  return m_property_offset + m_property_table.size ();
}

std::pair<bool, size_t>
MethodTable::find_method (bool st, const std::string &name) const
{
  std::map <std::pair<bool, std::string>, size_t>::const_iterator t = m_name_map.find (std::make_pair (st, name));
  if (t != m_name_map.end ()) {
    return std::make_pair (true, t->second + m_method_offset);
  } else {
    return std::make_pair (false, 0);
  }
}

std::pair<bool, size_t>
MethodTable::find_property (bool st, const std::string &name) const
{
  std::map <std::pair<bool, std::string>, size_t>::const_iterator t = m_property_name_map.find (std::make_pair (st, name));
  if (t != m_property_name_map.end ()) {
    return std::make_pair (true, t->second + m_property_offset);
  } else {
    return std::make_pair (false, 0);
  }
}

bool
MethodTable::is_property_setter (bool st, const std::string &name)
{
  std::pair<bool, size_t> p = find_property (st, name);
  if (! p.first) {
    return false;
  }
  return (begin_setters (p.second) != end_setters (p.second));
}

bool
MethodTable::is_property_getter (bool st, const std::string &name)
{
  std::pair<bool, size_t> p = find_property (st, name);
  if (! p.first) {
    return false;
  }
  return (begin_getters (p.second) != end_getters (p.second));
}

/**
 *  @brief Returns true, if the name is a reserved keyword
 */
static bool is_reserved_word (const std::string &name)
{
  return (name == "and" ||
          name == "del" ||
          name == "from" ||
          name == "not" ||
          name == "while" ||
          name == "as" ||
          name == "elif" ||
          name == "global" ||
          name == "or" ||
          name == "with" ||
          name == "assert" ||
          name == "else" ||
          name == "if" ||
          name == "pass" ||
          name == "yield" ||
          name == "break" ||
          name == "except" ||
          name == "import" ||
          name == "print" ||
          name == "class" ||
          name == "exec" ||
          name == "in" ||
          name == "raise" ||
          name == "continue" ||
          name == "finally" ||
          name == "is" ||
          name == "return" ||
          name == "def" ||
          name == "for" ||
          name == "lambda" ||
          name == "try" ||
          name == "None");
}

/**
 *  @brief Extracts the Python name from a generic name
 *
 *  Returns an empty string if no Python name could be generated.
 */
static std::string extract_python_name (const std::string &name)
{
  //  some operator replacements
  if (name == "++") {
    return "inc";
  } else if (name == "--") {
    return "dec";
  } else if (name == "()") {
    return "call";
  } else if (name == "!") {
    return "not";
  } else if (name == "==") {
    return "__eq__";
  } else if (name == "!=") {
    return "__ne__";
  } else if (name == "<") {
    return "__lt__";
  } else if (name == "<=") {
    return "__le__";
  } else if (name == ">") {
    return "__gt__";
  } else if (name == ">=") {
    return "__ge__";
  } else if (name == "<=>") {
    return "__cmp__";
  } else if (name == "+") {
    return "__add__";
  } else if (name == "+@") {
    return "__pos__";
  } else if (name == "-") {
    return "__sub__";
  } else if (name == "-@") {
    return "__neg__";
  } else if (name == "/") {
    #if PY_MAJOR_VERSION < 3
    return "__div__";
    #else
    return "__truediv__";
    #endif
  } else if (name == "*" || name == "*!") {
    return "__mul__";
  } else if (name == "%") {
    return "__mod__";
  } else if (name == "<<") {
    return "__lshift__";
  } else if (name == ">>") {
    return "__rshift__";
  } else if (name == "~") {
    return "__invert__";
  } else if (name == "&") {
    return "__and__";
  } else if (name == "|") {
    return "__or__";
  } else if (name == "^") {
    return "__xor__";
  } else if (name == "+=") {
    return "__iadd__";
  } else if (name == "-=") {
    return "__isub__";
  } else if (name == "/=") {
    #if PY_MAJOR_VERSION < 3
    return "__idiv__";
    #else
    return "__itruediv__";
    #endif
  } else if (name == "*=") {
    return "__imul__";
  } else if (name == "%=") {
    return "__imod__";
  } else if (name == "<<=") {
    return "__ilshift__";
  } else if (name == ">>=") {
    return "__irshift__";
  } else if (name == "&=") {
    return "__iand__";
  } else if (name == "|=") {
    return "__ior__";
  } else if (name == "^=") {
    return "__ixor__";
  } else if (name == "[]") {
    return "__getitem__";
  } else if (name == "[]=") {
    return "__setitem__";
  } else {

    const char *c = name.c_str ();
    if (! isalnum (*c) && *c != '_') {
      return std::string ();
    }

    //  question-mark symbol and trailing = are removed.
    size_t n = 0;
    for ( ; *c; ++c) {
      if (*c == '=' || *c == '?') {
        if (! c[1]) {
          if (*c == '=') {
            //  Normally, this method is replaced by an attribute.
            //  If that fails, we prepend a "set_" to make the name unique.
            return "set_" + std::string (name, 0, n);
          } else {
            return std::string (name, 0, n);
          }
        } else {
          return std::string ();
        }
      } else if (! isalnum (*c) && *c != '_') {
        return std::string ();
      } else {
        ++n;
      }
    }

    return name;

  }
}

/**
 *  @brief Returns true, if the method with the given name shall fallback to NotImplemented
 */
static bool is_method_with_fallback (const std::string &name)
{
  if (name == "+") {
    return true;
  } else if (name == "-") {
    return true;
  } else if (name == "/") {
    #if PY_MAJOR_VERSION < 3
    return false;
    #else
    return true;
    #endif
  } else if (name == "*") {
    return true;
  } else if (name == "%") {
    return true;
  } else if (name == "<<") {
    return true;
  } else if (name == ">>") {
    return true;
  } else if (name == "&") {
    return true;
  } else if (name == "|") {
    return true;
  } else if (name == "^") {
    return true;
  } else {
    return false;
  }
}

void
MethodTable::add_method (const std::string &name, const gsi::MethodBase *mb)
{
  if (is_reserved_word (name)) {

    //  drop non-standard names
    if (tl::verbosity () >= 20) {
      tl::warn << tl::to_string (tr ("Class ")) << mp_cls_decl->name () << ": " << tl::to_string (tr ("no Python mapping for method (reserved word) ")) << name;
    }

    std::string new_name = name + "_";

    add_method_basic (new_name, mb);
    mp_module->add_python_doc (mb, tl::sprintf (tl::to_string (tr ("This attribute is available as '%s' in Python")), new_name));

  } else if (name == "new" && mb->ret_type ().type () == gsi::T_object && mb->ret_type ().pass_obj ()) {

    add_method_basic (name, mb);

    add_method_basic ("__init__", mb, true /*enabled*/, true /*constructor*/);
    mp_module->add_python_doc (mb, tl::to_string (tr ("This method is the default initializer of the object")));

  } else if (name == "to_s" && mb->compatible_with_num_args (0)) {

    add_method_basic (name, mb);

    //  The str method is also routed via the tp_str implementation
    add_method_basic ("__str__", mb);

#if defined(GSI_ALIAS_INSPECT)
    //  also alias to "__repr__" unless there is an explicit "inspect" method
    bool alias_inspect = true;
    for (gsi::ClassBase::method_iterator m = mp_cls_decl->begin_methods (); m != mp_cls_decl->end_methods () && alias_inspect; ++m) {
      if (! (*m)->is_callback () && ! (*m)->is_signal ()) {
        for (gsi::MethodBase::synonym_iterator syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms () && alias_inspect; ++syn) {
          if (! syn->is_getter && ! syn->is_setter && syn->name == "inspect") {
            alias_inspect = false;
          }
        }
      }
    }
#else
    bool alias_inspect = false;
#endif
    if (alias_inspect) {
      add_method_basic ("__repr__", mb);
      mp_module->add_python_doc (mb, tl::to_string (tr ("This method is also available as 'str(object)' and 'repr(object)'")));
    } else {
      mp_module->add_python_doc (mb, tl::to_string (tr ("This method is also available as 'str(object)'")));
    }

  } else if (name == "hash" && mb->compatible_with_num_args (0)) {

    //  The hash method is also routed via the tp_hash implementation
    add_method_basic ("__hash__", mb);

    add_method_basic (name, mb);
    mp_module->add_python_doc (mb, tl::to_string (tr ("This method is also available as 'hash(object)'")));

  } else if (name == "inspect" && mb->compatible_with_num_args (0)) {

    //  The str method is also routed via the tp_str implementation
    add_method_basic ("__repr__", mb);

    add_method_basic (name, mb);
    mp_module->add_python_doc (mb, tl::to_string (tr ("This method is also available as 'repr(object)'")));

  } else if (name == "size" && mb->compatible_with_num_args (0)) {

    //  The size method is also routed via the sequence methods protocol if there
    //  is a [] function
    add_method_basic ("__len__", mb);

    add_method_basic (name, mb);
    mp_module->add_python_doc (mb, tl::to_string (tr ("This method is also available as 'len(object)'")));

  } else if (name == "each" && mb->compatible_with_num_args (0) && mb->ret_type ().is_iter ()) {

    //  each makes the object iterable
    add_method_basic ("__iter__", mb);

    add_method_basic (name, mb);
    mp_module->add_python_doc (mb, tl::to_string (tr ("This method enables iteration of the object")));

  } else if (name == "dup" && mb->compatible_with_num_args (0)) {

    //  If the object supports the dup method, then it is a good
    //  idea to define the __copy__ and __deepcopy__ method.
    add_method_basic ("__copy__", mb);
    add_method_basic ("__deepcopy__", mb);

    add_method_basic (name, mb);
    mp_module->add_python_doc (mb, tl::to_string (tr ("This method also implements '__copy__' and '__deepcopy__'")));

  } else {

    std::string py_name = extract_python_name (name);
    if (py_name.empty ()) {

      //  drop non-standard names
      if (tl::verbosity () >= 20) {
        tl::warn << tl::to_string (tr ("Class ")) << mp_cls_decl->name () << ": " << tl::to_string (tr ("no Python mapping for method ")) << name;
      }

      add_method_basic (name, mb, false);
      mp_module->add_python_doc (mb, tl::to_string (tr ("This method is not available for Python")));

    } else {

      bool fb = is_method_with_fallback (name);
      add_method_basic (py_name, mb, true, false, fb);

      if (name == "*") {
        //  Supply a commutative multiplication version unless the operator is "*!"
        add_method_basic ("__rmul__", mb);
        mp_module->add_python_doc (mb, tl::to_string (tr ("This method also implements '__rmul__'")));
      }

    }

  }
}

void
MethodTable::add_setter (const std::string &name, const gsi::MethodBase *setter)
{
  if (is_reserved_word (name)) {

    std::string new_name = name + "_";

    add_setter_basic (new_name, setter);
    mp_module->add_python_doc (setter, tl::sprintf (tl::to_string (tr ("This member is available as '%s' in Python")), new_name));

  } else {
    add_setter_basic (name, setter);
  }
}

void
MethodTable::add_setter_basic (const std::string &name, const gsi::MethodBase *setter)
{
  bool st = setter->is_static ();

  std::map<std::pair<bool, std::string>, size_t>::iterator n = m_property_name_map.find (std::make_pair (st, name));
  if (n == m_property_name_map.end ()) {

    m_property_name_map.insert (std::make_pair (std::make_pair(st, name), m_property_table.size ()));
    m_property_table.push_back (std::make_pair (MethodTableEntry (name, st, false), MethodTableEntry (name, st, false)));
    m_property_table.back ().first.add (setter);

  } else {

    m_property_table [n->second].first.add (setter);

  }
}

void
MethodTable::add_getter (const std::string &name, const gsi::MethodBase *getter)
{
  if (is_reserved_word (name)) {

    std::string new_name = name + "_";

    add_getter_basic (new_name, getter);
    mp_module->add_python_doc (getter, tl::sprintf (tl::to_string (tr ("This member is available as '%s' in Python")), new_name));

  } else {
    add_getter_basic (name, getter);
  }
}

void
MethodTable::add_getter_basic (const std::string &name, const gsi::MethodBase *getter)
{
  bool st = getter->is_static ();

  std::map<std::pair<bool, std::string>, size_t>::iterator n = m_property_name_map.find (std::make_pair (st, name));
  if (n == m_property_name_map.end ()) {

    m_property_name_map.insert (std::make_pair (std::make_pair(st, name), m_property_table.size ()));
    m_property_table.push_back (std::make_pair (MethodTableEntry (name, st, false), MethodTableEntry (name, st, false)));
    m_property_table.back ().second.add (getter);

  } else {

    m_property_table [n->second].second.add (getter);

  }
}

bool
MethodTable::is_enabled (size_t mid) const
{
  return m_table [mid - m_method_offset].is_enabled ();
}

void
MethodTable::set_enabled (size_t mid, bool en)
{
  m_table [mid - m_method_offset].set_enabled (en);
}

bool
MethodTable::fallback_not_implemented (size_t mid) const
{
  return m_table [mid - m_method_offset].fallback_not_implemented ();
}

void
MethodTable::set_fallback_not_implemented (size_t mid, bool f)
{
  m_table [mid - m_method_offset].set_fallback_not_implemented (f);
}

bool
MethodTable::is_init(size_t mid) const
{
  return m_table [mid - m_method_offset].is_init ();
}

void
MethodTable::set_init (size_t mid, bool f)
{
  m_table [mid - m_method_offset].set_init (f);
}

bool
MethodTable::is_static (size_t mid) const
{
  return m_table [mid - m_method_offset].is_static ();
}

bool
MethodTable::is_protected (size_t mid) const
{
  return m_table [mid - m_method_offset].is_protected ();
}

void
MethodTable::alias (size_t mid, const std::string &new_name)
{
  bool st = is_static (mid);
  auto nm = m_name_map.find (std::make_pair (st, new_name));
  tl_assert (nm == m_name_map.end ());

  m_table.push_back (m_table [mid - m_method_offset]);
  m_table.back ().set_name (new_name);
  m_name_map.insert (std::make_pair (std::make_pair (st, new_name), m_table.size () - 1 - m_method_offset));
}

void
MethodTable::rename (size_t mid, const std::string &new_name)
{
  std::string old_name = name (mid);
  bool st = is_static (mid);

  m_table [mid - m_method_offset].set_name (new_name);

  auto nm = m_name_map.find (std::make_pair (st, old_name));
  if (nm != m_name_map.end ()) {
    m_name_map.erase (nm);
    m_name_map.insert (std::make_pair (std::make_pair (st, new_name), mid));
  }
}

const std::string &
MethodTable::name (size_t mid) const
{
  return m_table [mid - m_method_offset].name ();
}

const std::string &
MethodTable::property_name (size_t mid) const
{
  return m_property_table [mid - m_property_offset].first.name ();
}

MethodTableEntry::method_iterator
MethodTable::begin_setters (size_t mid) const
{
  return m_property_table[mid - m_property_offset].first.begin ();
}

MethodTableEntry::method_iterator
MethodTable::end_setters (size_t mid) const
{
  return m_property_table[mid - m_property_offset].first.end ();
}

MethodTableEntry::method_iterator
MethodTable::begin_getters (size_t mid) const
{
  return m_property_table[mid - m_property_offset].second.begin ();
}

MethodTableEntry::method_iterator
MethodTable::end_getters (size_t mid) const
{
  return m_property_table[mid - m_property_offset].second.end ();
}

MethodTableEntry::method_iterator
MethodTable::begin (size_t mid) const
{
  return m_table[mid - m_method_offset].begin ();
}

MethodTableEntry::method_iterator
MethodTable::end (size_t mid) const
{
  return m_table[mid - m_method_offset].end ();
}

void
MethodTable::finish ()
{
  for (std::vector<MethodTableEntry>::iterator m = m_table.begin (); m != m_table.end (); ++m) {
    m->finish ();
    if (m->is_enabled ()) {
      //  disable methods which are also present as properties
      if (m_property_name_map.find (std::make_pair (m->is_static (), m->name ())) != m_property_name_map.end ()) {
        m->set_enabled (false);
      }
    }
  }
  for (std::vector<std::pair<MethodTableEntry, MethodTableEntry> >::iterator m = m_property_table.begin (); m != m_property_table.end (); ++m) {
    m->first.finish ();
    m->second.finish ();
  }
}

void
MethodTable::add_method_basic (const std::string &name, const gsi::MethodBase *mb, bool enabled, bool init, bool fallback_not_implemented)
{
  bool st = mb->is_static () && ! init;

  std::map<std::pair<bool, std::string>, size_t>::iterator n = m_name_map.find (std::make_pair (st, name));
  if (n == m_name_map.end ()) {

    m_name_map.insert (std::make_pair (std::make_pair (st, name), m_table.size ()));
    m_table.push_back (MethodTableEntry (name, st, mb->is_protected ()));
    if (! enabled) {
      m_table.back ().set_enabled (false);
    }
    if (init) {
      m_table.back ().set_init (true);
    }
    if (fallback_not_implemented) {
      m_table.back ().set_fallback_not_implemented (true);
    }
    m_table.back ().add (mb);

  } else {

    if (m_table [n->second].is_protected () != mb->is_protected ()) {
      tl::warn << "Class " << mp_cls_decl->name () << ": method '" << name << " is both a protected and non-protected";
    }

    m_table [n->second].add (mb);
    if (! enabled) {
      m_table [n->second].set_enabled (false);
    }
    if (init) {
      tl_assert (m_table [n->second].is_init ());
    }
    if (fallback_not_implemented) {
      m_table.back ().set_fallback_not_implemented (true);
    }

  }
}

MethodTable *
MethodTable::method_table_by_class (const gsi::ClassBase *cls_decl)
{
  PythonClassClientData *cd = dynamic_cast<PythonClassClientData *>(cls_decl->data (gsi::ClientIndex::Python));
  return cd ? &cd->method_table : 0;
}

// -------------------------------------------------------------------
//  PythonClassClientData implementation

PythonClassClientData::PythonClassClientData (const gsi::ClassBase *_cls, PyTypeObject *_py_type, PyTypeObject *_py_type_static, PythonModule *module)
  : py_type_object ((PyObject *) _py_type), py_type_object_static ((PyObject *) _py_type_static), method_table (_cls, module)
{
  //  .. nothing yet ..
}

PythonClassClientData::~PythonClassClientData ()
{
  //  This destructor is called from the exit code. Python may have shut down already.
  //  We must not try to release the objects in that case and simply don't care about them any longer.
  py_type_object.release ();
  py_type_object_static.release ();
}

PyTypeObject *
PythonClassClientData::py_type (const gsi::ClassBase &cls_decl, bool as_static)
{
  PythonClassClientData *cd = dynamic_cast<PythonClassClientData *>(cls_decl.data (gsi::ClientIndex::Python));
  return (PyTypeObject *) (cd ? (as_static ? cd->py_type_object_static.get () : cd->py_type_object.get ()) : 0);
}

void
PythonClassClientData::initialize (const gsi::ClassBase &cls_decl, PyTypeObject *py_type, bool as_static, PythonModule *module)
{
  PythonClassClientData *cd = dynamic_cast<PythonClassClientData *>(cls_decl.data (gsi::ClientIndex::Python));
  if (cd) {
    if (as_static) {
      cd->py_type_object_static = (PyObject *) py_type;
    } else {
      cd->py_type_object = (PyObject *) py_type;
    }
  } else {
    cls_decl.set_data (gsi::ClientIndex::Python, new PythonClassClientData (&cls_decl, as_static ? NULL : py_type, as_static ? py_type : NULL, module));
  }
}

}

