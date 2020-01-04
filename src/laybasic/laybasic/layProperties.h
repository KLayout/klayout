
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#ifndef HDR_layProperties
#define HDR_layProperties

#include "laybasicCommon.h"

#include <QFrame>

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
   *  @brief Move the current pointer to the end of the selected objects list
   *
   *  Must be implemented by the Editable function and must set the 
   *  selected object pointer to past-the-end of the list. at_end () must 
   *  return true after this.
   */
  virtual void back () = 0;

  /**
   *  @brief A helper function for the dialog that additionally reports
   *  if there are any elements in the selection
   */
  bool back_checked () 
  {
    back ();
    return ! at_begin ();
  }

  /**
   *  @brief Move the current pointer to the beginning of the selected objects list
   *
   *  Must be implemented by the Editable function and must set the 
   *  selected object pointer to the beginning of the list. at_begin () must 
   *  return true after this.
   *  This method must return true, if there are any elements in the selection
   *  - i.e. operator++ () will be successful afterwards.
   */
  virtual void front () = 0;

  /**
   *  @brief A helper function for the dialog that additionally reports
   *  if there are any elements in the selection
   */
  bool front_checked () 
  {
    front ();
    return ! at_end ();
  }

  /**
   *  @brief Tell if the current object references the first one
   *
   *  Must be implemented by the Editable function and must return
   *  true if the current object is the first one - i.e. if an
   *  operator-- would render the status invalid.
   *  If no object is referenced, this method and at_end () must return true.
   */
  virtual bool at_begin () const = 0;

  /**
   *  @brief Tell if the current object references past the last one
   *
   *  If the current object pointer is past the end of the selected
   *  object space, 
   */
  virtual bool at_end () const = 0;

  /**
   *  @brief Step one elemement back 
   *
   *  This method is supposed to move the current pointer one position
   *  back. If at_begin () was true before, the result may be unpredictable.
   *  The dialog will call update () to update the display accordingly.
   */
  virtual void operator-- () = 0;

  /**
   *  @brief Advance one element
   *
   *  This method is supposed to move the current pointer one position
   *  forward. If at_end () was true before, the result may be unpredictable.
   *  The dialog will call update () to update the display accordingly.
   */
  virtual void operator++ () = 0;

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
   *  Apply any changes to the current object. If nothing was 
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
  virtual void apply_to_all () 
  {
    //  default implementation is empty.
  }

  /**
   *  @brief Return the Editable object that this properties page
   *  object was issued from.
   */
  lay::Editable *editable () 
  {
    return mp_editable;
  }

  /**
   *  @brief Gets the transaction manager object
   *  Use this object to implement undable operations on the properties page.
   */
  db::Manager *manager ()
  {
    return mp_manager;
  }

private:
  db::Manager *mp_manager;
  lay::Editable *mp_editable;
};

}

#endif

