
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


#ifndef HDR_lymMacroCollection
#define HDR_lymMacroCollection

#include "lymCommon.h"
#include "lymMacro.h"

#include <string>
#include <map>
#include <set>

#if defined(HAVE_QT)
#  include <QObject>
#endif

namespace lym
{

/**
 *  @brief Represents a collection of macros
 *
 *  A collection is representing a set of macros, usually associated with
 *  a folder containing *.lym, *.rb or other script files.
 */
class LYM_PUBLIC MacroCollection
#if defined(HAVE_QT)
  : public QObject
#endif
{
#if defined(HAVE_QT)
Q_OBJECT
#endif

public:
  typedef std::multimap <std::string, Macro *>::iterator iterator;
  typedef std::multimap <std::string, Macro *>::const_iterator const_iterator;
  typedef std::map <std::string, MacroCollection *>::iterator child_iterator;
  typedef std::map <std::string, MacroCollection *>::const_iterator const_child_iterator;

  /**
   *  @brief Some constants for virtual_mode
   */
  enum FolderType {
    NotVirtual = 0,
    ProjectFolder = 1,
    TechFolder = 2,
    SaltFolder = 3
  };

  /**
   *  @brief Constructor
   *
   *  The default constructor create
   */
  MacroCollection ();

  /**
   *  @brief Destructor
   */
  ~MacroCollection ();

  /**
   *  @brief Add a folder (will also scan the folder)
   *
   *  @return A pointer to the new collection if successful
   *
   *  If force_create is true (the default), the folder will be created if it does not
   *  exist yet. On error, 0 is returned.
   */
  MacroCollection *add_folder (const std::string &description, const std::string &path, const std::string &category, bool readonly, bool auto_create = true);

  /**
   *  @brief Gets the category tag of the collection
   *
   *  A category tag can be used to categorize the collections. For example, DRC categories are handled differently
   *  from the other categories.
   */
  const std::string &category () const
  {
    return m_category;
  }

  /**
   *  @brief Sets the category tags
   */
  void set_category (const std::string &d)
  {
    m_category = d;
  }

  /**
   *  @brief Collect all Macro and MacroCollection objects inside a tree starting from this collection
   */
  void collect_used_nodes(std::set <Macro *> &macros, std::set <MacroCollection *> &macro_collections);

  /**
   *  @brief Saves all macros in the collection
   *
   *  Saves only those macros that have is_modified and whose path is set.
   */
  void save ();

  /**
   *  @brief Delete the original folder (the directory behind the macro)
   *
   *  Returns true if the folder was deleted successfully.
   *  The folder cannot be deleted if it contains any files, also some that are not listed because
   *  they don't end with .lym, .rb or similar.
   */
  bool del ();

  /**
   *  @brief Gets the name of the collection
   *
   *  For virtual collections this is the path.
   */
  std::string name () const
  {
    return m_path;
  }

  /**
   *  @brief Gets the path of the folder representing that collection
   */
  std::string path () const;

  /**
   *  @brief Returns the parent of the macro collection
   *
   *  Returns 0, if there is no parent of this collection (this is the root)
   */
  lym::MacroCollection *parent ()
  {
    return mp_parent;
  }

  /**
   *  @brief Returns the parent of the macro collection (const version)
   *
   *  Returns 0, if there is no parent of this collection (this is the root)
   */
  const lym::MacroCollection *parent () const
  {
    return mp_parent;
  }

  /**
   *  @brief Returns != 0, if the macro collection is a virtual node
   *
   *  A virtual node does not correspond to a location in the file system.
   *  A virtual node cannot have macros but only children.
   *  The return value indicates the kind of virtual use.
   */
  int virtual_mode () const
  {
    return m_virtual_mode;
  }

  /**
   *  @brief Sets the virtual mode
   *
   *  See virtual_mode for details about the virtual mode.
   */
  void set_virtual_mode (int m)
  {
    m_virtual_mode = m;
  }

  /**
   *  @brief Gets a value indicating whether the macro collection is readonly
   */
  bool is_readonly () const
  {
    return m_readonly;
  }

  /**
   *  @brief Sets a value indicating whether the macro is readonly
   *  In contrast to the private \set_readonly method, this version delivers a "changed" signal when
   *  the flag changed.
   */
  void make_readonly (bool f);

  /**
   *  @brief Gets the macro collection's description text
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Sets the description
   */
  void set_description (const std::string &d)
  {
    m_description = d;
  }

  /**
   *  @brief Gets the display string
   */
  std::string display_string () const;

  /**
   *  @brief Rename a Macro
   *
   *  Renames the macro. If the macro is a file, the file will be renamed as well.
   *  This method will return true, if the rename was successful.
   */
  bool rename (const std::string &n);

  /**
   *  @brief Adds a macro to the collection
   *
   *  If a macro with the name of the new macro already exists, it is replaced
   *  (like in the file system). This method will traverse the tree to find
   *  the location of the macro using the path information of the macro and insert
   *  the macro there.
   *
   *  The collection becomes the owner of the object passed to this method
   *
   *  @return true, if the macro could be added successfully.
   */
  bool add (lym::Macro *m);

  /**
   *  @brief Adds a macro in an unspecific way
   *
   *  "unspecific" means that the path is not looked up - the macro is
   *  added irregardless whether the path matches or not.
   *  This is a way to build macro collections without connection
   *  to some file system point.
   */
  void add_unspecific (lym::Macro *m);

  /**
   *  @brief Empties the collection
   *  Note: only the unspecific on_changed event is generated.
   */
  void clear ();

  /**
   *  @brief Erases the given macro from the list
   *
   *  This does not remove the file but just remove the macro object.
   *  This will also delete the macro object.
   */
  void erase (lym::Macro *m);

  /**
   *  @brief Erases the entry with the given iterator
   */
  void erase (iterator i);

  /**
   *  @brief Erases the given macro collection from the list of child collections
   *
   *  This does not remove the directory but just removes the macro collection object.
   *  This will also delete the macro collection object.
   */
  void erase (lym::MacroCollection *m);

  /**
   *  @brief Erases the folder with the given iterator
   */
  void erase (child_iterator i);

  /**
   *  @brief Creates a new macro in that collection (with a new name)
   *
   *  If a name is given, it is used as a prefix to create a unique name for a macro with that format.
   */
  lym::Macro *create (const char *name = 0, Macro::Format format = Macro::NoFormat);

  /**
   *  @brief Creates a new macro collection in that collection (with a new name)
   *
   *  If a name is given, it is used as a prefix to create a unique name.
   *  This method will also create the directory for this folder.
   *  If not successful, it will return 0.
   */
  lym::MacroCollection *create_folder (const char *name = 0, bool mkdir = true);

  /**
   *  @brief Gets the begin iterator of the macros
   */
  iterator begin ()
  {
    return m_macros.begin ();
  }

  /**
   *  @brief Gets the end iterator of the macros
   */
  iterator end ()
  {
    return m_macros.end ();
  }

  /**
   *  @brief Gets the begin iterator of the macros (const version)
   */
  const_iterator begin () const
  {
    return m_macros.begin ();
  }

  /**
   *  @brief Gets the end iterator of the macros (const version)
   */
  const_iterator end () const
  {
    return m_macros.end ();
  }

  /**
   *  @brief Gets the begin iterator of the folders
   *
   *  The iterator will deliver a pair of a string and a MacroCollection object.
   *  The string is the absolute path of the child folder. Child folders do not
   *  necessarily live inside the directory of the parent folder. Specifically for
   *  the root folder, children with any kind of paths may be present.
   */
  child_iterator begin_children ()
  {
    return m_folders.begin ();
  }

  /**
   *  @brief Gets the end iterator of the folders
   */
  child_iterator end_children ()
  {
    return m_folders.end ();
  }

  /**
   *  @brief Gets the begin iterator of the folders (const version)
   */
  const_child_iterator begin_children () const
  {
    return m_folders.begin ();
  }

  /**
   *  @brief Gets the end iterator of the folders (const version)
   */
  const_child_iterator end_children () const
  {
    return m_folders.end ();
  }

  /**
   *  @brief Gets a macro by name
   *
   *  If no macro with that name exists, this method will return 0.
   */
  Macro *macro_by_name (const std::string &name, Macro::Format format);

  /**
   *  @brief Gets a macro by name
   *
   *  If no macro with that name exists, this method will return 0.
   */
  const Macro *macro_by_name (const std::string &name, Macro::Format format) const;

  /**
   *  @brief Gets a folder by name
   *
   *  If no folder with that name exists, this method will return 0.
   *  The name is either relative to the folders path or it is an absolute path.
   */
  MacroCollection *folder_by_name (const std::string &name);

  /**
   *  @brief Gets a folder by name
   *
   *  If no folder with that name exists, this method will return 0.
   *  The name is either relative to the folders path or it is an absolute path.
   */
  const MacroCollection *folder_by_name (const std::string &name) const;

  /**
   *  @brief Finds a macro by path
   *
   *  This method is called from the root collection and delivers the macro which
   *  matches the given path or 0.
   */
  lym::Macro *find_macro (const std::string &path);

  /**
   *  @brief Returns true, if the collection has an autorun macro
   */
  bool has_autorun () const;

  /**
   *  @brief Runs all macros marked with auto-run
   */
  void autorun ();

  /**
   *  @brief Returns true, if the collection has an early autorun macro
   */
  bool has_autorun_early () const;

  /**
   *  @brief Runs all macros marked with early auto-run
   */
  void autorun_early ();

  /**
   *  @brief Redo the scan (will add new files or folders)
   *
   *  This method must be called on root.
   */
  void rescan ();

  /**
   *  @brief Reloads the macro collection
   *
   *  This method is similar to rescan, but it will also remove folders and macros.
   *  In safe mode (safe = true), modified macros won't be overwritten.
   */
  void reload (bool safe);

  /**
   *  @brief Gets the root of the macro hierarchy corresponding to the configuration space
   */
  static MacroCollection &root ();

  /**
   *  @brief Dump the macro tree (for debugging)
   */
  void dump (int l = 0);

#if defined(HAVE_QT)
signals:
  /**
   *  @brief This signal is sent when the collection changes
   */
  void changed ();

  /**
   *  @brief This signal is sent by collection when a child collection is deleted in this collection
   */
  void child_deleted (lym::MacroCollection *);

  /**
   *  @brief This signal is sent by the root object when a macro collection is deleted
   */
  void macro_collection_deleted (lym::MacroCollection *);

  /**
   *  @brief This signal is sent by collection when a macro is deleted in this collection
   */
  void macro_deleted_here (lym::Macro *);

  /**
   *  @brief This signal is sent by the root object when a macro is deleted
   */
  void macro_deleted (lym::Macro *);

  /**
   *  @brief This signal is sent by the root object when a macro changes
   *
   *  This signal is only emitted by the root, but it may originate from a
   *  macro inside the tree.
   */
  void macro_changed (lym::Macro *);

  /**
   *  @brief This signal is sent by the root object when a macro collection changes
   *
   *  This signal is only emitted by the root, but it may originate from a
   *  macro collection inside the tree.
   */
  void macro_collection_changed (lym::MacroCollection *);

  /**
   *  @brief This signal is sent by the root object befor the macro collection changes
   */
  void about_to_change ();

  /**
   *  @brief This signal is emitted from the collection root if the menu needs to be updated
   */
  void menu_needs_update ();
#endif

private:
  friend class Macro;

  std::string m_path;
  std::string m_description;
  std::string m_category;
  std::multimap <std::string, Macro *> m_macros;
  std::map <std::string, MacroCollection *> m_folders;
  lym::MacroCollection *mp_parent;
  int m_virtual_mode;
  bool m_readonly;

  void on_child_deleted (MacroCollection *mc);
  void on_macro_collection_deleted (MacroCollection *mc);
  void on_macro_deleted_here (Macro *macro);
  void on_macro_deleted (Macro *macro);
  void on_macro_changed (Macro *macro);
  void on_macro_collection_changed (MacroCollection *mc);
  void on_changed ();
  void on_menu_needs_update ();

  void scan ();
  void create_entry (const std::string &path);

  void rename_macro (Macro *macro, const std::string &new_name);

  void begin_changes ();

  void set_name (const std::string &n)
  {
    m_path = n;
  }

  void set_parent (lym::MacroCollection *parent)
  {
    mp_parent = parent;
  }

  void set_readonly (bool f)
  {
    m_readonly = f;
  }

  void do_clear ();

  //  no copying
  MacroCollection (const MacroCollection &d);
  MacroCollection &operator= (const MacroCollection &d);
};

}

#endif

