# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2020 Matthias Koefferlein
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

class LAYTechnologies_TestClass < TestBase

  def test_1

    RBA::Technology::technologies_from_xml(<<END)
<technologies>
  <technology>
    <name/>
  </technology>
  <technology>
    <name>MINE</name>
  </technology>
</technologies>
END
    assert_equal(RBA::Technology::technology_names.inspect, '["", "MINE"]')

    s = RBA::Technology::technologies_to_xml
    RBA::Technology::technologies_from_xml("<technologies/>")
    assert_equal(RBA::Technology::technology_names.inspect, '[""]')
    RBA::Technology::technologies_from_xml(s)
    assert_equal(RBA::Technology::technology_names.inspect, '["", "MINE"]')

    tech = RBA::Technology::technology_by_name("MINE")
    assert_equal(tech.name, "MINE")
    tech.name = "MINE2"
    assert_equal(tech.name, "MINE2")
    
    assert_equal(RBA::Technology::technology_names.inspect, '["", "MINE2"]')

    assert_equal(RBA::Technology::has_technology?("MINE"), false)
    assert_equal(RBA::Technology::has_technology?("MINE2"), true)
    tech = RBA::Technology::technology_by_name("MINE")
    assert_equal(tech != nil, true)
    assert_equal(tech.name, "")
    tech2 = RBA::Technology::technology_by_name("MINE2")
    assert_equal(tech != nil, true)

    RBA::Technology::remove_technology("X")
    assert_equal(RBA::Technology::technology_names.inspect, '["", "MINE2"]')
    RBA::Technology::remove_technology("MINE2")
    assert_equal(RBA::Technology::technology_names.inspect, '[""]')

  end

  def test_2

    tech = RBA::Technology::technology_from_xml(<<END)
<technology>
  <name>X</name>
</technology>
END
    assert_equal(tech.name, "X")

    tech.name = "Y"
    assert_equal(tech.name, "Y")

    tech.description = "A big Y"
    assert_equal(tech.description, "A big Y")

    tech.dbu = 5.0
    assert_equal(tech.dbu, 5.0)

    tech.default_base_path = "/default/path"
    assert_equal(tech.default_base_path, "/default/path")
    assert_equal(tech.base_path, "/default/path")
    assert_equal(tech.correct_path("/default/path/myfile.xml"), "myfile.xml")
    assert_equal(tech.eff_path("myfile.xml").gsub("\\", "/"), "/default/path/myfile.xml")

    tech.explicit_base_path = "/basic/path"
    assert_equal(tech.explicit_base_path, "/basic/path")
    assert_equal(tech.base_path, "/basic/path")
    assert_equal(tech.correct_path("/basic/path/myfile.xml"), "myfile.xml")
    assert_equal(tech.eff_path("myfile.xml").gsub("\\", "/"), "/basic/path/myfile.xml")

    tech.layer_properties_file = "x.lyp"
    assert_equal(tech.layer_properties_file, "x.lyp")
    assert_equal(tech.eff_layer_properties_file.gsub("\\", "/"), "/basic/path/x.lyp")

    tech.add_other_layers = true
    assert_equal(tech.add_other_layers?, true)
    tech.add_other_layers = false
    assert_equal(tech.add_other_layers?, false)

    opt = tech.load_layout_options
    opt.dxf_dbu = 2.5
    tech.load_layout_options = opt
    assert_equal(tech.load_layout_options.dxf_dbu, 2.5)
    opt = tech.save_layout_options
    opt.dbu = 0.125
    tech.save_layout_options = opt
    assert_equal(tech.save_layout_options.dbu, 0.125)

    assert_equal(tech.component_names.size > 0, true)
    assert_equal(tech.component_names.find("connectivity") != nil, true)
    assert_equal(tech.component("connectivity").class.to_s, "RBA::NetTracerTechnology")

  end

end

load("test_epilogue.rb")
