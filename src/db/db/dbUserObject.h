
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



#ifndef HDR_dbUserObject
#define HDR_dbUserObject

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbPoint.h"
#include "dbTrans.h"
#include "dbObjectTag.h"
#include "dbBox.h"
#include "dbMemStatistics.h"
#include "tlClassRegistry.h"

#include <string>
#include <string.h>

namespace db {

template <class Coord> class generic_repository;
class ArrayRepository;

/**
 *  @brief Deliver unique and incremental class ID's (once)
 */
DB_PUBLIC unsigned int get_unique_user_object_class_id ();

/**
 *  @brief The base class for the user object class
 *
 *  Each user object class must implement this interface
 *  in order to be able to be put into a user_object<C>.
 */
template <class C>
class DB_PUBLIC_TEMPLATE user_object_base
{
public:
  typedef C coord_type;

  virtual ~user_object_base () { }

  /**
   *  @brief Compare with another object
   *
   *  The implementation is supposed to return true if the
   *  object is identical to this. The pointer passed should be
   *  dynamic_cast to the derived class and false should be 
   *  returned if the class is not identical.
   */
  virtual bool equals (const user_object_base<C> *d) const = 0;

  /**
   *  @brief Compare with another object
   *
   *  The implementation is supposed to return true if the
   *  object is "less" to this. The pointer passed should be
   *  dynamic_cast to the derived class and it is guaranteed that
   *  this is possible.
   */
  virtual bool less (const user_object_base<C> *d) const = 0;

  /**
   *  @brief Return a unique class Id
   *
   *  This value must be "some" unique class ID. 
   *  This is either an integer assigned by convention or by calling 
   *  get_unique_user_object_class_id once in the initialisation of a static member
   *  Used for operator< () implementation.
   */
  virtual unsigned int class_id () const = 0;

  /**
   *  @brief Clone the object
   *
   *  This method is supposed to return a pointer to a new
   *  object identical to this one.
   */
  virtual user_object_base<C> *clone () const = 0;

  /**
   *  @brief Compute the bounding box of this object
   *
   *  This method is supposed to return the bounding box
   *  of this object.
   */
  virtual db::box<C> box () const = 0;

  /**
   *  @brief Transform this object with a simple_trans
   */
  virtual void transform (const simple_trans<C> &t) 
  { 
    transform (complex_trans<C, C> (t));
  }

  /**
   *  @brief Transform this object with a fixpoint_trans
   */
  virtual void transform (const fixpoint_trans<C> &t) 
  { 
    transform (complex_trans<C, C> (t));
  }

  /**
   *  @brief Transform this object with a complex_trans
   */
  virtual void transform (const complex_trans<C, C> & /*t*/) 
  { 
    //  .. the default implementation does nothing ..
  }

  /**
   *  @brief Returns the class name
   *
   *  This class name can be used to construct an object from a string using the generic factory.
   *  The class name can be 0 indicating that it is not possible to create an object from a string
   *  and that the object is not registered in the generic factory.
   */
  virtual const char *class_name () const { return 0; }

  /**
   *  @brief Fill from a string
   *
   *  This method needs to be implemented mainly if the object is to be created from the
   *  generic factory.
   */
  virtual void from_string (const char * /*str*/, const char * /*base_path*/) { }

  /**
   *  @brief Convert to a string
   *
   *  This method needs to be implemented mainly if the object is to be created from the
   *  generic factory.
   */
  virtual std::string to_string () const { return std::string (); }

  /**
   *  @brief Collect memory statistics
   */
  virtual void mem_stat (db::MemStatistics * /*stat*/, db::MemStatistics::purpose_t /*purpose*/, int /*cat*/, bool /*no_self*/, void * /*parent*/) const { }
};

template <class C>
class user_object
{
public:
  typedef C coord_type;
  typedef db::box<C> box_type;
  typedef db::point<C> point_type;
  typedef db::object_tag< user_object<C> > tag;

  /**
   *  @brief The default constructor.
   *  
   *  The default constructor creates an empty object.
   */
  user_object ()
    : mp_obj (0)
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The constructor taking a pointer to a base object
   *  
   *  This constructor takes over the ownership over the object
   *  passed.
   */
  user_object (user_object_base<C> *obj)
    : mp_obj (obj)
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The copy constructor
   */
  user_object (const user_object<C> &d)
    : mp_obj (0)
  {
    if (d.mp_obj) {
      set_ptr (d.mp_obj->clone ());
    }
  }

  /**
   *  @brief The move constructor
   */
  user_object (user_object<C> &&d)
    : mp_obj (0)
  {
    if (d.mp_obj) {
      set_ptr (d.mp_obj);
      d.mp_obj = 0;
    }
  }

  /**
   *  @brief Assignment operator
   */
  user_object<C> &operator= (const user_object<C> &d)
  {
    if (d.mp_obj) {
      set_ptr (d.mp_obj->clone ());
    } else {
      set_ptr (0);
    }
    return *this;
  }

  /**
   *  @brief Assignment operator (move)
   */
  user_object<C> &operator= (user_object<C> &&d)
  {
    if (d.mp_obj) {
      set_ptr (d.mp_obj);
      d.mp_obj = 0;
    } else {
      set_ptr (0);
    }
    return *this;
  }

  /**
   *  @brief The destructor
   */
  ~user_object ()
  {
    set_ptr (0);
  }

  /**
   *  @brief The (dummy) translation operator
   */
  void translate (const user_object<C> &d, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
  }

  /**
   *  @brief The (dummy) translation operator with transformation
   */
  template <class T>
  void translate (const user_object<C> &d, const T &t, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
    transform (t);
  }

  /** 
   *  @brief Equality test
   */
  bool operator== (const user_object<C> &d) const
  {
    if (mp_obj == 0 || d.mp_obj == 0) {
      return mp_obj == d.mp_obj;
    } else {
      return mp_obj->equals (d.mp_obj);
    }
  }

  /** 
   *  @brief Inequality test
   */
  bool operator!= (const user_object<C> &b) const
  {
    return !operator== (b);
  }

  /**
   *  @brief Less operator implementation
   */
  bool operator< (const user_object<C> &b) const
  {
    if (mp_obj == 0 || b.mp_obj == 0) {
      return mp_obj < b.mp_obj;
    } 
    if (mp_obj->class_id () != b.mp_obj->class_id ()) {
      return (mp_obj->class_id () < b.mp_obj->class_id ());
    }
    return mp_obj->less (b.mp_obj);
  }

  /**
   *  @brief Get the pointer to the base object
   */
  const user_object_base<C> *ptr () const
  {
    return mp_obj;
  }

  /**
   *  @brief Get the pointer to the base object (non-const version)
   */
  user_object_base<C> *ptr () 
  {
    return mp_obj;
  }

  /**
   *  @brief Replace the pointer
   */
  void set_ptr (user_object_base<C> *ptr)
  {
    if (mp_obj) {
      delete mp_obj;
    }
    mp_obj = ptr;
  }

  /**
   *  @brief Get the bounding box
   *
   *  This may cause coordinate overflow if the user object cannot be represented
   *  in the target coordinate types.
   */
  db::box<C> box () const
  {
    if (mp_obj) {
      return mp_obj->box (); 
    } else {
      return db::box<C> ();
    }
  }

  /**
   *  @brief Transform the object with the given transformation
   *
   *  @param t The transformation to apply.
   *  The actual behaviour is implemented by the base object.
   */
  template <class Trans>
  void transform (const Trans &t)
  {
    if (mp_obj) {
      mp_obj->transform (t);
    }
  }

  /**
   *  @brief Return the transformed object
   *
   *  @param t The transformation to apply.
   *  The actual behaviour is implemented by the base object.
   */
  template <class Trans>
  db::user_object<C> transformed (const Trans &t) const
  {
    user_object o (*this);
    o.transform (t);
    return o;
  }

  /**
   *  @brief swap with another object
   */
  void swap (db::user_object<C> &other)
  {
    std::swap (mp_obj, other.mp_obj);
  }

  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
    if (mp_obj) {
      mp_obj->mem_stat (stat, purpose, cat, false, (void *) this);
    }
  }

private:
  db::user_object_base<C> *mp_obj;
};

/**
 *  @brief The standard user object base class typedef
 */
typedef user_object_base<db::Coord>  UserObjectBase;

/**
 *  @brief The double coordinate user object base class typedef
 */
typedef user_object_base<db::DCoord> DUserObjectBase;

/**
 *  @brief The standard user object typedef
 */
typedef user_object<db::Coord>  UserObject;

/**
 *  @brief The double coordinate user object typedef
 */
typedef user_object<db::DCoord> DUserObject;

/**
 *  @brief The base object of a factory-instantiable object
 */
template <class C>
class DB_PUBLIC_TEMPLATE user_object_factory_base
{
public:
  user_object_factory_base () { }
  virtual ~user_object_factory_base () { }
  virtual const char *class_name () const = 0;
  virtual user_object_base<C> *create () const = 0;
};

/**
 *  @brief An implementation of a user object factory
 *
 *  This implements a factory for objects of class X with coordinate base type C.
 */
template <class X, class C>
class DB_PUBLIC_TEMPLATE user_object_factory_impl
  : public user_object_factory_base <C>
{
public:
  user_object_factory_impl (const char *class_name) 
    : mp_class_name (class_name) 
  { }

  virtual const char *class_name () const 
  { 
    return mp_class_name; 
  }

  virtual user_object_base<C> *create () const 
  {
    return new X ();
  }

public:
  const char *mp_class_name;
};

/**
 *  @brief The generic factory
 *
 *  This factory creates an user object from a given string and class name.
 */
template <class C>
class user_object_factory
  : public tl::Registrar< user_object_factory_base<C> >
{
public:
  /** 
   *  @brief Create a UserObject from a string with the given class and using the provided string to create the object from.
   *
   *  If the class name is not registered, no object is created and 0 is returned.
   */
  static user_object_base<C> *create (const char *class_name, const char *string, const char *base_path)
  {
    tl::Registrar< user_object_factory_base<C> > *factory = tl::Registrar< user_object_factory_base<C> >::get_instance ();
    if (factory != 0) {
      for (typename tl::Registrar< user_object_factory_base<C> >::iterator i = factory->begin (); i != factory->end (); ++i) {
        if (strcmp (class_name, i->class_name ()) == 0) {
          user_object_base<C> *obj = i->create ();
          obj->from_string (string, base_path);
          return obj;
        }
      }
    }
    return 0;
  }
};

/**
 *  @brief Collect memory statistics
 */
template <class X>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const user_object<X> &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief typedef for a factory object for integer coordinate user objects
 */
typedef user_object_factory<db::Coord> UserObjectFactory;

/**
 *  @brief typedef for a factory object for integer coordinate user objects
 */
typedef tl::RegisteredClass<user_object_factory_base<db::Coord> > UserObjectDeclaration;

/**
 *  @brief typedef for a factory object for double coordinate user objects
 */
typedef user_object_factory<db::DCoord> DUserObjectFactory;

/**
 *  @brief typedef for a factory object for double coordinate user objects
 */
typedef tl::RegisteredClass<user_object_factory_base<db::DCoord> > DUserObjectDeclaration;

} // namespace db

//  inject a swap specialization into the std namespace:
namespace std 
{  
  template <class C>
  inline void swap (db::user_object<C> &a, db::user_object<C> &b)
  {
    a.swap (b);
  }
}

#endif

