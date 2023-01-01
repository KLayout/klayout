
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

 
#ifndef HDR_tlClassRegistry
#define HDR_tlClassRegistry

#include "tlCommon.h"

#include "tlLog.h"
#include <typeinfo>

namespace tl
{

template <class X> class Registrar;

/**
 *  @brief A helper object that is used to manage the registered classes
 */
template <class X>
class RegistrarNode
{
private:
  RegistrarNode ()
    : mp_object (0), m_owned (true), m_position (0), mp_next (0)
  {
    // .. nothing else ..
  }

  ~RegistrarNode ()
  {
    if (m_owned) {
      delete mp_object;
    }
    mp_object = 0;
  }

  X *mp_object;
  bool m_owned;
  int m_position;
  std::string m_name;
  RegistrarNode *mp_next; 

  friend class Registrar<X>;
  friend class Registrar<X>::iterator;
};

/**
 *  @brief A generic class registration facility
 *
 *  It can register a set of objects by instantiating a certain object
 *  and somewhere (else) instantiating an registrar.
 *  The objects are classified by a base class they are derived of.
 *  The collection of registered objects can be iterated.
 *  See tlClassRegistry.ut for an example.
 */ 

/**
 *  @brief A class representant that is registered
 */
template <class X>
class RegisteredClass 
{
public:
  /** 
   *  @brief register an object of type X
   *
   *  This will register the given object. The X pointer
   *  will become owned by the registrar if "owned" is true.
   *  The position parameter tells where to insert the class in the
   *  chain - higher positions come later.
   *  The name is an arbitrary string that is used for debugging purposes only.
   */
  RegisteredClass (X *inst, int position = 0, const char *name = "", bool owned = true) 
    : m_owned (owned)
  { 
    Registrar<X> *instance = Registrar<X>::get_instance ();
    if (! instance) {
      instance = new Registrar<X> ();
      Registrar<X>::set_instance (instance);
    }
    mp_node = instance->insert (inst, owned, position, name);
    
    if (tl::verbosity () >= 40) {
      tl::info << "Registered object '" << name << "' with priority " << position;
    }
  }

  ~RegisteredClass ()
  {
    Registrar<X> *instance = Registrar<X>::get_instance ();
    if (instance) {

      //  remove the associated object
      instance->remove (mp_node);

      if (instance->begin () == instance->end ()) {
        //  no more registered objects left - remove registrar
        delete instance;
        Registrar<X>::set_instance (0);
      }

    }
  }

private:
  RegistrarNode<X> *mp_node;
  bool m_owned; 
};

/**
 *  @brief A base class for the registrar types
 */
class RegistrarBase { };

/**
 *  @brief Sets the registrar instance by type
 */
TL_PUBLIC void set_registrar_instance_by_type (const std::type_info &ti, RegistrarBase *rb);

/**
 *  @brief Gets the registrar instance by type
 *  Returns 0 if no registrar instance is set by this type;
 */
TL_PUBLIC RegistrarBase *registrar_instance_by_type (const std::type_info &ti);

/**
 *  @brief The registrar capable of registering objects of type Y derived from X
 *
 *  Objects of type Y derived from X can simply be created and registered by 
 *  instantiation (statically) an object of tl::Registrar<X>::Class which
 *  will receive an new'ed object of type Y and register it in the <X> registration
 *  space. This object will then be owned by the registrar and is destroyed upon
 *  shut down.
 */
template <class X>
class Registrar
    : public RegistrarBase
{
public:
  class iterator
  {
  public:
    iterator (RegistrarNode<X> *p)
      : mp_pos (p)
    {
      //  .. nothing yet ..
    }

    bool operator== (iterator d) const
    {
      return mp_pos == d.mp_pos;
    }

    bool operator!= (iterator d) const
    {
      return mp_pos != d.mp_pos;
    }

    iterator &operator++ () 
    {
      mp_pos = mp_pos->mp_next;
      return *this;
    }

    const std::string &current_name () const
    {
      return mp_pos->m_name;
    }

    int current_position () const
    {
      return mp_pos->m_position;
    }

    X &operator* () const
    {
      return *(mp_pos->mp_object);
    }

    X *operator-> () const
    {
      return mp_pos->mp_object;
    }

    X *take () const
    {
      X *x = mp_pos->mp_object;
      mp_pos->mp_object = 0;
      return x;
    }

  private:
    RegistrarNode<X> *mp_pos;
  };

  /**
   *  @brief Constructor
   */
  Registrar <X> () 
    : mp_first (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief begin() iterator over the registered objects
   */
  static iterator begin () 
  {
    if (get_instance ()) {
      return iterator (get_instance ()->mp_first);
    } else {
      return iterator (0); 
    }
  }

  /**
   *  @brief end() iterator over the registered objects
   */
  static iterator end () 
  {
    return iterator (0); 
  }

  static Registrar<X> *get_instance () 
  {
    return static_cast<Registrar<X> *> (registrar_instance_by_type (typeid (X)));
  }

  static void set_instance (Registrar<X> *instance)
  {
    set_registrar_instance_by_type (typeid (X), instance);
  }

private:
  friend class iterator;
  template <class Y> friend class RegisteredClass;

  RegistrarNode<X> *insert (X *cls, bool owned, int position, const std::string &name)
  {
    RegistrarNode<X> **link = &mp_first;
    while (*link && (*link)->m_position < position) {
      link = &((*link)->mp_next);
    }

    RegistrarNode<X> *node = new RegistrarNode<X> ();
    node->mp_object = cls;
    node->m_owned = owned;
    node->m_position = position;
    node->m_name = name;
    node->mp_next = *link;
    *link = node;

    return node;
  }

  void remove (RegistrarNode<X> *node)
  {
    RegistrarNode<X> **link = &mp_first;
    while (*link && *link != node) {
      link = &((*link)->mp_next);
    }

    if (*link) {
      RegistrarNode<X> *node = *link;
      *link = node->mp_next;
      delete node;
    }
  }

  RegistrarNode<X> *mp_first;
};

}

#endif

