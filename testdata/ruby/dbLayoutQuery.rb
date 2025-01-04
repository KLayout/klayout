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

class DBLayoutQuery_TestClass < TestBase

  def test_1

    ly = RBA::Layout::new
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t11.gds")

    q = RBA::LayoutQuery::new("select cell.name, cell.bbox from *")
    res = []
    q.each(ly) do |iter|
      res << iter.data.map(&:to_s).join(", ")
    end

    assert_equal(res.size, 2)
    assert_equal(res[0], "TOPTOP, (0,0;32800,12800)")
    assert_equal(res[1], "TOP, (0,0;900,900)")

  end

  def test_2

    ly = RBA::Layout::new
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t11.gds")

    q = RBA::LayoutQuery::new("delete TOP")
    q.execute(ly)

    q = RBA::LayoutQuery::new("select cell.name, cell.bbox from *")
    res = []
    q.each(ly) do |iter|
      res << iter.data.map(&:to_s).join(", ")
    end

    assert_equal(res.size, 1)
    assert_equal(res[0], "TOPTOP, ()")

  end

  def test_3

    q = RBA::LayoutQuery::new("delete TOP")
    assert_equal(q.property_names.sort.join(","), "bbox,cell,cell_bbox,cell_dbbox,cell_index,cell_name,dbbox,hier_levels,initial_cell,initial_cell_index,initial_cell_name,inst,instances,path,path_dtrans,path_names,path_trans,references,shape,tot_weight,weight")

  end

  # variables
  def test_4

    ly = RBA::Layout::new
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t11.gds")

    ctx = RBA::ExpressionContext::new
    ctx.var("suffix", "!")
    ctx.var("all", [])
    ctx.var("nonmod", "")

    q = RBA::LayoutQuery::new("select cell.name + suffix from *")
    res = []
    q.each(ly, ctx) do |iter|
      res << iter.data.inspect
    end

    assert_equal(res.size, 2)
    assert_equal(res[0], "[\"TOPTOP!\"]")
    assert_equal(res[1], "[\"TOP!\"]")

    q = RBA::LayoutQuery::new("with * do var nonmod = cell.name; all.push(nonmod)")
    q.execute(ly, ctx)

    assert_equal(ctx.eval("all").join(","), "TOPTOP,TOP")
    # not modified, because we used "var nonmod" in the query which
    # creates a local variable:
    assert_equal(ctx.eval("nonmod"), "")

  end

end

load("test_epilogue.rb")

