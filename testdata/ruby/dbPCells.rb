
$:.push(File::dirname($0))

load("test_prologue.rb")

class BoxPCell < RBA::PCellDeclaration

  def display_text(parameters)
    # provide a descriptive text for the cell
    return "Box(L=#{parameters[0].to_s},W=#{'%.3f' % parameters[1].to_s},H=#{'%.3f' % parameters[2].to_s})"
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
    

class DBPCell_TestClass < TestBase

  def test_1

    # instantiate and register the library
    tl = PCellTestLib::new

    begin

      ly = RBA::Layout::new(true)
      ly.dbu = 0.01

      li1 = ly.layer_indices.find { |li| ly.get_info(li).to_s == "1/0" }
      assert_equal(li1 == nil, true)

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

      assert_equal(ly.begin_shapes(c1.cell_index, li1).shape.to_s, "box (-250,-500;250,500)")
      assert_equal(ly.begin_shapes(lib_proxy.cell_index, li2).shape.to_s, "box (0,0;10,20)")

      param = { "w" => 1, "h" => 2 }
      c1.change_pcell_parameters(pcell_inst, param)
      assert_equal(ly.begin_shapes(c1.cell_index, li1).shape.to_s, "box (-50,-100;50,100)")
      
      param = [ RBA::LayerInfo::new(1, 0), 5.0, 5.0 ]
      c1.change_pcell_parameters(pcell_inst, param)
      assert_equal(ly.begin_shapes(c1.cell_index, li1).shape.to_s, "box (-250,-250;250,250)")
      
      pcell_inst.change_pcell_parameters({ "w" => 2.0, "h" => 10.0 })
      assert_equal(ly.begin_shapes(c1.cell_index, li1).shape.to_s, "box (-100,-500;100,500)")

      pcell_inst.change_pcell_parameters([ RBA::LayerInfo::new(1, 0), 5.0, 5.0 ])
      assert_equal(ly.begin_shapes(c1.cell_index, li1).shape.to_s, "box (-250,-250;250,250)")

      pcell_inst.change_pcell_parameter("w", 5.0)
      pcell_inst.change_pcell_parameter("h", 1.0)
      assert_equal(ly.begin_shapes(c1.cell_index, li1).shape.to_s, "box (-250,-50;250,50)")

      c1.change_pcell_parameter(pcell_inst, "w", 10.0)
      c1.change_pcell_parameter(pcell_inst, "h", 2.0)
      assert_equal(ly.begin_shapes(c1.cell_index, li1).shape.to_s, "box (-500,-100;500,100)")

      assert_equal(ly.cell(pcell_inst.cell_index).is_pcell_variant?, true)
      assert_equal(pcell_inst.is_pcell?, true)
      new_id = ly.convert_cell_to_static(pcell_inst.cell_index)
      assert_equal(new_id == pcell_inst.cell_index, false)
      assert_equal(ly.cell(new_id).is_pcell_variant?, false)
      param = [ RBA::LayerInfo::new(1, 0), 5.0, 5.0 ]
      c1.change_pcell_parameters(pcell_inst, param)
      assert_equal(ly.begin_shapes(c1.cell_index, li1).shape.to_s, "box (-250,-250;250,250)")
      pcell_inst.cell_index = new_id
      assert_equal(ly.begin_shapes(c1.cell_index, li1).shape.to_s, "box (-500,-100;500,100)")

#ly.destroy

    ensure
#tl.delete
    end
    
  end

  def test_2

    # instantiate and register the library
    tl = PCellTestLib::new

    begin

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

      assert_equal(ly.begin_shapes(pcell_inst.cell_index, li1).shape.to_s, "box (-500,-100;500,100)")
      pcell_inst.convert_to_static
      assert_equal(pcell_inst.is_pcell?, false)
      assert_equal(ly.begin_shapes(pcell_inst.cell_index, li1).shape.to_s, "box (-500,-100;500,100)")
      pcell_inst.convert_to_static
      assert_equal(pcell_inst.is_pcell?, false)
      assert_equal(ly.begin_shapes(pcell_inst.cell_index, li1).shape.to_s, "box (-500,-100;500,100)")

#ly.destroy
    
    ensure
#tl.delete
    end
    
  end

  def test_3

    # instantiate and register the library
    tl = PCellTestLib::new

    begin

      ly = RBA::Layout::new(true)
      ly.dbu = 0.01

      c1 = ly.create_cell("c1")

      lib = RBA::Library::library_by_name("PCellTestLib")
      pcell_decl_id = lib.layout.pcell_id("Box")

      param = { "w" => 4.0, "h" => 8.0, "l" => RBA::LayerInfo::new(1, 0) }
      pcell_var_id = ly.add_pcell_variant(lib, pcell_decl_id, param)
      pcell_var = ly.cell(pcell_var_id)
      pcell_inst = c1.insert(RBA::CellInstArray::new(pcell_var_id, RBA::Trans::new))

      assert_equal(ly.begin_shapes(c1.cell_index, ly.layer(1, 0)).shape.to_s, "box (-200,-400;200,400)")
      
    ensure
#tl.delete
    end
    
  end

  def test_4

    # instantiate and register the library
    tl = PCellTestLib::new

    begin

      lib = RBA::Library::library_by_name("PCellTestLib")
      pcell_decl_id = lib.layout.pcell_id("Box")

      param = { "w" => 4.0, "h" => 8.0, "l" => RBA::LayerInfo::new(1, 0) }
      pcell_var_id = lib.layout.add_pcell_variant(pcell_decl_id, param)

      assert_equal(lib.layout.begin_shapes(pcell_var_id, lib.layout.layer(1, 0)).shape.to_s, "box (-2000,-4000;2000,4000)")
    
    ensure
#tl.delete
    end
    
  end

  def test_5

    # instantiate and register the library
    tl = PCellTestLib::new

    begin

      lib = RBA::Library::library_by_name("PCellTestLib")
      pcell_decl_id = lib.layout.pcell_id("Box")

      param = { "w" => 3.0, "h" => 7.0, "l" => RBA::LayerInfo::new(2, 0) }
      pcell_var_id = lib.layout.add_pcell_variant(pcell_decl_id, param)

      assert_equal(lib.layout.begin_shapes(pcell_var_id, lib.layout.layer(2, 0)).shape.to_s, "box (-1500,-3500;1500,3500)")
    
    ensure
#tl.delete
    end

  end

  def test_6

    # instantiate and register the library
    tl = PCellTestLib::new

    begin

      lib = RBA::Library::library_by_name("PCellTestLib")

      param = { "w" => 3.0, "h" => 8.0, "l" => RBA::LayerInfo::new(3, 0) }
      pcell_var = lib.layout.create_cell("Box", param)

      assert_equal(lib.layout.begin_shapes(pcell_var.cell_index, lib.layout.layer(3, 0)).shape.to_s, "box (-1500,-4000;1500,4000)")
    
    ensure
#tl.delete
    end

  end

  def test_7

    # instantiate and register the library
    tl = PCellTestLib::new

    begin

      ly = RBA::Layout::new(true)
      ly.dbu = 0.01

      param = { "w" => 4.0, "h" => 8.0, "l" => RBA::LayerInfo::new(4, 0) }
      cell = ly.create_cell("Box", "PCellTestLib", param)

      assert_equal(ly.begin_shapes(cell, ly.layer(4, 0)).shape.to_s, "box (-200,-400;200,400)")
    
    ensure
#tl.delete
    end

  end

  def test_8

    # instantiate and register the library
    tl = PCellTestLib::new

    begin

      lib = RBA::Library::library_by_name("PCellTestLib")
      ly = RBA::Layout::new(true)
      ly.dbu = 0.01

      param = { "w" => 2.0, "h" => 6.0, "l" => RBA::LayerInfo::new(5, 0) }
      pcell_var = lib.layout.create_cell("Box", param)
      pcell_var.name = "BOXVAR"

      cell = ly.create_cell("BOXVAR", "PCellTestLib")

      assert_equal(cell.begin_shapes_rec(ly.layer(5, 0)).shape.to_s, "box (-100,-300;100,300)")
    
    ensure
#tl.delete
    end

  end

end

load("test_epilogue.rb")

