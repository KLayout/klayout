
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


#include "layApplication.h"
#include "laySignalHandler.h"
#include "laybasicConfig.h"
#include "layConfig.h"
#include "layMainWindow.h"
#include "layMacroEditorDialog.h"
#include "layNativePlugin.h"
#include "layVersion.h"
#include "laySignalHandler.h"
#include "layRuntimeErrorForm.h"
#include "layReaderErrorForm.h"
#include "layProgress.h"
#include "layTextProgress.h"
#include "layBackgroundAwareTreeStyle.h"
#include "layMacroController.h"
#include "layTechnologyController.h"
#include "laySaltController.h"
#include "laySystemPaths.h"
#include "layPasswordDialog.h"
#include "layInit.h"
#include "lymMacro.h"
#include "gtf.h"
#include "gsiDecl.h"
#include "gsiInterpreter.h"
#include "rba.h"
#include "pya.h"
#include "rdb.h"
#include "dbStatic.h"
#include "dbLibrary.h"
#include "dbLibraryManager.h"
#include "dbInit.h"
#include "dbLayoutToNetlist.h"
#include "tlExceptions.h"
#include "tlException.h"
#include "tlAssert.h"
#include "tlLog.h"
#include "tlString.h"
#include "tlExpression.h"
#include "tlExceptions.h"
#include "tlInternational.h"
#include "tlHttpStream.h"
#include "tlArch.h"
#include "tlFileUtils.h"

#include <QIcon>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QAction>
#include <QMessageBox>

#include <iostream>
#include <memory>
#include <algorithm>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace gsi
{
  void make_application_decl (bool non_gui_mode);
}

namespace lay
{

// --------------------------------------------------------------------------------
//  Exception handlers

static void ui_exception_handler_tl (const tl::Exception &ex, QWidget *parent)
{
  //  Prevents severe side effects if there are pending deferred methods
  tl::NoDeferredMethods silent;

  //  if any transaction is pending (this may happen when an operation threw an exception)
  //  close transactions.
  if (lay::MainWindow::instance () && lay::MainWindow::instance ()->manager ().transacting ()) {
    lay::MainWindow::instance ()->manager ().commit ();
  }

  const tl::ExitException *gsi_exit = dynamic_cast <const tl::ExitException *> (&ex);
  const tl::BreakException *gsi_break = dynamic_cast <const tl::BreakException *> (&ex);
  const tl::ScriptError *gsi_excpt = dynamic_cast <const tl::ScriptError *> (&ex);
  const db::ReaderUnknownFormatException *reader_excpt = dynamic_cast <const db::ReaderUnknownFormatException *> (&ex);

  if (gsi_exit || gsi_break) {
    //  exit and break exceptions are not shown - they are issued when a script is aborted or
    //  an operation is cancelled
  } else if (gsi_excpt) {

    //  show and GSI errors in the context of the macro editor if that is open
    if (! parent && lay::MacroEditorDialog::instance () && lay::MacroEditorDialog::instance ()->isVisible ()) {
      parent = lay::MacroEditorDialog::instance ();
    } 
    if (! parent) {
      parent = QApplication::activeWindow () ? QApplication::activeWindow () : lay::MainWindow::instance ();
    }

    if (gsi_excpt->line () > 0) {
      tl::error << gsi_excpt->sourcefile () << ":" << gsi_excpt->line () << ": " 
                << gsi_excpt->msg ();
    } else {
      tl::error << gsi_excpt->msg ();
    }

    lay::RuntimeErrorForm error_dialog (parent, "ruby_error_form", gsi_excpt);
    error_dialog.exec ();

  } else {

    tl::error << ex.msg (); 
    if (! parent) {
      parent = QApplication::activeWindow () ? QApplication::activeWindow () : lay::MainWindow::instance ();
    }

    if (reader_excpt) {
      lay::ReaderErrorForm error_dialog (parent, "reader_error_form", reader_excpt);
      error_dialog.exec ();
    } else {
      QMessageBox::critical (parent, QObject::tr ("Error"), tl::to_qstring (ex.msg ()));
    }

  }
}

static void ui_exception_handler_std (const std::exception &ex, QWidget *parent)
{
  //  Prevents severe side effects if there are pending deferred methods
  tl::NoDeferredMethods silent;

  //  if any transaction is pending (this may happen when an operation threw an exception)
  //  close transactions.
  if (lay::MainWindow::instance () && lay::MainWindow::instance ()->manager ().transacting ()) {
    lay::MainWindow::instance ()->manager ().commit ();
  }

  tl::error << ex.what (); 
  if (! parent) {
    parent = QApplication::activeWindow () ? QApplication::activeWindow () : lay::MainWindow::instance ();
  }
  QMessageBox::critical (parent, QObject::tr ("Error"), tl::to_qstring (ex.what ())); 
}

static void ui_exception_handler_def (QWidget *parent)
{
  //  Prevents severe side effects if there are pending deferred methods
  tl::NoDeferredMethods silent;

  //  if any transaction is pending (this may happen when an operation threw an exception)
  //  close transactions.
  if (lay::MainWindow::instance () && lay::MainWindow::instance ()->manager ().transacting ()) {
    lay::MainWindow::instance ()->manager ().commit ();
  }

  if (! parent) {
    parent = QApplication::activeWindow () ? QApplication::activeWindow () : lay::MainWindow::instance ();
  }
  QMessageBox::critical (parent, QObject::tr ("Error"), QObject::tr ("An unspecific error occurred")); 
}

// --------------------------------------------------------------------------------

static ApplicationBase *ms_instance = 0;

// --------------------------------------------------------------------------------
//  ApplicationBase implementation

ApplicationBase::ApplicationBase (bool non_ui_mode)
  : gsi::ObjectBase (),
    m_lyp_map_all_cvs (true), 
    m_lyp_add_default (false),
    m_run_macro_and_exit (true),
    m_packages_with_dep (false),
    m_write_config_file (false),
    m_gtf_replay_rate (0),
    m_gtf_replay_stop (-1),
    m_gtf_record (),
    m_gtf_save_incremental (false),
    m_no_macros (false),
    m_same_view (false),
    m_sync_mode (false),
    m_no_gui (non_ui_mode),
    m_vo_mode (false),
    m_editable (false),
    m_editable_set (false),
    m_enable_undo (true),
    mp_ruby_interpreter (0),
    mp_python_interpreter (0)
{
  // TODO: offer a strict mode for exception handling where this takes place:
  // lay::ApplicationBase::instance ()->exit (1);
  if (! m_no_gui) {
    tl::set_ui_exception_handlers (ui_exception_handler_tl, ui_exception_handler_std, ui_exception_handler_def);
  }

  gsi::make_application_decl (m_no_gui);

  //  initialize the system codecs (Hint: this must be done after the QApplication is initialized because
  //  it will call setlocale)
  tl::initialize_codecs ();

  tl_assert (ms_instance == 0);
  ms_instance = this;

  //  get and create the klayout appdata folder if required
  m_appdata_path = lay::get_appdata_path ();

  //  get the installation path
  m_inst_path = tl::get_inst_path ();
}

void
ApplicationBase::parse_cmd (int &argc, char **argv)
{
  //  get the KLayout path
  m_klayout_path = lay::get_klayout_path ();

  //  by default write the configuration
  m_write_config_file = true;

  if (! m_no_gui) {

    //  create the configuration files paths and collect the initialization config files
    //  (the ones used for reset) into m_initial_config_files.
    {
      //  Fallback to ~/.layviewrc for backward compatibility
      QDir qd (QDir::home ());
      QString filename = QString::fromUtf8 (".layviewrc");
      if (qd.exists (filename) && QFileInfo (qd.absoluteFilePath (filename)).isReadable ()) {
        m_config_files.push_back (tl::to_string (qd.absoluteFilePath (filename)));
        m_config_file_to_delete = m_config_files.back ();
      }
    }

    m_config_file_to_write.clear ();
    if (! m_appdata_path.empty ()) {
      m_config_file_to_write = tl::to_string (QDir (tl::to_qstring (m_appdata_path)).absoluteFilePath (QString::fromUtf8 ("klayoutrc")));
    }

    //  Hint: the order is reverse in the sense that the first one wins ...
    for (std::vector <std::string>::const_iterator p = m_klayout_path.end (); p != m_klayout_path.begin (); ) {
      --p;
      QDir qd (tl::to_qstring (*p));
      QString filename = QString::fromUtf8 ("klayoutrc");
      if (qd.exists (filename) && QFileInfo (qd.absoluteFilePath (filename)).isReadable ()) {
        m_config_files.push_back (tl::to_string (qd.absoluteFilePath (filename)));
        if (m_config_files.back () != m_config_file_to_write) {
          m_initial_config_files.push_back (m_config_files.back ());
        }
      }
    }

  }

  //  transscribe the arguments to UTF8
  std::vector<std::string> args;
  args.reserve (argc);
  for (int i = 0; i < argc; ++i) {
    args.push_back (argv [i]);
  }

#if defined(KLAYOUT_VIEWER_ONLY)
  //  viewer-only mode compiled in
  m_vo_mode = true;
#else
  //  determine viewer-only mode from executable name. "klayout_vo*" will enable
  //  viewer-only mode
  std::string vo_exe_name (std::string (lay::Version::exe_name ()) + "_vo");
  if (! args.empty () && std::string (tl::to_string (QFileInfo (tl::to_qstring (args.front ())).fileName ()), 0, vo_exe_name.size ()) == vo_exe_name) {
    m_vo_mode = true;
  }
#endif

  m_no_macros = false;

  // currently: technology is always set to make "default" technology the default
  bool tech_set = true;
  std::string tech;
  std::string tech_file;

  for (int i = 1; i < argc; ++i) {

    const std::string &a = args [i];

    if (a == "-d" && (i + 1) < argc) {

      int v = 0;
      tl::from_string (args [++i], v);
      if (v < 0) {
        v = 0;
      }
      tl::verbosity (v);

    } else if (a == "-k" && (i + 1) < argc) {

      //  ignored (handled earlier)
      ++i;

    } else if (a == "-l" && (i + 1) < argc) {

      m_layer_props_file = args [++i];

    } else if (a == "-lx") {

      m_lyp_add_default = true;

    } else if (a == "-lf") {

      m_lyp_map_all_cvs = false;

    } else if (a == "-u" && (i + 1) < argc) {

      m_session_file = args [++i];

    } else if (a == "-wd" && (i + 1) < argc) {

      const char *p = args [++i].c_str ();
      const char *n0 = p;
      while (*p && *p != '=') {
        ++p;
      }
      std::string n (n0, p - n0);
      if (*p == '=') {
        tl::Eval::set_global_var (n, tl::Variant (p + 1));
      } else {
        tl::Eval::set_global_var (n, tl::Variant (true));
      }

    } else if (a == "-rd" && (i + 1) < argc) {

      std::string v;
      const char *p = args [++i].c_str ();
      const char *n0 = p;
      while (*p && *p != '=') {
        ++p;
      }
      std::string n (n0, p - n0);
      m_variables.push_back (std::make_pair (n, v));
      if (*p == '=') {
        m_variables.back ().second = ++p;
      }

    } else if (a == "-rm" && (i + 1) < argc) {

      m_load_macros.push_back (args [++i]);

    } else if (a == "-r" && (i + 1) < argc) {

      if (! m_run_macro.empty ()) {
        throw tl::Exception (tl::to_string (QObject::tr ("Option -r or -rr can only be used once")));
      }
      m_run_macro = args [++i];

    } else if (a == "-rr" && (i + 1) < argc) {

      if (! m_run_macro.empty ()) {
        throw tl::Exception (tl::to_string (QObject::tr ("Option -r or -rr can only be used once")));
      }
      m_run_macro = args [++i];
      m_run_macro_and_exit = false;

    } else if (a == "-rx") {

      m_no_macros = true;

    } else if (a == "-gr" && (i + 1) < argc) {

      m_gtf_record = args [++i];

    } else if (a == "-gi") {

      m_gtf_save_incremental = true;

    } else if (a == "-gp" && (i + 1) < argc) {

      m_gtf_replay = args [++i];
      //  test mode replay forces sync mode to true
      m_sync_mode = true;

    } else if (a == "-gx" && (i + 1) < argc) {

      int r = 0;
      tl::from_string (args [++i], r);
      m_gtf_replay_rate = r > 0 ? r : 0;

    } else if (a == "-gb" && (i + 1) < argc) {

      int s = 0;
      tl::from_string (args [++i], s);
      m_gtf_replay_stop = s > 0 ? s : 0;

    } else if (a == "-c" && (i + 1) < argc) {

      m_config_files.clear ();
      m_config_files.push_back (args [++i]);
      m_initial_config_files = m_config_files;
      m_config_file_to_write = m_config_files.back ();

    } else if (a == "-nc") {

      m_config_files.clear ();
      m_initial_config_files = m_config_files;
      m_config_file_to_write.clear ();

    } else if (a == "-n" && (i + 1) < argc) {

      tech = args [++i];
      tech_file.clear ();
      tech_set = true;

    } else if (a == "-nn" && (i + 1) < argc) {

      tech_file = args [++i];
      tech.clear ();
      tech_set = true;

    } else if (a == "-p" && (i + 1) < argc) {

      lay::load_plugin (args [++i]);

    } else if (a == "-s") {

      m_same_view = true;

    } else if (a == "-e") {

      m_editable = ! m_vo_mode;
      m_editable_set = true;

    } else if (a == "-ne") {

      m_editable = false;
      m_editable_set = true;

    } else if (a == "-i") {

      m_enable_undo = false;

    } else if (a == "-ni") {

      m_enable_undo = true;

    } else if (a == "-j" && (i + 1) < argc) {

      m_custom_macro_paths.push_back (std::pair<std::string, std::string> (args [++i], std::string ()));

    } else if (a == "-nt") {

      m_write_config_file = true;

    } else if (a == "-t") {

      m_write_config_file = false;

    } else if (a == "-z") {

      m_no_gui = true;

    } else if (a == "-zz") {

      m_no_gui = true;
      //  other consequences have been dealt with before

    } else if (a == "-b") {

      //  -nc:
      m_config_files.clear ();
      m_initial_config_files = m_config_files;
      m_config_file_to_write.clear ();

      //  -rx:
      m_no_macros = true;

      //  -zz:
      m_no_gui = true;
      //  other consequences have been dealt with before

    } else if (a == "-x") {

      m_sync_mode = true;

    } else if (a == "-y" && (i + 1) < argc) {

      m_package_inst.push_back (args [++i]);

    } else if (a == "-yd") {

      m_packages_with_dep = true;

    } else if (a == "-m" && (i + 1) < argc) {

      m_files.push_back (std::make_pair (rdb_file, std::make_pair (std::string (args [++i]), std::string ())));

    } else if (a == "-mn" && (i + 1) < argc) {

      m_files.push_back (std::make_pair (l2ndb_file, std::make_pair (std::string (args [++i]), std::string ())));

    } else if (a[0] == '-') {

      throw tl::Exception (tl::to_string (QObject::tr ("Unknown option: ")) + a);

    } else if (tech_set) {

      if (! tech.empty ()) {
        m_files.push_back (std::make_pair (layout_file_with_tech, std::make_pair (a, tech)));
      } else if (! tech_file.empty ()) {
        m_files.push_back (std::make_pair (layout_file_with_tech_file, std::make_pair (a, tech_file)));
      } else {
        m_files.push_back (std::make_pair (layout_file, std::make_pair (a, std::string ())));
      }

    } else {

      m_files.push_back (std::make_pair (layout_file, std::make_pair (a, std::string ())));

    }

  }
}

void
ApplicationBase::init_app ()
{
  //  Try to locate the plugins:
  //  The are installed either
  //    - directly in one of the KLAYOUT_PATH directories
  //    - in a folder named by the architecture (i.e. "i686-win32-mingw" or "x86_64-linux-gcc") below
  //      one of these folders
  //    - in one of the Salt packages
  //    - in one of the Salt packages, in a folder named after the architecture
  //  Below this, the following folders are looked up:
  //    - db_plugins    for db module plugins
  //    - lay_plugins   for lay module plugins

  std::string version = lay::Version::version ();
  std::vector<std::string> vv = tl::split (version, ".");

  std::string arch_string = tl::arch_string ();
  std::vector<std::string> as = tl::split (arch_string, "-");
  if (as.size () > 2) {
    as.resize (2);
  }
  std::string short_arch_string = tl::join (as, "-");

  std::vector<std::string> klp_paths;

  for (std::vector <std::string>::const_iterator p = m_klayout_path.begin (); p != m_klayout_path.end (); ++p) {

    klp_paths.push_back (*p);
    klp_paths.push_back (tl::to_string (QDir (tl::to_qstring (klp_paths.back ())).filePath (tl::to_qstring (tl::arch_string ()))));

    lay::Salt salt;
    salt.add_location (tl::to_string (QDir (tl::to_qstring (*p)).filePath (QString::fromUtf8 ("salt"))));

    //  Build the search path for the plugin locations. The search priority is for example:
    //    salt/mypackage/x86_64-linux-gcc-0.25.1
    //    salt/mypackage/x86_64-linux-gcc-0.25
    //    salt/mypackage/x86_64-linux-gcc-0
    //    salt/mypackage/x86_64-linux-gcc
    //    salt/mypackage/x86_64-linux
    //    salt/mypackage

    for (lay::Salt::flat_iterator g = salt.begin_flat (); g != salt.end_flat (); ++g) {
      QDir dir = QDir (tl::to_qstring ((*g)->path ()));
      klp_paths.push_back (tl::to_string (dir.filePath (tl::to_qstring (arch_string + "-" + lay::Version::version()))));
      if (vv.size () >= 2) {
        klp_paths.push_back (tl::to_string (dir.filePath (tl::to_qstring (arch_string + "-" + vv[0] + "." + vv[1]))));
      }
      if (vv.size () >= 1) {
        klp_paths.push_back (tl::to_string (dir.filePath (tl::to_qstring (arch_string + "-" + vv[0]))));
      }
      klp_paths.push_back (tl::to_string (dir.filePath (tl::to_qstring (arch_string + "-" + tl::to_string (lay::Version::version ())))));
      klp_paths.push_back (tl::to_string (dir.filePath (tl::to_qstring (arch_string))));
      klp_paths.push_back (tl::to_string (dir.filePath (tl::to_qstring (short_arch_string))));
      klp_paths.push_back ((*g)->path ());
    }

  }

  //  initialize the modules (load their plugins from the paths)
  db::init (klp_paths);
  lay::init (klp_paths);

  //  initialize the GSI class system (Variant binding, Expression support)
  //  We have to do this now since plugins may register GSI classes and before the
  //  ruby interpreter, because it depends on a proper class system.
  gsi::initialize ();

  //  initialize the tl::Expression subsystem with GSI-bound classes
  gsi::initialize_expressions ();

  //  create the ruby and python interpreter instances now.
  //  Hint: we do this after load_plugin, because that way the plugins can register GSI classes and methods.
  //  TODO: do this through some auto-registration
  mp_ruby_interpreter = new rba::RubyInterpreter ();
  mp_python_interpreter = new pya::PythonInterpreter ();

  //  Read some configuration values that we need early
  bool editable_from_config = false;

  {
    lay::Dispatcher cfg;

    for (std::vector <std::string>::const_iterator c = m_config_files.begin (); c != m_config_files.end (); ++c) {
      try {
        cfg.read_config (*c);
      } catch (...) { }
    }

    try {
      cfg.config_get (cfg_edit_mode, editable_from_config);
    } catch (...) { }

    try {
      std::string mp;
      cfg.config_get (cfg_custom_macro_paths, mp);
      tl::Extractor ex (mp.c_str ());
      while (! ex.at_end ()) {
        std::string p;
        ex.read_word_or_quoted (p);
        m_custom_macro_paths.push_back (std::pair<std::string, std::string> (p, std::string ()));
        if (ex.test (":")) {
          ex.read_word (m_custom_macro_paths.back ().second);
        }
        ex.test (";");
      }
    } catch (...) { }

  }

  //  Install the signal handlers after the interpreters, so we can be sure we
  //  installed our handler.
  install_signal_handlers ();

  lay::SaltController *sc = lay::SaltController::instance ();
  lay::TechnologyController *tc = lay::TechnologyController::instance ();
  lay::MacroController *mc = lay::MacroController::instance ();

  if (sc) {

    //  auto-import salt grains
    for (std::vector <std::string>::const_iterator p = m_klayout_path.begin (); p != m_klayout_path.end (); ++p) {
      sc->add_path (*p);
    }

    sc->set_salt_mine_url (lay::salt_mine_url ());

    //  Do package installation if requested.
    if (!m_package_inst.empty ()) {
      if (! sc->install_packages (m_package_inst, m_packages_with_dep)) {
        exit (1);
      } else {
        exit (0);
      }
    }

    sc->salt_changed_event.add (this, &ApplicationBase::salt_changed);

  }

  if (tc) {

    //  auto-import technologies
    for (std::vector <std::string>::const_iterator p = m_klayout_path.begin (); p != m_klayout_path.end (); ++p) {
      tc->add_path (*p);
    }

    tc->load ();

    bool needs_reload = false;

    //  disambiguator for tech name
    std::map<std::string, int> tech_disambiguator;
    std::map<std::string, std::string> tech_name_for_file;

    for (auto t = db::Technologies::instance ()->begin (); t != db::Technologies::instance ()->end (); ++t) {
      tech_disambiguator.insert (std::make_pair (t->name (), 0));
    }

    //  import technologies from the command line
    for (std::vector <std::pair<file_type, std::pair<std::string, std::string> > >::iterator f = m_files.begin (); f != m_files.end (); ++f) {

      if (f->first == layout_file_with_tech_file) {

        const std::string &tech_file = f->second.second;

        if (tl::verbosity () >= 20) {
          tl::info << "Importing technology from " << f->second.second;
        }

        db::Technology t;
        t.load (tech_file);

        //  disambiguate the name, so we do not overwrite technologies with the same name from the config
        if (tech_name_for_file.find (tech_file) != tech_name_for_file.end ()) {
          t.set_name (tech_name_for_file [tech_file]);
        } else if (tech_disambiguator.find (t.name ()) != tech_disambiguator.end ()) {
          int &index = tech_disambiguator [t.name ()];
          index += 1;
          t.set_name (t.name () + tl::sprintf ("[%d]", index));
          tech_name_for_file.insert (std::make_pair (tech_file, t.name ()));
        } else {
          tech_disambiguator.insert (std::make_pair (t.name (), 0));
          tech_name_for_file.insert (std::make_pair (tech_file, t.name ()));
        }

        tc->add_temp_tech (t);

        f->first = layout_file_with_tech;
        f->second.second = t.name ();

        needs_reload = true;

      }

    }

    if (needs_reload) {
      tc->load ();
    }

  }

  if (mc) {

    //  create the basic macro categories

    if (ruby_interpreter ().available ()) {
      std::vector<std::string> folders;
      folders.push_back ("macros");
      folders.push_back ("ruby");
      mc->add_macro_category ("macros", "Ruby", folders);
    }

    if (python_interpreter ().available ()) {
      std::vector<std::string> folders;
      folders.push_back ("pymacros");
      folders.push_back ("python");
      mc->add_macro_category ("pymacros", "Python", folders);
    }

    mc->enable_implicit_macros (! m_no_macros);

    //  Add the global ruby modules as the first ones.
    //  TODO: this is a deprecated feature.
    std::vector<std::string> global_modules = scan_global_modules ();
    m_load_macros.insert (m_load_macros.begin (), global_modules.begin (), global_modules.end ());

    size_t local_folders = (lay::get_appdata_path ().empty () ? 0 : 1);

    for (std::vector <std::string>::const_iterator p = m_klayout_path.begin (); p != m_klayout_path.end (); ++p) {
      if (size_t (p - m_klayout_path.begin ()) < local_folders) {
        mc->add_path (*p, tl::to_string (QObject::tr ("Local")), std::string (), false);
      } else if (m_klayout_path.size () == 1 + local_folders) {
        mc->add_path (*p, tl::to_string (QObject::tr ("Global")), std::string (), true);
      } else {
        mc->add_path (*p, tl::to_string (QObject::tr ("Global")) + " - " + *p, std::string (), true);
      }
    }

    //  Install the custom folders
    for (std::vector <std::pair<std::string, std::string> >::const_iterator p = m_custom_macro_paths.begin (); p != m_custom_macro_paths.end (); ++p) {
      mc->add_path (p->first, tl::to_string (QObject::tr ("Project")) + " - " + p->first, p->second, false);
    }

    //  Actually load the macros and/or establish the search path
    mc->finish ();

  }

  //  If the editable flag was not set, use it from the 
  //  configuration. Since it is too early now, we cannot use the
  //  configuration once it is read
  if (! m_editable_set && ! m_vo_mode) {
    m_editable = editable_from_config;
  }

  db::set_default_editable_mode (m_editable);

  if (! m_gtf_record.empty ()) {
    //  since the recorder tracks QAction connections etc., it must be instantiated before every other
    //  object performing a gtf::action_connect for example
    prepare_recording (m_gtf_record, m_gtf_save_incremental);
  }

  tl::Eval::set_global_var ("appdata_path", tl::Variant (m_appdata_path));
  tl::Eval::set_global_var ("inst_path", tl::Variant (m_inst_path));

  tl::Variant kp (m_klayout_path.begin (), m_klayout_path.end ());
  tl::Eval::set_global_var ("klayout_path", kp);

  //  call "autorun_early" on all plugins that wish so
  for (std::list<lay::PluginDescriptor>::const_iterator p = lay::plugins ().begin (); p != lay::plugins ().end (); ++p) {
    if (p->autorun_early) {
      (*p->autorun_early) ();
    }
  }

  //  run all early autorun macros
  lym::MacroCollection::root ().autorun_early ();

  //  redo gsi::initialize as the macros may have registered new external classes
  //  through the "doc to external class" mechanism.
  gsi::initialize ();

  //  autorun_early may have added macro categories, so we need to call finish() again
  if (mc) {

    mc->finish ();

    //  as this regenerates the macro collection, autorun_early is required again
    //  note: this does not re-execute macros that have been executed already
    lym::MacroCollection::root ().autorun_early ();

  }

  //  rescan the folders because early autorun macros might have added 
  //  suffixes through the MacroInterpreter interface.
  lym::MacroCollection::root ().rescan ();

  //  and yet another autorun_early pass ..
  lym::MacroCollection::root ().autorun_early ();

  //  creates the main window or plugin root as required
  setup ();

  //  initialize the plugins for the first time
  if (tl::verbosity () >= 20) {
    tl::info << "Initializing plugins:";
  }
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    lay::PluginDeclaration *pd = const_cast<lay::PluginDeclaration *> (&*cls);
    if (tl::verbosity () >= 20) {
      tl::info << "  " << cls.current_name () << " [" << cls.current_position () << "]";
    }
    pd->initialize (dispatcher ());
  }

  //  establish the configuration
  dispatcher ()->config_setup ();

  //  deferred method processing for those plugins which need this
  process_events ();

  //  some info output
  if (tl::verbosity () >= 20) {

    tl::info << "KLayout path:";
    for (std::vector <std::string>::const_iterator c = m_klayout_path.begin (); c != m_klayout_path.end (); ++c) {
      tl::info << "  " << *c;
    }
    tl::info << "Config file to write: " << m_config_file_to_write;
    tl::info << "Config files to read:";
    for (std::vector <std::string>::const_iterator c = m_config_files.begin (); c != m_config_files.end (); ++c) {
      tl::info << "  " << *c;
    }

  }
}

void
ApplicationBase::add_macro_category (const std::string &name, const std::string &description, const std::vector<std::string> &folders)
{
  lay::MacroController *mc = lay::MacroController::instance ();
  if (mc) {
    mc->add_macro_category (name, description, folders);
  }
}

void
ApplicationBase::salt_changed ()
{
BEGIN_PROTECTED_SILENT
  salt_changed_event ();
END_PROTECTED_SILENT
}

ApplicationBase::~ApplicationBase ()
{
  tl::set_ui_exception_handlers (0, 0, 0);

  //  check whether shutdown was called
  tl_assert (ms_instance == 0);
}

std::vector<std::string>
ApplicationBase::scan_global_modules ()
{
  //  NOTE:
  //  this is deprecated functionality - for backward compatibility, global "*.rbm" and "*.pym" modules
  //  are still considered. The desired solution is autorun macros.

  //  try to locate a global rbainit file and rbm modules
  std::vector<std::string> global_modules;
  std::set<std::string> modules;

  //  try to locate a global plugins
  for (std::vector <std::string>::const_iterator p = m_klayout_path.begin (); p != m_klayout_path.end (); ++p) {

#if 0
    //  deprecated functionality
    QFileInfo rbainit_file (tl::to_qstring (*p), QString::fromUtf8 ("rbainit"));
    if (rbainit_file.exists () && rbainit_file.isReadable ()) {
      std::string m = tl::to_string (rbainit_file.absoluteFilePath ());
      if (modules.find (m) == modules.end ()) {
        global_modules.push_back (m);
        modules.insert (m);
      }
    }
#endif

    QDir inst_path_dir (tl::to_qstring (*p));

    QStringList name_filters;
    name_filters << QString::fromUtf8 ("*.rbm");
    name_filters << QString::fromUtf8 ("*.pym");

    QStringList inst_modules = inst_path_dir.entryList (name_filters);
    inst_modules.sort ();

    for (QStringList::const_iterator im = inst_modules.begin (); im != inst_modules.end (); ++im) {
      QFileInfo rbm_file (tl::to_qstring (*p), *im);
      if (rbm_file.exists () && rbm_file.isReadable ()) {
        std::string m = tl::to_string (rbm_file.absoluteFilePath ());
        if (modules.find (m) == modules.end ()) {
          tl::warn << tl::to_string (QObject::tr ("Global modules are deprecated. Turn '%1' into an autorun macro instead and put it into 'macros' or 'pymacros'.").arg (tl::to_qstring (m)));
          global_modules.push_back (m);
          modules.insert (m);
        }
      }
    }

  }

  return global_modules;
}

void
ApplicationBase::exit (int result)
{
  if (! result) {
    finish ();
  }

  //  uninitialize the plugins
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    lay::PluginDeclaration *pd = const_cast<lay::PluginDeclaration *> (&*cls);
    pd->uninitialize (dispatcher ());
  }

  shutdown ();

  ::exit (result);
}

void 
ApplicationBase::finish ()
{
  if (dispatcher () && m_write_config_file) {

    if (! m_config_file_to_write.empty ()) {
      if (tl::verbosity () >= 20) {
        tl::info << tl::to_string (QObject::tr ("Updating configuration file ")) << m_config_file_to_write;
      }
      dispatcher ()->write_config (m_config_file_to_write);
    }
    if (! m_config_file_to_delete.empty () && m_config_file_to_delete != m_config_file_to_write) {
      if (tl::verbosity () >= 20) {
        tl::info << tl::to_string (QObject::tr ("Deleting configuration file ")) << m_config_file_to_delete;
      }
      QFile::remove (tl::to_qstring (m_config_file_to_delete));
    }

  }
}

void
ApplicationBase::prepare_recording (const std::string & /*gtf_record*/, bool /*gtf_record_incremental*/)
{
  //  the base class does nothing
}

void
ApplicationBase::start_recording ()
{
  //  the base class does nothing
}

void
ApplicationBase::shutdown ()
{
  if (mp_ruby_interpreter) {
    delete mp_ruby_interpreter;
    mp_ruby_interpreter = 0;
  }

  if (mp_python_interpreter) {
    delete mp_python_interpreter;
    mp_python_interpreter = 0;
  }

  ms_instance = 0;
}

ApplicationBase *
ApplicationBase::instance ()
{
  return ms_instance;
}

std::string
ApplicationBase::version ()
{
  return std::string (lay::Version::name ()) + " " + lay::Version::version ();
}
      
std::string 
ApplicationBase::usage ()
{
  std::string r;
  r = std::string (lay::Version::exe_name ()) + " [<options>] [<file>] ..\n";
  r += tl::to_string (QObject::tr ("options")) + "\n";
  r += tl::to_string (QObject::tr ("  -b                  Batch mode (same as -zz -nc -rx)")) + "\n";
  r += tl::to_string (QObject::tr ("  -c <config file>    Use this configuration file")) + "\n";
  r += tl::to_string (QObject::tr ("  -nc                 Don't use a configuration file (implies -t)")) + "\n";
  r += tl::to_string (QObject::tr ("  -d <debug level>    Set debug level")) + "\n";
  r += tl::to_string (QObject::tr ("  -e                  Editable mode (allow editing of files)")) + "\n";
  r += tl::to_string (QObject::tr ("  -ne                 Readonly mode (editing of files is disabled)")) + "\n";
  r += tl::to_string (QObject::tr ("  -gr <file name>     Record GUI test file")) + "\n";
  r += tl::to_string (QObject::tr ("  -gp <file name>     Replay GUI test file")) + "\n";
  r += tl::to_string (QObject::tr ("  -gb <line number>   Replay GUI test file up to (including) line")) + "\n";
  r += tl::to_string (QObject::tr ("  -gx <millisec>      Replay rate for GUI test file")) + "\n";
  r += tl::to_string (QObject::tr ("  -gi                 Incremental logs for GUI test file")) + "\n";
  r += tl::to_string (QObject::tr ("  -i                  Disable undo buffering (less memory requirements)")) + "\n";
  r += tl::to_string (QObject::tr ("  -ni                 Enable undo buffering (default, overrides previous -i option)")) + "\n";
  r += tl::to_string (QObject::tr ("  -j <path>           Add the given path to the macro project paths")) + "\n"; 
  r += tl::to_string (QObject::tr ("  -k <log file>       Write log to the given file plus stdout/stderr")) + "\n";
  r += tl::to_string (QObject::tr ("  -l <lyp file>       Use layer properties file")) + "\n";
  r += tl::to_string (QObject::tr ("  -lx                 With -l: add other layers as well")) + "\n";
  r += tl::to_string (QObject::tr ("  -lf                 With -l: use the lyp file as it is (no expansion to multiple layouts)")) + "\n";
  r += tl::to_string (QObject::tr ("  -m <database file>  Load RDB (report database) file (into previous layout view)")) + "\n";
  r += tl::to_string (QObject::tr ("  -mn <database file> Load L2NDB (layout to netlist database) file (into previous layout view)")) + "\n";
  r += tl::to_string (QObject::tr ("  -n <technology>     Technology to use for next layout(s) on command line")) + "\n";
  r += tl::to_string (QObject::tr ("  -nn <tech file>     Technology file (.lyt) to use for next layout(s) on command line")) + "\n";
  r += tl::to_string (QObject::tr ("  -p <plugin>         Load the plugin (can be used multiple times)")) + "\n";
  r += tl::to_string (QObject::tr ("  -r <script>         Execute main script on startup (after having loaded files etc.)")) + "\n";
  r += tl::to_string (QObject::tr ("  -rr <script>        Like -r, but does not exit after executing the script")) + "\n";
  r += tl::to_string (QObject::tr ("  -rm <script>        Execute script on startup before loading files (can be used multiple times)")) + "\n";
  r += tl::to_string (QObject::tr ("  -rd <name>=<value>  Specify script variable")) + "\n";
  r += tl::to_string (QObject::tr ("  -rx                 Ignore all implicit macros (*.rbm, rbainit, *.lym)")) + "\n";
  r += tl::to_string (QObject::tr ("  -s                  Load files into same view")) + "\n";
  r += tl::to_string (QObject::tr ("  -t                  Don't update the configuration file on exit")) + "\n";
  r += tl::to_string (QObject::tr ("  -nt                 Update the configuration file on exit (default, overrides previous -t option)")) + "\n";
  r += tl::to_string (QObject::tr ("  -u <file name>      Restore session from given file")) + "\n";
  r += tl::to_string (QObject::tr ("  -v                  Print program version and exit")) + "\n";
  r += tl::to_string (QObject::tr ("  -wd <name>=<value>  Define a variable within expressions")) + "\n";
  r += tl::to_string (QObject::tr ("  -x                  Synchronous drawing mode")) + "\n";
  r += tl::to_string (QObject::tr ("  -y <package>        Package installation: install package(s) and exit - can be used more than once")) + "\n";
  r += tl::to_string (QObject::tr ("                      ('package' is a name, an URL and optionally a version in round brackets)")) + "\n";
  r += tl::to_string (QObject::tr ("  -yd                 With -y: include dependencies")) + "\n";
  r += tl::to_string (QObject::tr ("  -z                  Non-GUI mode (hidden views)")) + "\n";
  r += tl::to_string (QObject::tr ("  -zz                 Non-GUI mode (database only, implies -nc)")) + "\n";
  return r;
}

int
ApplicationBase::run ()
{
  lay::MainWindow *mw = main_window ();
  gtf::Player player (0);

  if (mw) {

    (mw)->set_synchronous (m_sync_mode);

    if (! m_no_gui) {
      (mw)->setWindowTitle (tl::to_qstring (version ()));
      (mw)->resize (800, 600);
      (mw)->show ();
    }

    if (! m_gtf_replay.empty ()) {
      player.load (m_gtf_replay);
    }

    start_recording ();

  }

  int result = 0;

  bool config_failed = false;

  for (std::vector <std::string>::const_iterator c = m_config_files.begin (); c != m_config_files.end (); ++c) {
    BEGIN_PROTECTED_CLEANUP
      dispatcher ()->read_config (*c);
      //  if the last config was read successfully no reset will happen:
      config_failed = false;
    END_PROTECTED_CLEANUP {
      config_failed = true;
    }
  }

  if (config_failed) {
    reset_config ();
  }

  for (std::vector< std::pair<std::string, std::string> >::const_iterator v = m_variables.begin (); v != m_variables.end (); ++v) {
    ruby_interpreter ().define_variable (v->first, v->second);
    python_interpreter ().define_variable (v->first, v->second);
    tl::log << "Variable definition: " << v->first << "='" << v->second << "'";
  }

  for (std::vector<std::string>::const_iterator m = m_load_macros.begin (); m != m_load_macros.end (); ++m) {

    BEGIN_PROTECTED 

      std::unique_ptr<lym::Macro> macro (new lym::Macro ());
      macro->load_from (*m);
      macro->set_file_path (*m);
      if (macro->show_in_menu ()) {
        //  menu-based macros are just registered so they are shown in the menu
        lay::MacroController *mc = lay::MacroController::instance ();
        if (mc) {
          tl::log << "Registering macro '" << *m << "'";
          mc->add_temp_macro (macro.release ());
        }
      } else {
        //  other macros given with -rm are run
        tl::log << "Run macro '" << *m << "'";
        macro->run ();
      }

    END_PROTECTED

  }

  //  Run plugin and macro specific initializations
  autorun ();

  //  Some objects we need during batch mode view generation
  db::Manager batch_mode_manager;
  tl::shared_ptr<LayoutView> batch_mode_view;

  if (mw) {

    for (std::vector <std::pair<file_type, std::pair<std::string, std::string> > >::const_iterator f = m_files.begin (); f != m_files.end (); ++f) {

      if (f->first == layout_file || f->first == layout_file_with_tech) {

        std::string filename = f->second.first;

        if (f->first != layout_file_with_tech) {
          mw->add_mru (f->second.first);
          mw->load_layout (f->second.first, m_same_view ? 2 /*same view*/ : 1 /*new view*/);
        } else {
          mw->add_mru (f->second.first, f->second.second);
          mw->load_layout (f->second.first, f->second.second, m_same_view ? 2 /*same view*/ : 1 /*new view*/);
        }

        //  Make the first one loaded the active one.
        if (mw->current_view ()) {
          mw->current_view ()->set_active_cellview_index (0);
        }

      } else if (f->first == rdb_file) {

        if (mw->current_view () == 0) {
          mw->create_view ();
        }

        if (mw->current_view () != 0) {
          std::unique_ptr <rdb::Database> db (new rdb::Database ());
          db->load (f->second.first);
          int rdb_index = mw->current_view ()->add_rdb (db.release ());
          mw->current_view ()->open_rdb_browser (rdb_index, mw->current_view ()->active_cellview_index ());
        }

      } else if (f->first == l2ndb_file) {

        if (mw->current_view () == 0) {
          mw->create_view ();
        }

        if (mw->current_view () != 0) {
          int l2ndb_index = mw->current_view ()->add_l2ndb (db::LayoutToNetlist::create_from_file (f->second.first));
          mw->current_view ()->open_l2ndb_browser (l2ndb_index, mw->current_view ()->active_cellview_index ());
        }

      }
    }

    if (! m_layer_props_file.empty ()) {

      mw->load_layer_properties (m_layer_props_file, true /*all views*/, m_lyp_add_default);

      tl::log << "Layer properties loaded '" << m_layer_props_file << "'";

      //  because the layer may carry transformations, we need to refit the cellviews.
      for (unsigned int v = 0; v != mw->views (); ++v) {
        mw->view (v)->zoom_fit ();
      }

    }

    if (! m_session_file.empty ()) {
      mw->restore_session (m_session_file);
      tl::log << "Session restored '" << m_session_file << "'";
    }

    if (! m_gtf_replay.empty ()) {
      player.replay (m_gtf_replay_rate, m_gtf_replay_stop);
    }

  } else {

    //  in batch mode create at least one

    for (std::vector <std::pair<file_type, std::pair<std::string, std::string> > >::const_iterator f = m_files.begin (); f != m_files.end (); ++f) {

      if (f->first == layout_file || f->first == layout_file_with_tech) {

        std::string filename = f->second.first;

        if (batch_mode_view.get () != 0 && ! m_same_view) {
          tl::warn << tl::sprintf (tl::to_string (tr ("Ignoring additional views in batch mode (file %s)")), filename);
          continue;
        }

        if (! batch_mode_view) {
          batch_mode_view.reset (create_view (batch_mode_manager));
        }

        if (f->first != layout_file_with_tech) {
          batch_mode_view->load_layout (f->second.first, true);
        } else {
          batch_mode_view->load_layout (f->second.first, f->second.second, true);
        }

        //  Make the first one loaded the active one.
        batch_mode_view->set_active_cellview_index (0);

      } else if (f->first == rdb_file) {

        if (! batch_mode_view) {
          batch_mode_view.reset (create_view (batch_mode_manager));
        }

        std::unique_ptr <rdb::Database> db (new rdb::Database ());
        db->load (f->second.first);
        batch_mode_view->add_rdb (db.release ());

      } else if (f->first == l2ndb_file) {

        if (! batch_mode_view) {
          batch_mode_view.reset (create_view (batch_mode_manager));
        }

        batch_mode_view->add_l2ndb (db::LayoutToNetlist::create_from_file (f->second.first));

      }
    }

    if (! m_layer_props_file.empty () && batch_mode_view.get ()) {

      batch_mode_view->load_layer_props (m_layer_props_file, m_lyp_add_default);

      tl::log << "Layer properties loaded '" << m_layer_props_file << "'";

      //  because the layer may carry transformations, we need to refit the cellviews.
      batch_mode_view->zoom_fit ();

    }

  }

  //  Give the plugins a change to do some last-minute initialisation and checks
  if (dispatcher ()) {
    for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
      lay::PluginDeclaration *pd = const_cast<lay::PluginDeclaration *> (&*cls);
      pd->initialized (dispatcher ());
    }
  }

  if (mw && ! m_no_gui && m_gtf_replay.empty () && m_gtf_record.empty ()) {
    //  Show initial tip window if required
    mw->about_to_exec ();
  }

  if (! m_run_macro.empty ()) {

    tl::log << "Run macro '" << m_run_macro << "'";
    lym::Macro macro;
    macro.load_from (m_run_macro);
    macro.set_file_path (m_run_macro);
    result = macro.run ();

    if (result == 0 && ! m_run_macro_and_exit) {
      result = exec ();
    }

  } else {
    result = exec ();
  }

  finish ();

  batch_mode_view.reset (0);

  return result;
}

lay::LayoutView *
ApplicationBase::create_view (db::Manager &manager)
{
  //  create a new view
  lay::LayoutView *view = new lay::LayoutView (&manager, lay::ApplicationBase::instance ()->is_editable (), dispatcher ());

  //  set initial attributes
  view->set_synchronous (m_sync_mode);

  int tl = 0;
  dispatcher ()->config_get (cfg_initial_hier_depth, tl);
  view->set_hier_levels (std::make_pair (0, tl));

  view->set_current ();

  return view;
}

void
ApplicationBase::autorun ()
{
  //  call "autorun" on all plugins that wish so
  for (std::list <lay::PluginDescriptor>::const_iterator p = lay::plugins ().begin (); p != lay::plugins ().end (); ++p) {
    if (p->autorun) {
      (*p->autorun) ();
    }
  }

  //  run all autorun macros
  lym::MacroCollection::root ().autorun ();
}

void
ApplicationBase::set_editable (bool e)
{
  if (m_editable != e) {
    m_editable = e;
    db::set_default_editable_mode (m_editable);
  }
}

static void 
dump_children (QObject *obj, int level = 0)
{
  QObjectList children = obj->children ();
  if (! children.isEmpty () || ! obj->objectName ().isEmpty ()) {
    std::string info;
    for (int i = 0; i < level; ++i) {
      info += "  ";
    }
    if (obj->objectName ().isEmpty ()) {
      info += "<unnamed>";
    } else {
      info += tl::to_string (obj->objectName ());
    }
    tl::info << info;
    for (QObjectList::const_iterator child = children.begin (); child != children.end (); ++child) {
      dump_children (*child, level + 1);
    }
  }
}

void
ApplicationBase::process_events_impl (QEventLoop::ProcessEventsFlags /*flags*/, bool /*silent*/)
{
  //  in the non-UI case there are no events, but we can at least schedule deferred method calls.
  tl::DeferredMethodScheduler::execute ();
}

bool 
ApplicationBase::write_config (const std::string &config_file)
{
  return dispatcher () ? dispatcher ()->write_config (config_file) : 0;
}

void 
ApplicationBase::reset_config ()
{
  clear_config ();
  for (std::vector <std::string>::const_iterator c = m_initial_config_files.begin (); c != m_initial_config_files.end (); ++c) {
    try {
      read_config (*c);
    } catch (...) { }
  }
}

void 
ApplicationBase::clear_config ()
{
  if (dispatcher ()) {
    dispatcher ()->clear_config ();
  }
}

bool 
ApplicationBase::read_config (const std::string &config_file)
{
  return dispatcher () ? dispatcher ()->read_config (config_file) : true;
}

void 
ApplicationBase::set_config (const std::string &name, const std::string &value)
{
  if (dispatcher ()) {
    dispatcher ()->config_set (name, value);
  }
}

void 
ApplicationBase::config_end ()
{
  if (dispatcher ()) {
    dispatcher ()->config_end ();
  }
}

std::string 
ApplicationBase::get_config (const std::string &name) const
{
  if (dispatcher ()) {
    return dispatcher ()->config_get (name);
  } else {
    return std::string ();
  }
}

std::vector<std::string> 
ApplicationBase::get_config_names () const
{
  std::vector<std::string> names;
  if (dispatcher ()) {
    dispatcher ()->get_config_names (names);
  }
  return names;
}

// --------------------------------------------------------------------------------
//  GuiApplication implementation

GuiApplication::GuiApplication (int &argc, char **argv)
  : QApplication (argc, argv), ApplicationBase (false),
    mp_mw (0),
    mp_recorder (0),
    m_in_notify (0)
{
  //  install a special style proxy to overcome the issue of black-on-black tree expanders
  setStyle (new lay::BackgroundAwareTreeStyle (0));

  setWindowIcon (QIcon (QString::fromUtf8 (":/logo.png")));
}

GuiApplication::~GuiApplication ()
{
  //  uninitialize the plugins
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    lay::PluginDeclaration *pd = const_cast<lay::PluginDeclaration *> (&*cls);
    pd->uninitialize (dispatcher ());
  }

  shutdown ();
}

void
GuiApplication::initialize ()
{
#if QT_VERSION >= 0x040500
  QCoreApplication::setAttribute (Qt::AA_DontShowIconsInMenus, false);
#endif
#if QT_VERSION >= 0x050000
  QCoreApplication::setAttribute (Qt::AA_UseHighDpiPixmaps, true);
#endif
#if QT_VERSION >= 0x050600
  QCoreApplication::setAttribute (Qt::AA_EnableHighDpiScaling, true);
#endif
}

bool
GuiApplication::notify (QObject *receiver, QEvent *e)
{
  QWheelEvent *wheel_event = dynamic_cast<QWheelEvent *>(e);
  if (wheel_event) {
    //  intercept wheel events targeting QComboBox objects to avoid
    //  changing them through wheel actions.
    for (auto r = receiver; r != 0; r = r->parent ()) {
      if (dynamic_cast<QComboBox *>(r)) {
        //  stop further processing
        return true;
      }
    }
  }

  if (dynamic_cast<QPaintEvent *> (e)) {
    //  NOTE: we don't want recursive paint events - the painters are not reentrant.
    //  Hence we disable process_events_impl (specifically for progress reporters).
    lay::BusySection busy;
    return do_notify (receiver, e);
  } else {
    return do_notify (receiver, e);
  }
}

bool
GuiApplication::do_notify (QObject *receiver, QEvent *e)
{
  bool in_notify = (m_in_notify > 0);

  bool ret = true;
  ++m_in_notify;

  //  Note: due to a bug in some Qt versions (i.e. 4.8.3) throwing exceptions across
  //  signals may not be safe. Hence the local BEGIN_PROTECTED .. END_PROTECTED approach
  //  is still preferred over the global solution through "notify".
  //  Because END_PROTECTED may raise other events (message box) and this may cause other
  //  exceptions, we use silent mode inside notify to avoid recursion.

  if (in_notify) {
    BEGIN_PROTECTED_SILENT
      ret = QApplication::notify (receiver, e);
    END_PROTECTED_SILENT
  } else {
    BEGIN_PROTECTED
      ret = QApplication::notify (receiver, e);
    END_PROTECTED
  }

  --m_in_notify;
  return ret;
}

void
GuiApplication::enter_busy_mode (bool bm)
{
  if (mp_mw) {
    mp_mw->enter_busy_mode (bm);
  }
}

bool
GuiApplication::is_busy () const
{
  return mp_mw && mp_mw->is_busy ();
}

void
GuiApplication::force_update_app_menu ()
{
#if defined(__APPLE__)
  //  This is a workaround for a bug in the MacOS native menu integration:
  //  this signal forces the menu to become updated. Without this, any
  //  new menu items stay disabled.
  emit focusWindowChanged (focusWindow ());
#endif
}

bool
GuiApplication::event (QEvent *event)
{
#if defined(__APPLE__)
  // By Thomas Lima (March 7, 2018)
  //
  // This event interceptor catches MacOS "Open With" event, and KLayout should respond
  // similarly to the Drop event in MainWindow::dropEvent.
  //
  // This particular implementation always creates a new window.
  //
  // This was implemented with the inspiration of http://doc.qt.io/qt-5/qfileopenevent.html
  if (event->type() == QEvent::FileOpen) {
      QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
      if (mp_mw)
      {
        const std::string tech = mp_mw->initial_technology();
        const std::string file = tl::to_string (openEvent->file());
        const int mode = 1; // open in new window
        mp_mw->load_layout (file, tech, mode);
        mp_mw->add_mru (file, tech);
      }
  }
#endif

  return QApplication::event(event);
}


int
GuiApplication::exec ()
{
  //  if requested, dump the widgets
  if (tl::verbosity () >= 40) {

    QWidgetList tl_widgets = QApplication::topLevelWidgets ();

    tl::info << tl::to_string (QObject::tr ("Widget tree:"));
    for (QWidgetList::const_iterator tl = tl_widgets.begin (); tl != tl_widgets.end (); ++tl) {
      if (! (*tl)->objectName ().isEmpty ()) {
        dump_children (*tl);
      }
    }
    tl::info << "";

    tl::info << tl::to_string (QObject::tr ("Actions list:"));
    for (QWidgetList::const_iterator tl = tl_widgets.begin (); tl != tl_widgets.end (); ++tl) {
      if (! (*tl)->objectName ().isEmpty ()) {
        QList<QAction *> actions = (*tl)->findChildren<QAction *> ();
        if (! actions.isEmpty ()) {
          tl::info << tl::to_string ((*tl)->objectName ()) << ":";
          for (QList<QAction *>::const_iterator a = actions.begin (); a != actions.end (); ++a) {
            if (! (*a)->objectName ().isEmpty ()) {
              tl::info << "  " << tl::to_string ((*a)->objectName ());
            }
          }
        }
      }
    }
    tl::info << "";

  }

  return QApplication::exec ();
}

void
GuiApplication::shutdown ()
{
  //  avoid deferred execution later on where there isn't a valid main window anymore
  //  (problem case: showing a dialog inside main windows's destroyed signal - this will
  //  process events and trigger execution if not disabled)
  if (! tl::DeferredMethodScheduler::instance ()->is_disabled ()) {
    tl::DeferredMethodScheduler::instance ()->execute ();
  }
  tl::DeferredMethodScheduler::instance ()->enable (false);

  if (mp_mw) {
    delete mp_mw;
    mp_mw = 0;
  }

  //  detach all top level widgets from Ruby/Python - we don't want the interpreter do this
  //  for us. Qt will delete all top level widgets itself.
  //  NOTE: we must only detach (= "keep" on C++ side), not delete them as top level widgets
  //  may be owned by someone else.
  QWidgetList tl_widgets = topLevelWidgets ();
  for (QWidgetList::iterator w = tl_widgets.begin (); w != tl_widgets.end (); ++w) {
    gsi::ObjectBase *gsi_obj = dynamic_cast<gsi::ObjectBase *> (*w);
    if (gsi_obj) {
      gsi_obj->keep ();
    }
  }

  while (! (tl_widgets = topLevelWidgets ()).empty ()) {
    delete tl_widgets [0];
  }

  if (mp_recorder) {
    delete mp_recorder;
    mp_recorder = 0;
  }

  ApplicationBase::shutdown ();
}

void
GuiApplication::finish ()
{
  //  save the recorded test events
  if (mp_recorder && mp_recorder->recording ()) {
    mp_recorder->stop ();
    mp_recorder->save ();
  }

  ApplicationBase::finish ();
}

void
GuiApplication::prepare_recording (const std::string &gtf_record, bool gtf_save_incremental)
{
  tl_assert (mp_recorder == 0);

  //  since the recorder tracks QAction connections etc., it must be instantiated before every other
  //  object performing a gtf::action_connect for example
  mp_recorder = new gtf::Recorder (this, gtf_record);
  mp_recorder->save_incremental (gtf_save_incremental);
}

void
GuiApplication::start_recording ()
{
  if (mp_recorder) {
    mp_recorder->start ();
  }
}

lay::Dispatcher *
GuiApplication::dispatcher () const
{
  return mp_mw ? mp_mw->dispatcher () : 0;
}

void
GuiApplication::setup ()
{
  tl_assert (mp_mw == 0);

  mp_mw = new lay::MainWindow (this, "main_window", is_undo_enabled ());

  QObject::connect (mp_mw, SIGNAL (closed ()), this, SLOT (quit ()));

  //  create a password dialog for use with the HTTP streams
  lay::PasswordDialog *pw_dialog = new lay::PasswordDialog (mp_mw);
  tl::InputHttpStream::set_credential_provider (pw_dialog);
}

void
GuiApplication::process_events_impl (QEventLoop::ProcessEventsFlags flags, bool silent)
{
  //  prevent recursive process_events
  if (is_busy ()) {
    return;
  }

  if (mp_mw) {

    lay::BusySection busy;

    if (silent) {
      tl::DeferredMethodScheduler::enable (false);
    }

    try {
#if QT_VERSION < 0x050000
      QApplication::syncX ();
#endif
      QApplication::processEvents (flags);
      //  Qt seems not to send posted UserEvents in some cases (e.g. in the unit test application with GLib?
      //  Glib not doing this without a main window visible?). Hence we do this explicitly here.
      QApplication::sendPostedEvents ();
    } catch (...) {
      //  ignore exceptions
    }

    if (silent) {
      tl::DeferredMethodScheduler::enable (true);
    }

  }
}

// --------------------------------------------------------------------------------
//  NonGuiApplication implementation

NonGuiApplication::NonGuiApplication (int &argc, char **argv)
  : QCoreApplication (argc, argv), ApplicationBase (true),
    mp_pr (0),
    mp_pb (0),
    mp_dispatcher (0)
{
  //  .. nothing yet ..
}

NonGuiApplication::~NonGuiApplication ()
{
  //  uninitialize the plugins
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    lay::PluginDeclaration *pd = const_cast<lay::PluginDeclaration *> (&*cls);
    pd->uninitialize (dispatcher ());
  }

  shutdown ();
}

int
NonGuiApplication::exec ()
{
  //  A non-GUI application does nothing on exec
  return 0;
}

void
NonGuiApplication::shutdown ()
{
  if (mp_dispatcher) {
    delete mp_dispatcher;
    mp_dispatcher = 0;
  }

  if (mp_pr) {
    delete mp_pr;
    mp_pr = 0;
  }

  if (mp_pb) {
    delete mp_pb;
    mp_pb = 0;
  }

  ApplicationBase::shutdown ();
}

void
NonGuiApplication::setup ()
{
  mp_pr = new lay::ProgressReporter ();
  mp_pb = new TextProgress (10 /*verbosity level*/);
  mp_pr->set_progress_bar (mp_pb);
  mp_dispatcher = new lay::Dispatcher ();
}

}

