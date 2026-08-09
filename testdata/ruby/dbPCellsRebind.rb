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

class PCellRebindVar < RBA::PCellDeclaration

  def initialize(name, layer, w_name, w_default)
    @name = name
    @layer = layer
    @w_name = w_name
    @w_default = w_default
  end

  def display_text(parameters)
    return "#{@name}(L=#{@layer},#{@w_name}=#{parameters[0].to_s},H=#{parameters[1].to_s})"
  end

  def get_parameters
  
    # prepare a set of parameter declarations
    param = []
    
    param.push(RBA::PCellParameterDeclaration.new(@w_name, RBA::PCellParameterDeclaration::TypeDouble, "Width", @w_default))
    param.push(RBA::PCellParameterDeclaration.new("H", RBA::PCellParameterDeclaration::TypeDouble, "Height", 1.0))
    
    return param
    
  end

  def get_layers(parameters)
    return [ RBA::LayerInfo::new(@layer, 0) ]
  end
  
  def produce(layout, layers, parameters, cell)
  
    # fetch the parameters
    w = parameters[0]
    h = parameters[1]
    
    # create the shape
    cell.shapes(layout.layer(@layer, 0)).insert(RBA::DBox.new(-w / 2, -h / 2, w / 2, h / 2))
    
  end

end

class RebindTestLib < RBA::Library

  def initialize(var)
  
    # set the description
    self.description = "PCell rebind test lib"
    
    # create the PCell declarations
    if var == 0
      layout.register_pcell("Box0", PCellRebindVar::new("Box0", 0, "W", 3.0))
      layout.register_pcell("Box1", PCellRebindVar::new("Box1", 1, "W", 2.0))
      layout.register_pcell("Box2A", PCellRebindVar::new("Box2A", 2, "W", 1.0))
    else
      layout.register_pcell("Box2B", PCellRebindVar::new("Box2B", 12, "W", 1.5))
      layout.register_pcell("Box1", PCellRebindVar::new("Box1", 11, "WW", 2.5))
      layout.register_pcell("Box0", PCellRebindVar::new("Box0", 10, "W", 3.5))
    end

    self.technology = (var == 0 ? "T0" : "T1")

    register("RebindTestLib")
    
  end

end

def c2s(cell)
  ly = cell.layout
  s = []
  ly.layer_indexes.collect { |li| ly.get_info(li) }.sort { |a,b| [ a.layer, a.datatype ] <=> [ b.layer, b.datatype ] }.each do |lp|
    bbox = cell.dbbox(ly.layer(lp))
    bbox.empty? || (s << lp.to_s + ":" + bbox.to_s)
  end
  s.join(";")
end

class DBPCellRebind_TestClass < TestBase

  def test_1

    tl0 = RebindTestLib::new(0)
    tl1 = RebindTestLib::new(1)

    assert_equal(tl0.is_for_technology("T0"), true)
    assert_equal(tl0.is_for_technology("T1"), false)
    assert_equal(tl1.is_for_technology("T0"), false)
    assert_equal(tl1.is_for_technology("T1"), true)

    ly = RBA::Layout::new
    ly.technology_name = "T0"

    top = ly.create_cell("TOP")

    # in T0:
    #   Box0 -> on layer 0, w parameter is "W" with default 3.0
    #   Box1 -> on layer 1, w parameter is "W" with default 2.0
    #   Box2A -> on layer 2, w parameter is "W" with default 1.0
    #   Box2B does not exist

    c0 = ly.create_cell("Box0", "RebindTestLib", { "W" => 0.3, "H" => 0.5 })
    c1 = ly.create_cell("Box1", "RebindTestLib", { "W" => 0.4, "H" => 0.6 })
    c2 = ly.create_cell("Box2A", "RebindTestLib", { "W" => 0.5, "H" => 0.7 })

    top.insert(RBA::CellInstArray::new(c0.cell_index(), RBA::Trans::new))
    top.insert(RBA::CellInstArray::new(c1.cell_index(), RBA::Trans::new))
    top.insert(RBA::CellInstArray::new(c2.cell_index(), RBA::Trans::new))

    assert_equal(c0.display_title, "RebindTestLib.Box0(L=0,W=0.3,H=0.5)")
    assert_equal(c2s(c0), "0/0:(-0.15,-0.25;0.15,0.25)")

    assert_equal(c1.display_title, "RebindTestLib.Box1(L=1,W=0.4,H=0.6)")
    assert_equal(c2s(c1), "1/0:(-0.2,-0.3;0.2,0.3)")

    assert_equal(c2.display_title, "RebindTestLib.Box2A(L=2,W=0.5,H=0.7)")
    assert_equal(c2s(c2), "2/0:(-0.25,-0.35;0.25,0.35)")

    # in T0:
    #   Box0 -> on layer 0, w parameter is "W" with default 3.5
    #   Box1 -> on layer 1, w parameter is "WW" with default 2.5
    #   Box2A does not exist
    #   Box2B -> on layer 2, w parameter is "W" with default 1.5

    ly.technology_name = "T1"

    assert_equal(c0.destroyed, false)
    assert_equal(c1.destroyed, false)
    assert_equal(c2.destroyed, true)   # becomes a cold proxy

    c0 = ly.cell("Box0")
    c1 = ly.cell("Box1")
    c2 = ly.cell("Box2A")

    # layer changed, but parameters can be translated
    assert_equal(c0.display_title, "RebindTestLib.Box0(L=10,W=0.3,H=0.5)")
    assert_equal(c2s(c0), "10/0:(-0.15,-0.25;0.15,0.25)")

    # Layer changed and "W" parameter can't be translated as the name changed to "WW"
    assert_equal(c1.display_title, "RebindTestLib.Box1(L=11,WW=2.5,H=0.6)")
    assert_equal(c2s(c1), "11/0:(-1.25,-0.3;1.25,0.3)")

    # Box2A does no longer exist -> cold proxy
    assert_equal(c2.display_title, "<defunct>RebindTestLib.Box2A")
    assert_equal(c2s(c2), "2/0:(-0.25,-0.35;0.25,0.35)")

    ly.technology_name = "T0"

    assert_equal(c0.destroyed, false)
    assert_equal(c1.destroyed, false)
    assert_equal(c2.destroyed, true)   # became an active PCell variant again

    c0 = ly.cell("Box0")
    c1 = ly.cell("Box1")
    c2 = ly.cell("Box2A")

    # layer changed, but parameters can be translated
    assert_equal(c0.display_title, "RebindTestLib.Box0(L=0,W=0.3,H=0.5)")
    assert_equal(c2s(c0), "0/0:(-0.15,-0.25;0.15,0.25)")

    # Layer changed and "WW" parameter can't be translated as the name changed back to "W"
    assert_equal(c1.display_title, "RebindTestLib.Box1(L=1,W=2.0,H=0.6)")
    assert_equal(c2s(c1), "1/0:(-1,-0.3;1,0.3)")

    # Box2A is back to normal as the cold proxy kept the information
    assert_equal(c2.display_title, "RebindTestLib.Box2A(L=2,W=0.5,H=0.7)")
    assert_equal(c2s(c2), "2/0:(-0.25,-0.35;0.25,0.35)")

    tl0._destroy
    tl1._destroy

  end

end

load("test_epilogue.rb")

