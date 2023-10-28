

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


#include "utTestConsole.h"

#include "tlUnitTest.h"
#include "tlStaticObjects.h"
#include "tlTimer.h"
#include "tlCommandLineParser.h"
#include "tlFileUtils.h"
#include "tlGlobPattern.h"
#include "lymMacroCollection.h"
#include "rba.h"
#include "pya.h"
#include "gsiDecl.h"
#include "gsiExternalMain.h"
#include "dbStatic.h"
#include "dbInit.h"

//  This hard-links the GSI test classes
#include "../gsi_test/gsiTestForceLink.h"

#include "version.h"

#if defined(HAVE_QT)

//  For testing the document structure
#  include "docForceLink.h"
#  include "iconsForceLink.h"

#  include "layApplication.h"
#  include "layMainWindow.h"
#  include "laySystemPaths.h"
#  include "layVersion.h"

#  include <QDir>
#  include <QFileInfo>
#  include <QTextCodec>

#endif

#if !defined(_WIN32)
#  include <dlfcn.h>
#endif
#if defined(_WIN32)
#  include <Windows.h>
#endif

//  required to force linking of the "rdb", "lib" and "drc" module 
//  and the plugins/auxiliary modules (some in non-Qt case)
#include "libForceLink.h"
#include "rdbForceLink.h"
#include "antForceLink.h"
#include "imgForceLink.h"
#include "edtForceLink.h"
#include "lymForceLink.h"
#if defined(HAVE_RUBY)
#  include "drcForceLink.h"
#  include "lvsForceLink.h"
#endif

static int main_cont (int &argc, char **argv);

#ifdef _WIN32 // for VC++

//  for VC++/MinGW provide a wrapper for main.
#include <Windows.h>

extern "C"
int WINAPI 
WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*prevInstance*/, LPSTR /*lpCmdLine*/, int /*nShowCmd*/)
{
  int argCount = 0;
  LPWSTR *szArgList = CommandLineToArgvW(GetCommandLineW(), &argCount);

  //  fail safe behaviour
  if (!szArgList) {
    MessageBox(NULL, L"Unable to parse command line", L"Error", MB_OK);
    return 10;
  }

  char **argv = new char *[argCount];
  for (int i = 0; i < argCount; i++) {
    std::wstring a;
    for (WCHAR *wc = szArgList [i]; *wc; ++wc) {
      a += wchar_t ((unsigned int) *wc);
    }
    std::string aa = tl::to_string (a);
    argv [i] = new char [aa.size () + 1];
    strcpy (argv [i], aa.c_str ());
  }

  int ret = rba::RubyInterpreter::initialize (argCount, argv, &main_cont);

  //  NOTE: this needs to happen after the Ruby interpreter went down since otherwise the GC will
  //  access objects that are already cleaned up.
  tl::StaticObjects::cleanup ();

  for (int i = 0; i < argCount; i++) {
    delete[] argv [i];
  }
  delete[] argv;

  LocalFree(szArgList);
  return ret;
}

#else

int
main(int a_argc, const char **a_argv)
{
  char **argv = new char *[a_argc];
  for (int i = 0; i < a_argc; i++) {
    tl::string aa = tl::system_to_string (a_argv[i]);
    argv [i] = new char [aa.size () + 1];
    strcpy (argv [i], aa.c_str ());
  }

  int ret = rba::RubyInterpreter::initialize (a_argc, argv, &main_cont);

  //  NOTE: this needs to happen after the Ruby interpreter went down since otherwise the GC will
  //  access objects that are already cleaned up.
  tl::StaticObjects::cleanup ();

  for (int i = 0; i < a_argc; i++) {
    delete[] argv [i];
  }
  delete[] argv;

  return ret;
}

#endif

static bool
run_test (tl::TestBase *t, bool editable, bool slow, int repeat)
{
#if defined(HAVE_QT)
  //  provide a clean main window without any views attached
  if (lay::MainWindow::instance ()) {
    lay::MainWindow::instance ()->close_all ();
  }
#endif

  for (int i = 0; i < repeat; ++i) {
    if (repeat > 1) {
      ut::noctrl << "Repeat iteration " << i + 1 << " of " << repeat;
    }
    if (! t->do_test (editable, slow)) {
      return false;
    }
  }
  return true;
}

static int
run_tests (const std::vector<tl::TestBase *> &selected_tests, bool editable, bool non_editable, bool slow, int repeat, bool gsi_coverage, const std::vector<std::string> &class_names_vector)
{
  std::set<std::string> class_names;
  class_names.insert (class_names_vector.begin (), class_names_vector.end ());

  tl::Timer grand_timer;
  grand_timer.start ();

  int failed_ne = 0, failed_e = 0;
  std::vector <tl::TestBase *> failed_tests_e, failed_tests_ne;
  int skipped_ne = 0, skipped_e = 0;
  std::vector <tl::TestBase *> skipped_tests_e, skipped_tests_ne;

  for (int e = 0; e < 2; ++e) {

    if ((non_editable && e == 0) || (editable && e == 1)) {

      std::string mode (e == 0 ? "non-editable" : "editable");
      ut::ctrl << "<testsuite name=\"ut-runner-" << mode << "\">";

      ut::noctrl << tl::replicate ("=", ut::TestConsole::instance ()->real_columns ());
      ut::noctrl << "Running tests in " << mode << " mode ...";

      db::set_default_editable_mode (e != 0);
#if defined(HAVE_QT)
      lay::ApplicationBase::instance ()->set_editable (e != 0);
#endif

      int failed = 0;
      std::vector <tl::TestBase *> failed_tests;
      int skipped = 0;
      std::vector <tl::TestBase *> skipped_tests;

      tl::Timer timer;

      timer.start ();

      try {

        failed = 0;
        failed_tests.clear ();
        skipped = 0;
        skipped_tests.clear ();

        for (std::vector <tl::TestBase *>::const_iterator t = selected_tests.begin (); t != selected_tests.end (); ++t) {
          (*t)->remove_tmp_folder ();
        }

        for (std::vector <tl::TestBase *>::const_iterator t = selected_tests.begin (); t != selected_tests.end (); ++t) {

          ut::ctrl << "<testcase name=\"" << (*t)->name () << "\">";

          ut::noctrl << tl::replicate ("-", ut::TestConsole::instance ()->real_columns ());
          ut::noctrl << "Running " << (*t)->name ();

          try {

            ut::ctrl << "<system-out>";

            tl::Timer timer;
            timer.start();

            if (! run_test (*t, e != 0, slow, repeat)) {

              ut::ctrl << "</system-out>";

              ut::ctrl << "<error message=\"" << "Test " << tl::escaped_to_html ((*t)->name (), false) << " failed (continued mode - see previous messages)" << "\"/>";
              tl::error << "Test " << (*t)->name () << " failed (continued mode - see previous messages)";

              failed_tests.push_back (*t);
              ++failed;

            } else {
              ut::ctrl << "</system-out>";
            }

            timer.stop();

            ut::noctrl << "Time: " << timer.sec_wall () << "s (wall) " << timer.sec_user () << "s (user) " << timer.sec_sys () << "s (sys)";
            ut::noctrl << "Memory: " << timer.memory_size () / 1024 << "k";
            ut::ctrl << "<x-testcase-times wall=\"" << timer.sec_wall () << "\" user=\"" << timer.sec_user () << "\" sys=\"" << timer.sec_sys () << "\" memory=\"" << timer.memory_size () << "\"/>";

          } catch (tl::CancelException &) {

            ut::ctrl << "</system-out>";
            ut::ctrl << "<skipped/>";
            tl::error << "Test " << (*t)->name () << " skipped";

            skipped_tests.push_back (*t);
            ++skipped;

          } catch (tl::Exception &ex) {

            ut::ctrl << "</system-out>";
            ut::ctrl << "<failure message=\"" << tl::escaped_to_html (ex.msg (), false) << "\"/>";
            tl::error << "Test " << (*t)->name () << " failed:";
            tl::info << ex.msg ();

            failed_tests.push_back (*t);
            ++failed;

          }

          ut::ctrl << "</testcase>";

        }

      } catch (tl::Exception &ex) {
        tl::error << "Caught tl::exception: " << ex.msg ();
        failed = 1;
      } catch (std::exception &ex) {
        tl::error << "Caught std::exception: " << std::string (ex.what ());
        failed = 1;
      } catch (...) {
        tl::error << "Caught unspecific exception";
        failed = 1;
      }

      timer.stop ();

      ut::ctrl << "<x-summary mode=\"" << mode << "\">";

      ut::noctrl << tl::replicate ("=", ut::TestConsole::instance ()->real_columns ());
      ut::noctrl << "Summary";

      if (skipped > 0) {
        if (e == 0) {
          skipped_tests_ne = skipped_tests;
          skipped_ne = skipped;
        } else {
          skipped_tests_e = skipped_tests;
          skipped_e = skipped;
        }
        tl::warn << skipped << " test(s) skipped";
      }

      if (failed > 0) {
        if (e == 0) {
          failed_tests_ne = failed_tests;
          failed_ne = failed;
        } else {
          failed_tests_e = failed_tests;
          failed_e = failed;
        }
        tl::warn << failed << " test(s) failed";
      } else {
        tl::info << "All tests passed in " << mode << " mode.";
      }

      ut::ctrl << "</x-summary>";

      ut::noctrl << "Total time: " << timer.sec_wall () << "s (wall) " << timer.sec_user () << "s (user) " << timer.sec_sys () << "s (sys)";
      ut::ctrl << "<x-summary-times mode=\"" << mode << "\" wall=\"" << timer.sec_wall () << "\" user=\"" << timer.sec_user () << "\" sys=\"" << timer.sec_sys () << "\"/>";

      ut::ctrl << "</testsuite>";

    }

  }

  grand_timer.stop ();

  //  GSI diagnostics: print all methods that have not been called
  if (gsi_coverage) {

    ut::noctrl << tl::replicate ("=", ut::TestConsole::instance ()->real_columns ());
    ut::noctrl << "GSI coverage test";

    ut::ctrl << "<x-gsi-coverage>";

    bool first = true;
    for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {

      if (gsi_coverage && !class_names.empty () && class_names.find (c->name ()) == class_names.end ()) {
        continue;
      }

      bool first_of_class = true;
      for (gsi::ClassBase::method_iterator m = c->begin_methods (); m != c->end_methods (); ++m) {

        if (!dynamic_cast<const gsi::SpecialMethod *> (*m) && !(*m)->was_called ()) {

          if (first) {
            first = false;
            tl::warn << "GSI coverage test failed - the following methods were not called:";
          }
          if (first_of_class) {
            tl::warn << tl::replicate (" ", tl::indent ()) << "Class " << c->name ();
            first_of_class = false;
          }
          tl::warn << tl::replicate (" ", tl::indent () * 2) << (*m)->to_string ();

        }

      }

    }

    if (first) {
      tl::info << "GSI coverage test passed.";
    }

    ut::ctrl << "</x-gsi-coverage>";

  }

  ut::noctrl << tl::replicate ("=", ut::TestConsole::instance ()->real_columns ());
  ut::noctrl << "Grand Summary";

  ut::ctrl << "<x-grand-summary>";

  if (skipped_e + skipped_ne > 0) {
    if (non_editable) {
      tl::warn << "Skipped in non-editable mode";
      for (std::vector <tl::TestBase *>::const_iterator f = skipped_tests_ne.begin (); f != skipped_tests_ne.end (); ++f) {
        tl::warn << tl::replicate (" ", tl::indent ()) << (*f)->name ();
      }
    }
    if (editable) {
      tl::warn << "Skipped in editable mode";
      for (std::vector <tl::TestBase *>::const_iterator f = skipped_tests_e.begin (); f != skipped_tests_e.end (); ++f) {
        tl::warn << tl::replicate (" ", tl::indent ()) << (*f)->name ();
      }
    }
    tl::warn << tl::to_string (skipped_e + skipped_ne) << " test(s) skipped";
  }

  int result = failed_e + failed_ne;
  if (result > 0) {
    if (non_editable) {
      tl::warn << "Failed in non-editable mode";
      for (std::vector <tl::TestBase *>::const_iterator f = failed_tests_ne.begin (); f != failed_tests_ne.end (); ++f) {
        tl::warn << tl::replicate (" ", tl::indent ()) << (*f)->name ();
      }
    }
    if (editable) {
      tl::warn << "Failed in editable mode";
      for (std::vector <tl::TestBase *>::const_iterator f = failed_tests_e.begin (); f != failed_tests_e.end (); ++f) {
        tl::warn << tl::replicate (" ", tl::indent ()) << (*f)->name ();
      }
    }
    tl::warn << tl::to_string (result) << " test(s) failed";
  } else {
    tl::info << "All tests passed.";
  }

  ut::ctrl << "</x-grand-summary>";

  ut::noctrl << "Grand total time: " << grand_timer.sec_wall () << "s (wall) " << grand_timer.sec_user () << "s (user) " << grand_timer.sec_sys () << "s (sys)";
  ut::ctrl << "<x-grand-summary-times wall=\"" << grand_timer.sec_wall () << "\" user=\"" << grand_timer.sec_user () << "\" sys=\"" << grand_timer.sec_sys () << "\"/>";

  return result;
}

static int
main_cont (int &argc, char **argv)
{
  ut::TestConsole console (stdout);

  std::unique_ptr<rba::RubyInterpreter> ruby_interpreter;
  std::unique_ptr<pya::PythonInterpreter> python_interpreter;

#if defined(HAVE_QT)

  //  install the version strings
  lay::Version::set_exe_name (prg_exe_name);
  lay::Version::set_name (prg_name);
  lay::Version::set_version (prg_version);

  std::string subversion (prg_date);
  subversion += " r";
  subversion += prg_rev;
  lay::Version::set_subversion (subversion.c_str ());

#endif

  int result = 0;

  try {

    pya::PythonInterpreter::initialize ();
    gsi::initialize_external ();

    //  Search and initialize plugin unit tests

    std::string inst_dir = tl::get_inst_path ();
    std::vector<std::string> inst_modules = tl::dir_entries (inst_dir, true, false);
    std::sort (inst_modules.begin (), inst_modules.end ());

    for (std::vector<std::string>::const_iterator im = inst_modules.begin (); im != inst_modules.end (); ++im) {

      if (tl::extension_last (*im) != "ut") {
        continue;
      }

      std::string ut_file = tl::absolute_file_path (tl::combine_path (inst_dir, *im));
      if (tl::file_exists (ut_file)) {

        tl::log << "Loading unit tests " << ut_file;

        //  NOTE: since we are using a different suffix ("*.ut"), we can't use QLibrary.
#ifdef _WIN32
        //  there is no "dlopen" on mingw, so we need to emulate it.
        HINSTANCE handle = LoadLibraryW (tl::to_wstring (ut_file).c_str ());
        if (! handle) {
          throw tl::Exception (tl::sprintf ("Unable to load plugin tests: %s with error message: %s", ut_file.c_str (), GetLastError ()));
        }
#else
        void *handle;
        handle = dlopen (tl::string_to_system (ut_file).c_str (), RTLD_LAZY);
        if (! handle) {
          throw tl::Exception (tl::sprintf ("Unable to load plugin tests: %s", ut_file.c_str ()));
        }
#endif

      }

    }

    if (! tl::TestRegistrar::instance()) {
      throw tl::Exception ("No test libraries found - make sure, the *.ut files are next to the ut_runner executable.");
    }

#if defined(HAVE_QT)

    //  NOTE: we need an application object, but we don't call parse_cmd. This makes the object
    //  behave neutral as far as possible.
    lay::GuiApplication app (argc, argv);
    app.init_app ();

    app.ruby_interpreter ().push_console (&console);
    app.python_interpreter ().push_console (&console);

    app.autorun ();

#if QT_VERSION < 0x050000
    QTextCodec::setCodecForTr (QTextCodec::codecForName ("utf8"));
#endif

#else

    //  select the system locale
    setlocale (LC_ALL, "");

    //  initialize the modules (load their plugins from the paths)
    db::init ();

    //  initialize the GSI class system (Variant binding, Expression support)
    //  We have to do this now since plugins may register GSI classes and before the
    //  ruby interpreter, because it depends on a proper class system.
    gsi::initialize ();

    //  initialize the tl::Expression subsystem with GSI-bound classes
    gsi::initialize_expressions ();

    //  instantiate the interpreters

    ruby_interpreter.reset (new rba::RubyInterpreter ());
    ruby_interpreter->push_console (&console);

    python_interpreter.reset (new pya::PythonInterpreter ());
    python_interpreter->push_console (&console);

    lym::MacroCollection &lym_root = lym::MacroCollection::root ();
    lym_root.add_folder (tl::to_string (tr ("Built-In")), ":/built-in-macros", "macros", true);
    lym_root.add_folder (tl::to_string (tr ("Built-In")), ":/built-in-pymacros", "pymacros", true);

    lym_root.autorun_early ();
    lym_root.autorun ();

#endif

    bool editable = false, non_editable = false;
    bool gsi_coverage = false;
    std::vector<std::string> class_names;
    std::vector<std::string> test_list;
    std::vector<std::string> exclude_test_list;

    bool xml_format = false;
    bool list_tests = false;
    bool slow = false;
    bool verbose = false;
    bool debug_mode = false;
    bool continue_flag = false;
    int repeat = 1;
    std::string output;

    tl::CommandLineOptions cmd;
    cmd << tl::arg ("-a", &xml_format, "Provide XML output format (JUnit format)")
        << tl::arg ("-o=log", &output, "Sends output to the given file")
        << tl::arg ("-l", &list_tests, "Lists tests and exits")
        << tl::arg ("-e", &editable, "Uses editable mode")
        << tl::arg ("-ne", &non_editable, "Uses non-editable mode")
        << tl::arg ("-c", &continue_flag, "Continues after an error")
        << tl::arg ("-i", &debug_mode, "Uses debug mode",
                    "In debug mode, execution stops after an error and if possible, fix instructions are "
                    "printed."
                   )
        << tl::arg ("-s", &slow, "Includes slow (long runner) tests")
        << tl::arg ("-v", &verbose, "Provides verbose output")
        << tl::arg ("-g", &gsi_coverage, "Produces a GSI test coverage statistics")
        << tl::arg ("-r=n", &repeat, "Repeat the tests n times each")
        << tl::arg ("*-gg=class", &class_names, "Produces a specific GDS coverage statistics",
                    "With this specification, coverage will be printed for this specific class. "
                    "This option can be used multiple times to add more classes."
                   )
        << tl::arg ("-x=test", &exclude_test_list, "Exclude the following tests",
                    "This option can be given multiple times or with a comma-separated list "
                    "of pattern. Test tests matching one of the exclude pattern "
                    "are not executed."
                   )
        << tl::arg ("?*test", &test_list, "The pattern for the tests to execute")
      ;

    cmd.brief ("The runner executable for execution of the unit tests");

    cmd.parse (argc, argv);

    if (!editable && !non_editable) {
      editable = non_editable = true;
    }

    if (!class_names.empty ()) {
      gsi_coverage = true;
    }

    if (list_tests) {
      tl::info << "List of installed tests:";
      for (std::vector<tl::TestBase *>::const_iterator i = tl::TestRegistrar::instance()->tests ().begin (); i != tl::TestRegistrar::instance()->tests ().end (); ++i) {
        tl::info << "  " << (*i)->name ();
      }
      throw tl::CancelException ();
    }

    tl::set_verbose (verbose);
    tl::set_xml_format (xml_format);
    tl::set_continue_flag (continue_flag);
    tl::set_debug_mode (debug_mode);

    //  set some global variables
    if (rba::RubyInterpreter::instance ()) {
      rba::RubyInterpreter::instance ()->define_variable ("ut_inst_path", tl::get_inst_path ());
    }
    if (pya::PythonInterpreter::instance ()) {
      pya::PythonInterpreter::instance ()->define_variable ("ut_inst_path", tl::get_inst_path ());
    }

    FILE *output_file = 0;

    try {

      if (! output.empty ()) {
        output_file = fopen (output.c_str (), "w");
        if (! output_file) {
          throw tl::Exception (std::string ("Unable to open log file for writing :") + output);
        }
        console.send_to (output_file);
      }

      ut::ctrl << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
      ut::ctrl << "<testsuites>";

      ut::noctrl << tl::replicate ("=", console.real_columns ());
      ut::noctrl << "Entering KLayout test suite";

      ut::noctrl << "TESTSRC=" << tl::testsrc ();
      ut::noctrl << "TESTTMP=" << tl::absolute_file_path (tl::testtmp ());

      std::vector<tl::TestBase *> subset;

      ut::noctrl << "Selected tests:";

      for (std::vector<tl::TestBase *>::const_iterator i = tl::TestRegistrar::instance()->tests ().begin (); i != tl::TestRegistrar::instance()->tests ().end (); ++i) {

        bool exclude = false;

        for (std::vector<std::string>::const_iterator m = exclude_test_list.begin (); m != exclude_test_list.end () && !exclude; ++m) {
          tl::GlobPattern re (*m);
          re.set_case_sensitive (false);
          re.set_header_match (true);
          if (re.match ((*i)->name ())) {
            exclude = true;
          }
        }

        if (test_list.empty ()) {

          if (!exclude) {
            subset.push_back (*i);
            ut::noctrl << "  " << (*i)->name ();
          }

        } else {

          for (std::vector<std::string>::const_iterator m = test_list.begin (); !exclude && m != test_list.end (); ++m) {
            tl::GlobPattern re (*m);
            re.set_case_sensitive (false);
            re.set_header_match (true);
            if (re.match ((*i)->name ())) {
              ut::noctrl << "  " << (*i)->name ();
              subset.push_back (*i);
              break;
            }
          }

        }

      }

      result = run_tests (subset, editable, non_editable, slow, repeat, gsi_coverage, class_names);

      ut::ctrl << "</testsuites>";

      if (output_file) {
        console.send_to (stdout);
        fclose (output_file);
      }

    } catch (...) {

      ut::ctrl << "</testsuites>";

      if (output_file) {
        console.send_to (stdout);
        fclose (output_file);
      }

      throw;
    }

  } catch (tl::CancelException &) {
    result = 0;
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    result = -1;
  } catch (std::exception &ex) {
    tl::error << ex.what ();
    result = -1;
  } catch (...) {
    tl::error << "Unspecific exception";
    result = -1;
  }

  return result;
}
