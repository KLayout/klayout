
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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
#include "layBusy.h"
#include "tlEvents.h"

#include <QApplication>
#include <QEventLoop>
#ifdef __APPLE__
#  include <QEvent>
#endif

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

namespace lym
{
  class MacroCollection;
}

namespace db
{
  class Manager;
}

namespace lay
{

class MainWindow;
class Dispatcher;
class ProgressReporter;
class ProgressBar;
class LayoutView;

/**
 *  @brief The application base class
 *
 *  This is the basic functionality for the application class.
 *  Two specializations exist: one for the GUI-less version (derived from QCoreApplication)
 *  and one for the GUI version (derived from QApplication).
 */
class LAY_PUBLIC ApplicationBase
  : public gsi::ObjectBase, public tl::Object
{
public:
  /**
   *  @brief The application constructor
   *
   *  @param argc The number of external command-line arguments passed.
   *  @param argv The external command-line arguments.
   */
  ApplicationBase (bool non_ui_mode);

  /**
   *  @brief Destructor
   */
  virtual ~ApplicationBase ();

  /**
   *  @brief The singleton instance
   */
  static ApplicationBase *instance ();

  /**
   *  @brief Exit the application
   *
   *  This will terminate the process with the given exit code.
   */
  void exit (int result);

  /**
   *  @brief Return the program's version
   */
  static std::string version ();

  /**
   *  @brief Return the program's usage string
   */
  static std::string usage ();

  /**
   *  @brief Returns the main window's reference
   *
   *  If the application has not been initialized properly or does not support GUI,
   *  this pointer is 0.
   */
  virtual MainWindow *main_window () const = 0;

  /**
   *  @brief Runs plugin and macro specific initializations
   */
  void autorun ();

  /**
   *  @brief Run the application
   *
   *  This method issues all the lower level methods required in order to perform the
   *  applications main code.
   *  Depending on the arguments and UI capabilities, this method will
   *  either execute the command line macros for open the main window.
   */
  int run ();

  /**
   *  @brief Executes the UI loop if GUI is enabled
   */
  virtual int exec () = 0;

  /**
   *  @brief Process pending events
   *
   *  This is basically a wrapper for the QApplication::processEvents method. It provides some special
   *  handling for the "close application window" case and a "silent" mode. In that mode, processing
   *  of deferred methods is disabled.
   */
  void process_events (QEventLoop::ProcessEventsFlags flags, bool silent = false)
  {
    process_events_impl (flags, silent);
  }

  /**
   *  @brief A shortcut for the default process_events
   */
  void process_events ()
  {
    process_events_impl (QEventLoop::AllEvents);
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
   *  This method silently does nothing, if the config file does not
   *  exist. If it does and an error occurred, the error message is printed
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
   *  @brief Adds a new macro category
   *
   *  This method is only effective when called during the autorun_early stage
   */
  void add_macro_category (const std::string &name, const std::string &description, const std::vector<std::string> &folders);

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
   *  @brief Reset config to global configuration
   */
  void reset_config ();

  /**
   *  @brief Synchronize macro collections with technology-specific macros
   *
   *  Returns a vector of new macro folders.
   */
  std::vector<lym::MacroCollection *> sync_tech_macro_locations ();

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

  /**
   *  @brief Parses the given command line arguments and configures the application object accordingly.
   */
  void parse_cmd (int &argc, char **argv);

  /**
   *  @brief Initializes the application
   *  This method needs to be called after "parse_cmd" and before the application is used.
   */
  void init_app ();

  /**
   *  @brief An event indicating that the package collection has changed
   */
  tl::Event salt_changed_event;

  /**
   *  @brief Gets the QApplication object
   *  This method will return non-null only if a GUI-enabled application is present.
   */
  virtual QApplication *qapp_gui () { return 0; }

protected:
  virtual void setup () = 0;
  virtual void shutdown ();
  virtual void prepare_recording (const std::string &gtf_record, bool gtf_record_incremental);
  virtual void start_recording ();
  virtual lay::Dispatcher *dispatcher () const = 0;
  virtual void finish ();
  virtual void process_events_impl (QEventLoop::ProcessEventsFlags flags, bool silent = false);

private:
  std::vector<std::string> scan_global_modules ();
  lay::LayoutView *create_view (db::Manager &manager);

  enum file_type {
    layout_file,
    layout_file_with_tech,
    layout_file_with_tech_file,
    rdb_file,
    l2ndb_file
  };

  std::vector <std::pair<file_type, std::pair<std::string, std::string> > > m_files;
  std::set <std::pair<std::string, std::string> > m_tech_macro_paths;
  std::string m_layer_props_file;
  bool m_lyp_map_all_cvs, m_lyp_add_default;
  std::string m_session_file;
  std::string m_run_macro;
  bool m_run_macro_and_exit;
  std::vector<std::pair<std::string, std::string> > m_custom_macro_paths;
  std::vector<std::string> m_load_macros;
  std::vector <std::string> m_package_inst;
  bool m_packages_with_dep;
  std::string m_gtf_replay;
  std::vector<std::string> m_config_files;
  std::vector<std::string> m_initial_config_files;
  std::string m_config_file_to_write;
  std::string m_config_file_to_delete;
  std::vector<std::string> m_klayout_path;
  std::string m_inst_path;
  std::string m_appdata_path;
  bool m_write_config_file;
  std::vector< std::pair<std::string, std::string> > m_variables;
  int m_gtf_replay_rate, m_gtf_replay_stop;
  std::string m_gtf_record;
  bool m_gtf_save_incremental;
  bool m_no_macros;
  bool m_same_view;
  bool m_sync_mode;
  bool m_no_gui;
  bool m_vo_mode;
  bool m_editable;
  bool m_editable_set;
  bool m_enable_undo;
  //  HINT: the ruby interpreter must be destroyed before MainWindow
  //  in order to maintain a valid MainWindow reference for ruby scripts and Ruby's GC all the time.
  gsi::Interpreter *mp_ruby_interpreter;
  gsi::Interpreter *mp_python_interpreter;

  void salt_changed ();
};

/**
 *  @brief The GUI-enabled application class
 */
class LAY_PUBLIC GuiApplication
  : public QApplication, public ApplicationBase, public lay::BusyMode
{
public:
  GuiApplication (int &argc, char **argv);
  ~GuiApplication ();

  QApplication *qapp_gui () { return this; }

  /**
   *  @brief Does some pre-initialization
   *
   *  Must be called before the GuiApplication object is created
   */
  static void initialize ();

  /**
   *  @brief Reimplementation of notify from QApplication
   */
  bool notify (QObject *receiver, QEvent *e);

  /**
   *  @brief Gets the application instance, cast to this class
   */
  static GuiApplication *instance ()
  {
    return dynamic_cast<GuiApplication *> (ApplicationBase::instance ());
  }

  /**
   *  @brief Specialization of exec
   */
  int exec ();

  /**
   *  @brief Hides QCoreApplication::exit
   */
  void exit (int result)
  {
    ApplicationBase::exit (result);
  }

  /**
   *  @brief Returns the main window's reference
   */
  virtual MainWindow *main_window () const
  {
    return mp_mw;
  }

  /**
   *  @brief Enters busy mode (true) or leaves it (false)
   *
   *  Use lay::BusySection to declare a section in "busy" mode. In busy mode, some features are disabled to
   *  prevent recursion in processing of events.
   */
  virtual void enter_busy_mode (bool bm);

  /**
   *  @brief Gets a value indicating whether busy mode is enabled
   */
  virtual bool is_busy () const;

  /**
   *  @brief Forces update of the application menu
   *  This function is used for work around a MacOS issue.
   */
  void force_update_app_menu ();

  /**
   *  @brief Handles events
   */
  bool event (QEvent *event);

protected:
  virtual void setup ();
  virtual void shutdown ();
  virtual void finish ();
  virtual void prepare_recording (const std::string &gtf_record, bool gtf_save_incremental);
  virtual void start_recording ();
  virtual void process_events_impl (QEventLoop::ProcessEventsFlags flags, bool silent);

  virtual lay::Dispatcher *dispatcher () const;

private:
  MainWindow *mp_mw;
  gtf::Recorder *mp_recorder;
  int m_in_notify;

  bool do_notify (QObject *receiver, QEvent *e);
};

/**
 *  @brief The non-GUI-enabled application class
 */
class LAY_PUBLIC NonGuiApplication
  : public QCoreApplication, public ApplicationBase
{
public:
  NonGuiApplication (int &argc, char **argv);
  ~NonGuiApplication ();

  /**
   *  @brief Gets the application instance, cast to this class
   */
  static NonGuiApplication *instance ()
  {
    return dynamic_cast<NonGuiApplication *> (ApplicationBase::instance ());
  }

  /**
   *  @brief Specialization of exec
   */
  int exec ();

  /**
   *  @brief Hides QCoreApplication::exit
   */
  void exit (int result)
  {
    ApplicationBase::exit (result);
  }

  /**
   *  @brief Returns the main window's reference
   *  This incarnation returns 0 since no GUI is supported.
   */
  virtual MainWindow *main_window () const
  {
    return 0;
  }

protected:
  virtual void setup ();
  virtual void shutdown ();

  virtual lay::Dispatcher *dispatcher () const
  {
    return mp_dispatcher;
  }

private:
  lay::ProgressReporter *mp_pr;
  lay::ProgressBar *mp_pb;
  lay::Dispatcher *mp_dispatcher;
};

} // namespace lay

#endif


