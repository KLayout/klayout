
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#include "gsiDecl.h"
#include "gsiExpression.h"
#include "gsiObjectHolder.h"
#include "gsiVariantArgs.h"

#include "tlExpression.h"
#include "tlLog.h"

#include <set>
#include <map>
#include <list>
#include <cstdio>
#include <algorithm>

namespace gsi
{

// -------------------------------------------------------------------
//  Method table implementation

/**
 *  @brief A single entry in the method table
 *  This class provides an entry for one name. It provides flags
 *  (ctor) for the method and a list of implementations
 *  (gsi::MethodBase objects)
 */
class ExpressionMethodTableEntry
{
public:
  typedef std::vector<const gsi::MethodBase *>::const_iterator method_iterator;

  ExpressionMethodTableEntry (const std::string &name)
    : m_name (name)
  { }

  const std::string &name () const
  {
    return m_name;
  }

  void add (const gsi::MethodBase *m)
  {
    m_methods.push_back (m);
  }

  void finish ()
  {
    //  remove duplicate entries in the method list
    std::vector<const gsi::MethodBase *> m = m_methods;
    std::sort(m.begin (), m.end ());
    m_methods.assign (m.begin (), std::unique (m.begin (), m.end ()));
  }

  method_iterator begin () const
  {
    return m_methods.begin ();
  }

  method_iterator end () const
  {
    return m_methods.end ();
  }

private:
  std::string m_name;
  std::vector<const gsi::MethodBase *> m_methods;
};

/**
 *  @brief The method table for a class
 *  The method table will provide the methods associated with a native method, i.e.
 *  a certain name. It only provides the methods, not a overload resolution strategy.
 */
class ExpressionMethodTable
  : public gsi::PerClassClientSpecificData
{
public:
  /**
   *  @brief Find a method by name and static flag
   *  This method will return a pair of true and the method ID if a method with 
   *  the static attribute and the name is found. Otherwise the first value of
   *  the returned pair will be false.
   */
  std::pair<bool, size_t> find (bool st, const std::string &name) const
  {
    std::map<std::pair<bool, std::string>, size_t>::const_iterator t = m_name_map.find (std::make_pair (st, name));
    if (! st && t == m_name_map.end ()) {
      //  can also use static methods for instances
      t = m_name_map.find (std::make_pair (true, name));
    }
    if (t != m_name_map.end ()) {
      return std::make_pair (true, t->second);
    } else {
      return std::make_pair (false, 0);
    }
  }

  /**
   *  @brief Returns the name of the method with ID mid
   */
  const std::string &name (size_t mid) const
  {
    return m_table [mid].name ();
  }

  /**
   *  @brief Begin iterator for the overloaded methods for method ID mid
   */
  ExpressionMethodTableEntry::method_iterator begin (size_t mid) const
  {
    return m_table[mid].begin ();
  }

  /**
   *  @brief End iterator for the overloaded methods for method ID mid
   */
  ExpressionMethodTableEntry::method_iterator end (size_t mid) const
  {
    return m_table[mid].end ();
  }

  static const ExpressionMethodTable *method_table_by_class (const gsi::ClassBase *cls_decl)
  {
    const ExpressionMethodTable *mt = dynamic_cast<const ExpressionMethodTable *>(cls_decl->gsi_data ());
    tl_assert (mt != 0);
    return mt;
  }

  static void initialize_class (const gsi::ClassBase *cls_decl)
  {
    ExpressionMethodTable *mtnc = new ExpressionMethodTable (cls_decl);
    cls_decl->set_gsi_data (mtnc);
  }

private:
  const gsi::ClassBase *mp_cls_decl;
  std::map<std::pair<bool, std::string>, size_t> m_name_map;
  std::vector<ExpressionMethodTableEntry> m_table;

  /**
   *  @brief Adds the given method with the given name to the list of methods registered under that name
   */
  void add_method (const std::string &name, const gsi::MethodBase *mb) 
  {
    bool st = mb->is_static ();

    std::map<std::pair<bool, std::string>, size_t>::iterator n = m_name_map.find (std::make_pair (st, name));
    if (n == m_name_map.end ()) {

      m_name_map.insert (std::make_pair (std::make_pair(st, name), m_table.size ()));
      m_table.push_back (ExpressionMethodTableEntry (name));
      m_table.back ().add (mb);

    } else {

      m_table [n->second].add (mb);

    }
  }

  /**
   *  @brief Private ctor - no construction from the outside
   */
  ExpressionMethodTable ();

  /**
   *  @brief Private ctor - no construction from the outside
   *  This constructor will create the method table for the given class.
   */
  ExpressionMethodTable (const gsi::ClassBase *cls_decl)
    : mp_cls_decl (cls_decl)
  { 
    for (gsi::ClassBase::method_iterator m = cls_decl->begin_methods (); m != cls_decl->end_methods (); ++m) {

      if (! (*m)->is_callback ()) {

        for (gsi::MethodBase::synonym_iterator syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms (); ++syn) {
          if (syn->is_setter) {
            add_method (syn->name + "=", *m);
          } else if (syn->name == "*!") {
            //  non-commutative multiplication
            add_method ("*", *m);
          } else {
            add_method (syn->name, *m);
          }
        }

      }

    }

    //  do some cleanup
    for (std::vector<ExpressionMethodTableEntry>::iterator m = m_table.begin (); m != m_table.end (); ++m) {
      m->finish ();
    }
  }
};

// -------------------------------------------------------------------

/**
 *  @brief Fetches the final object pointer from a tl::Variant
 */
inline void *get_object (tl::Variant &var)
{
  return var.to_user ();
}

/**
 *  @brief Fetches the object pointer
 *  In contrast to get_object, this function will fetch the pointer
 *  without trying to create the object and without checking whether
 *  it is destroyed already.
 */
void *get_object_raw (tl::Variant &var)
{
  void *obj = 0;
  if (var.type_code () == tl::Variant::t_user) {

    obj = var.native_ptr ();

  } else if (var.type_code () == tl::Variant::t_user_ref) {

    Proxy *p = dynamic_cast<Proxy *> (var.to_object ());
    if (p) {
      obj = p->raw_obj ();
    }

  }
  return obj;
}

// ---------------------------------------------------------------------
//  Implementation of initialize_expressions

class EvalClassFunction
  : public tl::EvalFunction
{
public:
  EvalClassFunction (const tl::VariantUserClassBase *var_cls)
    : mp_var_cls (var_cls)
  {
    //  .. nothing yet ..
  }

  bool supports_keyword_parameters () const
  {
    //  for future extensions
    return true;
  }

  void execute (const tl::ExpressionParserContext & /*context*/, tl::Variant &out, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> *kwargs) const
  {
    if (! args.empty () || kwargs) {
      throw tl::Exception (tl::to_string (tr ("Class '%s' is not a function - use 'new' to create a new object")), mp_var_cls->name ());
    }
    out = tl::Variant ((void *) 0, mp_var_cls, false);
  }

private:
  const tl::VariantUserClassBase *mp_var_cls;
};

void GSI_PUBLIC 
initialize_expressions ()
{
  //  just in case this did not happen yet ...
  gsi::initialize ();

  //  Go through all classes (maybe again)
  std::list<const gsi::ClassBase *> classes = gsi::ClassBase::classes_in_definition_order ();
  for (std::list<const gsi::ClassBase *>::const_iterator c = classes.begin (); c != classes.end (); ++c) {

    if ((*c)->is_external ()) {
      //  skip external classes
      continue;
    } else if ((*c)->declaration () != *c) {
      tl_assert ((*c)->parent () != 0);  //  top-level classes should be merged
      continue;
    }

    //  install the method table:
    ExpressionMethodTable::initialize_class (*c);

    //  register a function that creates a class object (use a function to avoid issues with
    //  late destruction of global variables which the class object is already gone)
    const tl::VariantUserClassBase *cc = (*c)->var_cls_cls ();
    if (cc) {
      tl::Eval::define_global_function ((*c)->name (), new EvalClassFunction (cc));
    }

  }
}

// -------------------------------------------------------------------------
//  VariantUserClassImpl implementation

VariantUserClassImpl::VariantUserClassImpl () 
  : mp_cls (0), mp_self (0), mp_object_cls (0), m_is_const (false)
{ 
  //  .. nothing yet ..
}

void 
VariantUserClassImpl::initialize (const gsi::ClassBase *cls, const tl::VariantUserClassBase *self, const tl::VariantUserClassBase *object_cls, bool is_const)
{
  mp_cls = cls;
  mp_self = self;
  mp_object_cls = object_cls;
  m_is_const = is_const;
}

VariantUserClassImpl::~VariantUserClassImpl () 
{ 
  mp_cls = 0;
}

bool
VariantUserClassImpl::has_method (const std::string &method) const
{
  const gsi::ClassBase *cls = mp_cls;

  while (cls) {
    if (ExpressionMethodTable::method_table_by_class (cls)->find (false, method).first) {
      return true;
    }
    cls = cls->base ();
  }

  return false;
}

bool 
VariantUserClassImpl::equal_impl (void *obj, void *other) const 
{
  if (obj) {

    if (! has_method ("==")) {

      //  No == method - use object identity
      return (void *) this == other;

    } else {

      tl::ExpressionParserContext context;

      tl::Variant out;

      tl::Variant object (obj, mp_object_cls, false);
      std::vector<tl::Variant> vv;
      vv.resize (1, tl::Variant ());
      vv[0].set_user (other, mp_object_cls, false);

      execute_gsi (context, out, object, "==", vv);

      return out.to_bool ();

    }

  } else {
    return false; 
  }
}

bool 
VariantUserClassImpl::less_impl (void *obj, void *other) const 
{
  if (obj) {

    if (! has_method ("<")) {

      //  No < method - use object pointers
      return (void *) this < other;

    } else {

      tl::ExpressionParserContext context;

      tl::Variant out;

      tl::Variant object (obj, mp_object_cls, false);
      std::vector<tl::Variant> vv;
      vv.resize (1, tl::Variant ());
      vv[0].set_user (other, mp_object_cls, false);

      execute_gsi (context, out, object, "<", vv);

      return out.to_bool ();

    }

  } else {
    return false; 
  }
}

std::string 
VariantUserClassImpl::to_string_impl (void *obj) const 
{
  if (obj) {

    if (! has_method ("to_s")) {

      //  no method to convert the object to a string
      return std::string ();

    } else {

      tl::ExpressionParserContext context;

      tl::Variant out;

      tl::Variant object (obj, mp_object_cls, false);
      std::vector<tl::Variant> vv;

      execute_gsi (context, out, object, "to_s", vv);

      return out.to_string ();

    }

  } else {
    return std::string (); 
  }
}

tl::Variant
VariantUserClassImpl::to_variant_impl (void *obj) const
{
  if (obj) {

    if (! has_method ("to_v")) {

      //  no method to convert the object to a string
      return tl::Variant ();

    } else {

      tl::ExpressionParserContext context;

      tl::Variant out;

      tl::Variant object (obj, mp_object_cls, false);
      std::vector<tl::Variant> vv;

      execute_gsi (context, out, object, "to_v", vv);

      return out;

    }

  } else {
    return tl::Variant ();
  }
}

int
VariantUserClassImpl::to_int_impl (void *obj) const
{
  if (obj) {

    if (! has_method ("to_i")) {

      //  no method to convert the object to an integer
      return 0;

    } else {

      tl::ExpressionParserContext context;

      tl::Variant out;

      tl::Variant object (obj, mp_object_cls, false);
      std::vector<tl::Variant> vv;

      execute_gsi (context, out, object, "to_i", vv);

      return out.to_int ();

    }

  } else {
    return 0;
  }
}

double
VariantUserClassImpl::to_double_impl (void *obj) const
{
  if (obj) {

    if (! has_method ("to_f")) {

      //  no method to convert the object to a double value
      return 0.0;

    } else {

      tl::ExpressionParserContext context;

      tl::Variant out;

      tl::Variant object (obj, mp_object_cls, false);
      std::vector<tl::Variant> vv;

      execute_gsi (context, out, object, "to_f", vv);

      return out.to_double ();

    }

  } else {
    return 0.0;
  }
}

void
VariantUserClassImpl::execute (const tl::ExpressionParserContext &context, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> *kwargs) const
{
  if (mp_object_cls == 0 && method == "is_a") {

    if (args.size () != 1 || kwargs) {
      throw tl::EvalError (tl::to_string (tr ("'is_a' method requires exactly one argument (no keyword arguments)")), context);
    }

    bool ret = false;
    if (args [0].is_user ()) {
      const tl::VariantUserClassBase *ub = args [0].user_cls ();
      if (ub && ub->gsi_cls () == mp_cls) {
        ret = true;
      }
    }

    out = ret;

  } else if (mp_object_cls != 0 && method == "new" && args.size () == 0 && ! kwargs) {

    void *obj = mp_cls->create ();
    if (obj) {

      if (mp_cls->is_managed ()) {

        Proxy *proxy = new Proxy (mp_cls);
        proxy->set (obj, true, false, true);

        //  gsi::Object based objects are managed through a Proxy and
        //  shared pointers within tl::Variant. That means: copy by reference.
        out.set_user_ref (proxy, mp_object_cls, true);

      } else {
        out.set_user (obj, mp_object_cls, true);
      }

    } else {
      out.reset ();
    }

  } else if (mp_object_cls == 0 && method == "dup") {

    if (args.size () != 0 || kwargs) {
      throw tl::EvalError (tl::to_string (tr ("'dup' method does not allow arguments (no keyword arguments)")), context);
    }

    void *obj = mp_cls->create ();
    if (obj) {

      mp_cls->assign (obj, get_object (object));

      if (mp_cls->is_managed ()) {

        Proxy *proxy = new Proxy (mp_cls);
        proxy->set (obj, true, false, true);

        //  gsi::Object based objects are managed through a Proxy and
        //  shared pointers within tl::Variant. That mean: copy by reference.
        out.set_user_ref (proxy, mp_cls->var_cls (false), true);

      } else {
        out.set_user (obj, mp_cls->var_cls (false), true);
      }

    } else {
      out.reset ();
    }

  } else {
    try {
      execute_gsi (context, out, object, method, args, kwargs);
    } catch (tl::EvalError &) {
      throw;
    } catch (tl::Exception &ex) {
      throw tl::EvalError (ex.msg (), context);
    }
  }
}

static tl::Variant
special_method_impl (gsi::MethodBase::special_method_type smt, tl::Variant &self, const std::vector<tl::Variant> &args)
{
  if (smt == gsi::MethodBase::Destroy) {
    self.user_destroy ();
  } else if (smt == gsi::MethodBase::Keep) {
    //  nothing to do here for GSI objects
  } else if (smt == gsi::MethodBase::Release) {
    //  nothing to do here for GSI objects
  } else if (smt == gsi::MethodBase::Create) {
    //  nothing to do here for GSI objects
  } else if (smt == gsi::MethodBase::IsConst) {
    return tl::Variant (self.user_is_const ());
  } else if (smt == gsi::MethodBase::Destroyed) {

    if (self.type_code () == tl::Variant::t_user) {
      return self.to_user () == 0;
    } else if (self.type_code () == tl::Variant::t_user_ref) {
      Proxy *proxy = dynamic_cast<Proxy *> (self.to_object ());
      if (proxy) {
        return proxy->destroyed ();
      }
    }

    return true;

  } else if (smt == gsi::MethodBase::Assign) {
    tl_assert (args.size () == 1);
    if (!args.front ().is_user () || self.user_cls () != args.front ().user_cls ()) {
      throw tl::Exception (tl::to_string (tr ("Source and target object must be of the same type for assignment")));
    }
    self.user_assign (args.front ());
  } else if (smt == gsi::MethodBase::Dup) {
    return self.user_dup ();
  }

  return tl::Variant ();
}

static std::pair<const ExpressionMethodTable *, size_t> find_method (const gsi::ClassBase *cls, bool as_static, const std::string &method)
{
  const ExpressionMethodTable *mt = 0;
  size_t mid = 0;

  while (cls) {

    mt = ExpressionMethodTable::method_table_by_class (cls);
    std::pair<bool, size_t> t = mt->find (as_static, method);
    if (t.first) {
      mid = t.second;
      return std::make_pair (mt, mid);
    }

    //  try unnamed child classes as static
    for (auto cc = cls->begin_child_classes (); cc != cls->end_child_classes (); ++cc) {
      if (cc->name ().empty ()) {
        std::pair<const ExpressionMethodTable *, size_t> m = find_method (cc->declaration (), true, method);
        if (m.first) {
          return m;
        }
      }
    }

    cls = cls->base ();

  }

  return std::make_pair ((const ExpressionMethodTable *) 0, size_t (0));
}

static const gsi::ClassBase *find_class_scope (const gsi::ClassBase *cls, const std::string &name)
{
  while (cls) {

    //  try named child classes
    for (auto cc = cls->begin_child_classes (); cc != cls->end_child_classes (); ++cc) {
      if (cc->name () == name) {
        return cc->declaration ();
      }
    }

    //  try unnamed child classes as additional bases
    for (auto cc = cls->begin_child_classes (); cc != cls->end_child_classes (); ++cc) {
      if (cc->name ().empty ()) {
        const gsi::ClassBase *scope = find_class_scope (cc->declaration (), name);
        if (scope) {
          return scope;
        }
      }
    }

    cls = cls->base ();

  }

  return 0;
}

inline int
num_args (const gsi::MethodBase *m)
{
  return int (m->end_arguments () - m->begin_arguments ());
}

std::set<std::string>
invalid_kwnames (const gsi::MethodBase *meth, const std::map<std::string, tl::Variant> *kwargs)
{
  std::set<std::string> valid_names;
  for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); a != meth->end_arguments (); ++a) {
    valid_names.insert (a->spec ()->name ());
  }

  std::set<std::string> invalid_names;
  for (auto i = kwargs->begin (); i != kwargs->end (); ++i) {
    if (valid_names.find (i->first) == valid_names.end ()) {
      invalid_names.insert (i->first);
    }
  }

  return invalid_names;
}

static bool
compatible_with_args (const gsi::MethodBase *m, int argc, const std::map<std::string, tl::Variant> *kwargs, std::string *why_not = 0)
{
  int nargs = num_args (m);
  int nkwargs = kwargs ? int (kwargs->size ()) : 0;

  if (argc > nargs) {
    if (why_not) {
      *why_not = tl::sprintf (tl::to_string (tr ("%d argument(s) expected, but %d given")), nargs, argc);
    }
    return false;
  } else if (argc == nargs) {
    //  no more arguments to consider
    if (nkwargs > 0) {
      if (why_not) {
        *why_not = tl::to_string (tr ("all arguments given, but additional keyword arguments specified"));
      }
      return false;
    } else {
      return true;
    }
  }

  if (kwargs) {

    int kwargs_taken = 0;

    while (argc < nargs) {
      const gsi::ArgType &atype = m->begin_arguments () [argc];
      auto i = kwargs->find (atype.spec ()->name ());
      if (i == kwargs->end ()) {
        if (! atype.spec ()->has_default ()) {
          if (why_not) {
            *why_not = tl::sprintf (tl::to_string (tr ("no argument specified for '%s' (neither positional or keyword)")), atype.spec ()->name ());
          }
          return false;
        }
      } else {
        ++kwargs_taken;
      }
      ++argc;
    }

    //  matches if all keyword arguments are taken
    if (kwargs_taken != nkwargs) {
      if (why_not) {
        std::set<std::string> invalid_names = invalid_kwnames (m, kwargs);
        if (invalid_names.size () > 1) {
          std::string names_str = tl::join (invalid_names.begin (), invalid_names.end (), ", ");
          *why_not = tl::to_string (tr ("unknown keyword parameters: ")) + names_str;
        } else if (invalid_names.size () == 1) {
          *why_not = tl::to_string (tr ("unknown keyword parameter: ")) + *invalid_names.begin ();
        }
      }
      return false;
    } else {
      return true;
    }

  } else {

    while (argc < nargs) {
      const gsi::ArgType &atype = m->begin_arguments () [argc];
      if (! atype.spec ()->has_default ()) {
        if (why_not) {
          if (argc < nargs - 1 && ! m->begin_arguments () [argc + 1].spec ()->has_default ()) {
            *why_not = tl::sprintf (tl::to_string (tr ("no value given for argument #%d and following")), argc + 1);
          } else {
            *why_not = tl::sprintf (tl::to_string (tr ("no value given for argument #%d")), argc + 1);
          }
        }
        return false;
      }
      ++argc;
    }

    return true;

  }
}

static std::string
describe_overload (const gsi::MethodBase *m, int argc, const std::map<std::string, tl::Variant> *kwargs)
{
  std::string res = m->to_string ();
  std::string why_not;
  if (compatible_with_args (m, argc, kwargs, &why_not)) {
    res += " " + tl::to_string (tr ("[match candidate]"));
  } else if (! why_not.empty ()) {
    res += " [" + why_not + "]";
  }
  return res;
}

static std::string
describe_overloads (const ExpressionMethodTable *mt, int mid, int argc, const std::map<std::string, tl::Variant> *kwargs)
{
  std::string res;
  for (auto m = mt->begin (mid); m != mt->end (mid); ++m) {
    res += std::string ("  ") + describe_overload (*m, argc, kwargs) + "\n";
  }
  return res;
}

static const tl::Variant *
get_kwarg (const gsi::ArgType &atype, const std::map<std::string, tl::Variant> *kwargs)
{
  if (kwargs) {
    auto i = kwargs->find (atype.spec ()->name ());
    if (i != kwargs->end ()) {
      return &i->second;
    }
  }
  return 0;
}

void
VariantUserClassImpl::execute_gsi (const tl::ExpressionParserContext & /*context*/, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> *kwargs) const
{
  tl_assert (object.is_user ());

  const gsi::ClassBase *clsact = mp_cls;
  if (clsact) {
    //  determine the real class of the object (it may be a subclass)
    void *obj = get_object_raw (object);
    if (obj) {
      clsact = clsact->subclass_decl (obj);
    }
  }

  auto m = find_method (clsact, mp_object_cls != 0 /*static*/, method);

  const ExpressionMethodTable *mt = m.first;
  size_t mid = m.second;

  if (! mt) {

    //  try class scope
    const gsi::ClassBase *scope = find_class_scope (clsact, method);
    if (scope) {

      if (! args.empty ()) {
        throw tl::Exception (tl::to_string (tr ("'%s' is not a function and cannot have parameters")), method);
      }

      //  we found a class scope: return a reference to that
      const tl::VariantUserClassBase *scope_var_cls = scope->var_cls_cls ();
      if (scope_var_cls) {
        out = tl::Variant ((void *) 0, scope_var_cls, false);
      } else {
        out = tl::Variant ();
      }
      return;

    } else {
      throw tl::Exception (tl::to_string (tr ("Unknown method '%s' of class '%s'")), method, clsact->name ());
    }

  }

  const gsi::MethodBase *meth = 0;
  int candidates = 0;

  for (ExpressionMethodTableEntry::method_iterator m = mt->begin (mid); m != mt->end (mid); ++m) {

    if ((*m)->is_signal()) {
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("Signals are not supported inside expressions (event %s)")), method.c_str ()));
    } else if ((*m)->is_callback()) {
      //  ignore callbacks
    } else if (compatible_with_args (*m, int (args.size ()), kwargs)) {
      ++candidates;
      meth = *m;
    }

  }

  //  no candidate -> error
  if (! meth) {
    throw tl::Exception (tl::to_string (tr ("Can't match arguments. Variants are:\n")) + describe_overloads (mt, mid, int (args.size ()), kwargs));
  }

  //  more than one candidate -> refine by checking the arguments
  if (candidates > 1) {

    meth = 0;
    candidates = 0;
    int score = 0;
    bool const_matching = true;

    for (ExpressionMethodTableEntry::method_iterator m = mt->begin (mid); m != mt->end (mid); ++m) {

      if (! (*m)->is_callback () && ! (*m)->is_signal ()) {

        //  check arguments (count and type)
        bool is_valid = compatible_with_args (*m, (int) args.size (), kwargs);
        int sc = 0;
        int i = 0;
        for (gsi::MethodBase::argument_iterator a = (*m)->begin_arguments (); is_valid && a != (*m)->end_arguments (); ++a, ++i) {
          const tl::Variant *arg = i >= int (args.size ()) ? get_kwarg (*a, kwargs) : &args[i];
          if (! arg) {
            is_valid = a->spec ()->has_default ();
          } else if (gsi::test_arg (*a, *arg, false /*strict*/)) {
            ++sc;
          } else if (test_arg (*a, *arg, true /*loose*/)) {
            //  non-scoring match
          } else {
            is_valid = false;
          }
        }

        if (is_valid) {

          //  constness matching candidates have precedence
          if ((*m)->is_const () != m_is_const) {
            if (const_matching && candidates > 0) {
              is_valid = false;
            } else {
              const_matching = false;
            }
          } else if (! const_matching) {
            const_matching = true;
            candidates = 0;
          }

        }

        if (is_valid) {

          //  otherwise take the candidate with the better score or the least number of arguments (faster)
          if (candidates > 0) {
            if (sc > score || (sc == score && num_args (meth) > num_args (*m))) {
              candidates = 1;
              meth = *m;
              score = sc;
            } else if (sc == score && num_args (meth) == num_args (*m)) {
              ++candidates;
              meth = *m;
            }
          } else {
            ++candidates;
            meth = *m;
            score = sc;
          }

        }

      }

    }

  }

  if (! meth) {
    throw tl::Exception (tl::to_string (tr ("No overload with matching arguments. Variants are:\n")) + describe_overloads (mt, mid, int (args.size ()), kwargs));
  }

  if (candidates > 1) {
    throw tl::Exception (tl::to_string (tr ("Ambiguous overload variants - multiple method declarations match arguments. Variants are:\n")) + describe_overloads (mt, mid, int (args.size ()), kwargs));
  }

  if (m_is_const && ! meth->is_const ()) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Cannot call non-const method %s, class %s on a const reference")), method.c_str (), mp_cls->name ()));
  }

  if (meth->is_signal ()) {

    //  TODO: events not supported yet
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Signals not supported yet (method %s, class %s)")), method.c_str (), mp_cls->name ()));

  } else if (meth->smt () != gsi::MethodBase::None) {

    if (kwargs) {
      throw tl::Exception (tl::to_string (tr ("Keyword arguments not permitted")));
    }

    out = special_method_impl (meth->smt (), object, args);

  } else {

    gsi::SerialArgs arglist (meth->argsize ());
    tl::Heap heap;

    int iarg = 0;
    int kwargs_taken = 0;
    int nkwargs = kwargs ? int (kwargs->size ()) : 0;

    for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); a != meth->end_arguments (); ++a, ++iarg) {

      try {

        const tl::Variant *arg = iarg >= int (args.size ()) ? get_kwarg (*a, kwargs) : &args[iarg];
        if (! arg) {
          if (a->spec ()->has_default ()) {
            if (kwargs_taken == nkwargs) {
              //  leave it to the consumer to establish the default values (that is faster)
              break;
            }
            const tl::Variant &def_value = a->spec ()->default_value ();
            //  NOTE: this const_cast means we need to take care that we do not use default values on "out" parameters.
            //  Otherwise there is a chance we will modify the default value.
            gsi::push_arg (arglist, *a, const_cast<tl::Variant &> (def_value), &heap);
          } else {
            throw tl::Exception (tl::to_string ("No argument provided (positional or keyword) and no default value available"));
          }
        } else {
          if (iarg >= int (args.size ())) {
            ++kwargs_taken;
          }
          //  Note: this const_cast is ugly, but it will basically enable "out" parameters
          //  TODO: clean this up.
          gsi::push_arg (arglist, *a, const_cast<tl::Variant &> (*arg), &heap);
        }

      } catch (tl::Exception &ex) {
        std::string msg = ex.msg () + tl::sprintf (tl::to_string (tr (" (argument '%s')")), a->spec ()->name ());
        throw tl::Exception (msg);
      }

    }

    if (kwargs_taken != nkwargs) {

      //  check if there are any left-over keyword parameters with unknown names
      std::set<std::string> invalid_names = invalid_kwnames (meth, kwargs);
      if (invalid_names.size () > 1) {
        std::string names_str = tl::join (invalid_names.begin (), invalid_names.end (), ", ");
        throw tl::Exception (tl::to_string (tr ("Unknown keyword parameters: ")) + names_str);
      } else if (invalid_names.size () == 1) {
        throw tl::Exception (tl::to_string (tr ("Unknown keyword parameter: ")) + *invalid_names.begin ());
      }

    }

    SerialArgs retlist (meth->retsize ());

    meth->call (get_object (object), arglist, retlist);

    if (meth->ret_type ().is_iter ()) {
      //  TODO: iterators not supported yet
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("Iterators not supported yet (method %s, class %s)")), method.c_str (), mp_cls->name ()));
    } else {
      out = tl::Variant ();
      try {
        gsi::pull_arg (retlist, meth->ret_type (), out, &heap);
      } catch (tl::Exception &ex) {
        std::string msg = ex.msg () + tl::to_string (tr (" (return value)"));
        throw tl::Exception (msg);
      }
    }

  }
}

}

