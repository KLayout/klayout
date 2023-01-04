
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

#include "tlUnitTest.h"
#include "tlFileUtils.h"
#include "tlTimer.h"
#include "tlStream.h"
#include "tlEnv.h"

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
  std::string ts = tl::get_env ("TESTSRC");
  if (ts.empty ()) {
    tl::warn << "TESTSRC undefined";
    ts = ".";
  }
  return ts;
}

std::string testdata ()
{
  return tl::combine_path (tl::testsrc (), "testdata");
}

std::string testdata_private ()
{
  std::string pp = tl::combine_path (tl::testsrc (), "private");
  pp = tl::combine_path (pp, "testdata");
  if (! tl::file_exists (pp)) {
    throw tl::CancelException ();
  }
  return pp;
}

std::string testtmp ()
{
  //  Ensures the test temp directory is present
  std::string tt = tl::get_env ("TESTTMP");
  if (tt.empty ()) {
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

  //  Because we don't want to capture logger messages, we switch verbosity to "silent" during capturing
  m_saved_verbosity = tl::verbosity ();
  tl::verbosity (0);
}

CaptureChannel::~CaptureChannel ()
{
  tl::verbosity (m_saved_verbosity);
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
  m_test = tl::basename (file) + ":" + name;
  m_testdir = tl::basename (file) + "_" + name;
  tl::TestRegistrar::reg (this);
}

void TestBase::remove_tmp_folder ()
{
  std::string tmpdir = tl::combine_path (tl::absolute_file_path (testtmp ()), m_testdir);
  if (tl::file_exists (tmpdir) && ! tl::rm_dir_recursive (tmpdir)) {
    throw tl::Exception ("Unable to clean temporary dir: " + tmpdir);
  }
}

bool TestBase::do_test (bool editable, bool slow)
{
  m_editable = editable;
  m_slow = slow;
  m_any_failed = false;

  //  Ensures the test temp directory is present
  std::string tmpdir = tl::combine_path (tl::absolute_file_path (testtmp ()), m_testdir);
  if (tl::file_exists (tmpdir) && ! tl::rm_dir_recursive (tmpdir)) {
    throw tl::Exception ("Unable to clean temporary dir: " + tmpdir);
  }
  if (! tl::mkpath (tmpdir)) {
    throw tl::Exception ("Unable to create path for temporary files: " + tmpdir);
  }

  m_testtmp = tmpdir;

  static std::string testname_value;
  static std::string testtmp_value;

  putenv (const_cast<char *> ("TESTNAME="));
  testname_value = std::string ("TESTNAME=") + m_test;
  putenv (const_cast<char *> (testname_value.c_str ()));

  putenv (const_cast<char *> ("TESTTMP_WITH_NAME="));
  testtmp_value = std::string ("TESTTMP_WITH_NAME=") + m_testtmp;
  putenv (const_cast<char *> (testtmp_value.c_str ()));

  reset_checkpoint ();

  execute (this);

  m_testtmp.clear ();

  return (!m_any_failed);
}

std::string TestBase::tmp_file (const std::string &fn) const
{
  tl_assert (! m_testtmp.empty ());
  return tl::combine_path (m_testtmp, fn);
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
  tl::InputStream s (path);

  //  NOTE: using the text reader means we normalize CRLF/LF
  tl::TextInputStream ts (s);
  std::string t;
  while (!ts.at_end ()) {
    t += ts.get_line ();
    t += "\n";
  }

  return t;
}

void TestBase::compare_text_files (const std::string &path_a, const std::string &path_b)
{
  bool equal = false;
  bool any = false;

  int n = 0;
  for ( ; ! equal; ++n) {

    std::string fn_a = path_a;  //  no variants for a
    std::string fn_b = path_b;
    if (n > 0) {
      fn_b += tl::sprintf (".%d", n);
    }

    if (tl::file_exists (fn_b)) {

      if (n == 1 && any) {
        throw tl::Exception (tl::sprintf ("Inconsistent reference variants for %s: there can be either variants (.1,.2,... suffix) or a single file (without suffix)", path_b));
      }

      any = true;

      std::string text_a = read_file (fn_a);
      std::string text_b = read_file (fn_b);

      equal = (text_a == text_b);

      if (equal && n > 0) {
        tl::info << tl::sprintf ("Found match on golden reference variant %s", fn_b);
      }

    } else if (n > 0) {
      if (! any) {
        tl::warn << tl::sprintf ("No golden data found (%s)", path_b);
      }
      break;
    }

  }

  if (! equal) {
    throw tl::Exception (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s%s",
                               tl::absolute_file_path (path_a),
                               tl::absolute_file_path (path_b),
                               (n > 1 ? "\nand variants" : "")));
  }
}

}
