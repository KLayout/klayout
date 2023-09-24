# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2023 Matthias Koefferlein
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

class DBNetlist_TestClass < TestBase

  def test_0_NetlistObject

    nlo = RBA::NetlistObject::new
    assert_equal(nlo.property(17), nil)
    assert_equal(nlo.property_keys.inspect, "[]")
    nlo.set_property(17, 42)
    assert_equal(nlo.property_keys.inspect, "[17]")
    assert_equal(nlo.property(17), 42)

    nlo2 = nlo.dup
    assert_equal(nlo2.property(17), 42)
    nlo.set_property(17, nil)
    assert_equal(nlo.property_keys.inspect, "[]")
    assert_equal(nlo.property(17), nil)
    assert_equal(nlo2.property(17), 42)

  end

  def test_1_NetlistBasicCircuit

    nl = RBA::Netlist::new
    c = RBA::Circuit::new
    nl.add(c)

    c.name = "XYZ"
    assert_equal(c.name, "XYZ")

    c.boundary = RBA::DPolygon::new(RBA::DBox::new(0, 1, 2, 3))
    assert_equal(c.boundary.to_s, "(0,1;0,3;2,3;2,1)")

    c.cell_index = 42
    assert_equal(c.cell_index, 42)

    assert_equal(nl.circuit_by_cell_index(42).name, "XYZ")
    assert_equal(nl.circuit_by_name("XYZ").name, "XYZ")
    assert_equal(nl.circuit_by_cell_index(17).inspect, "nil")
    assert_equal(nl.circuit_by_name("DOESNOTEXIST").inspect, "nil")

    assert_equal(nl.is_case_sensitive?, true)
    assert_equal(nl.circuit_by_name("xyz").inspect, "nil")
    nl.case_sensitive = false
    assert_equal(nl.is_case_sensitive?, false)
    assert_equal(nl.circuit_by_name("xyz").name, "XYZ")
    nl.case_sensitive = true

    cc = RBA::Circuit::new
    assert_equal(cc.dont_purge, false)
    cc.dont_purge = true
    assert_equal(cc.dont_purge, true)

    begin
      nl.remove(cc) # not in netlist yet
      assert_equal(false, true)
    rescue
    end

    nl.add(cc)
    cc.name = "ABC"

    begin
      nl.add(cc) # already in netlist
      assert_equal(false, true)
    rescue
    end

    names = []
    nl.each_circuit { |i| names << i.name }
    assert_equal(names, [ c.name, cc.name ])

    # NOTE: this will also remove the circuit from the netlist
    cc._destroy

    names = []
    nl.each_circuit { |i| names << i.name }
    assert_equal(names, [ c.name ])

    cc = RBA::Circuit::new
    nl.add(cc)
    cc.name = "UVW"

    names = []
    nl.each_circuit { |i| names << i.name }
    assert_equal(names, [ c.name, cc.name ])

    assert_equal(nl.circuits_by_name("X*").collect { |x| x.name }, [ "XYZ" ])
    assert_equal(nl.circuits_by_name("x*").collect { |x| x.name }, [])
    nl.case_sensitive = false
    assert_equal(nl.circuits_by_name("X*").collect { |x| x.name }, [ "XYZ" ])
    assert_equal(nl.circuits_by_name("x*").collect { |x| x.name }, [ "XYZ" ])
    nl.case_sensitive = true
    assert_equal(nl.circuits_by_name("???").collect { |x| x.name }, [ "XYZ", "UVW" ])
    assert_equal(nl.circuits_by_name("*").collect { |x| x.name }, [ "XYZ", "UVW" ])
    assert_equal(nl.circuits_by_name("P*").collect { |x| x.name }, [])
    assert_equal(nl.circuits_by_name("x*").collect { |x| x.name }, [])
    assert_equal(nl.circuits_by_name("").collect { |x| x.name }, [])

    # same as _destroy
    nl.remove(c)

    names = []
    nl.each_circuit { |i| names << i.name }
    assert_equal(names, [ cc.name ])

    nl._destroy
    assert_equal(cc._destroyed?, true)

  end

  def test_2_NetlistBasicDeviceClass

    nl = RBA::Netlist::new
    c = RBA::DeviceClass::new
    nl.add(c)

    c.name = "XYZ"
    assert_equal(c.name, "XYZ")

    cc = RBA::DeviceClass::new

    begin
      nl.remove(cc) # not in netlist yet
      assert_equal(false, true)
    rescue
    end

    nl.add(cc)
    cc.name = "ABC"

    begin
      nl.add(cc) # already in netlist
      assert_equal(false, true)
    rescue
    end

    names = []
    nl.each_device_class { |i| names << i.name }
    assert_equal(names, [ c.name, cc.name ])

    # NOTE: this will also remove the circuit from the netlist
    cc._destroy

    names = []
    nl.each_device_class { |i| names << i.name }
    assert_equal(names, [ c.name ])

    cc = RBA::DeviceClass::new
    nl.add(cc)
    cc.name = "UVW"

    assert_equal(nl.device_class_by_name("UVW") == cc, true)
    assert_equal(nl.device_class_by_name("doesntexist") == nil, true)

    assert_equal(cc.strict?, false)
    cc.strict = true
    assert_equal(cc.strict?, true)

    names = []
    nl.each_device_class { |i| names << i.name }
    assert_equal(names, [ c.name, cc.name ])

    # same as _destroy
    nl.remove(c)

    names = []
    nl.each_device_class { |i| names << i.name }
    assert_equal(names, [ cc.name ])

    nl._destroy
    assert_equal(cc._destroyed?, true)

  end

  def test_3_Pin

    c = RBA::Circuit::new
    p1 = c.create_pin("X")
    p2 = c.create_pin("B")
    assert_equal(p1.name, "X")
    assert_equal(p2.name, "B")

    assert_equal(p1.property(17), nil)
    p1.set_property(17, 42)
    assert_equal(p1.property(17), 42)

    assert_equal(p1.id, 0)
    assert_equal(p2.id, 1)

    names = []
    c.each_pin { |p| names << p.name }
    assert_equal(names, [ "X", "B" ])
    assert_equal(c.pin_by_name("A") == nil, true)
    assert_equal(c.pin_by_name("X") != nil, true)
    assert_equal(c.pin_by_name("X").id, 0)

    # modification of pin name
    c.rename_pin(p1.id, "A")
    assert_equal(p1.name, "A")
    assert_equal(p2.name, "B")
    names = []
    c.each_pin { |p| names << p.name }
    assert_equal(names, [ "A", "B" ])
    assert_equal(c.pin_by_name("X") == nil, true)
    assert_equal(c.pin_by_name("A") != nil, true)
    assert_equal(c.pin_by_name("A").id, 0)

    assert_equal(c.pin_by_id(0) == nil, false)
    assert_equal(c.pin_by_id(1) == nil, false)
    assert_equal(c.pin_by_id(0).name, "A")
    assert_equal(c.pin_by_id(1).name, "B")

    c.remove_pin(0)

    names = []
    c.each_pin { |p| names << p.name }
    assert_equal(names, [ "B" ])

    assert_equal(c.pin_by_id(0) == nil, true)
    assert_equal(c.pin_by_id(1) == nil, false)
    assert_equal(c.pin_by_id(1).name, "B")

  end

  def test_4_Device

    nl = RBA::Netlist::new

    dc = RBA::DeviceClass::new
    nl.add(dc)
    assert_equal(dc.netlist.object_id, nl.object_id)
    dc.name = "DC"
    dc.description = "A device class"

    pd = RBA::DeviceTerminalDefinition::new
    pd.name = "A"
    pd.description = "Terminal A"
    dc.add_terminal(pd)

    pd = RBA::DeviceTerminalDefinition::new
    pd.name = "B"
    pd.description = "Terminal B"
    dc.add_terminal(pd)

    assert_equal(dc.has_terminal?("X"), false)
    assert_equal(dc.has_terminal?("A"), true)
    assert_equal(dc.terminal_id("A"), 0)
    assert_equal(dc.terminal_id("B"), 1)

    c = RBA::Circuit::new
    nl.add(c)

    d1 = c.create_device(dc)
    d1.name = "D1"
    assert_equal(d1.name, "D1")
    assert_equal(d1.id, 1)
    assert_equal(c.device_by_id(d1.id).id, 1)
    assert_equal(c.device_by_id(2).inspect, "nil")
    assert_equal(c.device_by_name(d1.name).name, "D1")
    assert_equal(c.device_by_id(2).inspect, "nil")
    assert_equal(c.device_by_name("doesnt_exist").inspect, "nil")

    assert_equal(d1.property(17), nil)
    d1.set_property(17, 42)
    assert_equal(d1.property(17), 42)

    d2 = c.create_device(dc)
    assert_equal(d2.device_class.id, dc.id)
    assert_equal(d2.device_class.object_id, dc.object_id)   # by virtue of Ruby-to-C++ object mapping
    d2.name = "D2"

    names = []
    dcs = []
    c.each_device { |d| names << d.name; dcs << d.device_class.name }
    assert_equal(names, [ "D1", "D2" ])
    assert_equal(dcs, [ "DC", "DC" ])

    c.remove_device(d2)
    
    names = []
    c.each_device { |d| names << d.name }
    assert_equal(names, [ "D1" ])

    d2 = c.create_device(dc, "D2")

    names = []
    dcs = []
    c.each_device { |d| names << d.name; dcs << d.device_class.name }
    assert_equal(names, [ "D1", "D2" ])
    assert_equal(dcs, [ "DC", "DC" ])

    net = c.create_net("NET")

    assert_equal(net.property(17), nil)
    net.set_property(17, 42)
    assert_equal(net.property(17), 42)

    assert_equal(net.is_floating?, true)
    assert_equal(net.is_internal?, false)
    assert_equal(net.terminal_count, 0)
    assert_equal(net.pin_count, 0)

    d1.connect_terminal(1, net)

    assert_equal(net.is_floating?, false)
    assert_equal(net.is_internal?, false)
    assert_equal(net.terminal_count, 1)
    assert_equal(net.pin_count, 0)

    d1.connect_terminal(0, net)

    assert_equal(net.is_floating?, false) # not really, but this simple approach tells us so ...
    assert_equal(net.is_internal?, true)
    assert_equal(net.terminal_count, 2)
    assert_equal(net.pin_count, 0)

    d1.disconnect_terminal(0)
    assert_equal(net.terminal_count, 1)

    assert_equal(d1.net_for_terminal(1).name, "NET")
    assert_equal(d1.net_for_terminal("B").name, "NET")
    assert_equal(d1.net_for_terminal("X").inspect, "nil")
    assert_equal(d1.net_for_terminal(0).inspect, "nil")

    d1.disconnect_terminal("B")
    assert_equal(net.terminal_count, 0)
    assert_equal(d1.net_for_terminal(1).inspect, "nil")

    d1.connect_terminal("B", net)
    assert_equal(net.terminal_count, 1)
    assert_equal(d1.net_for_terminal(1).name, "NET")

    d2.connect_terminal(0, net)
    assert_equal(net.terminal_count, 2)

    dnames = [] 
    net.each_terminal { |p| dnames << p.device.name + ":" + p.terminal_def.name }
    assert_equal(dnames, [ "D1:B", "D2:A" ])
    dnames = [] 
    net.each_terminal { |p| dnames << p.device_class.name + ":" + p.terminal_id.to_s }
    assert_equal(dnames, [ "DC:1", "DC:0" ])
    net.each_terminal { |p| assert_equal(p.net.name, "NET") }

    d1.disconnect_terminal(1)
    assert_equal(d1.net_for_terminal(1).inspect, "nil")
    
    dnames = [] 
    net.each_terminal { |p| dnames << p.device.name + ":" + p.terminal_def.name }
    assert_equal(dnames, [ "D2:A" ])
    net.each_terminal { |p| assert_equal(p.net.name, "NET") }

    net.clear
    assert_equal(d1.net_for_terminal(1).inspect, "nil")
    assert_equal(d1.net_for_terminal(0).inspect, "nil")

    d2.connect_terminal(0, net)
    assert_equal(net.terminal_count, 1)

    assert_equal(d1.is_combined_device?, false)

    da = RBA::DeviceAbstract::new
    da.name = "xyz"
    d1.device_abstract = da

    a = []
    d1.each_combined_abstract { |i| a << i }
    assert_equal(a.size, 0)

    t = RBA::DeviceAbstractRef::new
    t.device_abstract = d1.device_abstract
    t.trans = RBA::DCplxTrans::new(RBA::DVector::new(1, 2))
    d1.add_combined_abstract(t)

    a = []
    d1.each_combined_abstract { |i| a << i }
    assert_equal(a.size, 1)
    assert_equal(a.collect { |i| i.device_abstract.name }.join(","), "xyz")
    assert_equal(a.collect { |i| i.trans.to_s }.join(","), "r0 *1 1,2")
    
    d1.clear_combined_abstracts

    a = []
    d1.each_combined_abstract { |i| a << i }
    assert_equal(a.size, 0)

    a = []
    d1.each_reconnected_terminal_for(0) { |i| a << i }
    assert_equal(a.size, 0)

    a = []
    d1.each_reconnected_terminal_for(1) { |i| a << i }
    assert_equal(a.size, 0)

    t = RBA::DeviceReconnectedTerminal::new
    t.device_index = 0
    t.other_terminal_id = 2
    d1.add_reconnected_terminal_for(1, t)

    t = RBA::DeviceReconnectedTerminal::new
    t.device_index = 1
    t.other_terminal_id = 1
    d1.add_reconnected_terminal_for(1, t)
    
    a = []
    d1.each_reconnected_terminal_for(0) { |i| a << i }
    assert_equal(a.size, 0)

    a = []
    d1.each_reconnected_terminal_for(1) { |i| a << i }
    assert_equal(a.size, 2)
    assert_equal(a.collect { |i| i.device_index.to_s }.join(","), "0,1")
    assert_equal(a.collect { |i| i.other_terminal_id.to_s }.join(","), "2,1")
    
    d1.clear_reconnected_terminals

    a = []
    d1.each_reconnected_terminal_for(0) { |i| a << i }
    assert_equal(a.size, 0)

    a = []
    d1.each_reconnected_terminal_for(1) { |i| a << i }
    assert_equal(a.size, 0)

  end

  def test_5_SubCircuit

    nl = RBA::Netlist::new

    cc = RBA::Circuit::new
    cc.name = "CC"
    nl.add(cc)

    p1 = cc.create_pin("A")
    p2 = cc.create_pin("B")

    assert_equal(p1.id, 0)
    assert_equal(p2.id, 1)

    c = RBA::Circuit::new
    c.name = "C"
    nl.add(c)

    sc1 = c.create_subcircuit(cc)
    sc1.name = "SC1"
    assert_equal(sc1.circuit.object_id, c.object_id)
    assert_equal(sc1.name, "SC1")
    assert_equal(sc1.circuit_ref.name, "CC")
    assert_equal(c.subcircuit_by_id(sc1.id).id, 1)
    assert_equal(c.subcircuit_by_id(2).inspect, "nil")
    assert_equal(c.subcircuit_by_name(sc1.name).name, "SC1")
    assert_equal(c.subcircuit_by_id(2).inspect, "nil")
    assert_equal(c.subcircuit_by_name("doesnt_exist").inspect, "nil")

    assert_equal(sc1.property(17), nil)
    sc1.set_property(17, 42)
    assert_equal(sc1.property(17), 42)

    refs = []
    cc.each_ref { |r| refs << r.name }
    assert_equal(refs.join(","), "SC1")

    sc2 = c.create_subcircuit(cc)
    sc2.name = "SC2"

    refs = []
    cc.each_ref { |r| refs << r.name }
    assert_equal(refs.join(","), "SC1,SC2")

    names = []
    ccn = []
    c.each_subcircuit { |sc| names << sc.name; ccn << sc.circuit_ref.name }
    assert_equal(names, [ "SC1", "SC2" ])
    assert_equal(ccn, [ "CC", "CC" ])

    c.remove_subcircuit(sc2)
    
    refs = []
    cc.each_ref { |r| refs << r.name }
    assert_equal(refs.join(","), "SC1")

    names = []
    c.each_subcircuit { |sc| names << sc.name}
    assert_equal(names, [ "SC1" ])

    sc2 = c.create_subcircuit(cc, "SC2")

    names = []
    ccn = []
    c.each_subcircuit { |sc| names << sc.name; ccn << sc.circuit_ref.name }
    assert_equal(names, [ "SC1", "SC2" ])
    assert_equal(ccn, [ "CC", "CC" ])

    net = c.create_net("NET")
    assert_equal(net.pin_count, 0)
    assert_equal(net.terminal_count, 0)
    assert_equal(net.is_floating?, true)
    assert_equal(net.is_internal?, false)

    sc1.connect_pin(1, net)
    assert_equal(net.pin_count, 0)
    assert_equal(net.subcircuit_pin_count, 1)
    assert_equal(net.terminal_count, 0)
    assert_equal(net.is_floating?, false)
    assert_equal(net.is_internal?, false)
    assert_equal(sc1.net_for_pin(1).name, "NET")
    assert_equal(sc1.net_for_pin(0).inspect, "nil")

    sc2.connect_pin(0, net)
    assert_equal(net.pin_count, 0)
    assert_equal(net.subcircuit_pin_count, 2)
    assert_equal(net.terminal_count, 0)
    assert_equal(net.is_floating?, false)
    assert_equal(net.is_internal?, false)

    cnames = [] 
    net.each_pin { |p| cnames << "+" + p.pin.name }
    net.each_subcircuit_pin { |p| cnames << p.subcircuit.name + ":" + p.pin.name }
    assert_equal(cnames, [ "SC1:B", "SC2:A" ])
    cnames = [] 
    net.each_pin { |p| cnames << "+" + p.pin.name }
    net.each_subcircuit_pin { |p| cnames << p.subcircuit.name + ":" + p.pin_id.to_s }
    assert_equal(cnames, [ "SC1:1", "SC2:0" ])
    net.each_pin { |p| assert_equal(p.net.name, "NET") }

    sc1.disconnect_pin(1)
    assert_equal(sc1.net_for_pin(1).inspect, "nil")
    
    cnames = [] 
    net.each_pin { |p| cnames << "+" + p.pin.name }
    net.each_subcircuit_pin { |p| cnames << p.subcircuit.name + ":" + p.pin.name }
    assert_equal(cnames, [ "SC2:A" ])
    net.each_subcircuit_pin { |p| assert_equal(p.net.name, "NET") }

    net.clear
    assert_equal(sc1.net_for_pin(1).inspect, "nil")
    assert_equal(sc1.net_for_pin(0).inspect, "nil")

  end

  def test_6_SubCircuitBasic

    nl = RBA::Netlist::new

    c = RBA::Circuit::new
    c.name = "C"
    nl.add(c)

    net = c.create_net

    assert_equal(net.circuit.name, "C")

    net.name = "NET"
    assert_equal(net.name, "NET")
    assert_equal(net.expanded_name, "NET")
    
    net.cluster_id = 42
    assert_equal(net.cluster_id, 42)
    assert_equal(net.expanded_name, "NET")

    net.name = ""
    assert_equal(net.expanded_name, "$42")

  end

  def test_7_DeviceClass

    nl = RBA::Netlist::new

    dc = RBA::DeviceClass::new
    nl.add(dc)
    dc.name = "DC"
    assert_equal(dc.name, "DC")
    dc.description = "A device class"
    assert_equal(dc.description, "A device class")

    pd = RBA::DeviceTerminalDefinition::new("A", "Terminal A")
    dc.add_terminal(pd)

    assert_equal(pd.id, 0)
    assert_equal(pd.name, "A")
    assert_equal(pd.description, "Terminal A")

    pd = RBA::DeviceTerminalDefinition::new
    pd.name = "B"
    pd.description = "Terminal B"
    dc.add_terminal(pd)

    assert_equal(pd.id, 1)
    assert_equal(pd.name, "B")
    assert_equal(pd.description, "Terminal B")

    names = []
    dc.terminal_definitions.each { |pd| names << pd.name }
    assert_equal(names, [ "A", "B" ])

    dc.clear_terminals

    names = []
    dc.terminal_definitions.each { |pd| names << pd.name }
    assert_equal(names, [])

    pd = RBA::DeviceParameterDefinition::new("P1", "Parameter 1")
    pd.default_value = 1.0
    pd.is_primary = true

    dc.add_parameter(pd)

    assert_equal(pd.id, 0)
    assert_equal(pd.name, "P1")
    assert_equal(pd.description, "Parameter 1")
    assert_equal(pd.default_value, 1.0)

    pd = RBA::DeviceParameterDefinition::new("", "")
    pd.name = "P2"
    pd.description = "Parameter 2"
    dc.add_parameter(pd)

    assert_equal(pd.id, 1)
    assert_equal(pd.name, "P2")
    assert_equal(pd.description, "Parameter 2")
    assert_equal(pd.default_value, 0.0)

    names = []
    dc.parameter_definitions.each { |pd| names << pd.name }
    assert_equal(names, [ "P1", "P2" ])

    dc.clear_parameters

    names = []
    dc.parameter_definitions.each { |pd| names << pd.name }
    assert_equal(names, [])

  end

  def test_8_Circuit

    nl = RBA::Netlist::new

    c = RBA::Circuit::new
    nl.add(c)
    assert_equal(c.netlist.object_id, nl.object_id)
    c.name = "C"
    assert_equal(c.name, "C")
    c.cell_index = 42
    assert_equal(c.cell_index, 42)

    assert_equal(c.property(17), nil)
    c.set_property(17, 42)
    assert_equal(c.property(17), 42)

    pina1 = c.create_pin("A1")
    pina2 = c.create_pin("A2")
    pinb1 = c.create_pin("B1")
    pinb2 = c.create_pin("B2")
    assert_equal(c.pin_count, 4)

    assert_equal(pina1.id, 0)
    assert_equal(pina2.id, 1)
    assert_equal(pinb1.id, 2)
    assert_equal(pinb2.id, 3)
    assert_equal(c.pin_by_id(0).name, "A1")
    assert_equal(c.pin_by_name("A1").name, "A1")
    assert_equal(c.pin_by_id(3).name, "B2")
    assert_equal(c.pin_by_name("B2").name, "B2")
    assert_equal(c.pin_by_id(17).inspect, "nil")
    assert_equal(c.pin_by_name("DOESNOTEXIST").inspect, "nil")

    assert_equal(c.pin_by_id(0).property(17), nil)
    c.pin_by_id(0).set_property(17, 42)
    assert_equal(c.pin_by_id(0).property(17), 42)
    assert_equal(c.pin_by_name("A1").property(17), 42)

    names = []
    c.each_pin { |p| names << p.name }
    assert_equal(names, [ "A1", "A2", "B1", "B2" ])

    net1 = c.create_net
    net1.cluster_id = 17
    net1.name = "NET1"

    assert_equal(c.net_by_cluster_id(17).name, "NET1")
    assert_equal(c.net_by_cluster_id(42).inspect, "nil")
    assert_equal(c.net_by_name("NET1").name, "NET1")
    assert_equal(c.nets_by_name("NET*").collect(&:name), ["NET1"])
    assert_equal(c.net_by_name("DOESNOTEXIST").inspect, "nil")
    assert_equal(c.nets_by_name("DOESNOTEXIST").collect(&:name), [])

    assert_equal(c.net_by_name("net1").inspect, "nil")
    nl.case_sensitive = false
    assert_equal(c.net_by_name("net1").name, "NET1")
    nl.case_sensitive = true

    net2 = c.create_net
    net2.name = "NET2"

    names = []
    c.each_net { |n| names << n.name }
    assert_equal(names, [ "NET1", "NET2" ])
    assert_equal(c.nets_by_name("NET*").collect(&:name), ["NET1", "NET2"])
    assert_equal(c.nets_by_name("net*").collect(&:name), [])
    nl.case_sensitive = false
    assert_equal(c.nets_by_name("NET*").collect(&:name), ["NET1", "NET2"])
    assert_equal(c.nets_by_name("net*").collect(&:name), ["NET1", "NET2"])
    nl.case_sensitive = true

    assert_equal(net1.pin_count, 0)
    c.connect_pin(pina1, net1)
    cnames = []
    net1.each_pin { |p| cnames << "+" + p.pin.name }
    net1.each_subcircuit_pin { |p| cnames << p.subcircuit.name + ":" + p.pin.name }
    assert_equal(cnames, [ "+A1" ])
    assert_equal(net1.pin_count, 1)
    c.connect_pin(pinb1.id, net1)
    c.connect_pin(pina2, net2)
    c.connect_pin(pinb2.id, net2)

    assert_equal(c.net_for_pin(pina1).name, "NET1")
    assert_equal(c.net_for_pin(pinb1.id).name, "NET1")
    assert_equal(c.net_for_pin(pina2).name, "NET2")
    assert_equal(c.net_for_pin(pinb2.id).name, "NET2")

    c.disconnect_pin(pinb1.id)
    c.disconnect_pin(pina2)

    assert_equal(c.net_for_pin(pina1).name, "NET1")
    assert_equal(c.net_for_pin(pinb1.id).inspect, "nil")
    assert_equal(c.net_for_pin(pina2).inspect, "nil")
    assert_equal(c.net_for_pin(pinb2.id).name, "NET2")

    c.remove_net(net1)

    assert_equal(c.net_for_pin(pina1).inspect, "nil")
    assert_equal(c.net_for_pin(pinb1.id).inspect, "nil")
    assert_equal(c.net_for_pin(pina2).inspect, "nil")
    assert_equal(c.net_for_pin(pinb2.id).name, "NET2")

    names = []
    c.each_net { |n| names << n.name }
    assert_equal(names, [ "NET2" ])

    c.clear

    names = []
    c.each_net { |n| names << n.name }
    assert_equal(names, [])

    assert_equal(c.pin_count, 0)

  end

  def test_9_DeviceParameters

    nl = RBA::Netlist::new

    dc = RBA::DeviceClass::new
    dc.name = "DC"
    nl.add(dc)

    dc.add_parameter(RBA::DeviceParameterDefinition::new("U", "Parameter U", 1.0))
    dc.add_parameter(RBA::DeviceParameterDefinition::new("V", "Parameter V", 2.0))

    assert_equal(dc.has_parameter?("U"), true)
    assert_equal(dc.has_parameter?("V"), true)
    assert_equal(dc.has_parameter?("X"), false)
    assert_equal(dc.parameter_id("U"), 0)
    assert_equal(dc.parameter_id("V"), 1)
    error = false
    begin
      dc.parameter_id("X") # raises an exception
    rescue => ex
      error = true
    end
    assert_equal(error, true)

    c = RBA::Circuit::new
    c.name = "C"
    nl.add(c)
    d1 = c.create_device(dc)

    assert_equal(d1.circuit.object_id, c.object_id)

    assert_equal(d1.parameter(0), 1.0)
    assert_equal(d1.parameter("U"), 1.0)
    assert_equal(d1.parameter(1), 2.0)
    assert_equal(d1.parameter("V"), 2.0)

    d1.set_parameter(0, 0.5)
    assert_equal(d1.parameter(0), 0.5)
    assert_equal(d1.parameter(1), 2.0)
    d1.set_parameter("U", -0.5)
    assert_equal(d1.parameter(0), -0.5)
    assert_equal(d1.parameter(1), 2.0)
    d1.set_parameter("V", 42)
    assert_equal(d1.parameter(0), -0.5)
    assert_equal(d1.parameter(1), 42)

    assert_equal(nl.to_s, <<END)
circuit C ();
  device DC $1 () (U=-0.5,V=42);
end;
END

  end

  def test_10_NetlistTopology

    nl = RBA::Netlist::new
    assert_equal(nl.top_circuit_count, 0)

    c1 = RBA::Circuit::new
    c1.name = "C1"
    c1.cell_index = 17
    nl.add(c1)
    assert_equal(nl.top_circuit_count, 1)

    c2 = RBA::Circuit::new
    c2.name = "C2"
    c1.cell_index = 42
    nl.add(c2)
    assert_equal(nl.top_circuit_count, 2)

    c3 = RBA::Circuit::new
    c3.name = "C3"
    nl.add(c3)
    assert_equal(nl.top_circuit_count, 3)

    names = []
    nl.each_circuit_top_down { |c| names << c.name }
    assert_equal(names.join(","), "C3,C2,C1")

    names = []
    nl.each_circuit_bottom_up { |c| names << c.name }
    assert_equal(names.join(","), "C1,C2,C3")

    names = []
    c1.each_child { |c| names << c.name }
    assert_equal(names.join(","), "")

    names = []
    c2.each_parent { |c| names << c.name }
    assert_equal(names.join(","), "")

    c1.create_subcircuit(c2)
    assert_equal(nl.top_circuit_count, 2)

    names = []
    c1.each_child { |c| names << c.name }
    assert_equal(names.join(","), "C2")

    names = []
    c2.each_parent { |c| names << c.name }
    assert_equal(names.join(","), "C1")

    c1.create_subcircuit(c2)
    c1.create_subcircuit(c3)
    assert_equal(nl.top_circuit_count, 1)

    names = []
    c1.each_child { |c| names << c.name }
    assert_equal(names.join(","), "C2,C3")

    names = []
    c2.each_parent { |c| names << c.name }
    assert_equal(names.join(","), "C1")

    names = []
    c3.each_parent { |c| names << c.name }
    assert_equal(names.join(","), "C1")

    names = []
    nl.each_circuit_top_down { |c| names << c.name }
    assert_equal(names.join(","), "C1,C3,C2")

    names = []
    nl.each_circuit_bottom_up { |c| names << c.name }
    assert_equal(names.join(","), "C2,C3,C1")

    c3.create_subcircuit(c2)
    assert_equal(nl.top_circuit_count, 1)

    names = []
    c2.each_parent { |c| names << c.name }
    assert_equal(names.join(","), "C1,C3")

    names = []
    c3.each_parent { |c| names << c.name }
    assert_equal(names.join(","), "C1")

    names = []
    nl.each_circuit_top_down { |c| names << c.name }
    assert_equal(names.join(","), "C1,C3,C2")

    names = []
    nl.each_circuit_bottom_up { |c| names << c.name }
    assert_equal(names.join(","), "C2,C3,C1")

  end

  def test_11_FlattenCircuits

    nl = RBA::Netlist::new

    dc = RBA::DeviceClassMOS3Transistor::new
    dc.name = "NMOS"
    nl.add(dc)

    dc = RBA::DeviceClassMOS3Transistor::new
    dc.name = "PMOS"
    nl.add(dc)

    nl.from_s(<<"END")
circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);
  subcircuit PTRANS SC1 ($1=$5,$2=$2,$3=IN);
  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3=IN);
  subcircuit PTRANS SC3 ($1=$5,$2=OUT,$3=$2);
  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=$2);
end;
circuit PTRANS ($1=$1,$2=$2,$3=$3);
  device PMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);
end;
circuit NTRANS ($1=$1,$2=$2,$3=$3);
  device NMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);
end;
END

    nl2 = nl.dup
    inv2 = nl2.circuit_by_name("INV2")
    inv2.flatten_subcircuit(inv2.subcircuit_by_name("SC1"))

    assert_equal(nl2.to_s, <<"END")
circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);
  device PMOS $1 (S=$5,G=IN,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);
  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3=IN);
  subcircuit PTRANS SC3 ($1=$5,$2=OUT,$3=$2);
  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=$2);
end;
circuit PTRANS ($1=$1,$2=$2,$3=$3);
  device PMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);
end;
circuit NTRANS ($1=$1,$2=$2,$3=$3);
  device NMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);
end;
END

    nl3 = nl2.dup
    nl3.flatten_circuit(nl3.circuit_by_name("NTRANS"))
    nl3.flatten_circuit(nl3.circuit_by_name("PTRANS"))

    assert_equal(nl3.to_s, <<"END")
circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);
  device PMOS $1 (S=$5,G=IN,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);
  device NMOS $2 (S=$4,G=IN,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);
  device NMOS $3 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);
  device PMOS $4 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);
end;
END

    nl4 = nl2.dup
    nl4.flatten_circuit("{P,N}TRANS")
    assert_equal(nl4.to_s, nl3.to_s)

    nl4 = nl2.dup
    nl4.flatten_circuits([ nl4.circuit_by_name("PTRANS"), nl4.circuit_by_name("NTRANS") ])
    assert_equal(nl4.to_s, nl3.to_s)

    nl2 = nl.dup
    nl2.flatten_circuit("*")   # smoke test
    assert_equal(nl2.to_s, "")

    nl2 = nl.dup
    cc = []
    nl2.each_circuit { |c| cc << c }
    nl2.flatten_circuits(cc) 
    assert_equal(nl2.to_s, "")

  end

  def test_12_BlankAndPurgeCircuits

    nl = RBA::Netlist::new

    dc = RBA::DeviceClassMOS3Transistor::new
    dc.name = "NMOS"
    nl.add(dc)

    dc = RBA::DeviceClassMOS3Transistor::new
    dc.name = "PMOS"
    nl.add(dc)

    nl.from_s(<<"END")
circuit RINGO (IN=IN,OSC=OSC,VSS=VSS,VDD=VDD);
  subcircuit INV2 INV2_SC1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);
  subcircuit INV2 INV2_SC2 (IN=FB,$2=(null),OUT=$I8,$4=VSS,$5=VDD);
end;
circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);
  subcircuit PTRANS SC1 ($1=$5,$2=$2,$3=IN);
  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3=IN);
  subcircuit PTRANS SC3 ($1=$5,$2=OUT,$3=$2);
  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=$2);
end;
circuit PTRANS ($1=$1,$2=$2,$3=$3);
  device PMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);
end;
circuit NTRANS ($1=$1,$2=$2,$3=$3);
  device NMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);
end;
END

    nl2 = nl.dup
    circuit = nl2.circuit_by_name("INV2")
    nl2.purge_circuit(circuit)

    assert_equal(nl2.to_s, <<"END")
circuit RINGO (IN=IN,OSC=OSC,VSS=VSS,VDD=VDD);
  subcircuit (null);
  subcircuit (null);
end;
END

    nl2 = nl.dup
    circuit = nl2.circuit_by_name("INV2")
    circuit.blank

    assert_equal(nl2.to_s, <<"END")
circuit RINGO (IN=IN,OSC=OSC,VSS=VSS,VDD=VDD);
  subcircuit INV2 INV2_SC1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);
  subcircuit INV2 INV2_SC2 (IN=FB,$2=(null),OUT=$I8,$4=VSS,$5=VDD);
end;
circuit INV2 (IN=(null),$2=(null),OUT=(null),$4=(null),$5=(null));
end;
END

    nl2 = nl.dup
    circuit = nl2.circuit_by_name("RINGO")
    nl2.purge_circuit(circuit)

    assert_equal(nl2.to_s, "")

    nl2 = nl.dup
    circuit = nl2.circuit_by_name("RINGO")
    circuit.blank

    assert_equal(nl2.to_s, <<"END")
circuit RINGO (IN=(null),OSC=(null),VSS=(null),VDD=(null));
end;
END

    nl2 = nl.dup
    nl2.blank_circuit("*")

    assert_equal(nl2.to_s, <<"END")
circuit RINGO (IN=(null),OSC=(null),VSS=(null),VDD=(null));
end;
END

    nl2 = nl.dup
    nl2.blank_circuit("RINGO")

    assert_equal(nl2.to_s, <<"END")
circuit RINGO (IN=(null),OSC=(null),VSS=(null),VDD=(null));
end;
END

  end

  def test_13_JoinNets

    nl = RBA::Netlist::new

    dc = RBA::DeviceClassMOS3Transistor::new
    dc.name = "NMOS"
    nl.add(dc)

    dc = RBA::DeviceClassMOS3Transistor::new
    dc.name = "PMOS"
    nl.add(dc)

    nl.from_s(<<"END")
circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);
  subcircuit PTRANS SC1 ($1=$5,$2=$2,$3=IN);
  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3=IN);
  subcircuit PTRANS SC3 ($1=$5,$2=OUT,$3=$2);
  subcircuit NTRANS SC4 ($1=$4,$2=OUT,$3=$2);
end;
circuit PTRANS ($1=$1,$2=$2,$3=$3);
  device PMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);
end;
circuit NTRANS ($1=$1,$2=$2,$3=$3);
  device NMOS $1 (S=$1,D=$2,G=$3) (L=0.25,W=0.95);
end;
END

    c = nl.circuit_by_name("INV2")
    c.join_nets(c.net_by_name("IN"), c.net_by_name("OUT"))

    assert_equal(nl.to_s, <<"END")
circuit INV2 ('IN,OUT'='IN,OUT',$2=$2,$3=$4,$4=$5);
  subcircuit PTRANS SC1 ($1=$5,$2=$2,$3='IN,OUT');
  subcircuit NTRANS SC2 ($1=$4,$2=$2,$3='IN,OUT');
  subcircuit PTRANS SC3 ($1=$5,$2='IN,OUT',$3=$2);
  subcircuit NTRANS SC4 ($1=$4,$2='IN,OUT',$3=$2);
end;
circuit PTRANS ($1=$1,$2=$2,$3=$3);
  device PMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);
end;
circuit NTRANS ($1=$1,$2=$2,$3=$3);
  device NMOS $1 (S=$1,G=$3,D=$2) (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);
end;
END

  end

  def test_14_issue617

    netlist = RBA::Netlist::new
    netlist.from_s(<<"END")
    circuit TOP ();
      subcircuit SC1 $1 (A='1', B='2');
    end;
    circuit SC1 (A='A', 'B'='B');
    end;
END

    def collect_net_names(net)
      names = []
      if ! net.name.empty?
        names << net.name
      end
      net.each_subcircuit_pin do |scp|
        subnet = scp.subcircuit.circuit_ref.net_for_pin(scp.pin_id)
        names += collect_net_names(subnet)
      end
      names
    end

    assert_equal(collect_net_names(netlist.circuit_by_name("TOP").net_by_name("1")), ["1", "A"])

  end

  def test_15_deviceParameterCompare
    
    dpc = RBA::EqualDeviceParameters::new(1)
    assert_equal(dpc.to_string, "#1:A0/R0")

    dpc += RBA::EqualDeviceParameters::new(2, 1.0, 0.25)
    assert_equal(dpc.to_string, "#1:A0/R0;#2:A1/R0.25")

    dpc += RBA::EqualDeviceParameters::ignore(3)
    assert_equal(dpc.to_string, "#1:A0/R0;#2:A1/R0.25;#3:ignore")

  end

  def test_16_deviceParameterObject

    pd = RBA::DeviceParameterDefinition::new("P1", "Parameter 1", 2.0, false, 17.5, 2.0)
    assert_equal(pd.default_value, 2.0)
    pd.default_value = 1.0
    assert_equal(pd.default_value, 1.0)
    assert_equal(pd.is_primary?, false)
    pd.is_primary = true
    assert_equal(pd.is_primary?, true)
    assert_equal(pd.si_scaling, 17.5)
    pd.si_scaling = 1.0
    assert_equal(pd.si_scaling, 1.0)
    assert_equal(pd.geo_scaling_exponent, 2.0)
    pd.geo_scaling_exponent = 1.0
    assert_equal(pd.geo_scaling_exponent, 1.0)

  end

end

load("test_epilogue.rb")

