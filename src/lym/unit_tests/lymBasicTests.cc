
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#include "lymMacro.h"
#include "lymMacroInterpreter.h"
#include "gsiInterpreter.h"
#include "rba.h"
#include "pya.h"
#include "tlFileUtils.h"

class TestCollectorConsole
  : public gsi::Console
{
public:
  TestCollectorConsole () { }
  ~TestCollectorConsole () { }

  virtual void write_str (const char *text, output_stream)
  {
    m_text += text;
  }

  virtual void flush () { }
  virtual bool is_tty () { return false; }
  virtual int columns () { return 80; }
  virtual int rows () { return 50; }

  const std::string &text () const { return m_text; }

private:
  std::string m_text;
};

#if defined(HAVE_RUBY)

static std::string np (const std::string &s)
{
  return tl::replaced (s, "\\", "/");
}

TEST(1_BasicRuby)
{
  tl_assert (rba::RubyInterpreter::instance () != 0);

  lym::Macro macro;

  macro.set_file_path (tl::testsrc () + "/testdata/lym/m1.rb");
  macro.set_interpreter (lym::Macro::Ruby);
  macro.load ();

  TestCollectorConsole console;
  rba::RubyInterpreter::instance ()->push_console (&console);
  try {
    EXPECT_EQ (macro.run (), 0);
    rba::RubyInterpreter::instance ()->remove_console (&console);
  } catch (...) {
    rba::RubyInterpreter::instance ()->remove_console (&console);
    throw;
  }

  EXPECT_EQ (console.text (), "Hello, world!\n");
}

TEST(2_RubyInclude)
{
  tl_assert (rba::RubyInterpreter::instance () != 0);

  lym::Macro macro;

  macro.set_file_path (tl::testsrc () + "/testdata/lym/m2.rb");
  macro.set_interpreter (lym::Macro::Ruby);
  macro.load ();

  TestCollectorConsole console;
  rba::RubyInterpreter::instance ()->push_console (&console);
  try {
    EXPECT_EQ (macro.run (), 0);
    rba::RubyInterpreter::instance ()->remove_console (&console);
  } catch (...) {
    rba::RubyInterpreter::instance ()->remove_console (&console);
    throw;
  }

  EXPECT_EQ (console.text (), "Stop 1: m2.rb:2\nf: a_inc.rb:3\nStop 2: m2.rb:8\n");
}

TEST(3_RubyInclude)
{
  tl_assert (rba::RubyInterpreter::instance () != 0);

  lym::Macro macro;

  macro.set_file_path (tl::testsrc () + "/testdata/lym/m3.rb");
  macro.set_interpreter (lym::Macro::Ruby);
  macro.load ();

  TestCollectorConsole console;
  rba::RubyInterpreter::instance ()->push_console (&console);
  try {
    EXPECT_EQ (macro.run (), 0);
    rba::RubyInterpreter::instance ()->remove_console (&console);
  } catch (...) {
    rba::RubyInterpreter::instance ()->remove_console (&console);
    throw;
  }

  EXPECT_EQ (np (console.text ()), np ("An error in " + tl::testsrc () + "/testdata/lym/b_inc.rb:3\n"));
}

TEST(4_RubyIncludeFromXML)
{
  tl_assert (rba::RubyInterpreter::instance () != 0);

  lym::Macro macro;

  macro.set_file_path (tl::testsrc () + "/testdata/lym/m4.rb");
  macro.set_interpreter (lym::Macro::Ruby);
  macro.load ();

  TestCollectorConsole console;
  rba::RubyInterpreter::instance ()->push_console (&console);
  try {
    EXPECT_EQ (macro.run (), 0);
    rba::RubyInterpreter::instance ()->remove_console (&console);
  } catch (...) {
    rba::RubyInterpreter::instance ()->remove_console (&console);
    throw;
  }

  EXPECT_EQ (np (console.text ()), np ("An error in " + tl::testsrc () + "/testdata/lym/b_inc.lym:3\n"));
}

TEST(11_DRCBasic)
{
  tl_assert (rba::RubyInterpreter::instance () != 0);

  lym::Macro macro;

  macro.set_file_path (tl::testsrc () + "/testdata/lym/m1.drc");
  macro.set_interpreter (lym::Macro::DSLInterpreter);
  macro.set_dsl_interpreter ("drc");
  macro.load ();

  TestCollectorConsole console;
  rba::RubyInterpreter::instance ()->push_console (&console);
  try {
    EXPECT_EQ (macro.run (), 0);
    rba::RubyInterpreter::instance ()->remove_console (&console);
  } catch (...) {
    rba::RubyInterpreter::instance ()->remove_console (&console);
    throw;
  }

  EXPECT_EQ (console.text (), "Result: (500,500;500,2000;1000,2000;1000,500) in m1.drc:20\n");
}

TEST(12_DRCBasic)
{
  tl_assert (rba::RubyInterpreter::instance () != 0);

  lym::Macro macro;

  macro.set_file_path (tl::testsrc () + "/testdata/lym/m2.drc");
  macro.set_interpreter (lym::Macro::DSLInterpreter);
  macro.set_dsl_interpreter ("drc");
  macro.load ();

  TestCollectorConsole console;
  rba::RubyInterpreter::instance ()->push_console (&console);
  try {
    EXPECT_EQ (macro.run (), 0);
    rba::RubyInterpreter::instance ()->remove_console (&console);
  } catch (...) {
    rba::RubyInterpreter::instance ()->remove_console (&console);
    throw;
  }

  EXPECT_EQ (console.text (), "Result: (500,500;500,2000;1000,2000;1000,500) in m2.drc:14\n");
}

#endif

#if defined(HAVE_PYTHON)

TEST(101_BasicPython)
{
  tl_assert (pya::PythonInterpreter::instance () != 0);

  lym::Macro macro;

  macro.set_file_path (tl::testsrc () + "/testdata/lym/m1.py");
  macro.set_interpreter (lym::Macro::Python);
  macro.load ();

  TestCollectorConsole console;
  pya::PythonInterpreter::instance ()->push_console (&console);
  try {
    EXPECT_EQ (macro.run (), 0);
    pya::PythonInterpreter::instance ()->remove_console (&console);
  } catch (...) {
    pya::PythonInterpreter::instance ()->remove_console (&console);
    throw;
  }

  EXPECT_EQ (console.text (), "Hello, world!\n");
}

TEST(102_PythonInclude)
{
  tl_assert (pya::PythonInterpreter::instance () != 0);

  lym::Macro macro;

  macro.set_file_path (tl::testsrc () + "/testdata/lym/m2.py");
  macro.set_interpreter (lym::Macro::Python);
  macro.load ();

  TestCollectorConsole console;
  pya::PythonInterpreter::instance ()->push_console (&console);
  try {
    EXPECT_EQ (macro.run (), 0);
    pya::PythonInterpreter::instance ()->remove_console (&console);
  } catch (...) {
    pya::PythonInterpreter::instance ()->remove_console (&console);
    throw;
  }

  EXPECT_EQ (console.text (), "Stop 1: m2.py:8\nf: a_inc.py:5\nStop 2: m2.py:14\n");
}

#endif
