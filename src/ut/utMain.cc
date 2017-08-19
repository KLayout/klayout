
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


#include "utHead.h"
#include "rba.h"
#include "pya.h"
#include "tlStaticObjects.h"
#include "tlTimer.h"
#include "tlSystemPaths.h"
#include "tlFileUtils.h"
#include "layApplication.h"
#include "gsiExpression.h"
#include "gsiExternalMain.h"
#include "gsiDecl.h"
#include "dbStatic.h"

#include "dbLayoutDiff.h"
#include "dbWriter.h"
#include "dbGDS2Reader.h"
#include "dbOASISReader.h"
#include "dbGDS2Writer.h"
#include "dbOASISWriter.h"
#include "dbGDS2Reader.h"

#include <cstdio>
#include <unistd.h>

#if !defined(_WIN32)
#  include <sys/ioctl.h>
#  include <dlfcn.h>
#endif
#if defined(_WIN32)
#  include <Windows.h>
#endif

#include <QDir>
#include <QRegExp>
#include <QTextCodec>

namespace ut 
{

// --------------------------------------------------------------------------------------

ut::Registrar *ut::Registrar::ms_instance = 0;

static bool s_verbose_flag = false;
static bool s_debug_mode = false;
static bool s_continue_flag = false;
static pya::PythonInterpreter *sp_python_interpreter = 0;
static rba::RubyInterpreter *sp_ruby_interpreter = 0;

bool verbose ()
{
  return s_verbose_flag;
}

pya::PythonInterpreter *python_interpreter ()
{
  return sp_python_interpreter;
}

rba::RubyInterpreter *ruby_interpreter ()
{
  return sp_ruby_interpreter;
}

bool is_debug_mode ()
{
  return s_debug_mode;
}

std::string testsrc ()
{
  const char *ts = getenv ("TESTSRC");
  if (! ts) {
    throw tl::Exception ("TESTSRC undefined");
  }
  return ts;
}

std::string testsrc_private ()
{
  QDir d (QDir (tl::to_qstring (ut::testsrc ())).filePath (QString::fromUtf8 ("private")));
  if (! d.exists ()) {
    throw tl::CancelException ();
  }
  return tl::to_string (d.path ());
}


bool equals (double a, double b)
{
  double m = fabs (0.5 * (a + b));
  if (m < 1e-30) {
    //  resolution limit is 1e-30
    return true;
  } else {
    double d = fabs (a - b);
    //  we consider two values equal for the purpose of unit tests if they have the
    //  same value within 1e-10 (0.00000001%).
    return d < 1e-10 * m;
  }
}

std::string replicate (const char *s, size_t n)
{
  std::string res;
  res.reserve (strlen (s) * n);
  while (n > 0) {
    res += s;
    --n;
  }
  return res;
}

void print_error (const std::string &s)
{
  tl::error << "ERROR: " << s;
}

// ------------------------------------------------
//  tl::Channel implementations for redirecting the log output

const char *ANSI_RED = "\033[31;1m";
const char *ANSI_BLUE = "\033[34m";
const char *ANSI_GREEN = "\033[32m";
const char *ANSI_RESET = "\033[0m";

/**
 *  @brief Redirects the interpreter output to ut::print_stdout and ut::print_stderr
 */
class TestConsole
  : public gsi::Console
{
public:
  static TestConsole *instance ()
  {
    tl_assert (ms_instance != 0);
    return ms_instance;
  }

  TestConsole (FILE *file, bool xml_format)
    : m_file (file), m_xml_format (xml_format), m_col (0), m_max_col (250), m_columns (50), m_rows (0), m_is_tty (false)
  {
    ms_instance = this;
    m_indent = 4;

    m_is_tty = isatty (fileno (file)) && ! xml_format;

#if !defined(_WIN32)
    if (m_is_tty) {
      struct winsize ws;
      ioctl (fileno (stdout), TIOCGWINSZ, &ws);
      m_columns = std::max (0, (int) ws.ws_col);
      m_rows = std::max (0, (int) ws.ws_row);
    }
#endif
  }

  ~TestConsole ()
  {
    if (ms_instance == this) {
      ms_instance = 0;
    }
  }

  int indent () const
  {
    return m_indent;
  }

  bool xml_format () const
  {
    return m_xml_format;
  }

  void write_str (const char *text, output_stream os)
  {
    if (os == OS_stderr) {
      begin_error ();
      basic_write (text);
      end ();
    } else {
      basic_write (text);
    }
  }

  void raw_write (const char *text)
  {
    fputs (text, m_file);
  }

  virtual void flush ()
  {
    fflush (m_file);
  }

  virtual bool is_tty ()
  {
    //  NOTE: this assumes we are delivering to stdout
    return m_is_tty;
  }

  virtual int columns ()
  {
    return std::max (m_columns - m_indent, 0);
  }

  virtual int rows ()
  {
    return m_rows;
  }

  int real_columns ()
  {
    return m_columns;
  }

  void begin_error ()
  {
    if (m_is_tty) {
      fputs (ANSI_RED, m_file);
    }
  }

  void begin_info ()
  {
    if (m_is_tty) {
      fputs (ANSI_GREEN, m_file);
    }
  }

  void begin_warn ()
  {
    if (m_is_tty) {
      fputs (ANSI_BLUE, m_file);
    }
  }

  void end ()
  {
    if (m_is_tty) {
      fputs (ANSI_RESET, m_file);
    }
  }

  void basic_write (const char *s)
  {
    if (m_xml_format) {

      for (const char *cp = s; *cp; ++cp) {
        if (*cp == '&') {
          fputs ("&amp;", m_file);
        } else if (*cp == '<') {
          fputs ("&lt;", m_file);
        } else if (*cp == '>') {
          fputs ("&gt;", m_file);
        } else {
          fputc (*cp, m_file);
        }
      }

    } else {

      //  line length limitation - this assumes we are always printing to the same terminal
      //  or we don't mix stderr/stdout.
      const char *cp;
      for (cp = s; *cp; ++cp) {
        if (*cp == '\n' || *cp == '\r') {
          m_col = 0;
          fputc (*cp, m_file);
        } else {
          if (m_col == 0) {
            for (int i = 0; i < m_indent; ++i) {
              fputc (' ', m_file);
            }
            m_col = m_indent;
          }
          if (m_col > m_max_col) {
            //  ignore char
          } else if (m_col == m_max_col) {
            fputs (" ...", m_file);
            ++m_col;
          } else if (*cp == '\033') {
            //  skip ANSI escape sequences (no increment of s_col)
            const char *cpend = cp + 1;
            if (*cpend == '[') {
              ++cpend;
              while (*cpend && *cpend != 'm') {
                ++cpend;
              }
              if (*cpend) {
                ++cpend;
              }
            }
            while (cp != cpend) {
              fputc (*cp++, m_file);
            }
            --cp;
          } else {
            fputc (*cp, m_file);
            ++m_col;
          }
        }
      }

    }
  }

private:
  FILE *m_file;
  bool m_xml_format;
  int m_col;
  int m_max_col;
  int m_columns, m_rows;
  bool m_is_tty;
  int m_indent;
  static TestConsole *ms_instance;
};

TestConsole *TestConsole::ms_instance = 0;

class InfoChannel : public tl::Channel
{
public:
  InfoChannel (int verbosity)
    : m_verbosity (verbosity)
  {
    //  .. nothing yet ..
  }

protected:
  virtual void puts (const char *s)
  {
    if (tl::verbosity () >= m_verbosity) {
      TestConsole::instance ()->basic_write (s);
    }
  }

  virtual void endl ()
  {
    if (tl::verbosity () >= m_verbosity) {
      TestConsole::instance ()->basic_write ("\n");
    }
  }

  virtual void end ()
  {
    TestConsole::instance ()->flush ();
  }

  virtual void begin ()
  {
    //  .. nothing yet ..
  }

private:
  int m_verbosity;
};

class WarningChannel : public tl::Channel
{
public:
  WarningChannel ()
  {
    //  .. nothing yet ..
  }

protected:
  virtual void puts (const char *s)
  {
    TestConsole::instance ()->basic_write (s);
  }

  virtual void endl ()
  {
    TestConsole::instance ()->basic_write ("\n");
  }

  virtual void end ()
  {
    TestConsole::instance ()->end ();
    TestConsole::instance ()->flush ();
  }

  virtual void begin ()
  {
    TestConsole::instance ()->begin_warn ();
  }
};

class ErrorChannel : public tl::Channel
{
public:
  ErrorChannel ()
  {
    //  .. nothing yet ..
  }

protected:
  virtual void puts (const char *s)
  {
    TestConsole::instance ()->basic_write (s);
  }

  virtual void endl ()
  {
    TestConsole::instance ()->basic_write ("\n");
  }

  virtual void end ()
  {
    TestConsole::instance ()->end ();
    TestConsole::instance ()->flush ();
  }

  virtual void begin ()
  {
    TestConsole::instance ()->begin_error ();
  }
};

void write_detailed_diff (std::ostream &os, const std::string &subject, const std::string &ref)
{
  os << replicate (" ", TestConsole::instance ()->indent ()) << "Actual value is:    " << tl::to_string (subject) << std::endl
     << replicate (" ", TestConsole::instance ()->indent ()) << "Reference value is: " << tl::to_string (ref) << std::endl
  ;
}

class CtrlChannel : public tl::Channel
{
public:
  CtrlChannel (bool with_xml)
    : m_with_xml (with_xml)
  {
    //  .. nothing yet ..
  }

protected:
  virtual void puts (const char *s)
  {
    if (m_with_xml == TestConsole::instance ()->xml_format ()) {
      TestConsole::instance ()->raw_write (s);
    }
  }

  virtual void endl ()
  {
    if (m_with_xml == TestConsole::instance ()->xml_format ()) {
      TestConsole::instance ()->raw_write ("\n");
    }
  }

  virtual void end ()
  {
    if (m_with_xml == TestConsole::instance ()->xml_format ()) {
      TestConsole::instance ()->end ();
      TestConsole::instance ()->flush ();
    }
  }

  virtual void begin ()
  {
    if (m_with_xml == TestConsole::instance ()->xml_format ()) {
      TestConsole::instance ()->begin_info ();
    }
  }

private:
  bool m_with_xml;
};

std::string
escape_xml (const std::string &s)
{
  std::string res;
  for (const char *cp = s.c_str (); *cp; ++cp) {
    if (*cp == '\"') {
      res += "&quot;";
    } else if (*cp == '<') {
      res += "&lt;";
    } else if (*cp == '>') {
      res += "&gt;";
    } else if (*cp == '&') {
      res += "&amp;";
    } else {
      res += *cp;
    }
  }
  return res;
}

tl::LogTee noctrl (new CtrlChannel (false), true);
tl::LogTee ctrl (new CtrlChannel (true), true);

// --------------------------------------------------------------------------------------
//  TestBase implementation

static QDir testtmp ()
{
  //  Ensures the test temp directory is present
  const char *tt = getenv ("TESTTMP");
  if (! tt) {
    throw tl::Exception ("TESTTMP undefined");
  }

  return QDir (tl::to_qstring (tt));
}

TestBase::TestBase (const std::string &file, const std::string &name)
  : m_cp_line (0), m_any_failed (false)
{
  QFileInfo f (tl::to_qstring (file));
  m_test = tl::to_string (f.baseName ()) + ":" + name;
  m_testdir = tl::to_string (f.baseName ()) + "_" + name;
  ut::Registrar::reg (this);
}

bool TestBase::do_test (const std::string & /*mode*/)
{
  ut::ctrl << "<system-out>";

  try {

    //  Ensures the test temp directory is present
    QDir dir (testtmp ());
    QDir tmpdir (dir.absoluteFilePath (tl::to_qstring (m_testdir)));
    if (tmpdir.exists () && ! tl::rm_dir_recursive (tmpdir.absolutePath ())) {
      throw tl::Exception ("Unable to clean temporary dir: " + tl::to_string (tmpdir.absolutePath ()));
    }
    if (! dir.mkpath (tl::to_qstring (m_testdir))) {
      throw tl::Exception ("Unable to create path for temporary files: " + tl::to_string (tmpdir.absolutePath ()));
    }
    dir.cd (tl::to_qstring (m_testdir));

    m_testtmp = dir.absolutePath ();

    static std::string testname_value;
    static std::string testtmp_value;

    putenv (const_cast<char *> ("TESTNAME="));
    testname_value = std::string ("TESTNAME=") + m_test;
    putenv (const_cast<char *> (testname_value.c_str ()));

    putenv (const_cast<char *> ("TESTTMP_WITH_NAME="));
    testtmp_value = std::string ("TESTTMP_WITH_NAME=") + m_testtmp.toUtf8().constData();
    putenv (const_cast<char *> (testtmp_value.c_str ()));

    reset_checkpoint ();

    tl::Timer timer;
    timer.start();

    execute (this);

    timer.stop();

    m_testtmp.clear ();

    ut::ctrl << "</system-out>";

    ut::noctrl << "Time: " << timer.sec_wall () << "s (wall) " << timer.sec_user () << "s (user) " << timer.sec_sys () << "s (sys)";
    ut::ctrl << "<x-testcase-times wall=\"" << timer.sec_wall () << "\" user=\"" << timer.sec_user () << "\" sys=\"" << timer.sec_sys () << "\"/>";

  } catch (...) {
    ut::ctrl << "</system-out>";
    throw;
  }

  return (!m_any_failed);
}

std::string TestBase::tmp_file (const std::string &fn) const
{
  tl_assert (! m_testtmp.isEmpty ());
  QDir dir (m_testtmp);
  return tl::to_string (dir.absoluteFilePath (tl::to_qstring (fn)));
}

/**
 *  @brief Recursively empties a directory
 */
static void empty_dir (QDir dir)
{
  QStringList entries = dir.entryList (QDir::AllEntries | QDir::NoDotAndDotDot);
  for (QStringList::const_iterator e = entries.begin (); e != entries.end (); ++e) {
    QString epath = dir.absoluteFilePath (*e);
    if (QFileInfo (epath).isDir ()) {
      empty_dir (QDir (epath));
      dir.rmdir (*e);
    } else if (! dir.remove (*e)) {
      throw tl::Exception ("Unable to remove file or directory: " + tl::to_string (dir.filePath (*e)));
    }
  }
}

void TestBase::remove_tmp_folder ()
{
  //  Ensures the test temp directory is present
  QDir dir (testtmp ());
  if (dir.cd (tl::to_qstring (m_test))) {

    empty_dir (dir);

    dir.cdUp ();
    if (! dir.rmdir (tl::to_qstring (m_test))) {
      throw tl::Exception ("Unable to remove directory: " + tl::to_string (dir.filePath (tl::to_qstring (m_test))));
    }

  }
}

void TestBase::checkpoint (const std::string &file, int line)
{
  m_cp_file = file;
  m_cp_line = line;
}

void TestBase::reset_checkpoint ()
{
  m_cp_file = std::string ();
  m_cp_line = 0;
}

void TestBase::raise (const std::string &file, int line, const std::string &msg)
{
  std::ostringstream sstr;
  sstr << file << ", line " << line << ": " << msg;
  if (s_continue_flag) {
    tl::error << sstr.str ();
    m_any_failed = true;
  } else {
    throw ut::Exception (sstr.str ());
  }
}

void TestBase::raise (const std::string &msg)
{
  std::ostringstream sstr;
  if (m_cp_line > 0) {
    sstr << "(last checkpoint: " << m_cp_file << ", line " << m_cp_line << "): ";
  }
  sstr << msg;
  if (s_continue_flag) {
    tl::error << sstr.str ();
    m_any_failed = true;
  } else {
    throw ut::Exception (sstr.str ());
  }
}

void TestBase::compare_layouts (const db::Layout &layout, const std::string &au_file, NormalizationMode norm, db::Coord tolerance)
{
  compare_layouts (layout, au_file, db::LayerMap (), true, norm, tolerance);
}

void TestBase::compare_layouts (const db::Layout &layout, const std::string &au_file, const db::LayerMap &lm, bool read_other_layers, NormalizationMode norm, db::Coord tolerance)
{
  //  normalize the layout by writing to GDS and reading from ..
 
  //  generate a "unique" name ...
  unsigned int hash = 0;
  for (const char *cp = au_file.c_str (); *cp; ++cp) {
    hash = (hash << 4) ^ (hash >> 4) ^ ((unsigned int) *cp);
  }

  std::string tmp_file;

  if (norm == WriteGDS2) {

    tmp_file = ut::TestBase::tmp_file (tl::sprintf ("tmp_%x.gds", hash));

    tl::OutputStream stream (tmp_file.c_str ());
    db::GDS2Writer writer;
    db::SaveLayoutOptions options;
    writer.write (const_cast<db::Layout &> (layout), stream, options);

  } else {

    tmp_file = ut::TestBase::tmp_file (tl::sprintf ("tmp_%x.oas", hash));

    tl::OutputStream stream (tmp_file.c_str ());
    db::OASISWriter writer;
    db::SaveLayoutOptions options;
    writer.write (const_cast<db::Layout &> (layout), stream, options);

  }

  const db::Layout *subject = 0;
  db::Layout layout2;

  if (norm != NoNormalization) {

    //  read all layers from the original layout, so the layer table is the same 
    for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
      layout2.insert_layer ((*l).first, *(*l).second);
    }
    
    tl::InputStream stream (tmp_file);
    db::Reader reader (stream);
    reader.read (layout2);

    subject = &layout2;

  } else {
    subject = &layout;
  }

  bool equal = false;
  bool any = false;

  int n = 0;
  for ( ; ! equal; ++n) {

    db::Layout layout_au;

    //  read all layers from the original layout, so the layer table is the same 
    for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
      layout_au.insert_layer ((*l).first, *(*l).second);
    }
    
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lm;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = read_other_layers;

    std::string fn = au_file;
    if (n > 0) {
      fn += tl::sprintf (".%d", n);
    }

    if (QFileInfo (tl::to_qstring (fn)).exists ()) {

      if (n == 1 && any) {
        throw tl::Exception (tl::sprintf ("Inconsistent reference variants for %s: there can be either variants (.1,.2,... suffix) or a single file (without suffix)", au_file));
      }

      any = true;

      tl::InputStream stream (fn);
      db::Reader reader (stream);
      reader.read (layout_au, options);

      equal = db::compare_layouts (*subject, layout_au, (n > 0 ? db::layout_diff::f_silent : db::layout_diff::f_verbose) | db::layout_diff::f_flatten_array_insts /*| db::layout_diff::f_no_text_details | db::layout_diff::f_no_text_orientation*/, tolerance, 100 /*max diff lines*/);
      if (equal && n > 0) {
        tl::info << tl::sprintf ("Found match on golden reference variant %s", fn);
      }

    } else if (n > 0) {
      if (! any) {
        tl::warn << tl::sprintf ("No golden data found (%s)", au_file);
      }
      break;
    }

  }

  if (! equal) {
    raise (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s%s",
                        tl::to_string (QFileInfo (tl::to_qstring (tmp_file)).absoluteFilePath ()),
                        tl::to_string (QFileInfo (tl::to_qstring (au_file)).absoluteFilePath ()),
                        (n > 1 ? "\nand variants" : "")));
  }
}

static std::string read_file (const std::string &path)
{
  QFile file (tl::to_qstring (path));
  if (! file.open (QIODevice::ReadOnly | QIODevice::Text)) {
    tl::warn << tl::sprintf ("Unable to open file %s", path);
  }

  QByteArray ba = file.readAll ();
  return std::string (ba.constData (), 0, ba.size ());
}

void TestBase::compare_text_files (const std::string &path_a, const std::string &path_b)
{
  std::string text_a = read_file (path_a);
  std::string text_b = read_file (path_b);

  if (text_a != text_b) {
    raise (tl::sprintf ("Compare failed - see:\n  file 1: %s\n  file 2: %s",
                        tl::to_string (QFileInfo (tl::to_qstring (path_a)).absoluteFilePath ()),
                        tl::to_string (QFileInfo (tl::to_qstring (path_b)).absoluteFilePath ())));
  }
}


static int main_cont (int argc, char **argv);

int
main (int argc, char **argv)
{
  int ret = rba::RubyInterpreter::initialize (argc, argv, &main_cont);

  //  NOTE: this needs to happen after the Ruby interpreter went down since otherwise the GC will
  //  access objects that are already cleaned up.
  tl::StaticObjects::cleanup ();

  return ret;
}

int
main_cont (int argc, char **argv)
{
  pya::PythonInterpreter::initialize ();
  gsi::initialize_external ();

  //  Search and initialize plugin unit tests

  QStringList name_filters;
  name_filters << QString::fromUtf8 ("*.ut");

  QDir inst_dir (tl::to_qstring (tl::get_inst_path ()));
  QStringList inst_modules = inst_dir.entryList (name_filters);
  inst_modules.sort ();

  for (QStringList::const_iterator im = inst_modules.begin (); im != inst_modules.end (); ++im) {

    QFileInfo ut_file (inst_dir.path (), *im);
    if (ut_file.exists () && ut_file.isReadable ()) {

      std::string pp = tl::to_string (ut_file.absoluteFilePath ());
      tl::log << "Loading plugin unit tests " << pp;

      //  NOTE: since we are using a different suffix ("*.ut"), we can't use QLibrary.
#ifdef _WIN32
      //  there is no "dlopen" on mingw, so we need to emulate it.
      HINSTANCE handle = LoadLibraryW ((const wchar_t *) tl::to_qstring (pp).constData ());
      if (! handle) {
        throw tl::Exception (tl::to_string (QObject::tr ("Unable to load plugin tests: %s with error message: %s ")), pp, GetLastError ());
      }
#else
      void *handle;
      handle = dlopen (tl::string_to_system (pp).c_str (), RTLD_LAZY);
      if (! handle) {
        throw tl::Exception (tl::to_string (QObject::tr ("Unable to load plugin tests: %s")), pp);
      }
#endif

    }

  }

  //  No side effects
  tl::set_klayout_path (std::vector<std::string> ());

  int ac = 2;
  static char av0[] = "unit_test";
  static char av1[] = "-z";   //  don't show main window
  static char av2[] = "-nc";  //  No configuration file
  static char av3[] = "-rx";  //  No mplicit macros
  char *av[] = { av0, av1, av2, av3, 0 };
  lay::Application app (ac, av, false);

#if QT_VERSION < 0x050000
  QTextCodec::setCodecForTr (QTextCodec::codecForName ("utf8"));
#endif

  bool editable = true, non_editable = true;
  bool exclude = false;
  bool gsi_coverage = false;
  bool gsi_coverage_selected = false;
  std::set<std::string> class_names;
  std::vector<std::string> test_list;
  std::vector<std::string> exclude_test_list;

  bool xml_format = false;

  for (int i = 1; i < argc; ++i) {

    std::string a = argv[i];
    if (a == "-h") {

      std::cout << "unit_test <Options> <Test list>" << std::endl
                << "Options:" << std::endl
                << "  -a          XML output format" << std::endl
                << "  -l          List tests and exit" << std::endl
                << "  -e          Editable mode only" << std::endl
                << "  -ne         Non-editable mode only" << std::endl
                << "  -c          Continue on error" << std::endl
                << "  -v          Verbose mode" << std::endl
                << "  -d          debug mode (stop on error, indicate fix instructions)" << std::endl
                << "  -g          GSI coverage mode - print GSI methods that have not been called" << std::endl
                << "  -gg <class> GSI coverage mode, confined to this class (can be given multiple times)" << std::endl
                << "  -x          Exclude following tests" << std::endl
                << "Test list: list of match strings selecting some tests (default: all)" << std::endl;
      return 0;

    } else if (a == "-l") {

      std::cout << "List of installed tests:" << std::endl;
      for (std::vector<ut::TestBase *>::const_iterator i = ut::Registrar::instance()->tests ().begin (); i != ut::Registrar::instance()->tests ().end (); ++i) {
        std::cout << "  " << (*i)->name () << std::endl;
      }
      return 0;

    } else if (a == "-a") {

      xml_format = true;

    } else if (a == "-g") {

      gsi_coverage = true;

    } else if (a == "-gg") {

      gsi_coverage = true;
      gsi_coverage_selected = true;
      if (i + 1 < argc) {
        ++i;
        class_names.insert (argv [i]);
      }

    } else if (a == "-e") {

      non_editable = false;
      editable = true;

    } else if (a == "-ne") {

      non_editable = true;
      editable = false;

    } else if (a == "-c") {

      ut::s_continue_flag = true;

    } else if (a == "-d") {

      ut::s_debug_mode = true;

    } else if (a == "-v") {

      ut::s_verbose_flag = true;

    } else if (a == "-x") {

      exclude = true;

    } else {

      if (exclude) {
        exclude_test_list.push_back (a);
      } else {
        test_list.push_back (a);
      }

    }

  }

  ut::TestConsole console (stdout, xml_format);

  //  redirect the log channels
  tl::warn.clear ();
  tl::warn.add (new ut::WarningChannel (), true);
  tl::info.clear ();
  tl::info.add (new ut::InfoChannel (0), true);
  tl::log.clear ();
  tl::log.add (new ut::InfoChannel (10), true);
  tl::error.clear ();
  tl::error.add (new ut::ErrorChannel (), true);

  int result = 0;

  try {

    tl::Timer grand_timer;

    grand_timer.start ();

    ut::sp_ruby_interpreter = dynamic_cast <rba::RubyInterpreter *> (&app.ruby_interpreter ());
    ut::sp_python_interpreter = dynamic_cast <pya::PythonInterpreter *> (&app.python_interpreter ());

    if (ut::sp_ruby_interpreter) {
      ut::sp_ruby_interpreter->push_console (&console);
    }
    if (ut::sp_python_interpreter) {
      ut::sp_python_interpreter->push_console (&console);
    }

    ut::ctrl << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
    ut::ctrl << "<testsuites>";

    ut::noctrl << replicate ("=", console.real_columns ());
    ut::noctrl << "Entering KLayout test suite";

    tl::info << "TESTSRC=" << ut::testsrc ();
    tl::info << "TESTTMP=" << tl::to_string (ut::testtmp ().absolutePath ());

    const std::vector<ut::TestBase *> *selected_tests = 0;
    std::vector<ut::TestBase *> subset;
    if (! test_list.empty ()) {

      selected_tests = &subset;
      tl::info << "Selected tests:";

      for (std::vector<ut::TestBase *>::const_iterator i = ut::Registrar::instance()->tests ().begin (); i != ut::Registrar::instance()->tests ().end (); ++i) {

        bool exclude = false;
        for (std::vector<std::string>::const_iterator m = exclude_test_list.begin (); m != exclude_test_list.end (); ++m) {
          QRegExp re (tl::to_qstring (*m), Qt::CaseInsensitive, QRegExp::Wildcard);
          if (re.indexIn (tl::to_qstring ((*i)->name ())) == 0) {
            exclude = true;
            break;
          }
        }

        for (std::vector<std::string>::const_iterator m = test_list.begin (); !exclude && m != test_list.end (); ++m) {
          QRegExp re (tl::to_qstring (*m), Qt::CaseInsensitive, QRegExp::Wildcard);
          if (re.indexIn (tl::to_qstring ((*i)->name ())) == 0) {
            tl::info << "  " << (*i)->name ();
            subset.push_back (*i);
            break;
          }
        }

      }

    } else {
      selected_tests = &ut::Registrar::instance()->tests ();
    }

    ut::s_verbose_flag = false;
    int failed_ne = 0, failed_e = 0;
    std::vector <ut::TestBase *> failed_tests_e, failed_tests_ne;
    int skipped_ne = 0, skipped_e = 0;
    std::vector <ut::TestBase *> skipped_tests_e, skipped_tests_ne;

    for (int e = 0; e < 2; ++e) {

      if ((non_editable && e == 0) || (editable && e == 1)) {

        std::string mode (e == 0 ? "non-editable" : "editable");
        ut::ctrl << "<testsuite name=\"ut-runner-" << mode << "\">";

        ut::noctrl << replicate ("=", console.real_columns ());
        ut::noctrl << "Running tests in " << mode << " mode ...";
        app.set_editable (e != 0);

        int failed = 0;
        std::vector <ut::TestBase *> failed_tests;
        int skipped = 0;
        std::vector <ut::TestBase *> skipped_tests;

        tl::Timer timer;

        timer.start ();

        try {

          failed = 0;
          failed_tests.clear ();
          skipped = 0;
          skipped_tests.clear ();

          for (std::vector <ut::TestBase *>::const_iterator t = selected_tests->begin (); t != selected_tests->end (); ++t) {
            (*t)->remove_tmp_folder ();
          }

          for (std::vector <ut::TestBase *>::const_iterator t = selected_tests->begin (); t != selected_tests->end (); ++t) {

            ut::ctrl << "<testcase name=\"" << (*t)->name () << "\">";

            ut::noctrl << replicate ("-", TestConsole::instance ()->real_columns ());
            ut::noctrl << "Running " << (*t)->name ();

            try {

              if (! (*t)->do_test (mode)) {

                ut::ctrl << "<error message=\"" << "Test " << escape_xml ((*t)->name ()) << " failed (continued mode - see previous messages)" << "\"/>";
                tl::error << "Test " << (*t)->name () << " failed (continued mode - see previous messages)";

                failed_tests.push_back (*t);
                ++failed;

              }

            } catch (tl::CancelException &) {

              ut::ctrl << "<skipped/>";
              tl::error << "Test " << (*t)->name () << " skipped";

              skipped_tests.push_back (*t);
              ++skipped;

            } catch (tl::Exception &ex) {

              ut::ctrl << "<failure message=\"" << escape_xml (ex.msg ()) << "\"/>";
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

        ut::noctrl << replicate ("=", console.real_columns ());
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

      ut::noctrl << replicate ("=", console.real_columns ());
      ut::noctrl << "GSI coverage test";

      ut::ctrl << "<x-gsi-coverage>";

      bool first = true;
      for (gsi::ClassBase::class_iterator c = gsi::ClassBase::begin_classes (); c != gsi::ClassBase::end_classes (); ++c) {

        if (gsi_coverage_selected && class_names.find (c->name ()) == class_names.end ()) {
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
              tl::warn << replicate (" ", console.indent ()) << "Class " << c->name ();
              first_of_class = false;
            }
            tl::warn << replicate (" ", console.indent () * 2) << (*m)->to_string ();

          }

        }

      }

      if (first) {
        tl::info << "GSI coverage test passed.";
      }

      ut::ctrl << "</x-gsi-coverage>";

    }

    ut::noctrl << ut::replicate ("=", console.real_columns ());
    ut::noctrl << "Grand Summary";

    ut::ctrl << "<x-grand-summary>";

    if (skipped_e + skipped_ne > 0) {
      if (non_editable) {
        tl::warn << "Skipped in non-editable mode";
        for (std::vector <ut::TestBase *>::const_iterator f = skipped_tests_ne.begin (); f != skipped_tests_ne.end (); ++f) {
          tl::warn << replicate (" ", console.indent ()) << (*f)->name ();
        }
      }
      if (editable) {
        tl::warn << "Skipped in editable mode";
        for (std::vector <ut::TestBase *>::const_iterator f = skipped_tests_e.begin (); f != skipped_tests_e.end (); ++f) {
          tl::warn << replicate (" ", console.indent ()) << (*f)->name ();
        }
      }
      tl::warn << tl::to_string (skipped_e + skipped_ne) << " test(s) skipped";
    }

    result = failed_e + failed_ne;
    if (result > 0) {
      if (non_editable) {
        tl::warn << "Failed in non-editable mode";
        for (std::vector <ut::TestBase *>::const_iterator f = failed_tests_ne.begin (); f != failed_tests_ne.end (); ++f) {
          tl::warn << replicate (" ", console.indent ()) << (*f)->name ();
        }
      }
      if (editable) {
        tl::warn << "Failed in editable mode";
        for (std::vector <ut::TestBase *>::const_iterator f = failed_tests_e.begin (); f != failed_tests_e.end (); ++f) {
          tl::warn << replicate (" ", console.indent ()) << (*f)->name ();
        }
      }
      tl::warn << tl::to_string (result) << " test(s) failed";
    } else {
      tl::info << "All tests passed.";
    }

    ut::ctrl << "</x-grand-summary>";

    ut::noctrl << "Grand total time: " << grand_timer.sec_wall () << "s (wall) " << grand_timer.sec_user () << "s (user) " << grand_timer.sec_sys () << "s (sys)";
    ut::ctrl << "<x-grand-summary-times wall=\"" << grand_timer.sec_wall () << "\" user=\"" << grand_timer.sec_user () << "\" sys=\"" << grand_timer.sec_sys () << "\"/>";

    if (ut::sp_ruby_interpreter) {
      ut::sp_ruby_interpreter->remove_console (&console);
    }
    if (ut::sp_python_interpreter) {
      ut::sp_python_interpreter->remove_console (&console);
    }

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

  ut::ctrl << "</testsuites>";

  return result;
}



} // namespace ut

