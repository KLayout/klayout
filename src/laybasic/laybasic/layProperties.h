
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

#if defined(HAVE_QT)

#ifndef HDR_layProperties
#define HDR_layProperties

#include "laybasicCommon.h"
#include "layEditable.h"

#include <QFrame>
#include <QIcon>

namespace db
{
  class Manager;
}

namespace lay
{

class Editable;

/**
 *  @brief The properties page object
 *
 *  The properties page object forms the interface between the 
 *  properties dialog and the selected object.
 */

class LAYBASIC_PUBLIC PropertiesPage
  : public QFrame
{
Q_OBJECT

public:
  /**
   *  @brief The constructor attaching the properties page to a parent widget
   *
   *  The constructor is supposed to position the element pointer to
   *  the first element in the selection. Unless the selection is
   *  empty, at_start () is supposed to return true then.
   *  The dialog will call update () to update the display accordingly.
   */
  PropertiesPage (QWidget *parent, db::Manager *manager, lay::Editable *editable);

  /**
   *  @brief The destructor
   */
  virtual ~PropertiesPage ();

  /**
   *  @brief Gets the number of entries represented by this page
   */
  virtual size_t count () const = 0;

  /**
   *  @brief Selects the entries with the given indexes
   *
   *  If multiple indexes are selected, the properties page shows
   *  all items with different values as "leave as is". Items with
   *  same value are shown with the given value.
   */
  virtual void select_entries (const std::vector<size_t> &entries) = 0;

  /**
   *  @brief Convenience function to select a specific entry
   */
  void select_entry (size_t entry)
  {
    std::vector<size_t> entries;
    entries.push_back (entry);
    select_entries (entries);
  }

  /**
   *  @brief Gets a description text for the nth entry
   */
  virtual std::string description (size_t entry) const = 0;

  /**
   *  @brief Gets the icon for the nth entry
   */
  virtual QIcon icon (size_t /*entry*/, int /*w*/, int /*h*/) const
  {
    return QIcon ();
  }

  /**
   *  @brief Gets a description text for the whole group
   */
  virtual std::string description () const = 0;

  /**
   *  @brief Gets the icon associated with the whole group
   */
  virtual QIcon icon (int /*w*/, int /*h*/) const
  {
    return QIcon ();
  }

  /**
   *  @brief Update the display
   *
   *  This method is called by the dialog to transfer data from the
   *  selected element into the display (QFrame and children).
   */
  virtual void update () { }

  /** 
   *  @brief Leave the properties page
   *
   *  This method is called when the properties page becomes hidden (unused)
   *  in the stack of properties pages. The main use is to notify the 
   *  owner to remove highlights.
   */
  virtual void leave () { }

  /** 
   *  @brief Report if the object can be changed
   *
   *  This method is supposed to return true if the current object
   *  cannot be changed. In this case, the "Apply" button is disabled.
   */
  virtual bool readonly () 
  {
    //  default implementation is "readonly"
    return true;
  }

  /** 
   *  @brief Apply any changes to the current object
   *
   *  Apply any changes to the current objects. If nothing was
   *  changed, the object may be left untouched.
   *  The dialog will start a transaction on the manager object.
   */
  virtual void apply () 
  {
    //  default implementation is empty.
  }

  /**
   *  @brief Returns true, if the properties page supports "apply all"
   */
  virtual bool can_apply_to_all () const
  {
    return false;
  }

  /** 
   *  @brief Apply current changes to all objects of the current kind
   *
   *  Apply any changes to the current object plus all other objects of the same kind. 
   *  If nothing was changed, the objects may be left untouched.
   *  The dialog will start a transaction on the manager object.
   */
  virtual void apply_to_all (bool /*relative*/)
  {
    //  default implementation is empty.
  }

  /**
   *  @brief Return the Editable object that this properties page
   *  object was issued from.
   */
  lay::Editable *editable () 
  {
    return mp_editable.get ();
  }

  /**
   *  @brief Gets the transaction manager object
   *  Use this object to implement undable operations on the properties page.
   */
  db::Manager *manager ()
  {
    return mp_manager;
  }

signals:
  /**
   *  @brief This signal is emitted if a value has been changed
   */
  void edited ();

private:
  db::Manager *mp_manager;
  tl::weak_ptr<lay::Editable> mp_editable;
};

}

#endif

#endif  //  defined(HAVE_QT)
