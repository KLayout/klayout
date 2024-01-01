
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


#include "layApplication.h"
#include "layFileDialog.h"
#include "layVersion.h"
#include "laySignalHandler.h"
#include "tlExceptions.h"
#include "tlInternational.h"
#include "tlException.h"
#include "tlLog.h"
#include "tlStaticObjects.h"
#include "rba.h"
#include "pya.h"
#include "gsiExternalMain.h"

#include "tlArch.h"

#include "version.h"

//  required to force linking of the "ext" and "lib" module
#include "libForceLink.h"
#include "antForceLink.h"
#include "imgForceLink.h"
#include "docForceLink.h"
#include "iconsForceLink.h"
#if defined(HAVE_RUBY)
#include "drcForceLink.h"
#include "lvsForceLink.h"
#endif

#if defined(HAVE_QTBINDINGS)

//  pulls in the Qt GSI binding modules
# include "gsiQtGuiExternals.h"
# include "gsiQtWidgetsExternals.h"
# include "gsiQtCoreExternals.h"
# include "gsiQtMultimediaExternals.h"
# include "gsiQtPrintSupportExternals.h"
# include "gsiQtXmlExternals.h"
# include "gsiQtXmlPatternsExternals.h"
# include "gsiQtSqlExternals.h"
# include "gsiQtSvgExternals.h"
# include "gsiQtNetworkExternals.h"
# include "gsiQtDesignerExternals.h"
# include "gsiQtUiToolsExternals.h"

FORCE_LINK_GSI_QTCORE
FORCE_LINK_GSI_QTGUI
FORCE_LINK_GSI_QTWIDGETS
FORCE_LINK_GSI_QTMULTIMEDIA
FORCE_LINK_GSI_QTPRINTSUPPORT
FORCE_LINK_GSI_QTXML
FORCE_LINK_GSI_QTXMLPATTERNS
FORCE_LINK_GSI_QTDESIGNER
FORCE_LINK_GSI_QTNETWORK
FORCE_LINK_GSI_QTSQL
FORCE_LINK_GSI_QTSVG
FORCE_LINK_GSI_QTUITOOLS

#else
# define QT_EXTERNAL_BASE(x)
#endif

#include <QTranslator>
#include <QLocale>
#include <QTextCodec>

#include <iostream>
#include <fstream>
#include <memory>
#include <cstdlib>

int klayout_main (int &argc, char **argv);

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
    QString a;
    for (WCHAR *wc = szArgList [i]; *wc; ++wc) {
      a += QChar ((unsigned int) *wc);
    }
    QByteArray aa = a.toUtf8 ();
    argv [i] = new char [aa.size () + 1];
    strcpy (argv [i], aa.constData ());
  }

  int ret = klayout_main (argCount, argv);

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

  int ret = klayout_main (a_argc, argv);

  for (int i = 0; i < a_argc; i++) {
    delete[] argv [i];
  }
  delete[] argv;

  return ret;
}

#endif

#if QT_VERSION >= 0x050000
void custom_message_handler(QtMsgType type, const QMessageLogContext & /*ctx*/, const QString &msg)
{
  switch (type) {
  case QtDebugMsg:
    fprintf(stderr, "Debug: %s\n", msg.toLocal8Bit ().constData ());
    break;
  case QtWarningMsg:
    if (tl::verbosity () > 0) {
      fprintf(stderr, "Warning: %s\n", msg.toLocal8Bit ().constData ());
    }
    break;
  case QtCriticalMsg:
    fprintf(stderr, "Critical: %s\n", msg.toLocal8Bit ().constData ());
    break;
  case QtFatalMsg:
    fprintf(stderr, "Fatal: %s\n", msg.toLocal8Bit ().constData ());
    abort();
  case QtInfoMsg:
    fprintf(stderr, "Info: %s\n", msg.toLocal8Bit ().constData ());
    break;
  }
}
#else
void custom_message_handler(QtMsgType type, const char *msg)
{
  switch (type) {
  case QtDebugMsg:
    fprintf(stderr, "Debug: %s\n", msg);
    break;
  case QtWarningMsg:
    if (tl::verbosity () > 0) {
      fprintf(stderr, "Warning: %s\n", msg);
    }
    break;
  case QtCriticalMsg:
    fprintf(stderr, "Critical: %s\n", msg);
    break;
  case QtFatalMsg:
    fprintf(stderr, "Fatal: %s\n", msg);
    abort();
  }
}
#endif

static int klayout_main_cont (int &argc, char **argv);

namespace {

class LogFileWriter
  : public tl::Channel
{
public:
  static std::unique_ptr<std::ofstream> m_os;

  LogFileWriter (int min_verbosity, const std::string &prefix)
    : m_min_verbosity (min_verbosity), m_prefix (prefix), m_new_line (true)
  { }

  static bool open (const std::string &path)
  {
    m_os.reset (new std::ofstream (path));
    return m_os->good ();
  }

  void puts (const char *s)
  {
    if (m_os && tl::verbosity () >= m_min_verbosity) {
      m_os->write (s, strlen (s));
    }
  }

  void endl ()
  {
    puts ("\n");
    m_new_line = true;
  }

  void end ()
  {
    if (m_os && tl::verbosity () >= m_min_verbosity) {
      m_os->flush ();
    }
  }

  void begin ()
  {
    if (m_new_line) {
      puts (m_prefix.c_str ());
      m_new_line = false;
    }
  }

  void yield () { }

private:
  int m_min_verbosity;
  std::string m_prefix;
  bool m_new_line;
};

std::unique_ptr<std::ofstream> LogFileWriter::m_os;

}

static void set_log_file (const std::string &log_file)
{
  if (LogFileWriter::open (log_file)) {
    tl::info.add (new LogFileWriter (0, std::string ()), true);
    tl::log.add (new LogFileWriter (10, std::string ()), true);
    tl::warn.add (new LogFileWriter (0, std::string ("Warning: ")), true);
    tl::error.add (new LogFileWriter (0, std::string ("ERROR: ")), true);
  }
}

/**
 *  @brief The basic entry point
 *  Note that by definition, klayout_main receives arguments in UTF-8
 */
int
klayout_main (int &argc, char **argv)
{
  //  install the version strings
  lay::Version::set_exe_name (prg_exe_name);
  lay::Version::set_name (prg_name);
  lay::Version::set_version (prg_version);

  std::string subversion (prg_date);
  subversion += " r";
  subversion += prg_rev;
  lay::Version::set_subversion (subversion.c_str ());

  std::string about_text (prg_author);
  about_text += "\n";
  about_text += prg_date;
  about_text += " r";
  about_text += prg_rev;
  about_text += "\n";
  about_text += "\n";
  about_text += prg_about_text;
  lay::Version::set_about_text (about_text.c_str ());

  //  Capture the shortcut command line arguments, log file and the verbosity settings
  //  for early errors and warnings

  for (int i = 1; i < argc; ++i) {

    if (argv [i] == std::string ("-v")) {

      tl::info << lay::ApplicationBase::version ();
      return 0;

    } else if (argv [i] == std::string ("-h")) {

      tl::info << lay::ApplicationBase::usage () << tl::noendl;
      return 0;

    } else if (argv [i] == std::string ("-k") && (i + 1) < argc) {

      set_log_file (argv [++i]);

    } else if (argv [i] == std::string ("-d") && (i + 1) < argc) {

      int v = 0;
      tl::from_string (argv [++i], v);
      if (v < 0) {
        v = 0;
      }
      tl::verbosity (v);

    }

  }

  //  This special initialization is required by the Ruby interpreter because it wants to mark the stack
  int ret = rba::RubyInterpreter::initialize (argc, argv, &klayout_main_cont);

  //  clean up all static data now, since we don't trust the static destructors.
  //  NOTE: this needs to happen after the Ruby interpreter went down since otherwise the GC will
  //  access objects that are already cleaned up.
  tl::StaticObjects::cleanup ();

  return ret;
}

int 
klayout_main_cont (int &argc, char **argv)
{
#if QT_VERSION >= 0x050000
  qInstallMessageHandler (custom_message_handler);
#else
  qInstallMsgHandler (custom_message_handler);
#endif

  int result = 0;

  try {

    //  initialize the Python interpreter
    pya::PythonInterpreter::initialize ();

    //  this registers the gsi definitions
    gsi::initialize_external ();

    bool non_ui_mode = false;

    //  If we have a -zz option, initialize a QCore application. Otherwise create a QApplication.
    //  That way we can use KLayout as a non-windows application with -zz or -b.
    for (int i = 1; i < argc; ++i) {
      if (argv [i] == std::string ("-zz") || argv [i] == std::string ("-b")) {
        non_ui_mode = true;
        break;
      }
    }

    std::unique_ptr<lay::ApplicationBase> app;
    if (non_ui_mode) {
      app.reset (new lay::NonGuiApplication (argc, argv));
    } else {
      lay::GuiApplication::initialize ();
      app.reset (new lay::GuiApplication (argc, argv));
      lay::enable_signal_handler_gui (true);
    }

    //  configures the application with the command line arguments
    app->parse_cmd (argc, argv);

    //  initialize the application
    app->init_app ();

    /* TODO: this kills valgrind
    QString locale = QLocale::system ().name ();
    QTranslator translator;
    if (app->qapp () && translator.load (QString::fromUtf8 ("klayout_") + locale)) {
      app->qapp ()->installTranslator (&translator);
    }
    */

#if QT_VERSION < 0x050000
    QTextCodec::setCodecForTr (QTextCodec::codecForName ("utf8"));
#endif

    if (app->has_gui ()) {

      BEGIN_PROTECTED_CLEANUP

      result = app->run ();

      END_PROTECTED_CLEANUP {
        result = 1;
      }

    } else {
      result = app->run ();
    }

  } catch (tl::ExitException &ex) {
    result = ex.status ();
  } catch (std::exception &ex) {
    tl::error << ex.what ();
    result = 1;
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    result = 1;
  } catch (...) {
    tl::error << tl::to_string (QObject::tr ("unspecific error"));
    result = 1;
  }

  return result;

}


