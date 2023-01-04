
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


#ifndef _HDR_gsiObject
#define _HDR_gsiObject

#include "tlObject.h"
#include "tlEvents.h"
#include "tlThreads.h"
#include "gsiCommon.h"

#include <memory>

//  For a comprehensive documentation see gsi.h

namespace gsi
{

class GSI_PUBLIC ClassBase;

/**
 *  @brief Definition of the client indexes
 *
 *  The client index is used to indicate a certain client for the object (Python, Ruby ...)
 */
struct ClientIndex
{
  enum {
    Basic           = 0,
    Ruby            = 1,
    Python          = 2,
    MaxClientIndex  = 3
  };
};

/**
 *  @brief A base class for objects that are connected to their script counterpart
 */
class GSI_PUBLIC ObjectBase
{
public:
  /**
   *  @brief The status changed event type
   */
  enum StatusEventType {
    ObjectDestroyed = 0,
    ObjectKeep = 1,
    ObjectRelease = 2
  };

  /**
   *  @brief Constructor
   */
  ObjectBase () 
    : mp_status_changed_event (0)
  {
  }

  /**
   *  @brief Copy constructor
   */
  ObjectBase (const ObjectBase &)
    : mp_status_changed_event (0)
  {
  }

  /**
   *  @brief Assignment
   */
  ObjectBase &operator= (const ObjectBase &)
  {
    return *this;
  }

  /**
   *  @brief Destructor
   */
  virtual ~ObjectBase ()
  {
    if (has_status_changed_event ()) {
      status_changed_event () (ObjectDestroyed);
    }
    if (has_status_changed_event ()) {
      delete mp_status_changed_event;
    }
    mp_status_changed_event = 0;
  }

  /**
   *  @brief Returns true, if the object was kept without any clients attached
   *  This function is supposed to capture the case where an object was kept in the
   *  constructor. The keep status cannot be propagated then and doing so needs to be
   *  delayed until a client attaches to it.
   *  This flag is reset when "status_changed_event ()" is called.
   */
  bool already_kept () const
  {
    return size_t (mp_status_changed_event) == 1;
  }

  /**
   *  @brief Returns true, if a status changed event was created already
   */
  bool has_status_changed_event () const
  {
    return mp_status_changed_event && ! already_kept ();
  }

  /**
   *  @brief Marks this object owned by the C++ side
   *  After calling this method, the script clients no longer will own the object.
   *  If the script object is deleted, the C++ object is not affected.
   */
  void keep () 
  {
    if (has_status_changed_event ()) {
      status_changed_event () (ObjectKeep);
    } else {
      mp_status_changed_event = reinterpret_cast<tl::event<StatusEventType> *> (1);
    }
  }

  /**
   *  @brief Marks this object as no longer owned by the C++ side
   *  After calling this method, the script clients (more specifically: the first one)
   *  will own the object.
   *  If the script object is deleted, the C++ object is deleted as well.
   */
  void release ()
  {
    if (has_status_changed_event ()) {
      status_changed_event () (ObjectRelease);
    } else {
      mp_status_changed_event = 0;
    }
  }

  /**
   *  @brief Finds a client object with the given type
   */
  template <class T>
  T *find_client ()
  {
    if (has_status_changed_event ()) {
      return status_changed_event ().find_receiver<T> ();
    } else {
      return 0;
    }
  }

  /**
   *  @brief Gets the status changed event
   */
  tl::event<StatusEventType> &status_changed_event ()
  {
    if (! has_status_changed_event()) {
      mp_status_changed_event = new tl::event<StatusEventType> ();
    }
    return *mp_status_changed_event;
  }

private:
  /**
   *  @brief The event that is triggered when the object's status changes
   *  This pointer can be "1" to indicate "early keep" condition.
   */
  tl::event<StatusEventType> *mp_status_changed_event;
};

/**
 *  @brief A proxy class that interfaces an gsi::ObjectBase with a tl::Object
 *
 *  Using a proxy object allows having a gsi::ObjectBase that does not derive
 *  from tl::Object and can derive from any base class (specifically QObject).
 *
 *  NOTE about MT safety: the model is:
 *
 *  - the Proxy belongs to a thread
 *  - the original object (the proxy target) belongs to a different thread
 *  - there can be multiple Proxy objects for one target
 *  - the target object itself and the methods by which the proxy acts on
 *    the target object are thread safe
 *
 *  This implies that all operations related to the manipulation of proxy
 *  to target relation need to be guarded by a lock.
 */
class GSI_PUBLIC Proxy
  : public tl::Object
{
public:
  Proxy (const gsi::ClassBase *_cls_decl);
  ~Proxy ();

  void destroy ();
  void detach ();
  void release ();
  void keep ();
  void set (void *obj, bool owned, bool const_ref, bool can_destroy);
  void *obj ();

  void *raw_obj ()
  {
    return m_obj;
  }

  bool destroyed () const
  {
    return m_destroyed;
  }

  bool owned () const
  {
    return m_owned;
  }

private:
  const gsi::ClassBase *m_cls_decl;
  void *m_obj;
  bool m_owned : 1;
  bool m_const_ref : 1;
  bool m_destroyed : 1;
  bool m_can_destroy : 1;
  static tl::Mutex m_lock;

  void *set_internal (void *obj, bool owned, bool const_ref, bool can_destroy);
  void object_status_changed (gsi::ObjectBase::StatusEventType type);
  void detach_internal ();
  void *obj_internal ();
};

}

#endif

