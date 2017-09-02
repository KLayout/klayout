
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

#include "tlUnitTest.h"
#include "tlFileUtils.h"
#include "tlTimer.h"
#include "tlStream.h"

#include <QDir>
#include <cmath>

namespace tl
{

// --------------------------------------------------------------------------------------

static bool s_verbose_flag = false;
static bool s_xml_format = false;
static bool s_debug_mode = false;
static bool s_continue_flag = false;
static int s_indent = 4;

bool verbose ()
{
  return s_verbose_flag;
}

void set_verbose (bool f)
{
  s_verbose_flag = f;
}

void set_indent (int i)
{
  s_indent = i;
}

int indent ()
{
  return s_indent;
}

bool xml_format ()
{
  return s_xml_format;
}

void set_xml_format (bool f)
{
  s_xml_format = f;
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
  QDir d (QDir (tl::to_qstring (tl::testsrc ())).filePath (QString::fromUtf8 ("private")));
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
//  CaptureChannel implementation

CaptureChannel::CaptureChannel ()
{
  tl::info.add (this, false);
  tl::error.add (this, false);
  tl::warn.add (this, false);
}

void CaptureChannel::puts (const char *s)
{
  m_text << s;
}

void CaptureChannel::endl ()
{
  m_text << "\n";
}

void CaptureChannel::end ()
{
  //  .. nothing yet ..
}

void CaptureChannel::begin ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------------------------
//  TestRegistrar implementation

tl::TestRegistrar *tl::TestRegistrar::ms_instance = 0;

TestRegistrar::TestRegistrar ()
  : m_tests ()
{
  //  .. nothing yet ..
}

void
TestRegistrar::reg (tl::TestBase *t)
{
  if (! ms_instance) {
    ms_instance = new TestRegistrar ();
  }
  ms_instance->m_tests.push_back (t);
}

TestRegistrar *
TestRegistrar::instance ()
{
  return ms_instance;
}

const std::vector <tl::TestBase *> &
TestRegistrar::tests () const
{
  return m_tests;
}

// --------------------------------------------------------------------------------------
//  TestBase implementation

TestBase::TestBase (const std::string &file, const std::string &name)
  : m_editable (false), m_slow (false), m_cp_line (0), m_any_failed (false)
{
  QFileInfo f (tl::to_qstring (file));
  m_test = tl::to_string (f.baseName ()) + ":" + name;
  m_testdir = tl::to_string (f.baseName ()) + "_" + name;
  tl::TestRegistrar::reg (this);
}

bool TestBase::do_test (bool editable, bool slow)
{
  m_editable = editable;
  m_slow = slow;

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

  execute (this);

  m_testtmp.clear ();

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
    throw tl::TestException (sstr.str ());
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
    throw tl::TestException (sstr.str ());
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

void TestBase::write_detailed_diff (std::ostream &os, const std::string &subject, const std::string &ref)
{
  os << replicate (" ", tl::indent ()) << "Actual value is:    " << tl::to_string (subject) << std::endl
     << replicate (" ", tl::indent ()) << "Reference value is: " << tl::to_string (ref) << std::endl
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
