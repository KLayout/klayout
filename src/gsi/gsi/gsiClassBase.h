
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


#ifndef _HDR_gsiClassBase
#define _HDR_gsiClassBase

#include "tlObject.h"
#include "tlObjectCollection.h"
#include "tlException.h"
#include "tlExpression.h"
#include "tlAssert.h"
#include "tlHeap.h"

#include "gsiExpression.h"
#include "gsiObject.h"
#include "gsiIterators.h"
#include "gsiCallback.h"
#include "gsiMethods.h"

#include <list>
#include <map>
#include <vector>
#include <typeinfo>
#include <memory>

namespace gsi
{

/**
 *  @brief A base class for client-specific data per class
 *
 *  Objects of this type are used inside the ClassBase object to store 
 *  information specific for certain clients.
 */
class PerClassClientSpecificData
{
public:
  PerClassClientSpecificData ()
  {
    //  .. nothing yet ..
  }

  virtual ~PerClassClientSpecificData()
  {
    //  .. nothing yet ..
  }
};

/**
 *  @brief The basic object to declare a class
 *
 *  This object represents a GSI class. It provides the methods to translate the typeless object
 *  pointer to a real C++ object and to perform some basic operations on the object.
 *  It also provides metainformation such as method declarations and similar.
 */
class GSI_PUBLIC ClassBase
  : public tl::Object
{
public:
  typedef tl::weak_collection<ClassBase> class_collection;
  typedef class_collection::const_iterator class_iterator;
  typedef Methods::iterator method_iterator;
  
  /**
   *  @brief Constructor
   *
   *  The constructor supplies information about the connector class (can be 0), a documentation string and the method declarations.
   */
  ClassBase (const std::string &doc, const Methods &mm, bool do_register = true);

  /**
   *  @brief Destructor
   */
  virtual ~ClassBase ();

  /**
   *  @brief Gets the pointer to the base class declaration object (can be 0)
   */
  const ClassBase *base () const
  {
    return mp_base;
  }

  /**
   *  @brief Returns a pointer to the type_info object if the class is an adaptor
   *
   *  If the class adapts another type (specifically enums), this method will
   *  return a pointer to the adapted type's type_info. 
   *  In other cases, the return value is 0.
   */
  virtual const std::type_info *adapted_type_info () const 
  {
    return 0;
  }

  /**
   *  @brief Returns the "real" (consolidated) declaration object
   *  
   *  The actual declaration object may be different from this declaration because of the class extension
   *  mechanism. Using that mechanism, class declarations can extend over multiple ClassBase objects. The
   *  individual declarations are merged and a consolidated class declaration object is provided.
   */
  virtual const ClassBase *declaration () const 
  {
    return 0;
  }

  /**
   *  @brief Consolidates the declaration
   *
   *  Before all classes are finalized and the script interpreters start to instantiate them, all
   *  class declarations are consolidated. If the consolidate method returns true, the class declaration
   *  is kept. Otherwise it is discarded.
   */
  virtual bool consolidate () const
  {
    return false;
  }

  /**
   *  @brief Gets the class name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Gets the module name
   */
  const std::string &module () const
  {
    return m_module;
  }

  /**
   *  @brief Gets the documentation string
   */
  const std::string &doc () const
  {
    return m_doc;
  }

  /**
   *  @brief Merge base and extension declarations
   *
   *  This method must be called at least once!
   *  It produces the consolidates class declaration object.
   */
  static void merge_declarations ();

  /**
   *  @brief Gets the parent declaration object
   *  This returns the parent class if this class is a child class.
   */
  const ClassBase *parent () const 
  {
    return mp_parent;
  }

  /**
   *  @brief Gets the fully qualified name (A::B::..)
   */
  std::string qname () const;

  /**
   *  @brief Adds a child class
   */
  void add_child_class (const ClassBase *cls);

  /**
   *  @brief Adds a subclass
   *  Subclasses are the ones that derive from us (opposite of base)
   */
  void add_subclass (const ClassBase *cls);

  /**
   *  @brief Iterates all child classes (begin)
   */
  tl::weak_collection<ClassBase>::const_iterator begin_child_classes () const
  {
    return m_child_classes.begin ();
  }
 
  /**
   *  @brief Iterates all subclasses (end)
   */
  tl::weak_collection<ClassBase>::const_iterator end_child_classes () const
  {
    return m_child_classes.end ();
  }
 
  /**
   *  @brief Iterates all classes present (begin)
   */
  static class_iterator begin_classes ()
  {
    return collection ().begin ();
  }
 
  /**
   *  @brief Iterates all classes present (end)
   */
  static class_iterator end_classes () 
  {
    return collection ().end ();
  }
 
  /**
   *  @brief Iterates all freshly registered classes (begin)
   *  This collection is emptied on "merge_declarations".
   */
  static class_iterator begin_new_classes ()
  {
    return new_collection ().begin ();
  }

  /**
   *  @brief Iterates all freshly registered classes (begin)
   */
  static class_iterator end_new_classes ()
  {
    return new_collection ().end ();
  }

  /**
   *  @brief Returns a list of all classes in definition order
   *
   *  Definition order is:
   *  - No duplicate class entries
   *  - Base classes before their derived classes
   *  - Child classes after their parent classes
   *
   *  If a module name is given, only top-level classes from this
   *  module will be considered. However, the list may also include
   *  base classes or child classes from outside the module.
   */
  static std::list<const gsi::ClassBase *> classes_in_definition_order (const char *mod_name = 0);

  /**
   *  @brief Iterates the methods (begin)
   */
  method_iterator begin_methods () const
  {
    return m_methods.begin ();
  }

  /**
   *  @brief Iterates the methods (end)
   */
  method_iterator end_methods () const
  {
    return m_methods.end ();
  }

  /**
   *  @brief Iterates the constructor methods (begin)
   */
  method_iterator begin_constructors () const
  {
    return m_constructors.begin ();
  }

  /**
   *  @brief Iterates the constructor methods (end)
   */
  method_iterator end_constructors () const
  {
    return m_constructors.end ();
  }

  /**
   *  @brief Iterates the callback methods (begin)
   */
  method_iterator begin_callbacks () const
  {
    return m_callbacks.begin ();
  }

  /**
   *  @brief Iterates the callback methods (end)
   */
  method_iterator end_callbacks () const
  {
    return m_callbacks.end ();
  }

  /**
   *  @brief Returns true, if this class is derived from the given base class
   */
  bool is_derived_from (const ClassBase *base) const;

  /**
   *  @brief Returns true, if this class can be used to initialize an object with type "target"
   */
  bool can_convert_to (const ClassBase *target) const;

  /**
   *  @brief Creates a new object initialized from "obj" of type "target"
   */
  void *create_obj_from (const ClassBase *target, void *obj) const;

  /**
   *  @brief Class implementation: destroys the given object
   *
   *  The implementation of this method is supposed to delete the object and
   *  free any resources associated with that object. After that operation,
   *  the object pointer is invalid.
   *
   *  This method may not be available if "can_destroy" is false.
   */
  virtual void destroy (void * /*obj*/) const 
  {
    tl_assert (false);
  }

  /**
   *  @brief Class implementation: creates an object (default constructor)
   *
   *  Creates and default-initializes the given object. This method may not be
   *  available if there is no default constructor (can_default_create is false). 
   *  In that case, some static factory method must be provided.
   */
  virtual void *create () const 
  {
    tl_assert (false);
    return 0;
  }

  /**
   *  @brief Creates a class representing an adapted object 
   *
   *  This method will create a new object representing the adapted object x.
   */
  virtual void *create_from_adapted (const void * /*x*/) const
  {
    tl_assert (false);
    return 0;
  }

  /**
   *  @brief Creates a class representing an adapted object 
   *
   *  This method will create a new object representing the adapted object x.
   *  It will consume the object given by x.
   */
  virtual void *create_from_adapted_consume (void * /*x*/) const
  {
    tl_assert (false);
    return 0;
  }

  /**
   *  @brief Returns a pointer to the adapted object from an adaptor class
   */
  virtual const void *adapted_from_obj (const void * /*obj*/) const
  {
    tl_assert (false);
    return 0;
  }

  /**
   *  @brief Returns a new pointer to the adapted object from an adaptor class
   */
  virtual void *create_adapted_from_obj (const void * /*obj*/) const
  {
    tl_assert (false);
    return 0;
  }

  /**
   *  @brief Class implementation: clones the given object
   *
   *  The implementation is supposed to create a deep copy of the source object.
   *  This method may not be available if can_copy is false.
   */
  virtual void *clone (const void * /*src*/) const 
  {
    tl_assert (false);
    return 0;
  }

  /**
   *  @brief Class implementation: assigns the source to the target object
   *
   *  The implementation is supposed to overwrite the target with the contents
   *  of the source. This is a deep copy.
   *  This method may not be available if can_copy is false.
   */
  virtual void assign (void * /*target*/, const void * /*src*/) const 
  {
    tl_assert (false);
  }

  /**
   *  @brief Class implementation: destruction supported predicate
   *
   *  This flag is true, if the class supports destruction of objects of this kind.
   */
  virtual bool can_destroy () const 
  {
    tl_assert (false);
    return false;
  }

  /**
   *  @brief Class implementation: copy supported predicate
   *
   *  This flag is true, if the class supports copying of objects of this kind.
   */
  virtual bool can_copy () const 
  {
    tl_assert (false);
    return false;
  }

  /**
   *  @brief Class implementation: default construction supported predicate
   *
   *  This flag is true, if the class supports default construction of objects of this kind.
   */
  virtual bool can_default_create () const 
  {
    tl_assert (false);
    return false;
  }

  /**
   *  @brief Class implementation: returns true if this class binds to a script class
   */
  virtual bool binds () const
  {
    tl_assert (false);
    return false;
  }

  /**
   *  @brief Returns true, if the given object can be cast to this class
   *
   *  When this method is called, it is guaranteed that the object is at least of 
   *  the base class type. This implies that there is a base class when this method
   *  is called.
   */
  virtual bool can_upcast (const void * /*p*/) const
  {
    tl_assert (false);
    return false;
  }

  /**
   *  @brief Class implementation: gets C++ type of object
   *
   *  This method delivers the C++ type_info object of this class.
   */
  virtual const std::type_info &type () const 
  {
    tl_assert (false);
    return typeid (void);
  }

  /**
   *  @brief Class implementation: gets C++ type of object
   *
   *  This method delivers the class declaration for an subclassed object p. p must be of a 
   *  subclass (derived class) of this class. This method will the declaration object 
   *  of object p. 
   */
  virtual const ClassBase *subclass_decl (const void * /*p*/) const 
  {
    tl_assert (false);
    return 0;
  }

  /**
   *  @brief Class Implementation: Returns true, if the object is managed
   *  "managed" objects are those which are derived from gsi::ObjectBase.
   */
  virtual bool is_managed () const
  {
    tl_assert (false);
    return false;
  }

  /**
   *  @brief Gets the basic gsi::ObjectBase object from a generic pointer
   *  This method will return 0, if the object is not managed.
   *  If required is false, a return value of 0 is permitted, indicating that
   *  no dynamic allocation of a gsi::ObjectBase has happened yet. This is useful
   *  in case of the Qt-GSI bridge which means a special Qt object needs to be
   *  created in order to provide the gsi::ObjectBase interface. If required is
   *  false, this initialization does not need to happen.
   */
  virtual gsi::ObjectBase *gsi_object (void * /*p*/, bool /*required*/ = true) const
  {
    tl_assert (false);
    return 0;
  }

  /**
   *  @brief Class Implementation: provide variant binding information (constness)
   */
  virtual const tl::VariantUserClassBase *var_cls (bool /*is_const*/) const
  {
    tl_assert (false);
    return 0;
  }

  /**
   *  @brief Class Implementation: provide variant binding information (meta/class object)
   */
  virtual const tl::VariantUserClassBase *var_cls_cls () const
  {
    tl_assert (false);
    return 0;
  }

  /**
   *  @brief Returns true, if the class is an external class provided by Python or Ruby code
   */
  virtual bool is_external () const
  {
    return false;
  }

  /**
   *  @brief Post-construction initialization
   *
   *  This method will be called by the GSI system to provide initialization after 
   *  the static initialization. Some schemes cannot be implementation statically, plus
   *  the initialization order is undetermined for static initialization.
   *  In that case, this initialization step is useful. It will call the initialize
   *  method on all method declarations.
   */
  virtual void initialize ();

  /**
   *  @brief Adds a method to the class
   *  The class becomes owner of the method object. 
   *  This method is public to allow dynamic extension of the documentation
   *  through scripts.
   *  Don't use it for other purposes.
   */
  void add_method (MethodBase *method, bool base_class = false);

  /**
   *  @brief Sets the per-client data for the given client index 
   *  This method is const to preserve the general const semantics of the class 
   *  while allowing clients to register information.
   */
  void set_data (int ch, PerClassClientSpecificData *data) const
  {
    mp_data[ch].reset (data);
  }

  /**
   *  @brief Gets the per-client data for the given client index
   */
  PerClassClientSpecificData *data (int ch) const
  {
    return mp_data[ch].get ();
  }

  /**
   *  @brief Sets the per-client data for gsi::Expressions
   *  This is a special slot since no client index is available for gsi::Expressions.
   */
  void set_gsi_data (PerClassClientSpecificData *data) const
  {
    set_data (ClientIndex::Basic, data);
  }

  /**
   *  @brief Gets the per-client data for gsi::Expressions
   */
  PerClassClientSpecificData *gsi_data () const
  {
    return data (ClientIndex::Basic);
  }

protected:
  static const class_collection &collection ();
  static const class_collection &new_collection ();

  const tl::weak_collection<ClassBase> &subclasses () const
  {
    return m_subclasses;
  }

  void set_name (const std::string &n) 
  {
    m_name = n;
  }

  void set_module (const std::string &m)
  {
    m_module = m;
  }

  void set_parent (const ClassBase *parent);
  void set_base (const ClassBase *base);

private:
  bool m_initialized;
  const ClassBase *mp_base, *mp_parent;
  std::string m_doc;
  Methods m_methods;
  std::vector<MethodBase *> m_callbacks, m_constructors;
  std::string m_name;
  std::string m_module;
  tl::weak_collection<ClassBase> m_child_classes, m_subclasses;
  mutable std::unique_ptr<PerClassClientSpecificData> mp_data[ClientIndex::MaxClientIndex];

  static class_collection *mp_class_collection;
  static class_collection *mp_new_class_collection;

  //  No copying
  ClassBase (const ClassBase &other);
  ClassBase &operator= (const ClassBase &other);
};

/**
 *  @brief Accessor to a declaration through name
 */
GSI_PUBLIC const ClassBase *class_by_name (const std::string &name);

/**
 *  @brief Accessor to a declaration through name
 *
 *  This version won't assert when there is no such class and
 *  return 0 instead.
 */
GSI_PUBLIC const ClassBase *class_by_name_no_assert (const std::string &name);

/**
 *  @brief Returns true if there is a class with the given name
 */
GSI_PUBLIC bool has_class (const std::string &name);

/**
 *  @brief Find a class declaration through the type info 
 */
GSI_PUBLIC const ClassBase *class_by_typeinfo (const std::type_info &ti);

/**
 *  @brief Find a class declaration through the type info
 *
 *  This version won't assert when there is no such class and
 *  return 0 instead.
 */
GSI_PUBLIC const ClassBase *class_by_typeinfo_no_assert (const std::type_info &ti);

/**
 *  @brief Returns true if there is a class with the given type info
 */
GSI_PUBLIC bool has_class (const std::type_info &ti);

}

#endif


