# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2025 Matthias Koefferlein
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

class LAYMacro_TestClass < TestBase

  def test_1

    macro = RBA::Macro::new

    macro.version = "1.7"
    assert_equal(macro.version, "1.7")

    macro.doc = "%doc"
    assert_equal(macro.doc, "%doc")

    macro.description = "%description"
    assert_equal(macro.description, "%description")

    macro.prolog = "%prolog"
    assert_equal(macro.prolog, "%prolog")

    macro.epilog = "%epilog"
    assert_equal(macro.epilog, "%epilog")

    macro.category = "%category"
    assert_equal(macro.category, "%category")

    macro.shortcut = "%shortcut"
    assert_equal(macro.shortcut, "%shortcut")

    assert_equal(macro.is_autorun?, false)
    macro.is_autorun = true
    assert_equal(macro.is_autorun?, true)

    assert_equal(macro.is_autorun_early?, false)
    macro.is_autorun_early = true
    assert_equal(macro.is_autorun_early?, true)

    macro.format = RBA::Macro::PlainTextFormat
    assert_equal(macro.format == RBA::Macro::PlainTextFormat, true)
    macro.format = RBA::Macro::MacroFormat
    assert_equal(macro.format == RBA::Macro::MacroFormat, true)

    macro.interpreter = RBA::Macro::Ruby
    assert_equal(macro.interpreter == RBA::Macro::Ruby, true)
    assert_equal(macro.interpreter_name, "Ruby")
    macro.interpreter = RBA::Macro::Python
    assert_equal(macro.interpreter == RBA::Macro::Python, true)
    assert_equal(macro.interpreter_name, "Python")

    macro.dsl_interpreter = "%dsl"
    assert_equal(macro.dsl_interpreter, "%dsl")

    macro.text = "%text"
    macro.format = RBA::Macro::PlainTextWithHashAnnotationsFormat
    assert_equal(macro.text, "%text")
    macro.sync_text_with_properties
    assert_equal(macro.text, "# $description: %description\n" +
      "# $prolog: %prolog\n" +
      "# $epilog: %epilog\n" +
      "# $version: 1.7\n" +
      "# $autorun\n" +
      "# $autorun-early\n" +
      "# $shortcut: %shortcut\n" +
      "%text")

    macro.text = "# $description: %description\n" +
      "# $prolog: %prolog\n" +
      "# $epilog: %epilog\n" +
      "# $version: 7.1\n" +
      "# $autorun\n" +
      "# $autorun-early\n" +
      "# $shortcut: %shortcut\n" +
      "%text"
    macro.sync_properties_with_text
    assert_equal(macro.version, "7.1")

    macro.group_name = "%group"
    assert_equal(macro.group_name, "%group")

    assert_equal(macro.show_in_menu?, false)
    macro.show_in_menu = true
    assert_equal(macro.show_in_menu?, true)

    macro.menu_path = "menu.path"
    assert_equal(macro.menu_path, "menu.path")

  end

  def test_2

    macro_file = File.join($ut_testtmp, "test.lym")
    File.open(macro_file, "w") do |file|
      file.write(<<"END")
<?xml version="1.0" encoding="utf-8"?>
<klayout-macro>
 <description>%description</description>
 <version>42</version>
 <interpreter>ruby</interpreter>
 <text>
$test_output = "x" + $test_input
</text>
</klayout-macro>
END
    end

    macro = RBA::Macro::new(macro_file)
    assert_equal(macro.description, "%description")
    assert_equal(macro.path, macro_file)

    $test_input = "42"
    $test_output = ""
    macro.run
    assert_equal($test_output, "x42")

    macro_file2 = File.join($ut_testtmp, "test2.lym")
    macro.save_to(macro_file2)

    macro2 = RBA::Macro::new(macro_file2)
    assert_equal(macro2.path, macro_file2)

  end

  def test_3
    
    pya = RBA::Interpreter.python_interpreter
    if !pya
      return
    end
    
    macro_file = File.join($ut_testtmp, "test.lym")
    File.open(macro_file, "w") do |file|
      file.write(<<"END")
<?xml version="1.0" encoding="utf-8"?>
<klayout-macro>
 <description>%description</description>
 <version>42</version>
 <interpreter>python</interpreter>
 <text>
print("Python calling!")
context.value = "x" + context.value
</text>
</klayout-macro>
END
    end

    macro = RBA::Macro::new(macro_file)
    assert_equal(macro.description, "%description")
    assert_equal(macro.path, macro_file)

    context = RBA::Value::new
    context.value = "42"
    pya.define_variable("context", context)

    macro.run
    assert_equal(context.value, "x42")

    begin
      pya.eval_string("\n1/0")
    rescue => ex
      puts "Got exception (expected): " + ex.to_s
      assert_equal(ex.to_s.index("ZeroDivisionError") != nil, true)
      assert_equal(ex.to_s.index("division by zero") != nil, true)
      # bug #1771
      assert_equal(ex.to_s.index("(eval)") != nil, true)
      assert_equal(ex.to_s.index(":2") != nil, true)
    end

  end

end

load("test_epilogue.rb")
