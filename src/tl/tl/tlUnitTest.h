
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

#ifndef HDR_tlUnitTest
#define HDR_tlUnitTest

#include "tlCommon.h"
#include "tlLog.h"
#include "tlException.h"
#include "tlString.h"
#include "tlInt128Support.h"

#include <string>
#include <vector>
#include <sstream>

namespace tl {

/**
 *  @brief Returns true, if the test is run in verbose mode
 *  Verbose mode is enabled through the "-v" option
 */
TL_PUBLIC bool verbose ();

/**
 *  @brief Sets verbose mode
 */
TL_PUBLIC void set_verbose (bool v);

/**
 *  @brief Gets the indent for the test output
 */
TL_PUBLIC int indent ();

/**
 *  @brief Sets the indent for the test output
 */
TL_PUBLIC void set_indent (int i);

/**
 *  @brief Returns true if XML output is enabled (JUnit format)
 */
TL_PUBLIC bool xml_format ();

/**
 *  @brief Sets XML format
 */
TL_PUBLIC void set_xml_format (bool x);

/**
 *  @brief Enables or disables "continue" mode
 *  In continue mode, the execution will proceed even in case of an error.
 */
TL_PUBLIC void set_continue_flag (bool f);

/**
 *  @brief Returns true, if the unit test is run in debug mode
 *  In debug mode, the unit tests shall offer information on how to fix the
 *  test. Specifically if layout compare is involved, it shall display the golden
 *  file name and the actual one and terminate to allow updating the files.
 */
TL_PUBLIC bool is_debug_mode ();

/**
 *  @brief Enables or disables debug mode
 */
TL_PUBLIC void set_debug_mode (bool f);

/**
 *  @brief Gets the path of the test source directory
 *  This path is specified through the environment variable $TESTSRC
 */
TL_PUBLIC std::string testsrc ();

/**
 *  @brief Gets the path of the test data
 *  This path is given by "$TESTSRC/testdata"
 */
TL_PUBLIC std::string testdata ();

/**
 *  @brief Gets the path of the private test data
 *  This path is specified through the environment variable $TESTSRC, "testdata" subdirectory and the
 *  private testdata directory. If no private test data is available, this
 *  method will throw a CancelException which makes the test skipped.
 */
TL_PUBLIC std::string testdata_private ();

/**
 *  @brief Gets the path to the temporary data
 *  This path is specified through the environment variable $TESTTMP
 */
TL_PUBLIC std::string testtmp ();

/**
 *  @brief A basic exception for the unit test framework
 */
struct TestException
  : public tl::Exception
{
  TestException (const std::string &msg)
    : tl::Exception (msg)
  { }
};

/**
 *  @brief A generic compare operator
 */
template <class X, class Y>
inline bool equals (const X &a, const Y &b)
{
  return a == b;
}

/**
 *  @brief A specialization of the compare operator for doubles
 */
TL_PUBLIC bool equals (double a, double b);

/**
 *  @brief Specialization of comparison of pointers vs. integers (specifically "0")
 */
template <class X>
inline bool equals (X *a, int b)
{
  return a == (X *) size_t (b);
}

/**
 *  @brief A specialization of comparison of double vs "anything"
 */
template <class Y>
inline bool equals (double a, const Y &b)
{
  return equals (a, double (b));
}

/**
 *  @brief A specialization of comparison of "anything" vs. double
 */
template <class X>
inline bool equals (const X &a, double b)
{
  return equals (double (a), b);
}

/**
 *  @brief A specialization of the compare operator for const char *
 */
inline bool equals (const char *a, const char *b)
{
  return equals (std::string (a), std::string (b));
}

/**
 *  @brief A specialization of the compare operator for std::string vs. const char *
 */
inline bool equals (const std::string &a, const char *b)
{
  return equals (a, std::string (b));
}

/**
 *  @brief A specialization of the compare operator for std::string vs. const char *
 */
inline bool equals (const char *a, const std::string &b)
{
  return equals (std::string (a), b);
}

/**
 *  @brief A utility class to capture the warning, error and info channels
 *
 *  Instantiate this class inside a test. Then run the test and finally
 *  obtain the collected output with CaptureChannel::captured_text().
 */
class TL_PUBLIC CaptureChannel : public tl::Channel
{
public:
  CaptureChannel ();
  ~CaptureChannel ();

  std::string captured_text () const
  {
    return m_text.str ();
  }

  void clear ()
  {
    m_text.str (std::string ());
  }

protected:
  virtual void puts (const char *s);
  virtual void endl ();
  virtual void end ();
  virtual void begin ();
  virtual void yield () { }

private:
  std::ostringstream m_text;
  int m_saved_verbosity;
};

/**
 *  @brief The base class for tests
 */
class TL_PUBLIC TestBase
{
public:
  /**
   *  @brief Constructor
   */
  TestBase (const std::string &file, const std::string &name);

  /**
   *  @brief Destructor
   */
  virtual ~TestBase () { }

  /**
   *  @brief Actually runs the test
   *  @return True, if the test was successful
   */
  bool do_test (bool editable, bool slow);

  /**
   *  @brief Indicates that the test is a long runner
   *  In this case the test is skipped unless in slow mode.
   */
  void test_is_long_runner ();

  /**
   *  @brief Indicates that the test is a test that should run in editable mode only.
   *  In this case the test is skipped unless in editable mode.
   */
  void test_is_editable_only ();

  /**
   *  @brief Indicates that the test is a test that should run in non-editable mode only.
   *  In this case the test is skipped unless in non-editable mode.
   */
  void test_is_non_editable_only ();

  /**
   *  @brief Raises an exception with the given string
   *  This version prints the last checkpoint for reference.
   */
  void raise (const std::string &s);

  /**
   *  @brief Raises an exception with the given string, file and line number
   */
  void raise (const std::string &file, int line, const std::string &s);

  /**
   *  @brief Registers a checkpoint
   */
  void checkpoint (const std::string &file, int line);

  /**
   *  @brief Resets the checkpoints set
   */
  void reset_checkpoint ();

  /**
   *  @brief Compares two text files
   */
  void compare_text_files (const std::string &path_a, const std::string &path_b);

  /**
   *  @brief The test's name
   *  @return The name of the test
   */
  const std::string &name () const
  {
    return m_test;
  }

  /**
   *  @brief Prepares a temporary file path
   *  @param fn The actual name of the file
   *  @return A path suitable for writing a temporary file
   *  The directory for the file will be created within this method.
   */
  std::string tmp_file (const std::string &fn = "tmp") const;

  /**
   *  @brief Removes the temporay file folder
   */
  void remove_tmp_folder ();

  /**
   *  @brief A generic diff printer
   */
  template <class X, class Y>
  void diff (const std::string &file, int line, const std::string &msg, const X &subject, const Y & /*ref*/)
  {
    std::ostringstream sstr;
    sstr << msg << " (actual value is " << subject << ")";
    raise (file, line, sstr.str ());
  }

  /**
   *  @brief A generic diff printer
   */
  template <class X, class Y>
  void detailed_diff (const std::string &file, int line, const std::string &msg, const X &subject, const Y &ref)
  {
    std::ostringstream sstr;
    sstr << msg << std::endl;
    write_detailed_diff (sstr, tl::to_string (subject), tl::to_string (ref));
    raise (file, line, sstr.str ());
  }

  /**
   *  @brief A diff printer for int vs. something
   */
  template <class Y>
  void diff (const std::string &file, int line, const std::string &msg, int subject, const Y &ref)
  {
    detailed_diff (file, line, msg, subject, ref);
  }

  /**
   *  @brief A diff printer for unsigned int vs. something
   */
  template <class Y>
  void diff (const std::string &file, int line, const std::string &msg, unsigned int subject, const Y &ref)
  {
    detailed_diff (file, line, msg, subject, ref);
  }

  /**
   *  @brief A diff printer for long vs. something
   */
  template <class Y>
  void diff (const std::string &file, int line, const std::string &msg, long subject, const Y &ref)
  {
    detailed_diff (file, line, msg, subject, ref);
  }

  /**
   *  @brief A diff printer for unsigned long vs. something
   */
  template <class Y>
  void diff (const std::string &file, int line, const std::string &msg, unsigned long subject, const Y &ref)
  {
    detailed_diff (file, line, msg, subject, ref);
  }

  /**
   *  @brief A diff printer for long long vs. something
   */
  template <class Y>
  void diff (const std::string &file, int line, const std::string &msg, long long subject, const Y &ref)
  {
    detailed_diff (file, line, msg, subject, ref);
  }

  /**
   *  @brief A diff printer for unsigned long long vs. something
   */
  template <class Y>
  void diff (const std::string &file, int line, const std::string &msg, unsigned long long subject, const Y &ref)
  {
    detailed_diff (file, line, msg, subject, ref);
  }

  /**
   *  @brief A diff printer for bool
   */
  inline void diff (const std::string &file, int line, const std::string &msg, bool subject, bool ref)
  {
    detailed_diff (file, line, msg, subject, ref);
  }

  /**
   *  @brief A diff printer for double
   */
  inline void diff (const std::string &file, int line, const std::string &msg, double subject, double ref)
  {
    detailed_diff (file, line, msg, subject, ref);
  }

  /**
   *  @brief A diff printer for strings
   */
  inline void diff (const std::string &file, int line, const std::string &msg, const std::string &subject, const std::string &ref)
  {
    detailed_diff (file, line, msg, subject, ref);
  }

  /**
   *  @brief A diff printer for strings vs. const char *
   */
  inline void diff (const std::string &file, int line, const std::string &msg, const std::string &subject, const char *ref)
  {
    detailed_diff (file, line, msg, subject, ref);
  }

  /**
   *  @brief A diff printer for strings vs. const char *
   */
  inline void diff (const std::string &file, int line, const std::string &msg, const char *subject, const std::string &ref)
  {
    detailed_diff (file, line, msg, subject, ref);
  }

  /**
   *  @brief A diff printer for C strings
   */
  inline void diff (const std::string &file, int line, const std::string &msg, const char *subject, const char *ref)
  {
    diff (file, line, msg, std::string (subject), std::string (ref));
  }

  /**
   *  @brief Main entry point for the compare feature (EXPECT_EQ and EXPECT_NE)
   */
  template <class T1, class T2>
  void eq_helper (bool eq, const T1 &a, const T2 &b, const char *what_expr, const char *equals_expr, const char *file, int line)
  {
    if (tl::equals (a, b) != eq) {
      std::ostringstream sstr;
      sstr << what_expr << " does not equal " << equals_expr;
      diff (file, line, sstr.str (), a, b);
    }
  }

protected:
  /**
   *  @brief Returns a value indicating whether the test runs in editable mode
   */
  bool is_editable () const
  {
    return m_editable;
  }

  /**
   *  @brief Returns a value indicating whether the test runs in slow mode
   */
  bool is_slow () const
  {
    return m_slow;
  }

private:
  virtual void execute (tl::TestBase *_this) = 0;

  void write_detailed_diff (std::ostream &os, const std::string &subject, const std::string &ref);

  bool m_editable, m_slow;
  std::string m_test;
  std::string m_testdir;
  //  last checkpoint
  std::string m_cp_file;
  int m_cp_line;
  bool m_any_failed;
  std::string m_testtmp;
};

/**
 *  @brief The registration facility for tests
 */
class TL_PUBLIC TestRegistrar
{
public:
  static void reg (tl::TestBase *t);
  static TestRegistrar *instance ();
  const std::vector <tl::TestBase *> &tests () const;

private:
  static TestRegistrar *ms_instance;

  TestRegistrar ();

  std::vector <tl::TestBase *> m_tests;
};

} // namespace tl

#define TEST(NAME) \
  namespace {\
struct TestImpl##NAME \
      : public tl::TestBase \
    { \
      TestImpl##NAME () : TestBase (__FILE__, #NAME) { } \
      virtual void execute (tl::TestBase *_this); \
    }; \
    static TestImpl##NAME TestImpl_Inst##NAME; \
  } \
  void TestImpl##NAME::execute (tl::TestBase *_this)

#define EXPECT_EQ(WHAT,EQUALS) \
  _this->checkpoint (__FILE__, __LINE__); \
  _this->eq_helper (true, (WHAT), (EQUALS), #WHAT, #EQUALS, __FILE__, __LINE__);

#define EXPECT_NE(WHAT,EQUALS) \
  _this->checkpoint (__FILE__, __LINE__); \
  _this->eq_helper (false, (WHAT), (EQUALS), #WHAT, #EQUALS, __FILE__, __LINE__);

#define EXPECT(WHAT) \
  _this->checkpoint (__FILE__, __LINE__); \
  if (!(WHAT)) { \
    std::ostringstream sstr; \
    sstr << #WHAT << " is not true"; \
    _this->raise (__FILE__, __LINE__, sstr.str ()); \
  }

#define CHECKPOINT() \
  _this->checkpoint (__FILE__, __LINE__);

#define FAIL_ARG(MSG,WHAT) \
  { \
    std::ostringstream sstr; \
    sstr << MSG << ", value is " << (WHAT); \
    _this->raise (__FILE__, __LINE__, sstr.str ()); \
  }

#endif
