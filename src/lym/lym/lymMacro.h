
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#ifndef HDR_lymMacro
#define HDR_lymMacro

#include "lymCommon.h"
#include "tlObject.h"

#include <string>
#include <map>
#include <set>

#include <QObject>

namespace lym
{

class MacroCollection;

/**
 *  @brief Represents a macro in the framework
 *
 *  A macro is basically a piece of script code that is either 
 *  executed on startup or on request.
 *
 *  A macro can be persisted to a file and is located in the 
 *  file hierarchy in one of the configuration folders.
 *  The path of the macro is given by the path property.
 *
 *  A macro can be readonly when it is located in the global
 *  configuration folder. It cannot be saved in that case.
 *
 *  The basic method of a macro is the "run" method. Basically
 *  a macro can be bound to an arbitrary interpreter and decides
 *  by itself which interpreter to use.
 */
class LYM_PUBLIC Macro
  : public QObject,
    public tl::Object
{
Q_OBJECT 

public:
  /**
   *  @brief Interpreter type
   */
  enum Interpreter { 

    /**
     *  @brief Pure Ruby
     */
    Ruby, 
    
    /**
     *  @brief Pure Python
     */
    Python, 
    
    /**
     *  @brief Plain text (no interpreter)
     */
    Text, 
    
    /**
     *  @brief General DSL (uses dsl_interpreter to identify the actual interpreter class)
     */
    DSLInterpreter, 

    /**
     *  @brief No specific language. Interpreter won't be available.
     */
    None 

  };

  /**
   *  @brief Specification of how the file is stored
   */
  enum Format { 

    /**
     *  @brief KLayout macro format (XML)
     */
    MacroFormat, 

    /**
     *  @brief Plain text format
     */
    PlainTextFormat,

    /**
     *  @brief Plain text format with hash comments for inserting properties into the text
     */
    PlainTextWithHashAnnotationsFormat,

    /**
     *  @brief No file associated
     */
    NoFormat 

  };

  /**
   *  @brief Constructor
   *
   *  The default constructor creates a dummy macro with no file associated
   */
  Macro ();

  /**
   *  @brief Assignment from another macro
   * 
   *  This will assign the definition of the macro but keep the name.
   *  It will also not modify the parent nor the readonly flag.
   */
  void assign (const lym::Macro &other);

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
   *  @brief Gets the interpreter name
   */
  std::string interpreter_name () const;

  /**
   *  @brief Gets the summary text 
   *
   *  The summary text is shown in the tooltip of the tabs
   */
  std::string summary () const;
  
  /**
   *  @brief Gets the path 
   *
   *  The path is the file where the macro is stored.
   *  The path is changed when the macro is saved.
   *  If the macro was never saved, the path is empty.
   */
  std::string path () const;
  
  /**
   *  @brief Saves the macro to it's path
   */
  void save ();

  /**
   *  @brief Saves the macro to the specificed path
   */
  void save_to (const std::string &path);

  /**
   *  @brief Delete the original file (the file behind the macro)
   *
   *  Returns true if the file was deleted successfully.
   */
  bool del ();

  /**
   *  @brief Loads the macro from a file
   */
  void load ();

  /**
   *  @brief Load the macro from an arbitrary file
   *
   *  This method does not change the macro's path nor does it set the is_file property.
   *  It is used for importing macros
   */
  void load_from (const std::string &path);

  /**
   *  @brief Load the macro from a string
   *
   *  This method does not change the macro's path nor does it set the is_file property.
   *  It is used for importing macros. 
   *  The url must be given in order to determine the format.
   */
  void load_from_string (const std::string &text, const std::string &url);

  /**
   *  @brief Gets the directory part of the macro's path.
   */
  std::string dir () const;

  /**
   *  @brief Gets the name of the macro
   *
   *  The name is a unique string derived from the file name.
   *  This does not include the ".lym" or ".rb" extension.
   */
  std::string name () const
  {
    return m_name;
  }

  /** 
   *  @brief Rename a Macro
   *
   *  Renames the macro. If the macro is a file, the file will be renamed as well.
   *  This method will return true, if the rename was successful.
   *  The name must not contain the suffix.
   */
  bool rename (const std::string &n);

  /**
   *  @brief Gets the macro's description text
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Sets the macro's description text
   */
  void set_description (const std::string &d);

  /**
   *  @brief Gets the category tags of the macro
   *
   *  The category tags string is a comma-separated list of categories to which the 
   *  macro shall apply.
   */
  const std::string &category () const
  {
    return m_category;
  }

  /**
   *  @brief Sets the category tags of the macro
   */
  void set_category (const std::string &c) 
  {
    m_category = c;
  }

  /**
   *  @brief Gets the macro's prolog string
   *  The prolog is the code executed before the macro is run itself.
   */
  const std::string &prolog () const
  {
    return m_prolog;
  }

  /**
   *  @brief Sets the macro's prolog string
   */
  void set_prolog (const std::string &s);

  /**
   *  @brief Gets the macro's epilog string
   *  The prolog is the code executed after the macro is run itself.
   */
  const std::string &epilog () const
  {
    return m_epilog;
  }

  /**
   *  @brief Sets the macro's epilog string
   */
  void set_epilog (const std::string &s);

  /**
   *  @brief Gets the macro's version string
   */
  const std::string &version () const
  {
    return m_version;
  }

  /**
   *  @brief Sets the macro's version string
   */
  void set_version (const std::string &s);

  /**
   *  @brief Gets the macro's documentation text
   */
  const std::string &doc () const
  {
    return m_doc;
  }

  /**
   *  @brief Sets the macro's documentation text
   */
  void set_doc (const std::string &d);

  /**
   *  @brief Gets the display string
   */
  std::string display_string () const;

  /**
   *  @brief Gets the macro's script text
   */
  const std::string &text () const;

  /**
   *  @brief Sets the macro's script text
   */
  void set_text (const std::string &t);

  /**
   *  @brief Returns true, if the macro needs to be saved
   */
  bool is_modified () const
  {
    return m_modified;
  }

  /**
   *  @brief Reset the modified state
   */
  void reset_modified ();

  /**
   *  @brief make the macro "a file"
   *  This method is supposed to support the case of loading a file through a string.
   */
  void set_is_file ();

  /**
   *  @brief Set the macro's file path
   *  The file path can be used when the macro is a standalone object and 
   *  there is no parent folder by which the path can be derived.
   */
  void set_file_path (const std::string &fp);

  /**
   *  @brief Installs any add-on documentation that this macro potentially provides
   *
   *  If the documentation text starts with @class ..., this method installes the documentation
   *  therein in the GSI class/method repository.
   */
  void install_doc () const;

  /**
   *  @brief Executes the macro
   *
   *  On error, this method throws an exception.
   *
   *  If the scripts exits with "exit", the status code will be returned by the run 
   *  method.
   */
  int run () const;

  /**
   *  @brief Returns true, if the macro can be executed
   */
  bool can_run () const;

  /**
   *  @brief Gets a value indicating whether the macro is readonly
   */
  bool is_readonly () const
  {
    return m_readonly;
  }

  /**
   *  @brief Sets a value indicating whether the macro is readonly
   */
  void set_readonly (bool f);

  /**
   *  @brief Gets a value indicating whether the macro shall be executed on startup
   */
  bool is_autorun () const
  {
    return m_autorun;
  }

  /**
   *  @brief Gets a value indicating whether the macro shall be executed early on startup (before the main window is created)
   */
  bool is_autorun_early () const
  {
    return m_autorun_early;
  }

  /**
   *  @brief Sets a value indicating whether the macro shall be executed on startup
   */
  void set_autorun (bool f);

  /**
   *  @brief Sets a value indicating whether the macro shall be executed early on startup
   */
  void set_autorun_early (bool f);

  /**
   *  @brief Gets a value indicating whether the macro shall be shown in the menu
   */
  bool show_in_menu () const
  {
    return m_show_in_menu;
  }

  /**
   *  @brief Sets a value indicating whether the macro shall be shown in the menu
   */
  void set_show_in_menu (bool f);

  /**
   *  @brief Gets the menu group name
   *
   *  The menu group name identifies a group into which the item it put.
   *  A group has a separator that groups all items with the same group name.
   */
  const std::string &group_name () const
  {
    return m_group_name;
  }

  /**
   *  @brief Sets the group name
   */
  void set_group_name (const std::string &g);

  /**
   *  @brief Gets the menu path
   *
   *  The menu path. This path identifies the place where the macro is put in the
   *  menu. If this path is empty but "show_in_menu" is true, the macro will be put
   *  into the "Tools/Macros" menu.
   */
  const std::string &menu_path () const
  {
    return m_menu_path;
  }

  /**
   *  @brief Sets the menu path
   */
  void set_menu_path (const std::string &mp);

  /**
   *  @brief Gets the keyboard shortcut
   */
  const std::string &shortcut () const
  {
    return m_shortcut;
  }

  /**
   *  @brief Sets the shortcut
   */
  void set_shortcut (const std::string &s);

  /**
   *  @brief Gets the format of this macro
   */
  Format format () const
  {
    return m_format;
  }

  /**
   *  @brief Sets the format of this macro
   */
  void set_format (Format f);

  /**
   *  @brief Gets the suffix for a given interpreter and format
   */
  static std::string suffix_for_format (Macro::Interpreter interpreter, const std::string &dsl_name, Macro::Format format);

  /**
   *  @brief Gets the interpreter, dsl interpreter name, autorun preference and format for a given file name 
   *
   *  Returns false, if the suffix is not a known suffix.
   */
  static bool format_from_suffix (const std::string &fn, Macro::Interpreter &interpreter, std::string &dsl_name, bool &autorun_pref, Macro::Format &format);

  /**
   *  @brief Gets the name of the DSL interpreter for the DSL interpreter types
   */
  const std::string &dsl_interpreter () const
  {
    return m_dsl_interpreter;
  }

  /**
   *  @brief Set the DSL interpreter name
   */
  void set_dsl_interpreter (const std::string &dsl_name);

  /**
   *  @brief Gets the interpreter set for this macro
   */
  Interpreter interpreter () const
  {
    return m_interpreter;
  }

  /**
   *  @brief Set the interpreter
   */
  void set_interpreter (Interpreter interpreter);

  /**
   *  @brief Gets a value indicating whether the macro is backed up by a file
   *  
   *  A macro is not a file as long as it's just constructed by not saved.
   */
  bool is_file () const
  {
    return m_is_file;
  }

  /**
   *  @brief Synchronize the text with the properties in PlainTextWithHashAnnotationsFormat format
   */
  void sync_text_with_properties ();

  /**
   *  @brief Synchronize the properties with the text in PlainTextWithHashAnnotationsFormat format
   */
  void sync_properties_with_text ();

  /**
   *  @brief Compares two macros
   */
  bool operator== (const Macro &other) const;

  /**
   *  @brief Compares two macros
   */
  bool operator!= (const Macro &other) const
  {
    return !(*this == other);
  }

signals:
  /**
   *  @brief This signal is sent when the macro changes
   */
  void changed ();
  
private:
  friend class MacroCollection;

  bool m_modified;
  std::string m_name;
  std::string m_description;
  std::string m_prolog;
  std::string m_epilog;
  std::string m_version;
  std::string m_doc;
  std::string m_text;
  std::string m_file_path;
  std::string m_category;
  bool m_readonly;
  bool m_autorun;
  bool m_autorun_default;
  bool m_autorun_early;
  bool m_show_in_menu;
  std::string m_group_name;
  std::string m_menu_path;
  std::string m_shortcut;
  bool m_is_file;
  lym::MacroCollection *mp_parent;
  Interpreter m_interpreter;
  std::string m_dsl_interpreter;
  Format m_format;

  void on_menu_needs_update ();
  void on_changed ();
  static bool format_from_suffix_string (const std::string &suffix, Macro::Interpreter &interpreter, std::string &dsl_name, bool &autorun_pref, Macro::Format &format);
  static std::pair<bool, std::string> format_from_filename (const std::string &fn, Macro::Interpreter &interpreter, std::string &dsl_name, bool &autorun_pref, Macro::Format &format);

  void set_autorun_default (bool f)
  {
    m_autorun_default = f;
  }

  void set_name (const std::string &name)
  {
    m_name = name;
  }

  void set_parent (lym::MacroCollection *parent)
  {
    mp_parent = parent;
  }

  //  no copying
  Macro (const Macro &d);
  Macro &operator= (const Macro &d);
};

/**
 *  @brief Represents a collection of macros
 *
 *  A collection is representing a set of macros, usually associated with 
 *  a folder containing *.lym, *.rb or other script files.
 */
class LYM_PUBLIC MacroCollection
  : public QObject
{
Q_OBJECT 

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
  MacroCollection *add_folder (const std::string &description, const std::string &path, const std::string &category, bool readonly, bool force_create = true);

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
   */
  MacroCollection *folder_by_name (const std::string &name);

  /**
   *  @brief Gets a folder by name
   *
   *  If no folder with that name exists, this method will return 0.
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

  /**
   *  @brief Scans a folder creating the macro collection
   */
  void scan (const std::string &path);

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

  //  no copying
  MacroCollection (const MacroCollection &d);
  MacroCollection &operator= (const MacroCollection &d);
};

}

#endif

