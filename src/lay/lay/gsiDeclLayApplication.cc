
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "layApplication.h"
#include "layMainWindow.h"
#include "laySignalHandler.h"
#include "gsiDecl.h"
#include "gsiSignals.h"
#include "tlArch.h"

#if defined(HAVE_QTBINDINGS)

#include "gsiQtGuiExternals.h"
#include "gsiQtWidgetsExternals.h"  //  for Qt5
#include "gsiQtCoreExternals.h"
#include "gsiQtXmlExternals.h"

//  this is here *once*
FORCE_LINK_GSI_QTCORE
FORCE_LINK_GSI_QTGUI
FORCE_LINK_GSI_QTWIDGETS
//  required because the GSI bindings use QDomDocument
FORCE_LINK_GSI_QTXML

#endif

namespace gsi
{

/** 
 *  @brief Makes the application crash (for testing the crash handler)
 */
void crash_me (int reason)
{
  union {
    void (*f_ptr)(int);
    char *c_ptr;
    const char *cc_ptr;
  } ugly_cast;

  if (reason == 0) {
    //  SIGABRT
    abort ();
  } else if (reason == 1) {
    //  SIGSEGV
    ugly_cast.c_ptr = 0;
    *ugly_cast.c_ptr = 0;
  } else if (reason == 2) {
    //  SIGILL
    //  NOTE: this triggers an illegal instruction on X86_64
    ugly_cast.cc_ptr = "abcd\0\0\0\0";
    (*ugly_cast.f_ptr) (0);
  } else if (reason == 3) {
    //  SIGFPE
    double x = -1.0;
    printf ("%g", sqrt (x));
  }
}

template <class C>
static std::string arch (C *)
{
  return tl::arch_string ();
}

template <class C>
static std::string version (C *)
{
  return C::version ();
}

template <class C>
static void add_macro_category (C *c, const std::string &name, const std::string &description, const std::vector<std::string> &folders)
{
  c->add_macro_category (name, description, folders);
}

template <class C>
static gsi::Methods application_methods ()
{
  return
    method<int> ("crash_me", &crash_me, gsi::arg ("mode"), "@hide") +
    method<QString, const QString &, size_t> ("symname", &lay::get_symbol_name_from_address, gsi::arg ("mod_name"), gsi::arg ("addr"), "@hide") +
    method<C, bool> ("is_editable?", &C::is_editable,
      "@brief Returns true if the application is in editable mode\n"
    ) +
    //  TODO: basically this method belongs to Dispatcher (aka MainWindow).
    //  There is separate declaration for Dispatcher which we have to synchronize
    //  with this method.
    method<C, std::string, const std::string &> ("get_config", &C::get_config, gsi::arg ("name"),
      "@brief Gets the value for a configuration parameter\n"
      "\n"
      "@param name The name of the configuration parameter whose value shall be obtained (a string)\n"
      "\n"
      "@return The value of the parameter\n"
      "\n"
      "This method returns the value of the given configuration parameter. If the parameter is not "
      "known, an exception will be thrown. Use \\get_config_names to obtain a list of all configuration "
      "parameter names available.\n"
      "\n"
      "Configuration parameters are always stored as strings. The actual format of this string is specific "
      "to the configuration parameter. The values delivered by this method correspond to the values stored "
      "in the configuration file "
    ) +
    //  TODO: basically this method belongs to Dispatcher (aka MainWindow).
    //  There is separate declaration for Dispatcher which we have to synchronize
    //  with this method.
    method<C, std::vector<std::string> > ("get_config_names", &C::get_config_names,
      "@brief Gets the configuration parameter names\n"
      "\n"
      "@return A list of configuration parameter names\n"
      "\n"
      "This method returns the names of all known configuration parameters. These names can be used to "
      "get and set configuration parameter values."
    ) +
    //  TODO: basically this method belongs to Dispatcher (aka MainWindow).
    //  There is separate declaration for Dispatcher which we have to synchronize
    //  with this method.
    method<C, const std::string &, const std::string &> ("set_config", &C::set_config, gsi::arg ("name"), gsi::arg ("value"),
      "@brief Sets a configuration parameter with the given name to the given value\n"
      "\n"
      "@param name The name of the configuration parameter to set\n"
      "@param value The value to which to set the configuration parameter\n"
      "\n"
      "This method sets the configuration parameter with the given name to the given value. "
      "Values can only be strings. Numerical values have to be converted into strings first. "
      "The actual format of the value depends on the configuration parameter. The name must "
      "be one of the names returned by \\get_config_names."
      "\n"
      "It is possible to write an arbitrary name/value pair into the configuration database which then is "
      "written to the configuration file."
    ) +
    //  TODO: basically this method belongs to Dispatcher (aka MainWindow).
    //  There is separate declaration for Dispatcher which we have to synchronize
    //  with this method.
    method<C> ("commit_config", &C::config_end,
      "@brief Commits the configuration settings\n"
      "\n"
      "Some configuration options are queued for performance reasons and become active only after 'commit_config' has been called. "
      "After a sequence of \\set_config calls, this method should be called to activate the "
      "settings made by these calls.\n"
      "\n"
      "This method has been introduced in version 0.25.\n"
    ) +
    //  TODO: basically this method belongs to Dispatcher (aka MainWindow).
    //  There is separate declaration for Dispatcher which we have to synchronize
    //  with this method.
    method<C, bool, const std::string &> ("write_config", &C::write_config, gsi::arg ("file_name"),
      "@brief Writes configuration to a file\n"
      "@return A value indicating whether the operation was successful\n"
      "\n"
      "If the configuration file cannot be written, \n"
      "is returned but no exception is thrown.\n"
    ) +
    //  TODO: basically this method belongs to Dispatcher (aka MainWindow).
    //  There is separate declaration for Dispatcher which we have to synchronize
    //  with this method.
    method<C, bool, const std::string &> ("read_config", &C::read_config, gsi::arg ("file_name"),
      "@brief Reads the configuration from a file\n"
      "@return A value indicating whether the operation was successful\n"
      "\n"
      "This method silently does nothing, if the config file does not\n"
      "exist. If it does and an error occurred, the error message is printed\n"
      "on stderr. In both cases, false is returned.\n"
    ) +
    method<C, lay::MainWindow *> ("main_window", &C::main_window,
      "@brief Returns a reference to the main window\n"
      "\n"
      "@return A object reference to the main window object."
    ) +
    method<C> ("execute|#exec", &C::exec,
      "@brief Executes the application's main loop\n"
      "\n"
      "This method must be called in order to execute the application in the main "
      "script if a script is provided."
    ) +
    method<C> ("process_events", (void (C::*)()) &C::process_events,
      "@brief Processes pending events\n"
      "\n"
      "This method processes pending events and dispatches them internally. Calling this "
      "method periodically during a long operation keeps the application 'alive'"
    ) +
    method<C, const std::string &> ("application_data_path", &C::appdata_path,
      "@brief Returns the application's data path (where the configuration file is stored for example)\n"
      "\n"
      "This method has been added in version 0.22."
    ) +
    method<C, const std::string &> ("inst_path", &C::inst_path,
      "@brief Returns the application's installation path (where the executable is located)\n"
      "\n"
      "This method has been added in version 0.18. Version 0.22 offers the method \\klayout_path which "
      "delivers all components of the search path."
    ) +
    method<C, const std::vector<std::string> &> ("klayout_path", &C::klayout_path,
      "@brief Returns the KLayout path (search path for KLayout components)\n"
      "\n"
      "The result is an array containing the components of the path.\n"
      "\n"
      "This method has been added in version 0.22."
    ) +
    method<C, int> ("exit", &C::exit, gsi::arg ("result"),
      "@brief Ends the application with the given exit status\n"
      "\n"
      "This method should be called instead of simply shutting down the process. It performs some "
      "important cleanup without which the process might crash. If the result code is 0 (success), "
      "the configuration file will be updated unless that has been disabled by the -nc command line switch."
      "\n"
      "This method has been added in version 0.22."
    ) +
    method_ext<C, std::string> ("version", &version<C>,
      "@brief Returns the application's version string\n"
    ) +
    method_ext<C, std::string> ("arch", &arch<C>,
      "@brief Returns the architecture string\n"
      "This method has been introduced in version 0.25."
    ) +
    method_ext<C, const std::string &, const std::string &, const std::vector<std::string> &> ("add_macro_category", &add_macro_category<C>, gsi::arg ("name"), gsi::arg ("description"), gsi::arg ("folders"),
      "@brief Creates a new macro category\n"
      "Creating a new macro category is only possible during the autorun_early stage. "
      "The new macro category must correspond to an interpreter registered at the same stage.\n"
      "This method has been introduced in version 0.28."
    ) +
    method<C *> ("instance", &C::instance,
      "@brief Return the singleton instance of the application\n"
      "\n"
      "There is exactly one instance of the application. This instance can be obtained with this "
      "method."
    ) +
    event<C> ("on_salt_changed", &C::salt_changed_event,
      "@brief This event is triggered when the package status changes.\n"
      "\n"
      "Register to this event if you are interested in package changes - i.e. installation or removal of packages or "
      "package updates.\n"
      "\n"
      "This event has been introduced in version 0.28."
    )
  ;
}

static std::string application_doc ()
{
  return
    "@brief The application object\n"
    "\n"
    "The application object is the main port from which to access all the internals "
    "of the application, in particular the main window."
  ;
}

/**
 *  @brief Creates the right application object declaration depending on the mode
 *
 *  This declaration factory will register a GuiApplication declaration (derived from QApplication)
 *  if in GUI mode and a NonGuiApplication declaration (derived from QCoreApplication).
 */
void
LAY_PUBLIC make_application_decl (bool non_gui_mode)
{
  static std::unique_ptr<Class<lay::GuiApplication> > gui_app_decl;
  static std::unique_ptr<Class<lay::NonGuiApplication> > non_gui_app_decl;

  if (non_gui_mode) {

    non_gui_app_decl.reset (
      new Class<lay::NonGuiApplication> (
#if defined(HAVE_QTBINDINGS)
        QT_EXTERNAL_BASE (QCoreApplication)
#endif
        "lay", "Application",
        application_methods<lay::NonGuiApplication> (),
        application_doc ()
      )
    );

  } else {

    gui_app_decl.reset (
      new Class<lay::GuiApplication> (
#if defined(HAVE_QTBINDINGS)
        QT_EXTERNAL_BASE (QApplication)
#endif
        "lay", "Application",
        application_methods<lay::GuiApplication> (),
        application_doc ()
      )
    );

  }
}

}
