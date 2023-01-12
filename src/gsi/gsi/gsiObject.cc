
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


#include "gsiCommon.h"
#include "gsiObject.h"
#include "gsiDecl.h"

#include "tlLog.h"
#include "tlInternational.h"

namespace gsi
{

tl::Mutex Proxy::m_lock;

Proxy::Proxy (const gsi::ClassBase *_cls_decl)
  : m_cls_decl (_cls_decl),
    m_obj (0),
    m_owned (false),
    m_const_ref (false),
    m_destroyed (false),
    m_can_destroy (false)
{
  //  .. nothing yet ..
}

Proxy::~Proxy ()
{
  void *prev_obj = 0;

  {
    tl::MutexLocker locker (&m_lock);
    try {
      prev_obj = set_internal (0, false, false, false);
    } catch (std::exception &ex) {
      tl::warn << "Caught exception in object destructor: " << ex.what ();
    } catch (tl::Exception &ex) {
      tl::warn << "Caught exception in object destructor: " << ex.msg ();
    } catch (...) {
      tl::warn << "Caught unspecified exception in object destructor";
    }
    m_destroyed = true;
  }

  //  destroy outside the locker because the destructor may raise status
  //  changed events
  if (prev_obj) {
    m_cls_decl->destroy (prev_obj);
  }
}

void
Proxy::destroy ()
{
  tl::MutexLocker locker (&m_lock);

  if (! m_cls_decl) {
    m_obj = 0;
    return;
  }

  if (!m_can_destroy && m_obj) {
    throw tl::Exception (tl::to_string (tr ("Object cannot be destroyed explicitly")));
  }

  //  first create the object if it was not created yet and check if it has not been
  //  destroyed already (the former is to ensure that the object is created at least)
  if (! m_obj) {
    if (m_destroyed) {
      throw tl::Exception (tl::to_string (tr ("Object has been destroyed already")));
    } else {
      m_obj = m_cls_decl->create ();
      m_owned = true;
    }
  }

  void *o = 0;
  if (m_owned || m_can_destroy) {
    o = m_obj;
  }
  detach_internal ();
  if (o) {
    m_cls_decl->destroy (o);
  }
}

void
Proxy::detach ()
{
  tl::MutexLocker locker (&m_lock);
  detach_internal ();
}

void
Proxy::release ()
{
  tl::MutexLocker locker (&m_lock);

  //  If the object is managed we first reset the ownership of all other clients
  //  and then make us the owner
  const gsi::ClassBase *cls = m_cls_decl;
  if (cls && cls->is_managed ()) {
    void *o = obj_internal ();
    if (o) {
      cls->gsi_object (o)->keep ();
    }
  }

  //  NOTE: this is fairly dangerous
  m_owned = true;
}

void
Proxy::keep ()
{
  tl::MutexLocker locker (&m_lock);

  const gsi::ClassBase *cls = m_cls_decl;
  if (cls) {
    void *o = obj_internal ();
    if (o) {
      if (cls->is_managed ()) {
        cls->gsi_object (o)->keep ();
      } else {
        //  Fallback: the object is not gsi-enabled: we use the ownership flag in this
        //  case to keep it alive. This will not reset the ownership flag for all clients.
        m_owned = false;
      }
    }
  }
}

void
Proxy::set (void *obj, bool owned, bool const_ref, bool can_destroy)
{
  void *prev_obj;

  {
    tl::MutexLocker locker (&m_lock);
    prev_obj = set_internal (obj, owned, const_ref, can_destroy);
  }

  //  destroy outside the locker because the destructor may raise status
  //  changed events
  if (prev_obj) {
    m_cls_decl->destroy (prev_obj);
  }
}

void *
Proxy::obj ()
{
  tl::MutexLocker locker (&m_lock);
  return obj_internal ();
}

void *
Proxy::obj_internal ()
{
  if (! m_obj) {
    if (m_destroyed) {
      throw tl::Exception (tl::to_string (tr ("Object has been destroyed already")));
    } else {
      //  delayed creation of a detached C++ object ..
      tl_assert (set_internal (m_cls_decl->create (), true, false, true) == 0);
    }
  }

  return m_obj;
}

void
Proxy::object_status_changed (gsi::ObjectBase::StatusEventType type)
{
  if (type == gsi::ObjectBase::ObjectDestroyed) {
    tl::MutexLocker locker (&m_lock);
    m_destroyed = true;  //  NOTE: must be set before detach and indicates that the object was destroyed externally.
    detach_internal ();
  } else if (type == gsi::ObjectBase::ObjectKeep) {
    //  NOTE: don't lock this as this will cause a deadlock from keep()
    m_owned = false;
  } else if (type == gsi::ObjectBase::ObjectRelease) {
    //  NOTE: don't lock this as this will cause a deadlock from release()
    m_owned = true;
  }
}

void *
Proxy::set_internal (void *obj, bool owned, bool const_ref, bool can_destroy)
{
  bool prev_owned = m_owned;

  m_owned = owned;
  m_can_destroy = can_destroy;
  m_const_ref = const_ref;
  void *prev_object = 0;

  const gsi::ClassBase *cls = m_cls_decl;
  if (! cls) {

    m_obj = obj;

  } else if (obj != m_obj) {

    //  cleanup
    if (m_obj) {

      if (cls->is_managed ()) {
        gsi::ObjectBase *gsi_object = cls->gsi_object (m_obj, false);
        if (gsi_object) {
          gsi_object->status_changed_event ().remove (this, &Proxy::object_status_changed);
        }
      }

      //  Destroy the object if we are owner. We don't destroy the object if it was locked
      //  (either because we are not owner or from C++ side using keep())
      if (prev_owned) {
        prev_object = m_obj;
        m_obj = 0;
      }

    }

    m_obj = obj;

    if (m_obj) {

      if (cls->is_managed ()) {
        gsi::ObjectBase *gsi_object = cls->gsi_object (m_obj);
        //  Consider the case of "keep inside constructor"
        if (m_owned && gsi_object->already_kept ()) {
          m_owned = false;
        }
        gsi_object->status_changed_event ().add (this, &Proxy::object_status_changed);
      }

    }

  }

  //  now we have a valid object (or nil) - we can reset "destroyed" state. Note: this has to be done
  //  here because before detach might be called on *this which resets m_destroyed.
  m_destroyed = false;

  return prev_object;
}

void
Proxy::detach_internal()
{
  if (! m_destroyed && m_cls_decl && m_cls_decl->is_managed ()) {
    gsi::ObjectBase *gsi_object = m_cls_decl->gsi_object (m_obj, false);
    if (gsi_object) {
      gsi_object->status_changed_event ().remove (this, &Proxy::object_status_changed);
    }
  }

  m_obj = 0;
  m_destroyed = true;
  m_const_ref = false;
  m_owned = false;
  m_can_destroy = false;
}

}

