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

def mapping_to_s(ly1, ly2, lm)
  r = ""
  ly1.layer_indices.each do |li|
    s = ly1.get_info(li).to_s
    if lm.has_mapping?(li)
      s += "=>" + ly2.get_info(lm.layer_mapping(li)).to_s
    end
    r == "" || (r += ";")
    r += s
  end
  r
end

def mapping_to_s_from_table(ly1, ly2, lm)
  r = ""
  table = lm.table
  ly1.layer_indices.each do |li|
    s = ly1.get_info(li).to_s
    if table[li]
      s += "=>" + ly2.get_info(table[li]).to_s
    end
    r == "" || (r += ";")
    r += s
  end
  r
end

class DBLayerMapping_TestClass < TestBase

  def test_1

    mp = RBA::LayerMapping::new
    mp.map(0, 1)
    assert_equal(mp.has_mapping?(0), true)
    assert_equal(mp.has_mapping?(1), false)
    assert_equal(mp.layer_mapping(0), 1)
    mp.clear
    assert_equal(mp.has_mapping?(0), false)
    assert_equal(mp.has_mapping?(1), false)
    mp.map(1, 2)
    assert_equal(mp.layer_mapping(1), 2)
    mp.map(1, 3)
    assert_equal(mp.layer_mapping(1), 3)

    ly1 = RBA::Layout::new

    a1 = ly1.insert_layer(RBA::LayerInfo::new(1, 0))
    a2 = ly1.insert_layer(RBA::LayerInfo::new(2, 0))
    a3 = ly1.insert_layer(RBA::LayerInfo::new("A"))

    ly2 = RBA::Layout::new
    assert_equal(mapping_to_s(ly2, ly1, mp), "")
    assert_equal(mapping_to_s_from_table(ly2, ly1, mp), "")

    b1 = ly2.insert_layer(RBA::LayerInfo::new("A"))
    b2 = ly2.insert_layer(RBA::LayerInfo::new(3, 0))
    b3 = ly2.insert_layer(RBA::LayerInfo::new(2, 0))

    mp = RBA::LayerMapping::new
    mp.create(ly1, ly2)
    assert_equal(mapping_to_s(ly2, ly1, mp), "A=>A;3/0;2/0=>2/0")
    assert_equal(mapping_to_s_from_table(ly2, ly1, mp), "A=>A;3/0;2/0=>2/0")

    mp = RBA::LayerMapping::new
    nl = mp.create_full(ly1, ly2)
    assert_equal(mapping_to_s(ly2, ly1, mp), "A=>A;3/0=>3/0;2/0=>2/0")
    assert_equal(mapping_to_s_from_table(ly2, ly1, mp), "A=>A;3/0=>3/0;2/0=>2/0")
    assert_equal(nl.inspect, "[3]")

  end

end

load("test_epilogue.rb")
