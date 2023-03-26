
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


#include "gsiDecl.h"
#include "tlLog.h"
#include "tlAssert.h"

#include <cstdio>
#include <set>

namespace gsi
{

// -------------------------------------------------------------------------
//  ClassBase implementation

ClassBase::class_collection *ClassBase::mp_class_collection = 0;
ClassBase::class_collection *ClassBase::mp_new_class_collection = 0;

namespace {
  struct type_info_compare
  {
    bool operator() (const std::type_info *a, const std::type_info *b) const
    {
      return a->before (*b);
    }
  };
}

//  TODO: thread-safe? Unlikely that multiple threads access this member -
//  we do a initial scan and after this no more write access here.
static std::vector<const ClassBase *> *sp_classes = 0;
typedef std::map<const ClassBase *, size_t> class_to_index_map_t;
static class_to_index_map_t *sp_class_to_index = 0;
typedef std::map<const std::type_info *, size_t> ti_to_class_map_t;
static ti_to_class_map_t *sp_ti_to_class_index = 0;
typedef std::map<std::string, const ClassBase *> tname_to_class_map_t;
static tname_to_class_map_t *sp_tname_to_class = 0;

ClassBase::ClassBase (const std::string &doc, const Methods &mm, bool do_register)
  : m_initialized (false), mp_base (0), mp_parent (0), m_doc (doc), m_methods (mm)
{ 
  if (do_register) {

    //  enter the class into the "new" collection
    if (! mp_new_class_collection) {
      mp_new_class_collection = new class_collection ();
    }
    mp_new_class_collection->push_back (this);

    //  invalidate the "typeinfo to class" map
    if (sp_classes) {
      delete sp_classes;
      sp_classes = 0;
    }
    if (sp_class_to_index) {
      delete sp_class_to_index;
      sp_class_to_index = 0;
    }
    if (sp_ti_to_class_index) {
      delete sp_ti_to_class_index;
      sp_ti_to_class_index = 0;
    }
    if (sp_tname_to_class) {
      delete sp_tname_to_class;
      sp_tname_to_class = 0;
    }

  }
}

ClassBase::~ClassBase ()
{
  //  .. nothing yet ..
}

void
ClassBase::set_parent (const ClassBase *p)
{
  if (mp_parent != p) {
    mp_parent = p;
    m_initialized = false;
  }
}

void
ClassBase::set_base (const ClassBase *b)
{
  if (mp_base != b) {
    mp_base = b;
    m_initialized = false;
  }
}

std::string 
ClassBase::qname () const
{
  std::string qn = name ();
  const gsi::ClassBase *p = this;
  while (p->parent ()) {
    p = p->parent ();
    qn = p->name () + "::" + qn;
  }
  return qn;
}

void
ClassBase::add_subclass (const ClassBase *cls)
{
  //  TODO: ugly const_cast hack
  ClassBase *non_const_cls = const_cast<ClassBase *> (cls);
  m_subclasses.push_back (non_const_cls);
  m_initialized = false;
}

void
ClassBase::add_child_class (const ClassBase *cls)
{
  //  TODO: ugly const_cast hack
  ClassBase *non_const_cls = const_cast<ClassBase *> (cls);
  non_const_cls->set_parent (this);
  //  child classes inherit the module of their parent
  non_const_cls->set_module (module ());
  m_child_classes.push_back (non_const_cls);
  m_initialized = false;
}

bool 
ClassBase::is_derived_from (const ClassBase *base) const
{
  if (! base) {
    return false;
  } else if (base == this) {
    return true;
  } else if (!mp_base) {
    return false;
  } else {
    return mp_base->is_derived_from (base);
  }
}

/**
 *  @brief This function returns true if the given method m of target can convert an object of type from into target.
 */
static bool 
is_constructor_of (const ClassBase *target, const MethodBase *m, const ClassBase *from)
{
  if (m->ret_type ().cls () != target) {
    //  the return type has to be a new'd pointer of the right type
    return false;
  }
  if (! m->compatible_with_num_args (1)) {
    //  the constructor has to accept one argument
    return false;
  }

  const ArgType &a0 = m->arg (0);

  if (! a0.cls () || ! from->is_derived_from (a0.cls ())) {
    //  the single argument must be of the right type
    return false;
  }

  //  And finally the argument must be of const ref or direct type
  if (a0.is_cref ()) {
    return true;
  } else if (! a0.is_ptr () && ! a0.is_cptr () && ! a0.is_ref ()) {
    return true;
  } else {
    return false;
  }
}

bool 
ClassBase::can_convert_to (const ClassBase *target) const
{
  for (method_iterator m = target->begin_constructors (); m != target->end_constructors (); ++m) {
    if (is_constructor_of (target, *m, this)) {
      return true;
    }
  }
  return false;
}

void *
ClassBase::create_obj_from (const ClassBase *from, void *obj) const
{
  const MethodBase *ctor = 0;

  for (method_iterator m = begin_constructors (); m != end_constructors (); ++m) {
    if (is_constructor_of (this, *m, from)) {
      if (ctor) {
        throw tl::Exception (tl::to_string (tr ("There are multiple conversion constructors available to convert object of type %s to type %s")), from->name (), name ());
      }
      ctor = *m;
    }
  }

  tl_assert (ctor != 0);

  SerialArgs ret (ctor->retsize ());

  SerialArgs args (ctor->argsize ());
  if (ctor->arg (0).is_cref ()) {
    args.write<void *> (obj);
  } else {
    //  direct type objects will transfer ownership to the caller by convention
    args.write<void *> (from->clone (obj));
  }

  ctor->call (0, args, ret);

  tl::Heap heap;
  return ret.read<void *> (heap);
}

const ClassBase::class_collection &
ClassBase::collection () 
{
  if (!mp_class_collection) {
    static const class_collection empty;
    return empty;
  } else {
    return *mp_class_collection;
  }
}

const ClassBase::class_collection &
ClassBase::new_collection ()
{
  if (!mp_new_class_collection) {
    static const class_collection empty;
    return empty;
  } else {
    return *mp_new_class_collection;
  }
}

static SpecialMethod *
sm_default_ctor (const char *name, const gsi::ClassBase *cls)
{
  SpecialMethod *sm = new SpecialMethod (name,
    tl::to_string (tr ("@brief Creates a new object of this class\n")),
    false,   //  non-const
    true,    //  static
    MethodBase::DefaultCtor);

  gsi::ArgType ret;
  ret.set_is_ptr (true);
  ret.set_type (gsi::T_object);
  ret.set_pass_obj (true);
  ret.set_cls (cls);
  sm->set_return (ret);

  return sm;
}

static SpecialMethod *
sm_destroy (const char *name)
{
  SpecialMethod *sm = new SpecialMethod (name,
    tl::to_string (tr ("@brief Explicitly destroys the object\nExplicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.\n"
                       "If the object is not owned by the script, this method will do nothing.")),
    false,   //  non-const
    false,   //  non-static
    MethodBase::Destroy);

  return sm;
}

static SpecialMethod *
sm_create (const char *name)
{
  SpecialMethod *sm = new SpecialMethod (name,
    tl::to_string (tr ("@brief Ensures the C++ object is created\n"
                       "Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. "
                       "Usually C++ objects are created on demand and not necessarily when the script object is created.")),
    false,   //  non-const
    false,   //  non-static
    MethodBase::Create);

  return sm;
}

static SpecialMethod *
sm_keep (const char *name)
{
  SpecialMethod *sm = new SpecialMethod (name,
    tl::to_string (tr ("@brief Marks the object as no longer owned by the script side.\n"
                       "Calling this method will make this object no longer owned by the script's memory management. "
                       "Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. "
                       "Technically speaking, this method will turn the script's reference into a weak reference. "
                       "After the script engine decides to delete the reference, the object itself will still exist. "
                       "If the object is not managed otherwise, memory leaks will occur.\n\n"
                       "Usually it's not required to call this method. It has been introduced in version 0.24.")),
    false,   //  non-const
    false,   //  non-static
    MethodBase::Keep);

  return sm;
}

static SpecialMethod *
sm_release (const char *name)
{
  SpecialMethod *sm = new SpecialMethod (name,
    tl::to_string (tr ("@brief Marks the object as managed by the script side.\n"
                       "After calling this method on an object, the script side will be responsible for the management of the object. "
                       "This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. "
                       "If necessary, the script side may delete the object if the script's reference is no longer required.\n\n"
                       "Usually it's not required to call this method. It has been introduced in version 0.24.")),
    false,   //  non-const
    false,   //  non-static
    MethodBase::Release);

  return sm;
}

static SpecialMethod *
sm_is_const (const char *name)
{
  SpecialMethod *sm = new SpecialMethod (name,
    tl::to_string (tr ("@brief Returns a value indicating whether the reference is a const reference\nThis method returns true, if self is a const reference.\n"
                       "In that case, only const methods may be called on self.")),
    true,    //  const
    false,   //  non-static
    MethodBase::IsConst);

  gsi::ArgType ret;
  ret.set_type (gsi::T_bool);
  sm->set_return (ret);

  return sm;
}

static SpecialMethod *
sm_destroyed (const char *name)
{
  SpecialMethod *sm = new SpecialMethod (name,
    tl::to_string (tr ("@brief Returns a value indicating whether the object was already destroyed\nThis method returns true, if the object was destroyed, either explicitly or by the C++ side.\n"
                       "The latter may happen, if the object is owned by a C++ object which got destroyed itself.")),
    true,    //  const
    false,   //  non-static
    MethodBase::Destroyed);

  gsi::ArgType ret;
  ret.set_type (gsi::T_bool);
  sm->set_return (ret);

  return sm;
}

static SpecialMethod *
sm_dup (const char *name, const gsi::ClassBase *cls)
{
  SpecialMethod *sm = new SpecialMethod (name,
    tl::to_string (tr ("@brief Creates a copy of self\n")),
    true,    //  const
    false,   //  non-static
    MethodBase::Dup);

  gsi::ArgType ret;
  ret.set_is_ptr (true);
  ret.set_type (gsi::T_object);
  ret.set_pass_obj (true);
  ret.set_cls (cls);
  sm->set_return (ret);

  return sm;
}

static SpecialMethod *
sm_assign (const char *name, const gsi::ClassBase *cls)
{
  SpecialMethod *sm = new SpecialMethod (name,
    tl::to_string (tr ("@brief Assigns another object to self")),
    false,   //  non-const
    false,   //  non-static
    MethodBase::Assign);

  gsi::ArgType a;
  a.init<void> (new gsi::ArgSpecBase ("other"));
  a.set_is_cref (true);
  a.set_type (gsi::T_object);
  a.set_cls (cls);
  sm->add_arg (a);

  return sm;
}

static const std::set<std::pair<std::string, bool> > &name_map_for_class (const gsi::ClassBase *cls, std::map<const gsi::ClassBase *, std::set<std::pair<std::string, bool> > > &cache)
{
  if (! cls) {
    static std::set<std::pair<std::string, bool> > empty;
    return empty;
  }

  std::map<const gsi::ClassBase *, std::set<std::pair<std::string, bool> > >::iterator cc = cache.find (cls);
  if (cc != cache.end ()) {
    return cc->second;
  }

  cc = cache.insert (std::make_pair ((const gsi::ClassBase *) 0, std::set<std::pair<std::string, bool> > ())).first;
  cc->second = name_map_for_class (cls->base (), cache);

  for (gsi::ClassBase::method_iterator m = cls->begin_methods (); m != cls->end_methods (); ++m) {
    for (gsi::MethodBase::synonym_iterator syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms (); ++syn) {
      cc->second.insert (std::make_pair (syn->name, (*m)->is_static ()));
    }
  }

  return cc->second;
}

#if defined(_DEBUG)
static std::string type_signature (const gsi::ArgType &t)
{
  gsi::ArgType tr (t);
  tr.set_is_ptr (false);
  tr.set_is_ref (false);
  tr.set_is_cptr (false);
  tr.set_is_cref (false);
  return tr.to_string ();
}

static std::string signature (const gsi::MethodBase *m, const gsi::MethodBase::MethodSynonym &s)
{
  std::string res;

  if (m->is_static ()) {
    res += "static ";
  }

  res += type_signature (m->ret_type ());
  res += " ";
  res += s.name;
  if (s.is_predicate) {
    res += "?";
  }
  if (s.is_setter) {
    res += "=";
  }

  res += "(";
  for (gsi::MethodBase::argument_iterator a = m->begin_arguments (); a != m->end_arguments (); ++a) {
    if (a != m->begin_arguments ()) {
      res += ", ";
    }
    res += type_signature (*a);
  }

  res += ")";

  if (m->is_const ()) {
    res += " const";
  }

  return res;
}
#endif

void
ClassBase::merge_declarations ()
{
  if (gsi::ClassBase::begin_new_classes () == gsi::ClassBase::end_new_classes ()) {
    //  Nothing to do.
    return;
  }

  //  Check for duplicate declarations
  std::set<const std::type_info *> types;
  std::set<std::string> names;
  for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {
    if (c->declaration () == &*c && !types.insert (&c->type ()).second) {
      tl::warn << "Duplicate GSI declaration of type " << c->type ().name ();
    }
    if (c->declaration () == &*c && !names.insert (c->declaration ()->name ()).second) {
      tl::warn << "Duplicate GSI declaration of name " << c->declaration ()->name ();
    }
  }

  std::vector <const gsi::ClassBase *> to_remove;

  //  Consolidate the classes (merge, remove etc.)
  for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_new_classes (); c != gsi::ClassBase::end_new_classes (); ++c) {
    if (! c->consolidate ()) {
      to_remove.push_back (&*c);
    }
  }

  //  removed the classes which are no longer required
  for (std::vector <const gsi::ClassBase *>::const_iterator ed = to_remove.begin (); ed != to_remove.end (); ++ed) {
    //  TODO: ugly const_cast hack
    mp_new_class_collection->erase (const_cast<gsi::ClassBase *> (*ed));
  }

  //  collect the subclasses of a class
  for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_new_classes (); c != gsi::ClassBase::end_new_classes (); ++c) {
    if (c->base ()) {
      //  TODO: ugly const_cast hack
      const_cast<gsi::ClassBase *> (c->base ())->add_subclass (c.operator-> ());
    }
  }

  std::map<const gsi::ClassBase *, std::set<std::pair<std::string, bool> > > name_maps;

  //  Add to the classes the special methods and clean up the method table
  for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_new_classes (); c != gsi::ClassBase::end_new_classes (); ++c) {

    //  Skip external classes (i.e. provided by Ruby or Python)
    if (c->is_external ()) {
      continue;
    }

    const std::set<std::pair<std::string, bool> > &name_map = name_map_for_class (c.operator-> (), name_maps);

    //  We don't want the declaration object to be non-const except for this case. So
    //  we const_cast here.
    gsi::ClassBase *non_const_decl = const_cast<gsi::ClassBase *> (c.operator-> ());

    if (name_map.find (std::make_pair ("new", true)) == name_map.end ()) {
      non_const_decl->add_method (sm_default_ctor ("new", &*c), false);
    }

    //  Note: "unmanage" and "manage" is a better name ...
    non_const_decl->add_method (sm_keep ("_unmanage"));
    non_const_decl->add_method (sm_release ("_manage"));

    if (name_map.find (std::make_pair ("create", false)) == name_map.end ()) {
      //  deprecate "create"
      non_const_decl->add_method (sm_create ("_create|#create"));
    } else {
      //  fallback name is "_create" to avoid conflicts
      non_const_decl->add_method (sm_create ("_create"));
    }

    if (c->can_destroy ()) {
      if (name_map.find (std::make_pair ("destroy", false)) == name_map.end ()) {
        //  deprecate "destroy"
        non_const_decl->add_method (sm_destroy ("_destroy|#destroy"));
      } else {
        //  fallback name is "_destroy" to avoid conflicts
        non_const_decl->add_method (sm_destroy ("_destroy"));
      }
    }

    if (c->can_copy ()) {

      if (name_map.find (std::make_pair ("dup", false)) == name_map.end ()) {
        non_const_decl->add_method (sm_dup ("dup", &*c));
      } else {
        //  fallback name is "_dup" to avoid conflicts
        non_const_decl->add_method (sm_dup ("_dup", &*c));
      }

      if (name_map.find (std::make_pair ("assign", false)) == name_map.end ()) {
        non_const_decl->add_method (sm_assign ("assign", &*c));
      } else {
        //  fallback name is "_assign" to avoid conflicts
        non_const_decl->add_method (sm_assign ("_assign", &*c));
      }

    }

    if (name_map.find (std::make_pair ("destroyed", false)) == name_map.end ()) {
      //  deprecate "destroyed"
      non_const_decl->add_method (sm_destroyed ("_destroyed?|#destroyed?"));
    } else {
      //  fallback name is "_destroyed" to avoid conflicts
      non_const_decl->add_method (sm_destroyed ("_destroyed?"));
    }

    if (name_map.find (std::make_pair ("is_const_object", false)) == name_map.end ()) {
      //  deprecate "is_const"
      non_const_decl->add_method (sm_is_const ("_is_const_object?|#is_const_object?"));
    } else {
      //  fallback name is "_is_const" to avoid conflicts
      non_const_decl->add_method (sm_is_const ("_is_const_object?"));
    }

  }

  //  finally merge the new classes into the existing ones
  if (! mp_class_collection) {
    mp_class_collection = new class_collection ();
  }
  for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_new_classes (); c != gsi::ClassBase::end_new_classes (); ++c) {
    gsi::ClassBase *non_const_decl = const_cast<gsi::ClassBase *> (c.operator-> ());
    mp_class_collection->push_back (non_const_decl);
  }
  mp_new_class_collection->clear ();

  //  do a full re-initialization - maybe merge_declarations modified existing classes too
  for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {
    //  Initialize the method table once again after we have merged the declarations
    //  TODO: get rid of that const cast
    (const_cast<gsi::ClassBase *> (&*c))->initialize ();

    //  there should be only main declarations since we merged
    //  (the declaration==0 case covers dummy declarations introduced by
    //  lym::ExternalClass)
    tl_assert (! c->declaration () || c->declaration () == &*c);
  }

#if defined(_DEBUG)
  //  do a sanity check
  for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {

    std::map<std::string, int> method_counts;

    for (gsi::ClassBase::method_iterator m = c->begin_methods (); m != c->end_methods (); ++m) {
      if (! (*m)->is_callback ()) {
        for (gsi::MethodBase::synonym_iterator s = (*m)->begin_synonyms (); s != (*m)->end_synonyms (); ++s) {
          method_counts [signature (*m, *s)] += 1;
        }
      }
    }

    for (std::map<std::string, int>::const_iterator mc = method_counts.begin (); mc != method_counts.end (); ++mc) {
      if (mc->second > 1) {
        tl::warn << "Ambiguous method declarations in class " << c->name () << " for method " << mc->first;
      }
    }

  }
#endif

}

static void collect_classes (const gsi::ClassBase *cls, std::list<const gsi::ClassBase *> &unsorted_classes)
{
  unsorted_classes.push_back (cls);

  for (auto cc = cls->begin_child_classes (); cc != cls->end_child_classes (); ++cc) {
    collect_classes (cc.operator-> (), unsorted_classes);
  }
}

static bool all_parts_available (const gsi::ClassBase *cls, const std::set<const gsi::ClassBase *> &taken)
{
  if (cls->declaration () && cls->declaration () != cls && taken.find (cls->declaration ()) == taken.end ()) {
    return false;
  }

  for (auto cc = cls->begin_child_classes (); cc != cls->end_child_classes (); ++cc) {
    if (! all_parts_available (cc.operator-> (), taken)) {
      return false;
    }
  }

  return true;
}

std::list<const gsi::ClassBase *>
ClassBase::classes_in_definition_order (const char *mod_name)
{
  std::set<const gsi::ClassBase *> taken;
  std::list<const gsi::ClassBase *> sorted_classes;

  std::list<const gsi::ClassBase *> unsorted_classes;
  for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {
    if (! mod_name || c->module () == mod_name) {
      //  only handle top-level classed from the requested modules
      //  (children or base classes from outside the module may be part of the returned list!)
      collect_classes (c.operator-> (), unsorted_classes);
    } else {
      //  we assume that these classes are taken by another run (i.e. "import x" in Python)
      taken.insert (c.operator-> ());
    }
  }

  while (! unsorted_classes.empty ()) {

    bool any = false;

    std::list<const gsi::ClassBase *> more_classes;

    for (std::list<const gsi::ClassBase *>::const_iterator c = unsorted_classes.begin (); c != unsorted_classes.end (); ++c) {

      //  don't handle classes twice
      if (taken.find (*c) != taken.end ()) {
        continue;
      }

      if (! all_parts_available (*c, taken)) {
        //  can't produce this class yet - it's a reference to another class which is not produced yet.
        more_classes.push_back (*c);
        continue;
      }

      if ((*c)->parent () != 0 && taken.find ((*c)->parent ()) == taken.end ()) {
        //  can't produce this class yet - it's a child of a parent that is not produced yet.
        more_classes.push_back (*c);
        continue;
      }

      if ((*c)->base () != 0 && taken.find ((*c)->base ()) == taken.end ()) {
        //  can't produce this class yet. The base class needs to be handled first.
        more_classes.push_back (*c);
        continue;
      }

      sorted_classes.push_back (*c);
      taken.insert (*c);
      any = true;

    }

    if (! any && ! more_classes.empty ()) {

      for (std::list<const gsi::ClassBase *>::const_iterator c = more_classes.begin (); c != more_classes.end (); ++c) {

        //  don't handle classes twice
        if (taken.find (*c) != taken.end ()) {
          //  not considered.
        } else if ((*c)->declaration () && (*c)->declaration () != *c && taken.find ((*c)->declaration ()) == taken.end ()) {
          //  can't produce this class yet - it refers to a class which is not available.
          tl::error << tl::sprintf ("class %s.%s refers to another class (%s.%s) which is not available", (*c)->module (), (*c)->name (), (*c)->declaration ()->module (), (*c)->declaration ()->name ());
        } else if ((*c)->parent () != 0 && taken.find ((*c)->parent ()) == taken.end ()) {
          //  can't produce this class yet - it's a child of a parent that is not produced yet.
          tl::error << tl::sprintf ("parent of class %s.%s not available (%s.%s)", (*c)->module (), (*c)->name (), (*c)->parent ()->module (), (*c)->parent ()->name ());
        } else if ((*c)->base () != 0 && taken.find ((*c)->base ()) == taken.end ()) {
          //  can't produce this class yet. The base class needs to be handled first.
          tl::error << tl::sprintf ("base of class %s.%s not available (%s.%s)", (*c)->module (), (*c)->name (), (*c)->base ()->module (), (*c)->base ()->name ());
        }

      }

      //  prevent infinite recursion
      throw tl::Exception ("Internal error: infinite recursion on class building. See error log for analysis");

    }

    unsorted_classes.swap (more_classes);

  }

  return sorted_classes;
}

void
ClassBase::initialize ()
{
  //  don't initialize again
  if (m_initialized) {
    return;
  }

  m_methods.initialize ();

  m_constructors.clear ();
  for (Methods::iterator m = m_methods.begin (); m != m_methods.end (); ++m) {
    if ((*m)->is_constructor ()) {
      m_constructors.push_back (*m);
    }
  }

  m_callbacks.clear ();
  for (Methods::iterator m = m_methods.begin (); m != m_methods.end (); ++m) {
    if ((*m)->is_callback ()) {
      m_callbacks.push_back (*m);
    }
  }

  m_initialized = true;
}

void
ClassBase::add_method (MethodBase *method, bool /*base_class*/)
{
  m_initialized = false;
  m_methods.add_method (method);
}

//  TODO: thread-safe? Unlikely that multiple threads access this member -
//  we do a initial scan and after this no more write access here.
static std::map<std::string, const ClassBase *> s_name_to_class;

const ClassBase *class_by_name_no_assert (const std::string &name)
{
  if (s_name_to_class.empty ()) {

    for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {

      if (c->declaration () != c.operator-> ()) {
        continue;
      }

      if (!s_name_to_class.insert (std::make_pair (c->name (), c.operator-> ())).second) {
        //  Duplicate registration of this class
        tl::error << "Duplicate registration of class " << c->name ();
        tl_assert (false);
      }

    }

  }

  std::map<std::string, const ClassBase *>::const_iterator c = s_name_to_class.find (name);
  if (c != s_name_to_class.end ()) {
    return c->second;
  } else {
    return 0;
  }
}

const ClassBase *class_by_name (const std::string &name)
{
  const ClassBase *cd = class_by_name_no_assert (name);
  if (! cd) {
    tl::error << "No class with name " << name;
    tl_assert (false);
  }
  return cd;
}

bool has_class (const std::string &name)
{
  return class_by_name_no_assert (name) != 0;
}

static void add_class_to_map (const gsi::ClassBase *c)
{
  if (c->declaration () != c || ! c->binds ()) {
    //  only consider non-extensions
    return;
  }

  const std::type_info *ti = c->adapted_type_info ();
  if (! ti) {
    ti = &c->type ();
  }

  if (! sp_classes) {
    sp_classes = new std::vector<const ClassBase *> ();
  }
  if (! sp_class_to_index) {
    sp_class_to_index = new class_to_index_map_t ();
  }
  if (! sp_ti_to_class_index) {
    sp_ti_to_class_index = new ti_to_class_map_t ();
  }
  if (! sp_tname_to_class) {
    sp_tname_to_class = new tname_to_class_map_t ();
  }

  auto c2i = sp_class_to_index->insert (std::make_pair (c, sp_classes->size ())).first;
  if (c2i->second >= sp_classes->size ()) {
    sp_classes->push_back (c);
  }

  if (!sp_ti_to_class_index->insert (std::make_pair (ti, c2i->second)).second) {
    //  Duplicate registration of this class
    tl::error << "Duplicate registration of class " << c->name () << " (type " << ti->name () << ")";
    tl_assert (false);
  } else {
    sp_tname_to_class->insert (std::make_pair (std::string (ti->name ()), c));
  }
}

const ClassBase *class_by_typeinfo_no_assert (const std::type_info &ti)
{
  if (! sp_ti_to_class_index || sp_ti_to_class_index->empty ()) {
    for (auto c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {
      add_class_to_map (c.operator-> ());
    }
    for (auto c = gsi::ClassBase::begin_new_classes (); c != gsi::ClassBase::end_new_classes (); ++c) {
      add_class_to_map (c.operator-> ());
    }
  }

  if (! sp_ti_to_class_index) {
    return 0;
  } else {
    auto c = sp_ti_to_class_index->find (&ti);
    if (c != sp_ti_to_class_index->end ()) {
      return sp_classes->operator[] (c->second);
    } else {
      //  try name lookup
      auto cn = sp_tname_to_class->find (std::string (ti.name ()));
      if (cn != sp_tname_to_class->end ()) {
        //  we can use this typeinfo as alias
        sp_ti_to_class_index->insert (std::make_pair (&ti, sp_class_to_index->operator[] (cn->second)));
        return cn->second;
      } else {
        return 0;
      }
    }
  }
}

const ClassBase *class_by_typeinfo (const std::type_info &ti)
{
  const ClassBase *cd = class_by_typeinfo_no_assert (ti);
  if (! cd) {
    tl::error << "No class with type " << ti.name ();
    tl_assert (false);
  }
  return cd;
}

bool has_class (const std::type_info &ti)
{
  return class_by_typeinfo_no_assert (ti) != 0;
}

}

