
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

#include "utTestBase.h"
#include "utTestConsole.h"
#include "utHead.h"
#include "tlFileUtils.h"
#include "tlTimer.h"
#include "tlStream.h"
#include "dbLayout.h"
#include "dbStreamLayers.h"
#include "dbGDS2Writer.h"
#include "dbOASISWriter.h"
#include "dbReader.h"
#include "dbCommonReader.h"
#include "dbLayoutDiff.h"

#include "pya.h"
#include "rba.h"

#include <QDir>

namespace ut
{

// --------------------------------------------------------------------------------------

static bool s_verbose_flag = false;
static bool s_debug_mode = false;
static bool s_continue_flag = false;

bool verbose ()
{
  return s_verbose_flag;
}

void set_verbose (bool f)
{
  s_verbose_flag = f;
}

void set_continue_flag (bool f)
{
  s_continue_flag = f;
}

bool is_debug_mode ()
{
  return s_debug_mode;
}

void set_debug_mode (bool f)
{
  s_debug_mode = f;
}

pya::PythonInterpreter *python_interpreter ()
{
  pya::PythonInterpreter *ip = pya::PythonInterpreter::instance ();
  tl_assert (ip != 0);
  return ip;
}

rba::RubyInterpreter *ruby_interpreter ()
{
  rba::RubyInterpreter *ip = rba::RubyInterpreter::instance ();
  tl_assert (ip != 0);
  return ip;
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

std::string testtmp ()
{
  //  Ensures the test temp directory is present
  const char *tt = getenv ("TESTTMP");
  if (! tt) {
    throw tl::Exception ("TESTTMP undefined");
  }
  return tt;
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

//  TODO: move this to tlString.h
static std::string replicate (const char *s, size_t n)
{
  std::string res;
  res.reserve (strlen (s) * n);
  while (n > 0) {
    res += s;
    --n;
  }
  return res;
}

// --------------------------------------------------------------------------------------
//  TestBase implementation

ut::Registrar *ut::Registrar::ms_instance = 0;

TestBase::TestBase (const std::string &file, const std::string &name)
  : m_editable (false), m_slow (false), m_cp_line (0), m_any_failed (false)
{
  QFileInfo f (tl::to_qstring (file));
  m_test = tl::to_string (f.baseName ()) + ":" + name;
  m_testdir = tl::to_string (f.baseName ()) + "_" + name;
  ut::Registrar::reg (this);
}

bool TestBase::do_test (bool editable, bool slow)
{
  m_editable = editable;
  m_slow = slow;

  ut::ctrl << "<system-out>";

  try {

    //  Ensures the test temp directory is present
    QDir dir (tl::to_qstring (testtmp ()));
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
  QDir dir (tl::to_qstring (testtmp ()));
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

void TestBase::test_is_editable_only ()
{
  if (!m_editable) {
    throw tl::CancelException ();
  }
}

void TestBase::test_is_non_editable_only ()
{
  if (m_editable) {
    throw tl::CancelException ();
  }
}

void TestBase::test_is_long_runner ()
{
  if (!m_slow) {
    throw tl::CancelException ();
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

void TestBase::write_detailed_diff (std::ostream &os, const std::string &subject, const std::string &ref)
{
  os << replicate (" ", TestConsole::instance ()->indent ()) << "Actual value is:    " << tl::to_string (subject) << std::endl
     << replicate (" ", TestConsole::instance ()->indent ()) << "Reference value is: " << tl::to_string (ref) << std::endl
  ;
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

}
