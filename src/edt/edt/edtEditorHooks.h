
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



#ifndef HDR_edtEditorHooks
#define HDR_edtEditorHooks

#include "edtCommon.h"
#include "dbTrans.h"
#include "gsiObject.h"

#include <set>
#include <string>

namespace lay
{
  class LayoutView;
  class ObjectInstPath;
}

namespace edt
{

/**
 *  @ brief The editor hooks handler object
 *
 *  Editor hooks are a way to hook into the editor feature - for example
 *  to implement dynamic DRC or display hints.
 *
 *  The protocols are:
 *
 *  1. Object Creation
 *
 *    begin_create { begin_new_objects { create } end_new_objects } [ commit_create ] end_create
 *
 *  2. Modification (i.e. partial edit)
 *
 *    begin_modify { begin_modifications { modified } end_modifications } [ commit_modify ] end_modify
 *
 *  3. Interactive edit (move, transform, interactive clone)
 *
 *    begin_edit { begin_edits { transformed } end_edits } [ commit_edit ] end_edit
 *
 *  Notation: { ... } means the sequence can be repeated, [ ... ] means the call is optional.
 */

class EDT_PUBLIC EditorHooks
  : public gsi::ObjectBase, public tl::Object
{
public:
  /**
   *  @brief Constructor
   */
  EditorHooks ();

  /**
   *  @brief Destructor
   */
  virtual ~EditorHooks ();

  //  creation protocol
  virtual void begin_create (lay::LayoutView * /*view*/) { }
  virtual void begin_new_objects () { }
  virtual void create (const lay::ObjectInstPath & /*object*/, double /*dbu*/) { }
  virtual void end_new_objects () { }
  virtual void commit_create () { }
  virtual void end_create () { }

  //  modification protocol
  virtual void begin_modify (lay::LayoutView * /*view*/) { }
  virtual void begin_modifications () { }
  virtual void modified (const lay::ObjectInstPath & /*object*/, double /*dbu*/) { }
  virtual void end_modifications () { }
  virtual void commit_modify () { }
  virtual void end_modify () { }

  //  editing protocol
  virtual void begin_edit (lay::LayoutView * /*view*/) { }
  virtual void begin_edits () { }
  virtual void transformed (const lay::ObjectInstPath & /*object*/, const db::DCplxTrans & /*trans*/, double /*dbu*/) { }
  virtual void end_edits () { }
  virtual void commit_edit () { }
  virtual void end_edit () { }

  /**
   *  @brief Gets the technology name this hook is associated with
   *
   *  If this attribute is non-empty, the hook is selected only when the given technology is
   *  used for the layout.
   */
  const std::set<std::string> &get_technologies () const
  {
    return m_technologies;
  }

  /**
   *  @brief Gets a value indicating whether this hook is associated with the given technology
   */
  bool is_for_technology (const std::string &name) const;

  /**
   *  @brief Gets a value indicating whether the hook is associated with any technology
   */
  bool for_technologies () const;

  /**
   *  @brief Sets the technology name this hook is associated with
   *
   *  This will reset the list of technologies to this one.
   *  If the given technology string is empty, the list of technologies will be cleared.
   */
  void set_technology (const std::string &t);

  /**
   *  @brief Clears the list of technologies this hook is associated with
   */
  void clear_technologies ();

  /**
   *  @brief Additionally associate the hook with the given technology
   */
  void add_technology (const std::string &tech);

  /**
   *  @brief Registers the editor hook
   */
  static void register_editor_hook (EditorHooks *hook);

  /**
   *  @brief Gets the editor hooks for a given technology
   *
   *  The order of the hooks is determined by the registration order.
   */
  static std::vector<EditorHooks *> get_editor_hooks (const std::string &for_technology);

private:
  std::set<std::string> m_technologies;

  // no copying.
  EditorHooks &operator= (const EditorHooks &);
  EditorHooks (const EditorHooks &);
};

}

#endif

