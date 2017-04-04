
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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


#ifndef HDR_layApplication
#define HDR_layApplication

#include "layCommon.h"

#include <QApplication>
#include <QEventLoop>

#include "gsi.h"

#include <set>
#include <string>
#include <memory>

namespace gsi
{
  class Interpreter;
}

namespace gtf
{
  class Recorder;
}

namespace tl
{
  class DeferredMethodScheduler;
}

namespace lay
{

class MainWindow;
class PluginRoot;
class ProgressReporter;
class ProgressBar;
class MacroCollection;

/**
 *  @brief The basic application object
 *
 *  This object encapsulates command line parsing, creation of the main window
 *  widget and the basic execution loop.
 */
class LAY_PUBLIC Application
  : public QApplication, public gsi::ObjectBase
{
public:
  /**
   *  @brief The application constructor 
   *
   *  @param argc The number of external command-line arguments passed.
   *  @param argv The external command-line arguments.
   *  @param non_ui_mode True, if the UI shall not be enabled
   */
  Application (int &argc, char **argv, bool non_ui_mode);

  /**
   *  @brief Destructor
   */
  ~Application ();

  /**
   *  @brief The singleton instance
   */
  static Application *instance ();

  /**
   *  @brief Exit the application
   *
   *  This will terminate the process with the given exit code.
   */
  void exit (int result);

  /**
   *  @brief Reimplementation of notify from QApplication
   */
  bool notify (QObject *receiver, QEvent *e);

  /**
   *  @brief Return the program's version 
   */
  std::string version () const;

  /**
   *  @brief Return the program's usage string 
   */
  std::string usage ();

  /**
   *  @brief Return the main window's reference
   *
   *  If the application has not been initialized properly, this pointer is 0.
   */
  MainWindow *main_window () const
  {
    return mp_mw;
  }

  /**
   *  @brief Run the application
   *
   *  This method issues all the lower level methods required in order to perform the
   *  applications main code.
   */
  int run ();

  /**
   *  @brief Execute the GUI main loop
   */
  int exec ();

  /**
   *  @brief Process pending events
   *
   *  This is basically a wrapper for the QApplication::processEvents method. It provides some special
   *  handling for the "close application window" case and a "silent" mode. In that mode, processing
   *  of deferred methods is disabled.
   */
  void process_events (QEventLoop::ProcessEventsFlags flags, bool silent = false);

  /**
   *  @brief A shortcut for the default process_events
   */
  void process_events ()
  {
    process_events (QEventLoop::AllEvents);
  }

  /**
   *  @brief Set a configuration parameter
   */
  void set_config (const std::string &name, const std::string &value);

  /**
   *  @brief Commits the configuration
   */
  void config_end ();

  /**
   *  @brief Clear the configuration
   */
  void clear_config ();

  /**
   *  @brief Write configuration to a file
   *
   *  If the configuration file cannot be written, false
   *  is returned but no exception is thrown.
   */
  bool write_config (const std::string &config_file);

  /**
   *  @brief Read the configuration from a file
   *
   *  This method siletly does nothing, if the config file does not
   *  exist. If it does and an error occured, the error message is printed
   *  on stderr. In both cases, false is returned.
   */
  bool read_config (const std::string &config_file);

  /**
   *  @brief Get a configuration parameter
   */
  std::string get_config (const std::string &name) const;

  /**
   *  @brief Obtain a list of names of configuration parameters
   */
  std::vector<std::string> get_config_names () const;

  /**
   *  @brief Gets a value indicating whether the give special application flag is set
   *  
   *  Special application flags are ways to introduce debug or flags for special
   *  use cases. Such flags have a name and currently are controlled externally by 
   *  an environment variable called "KLAYOUT_x" where x is the name. If that
   *  variable is set and the value is non-empty, the flag is regarded set.
   */
  bool special_app_flag (const std::string &name);

  /**
   *  @brief Obtain the list of macro categories
   */
  const std::vector< std::pair<std::string, std::string> > &macro_categories () const
  {
    return m_macro_categories;
  }

  /**
   *  @brief Return a reference to the Ruby interpreter
   */
  gsi::Interpreter &ruby_interpreter () 
  {
    return *mp_ruby_interpreter;
  }

  /**
   *  @brief Return a reference to the Ruby interpreter
   */
  gsi::Interpreter &python_interpreter () 
  {
    return *mp_python_interpreter;
  }

  /**
   *  @brief Return true, if undo buffering is enabled
   */
  bool is_undo_enabled () const
  {
    return m_enable_undo;
  }

  /**
   *  @brief Returns true, if the application is in pure "viewer only" mode
   */
  bool is_vo_mode () const
  {
    return m_vo_mode;
  }

  /**
   *  @brief Returns true, if the application is in editable mode
   */
  bool is_editable () const
  {
    return m_editable;
  }

  /**
   *  @brief Makes the application editable
   *  Setting this flag does not have an immediate effect. It will only be effective for
   *  new views.
   */
  void set_editable (bool e);

  /**
   *  @brief Return true, if the application has a GUI
   */
  bool has_gui () const
  {
    return ! m_no_gui;
  }

  /**
   *  @brief For debugging purposes: get a symbol name (a description actually) from an address
   */
  static QString symbol_name_from_address (const QString &mod_name, size_t addr);

  /**
   *  @brief Reset config to global configuration
   */
  void reset_config (); 

  /**
   *  @brief Synchronize macro collections with technology-specific macros
   *
   *  Returns a vector of new macro folders.
   */
  std::vector<lay::MacroCollection *> sync_tech_macro_locations ();

  /**
   *  @brief Obtain the KLayout installation path
   */
  const std::string &inst_path () const
  {
    return m_inst_path;
  }

  /**
   *  @brief Obtain the application data path
   */
  const std::string &appdata_path () const
  {
    return m_appdata_path;
  }

  /**
   *  @brief Obtain the KLayout path
   */
  const std::vector<std::string> &klayout_path () const
  {
    return m_klayout_path;
  }

private:
  void shutdown ();
  void finish ();

  enum file_type {
    layout_file,
    layout_file_with_tech,
    layout_file_with_tech_file,
    rdb_file
  };

  std::vector <std::pair<file_type, std::pair<std::string, std::string> > > m_files;
  std::set <std::pair<std::string, std::string> > m_tech_macro_paths;
  std::string m_layer_props_file;
  bool m_lyp_map_all_cvs, m_lyp_add_default;
  std::string m_session_file;
  std::string m_run_macro;
  std::vector<std::string> m_load_macros;
  std::string m_gtf_replay;
  std::vector<std::string> m_config_files;
  std::vector<std::string> m_initial_config_files;
  std::string m_config_file_to_write;
  std::string m_config_file_to_delete;
  std::vector<std::string> m_klayout_path;
  std::string m_inst_path;
  std::string m_appdata_path;
  std::vector< std::pair<std::string, std::string> > m_macro_categories;
  bool m_write_config_file;
  std::vector< std::pair<std::string, std::string> > m_variables;
  int m_gtf_replay_rate, m_gtf_replay_stop;
  bool m_no_macros;
  bool m_same_view;
  bool m_sync_mode;
  bool m_no_gui;
  bool m_vo_mode;
  bool m_editable;
  bool m_enable_undo;
  QCoreApplication *mp_qapp;
  QApplication *mp_qapp_gui;
  std::auto_ptr<tl::DeferredMethodScheduler> mp_dm_scheduler;
  //  HINT: the ruby interpreter must be destroyed before MainWindow
  //  in order to maintain a valid MainWindow reference for ruby scripts and Ruby's GC all the time.
  gsi::Interpreter *mp_ruby_interpreter;
  gsi::Interpreter *mp_python_interpreter;
  MainWindow *mp_mw;
  lay::ProgressReporter *mp_pr;
  lay::ProgressBar *mp_pb;
  lay::PluginRoot *mp_plugin_root;
  gtf::Recorder *mp_recorder;
};

} // namespace lay

namespace tl {
  template <> struct type_traits<lay::Application> : public type_traits<void> {
    typedef tl::false_tag has_copy_constructor;
    typedef tl::false_tag has_default_constructor;
  };
}

#endif


