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

ruby_dir = File.join(File::dirname($0), "..", "ruby")
if !$:.member?(ruby_dir)
  $:.push(ruby_dir)
end

load("test_prologue.rb")

# Tests for the buddy executables
#
# This tests actually runs inside a KLayout/unit_tests instance but 
# only for providing the test automation.

class Buddies_TestClass < TestBase

  def buddy_bin(name)
    file = File.join($ut_inst_path, name)
    return file
  end

  def test_basic

    # Basic - buddies can be called
    %w(strm2cif strm2dxf strm2gds strm2gdstxt strm2oas strm2mag strm2txt strmclip strmcmp strmrun strmxor).each do |bin|
      version = bin + " " + `#{self.buddy_bin(bin)} --version`
      assert_equal(version =~ /^#{bin} \d+\./, 0) 
    end

  end

  def test_converters

    signatures = {
      "strm2cif" => 0x28434946,
      "strm2dxf" => 0x300a5345,
      "strm2gds" => 0x00060002,
      "strm2gdstxt" => 0x48454144,
      "strm2oas" => 0x2553454d,
      "strm2txt" => 0x62656769,
      "strm2mag" => 0x6d616769,
    }

    # Windows CRLF -> LF translation
    signature_equiv = {
      0x300d0a53 => 0x300a5345
    }

    %w(strm2cif strm2dxf strm2gds strm2gdstxt strm2oas strm2txt strm2mag).each do |bin|
   
      puts "Testing #{bin} ..."

      if bin == "strm2mag"
        out_file = File.join($ut_testtmp, "TOP1.mag")
      else
        out_file = File.join($ut_testtmp, "out_" + bin)
      end
      if File.exist?(out_file)
        File.unlink(out_file)
      end
      assert_equal(File.exist?(out_file), false)

      in_file = File.join(File.dirname(__FILE__), "test1.gds")

      log = bin + "\n" + `#{self.buddy_bin(bin)} #{in_file} #{out_file} 2>&1`
      assert_equal(File.exist?(out_file), true)
      assert_equal(log, bin + "\n")

      File.open(out_file, "rb") do |file|
        sig = file.read(4).unpack('N').first
        sig = signature_equiv[sig] || sig
        assert_equal(sig, signatures[bin])
      end

    end

  end

  def test_strmxor

    out_file = File.join($ut_testtmp, "out")
    if File.exist?(out_file)
      File.unlink(out_file)
    end
    assert_equal(File.exist?(out_file), false)

    in_file1 = File.join(File.dirname(__FILE__), "test1.gds")
    in_file2 = File.join(File.dirname(__FILE__), "test2.gds")

    log = "strmxor\n" + `#{self.buddy_bin("strmxor")} #{in_file1} #{in_file2} #{out_file} 2>&1`
    assert_equal(File.exist?(out_file), true)
    assert_equal(log, <<"END")
strmxor
Warning: Layer 1/0 is not present in second layout, but in first
Warning: Layer 2/0 is not present in first layout, but in second
Result summary (layers without differences are not shown):

  Layer      Output       Differences (shape count)
  ----------------------------------------------------------------
  1/0        -            (no such layer in second layout)
  2/0        -            (no such layer in first layout)

END

  end

  def test_strmcmp

    in_file1 = File.join(File.dirname(__FILE__), "test1.gds")
    in_file2 = File.join(File.dirname(__FILE__), "test2.gds")

    log = "strmcmp\n" + `#{self.buddy_bin("strmcmp")} #{in_file1} #{in_file2} 2>&1`
    assert_equal(log, <<"END")
strmcmp
ERROR: Layer 1/0 is not present in layout b, but in a
ERROR: Layer 2/0 is not present in layout a, but in b
ERROR: Cell TOP1 is not present in layout b, but in a
ERROR: Cell TOP2 is not present in layout a, but in b
ERROR: Layouts differ
END

  end

  def test_strmclip

    out_file = File.join($ut_testtmp, "out")
    if File.exist?(out_file)
      File.unlink(out_file)
    end
    assert_equal(File.exist?(out_file), false)

    in_file = File.join(File.dirname(__FILE__), "test1.gds")

    log = "strmclip\n" + `#{self.buddy_bin("strmclip")} #{in_file} #{out_file} 2>&1`
    assert_equal(File.exist?(out_file), true)
    assert_equal(log, "strmclip\n")

  end

  def test_strmrun

    in_file = File.join(File.dirname(__FILE__), "test2.rb")

    log = "strmrun\n" + `#{self.buddy_bin("strmrun")} #{in_file} 2>&1`
    assert_equal(log, "strmrun\ntest2\n")

  end

end

load("test_epilogue.rb")
