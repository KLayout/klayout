# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2018 Matthias Koefferlein
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

class DBNetlistProperty_TestClass < TestBase

  def test_1_Basic

    np = RBA::NetlistProperty::new
    assert_equal(np.is_a?(RBA::NetlistProperty), true)
    assert_equal(np.to_s, "")

  end

  def test_2_NetName

    np = RBA::NetNameProperty::new
    assert_equal(np.is_a?(RBA::NetlistProperty), true)
    assert_equal(np.is_a?(RBA::NetNameProperty), true)
    assert_equal(np.to_s, "name:''")

    np.name = "abc"
    assert_equal(np.to_s, "name:abc")
    assert_equal(np.name, "abc")

  end

  def test_3_DevicePort

    np = RBA::DevicePortProperty::new
    assert_equal(np.is_a?(RBA::DevicePortProperty), true)
    assert_equal(np.is_a?(RBA::DevicePortProperty), true)
    assert_equal(np.to_s, "port")

    dc = RBA::GenericDeviceClass::new
    dp = RBA::DevicePortDefinition::new
    dp.name = "A"
    dc.add_port(dp)
    dp.name = "B"
    dc.add_port(dp)

    c = RBA::Circuit::new
    d = c.create_device(dc, "D")

    n = c.create_net("NET")
    d.connect_port(0, n)

    # there is no other way to produce a NetPortRef object yet
    n.each_port { |p| np.port_ref = p }

    assert_equal(np.to_s, "port:D:A")
    assert_equal(np.port_ref.device.name, "D")

  end

  def test_4_VariantBinding

    ly = RBA::Layout::new
    l1 = ly.insert_layer(RBA::LayerInfo::new(1, 0))
    tc = ly.create_cell("TOP")
    shapes = tc.shapes(l1)

    box = shapes.insert(RBA::Box::new(0, 0, 1000, 2000))

    # dry run
    box.set_property("a", 17)
    assert_equal(box.property("a"), 17)
    
    # set a NetNameProperty:
    box.set_property("nn", RBA::NetNameProperty::new("net42"))
    nn = box.property("nn")
    assert_equal(nn.is_a?(RBA::NetlistProperty), true)
    assert_equal(nn.is_a?(RBA::NetNameProperty), true)
    assert_equal(nn.name, "net42")

  end

end

load("test_epilogue.rb")
