# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2026 Matthias Koefferlein
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

if !$:.member?(File::dirname($0))
  $:.push(File::dirname($0))
end

load("test_prologue.rb")

# Tests for the klayout executable
#
# This tests actually runs inside a KLayout/unit_tests instance but 
# only for providing the test automation.

class KLayoutMain_TestClass < TestBase

  def setup
    @klayout_home_name = "KLAYOUT_HOME"
    @klayout_home = ENV[@klayout_home_name]
    # setting "KLAYOUT_HOME" to empty means we don't search any place
    # for macros
    ENV[@klayout_home_name] = ""
  end

  def teardown
    ENV[@klayout_home_name] = @klayout_home
  end

  def klayout_bin
    # special location for MacOS
    file = File.join(RBA::Application::instance.inst_path, "klayout.app", "Contents", "MacOS", "klayout")
    if !File.exist?(file)
      file = File.join(RBA::Application::instance.inst_path, "klayout")
    end
    return file
  end

  def test_1

    # Basic
    version = `#{self.klayout_bin} -v 2>&1`
    assert_equal(version, "#{RBA::Application.instance.version}\n")

  end

  def test_2

    # Basic Ruby
    out = `#{self.klayout_bin} -b -rd v1=42 -rd v2=hello -r #{File.join(File.dirname(__FILE__), "test.rb")} 2>&1`
    assert_equal(out, "Variable v1=42 v2=hello\n")

    out = `#{self.klayout_bin} -b -rd v1=42 -rd v2=hello -r #{File.join(File.dirname(__FILE__), "test.rb")} -rm #{File.join(File.dirname(__FILE__), "test2.rb")} -rm #{File.join(File.dirname(__FILE__), "test3.rb")} 2>&1`
    assert_equal(out, "test2\ntest3\nVariable v1=42 v2=hello\n")

  end

  def test_3

    # Basic Python
    out = `#{self.klayout_bin} -b -rd v1=42 -rd v2=hello -r #{File.join(File.dirname(__FILE__), "test.py")} 2>&1`
    assert_equal(out, "Variable v1=42 v2=hello\n")

  end

  def test_4

    # Application class
    if !RBA.constants.find { |x| x == :QDialog || x == "QDialog" }

      out = `#{self.klayout_bin} -b -r #{File.join(File.dirname(__FILE__), "test_app.rb")} 2>&1`
      assert_equal(out, "RBA::Application superclass Object\nMainWindow is not there\n")

      out = `#{self.klayout_bin} -z -nc -rx -r #{File.join(File.dirname(__FILE__), "test_app.rb")} 2>&1`
      assert_equal(out, "RBA::Application superclass Object\nMainWindow is there\n")

    else
 
      # QCoreApplication for (headless) mode
      out = `#{self.klayout_bin} -b -r #{File.join(File.dirname(__FILE__), "test_app.rb")} 2>&1`
      assert_equal(out, "RBA::Application superclass RBA::QCoreApplication_Native\nMainWindow is not there\n")

      # QApplication for GUI mode
      out = `#{self.klayout_bin} -z -nc -rx -r #{File.join(File.dirname(__FILE__), "test_app.rb")} 2>&1`
      assert_equal(out, "RBA::Application superclass RBA::QApplication_Native\nMainWindow is there\n")

    end

  end

  def test_5

    # Script variables
    out = `#{self.klayout_bin} -b -wd tv1=17 -wd tv2=25 -wd tv3 -r #{File.join(File.dirname(__FILE__), "test_script.rb")} 2>&1`
    assert_equal(out, "42\ntrue\n")

  end

  def test_6

    # Editable / Non-editable mode
    out = `#{self.klayout_bin} -b -ne -r #{File.join(File.dirname(__FILE__), "test_em.rb")} 2>&1`
    assert_equal(out, "false\n")

    out = `#{self.klayout_bin} -b -e -r #{File.join(File.dirname(__FILE__), "test_em.rb")} 2>&1`
    assert_equal(out, "true\n")

  end

  def test_7

    cfg_file = File.join($ut_testtmp, "config.xml")
    File.open(cfg_file, "w") { |file| file.puts("<config/>") }

    # Special configuration file
    `#{self.klayout_bin} -b -c #{cfg_file} -r #{File.join(File.dirname(__FILE__), "test_set_config1.rb")} 2>&1`

    out = `#{self.klayout_bin} -b -c #{cfg_file} -r #{File.join(File.dirname(__FILE__), "test_read_config.rb")} 2>&1`
    assert_equal(out, "42\n")

    # Update
    `#{self.klayout_bin} -b -c #{cfg_file} -r #{File.join(File.dirname(__FILE__), "test_set_config2.rb")} 2>&1`

    out = `#{self.klayout_bin} -b -c #{cfg_file} -r #{File.join(File.dirname(__FILE__), "test_read_config.rb")} 2>&1`
    assert_equal(out, "17\n")

    # Reset
    `#{self.klayout_bin} -b -c #{cfg_file} -r #{File.join(File.dirname(__FILE__), "test_set_config1.rb")} 2>&1`

    out = `#{self.klayout_bin} -b -c #{cfg_file} -r #{File.join(File.dirname(__FILE__), "test_read_config.rb")} 2>&1`
    assert_equal(out, "42\n")

    # No update
    `#{self.klayout_bin} -b -t -c #{cfg_file} -r #{File.join(File.dirname(__FILE__), "test_set_config2.rb")} 2>&1`

    out = `#{self.klayout_bin} -b -c #{cfg_file} -r #{File.join(File.dirname(__FILE__), "test_read_config.rb")} 2>&1`
    assert_equal(out, "42\n")

  end

  def test_8

    # Loading layouts
    out = `#{self.klayout_bin} -z -nc -rx #{File.join(File.dirname(__FILE__), "test1.gds")} -r #{File.join(File.dirname(__FILE__), "test_lay.rb")} 2>&1`
    assert_equal(out, "TOP1\n")

    out = `#{self.klayout_bin} -z -nc -rx #{File.join(File.dirname(__FILE__), "test1.gds")} #{File.join(File.dirname(__FILE__), "test2.gds")} -r #{File.join(File.dirname(__FILE__), "test_lay2.rb")} 2>&1`
    assert_equal(out, "TOP1\nTOP2\n")

    out = `#{self.klayout_bin} -z -nc -rx #{File.join(File.dirname(__FILE__), "test1.gds")} #{File.join(File.dirname(__FILE__), "test2.gds")} -s -r #{File.join(File.dirname(__FILE__), "test_lay2.rb")} 2>&1`
    assert_equal(out, "TOP1;TOP2\n")

  end

  def test_9

    # Sessions
    out = `#{self.klayout_bin} -z -nc -rx -u #{File.join(File.dirname(__FILE__), "session.lys")} -r #{File.join(File.dirname(__FILE__), "test_lay2.rb")} 2>&1`
    assert_equal(out, "TOP2;TOP1\n")

  end

  def test_10

    # Headless LayoutView
    out = `#{self.klayout_bin} -b -rd input=#{File.join(File.dirname(__FILE__), "test1.gds")} -r #{File.join(File.dirname(__FILE__), "test10.rb")} 2>&1`
    assert_equal(out, "(0,0;8,8)\n")

  end

  def test_11

    # Headless LayoutView (GUI enabled)
    out = `#{self.klayout_bin} -z -rd input=#{File.join(File.dirname(__FILE__), "test1.gds")} -r #{File.join(File.dirname(__FILE__), "test10.rb")} 2>&1`
    assert_equal(out, "(0,0;8,8)\n")

  end

  def test_12

    # Application.exit(0) - Python
    out = `#{self.klayout_bin} -z -r #{File.join(File.dirname(__FILE__), "test12.py")} 2>&1`
    assert_equal(out, "Before exit()\n")

    # Application.exit(0) - Ruby
    out = `#{self.klayout_bin} -z -r #{File.join(File.dirname(__FILE__), "test12.rb")} 2>&1`
    assert_equal(out, "Before exit()\n")

    # sys.exit(0) - Python
    out = `#{self.klayout_bin} -z -r #{File.join(File.dirname(__FILE__), "test12s.py")} 2>&1`
    assert_equal(out, "Before exit()\n")

    # quit() - Python (issue #1565)
    out = `#{self.klayout_bin} -z -r #{File.join(File.dirname(__FILE__), "test12q.py")} 2>&1`
    assert_equal(out, "Before quit()\n")

  end

end

load("test_epilogue.rb")
