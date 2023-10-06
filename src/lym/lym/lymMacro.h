
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


#ifndef HDR_lymMacro
#define HDR_lymMacro

#include "lymCommon.h"
#include "tlObject.h"

#include <string>
#include <map>
#include <set>

#if defined(HAVE_QT)
#  include <QObject>
#endif

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
class LYM_PUBLIC Macro :
#if defined(HAVE_QT)
    public QObject,
#endif
    public tl::Object
{
#if defined(HAVE_QT)
Q_OBJECT
#endif

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
   *  @brief Sets a value indicating whether the macro was alread auto-runned
   */
  void set_was_autorun (bool f);

  /**
   *  @brief Gets a value indicating whether the macro was alread auto-runned
   */
  bool was_autorun () const
  {
    return m_was_autorun;
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
   *  @brief Gets the priority of the macro in autorun and autorun-early mode
   *  0 is the first priority, -1 means "never execute".
   */
  int priority () const
  {
    return m_priority;
  }

  /**
   *  @brief Sets the priority
   */
  void set_priority (int p);

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

#if defined(HAVE_QT)
signals:
  /**
   *  @brief This signal is sent when the macro changes
   */
  void changed ();
#endif

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
  bool m_was_autorun;
  int m_priority;
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

}

#endif

