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

class LAYNetlistBrowser_TestClass < TestBase

  # Basic view creation and MainWindow events
  def test_1

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    mw.close_all

    cv = mw.load_layout(ENV["TESTSRC"] + "/testdata/lvs/ringo_mixed_hierarchy.gds", 0)
    lv = cv.view

    nb = lv.netlist_browser

    db = lv.lvsdb(lv.create_lvsdb("DB1"))
    db.read(ENV["TESTSRC"] + "/testdata/lvs/ringo_mixed_hierarchy.lvsdb")

    lv.show_lvsdb(0, 0)

    # Test path selections

    nl = nb.db.netlist

    # net
    n = nl.circuit_by_name("INVX1").net_by_name("OUT")
    pth = RBA::NetlistObjectsPath.from_first(n)
    nb.current_path = pth
    assert_equal(nb.current_path.first.net.to_s, "INVX1:OUT")
    assert_equal(nb.current_path.second.net.to_s, "INVX1:OUT")
  
    # device
    n = nl.circuit_by_name("INVX1").device_by_id(1)
    pth = RBA::NetlistObjectsPath.from_first(n)
    nb.current_path = pth
    assert_equal(nb.current_path.first.device.expanded_name, "$1")
    assert_equal(nb.current_path.second.device.expanded_name, "$1")

    # subcircuit
    n = nl.circuit_by_name("RINGO").subcircuit_by_id(2)
    pth = RBA::NetlistObjectsPath.from_first(n)
    nb.current_path = pth
    assert_equal(nb.current_path.first.path[-1].expanded_name, "$2")
    assert_equal(nb.current_path.second.path[-1].expanded_name, "$11")

    # circuit
    n = nl.circuit_by_name("INVX1")
    pth = RBA::NetlistObjectsPath.from_first(n)
    nb.current_path = pth
    assert_equal(nb.current_path.first.root.name, "INVX1")
    assert_equal(nb.current_path.second.root.name, "INVX1")

    nl = nb.db.reference

    # net:
    n = nl.circuit_by_name("INVX1").net_by_name("OUT")
    pth = RBA::NetlistObjectsPath.from_second(n)
    nb.current_path = pth
    assert_equal(nb.current_path.first.net.to_s, "INVX1:OUT")
    assert_equal(nb.current_path.second.net.to_s, "INVX1:OUT")
    
    # device
    n = nl.circuit_by_name("INVX1").device_by_name("$2")
    pth = RBA::NetlistObjectsPath.from_second(n)
    nb.current_path = pth
    assert_equal(nb.current_path.first.device.expanded_name, "$2")
    assert_equal(nb.current_path.second.device.expanded_name, "$2")

    # subcircuit
    n = nl.circuit_by_name("RINGO").subcircuit_by_name("$3")
    pth = RBA::NetlistObjectsPath.from_second(n)
    nb.current_path = pth
    assert_equal(nb.current_path.first.path[-1].expanded_name, "$10")
    assert_equal(nb.current_path.second.path[-1].expanded_name, "$3")

    # circuit
    n = nl.circuit_by_name("INVX1")
    pth = RBA::NetlistObjectsPath.from_second(n)
    nb.current_path = pth
    assert_equal(nb.current_path.first.root.name, "INVX1")
    assert_equal(nb.current_path.second.root.name, "INVX1")

    mw.close_all

  end

end

load("test_epilogue.rb")

