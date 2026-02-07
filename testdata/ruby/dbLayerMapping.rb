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

  def test_2

    lmap = RBA::LayerMap::new 

    lmap.map( "1/0", 0 )
    assert_equal(lmap.is_mapped(RBA::LayerInfo::new(1, 0)), true)
    assert_equal(lmap.is_mapped(RBA::LayerInfo::new(1, 1)), false)
    assert_equal(lmap.is_mapped(RBA::LayerInfo::new(2, 2)), false)
    lmap.map( "2/2", 0 )
    assert_equal(lmap.is_mapped(RBA::LayerInfo::new(2, 2)), true)
    lmap.map( "10/2", 0 )
    assert_equal( lmap.mapping_str(0), "1/0;2/2;10/2" )
    
    lmap.map( "3/2", 1 )
    lmap.map( "2/2", 1 )
    lmap.map( "4/2", 1 )
    lmap.map( "1/2", 1 )
    lmap.map( "0/0", 1 )
    assert_equal( lmap.mapping_str(1), "0/0;1-4/2" )   # could be "0/0;1-4/2" as well ...

    lmap.map( RBA::LayerInfo::new(2, 2), RBA::LayerInfo::new(4, 4), 2 )
    lmap.map( RBA::LayerInfo::new(0, 1), 2 )
    assert_equal( lmap.mapping_str(2), "0/1;2-4/2-4" )

    assert_equal( lmap.is_mapped?(RBA::LayerInfo::new(2, 4)), true )
    assert_equal( lmap.is_mapped?(RBA::LayerInfo::new(0, 0)), true )
    assert_equal( lmap.is_mapped?(RBA::LayerInfo::new(0, 100)), false )

    assert_equal( lmap.logical(RBA::LayerInfo::new(2, 4)), 2 )
    assert_equal( lmap.logical(RBA::LayerInfo::new(4, 2)), 2 )
    assert_equal( lmap.logical(RBA::LayerInfo::new(1, 2)), 1 )
    assert_equal( lmap.logical(RBA::LayerInfo::new(0, 0)), 1 )
    assert_equal( lmap.logical(RBA::LayerInfo::new(100, 0)), -1 )
    assert_equal( lmap.logical(RBA::LayerInfo::new(10, 2)), 0 )
    assert_equal( lmap.logical(RBA::LayerInfo::new(1, 0)), 0 )

    assert_equal( lmap.mapping(2).to_s, "2/2" )
    assert_equal( lmap.mapping(1).to_s, "1/2" )
    assert_equal( lmap.mapping(0).to_s, "10/2" )

    lmap2 = lmap.dup

    lmap.clear 
    assert_equal( lmap.is_mapped?(RBA::LayerInfo::new(2, 4)), false )
    assert_equal( lmap.is_mapped?(RBA::LayerInfo::new(0, 0)), false )
    assert_equal( lmap.is_mapped?(RBA::LayerInfo::new(0, 100)), false )

    assert_equal( lmap2.is_mapped?(RBA::LayerInfo::new(2, 4)), true )
    assert_equal( lmap2.is_mapped?(RBA::LayerInfo::new(0, 0)), true )
    assert_equal( lmap2.is_mapped?(RBA::LayerInfo::new(0, 100)), false )

    assert_equal( lmap2.mapping(0).to_s, "10/2" )

    lmap2.assign( lmap )
    assert_equal( lmap2.is_mapped?(RBA::LayerInfo::new(2, 4)), false )
    assert_equal( lmap2.is_mapped?(RBA::LayerInfo::new(0, 0)), false )
    assert_equal( lmap2.is_mapped?(RBA::LayerInfo::new(0, 100)), false )

    lmap = RBA::LayerMap::new 
    lmap.map( "2/2:4/4", 0 )
    lmap.map( RBA::LayerInfo::new(0, 1), 2, RBA::LayerInfo::new(5, 5) )
    assert_equal( lmap.mapping_str(0), "2/2 : 4/4" )
    assert_equal( lmap.mapping(0).to_s, "4/4" )
    assert_equal( lmap.mapping(2).to_s, "5/5" )

    lmap = RBA::LayerMap::new

    lmap.map("*/*", 0)
    lmap.unmap(RBA::LayerInfo::new(5, 10))
    assert_equal(lmap.mapping_str(0), "0-4/*;5/0-9,11-*;6-*/*")

    lmap.clear
    lmap.map("*/*", 0)
    lmap.unmap(RBA::LayerInfo::new(5, 10), RBA::LayerInfo::new(16, 21))
    assert_equal(lmap.mapping_str(0), "0-4/*;5-16/0-9,22-*;17-*/*")

    lmap.clear
    lmap.map("*/*", 0)
    lmap.unmap("5-16/10-21")
    assert_equal(lmap.mapping_str(0), "0-4/*;5-16/0-9,22-*;17-*/*")

    lmap.clear
    lmap.map("*/*", 0)
    lmap.mmap(RBA::LayerInfo::new(5, 10), 1)
    assert_equal(lmap.mapping_str(0), "+*/*")
    assert_equal(lmap.mapping_str(1), "+5/10")
    
    lmap.clear
    lmap.map("*/*", 0)
    lmap.mmap(RBA::LayerInfo::new(5, 10), 1, RBA::LayerInfo::new(100, 0))
    assert_equal(lmap.mapping_str(0), "+*/*")
    assert_equal(lmap.mapping_str(1), "+5/10 : 100/0")
    
    lmap.clear
    lmap.map("*/*", 0)
    lmap.mmap(RBA::LayerInfo::new(5, 10), RBA::LayerInfo::new(16, 21), 1)
    assert_equal(lmap.mapping_str(0), "+*/*")
    assert_equal(lmap.mapping_str(1), "+5-16/10-21")
    
    lmap.clear
    lmap.map("*/*", 0)
    lmap.mmap(RBA::LayerInfo::new(5, 10), RBA::LayerInfo::new(16, 21), 1, RBA::LayerInfo::new(100, 0))
    assert_equal(lmap.mapping_str(0), "+*/*")
    assert_equal(lmap.mapping_str(1), "+5-16/10-21 : 100/0")
    
    lmap.clear
    lmap.map("*/*", 0)
    lmap.mmap("5-16/10-21", 1)
    assert_equal(lmap.mapping_str(0), "+*/*")
    assert_equal(lmap.mapping_str(1), "+5-16/10-21")

    assert_equal(lmap.logicals(RBA::LayerInfo::new(5, 10)), [ 0, 1 ])
    assert_equal(lmap.logicals(RBA::LayerInfo::new(0, 10)), [ 0 ])
    
    # optional log layer
    lmap.clear
    lmap.map("*/*")
    lmap.mmap("5-16/10-22")
    assert_equal(lmap.mapping_str(0), "+*/*")
    assert_equal(lmap.mapping_str(1), "+5-16/10-22")
    
  end

  def test_3
   
    # LayerMap tests
    lm = RBA::LayerMap.new
    lm.map("1-10/5-20", 0)
    lm.map("3/0", 1)
    assert_equal(lm.is_mapped?(RBA::LayerInfo.new(0, 0)), false)
    assert_equal(lm.is_mapped?(RBA::LayerInfo.new(3, 0)), true)
    assert_equal(lm.is_mapped?(RBA::LayerInfo.new(3, 1)), false)
    assert_equal(lm.is_mapped?(RBA::LayerInfo.new(3, 5)), true)
    assert_equal(lm.is_mapped?(RBA::LayerInfo.new(3, 20)), true)
    assert_equal(lm.is_mapped?(RBA::LayerInfo.new(1, 5)), true)
    assert_equal(lm.is_mapped?(RBA::LayerInfo.new(1, 20)), true)
    assert_equal(lm.mapping_str(1), "3/0")
    assert_equal(lm.mapping(1).to_s, "3/0")
    assert_equal(lm.logical(RBA::LayerInfo.new(3, 0)), 1)

    lm.clear
    assert_equal(lm.is_mapped?(RBA::LayerInfo.new(3, 0)), false)
    lm.map(RBA::LayerInfo.new(3, 0), 2)
    assert_equal(lm.logical(RBA::LayerInfo.new(3, 0)), 2)

    lm.map(RBA::LayerInfo.new(1, 0), RBA::LayerInfo.new(10, 10), 1)
    assert_equal(lm.logical(RBA::LayerInfo.new(3, 0)), 1)
    assert_equal(lm.logical(RBA::LayerInfo.new(3, 3)), 1)
    assert_equal(lm.logical(RBA::LayerInfo.new(1, 3)), 1)
  end

end

load("test_epilogue.rb")
