# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2024 Matthias Koefferlein
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

class DBGlyph_TestClass < TestBase

  # Glyph basics
  def test_1_Glyph

    tg = RBA::TextGenerator.default_generator

    # TODO: no default generator in non-Qt mode (no resources)
    tg || return

    assert_equal(tg.name, "std_font")
    assert_equal(tg.description, "Non-manhattan (0.6 x 0.8 um cell)")
    assert_equal(tg.width.to_s, "600")
    assert_equal(tg.dwidth.to_s, "0.6")
    assert_equal(tg.height.to_s, "800")
    assert_equal(tg.dheight.to_s, "0.8")
    assert_equal(tg.design_grid.to_s, "50")
    assert_equal(tg.ddesign_grid.to_s, "0.05")
    assert_equal(tg.line_width.to_s, "100")
    assert_equal(tg.dline_width.to_s, "0.1")
    assert_equal(tg.dbu.to_s, "0.001")
    assert_equal(tg.background.to_s, "(-100,-100;600,800)")
    assert_equal(tg.dbackground.to_s, "(-0.1,-0.1;0.6,0.8)")
    assert_equal(tg.text("HO", 0.01).to_s, "(0,0;0,70;10,70;10,40;40,40;40,70;50,70;50,0;40,0;40,30;10,30;10,0);(70,0;60,10;60,60;70,70;100,70;110,60;110,35;100,35;100,55;95,60;75,60;70,55;70,15;75,10;95,10;100,15;100,35;110,35;110,10;100,0)")
    assert_equal(tg.text("H", 0.01).to_s, "(0,0;0,70;10,70;10,40;40,40;40,70;50,70;50,0;40,0;40,30;10,30;10,0)")
    assert_equal(tg.text("H", 0.01, 2.0).to_s, "(0,0;0,140;20,140;20,80;80,80;80,140;100,140;100,0;80,0;80,60;20,60;20,0)")
    assert_equal(tg.text("H", 0.01, 1.0, true).to_s, "(-10,-10;-10,70;40,70;40,40;10,40;10,70;0,70;0,0;10,0;10,30;40,30;40,0;50,0;50,70;-10,70;-10,80;60,80;60,-10)")
    assert_equal(tg.text("H", 0.01, 1.0, false, 0.01).to_s, "(-1,-1;-1,71;11,71;11,41;39,41;39,71;51,71;51,-1;39,-1;39,29;11,29;11,-1)")
    assert_equal(tg.text("--\\n--", 0.01, 1.0, false, 0.0).to_s, "(5,30;5,40;45,40;45,30);(65,30;65,40;105,40;105,30);(5,-50;5,-40;45,-40;45,-50);(65,-50;65,-40;105,-40;105,-50)")
    assert_equal(tg.text("--\\n--", 0.01, 1.0, false, 0.0, 0.1).to_s, "(5,30;5,40;45,40;45,30);(75,30;75,40;115,40;115,30);(5,-50;5,-40;45,-40;45,-50);(75,-50;75,-40;115,-40;115,-50)")
    assert_equal(tg.text("--\\n--", 0.01, 1.0, false, 0.0, 0.1, 0.1).to_s, "(5,30;5,40;45,40;45,30);(75,30;75,40;115,40;115,30);(5,-60;5,-50;45,-50;45,-60);(75,-60;75,-50;115,-50;115,-60)")

    tgg = RBA::TextGenerator.generators
    assert_equal(tgg.size > 0, true)
    assert_equal(tgg[0].name, "std_font")

    tg = RBA::TextGenerator::new
    tg.load_from_resource(":/fonts/std_font.gds")
    assert_equal(tg.name, "std_font")

    # gives an error because the resource does not exist
    begin
      tg = RBA::TextGenerator::new
      tg.load_from_resource(":/fonts/does_not_exist.gds")
      assert_equal(false, true)
    rescue => ex
      assert_equal(ex.to_s, "Resource not found: :/fonts/does_not_exist.gds in TextGenerator::load_from_resource")
    end

  end

end

load("test_epilogue.rb")
