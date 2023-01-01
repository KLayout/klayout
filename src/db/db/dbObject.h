
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


#ifndef HDR_dbObject
#define HDR_dbObject

#include "dbCommon.h"

#include "dbManager.h"

namespace db
{

/**
 *  @brief The base class of a database object
 *
 *  The main purpose of the db::Object class is to provide
 *  transaction and recall functionality. Basically this is 
 *  an implementation of the strategy pattern that is used
 *  to implement the recall functionality.
 */

class DB_PUBLIC Object
{
public:
  /**
   *  @brief Default ctor
   *
   *  Attach the object to a manager if required
   */
  Object (db::Manager *manager = 0);

  /**
   *  @brief Destructor
   *
   *  Detaches the object from the manager
   */
  virtual ~Object ();

  /** 
   *  @brief Copy constructor
   *
   *  The copy constructor copies the attachment to
   *  a manager object. 
   */
  Object (const Object &);

  /**
   *  @brief Manager object retrieval
   *
   *  Obtain the pointer to the management object this
   *  object is attached to. Returns 0 if not attached to a manager.
   */
  db::Manager *manager () const
  {
    return mp_manager;
  }

  /**
   *  @brief Attach to a different manager or detach
   *
   *  Changes the attachment of the object to any
   *  manager. Pass 0 to detach it from any manager.
   */
  void manager (db::Manager *p_manager);

  /** 
   *  @brief The undo 'strategy'
   *
   *  For a detailed description of this method see db::Manager::queue.
   *  This method has been declared to be no-throwing since it is
   *  assumed that once the operation is successfully queued it can be undone
   *  in every case.
   */
  virtual void undo (db::Op * /*op*/) 
  { }

  /** 
   *  @brief The redo 'strategy'
   *
   *  For a detailed description of this method see db::Manager::queue.
   *  This method has been declared to be no-throwing since it is
   *  assumed that once the operation is successfully queued it can be redone
   *  in every case.
   */
  virtual void redo (db::Op * /*op*/)
  { }

  /**
   *  @brief The id getter method
   */
  db::Manager::ident_t id () const 
  {
    return m_id;
  }

  /**
   *  @brief A convenience function to determine if we are transacting
   */
  bool transacting () const
  {
    return manager () && manager ()->transacting ();
  }

  /**
   *  @brief A convenience function to determine if we are in an undo or redo replay operation
   */
  bool replaying () const
  {
    return manager () && manager ()->replaying ();
  }

  /**
   *  @brief Begins a transaction (same as calling manager ()->transaction (), but safe against null manager ())
   */
  void transaction (const std::string &description, db::Manager::transaction_id_t join_with = 0)
  {
    if (manager ()) {
      manager ()->transaction (description, join_with);
    }
  }

  /**
   *  @brief Ends a transaction (same as calling manager ()->commit (), but safe against null manager ())
   */
  void commit ()
  {
    if (manager ()) {
      manager ()->commit ();
    }
  }

private:
  db::Manager::ident_t m_id;
  db::Manager *mp_manager;

  //  The assignment operator is private in order to
  //  force a specific treatment of the manager object attachment
  Object &operator= (const Object &);
};

} // namespace db

#endif

