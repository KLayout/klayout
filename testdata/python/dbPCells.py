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


import pya
import unittest
import math
import sys

class BoxPCell(pya.PCellDeclaration):

  def display_text(self, parameters):
    # provide a descriptive text for the cell
    return "Box(L=" + str(parameters[0]) + ",W=" + ('%.3f' % parameters[1]) + ",H=" + ('%.3f' % parameters[2]) + ")"
  
  def get_parameters(self):
  
    # prepare a set of parameter declarations
    param = []
    
    param.append(pya.PCellParameterDeclaration("l", pya.PCellParameterDeclaration.TypeLayer, "Layer", pya.LayerInfo(0, 0)))
    param.append(pya.PCellParameterDeclaration("w", pya.PCellParameterDeclaration.TypeDouble, "Width", 1.0))
    param.append(pya.PCellParameterDeclaration("h", pya.PCellParameterDeclaration.TypeDouble, "Height", 1.0))
    
    return param
    

  def get_layers(self, parameters):
    return [ parameters[0] ]
  
  def produce(self, layout, layers, parameters, cell):
  
    dbu = layout.dbu

    # fetch the parameters
    l = parameters[0]
    w = parameters[1] / layout.dbu
    h = parameters[2] / layout.dbu
    
    # create the shape
    cell.shapes(layers[0]).insert(pya.Box(-w / 2, -h / 2, w / 2, h / 2))

  def can_create_from_shape(self, layout, shape, layer):
    return shape.is_box()

  def transformation_from_shape(self, layout, shape, layer):
    return pya.Trans(shape.box.center() - pya.Point())

  def parameters_from_shape(self, layout, shape, layer):
    return [ layout.get_info(layer), shape.box.width() * layout.dbu, shape.box.height() * layout.dbu ]
    
class PCellTestLib(pya.Library):

  def __init__(self):  
  
    # set the description
    self.description = "PCell test lib"
    
    # create the PCell declarations
    self.layout().register_pcell("Box", BoxPCell())

    sb_index = self.layout().add_cell("StaticBox")
    l10 = self.layout().insert_layer(pya.LayerInfo(10, 0))
    sb_cell = self.layout().cell(sb_index)
    sb_cell.shapes(l10).insert(pya.Box(0, 0, 100, 200))
    
    # register us with the name "PCellTestLib"
    self.register("PCellTestLib")


if "PCellDeclarationHelper" in pya.__dict__:

  # A PCell based on the declaration helper

  class BoxPCell2(pya.PCellDeclarationHelper):

    def __init__(self):

      super(BoxPCell2, self).__init__()
    
      self.param("layer", self.TypeLayer, "Layer", default = pya.LayerInfo(0, 0))
      self.param("width", self.TypeDouble, "Width", default = 1.0)
      self.param("height", self.TypeDouble, "Height", default = 1.0)
      
    def display_text_impl(self):
      # provide a descriptive text for the cell
      return "Box2(L=" + str(self.layer) + ",W=" + ('%.3f' % self.width) + ",H=" + ('%.3f' % self.height) + ")"
    
    def wants_lazy_evaluation(self):
      return True

    def produce_impl(self):
    
      dbu = self.layout.dbu

      # fetch the parameters
      l = self.layer_layer
      w = self.width / self.layout.dbu
      h = self.height / self.layout.dbu
      
      # create the shape
      self.cell.shapes(l).insert(pya.Box(-w / 2, -h / 2, w / 2, h / 2))
      
    def can_create_from_shape_impl(self):
      return self.shape.is_box()

    def transformation_from_shape_impl(self):
      return pya.Trans(self.shape.box.center() - pya.Point())

    def parameters_from_shape_impl(self):
      self.layer = self.layout.get_info(self.layer)
      self.width = self.shape.box.width() * self.layout.dbu
      self.height = self.shape.box.height() * self.layout.dbu
      
  class PCellTestLib2(pya.Library):

    def __init__(self):  
    
      # set the description
      self.description = "PCell test lib2"
      
      # create the PCell declarations
      self.layout().register_pcell("Box2", BoxPCell2())

      # register us with the name "PCellTestLib2"
      self.register("PCellTestLib2")

  # A recursive PCell

  class RecursivePCell(pya.PCellDeclarationHelper):

    def __init__(self):

      super(RecursivePCell, self).__init__()
    
      self.param("layer", self.TypeLayer, "Layer", default = pya.LayerInfo(0, 0))
      self.param("line", self.TypeShape, "Line", default = pya.Edge(0, 0, 10000, 0))
      self.param("level", self.TypeInt, "Level", default = 1)
      
    def display_text_impl(self):
      # provide a descriptive text for the cell
      return "RecursivePCell(L=" + str(self.layer) + ",E=" + str(pya.CplxTrans(self.layout.dbu) * self.line) + ",LVL=" + str(self.level)
    
    def produce_impl(self):
    
      # fetch the parameters
      l = self.layer_layer
      e = self.line

      if self.level <= 0:
        self.cell.shapes(l).insert(e)
        return

      d3 = e.d() * (1.0 / 3.0)
      d3n = pya.Vector(-d3.y, d3.x)

      e1 = pya.Edge(e.p1, e.p1 + d3)
      e2 = pya.Edge(e1.p2, e1.p2 + d3 * 0.5 + d3n * math.cos(math.pi / 6))
      e3 = pya.Edge(e2.p2, e.p1 + d3 * 2.0)
      e4 = pya.Edge(e3.p2, e.p2)

      for e in [ e1, e2, e3, e4 ]:
        t = pya.Trans(e.p1 - pya.Point())
        cc = self.layout.create_cell("RecursivePCell", { "layer": self.layer, "line": t.inverted() * e, "level": self.level - 1 })
        self.cell.insert(pya.CellInstArray(cc, t))
      
  class PCellTestLib3(pya.Library):

    def __init__(self):  
    
      # set the description
      self.description = "PCell test lib3"
      
      # create the PCell declarations
      self.layout().register_pcell("RecursivePCell", RecursivePCell())

      # register us with the name "PCellTestLib3"
      self.register("PCellTestLib3")



def inspect_LayerInfo(self):
    return "<" + str(self) + ">"

pya.LayerInfo.__repr__ = inspect_LayerInfo

def find_layer(ly, lp):

  for li in ly.layer_indices():
    if str(ly.get_info(li)) == lp:
      return li
  return None


def nh(h):
  """
  Returns a normalized hash representation
  """
  v = []
  for k in sorted(h):
    v.append(repr(k) + ": " + repr(h[k]))
  return "{" + (", ".join(v)) + "}"

class DBPCellTests(unittest.TestCase):

  def test_0(self):

    # PCellDeclarationHelper is inside "pya.__all__"
    if hasattr(pya, "__all__"):
      self.assertEqual("PCellDeclarationHelper" in pya.__all__, True)

  def test_1(self):

    # instantiate and register the library
    tl = PCellTestLib()

    ly = pya.Layout(True)
    ly.dbu = 0.01

    li1 = find_layer(ly, "1/0")
    self.assertEqual(li1 == None, True)

    ci1 = ly.add_cell("c1")
    c1 = ly.cell(ci1)

    lib = pya.Library.library_by_name("NoLib")
    self.assertEqual(lib == None, True)
    lib = pya.Library.library_by_name("PCellTestLib")
    self.assertEqual(lib != None, True)
    pcell_decl = lib.layout().pcell_declaration("x")
    self.assertEqual(pcell_decl == None, True)
    pcell_decl = lib.layout().pcell_declaration("Box")
    self.assertEqual(pcell_decl != None, True)
    pcell_decl_id = lib.layout().pcell_id("Box")
    self.assertEqual(pcell_decl.id(), pcell_decl_id)
    self.assertEqual(":".join(lib.layout().pcell_names()), "Box")
    self.assertEqual(lib.layout().pcell_ids(), [ pcell_decl_id ])
    self.assertEqual(lib.layout().pcell_declaration(pcell_decl_id).id(), pcell_decl_id)

    param = [ pya.LayerInfo(1, 0) ]  # rest is filled with defaults
    pcell_var_id = ly.add_pcell_variant(lib, pcell_decl_id, param)
    pcell_var = ly.cell(pcell_var_id)
    pcell_inst = c1.insert(pya.CellInstArray(pcell_var_id, pya.Trans()))
    self.assertEqual(pcell_var.layout().__repr__(), ly.__repr__())
    self.assertEqual(pcell_var.library().__repr__(), lib.__repr__())
    self.assertEqual(pcell_var.is_pcell_variant(), True)
    self.assertEqual(pcell_var.display_title(), "PCellTestLib.Box(L=1/0,W=1.000,H=1.000)")
    self.assertEqual(pcell_var.basic_name(), "Box")
    self.assertEqual(pcell_var.pcell_declaration().wants_lazy_evaluation(), False)
    self.assertEqual(c1.is_pcell_variant(), False)
    self.assertEqual(c1.is_pcell_variant(pcell_inst), True)
    self.assertEqual(pcell_var.pcell_id(), pcell_decl_id)
    self.assertEqual(pcell_var.pcell_library().__repr__(), lib.__repr__())
    self.assertEqual(pcell_var.pcell_parameters().__repr__(), "[<1/0>, 1.0, 1.0]")
    self.assertEqual(nh(pcell_var.pcell_parameters_by_name()), "{'h': 1.0, 'l': <1/0>, 'w': 1.0}")
    self.assertEqual(pcell_var.pcell_parameter("h").__repr__(), "1.0")
    self.assertEqual(c1.pcell_parameters(pcell_inst).__repr__(), "[<1/0>, 1.0, 1.0]")
    self.assertEqual(nh(c1.pcell_parameters_by_name(pcell_inst)), "{'h': 1.0, 'l': <1/0>, 'w': 1.0}")
    self.assertEqual(c1.pcell_parameter(pcell_inst, "h").__repr__(), "1.0")
    self.assertEqual(nh(pcell_inst.pcell_parameters_by_name()), "{'h': 1.0, 'l': <1/0>, 'w': 1.0}")
    self.assertEqual(pcell_inst["h"].__repr__(), "1.0")
    self.assertEqual(pcell_inst["i"].__repr__(), "None")
    self.assertEqual(pcell_inst.pcell_parameter("h").__repr__(), "1.0")
    self.assertEqual(pcell_var.pcell_declaration().__repr__(), pcell_decl.__repr__())
    self.assertEqual(c1.pcell_declaration(pcell_inst).__repr__(), pcell_decl.__repr__())
    self.assertEqual(pcell_inst.pcell_declaration().__repr__(), pcell_decl.__repr__())

    pcell_inst.change_pcell_parameter("h", 2.0)
    self.assertEqual(nh(pcell_inst.pcell_parameters_by_name()), "{'h': 2.0, 'l': <1/0>, 'w': 1.0}")
    pcell_inst.set_property("abc", "a property")
    self.assertEqual(pcell_inst.property("abc").__repr__(), "'a property'")

    c1.clear()

    param = [ pya.LayerInfo(1, 0), 5.0, 10.0 ]
    pcell_var_id = ly.add_pcell_variant(lib, pcell_decl_id, param)
    pcell_var = ly.cell(pcell_var_id)
    pcell_inst = c1.insert(pya.CellInstArray(pcell_var_id, pya.Trans()))
    self.assertEqual(pcell_var.layout().__repr__(), ly.__repr__())
    self.assertEqual(pcell_var.library().__repr__(), lib.__repr__())
    self.assertEqual(pcell_var.is_pcell_variant(), True)
    self.assertEqual(pcell_var.display_title(), "PCellTestLib.Box(L=1/0,W=5.000,H=10.000)")
    self.assertEqual(pcell_var.basic_name(), "Box")
    self.assertEqual(c1.is_pcell_variant(), False)
    self.assertEqual(c1.is_pcell_variant(pcell_inst), True)
    self.assertEqual(pcell_var.pcell_id(), pcell_decl_id)
    self.assertEqual(pcell_var.pcell_library().__repr__(), lib.__repr__())
    self.assertEqual(pcell_var.pcell_parameters().__repr__(), "[<1/0>, 5.0, 10.0]")
    self.assertEqual(c1.pcell_parameters(pcell_inst).__repr__(), "[<1/0>, 5.0, 10.0]")
    self.assertEqual(pcell_inst.pcell_parameters().__repr__(), "[<1/0>, 5.0, 10.0]")
    self.assertEqual(pcell_var.pcell_declaration().__repr__(), pcell_decl.__repr__())
    self.assertEqual(c1.pcell_declaration(pcell_inst).__repr__(), pcell_decl.__repr__())

    li1 = find_layer(ly, "1/0")
    self.assertEqual(li1 != None, True)
    self.assertEqual(ly.is_valid_layer(li1), True)
    self.assertEqual(str(ly.get_info(li1)), "1/0")

    lib_proxy_id = ly.add_lib_cell(lib, lib.layout().cell_by_name("StaticBox"))
    lib_proxy = ly.cell(lib_proxy_id)
    self.assertEqual(lib_proxy.display_title(), "PCellTestLib.StaticBox")
    self.assertEqual(lib_proxy.basic_name(), "StaticBox")
    self.assertEqual(lib_proxy.layout().__repr__(), ly.__repr__())
    self.assertEqual(lib_proxy.library().__repr__(), lib.__repr__())
    self.assertEqual(lib_proxy.is_pcell_variant(), False)
    self.assertEqual(lib.layout().cell(lib.layout().cell_by_name("StaticBox")).library().__repr__(), "None")

    li2 = find_layer(ly, "10/0")
    self.assertEqual(li2 != None, True)

    self.assertEqual(ly.begin_shapes(c1.cell_index(), li1).shape().__str__(), "box (-250,-500;250,500)")
    self.assertEqual(ly.begin_shapes(lib_proxy.cell_index(), li2).shape().__str__(), "box (0,0;10,20)")

    param = { "w": 1, "h": 2 }
    c1.change_pcell_parameters(pcell_inst, param)
    self.assertEqual(ly.begin_shapes(c1.cell_index(), li1).shape().__str__(), "box (-50,-100;50,100)")
    
    param = [ pya.LayerInfo(1, 0), 5.0, 5.0 ]
    c1.change_pcell_parameters(pcell_inst, param)
    self.assertEqual(ly.begin_shapes(c1.cell_index(), li1).shape().__str__(), "box (-250,-250;250,250)")
    
    pcell_inst.change_pcell_parameters({ "w": 2.0, "h": 10.0 })
    self.assertEqual(ly.begin_shapes(c1.cell_index(), li1).shape().__str__(), "box (-100,-500;100,500)")

    pcell_inst.change_pcell_parameters([ pya.LayerInfo(1, 0), 5.0, 5.0 ])
    self.assertEqual(ly.begin_shapes(c1.cell_index(), li1).shape().__str__(), "box (-250,-250;250,250)")

    pcell_inst.change_pcell_parameter("w", 5.0)
    pcell_inst.change_pcell_parameter("h", 1.0)
    self.assertEqual(ly.begin_shapes(c1.cell_index(), li1).shape().__str__(), "box (-250,-50;250,50)")

    c1.change_pcell_parameter(pcell_inst, "w", 10.0)
    c1.change_pcell_parameter(pcell_inst, "h", 2.0)
    self.assertEqual(ly.begin_shapes(c1.cell_index(), li1).shape().__str__(), "box (-500,-100;500,100)")

    self.assertEqual(ly.cell(pcell_inst.cell_index).is_pcell_variant(), True)
    self.assertEqual(pcell_inst.is_pcell(), True)
    new_id = ly.convert_cell_to_static(pcell_inst.cell_index)
    self.assertEqual(new_id == pcell_inst.cell_index, False)
    self.assertEqual(ly.cell(new_id).is_pcell_variant(), False)
    param = [ pya.LayerInfo(1, 0), 5.0, 5.0 ]
    c1.change_pcell_parameters(pcell_inst, param)
    self.assertEqual(ly.begin_shapes(c1.cell_index(), li1).shape().__str__(), "box (-250,-250;250,250)")
    pcell_inst.cell_index = new_id
    self.assertEqual(ly.begin_shapes(c1.cell_index(), li1).shape().__str__(), "box (-500,-100;500,100)")

    l10 = ly.layer(10, 0)
    c1.shapes(l10).insert(pya.Box(0, 10, 100, 210))
    l11 = ly.layer(11, 0)
    c1.shapes(l11).insert(pya.Text("hello", pya.Trans()))
    self.assertEqual(pcell_decl.can_create_from_shape(ly, ly.begin_shapes(c1.cell_index(), l11).shape(), l10), False)
    self.assertEqual(pcell_decl.can_create_from_shape(ly, ly.begin_shapes(c1.cell_index(), l10).shape(), l10), True)
    self.assertEqual(repr(pcell_decl.parameters_from_shape(ly, ly.begin_shapes(c1.cell_index(), l10).shape(), l10)), "[<10/0>, 1.0, 2.0]")
    self.assertEqual(str(pcell_decl.transformation_from_shape(ly, ly.begin_shapes(c1.cell_index(), l10).shape(), l10)), "r0 50,110")


  def test_1a(self):

    if not "PCellDeclarationHelper" in pya.__dict__:
      return

    # instantiate and register the library
    tl = PCellTestLib2()

    ly = pya.Layout(True)
    ly.dbu = 0.01

    li1 = find_layer(ly, "1/0")
    self.assertEqual(li1 == None, True)

    ci1 = ly.add_cell("c1")
    c1 = ly.cell(ci1)

    lib = pya.Library.library_by_name("PCellTestLib2")
    self.assertEqual(lib != None, True)
    pcell_decl = lib.layout().pcell_declaration("Box2")

    param = [ pya.LayerInfo(1, 0) ]  # rest is filled with defaults
    pcell_var_id = ly.add_pcell_variant(lib, pcell_decl.id(), param)
    pcell_var = ly.cell(pcell_var_id)
    pcell_inst = c1.insert(pya.CellInstArray(pcell_var_id, pya.Trans()))
    self.assertEqual(pcell_var.basic_name(), "Box2")
    self.assertEqual(pcell_var.pcell_parameters().__repr__(), "[<1/0>, 1.0, 1.0]")
    self.assertEqual(pcell_var.display_title(), "PCellTestLib2.Box2(L=1/0,W=1.000,H=1.000)")
    self.assertEqual(nh(pcell_var.pcell_parameters_by_name()), "{'height': 1.0, 'layer': <1/0>, 'width': 1.0}")
    self.assertEqual(pcell_var.pcell_parameter("height").__repr__(), "1.0")
    self.assertEqual(c1.pcell_parameters(pcell_inst).__repr__(), "[<1/0>, 1.0, 1.0]")
    self.assertEqual(nh(c1.pcell_parameters_by_name(pcell_inst)), "{'height': 1.0, 'layer': <1/0>, 'width': 1.0}")
    self.assertEqual(c1.pcell_parameter(pcell_inst, "height").__repr__(), "1.0")
    self.assertEqual(nh(pcell_inst.pcell_parameters_by_name()), "{'height': 1.0, 'layer': <1/0>, 'width': 1.0}")
    self.assertEqual(pcell_inst["height"].__repr__(), "1.0")
    self.assertEqual(pcell_inst.pcell_parameter("height").__repr__(), "1.0")
    self.assertEqual(pcell_var.pcell_declaration().__repr__(), pcell_decl.__repr__())
    self.assertEqual(c1.pcell_declaration(pcell_inst).__repr__(), pcell_decl.__repr__())
    self.assertEqual(pcell_inst.pcell_declaration().__repr__(), pcell_decl.__repr__())
    self.assertEqual(pcell_decl.wants_lazy_evaluation(), True)

    li1 = find_layer(ly, "1/0")
    self.assertEqual(li1 == None, False)
    pcell_inst.change_pcell_parameter("height", 2.0)
    self.assertEqual(nh(pcell_inst.pcell_parameters_by_name()), "{'height': 2.0, 'layer': <1/0>, 'width': 1.0}")

    self.assertEqual(ly.begin_shapes(c1.cell_index(), li1).shape().__str__(), "box (-50,-100;50,100)")

    param = { "layer": pya.LayerInfo(2, 0), "width": 2, "height": 1 }
    li2 = ly.layer(2, 0)
    c1.change_pcell_parameters(pcell_inst, param)
    self.assertEqual(ly.begin_shapes(c1.cell_index(), li2).shape().__str__(), "box (-100,-50;100,50)")

    l10 = ly.layer(10, 0)
    c1.shapes(l10).insert(pya.Box(0, 10, 100, 210))
    l11 = ly.layer(11, 0)
    c1.shapes(l11).insert(pya.Text("hello", pya.Trans()))
    self.assertEqual(pcell_decl.can_create_from_shape(ly, ly.begin_shapes(c1.cell_index(), l11).shape(), l10), False)
    self.assertEqual(pcell_decl.can_create_from_shape(ly, ly.begin_shapes(c1.cell_index(), l10).shape(), l10), True)
    self.assertEqual(repr(pcell_decl.parameters_from_shape(ly, ly.begin_shapes(c1.cell_index(), l10).shape(), l10)), "[<10/0>, 1.0, 2.0]")
    self.assertEqual(str(pcell_decl.transformation_from_shape(ly, ly.begin_shapes(c1.cell_index(), l10).shape(), l10)), "r0 50,110")


  def test_2(self):

    # instantiate and register the library
    tl = PCellTestLib()

    ly = pya.Layout(True)
    ly.dbu = 0.01

    ci1 = ly.add_cell("c1")
    c1 = ly.cell(ci1)

    lib = pya.Library.library_by_name("PCellTestLib")
    pcell_decl_id = lib.layout().pcell_id("Box")

    param = [ pya.LayerInfo(1, 0), 10.0, 2.0 ]
    pcell_var_id = ly.add_pcell_variant(lib, pcell_decl_id, param)
    pcell_var = ly.cell(pcell_var_id)
    pcell_inst = c1.insert(pya.CellInstArray(pcell_var_id, pya.Trans()))

    li1 = find_layer(ly, "1/0")
    self.assertEqual(li1 != None, True)
    self.assertEqual(ly.is_valid_layer(li1), True)
    self.assertEqual(str(ly.get_info(li1)), "1/0")

    self.assertEqual(pcell_inst.is_pcell(), True)

    self.assertEqual(ly.begin_shapes(pcell_inst.cell_index, li1).shape().__str__(), "box (-500,-100;500,100)")
    pcell_inst.convert_to_static()
    self.assertEqual(pcell_inst.is_pcell(), False)
    self.assertEqual(ly.begin_shapes(pcell_inst.cell_index, li1).shape().__str__(), "box (-500,-100;500,100)")
    pcell_inst.convert_to_static()
    self.assertEqual(pcell_inst.is_pcell(), False)
    self.assertEqual(ly.begin_shapes(pcell_inst.cell_index, li1).shape().__str__(), "box (-500,-100;500,100)")

  def test_3(self):

    # instantiate and register the library
    tl = PCellTestLib()

    ly = pya.Layout(True)
    ly.dbu = 0.01

    c1 = ly.create_cell("c1")

    lib = pya.Library.library_by_name("PCellTestLib")
    pcell_decl_id = lib.layout().pcell_id("Box")

    param = { "w": 4.0, "h": 8.0, "l": pya.LayerInfo(1, 0) }
    pcell_var_id = ly.add_pcell_variant(lib, pcell_decl_id, param)
    pcell_var = ly.cell(pcell_var_id)
    pcell_inst = c1.insert(pya.CellInstArray(pcell_var_id, pya.Trans()))

    self.assertEqual(ly.begin_shapes(c1.cell_index(), ly.layer(1, 0)).shape().__str__(), "box (-200,-400;200,400)")
      
  def test_4(self):

    # instantiate and register the library
    tl = PCellTestLib()

    lib = pya.Library.library_by_name("PCellTestLib")
    pcell_decl_id = lib.layout().pcell_id("Box")

    param = { "w": 4.0, "h": 8.0, "l": pya.LayerInfo(1, 0) }
    pcell_var_id = lib.layout().add_pcell_variant(pcell_decl_id, param)

    self.assertEqual(lib.layout().begin_shapes(pcell_var_id, lib.layout().layer(1, 0)).shape().__str__(), "box (-2000,-4000;2000,4000)")
    
  def test_5(self):

    # instantiate and register the library
    tl = PCellTestLib()

    lib = pya.Library.library_by_name("PCellTestLib")
    pcell_decl_id = lib.layout().pcell_id("Box")

    param = { "w": 3.0, "h": 7.0, "l": pya.LayerInfo(2, 0) }
    pcell_var_id = lib.layout().add_pcell_variant(pcell_decl_id, param)

    self.assertEqual(lib.layout().begin_shapes(pcell_var_id, lib.layout().layer(2, 0)).shape().__str__(), "box (-1500,-3500;1500,3500)")
    

  def test_6(self):

    # instantiate and register the library
    tl = PCellTestLib()

    lib = pya.Library.library_by_name("PCellTestLib")

    param = { "w": 3.0, "h": 8.0, "l": pya.LayerInfo(3, 0) }
    pcell_var = lib.layout().create_cell("Box", param)

    self.assertEqual(lib.layout().begin_shapes(pcell_var.cell_index(), lib.layout().layer(3, 0)).shape().__str__(), "box (-1500,-4000;1500,4000)")
    

  def test_7(self):

    # instantiate and register the library
    tl = PCellTestLib()

    ly = pya.Layout(True)
    ly.dbu = 0.01

    param = { "w": 4.0, "h": 8.0, "l": pya.LayerInfo(4, 0) }
    cell = ly.create_cell("Box", "PCellTestLib", param)

    self.assertEqual(ly.begin_shapes(cell, ly.layer(4, 0)).shape().__str__(), "box (-200,-400;200,400)")

  def test_8(self):

    # instantiate and register the library
    tl = PCellTestLib()

    lib = pya.Library.library_by_name("PCellTestLib")
    ly = pya.Layout(True)
    ly.dbu = 0.01

    param = { "w": 2.0, "h": 6.0, "l": pya.LayerInfo(5, 0) }
    pcell_var = lib.layout().create_cell("Box", param)
    pcell_var.name = "BOXVAR"

    cell = ly.create_cell("BOXVAR", "PCellTestLib")

    self.assertEqual(cell.begin_shapes_rec(ly.layer(5, 0)).shape().__str__(), "box (-100,-300;100,300)")

  def test_9(self):

    if not "PCellDeclarationHelper" in pya.__dict__:
      return

    # instantiate and register the library
    tl = PCellTestLib3()

    ly = pya.Layout(True)

    li1 = find_layer(ly, "1/0")
    self.assertEqual(li1 == None, True)

    c1 = ly.create_cell("c1")

    c2 = ly.create_cell("RecursivePCell", "PCellTestLib3", { "layer": pya.LayerInfo(1, 0), "level": 4, "line": pya.Edge(0, 0, 20000, 0) })
    c1.insert(pya.CellInstArray(c2.cell_index(), pya.Trans()))

    self.assertEqual(c2.display_title(), "PCellTestLib3.RecursivePCell(L=1/0,E=(0,0;20,0),LVL=4")
    self.assertEqual(str(c1.dbbox()), "(0,0;20,5.774)")

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBPCellTests)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)

