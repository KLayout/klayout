
$:.push(File::dirname($0))

load("test_prologue.rb")

def mapping_to_s(ly1, ly2, cm)
  r = ""
  ly1.each_cell_top_down do |c|
    s = ly1.cell(c).name
    if cm.has_mapping?(c)
      s += "=>" + ly2.cell(cm.cell_mapping(c)).name
    end
    r == "" || (r += ";")
    r += s
  end
  r
end

class DBCellMapping_TestClass < TestBase

  def test_1

    mp = RBA::CellMapping::new
    mp.map(0, 1)
    assert_equal(mp.has_mapping?(0), true)
    assert_equal(mp.has_mapping?(1), false)
    assert_equal(mp.cell_mapping(0), 1)
    mp.clear
    assert_equal(mp.has_mapping?(0), false)
    assert_equal(mp.has_mapping?(1), false)
    mp.map(1, 2)
    assert_equal(mp.cell_mapping(1), 2)

    ly = RBA::Layout::new

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )
    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst_1 = RBA::CellInstArray::new( c1.cell_index, tr )
    new_inst_1 = c2.insert( inst_1 )
    new_inst_2 = c2.insert( inst_1 )
    inst_2 = RBA::CellInstArray::new( c3.cell_index, tr*tr )
    new_inst_3 = c1.insert( inst_2 )

    top1 = ci2

    ly1 = ly

    ly = RBA::Layout::new

    ci0 = ly.add_cell( "c0" )
    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )
    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst_1 = RBA::CellInstArray::new( c1.cell_index, tr )
    new_inst_1 = c2.insert( inst_1 )
    new_inst_2 = c2.insert( inst_1 )
    inst_2 = RBA::CellInstArray::new( c3.cell_index, tr*tr )
    new_inst_3 = c1.insert( inst_2 )

    top2 = ci2

    ly2 = ly

    mp = RBA::CellMapping::new
    mp.from_names(ly1, top1, ly2, top2)
    assert_equal(mapping_to_s(ly2, ly1, mp), "c0;c2=>c2;c1=>c1;c3=>c3")

    mp = RBA::CellMapping::new
    mp.from_geometry(ly1, top1, ly2, top2)
    assert_equal(mapping_to_s(ly2, ly1, mp), "c0;c2=>c2;c1=>c1;c3=>c3")

    ly = RBA::Layout::new

    ci0 = ly.add_cell( "c0" )
    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "cx" )
    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst_1 = RBA::CellInstArray::new( c1.cell_index, tr )
    new_inst_1 = c2.insert( inst_1 )
    new_inst_2 = c2.insert( inst_1 )
    inst_2 = RBA::CellInstArray::new( c3.cell_index, tr*tr )
    new_inst_3 = c1.insert( inst_2 )

    top2 = ci2

    ly2 = ly

    mp = RBA::CellMapping::new
    mp.from_names(ly1, top1, ly2, top2)
    assert_equal(mapping_to_s(ly2, ly1, mp), "c0;c2=>c2;c1=>c1;cx")

    ly1dup = ly1.dup
    mp = RBA::CellMapping::new
    nc = mp.from_names_full(ly1dup, top1, ly2, top2)
    assert_equal(mapping_to_s(ly2, ly1dup, mp), "c0;c2=>c2;c1=>c1;cx=>cx")
    assert_equal(nc.inspect, "[3]")

    mp = RBA::CellMapping::new
    mp.from_geometry(ly1, top1, ly2, top2)
    assert_equal(mapping_to_s(ly2, ly1, mp), "c0;c2=>c2;c1=>c1;cx=>c3")

    ly = RBA::Layout::new

    ci0 = ly.add_cell( "c0" )
    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )
    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst_1 = RBA::CellInstArray::new( c1.cell_index, tr )
    new_inst_1 = c2.insert( inst_1 )
    new_inst_2 = c2.insert( inst_1 )
    inst_2 = RBA::CellInstArray::new( c3.cell_index, tr )
    new_inst_3 = c1.insert( inst_2 )

    top2 = ci2

    ly2 = ly

    mp = RBA::CellMapping::new
    mp.from_names(ly1, top1, ly2, top2)
    assert_equal(mapping_to_s(ly2, ly1, mp), "c0;c2=>c2;c1=>c1;c3=>c3")

    mp = RBA::CellMapping::new
    mp.from_geometry(ly1, top1, ly2, top2)
    assert_equal(mapping_to_s(ly2, ly1, mp), "c0;c2=>c2;c1=>c1;c3")

    ly1dup = ly1.dup
    mp = RBA::CellMapping::new
    nc = mp.from_geometry_full(ly1dup, top1, ly2, top2)
    assert_equal(mapping_to_s(ly2, ly1dup, mp), "c0;c2=>c2;c1=>c1;c3=>c3$1")
    assert_equal(nc.inspect, "[3]")

  end

end

load("test_epilogue.rb")
