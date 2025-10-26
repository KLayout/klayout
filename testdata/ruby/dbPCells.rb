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

def first_shape(s)

  sdup = s.dup
  shape = sdup.shape
  sdup._destroy

  return shape

end

class BoxPCell < RBA::PCellDeclaration

  def display_text(parameters)
    # provide a descriptive text for the cell
    return "Box(L=#{parameters[0].to_s},W=#{'%.3f' % parameters[1].to_s},H=#{'%.3f' % parameters[2].to_s})"
  end

  def cell_name(parameters)
    # provide a cell name for the PCell
    return "Box_L#{parameters[0].to_s.gsub('/','d')}_W#{('%.3f' % parameters[1]).gsub('.','p')}_H#{('%.3f' % parameters[2]).gsub('.','p')}"
  end
  
  def get_parameters
  
    # prepare a set of parameter declarations
    param = []
    
    param.push(RBA::PCellParameterDeclaration.new("l", RBA::PCellParameterDeclaration::TypeLayer, "Layer", RBA::LayerInfo::new(0, 0)))
    param.push(RBA::PCellParameterDeclaration.new("w", RBA::PCellParameterDeclaration::TypeDouble, "Width", 1.0))
    param.push(RBA::PCellParameterDeclaration.new("h", RBA::PCellParameterDeclaration::TypeDouble, "Height", 1.0))
    
    return param
    
  end

  def get_layers(parameters)
    return [ parameters[0] ]
  end
  
  def produce(layout, layers, parameters, cell)
  
    dbu = layout.dbu

    # fetch the parameters
    l = parameters[0]
    w = parameters[1] / layout.dbu
    h = parameters[2] / layout.dbu
    
    # create the shape
    cell.shapes(layers[0]).insert(RBA::Box.new(-w / 2, -h / 2, w / 2, h / 2))
    
  end

  def can_create_from_shape(layout, shape, layer)
    return shape.is_box?
  end

  def transformation_from_shape(layout, shape, layer)
    return RBA::Trans::new(shape.box.center - RBA::Point::new)
  end

  def parameters_from_shape(layout, shape, layer)
    return [ layout.get_info(layer), shape.box.width * layout.dbu, shape.box.height * layout.dbu ]
  end
    
end

class PCellTestLib < RBA::Library

  def initialize  
  
    # set the description
    self.description = "PCell test lib"
    
    # create the PCell declarations
    layout.register_pcell("Box", BoxPCell::new)

    sb_index = layout.add_cell("StaticBox")
    l10 = layout.insert_layer(RBA::LayerInfo::new(10, 0))
    sb_cell = layout.cell(sb_index)
    sb_cell.shapes(l10).insert(RBA::Box::new(0, 0, 100, 200))
    
    # register us with the name "MyLib"
    register("PCellTestLib")
    
  end

end

if RBA.constants.member?(:PCellDeclarationHelper)

  # A PCell based on the declaration helper

  class BoxPCell2 < RBA::PCellDeclarationHelper

    def initialize

      super()

      param("layer", BoxPCell2::TypeLayer, "Layer", :default => RBA::LayerInfo::new(0, 0))
      param("width", BoxPCell2::TypeDouble, "Width", :default => 1.0)
      param("height", BoxPCell2::TypeDouble, "Height", :default => 1.0)
      param("secret", BoxPCell2::TypeInt, "Secret", :default => 0)

    end
      
    def display_text_impl
      # provide a descriptive text for the cell
      return "Box2(L=" + layer.to_s + ",W=" + ('%.3f' % width) + ",H=" + ('%.3f' % height) + ")"
    end
    
    def cell_name_impl
      # provide a cell name for the PCell
      return "Box2_L" + layer.to_s.gsub('/', 'd') + "_W" + ('%.3f' % width).gsub('.', 'p') + "_H" + ('%.3f' % height).gsub('.', 'p')
    end
    
    def produce_impl
    
      # fetch the parameters
      l = layer_layer
      w = width / layout.dbu
      h = height / layout.dbu
      
      # create the shape
      cell.shapes(l).insert(RBA::Box::new(-w / 2, -h / 2, w / 2, h / 2))

    end
      
    def coerce_parameters_impl
      self.secret = [0, self.secret].max
    end

    def can_create_from_shape_impl
      return self.shape.is_box?
    end

    def transformation_from_shape_impl
      return RBA::Trans::new(shape.box.center - RBA::Point::new)
    end

    def parameters_from_shape_impl
      # NOTE: because there is one parameter called "layer" already, we need to use
      # the "_layer" fallback to access the argument to this method
      set_layer(_layout.get_info(_layer))
      set_width(shape.box.width * _layout.dbu)
      set_height(shape.box.height * _layout.dbu)
    end
      
  end

  class PCellTestLib2 < RBA::Library

    def initialize  
    
      # set the description
      description = "PCell test lib2"
      
      # create the PCell declarations
      layout.register_pcell("Box2", BoxPCell2::new)

      # register us with the name "MyLib"
      self.register("PCellTestLib2")

    end

  end

  # A recursive PCell

  class RecursivePCell < RBA::PCellDeclarationHelper

    def initialize

      super()
    
      param("layer", RecursivePCell::TypeLayer, "Layer", :default => RBA::LayerInfo::new(0, 0))
      param("line", RecursivePCell::TypeShape, "Line", :default => RBA::Edge::new(0, 0, 10000, 0))
      param("level", RecursivePCell::TypeInt, "Level", :default => 1)

    end
      
    def display_text_impl
      # provide a descriptive text for the cell
      return "RecursivePCell(L=" + self.layer.to_s + ",E=" + (RBA::CplxTrans::new(self.layout.dbu) * self.line).to_s + ",LVL=" + self.level.to_s
    end
    
    def produce_impl
    
      # fetch the parameters
      l = self.layer_layer
      e = self.line

      if self.level <= 0
        self.cell.shapes(l).insert(e)
        return
      end

      d3 = e.d * (1.0 / 3.0)
      d3n = RBA::Vector::new(-d3.y, d3.x)

      e1 = RBA::Edge::new(e.p1, e.p1 + d3)
      e2 = RBA::Edge::new(e1.p2, e1.p2 + d3 * 0.5 + d3n * Math::cos(Math::PI / 6))
      e3 = RBA::Edge::new(e2.p2, e.p1 + d3 * 2.0)
      e4 = RBA::Edge::new(e3.p2, e.p2)

      [ e1, e2, e3, e4 ].each do |e|
        t = RBA::Trans::new(e.p1 - RBA::Point::new)
        cc = self.layout.create_cell("RecursivePCell", { "layer" => self.layer, "line" => t.inverted * e, "level" => self.level - 1 })
        self.cell.insert(RBA::CellInstArray::new(cc, t))
      end

    end

  end
      
  class PCellTestLib3 < RBA::Library

    def initialize
    
      # set the description
      self.description = "PCell test lib3"
      
      # create the PCell declarations
      self.layout().register_pcell("RecursivePCell", RecursivePCell::new)

      # register us with the name "PCellTestLib3"
      self.register("PCellTestLib3")

    end

  end

end

# A helper for testing: provide an inspect method
class RBA::LayerInfo
  def inspect
    "<" + self.to_s + ">"
  end
end

def norm_hash(hash)
 
  str = []
  hash.keys.sort.each do |k|
    str << k.inspect + "=>" + hash[k].inspect
  end
  "{" + str.join(", ") + "}"

end
    

class DBPCellAPI_TestClass < TestBase

  def test_1

    # PCellParameterDeclaration

    decl = RBA::PCellParameterDeclaration::new("name", RBA::PCellParameterDeclaration::TypeString, "description")

    assert_equal(decl.name, "name")
    assert_equal(decl.description, "description")
    assert_equal(decl.default.inspect, "nil")
    assert_equal(decl.unit, "")
    assert_equal(decl.type, RBA::PCellParameterDeclaration::TypeString)

    decl = RBA::PCellParameterDeclaration::new("name", RBA::PCellParameterDeclaration::TypeString, "description", "17")

    assert_equal(decl.name, "name")
    assert_equal(decl.description, "description")
    assert_equal(decl.type, RBA::PCellParameterDeclaration::TypeString)
    assert_equal(decl.default.to_s, "17")
    assert_equal(decl.unit, "")

    decl = RBA::PCellParameterDeclaration::new("name", RBA::PCellParameterDeclaration::TypeString, "description", "17", "unit")

    assert_equal(decl.name, "name")
    assert_equal(decl.description, "description")
    assert_equal(decl.type, RBA::PCellParameterDeclaration::TypeString)
    assert_equal(decl.default.to_s, "17")
    assert_equal(decl.unit, "unit")

    decl.name = "n"
    assert_equal(decl.name, "n")
    decl.description = "d"
    assert_equal(decl.description, "d")
    decl.unit = "u"
    assert_equal(decl.unit, "u")
    decl.tooltip = "ttt"
    assert_equal(decl.tooltip, "ttt")
    decl.type = RBA::PCellParameterDeclaration::TypeBoolean
    assert_equal(decl.type, RBA::PCellParameterDeclaration::TypeBoolean)
    decl.default = true
    assert_equal(decl.default.to_s, "true")

    decl.type = RBA::PCellParameterDeclaration::TypeInt
    assert_equal(decl.min_value.inspect, "nil")
    assert_equal(decl.max_value.inspect, "nil")
    decl.min_value = "-1"
    assert_equal(decl.min_value.to_s, "-1")
    decl.max_value = "42"
    assert_equal(decl.max_value.to_s, "42")
    decl.min_value = nil
    decl.max_value = nil
    assert_equal(decl.min_value.inspect, "nil")
    assert_equal(decl.max_value.inspect, "nil")

    assert_equal(decl.hidden?, false)
    decl.hidden = true
    assert_equal(decl.hidden?, true)

    assert_equal(decl.readonly?, false)
    decl.readonly = true
    assert_equal(decl.readonly?, true)

    decl.add_choice("first", 42)
    assert_equal(decl.choice_values, [42])
    assert_equal(decl.choice_descriptions, ["first"])
    decl.clear_choices
    assert_equal(decl.choice_values, [])
    assert_equal(decl.choice_descriptions, [])

  end

end

class DBPCell_TestClass < TestBase

  def test_1

    # instantiate and register the library
    tl = PCellTestLib::new

    ly = RBA::Layout::new(true)
    ly.dbu = 0.01

    li1 = ly.layer(1, 0)

    ci1 = ly.add_cell("c1")
    c1 = ly.cell(ci1)

    lib = RBA::Library::library_by_name("NoLib")
    assert_equal(lib == nil, true)
    lib = RBA::Library::library_by_name("PCellTestLib")
    assert_equal(lib != nil, true)
    pcell_decl = lib.layout.pcell_declaration("x")
    assert_equal(pcell_decl == nil, true)
    pcell_decl = lib.layout.pcell_declaration("Box")
    assert_equal(pcell_decl != nil, true)
    pcell_decl_id = lib.layout.pcell_id("Box")
    assert_equal(pcell_decl.id, pcell_decl_id)
    assert_equal(lib.layout.pcell_names.join(":"), "Box")
    assert_equal(lib.layout.pcell_ids, [ pcell_decl_id ])
    assert_equal(lib.layout.pcell_declaration(pcell_decl_id).id, pcell_decl_id)

    param = [ RBA::LayerInfo::new(1, 0) ]  # rest is filled with defaults
    pcell_var_id = ly.add_pcell_variant(lib, pcell_decl_id, param)
    pcell_var = ly.cell(pcell_var_id)
    assert_equal(pcell_var.name, "Box_L1d0_W1p000_H1p000")
    assert_equal(pcell_var.qname, "PCellTestLib.Box")
    assert_equal(pcell_var.basic_name, "Box")
    pcell_inst = c1.insert(RBA::CellInstArray::new(pcell_var_id, RBA::Trans::new))
    assert_equal(pcell_var.layout.inspect, ly.inspect)
    assert_equal(pcell_var.library.inspect, lib.inspect)
    assert_equal(pcell_var.is_pcell_variant?, true)
    assert_equal(pcell_var.display_title, "PCellTestLib.Box(L=1/0,W=1.000,H=1.000)")
    assert_equal(pcell_var.basic_name, "Box")
    assert_equal(c1.is_pcell_variant?, false)
    assert_equal(c1.is_pcell_variant?(pcell_inst), true)
    assert_equal(pcell_var.pcell_id, pcell_decl_id)
    assert_equal(pcell_var.pcell_library.inspect, lib.inspect)
    assert_equal(pcell_var.pcell_parameters.inspect, "[<1/0>, 1.0, 1.0]")
    assert_equal(norm_hash(pcell_var.pcell_parameters_by_name), "{\"h\"=>1.0, \"l\"=><1/0>, \"w\"=>1.0}")
    assert_equal(pcell_var.pcell_parameter("h").inspect, "1.0")
    assert_equal(c1.pcell_parameters(pcell_inst).inspect, "[<1/0>, 1.0, 1.0]")
    assert_equal(norm_hash(c1.pcell_parameters_by_name(pcell_inst)), "{\"h\"=>1.0, \"l\"=><1/0>, \"w\"=>1.0}")
    assert_equal(c1.pcell_parameter(pcell_inst, "h").inspect, "1.0")
    assert_equal(norm_hash(pcell_inst.pcell_parameters_by_name), "{\"h\"=>1.0, \"l\"=><1/0>, \"w\"=>1.0}")
    assert_equal(pcell_inst["h"].inspect, "1.0")
    assert_equal(pcell_inst["i"].inspect, "nil")
    assert_equal(pcell_inst.pcell_parameter("h").inspect, "1.0")
    assert_equal(pcell_var.pcell_declaration.inspect, pcell_decl.inspect)
    assert_equal(c1.pcell_declaration(pcell_inst).inspect, pcell_decl.inspect)
    assert_equal(pcell_inst.pcell_declaration.inspect, pcell_decl.inspect)

    pcell_inst["h"] = 2.0
    assert_equal(norm_hash(pcell_inst.pcell_parameters_by_name), "{\"h\"=>2.0, \"l\"=><1/0>, \"w\"=>1.0}")
    pcell_inst["abc"] = "a property"
    assert_equal(pcell_inst.property("abc").inspect, "\"a property\"")

    c1.clear

    param = [ RBA::LayerInfo::new(1, 0), 5.0, 10.0 ]
    pcell_var_id = ly.add_pcell_variant(lib, pcell_decl_id, param)
    pcell_var = ly.cell(pcell_var_id)
    pcell_inst = c1.insert(RBA::CellInstArray::new(pcell_var_id, RBA::Trans::new))
    assert_equal(pcell_var.layout.inspect, ly.inspect)
    assert_equal(pcell_var.library.inspect, lib.inspect)
    assert_equal(pcell_var.is_pcell_variant?, true)
    assert_equal(pcell_var.display_title, "PCellTestLib.Box(L=1/0,W=5.000,H=10.000)")
    assert_equal(pcell_var.basic_name, "Box")
    assert_equal(c1.is_pcell_variant?, false)
    assert_equal(c1.is_pcell_variant?(pcell_inst), true)
    assert_equal(pcell_var.pcell_id, pcell_decl_id)
    assert_equal(pcell_var.pcell_library.inspect, lib.inspect)
    assert_equal(pcell_var.pcell_parameters.inspect, "[<1/0>, 5.0, 10.0]")
    assert_equal(c1.pcell_parameters(pcell_inst).inspect, "[<1/0>, 5.0, 10.0]")
    assert_equal(pcell_inst.pcell_parameters.inspect, "[<1/0>, 5.0, 10.0]")
    assert_equal(pcell_var.pcell_declaration.inspect, pcell_decl.inspect)
    assert_equal(c1.pcell_declaration(pcell_inst).inspect, pcell_decl.inspect)

    li1 = ly.layer_indices.find { |li| ly.get_info(li).to_s == "1/0" }
    assert_equal(li1 != nil, true)
    assert_equal(ly.is_valid_layer?(li1), true)
    assert_equal(ly.get_info(li1).to_s, "1/0")

    lib_proxy_id = ly.add_lib_cell(lib, lib.layout.cell_by_name("StaticBox"))
    lib_proxy = ly.cell(lib_proxy_id)
    assert_equal(lib_proxy.display_title, "PCellTestLib.StaticBox")
    assert_equal(lib_proxy.basic_name, "StaticBox")
    assert_equal(lib_proxy.layout.inspect, ly.inspect)
    assert_equal(lib_proxy.library.inspect, lib.inspect)
    assert_equal(lib_proxy.is_pcell_variant?, false)
    assert_equal(lib.layout.cell(lib.layout.cell_by_name("StaticBox")).library.inspect, "nil")

    li2 = ly.layer_indices.find { |li| ly.get_info(li).to_s == "10/0" }
    assert_equal(li2 != nil, true)

    assert_equal(first_shape(ly.begin_shapes(c1.cell_index, li1)).to_s, "box (-250,-500;250,500)")
    assert_equal(first_shape(ly.begin_shapes(lib_proxy.cell_index, li2)).to_s, "box (0,0;10,20)")

    param = { "w" => 1, "h" => 2 }
    c1.change_pcell_parameters(pcell_inst, param)
    assert_equal(first_shape(ly.begin_shapes(c1.cell_index, li1)).to_s, "box (-50,-100;50,100)")
    
    param = [ RBA::LayerInfo::new(1, 0), 5.0, 5.0 ]
    c1.change_pcell_parameters(pcell_inst, param)
    assert_equal(first_shape(ly.begin_shapes(c1.cell_index, li1)).to_s, "box (-250,-250;250,250)")
    
    pcell_inst.change_pcell_parameters({ "w" => 2.0, "h" => 10.0 })
    assert_equal(first_shape(ly.begin_shapes(c1.cell_index, li1)).to_s, "box (-100,-500;100,500)")

    pcell_inst.change_pcell_parameters([ RBA::LayerInfo::new(1, 0), 5.0, 5.0 ])
    assert_equal(first_shape(ly.begin_shapes(c1.cell_index, li1)).to_s, "box (-250,-250;250,250)")

    pcell_inst.change_pcell_parameter("w", 5.0)
    pcell_inst.change_pcell_parameter("h", 1.0)
    assert_equal(first_shape(ly.begin_shapes(c1.cell_index, li1)).to_s, "box (-250,-50;250,50)")

    c1.change_pcell_parameter(pcell_inst, "w", 10.0)
    c1.change_pcell_parameter(pcell_inst, "h", 2.0)
    assert_equal(first_shape(ly.begin_shapes(c1.cell_index, li1)).to_s, "box (-500,-100;500,100)")

    assert_equal(ly.cell(pcell_inst.cell_index).is_pcell_variant?, true)
    assert_equal(pcell_inst.is_pcell?, true)
    new_id = ly.convert_cell_to_static(pcell_inst.cell_index)
    assert_equal(new_id == pcell_inst.cell_index, false)
    assert_equal(ly.cell(new_id).is_pcell_variant?, false)
    param = [ RBA::LayerInfo::new(1, 0), 5.0, 5.0 ]
    c1.change_pcell_parameters(pcell_inst, param)
    assert_equal(first_shape(ly.begin_shapes(c1.cell_index, li1)).to_s, "box (-250,-250;250,250)")
    pcell_inst.cell_index = new_id
    assert_equal(first_shape(ly.begin_shapes(c1.cell_index, li1)).to_s, "box (-500,-100;500,100)")

    l10 = ly.layer(10, 0)
    c1.shapes(l10).insert(RBA::Box::new(0, 10, 100, 210))
    l11 = ly.layer(11, 0)
    c1.shapes(l11).insert(RBA::Text::new("hello", RBA::Trans::new))
    assert_equal(pcell_decl.can_create_from_shape(ly, first_shape(ly.begin_shapes(c1.cell_index(), l11)), l10), false)
    assert_equal(pcell_decl.can_create_from_shape(ly, first_shape(ly.begin_shapes(c1.cell_index(), l10)), l10), true)
    assert_equal(pcell_decl.parameters_from_shape(ly, first_shape(ly.begin_shapes(c1.cell_index(), l10)), l10).inspect, "[<10/0>, 1.0, 2.0]")
    assert_equal(pcell_decl.transformation_from_shape(ly, first_shape(ly.begin_shapes(c1.cell_index(), l10)), l10).to_s, "r0 50,110")

    ly._destroy
    tl._destroy
    
  end

  def test_1a

    if !RBA.constants.member?(:PCellDeclarationHelper)
      return
    end

    # instantiate and register the library
    tl = PCellTestLib2::new

    ly = RBA::Layout::new(true)
    ly.dbu = 0.01

    ci1 = ly.add_cell("c1")
    c1 = ly.cell(ci1)

    lib = RBA::Library.library_by_name("PCellTestLib2")
    assert_equal(lib != nil, true)
    pcell_decl = lib.layout().pcell_declaration("Box2")

    param = [ RBA::LayerInfo::new(1, 0) ]  # rest is filled with defaults
    pcell_var_id = ly.add_pcell_variant(lib, pcell_decl.id(), param)
    pcell_var = ly.cell(pcell_var_id)
    pcell_inst = c1.insert(RBA::CellInstArray::new(pcell_var_id, RBA::Trans::new))
    assert_equal(pcell_var.basic_name, "Box2")
    assert_equal(pcell_var.name, "Box2_L1d0_W1p000_H1p000")
    assert_equal(pcell_var.qname, "PCellTestLib2.Box2")
    assert_equal(pcell_var.pcell_parameters().inspect, "[<1/0>, 1.0, 1.0, 0]")
    assert_equal(pcell_var.display_title(), "PCellTestLib2.Box2(L=1/0,W=1.000,H=1.000)")
    assert_equal(norm_hash(pcell_var.pcell_parameters_by_name()), "{\"height\"=>1.0, \"layer\"=><1/0>, \"secret\"=>0, \"width\"=>1.0}")
    assert_equal(pcell_var.pcell_parameter("height").inspect(), "1.0")
    assert_equal(c1.pcell_parameters(pcell_inst).inspect(), "[<1/0>, 1.0, 1.0, 0]")
    assert_equal(c1.pcell_parameter(pcell_inst, "secret"), 0)
    assert_equal(norm_hash(c1.pcell_parameters_by_name(pcell_inst)), "{\"height\"=>1.0, \"layer\"=><1/0>, \"secret\"=>0, \"width\"=>1.0}")
    assert_equal(c1.pcell_parameter(pcell_inst, "height").inspect(), "1.0")
    assert_equal(norm_hash(pcell_inst.pcell_parameters_by_name()), "{\"height\"=>1.0, \"layer\"=><1/0>, \"secret\"=>0, \"width\"=>1.0}")
    assert_equal(pcell_inst["height"].inspect(), "1.0")
    assert_equal(pcell_inst.pcell_parameter("height").inspect(), "1.0")
    assert_equal(pcell_var.pcell_declaration().inspect(), pcell_decl.inspect)
    assert_equal(c1.pcell_declaration(pcell_inst).inspect(), pcell_decl.inspect)
    assert_equal(pcell_inst.pcell_declaration().inspect(), pcell_decl.inspect)

    li1 = ly.layer(1, 0)
    assert_equal(li1 == nil, false)
    pcell_inst.change_pcell_parameter("height", 2.0)
    assert_equal(norm_hash(pcell_inst.pcell_parameters_by_name()), "{\"height\"=>2.0, \"layer\"=><1/0>, \"secret\"=>0, \"width\"=>1.0}")

    assert_equal(first_shape(ly.begin_shapes(c1.cell_index(), li1)).to_s, "box (-50,-100;50,100)")

    param = { "layer" => RBA::LayerInfo::new(2, 0), "width" => 2, "height" => 1 }
    li2 = ly.layer(2, 0)
    c1.change_pcell_parameters(pcell_inst, param)
    assert_equal(first_shape(ly.begin_shapes(c1.cell_index(), li2)).to_s, "box (-100,-50;100,50)")

    l10 = ly.layer(10, 0)
    c1.shapes(l10).insert(RBA::Box::new(0, 10, 100, 210))
    l11 = ly.layer(11, 0)
    c1.shapes(l11).insert(RBA::Text::new("hello", RBA::Trans::new))
    assert_equal(pcell_decl.can_create_from_shape(ly, first_shape(ly.begin_shapes(c1.cell_index(), l11)), l10), false)
    assert_equal(pcell_decl.can_create_from_shape(ly, first_shape(ly.begin_shapes(c1.cell_index(), l10)), l10), true)
    assert_equal(pcell_decl.parameters_from_shape(ly, first_shape(ly.begin_shapes(c1.cell_index(), l10)), l10).inspect, "[<10/0>, 1.0, 2.0, 0]")
    assert_equal(pcell_decl.transformation_from_shape(ly, first_shape(ly.begin_shapes(c1.cell_index(), l10)), l10).to_s, "r0 50,110")

    param2 = c1.pcell_parameters(pcell_inst)
    param2[3] = -2  # "secret"
    pcell_var_id2 = ly.add_pcell_variant(lib, pcell_decl.id(), param2)
    pcell_var2 = ly.cell(pcell_var_id2)
    pcell_inst2 = c1.insert(RBA::CellInstArray::new(pcell_var_id2, RBA::Trans::new))

    assert_equal(c1.pcell_parameter(pcell_inst2, "secret"), -2)  # coerce_parameters NOT called!
    assert_not_equal(pcell_inst.cell.library_cell_index, pcell_inst2.cell.library_cell_index)

    param = { "secret" => -1 }
    c1.change_pcell_parameters(pcell_inst, param)
    assert_equal(c1.pcell_parameter(pcell_inst, "secret"), -1)  # coerce_parameters NOT called!

    tl.refresh
    assert_equal(c1.pcell_parameter(pcell_inst2, "secret"), 0)  # coerce_parameters was called
    assert_equal(c1.pcell_parameter(pcell_inst, "secret"), 0)  # coerce_parameters was called

    # same variant now
    assert_equal(pcell_inst.cell.library_cell_index, pcell_inst2.cell.library_cell_index)

    param = { "secret" => 42 }
    c1.change_pcell_parameters(pcell_inst, param)
    assert_equal(c1.pcell_parameter(pcell_inst, "secret"), 42)
    assert_not_equal(pcell_inst.cell.library_cell_index, pcell_inst2.cell.library_cell_index)

    ly._destroy
    tl._destroy
    
  end

  def test_2

    # instantiate and register the library
    tl = PCellTestLib::new

    ly = RBA::Layout::new(true)
    ly.dbu = 0.01

    ci1 = ly.add_cell("c1")
    c1 = ly.cell(ci1)

    lib = RBA::Library::library_by_name("PCellTestLib")
    pcell_decl_id = lib.layout.pcell_id("Box")

    param = [ RBA::LayerInfo::new(1, 0), 10.0, 2.0 ]
    pcell_var_id = ly.add_pcell_variant(lib, pcell_decl_id, param)
    pcell_var = ly.cell(pcell_var_id)
    pcell_inst = c1.insert(RBA::CellInstArray::new(pcell_var_id, RBA::Trans::new))

    li1 = ly.layer_indices.find { |li| ly.get_info(li).to_s == "1/0" }
    assert_equal(li1 != nil, true)
    assert_equal(ly.is_valid_layer?(li1), true)
    assert_equal(ly.get_info(li1).to_s, "1/0")

    assert_equal(pcell_inst.is_pcell?, true)

    assert_equal(first_shape(ly.begin_shapes(pcell_inst.cell_index, li1)).to_s, "box (-500,-100;500,100)")
    pcell_inst.convert_to_static
    assert_equal(pcell_inst.is_pcell?, false)
    assert_equal(first_shape(ly.begin_shapes(pcell_inst.cell_index, li1)).to_s, "box (-500,-100;500,100)")
    pcell_inst.convert_to_static
    assert_equal(pcell_inst.is_pcell?, false)
    assert_equal(first_shape(ly.begin_shapes(pcell_inst.cell_index, li1)).to_s, "box (-500,-100;500,100)")

    ly._destroy
    tl._destroy

  end

  def test_3

    # instantiate and register the library
    tl = PCellTestLib::new

    ly = RBA::Layout::new(true)
    ly.dbu = 0.01

    c1 = ly.create_cell("c1")

    lib = RBA::Library::library_by_name("PCellTestLib")
    pcell_decl_id = lib.layout.pcell_id("Box")

    param = { "w" => 4.0, "h" => 8.0, "l" => RBA::LayerInfo::new(1, 0) }
    pcell_var_id = ly.add_pcell_variant(lib, pcell_decl_id, param)
    pcell_var = ly.cell(pcell_var_id)
    pcell_inst = c1.insert(RBA::CellInstArray::new(pcell_var_id, RBA::Trans::new))

    assert_equal(first_shape(ly.begin_shapes(c1.cell_index, ly.layer(1, 0))).to_s, "box (-200,-400;200,400)")
    
    ly._destroy
    tl._destroy

  end

  def test_4

    # instantiate and register the library
    tl = PCellTestLib::new

    lib = RBA::Library::library_by_name("PCellTestLib")
    pcell_decl_id = lib.layout.pcell_id("Box")

    param = { "w" => 4.0, "h" => 8.0, "l" => RBA::LayerInfo::new(1, 0) }
    pcell_var_id = lib.layout.add_pcell_variant(pcell_decl_id, param)

    assert_equal(first_shape(lib.layout.begin_shapes(pcell_var_id, lib.layout.layer(1, 0))).to_s, "box (-2000,-4000;2000,4000)")
    
    tl._destroy

  end

  def test_5

    # instantiate and register the library
    tl = PCellTestLib::new

    lib = RBA::Library::library_by_name("PCellTestLib")
    pcell_decl_id = lib.layout.pcell_id("Box")

    param = { "w" => 3.0, "h" => 7.0, "l" => RBA::LayerInfo::new(2, 0) }
    pcell_var_id = lib.layout.add_pcell_variant(pcell_decl_id, param)

    assert_equal(first_shape(lib.layout.begin_shapes(pcell_var_id, lib.layout.layer(2, 0))).to_s, "box (-1500,-3500;1500,3500)")

    tl._destroy

  end

  def test_6

    # instantiate and register the library
    tl = PCellTestLib::new

    lib = RBA::Library::library_by_name("PCellTestLib")

    param = { "w" => 3.0, "h" => 8.0, "l" => RBA::LayerInfo::new(3, 0) }
    pcell_var = lib.layout.create_cell("Box", param)

    assert_equal(first_shape(lib.layout.begin_shapes(pcell_var.cell_index, lib.layout.layer(3, 0))).to_s, "box (-1500,-4000;1500,4000)")
    
    tl._destroy

  end

  def test_7

    # instantiate and register the library
    tl = PCellTestLib::new

    ly = RBA::Layout::new(true)
    ly.dbu = 0.01

    param = { "w" => 4.0, "h" => 8.0, "l" => RBA::LayerInfo::new(4, 0) }
    cell = ly.create_cell("Box", "PCellTestLib", param)

    assert_equal(first_shape(ly.begin_shapes(cell, ly.layer(4, 0))).to_s, "box (-200,-400;200,400)")
    
    tl._destroy
    ly._destroy

  end

  def test_8

    # instantiate and register the library
    tl = PCellTestLib::new

    lib = RBA::Library::library_by_name("PCellTestLib")
    ly = RBA::Layout::new(true)
    ly.dbu = 0.01

    param = { "w" => 2.0, "h" => 6.0, "l" => RBA::LayerInfo::new(5, 0) }
    pcell_var = lib.layout.create_cell("Box", param)
    pcell_var.name = "BOXVAR"

    cell = ly.create_cell("BOXVAR", "PCellTestLib")

    assert_equal(first_shape(cell.begin_shapes_rec(ly.layer(5, 0))).to_s, "box (-100,-300;100,300)")

    tl._destroy
    ly._destroy

  end

  def test_9

    layout = RBA::Layout::new

    pcell = BoxPCell::new 
    assert_equal(pcell.layout == nil, true)
    
    # sets the layout reference of the PCell declaration:
    layout.register_pcell("Box", pcell)

    assert_equal(pcell.layout == nil, false)
    assert_equal(pcell.layout.object_id == layout.object_id, true)

    layout._destroy

  end

  # issue #1782

  class Circle1782 < RBA::PCellDeclarationHelper

    def initialize
      super()
      param("l", TypeLayer, "Layer", :default => RBA::LayerInfo::new(1, 0))
      param("r", TypeDouble, "Radius", :default => 1.0)
      param("n", TypeInt, "Number of points", :default => 16)     
    end

    def display_text_impl
      r = self.r
      if !r
        r = "nil"
      else
        r = '%.3f' % r
      end
      "Circle(L=" + self.l.to_s + ",R=" + r + ")"
    end
  
    def produce_impl
      r = self.r
      if self.r.to_s == 'NaN'
        r = 2.0
      end
      da = Math::PI * 2 / self.n
      pts = self.n.times.collect do |i|
        RBA::DPoint::new(r * Math::cos(i * da), r * Math::sin(i * da))
      end
      self.cell.shapes(self.l_layer).insert(RBA::DPolygon::new(pts))
    end

  end

  class CircleLib1782 < RBA::Library

    def initialize(name)
      self.description = "Circle Library"
      self.layout.register_pcell("Circle", Circle1782::new)
      register(name)
    end

    def reregister_pcell
      self.layout.register_pcell("Circle", Circle1782::new)
    end

  end

  def test_10

    lib = CircleLib1782::new("CircleLib")

    ly = RBA::Layout::new

    top = ly.create_cell("TOP")

    names = []

    2.times do |pass|
      
      5.times do |i|
        5.times do |j|
          if (i + j) % 2 == 0
            r = Float::NAN
          else
            r = (i + j) * 0.5
          end
          n = i * 5 + j
          c = ly.create_cell("Circle", "CircleLib", { "l" => RBA::LayerInfo::new(1, 0), "r" => r, "n" => n })
          if pass == 0
            names << c.name
          else
            # triggered bug #1782 - basically all variants are supposed to be unique, but 
            # the NaN spoiled the hash maps
            assert_equal(names.shift, c.name)
          end
          top.insert(RBA::DCellInstArray::new(c, RBA::DTrans::new(i * 10.0, j * 10.0)))
        end
      end

    end

    tmp = File::join($ut_testtmp, "tmp.gds")
    ly.write(tmp)

    # this should not throw an internal error
    ly._destroy

    # we should be able to read the Layout back
    ly = RBA::Layout::new
    ly.read(tmp)
    assert_equal(ly.top_cell.name, "TOP")
    assert_equal(ly.cells, 26)
    ly._destroy

    lib._destroy

  end

  def test_11

    lib = CircleLib1782::new("CircleLib")

    ly = RBA::Layout::new

    top = ly.create_cell("TOP")

    names = []

    c = ly.create_cell("Circle", "CircleLib", { "l" => RBA::LayerInfo::new(1, 0), "r" => 2.0, "n" => 64 })

    # triggered another flavor of #1782
    lib.reregister_pcell

    c = ly.create_cell("Circle", "CircleLib", { "l" => RBA::LayerInfo::new(1, 0), "r" => 2.0, "n" => 64 })
    top.insert(RBA::DCellInstArray::new(c, RBA::DTrans::new()))

    tmp = File::join($ut_testtmp, "tmp.gds")
    ly.write(tmp)

    # this should not throw an internal error
    ly._destroy

    # we should be able to read the Layout back
    ly = RBA::Layout::new
    ly.read(tmp)
    assert_equal(ly.top_cell.name, "TOP")
    assert_equal(ly.cells, 2)
    ly._destroy

    lib._destroy

  end

  def test_12

    if !RBA.constants.member?(:PCellDeclarationHelper)
      return
    end

    # instantiate and register the library
    tl = PCellTestLib3::new

    ly = RBA::Layout::new

    li1 = ly.find_layer("1/0")
    assert_equal(li1 == nil, true)

    c1 = ly.create_cell("c1")

    c2 = ly.create_cell("RecursivePCell", "PCellTestLib3", { "layer" => RBA::LayerInfo::new(1, 0), "level" => 4, "line" => RBA::Edge::new(0, 0, 20000, 0) })
    c1.insert(RBA::CellInstArray::new(c2.cell_index(), RBA::Trans::new))

    assert_equal(c2.display_title, "PCellTestLib3.RecursivePCell(L=1/0,E=(0,0;20,0),LVL=4")
    assert_equal(c1.dbbox.to_s, "(0,0;20,5.774)")

  end

  # convert to static cell
  def test_13

    if !RBA.constants.member?(:PCellDeclarationHelper)
      return
    end

    # instantiate and register the library
    tl = PCellTestLib2::new

    ly = RBA::Layout::new(true)
    ly.dbu = 0.01

    ci1 = ly.add_cell("c1")
    c1 = ly.cell(ci1)

    lib = RBA::Library.library_by_name("PCellTestLib2")
    assert_equal(lib != nil, true)
    pcell_decl = lib.layout().pcell_declaration("Box2")

    param = [ RBA::LayerInfo::new(1, 0) ]  # rest is filled with defaults
    pcell_var_id = ly.add_pcell_variant(lib, pcell_decl.id(), param)
    static_id = ly.convert_cell_to_static(pcell_var_id)
    static = ly.cell(static_id)
    assert_equal(static.basic_name, "Box2_L1d0_W1p000_H1p000")
    assert_equal(static.name, "Box2_L1d0_W1p000_H1p000")
    assert_equal(static.qname, "Box2_L1d0_W1p000_H1p000")
    assert_equal(static.is_proxy?, false)

  end

end

class DBPCellParameterStates_TestClass < TestBase

  def test_1

    ps = RBA::PCellParameterState::new

    ps.value = 17
    assert_equal(ps.value, 17)
    ps.value = "u"
    assert_equal(ps.value, "u")

    ps.visible = true
    assert_equal(ps.is_visible?, true)
    ps.visible = false
    assert_equal(ps.is_visible?, false)

    ps.enabled = true
    assert_equal(ps.is_enabled?, true)
    ps.enabled = false
    assert_equal(ps.is_enabled?, false)

    ps.readonly = true
    assert_equal(ps.is_readonly?, true)
    ps.readonly = false
    assert_equal(ps.is_readonly?, false)

    ps.tooltip = "uvw"
    assert_equal(ps.tooltip, "uvw")

    ps.icon = RBA::PCellParameterState::InfoIcon
    assert_equal(ps.icon, RBA::PCellParameterState::InfoIcon)

  end

end

load("test_epilogue.rb")

