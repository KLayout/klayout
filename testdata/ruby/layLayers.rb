# encoding: UTF-8

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

if !$:.member?(File::dirname($0))
  $:.push(File::dirname($0))
end

load("test_prologue.rb")

class LAYLayers_TestClass < TestBase

  def lnode_str(space, l)
    return space + l.current.source(true) + "\n";
  end

  def lnodes_str(space, l)
    res = ""
    while ! l.at_end?  
      res += lnode_str(space, l)
      if (l.current.has_children?) 
        l.down_first_child
        res += lnodes_str("  #{space}", l)
        l.up
      end
      l.next_sibling(1)
    end
    return res
  end

  def lnodes_str2(v)
    res = []
    v.each_layer do |c|
      res << c.source(true)
    end
    return res.join("\n")
  end

  def lnodes_str3(v, index)
    res = []
    v.each_layer(index) do |c|
      res << c.source(true)
    end
    return res.join("\n")
  end

  def test_1

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    mw.close_all

    mw.load_layout( ENV["TESTSRC"] + "/testdata/gds/t11.gds", 1 ) 
    mw.load_layout( ENV["TESTSRC"] + "/testdata/gds/t10.gds", 2 ) 

    cv = mw.current_view

    cv.clear_layers

    assert_equal( lnodes_str( "", cv.begin_layers ), "" )

    pos = cv.end_layers
    assert_equal( pos.parent.is_null?, true )
    p = pos.dup
    p.up
    assert_equal( p.is_null?, true )
    assert_equal( pos.is_null?, false )

    assert_equal( pos == cv.begin_layers, true )
    assert_equal( pos != cv.begin_layers, false )

    l1 = cv.insert_layer( pos, RBA::LayerProperties::new )

    assert_equal( pos == cv.begin_layers, true )
    assert_equal( pos != cv.begin_layers, false )
    assert_equal( pos == cv.end_layers, false )
    assert_equal( pos != cv.end_layers, true )
    assert_equal( pos < cv.end_layers, true )
    assert_equal( cv.end_layers < pos, false )
    assert_equal( pos < cv.begin_layers, false )
    assert_equal( cv.begin_layers < pos, false )
    assert_equal( pos.at_top?, true )

    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n" )

    new_p = RBA::LayerProperties::new
    new_p.source = "1/0@1"
    l11 = cv.insert_layer( pos.last_child, new_p )

    p12 = pos.last_child
    assert_equal( p12.parent.is_null?, false )
    assert_equal( p12.parent == pos, true )

    pp = pos.dup
    pp.down_last_child
    assert_equal( pp == p12, true )
    assert_equal( pp == pos, false )
    assert_equal( pp.parent == pos, true )
    pp.up
    assert_equal( pp == pos, true )

    assert_equal( p12.at_top?, false )
    p12.to_sibling( 0 )
    assert_equal( p12 == pos.first_child, true )
    assert_equal( p12.child_index, 0 )
    p12.to_sibling( 1 )
    assert_equal( p12.child_index, 1 )
    assert_equal( p12 == pos.last_child, true )
    assert_equal( p12.num_siblings, 1 )

    l12 = cv.insert_layer( p12, RBA::LayerProperties::new )
    l12_new = RBA::LayerProperties::new
    l12_new.source = "1/0@2"
    cv.set_layer_properties( p12, l12_new )

    assert_equal( p12.current.cellview, 1 )
    assert_equal( p12.current.has_upper_hier_level?(true), false )
    assert_equal( p12.current.has_lower_hier_level?(true), false )

    l12_new.source = "@* #1..2"
    cv.set_layer_properties( p12, l12_new )

    assert_equal( p12.current.cellview, 0 )
    assert_equal( p12.current.has_upper_hier_level?(true), true )
    assert_equal( p12.current.has_upper_hier_level?, true )
    assert_equal( p12.current.upper_hier_level(true), 2 )
    assert_equal( p12.current.upper_hier_level, 2 )
    assert_equal( p12.current.has_lower_hier_level?(true), true )
    assert_equal( p12.current.has_lower_hier_level?, true )
    assert_equal( p12.current.lower_hier_level(true), 1 )
    assert_equal( p12.current.lower_hier_level, 1 )

    l12_new.source = "@* (0,0 *0.5) (0,5 r45 *2.5)"
    cv.set_layer_properties( p12, l12_new )
    trans = p12.current.trans( true )
    assert_equal( trans.map(&:to_s), p12.current.trans.map(&:to_s) )
    assert_equal( trans.size, 2 )
    assert_equal( trans [0].to_s, "r0 *0.5 0,0" )
    assert_equal( trans [1].to_s, "r45 *2.5 0,5" )

    l12_new.source = "1/0@2"
    cv.set_layer_properties( p12, l12_new )

    assert_equal( p12.num_siblings, 2 )

    pos = cv.end_layers

    new_p = RBA::LayerProperties::new
    new_p.source = "@1"
    l2 = cv.insert_layer( pos, new_p )

    new_p = RBA::LayerProperties::new
    new_p.source = "7/0@*"
    l21 = cv.insert_layer( pos.first_child, new_p )

    p22 = pos.last_child
    new_p = RBA::LayerProperties::new
    l22 = cv.insert_layer( pos.last_child, new_p )

    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@1\n  7/0@1\n  */*@1\n" )

    new_p = l2.dup
    new_p.source = "@2"
    cv.set_layer_properties( pos, new_p )

    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n  */*@2\n" )

    pos.first_child.current.source = "7/0@1"
    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@1\n  */*@2\n" )
    pos.current.source = "@*"
    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@*\n  7/0@1\n  */*@*\n" )
    pos.current.source = "@2"
    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@1\n  */*@2\n" )
    pos.first_child.current.source = "7/1@*"
    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/1@2\n  */*@2\n" )
    pos.first_child.current.source = "7/0@*"
    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n  */*@2\n" )

    l22_new = RBA::LayerProperties::new
    l22_new.source = "7/1@*"
    cv.replace_layer_node( p22, l22_new )

    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n  7/1@2\n" )

    cv.delete_layer( p22 )

    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n" )

    new_p = l2.dup
    new_p.source = "%5@2"
    cv.set_layer_properties( pos, new_p )

    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  1/0@2\n%5@2\n  %5@2\n" )

    mw.close_all

  end

  def test_1a

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    mw.close_all

    mw.load_layout( ENV["TESTSRC"] + "/testdata/gds/t11.gds", 1 ) 
    mw.load_layout( ENV["TESTSRC"] + "/testdata/gds/t10.gds", 2 ) 

    cv = mw.current_view

    cv.clear_layers

    cv.insert_layer_list(1)
    cv.rename_layer_list(1, "x")
    assert_equal(cv.current_layer_list, 1)
    cv.set_current_layer_list(0)
    assert_equal(cv.current_layer_list, 0)
    cv.set_current_layer_list(1)
    assert_equal(cv.current_layer_list, 1)

    assert_equal( lnodes_str( "", cv.begin_layers(0) ), "" )
    assert_equal( lnodes_str( "", cv.begin_layers(1) ), "" )

    pos = cv.end_layers(0)
    assert_equal( pos.parent.is_null?, true )
    p = pos.dup
    p.up
    assert_equal( p.is_null?, true )
    assert_equal( pos.is_null?, false )

    assert_equal( pos == cv.begin_layers(0), true )
    assert_equal( pos != cv.begin_layers(0), false )

    l1 = cv.insert_layer( 0, pos, RBA::LayerProperties::new )

    assert_equal( pos == cv.begin_layers(0), true )
    assert_equal( pos != cv.begin_layers(0), false )
    assert_equal( pos == cv.end_layers(0), false )
    assert_equal( pos != cv.end_layers(0), true )
    assert_equal( pos < cv.end_layers(0), true )
    assert_equal( cv.end_layers(0) < pos, false )
    assert_equal( pos < cv.begin_layers(0), false )
    assert_equal( cv.begin_layers(0) < pos, false )
    assert_equal( pos.at_top?, true )

    assert_equal( lnodes_str( "", cv.begin_layers(0) ), "*/*@*\n" )
    assert_equal( lnodes_str( "", cv.begin_layers(1) ), "" )
    assert_equal( lnodes_str( "", cv.begin_layers() ), "" )

    new_p = RBA::LayerProperties::new
    new_p.source = "1/0@1"
    l11 = cv.insert_layer( 0, pos.last_child, new_p )

    p12 = pos.last_child
    assert_equal( p12.parent.is_null?, false )
    assert_equal( p12.parent == pos, true )

    pp = pos.dup
    pp.down_last_child
    assert_equal( pp == p12, true )
    assert_equal( pp == pos, false )
    assert_equal( pp.parent == pos, true )
    pp.up
    assert_equal( pp == pos, true )

    assert_equal( p12.at_top?, false )
    p12.to_sibling( 0 )
    assert_equal( p12 == pos.first_child, true )
    assert_equal( p12.child_index, 0 )
    p12.to_sibling( 1 )
    assert_equal( p12.child_index, 1 )
    assert_equal( p12 == pos.last_child, true )
    assert_equal( p12.num_siblings, 1 )

    l12 = cv.insert_layer( 0, p12, RBA::LayerProperties::new )
    l12_new = RBA::LayerProperties::new
    l12_new.source = "1/0@2"
    cv.set_layer_properties( 0, p12, l12_new )

    assert_equal( p12.current.cellview, 1 )
    assert_equal( p12.current.has_upper_hier_level?(true), false )
    assert_equal( p12.current.has_upper_hier_level?, false )
    assert_equal( p12.current.has_lower_hier_level?(true), false )
    assert_equal( p12.current.has_lower_hier_level?, false )

    l12_new.source = "@* #1..2"
    cv.set_layer_properties( 0, p12, l12_new )

    assert_equal( p12.current.cellview, 0 )
    assert_equal( p12.current.has_upper_hier_level?(true), true )
    assert_equal( p12.current.has_upper_hier_level?, true )
    assert_equal( p12.current.upper_hier_level(true), 2 )
    assert_equal( p12.current.upper_hier_level, 2 )
    assert_equal( p12.current.has_lower_hier_level?(true), true )
    assert_equal( p12.current.has_lower_hier_level?, true )
    assert_equal( p12.current.lower_hier_level(true), 1 )
    assert_equal( p12.current.lower_hier_level, 1 )

    l12_new.source = "@* (0,0 *0.5) (0,5 r45 *2.5)"
    cv.set_layer_properties( 0, p12, l12_new )
    trans = p12.current.trans( true )
    assert_equal( trans.size, 2 )
    assert_equal( trans [0].to_s, "r0 *0.5 0,0" )
    assert_equal( trans [1].to_s, "r45 *2.5 0,5" )

    l12_new.source = "1/0@2"
    cv.set_layer_properties( 0, p12, l12_new )

    assert_equal( p12.num_siblings, 2 )

    pos = cv.end_layers(0)

    new_p = RBA::LayerProperties::new
    new_p.source = "@1"
    l2 = cv.insert_layer( 0, pos, new_p )

    new_p = RBA::LayerProperties::new
    new_p.source = "7/0@*"
    l21 = cv.insert_layer( 0, pos.first_child, new_p )

    p22 = pos.last_child
    new_p = RBA::LayerProperties::new
    l22 = cv.insert_layer( 0, pos.last_child, new_p )

    assert_equal( lnodes_str( "", cv.begin_layers(0) ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@1\n  7/0@1\n  */*@1\n" )
    assert_equal( lnodes_str( "", cv.begin_layers(1) ), "" )
    assert_equal( lnodes_str( "", cv.begin_layers ), "" )

    new_p = l2.dup
    new_p.source = "@2"
    cv.set_layer_properties( 0, pos, new_p )

    assert_equal( lnodes_str( "", cv.begin_layers(0) ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n  */*@2\n" )

    l22_new = RBA::LayerProperties::new
    l22_new.source = "7/1@*"
    cv.replace_layer_node( 0, p22, l22_new )

    assert_equal( lnodes_str( "", cv.begin_layers(0) ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n  7/1@2\n" )

    cv.delete_layer( 0, p22 )

    assert_equal( lnodes_str( "", cv.begin_layers(0) ), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n" )

    new_p = l2.dup
    new_p.source = "%5@2"
    cv.set_layer_properties( 0, pos, new_p )

    assert_equal( lnodes_str( "", cv.begin_layers(0) ), "*/*@*\n  1/0@1\n  1/0@2\n%5@2\n  %5@2\n" )

    # build a tree by building a separate tree
    new_p = RBA::LayerPropertiesNode::new
    assert_equal(new_p.has_children?, false)
    n1 = new_p.add_child(RBA::LayerProperties::new)
    assert_equal(n1.has_children?, false)
    n1.source = "101/0"
    n2 = RBA::LayerPropertiesNode::new
    assert_equal(n2.has_children?, false)
    n21 = n2.add_child(RBA::LayerProperties::new)
    n21.source = "102/0"
    assert_equal(n2.has_children?, true)
    n22 = n2.add_child(RBA::LayerProperties::new)
    assert_equal(n2.has_children?, true)
    n22.source = "103/0"
    new_p.add_child(n2)
    assert_equal(new_p.has_children?, true)

    p = pos.last_child
    ll = cv.insert_layer( 0, p, new_p )
    assert_equal(p.current.has_children?, true)
    assert_equal(p.first_child.current.has_children?, false)
    assert_equal(p.first_child.current.source(false), "101/0@1")
    assert_equal(p.first_child.current.source, "%5@1")

    # (test clear_children):
    new_p.clear_children
    assert_equal(new_p.has_children?, false)

    assert_equal(ll.has_children?, false)

    cv.transaction("Delete")
    li = cv.begin_layers(0)
    a = []
    while !li.at_end?
      a.push(li.dup)
      li.next
    end
    assert_equal(a.size, 10)
    cv.delete_layers(0, a)
    assert_equal(cv.begin_layers(0).at_end?, true)
    cv.commit
    mw.cm_undo 
    assert_equal(cv.begin_layers(0).at_end?, false)

    cv.transaction("Delete")
    i = 0
    while !cv.begin_layers(0).at_end?
      cv.delete_layer(0, cv.begin_layers(0))
      i += 1
    end
    assert_equal(i, 2)
    assert_equal(cv.begin_layers(0).at_end?, true)
    cv.commit
    mw.cm_undo 
    assert_equal(cv.begin_layers(0).at_end?, false)

    mw.close_all

  end

  def test_2

    if !RBA.constants.member?(:Application)
      return
    end

    p = RBA::LayerPropertiesNode::new

    assert_equal( p.source( false ), "*/*@*" )
    assert_equal( p.source, "*/*@*" )
    assert_equal( p.has_source_name?( false ), false )
    assert_equal( p.has_source_name?, false )
    assert_equal( p.has_frame_color?, false )
    assert_equal( p.has_frame_color?(true), false )
    assert_equal( p.has_fill_color?, false )
    assert_equal( p.has_fill_color?(true), false )
    assert_equal( p.has_dither_pattern?, false )
    assert_equal( p.has_dither_pattern?(true), false )
    assert_equal( p.has_line_style?, false )
    assert_equal( p.has_line_style?(true), false )

    p.name = "u"
    assert_equal( p.name, "u" )

    p.source_name = "x"
    assert_equal( p.source_name( false ), "x" )
    assert_equal( p.source_name, "x" )
    assert_equal( p.source( false ), "x@*" )
    assert_equal( p.source, "x@*" )
    assert_equal( p.flat.source, "x@*" )
    assert_equal( p.dup.source, "x@*" )
    assert_equal( p.has_source_name?( false ), true )
    assert_equal( p.has_source_name?, true )

    p.clear_source_name
    assert_equal( p.source( false ), "*/*@*" )
    assert_equal( p.has_source_name?( false ), false )

    p.source_layer_index = 6
    assert_equal( p.source( false ), "%6@*" )
    assert_equal( p.source_layer_index( false ), 6 )
    assert_equal( p.source_layer_index, 6 )

    p.source_layer = 6
    p.source_datatype = 5
    assert_equal( p.source( false ), "%6@*" )

    p.source_layer_index = -1
    assert_equal( p.source( false ), "6/5@*" )
    assert_equal( p.source_layer_index( false ), -1 )
    assert_equal( p.source_layer_index, -1 )
    assert_equal( p.source_layer( false ), 6 )
    assert_equal( p.source_layer, 6 )
    assert_equal( p.source_datatype( false ), 5 )
    assert_equal( p.source_datatype, 5 )

    arr = [ RBA::CplxTrans.new( RBA::CplxTrans::M45 ), RBA::CplxTrans.new( RBA::CplxTrans::R270 ) ]
    p.trans = arr
    assert_equal( p.source( false ), "6/5@* (m45 *1 0,0) (r270 *1 0,0)" )
    assert_equal( arr == p.trans( false ), true )

    p.source_cellview = 1 
    assert_equal( p.source( false ), "6/5@2 (m45 *1 0,0) (r270 *1 0,0)" )
    assert_equal( p.flat.source, "6/5@2 (m45 *1 0,0) (r270 *1 0,0)" )
    assert_equal( p.source_cellview( false ), 1 )
    assert_equal( p.source_cellview, 1 )
    p.source_cellview = -1 
    assert_equal( p.source( false ), "6/5@* (m45 *1 0,0) (r270 *1 0,0)" )

    p.upper_hier_level = 17
    assert_equal( p.source( false ), "6/5@* (m45 *1 0,0) (r270 *1 0,0) #..17" )
    assert_equal( p.upper_hier_level( false ), 17 )
    assert_equal( p.upper_hier_level, 17 )
    assert_equal( p.has_upper_hier_level?( false ), true )
    assert_equal( p.upper_hier_level_relative?, false )
    assert_equal( p.upper_hier_level_relative?(true), false )
    p.set_upper_hier_level(11, true)
    assert_equal( p.upper_hier_level_mode( false ), 0 )
    assert_equal( p.upper_hier_level_mode, 0 )
    assert_equal( p.upper_hier_level, 11 )
    assert_equal( p.upper_hier_level_relative?, true )
    assert_equal( p.upper_hier_level_relative?(true), true )
    p.set_upper_hier_level(11, true, 1)
    assert_equal( p.upper_hier_level, 11 )
    assert_equal( p.upper_hier_level_mode( false ), 1 )
    assert_equal( p.upper_hier_level_mode, 1 )
    p.set_upper_hier_level(11, true, 2)
    assert_equal( p.upper_hier_level_mode( false ), 2 )
    assert_equal( p.upper_hier_level_mode, 2 )
    p.clear_upper_hier_level 
    assert_equal( p.source( false ), "6/5@* (m45 *1 0,0) (r270 *1 0,0)" )
    assert_equal( p.has_upper_hier_level?( false ), false )
    assert_equal( p.has_upper_hier_level?, false )

    p.lower_hier_level = 17
    assert_equal( p.source( false ), "6/5@* (m45 *1 0,0) (r270 *1 0,0) #17.." )
    assert_equal( p.source, "6/5@* (m45 *1 0,0) (r270 *1 0,0) #17.." )
    assert_equal( p.lower_hier_level( false ), 17 )
    assert_equal( p.lower_hier_level, 17 )
    assert_equal( p.has_lower_hier_level?( false ), true )
    assert_equal( p.has_lower_hier_level?, true )
    assert_equal( p.lower_hier_level_relative?, false )
    assert_equal( p.lower_hier_level_relative?(true), false )
    p.set_lower_hier_level(10, true)
    assert_equal( p.lower_hier_level, 10 )
    assert_equal( p.lower_hier_level_relative?, true )
    assert_equal( p.lower_hier_level_relative?(true), true )
    p.set_lower_hier_level(11, true, 1)
    assert_equal( p.lower_hier_level, 11 )
    assert_equal( p.lower_hier_level_mode( false ), 1 )
    assert_equal( p.lower_hier_level_mode, 1 )
    p.set_lower_hier_level(11, true, 2)
    assert_equal( p.lower_hier_level_mode( false ), 2 )
    assert_equal( p.lower_hier_level_mode, 2 )
    p.clear_lower_hier_level 
    assert_equal( p.source( false ), "6/5@* (m45 *1 0,0) (r270 *1 0,0)" )
    assert_equal( p.source, "6/5@* (m45 *1 0,0) (r270 *1 0,0)" )
    assert_equal( p.has_lower_hier_level?( false ), false )
    assert_equal( p.has_lower_hier_level?, false )

    p.dither_pattern = 18
    assert_equal( p.dither_pattern(true), 18)
    assert_equal( p.flat.dither_pattern(true), 18)
    assert_equal( p.dither_pattern, 18)
    assert_equal( p.eff_dither_pattern, 18)
    assert_equal( p.eff_dither_pattern(true), 18)
    assert_equal( p.has_dither_pattern?, true )
    assert_equal( p.has_dither_pattern?(true), true )

    p.line_style = 12
    assert_equal( p.line_style(true), 12)
    assert_equal( p.flat.line_style(true), 12)
    assert_equal( p.line_style, 12)
    assert_equal( p.eff_line_style, 12)
    assert_equal( p.eff_line_style(true), 12)
    assert_equal( p.has_line_style?, true )
    assert_equal( p.has_line_style?(true), true )

    p.animation = 2
    assert_equal( p.animation(true), 2)
    assert_equal( p.flat.animation(true), 2)
    assert_equal( p.animation, 2)

    p.marked = true
    assert_equal( p.marked?(true), true)
    assert_equal( p.flat.marked?(true), true)
    assert_equal( p.marked?, true)

    p.marked = false
    assert_equal( p.marked?(false), false)
    assert_equal( p.flat.marked?(false), false)
    assert_equal( p.marked?, false)

    p.transparent = true
    assert_equal( p.transparent?(true), true)
    assert_equal( p.flat.transparent?(true), true)
    assert_equal( p.transparent?, true)

    p.transparent = false
    assert_equal( p.transparent?(false), false)
    assert_equal( p.flat.transparent?(false), false)
    assert_equal( p.transparent?, false)

    p.visible = true
    assert_equal( p.visible?(true), true)
    assert_equal( p.flat.visible?(true), true)
    assert_equal( p.visible?, true)

    p.visible = false
    assert_equal( p.visible?(false), false)
    assert_equal( p.flat.visible?(false), false)
    assert_equal( p.visible?, false)

    p.valid = true
    assert_equal( p.valid?(true), true)
    assert_equal( p.flat.valid?(true), true)
    assert_equal( p.valid?, true)

    p.valid = false
    assert_equal( p.valid?(false), false)
    assert_equal( p.flat.valid?(false), false)
    assert_equal( p.valid?, false)

    p.xfill = true
    assert_equal( p.xfill?(true), true)
    assert_equal( p.flat.xfill?(true), true)
    assert_equal( p.xfill?, true)

    p.xfill = false
    assert_equal( p.xfill?(false), false)
    assert_equal( p.flat.xfill?(false), false)
    assert_equal( p.xfill?, false)

    p.width = 3
    assert_equal( p.width(true), 3)
    assert_equal( p.flat.width(true), 3)
    assert_equal( p.width, 3)

    p.frame_color = 0xff000031
    assert_equal( p.frame_color(true), 0xff000031)
    assert_equal( p.flat.frame_color(true), 0xff000031)
    assert_equal( p.frame_color, 0xff000031)
    assert_equal( p.has_frame_color?, true )
    assert_equal( p.has_frame_color?(true), true )
    assert_equal( p.has_fill_color?, false )
    assert_equal( p.has_fill_color?(true), false )

    p.fill_color = 0xff000032
    assert_equal( p.fill_color(true), 0xff000032)
    assert_equal( p.flat.fill_color(true), 0xff000032)
    assert_equal( p.fill_color, 0xff000032)
    assert_equal( p.has_frame_color?, true )
    assert_equal( p.has_fill_color?, true )

    p.frame_brightness = 41
    assert_equal( p.frame_brightness(true), 41)
    assert_equal( p.flat.frame_brightness(true), 41)
    assert_equal( p.frame_brightness, 41)

    p.fill_brightness = 42
    assert_equal( p.fill_brightness(true), 42)
    assert_equal( p.flat.fill_brightness(true), 42)
    assert_equal( p.fill_brightness, 42)
    assert_equal( "#%06x" % p.eff_frame_color, "#33335b" )
    assert_equal( "#%06x" % p.eff_fill_color, "#34345c" )
    assert_equal( "#%06x" % p.eff_frame_color(true), "#33335b" )
    assert_equal( "#%06x" % p.eff_fill_color(true), "#34345c" )

    p.clear_fill_color
    assert_equal( p.has_fill_color?, false )
    p.clear_frame_color
    assert_equal( p.has_frame_color?, false )
    p.clear_dither_pattern
    assert_equal( p.has_dither_pattern?, false )
    p.clear_line_style
    assert_equal( p.has_line_style?, false )

    assert_equal( p.is_expanded?, false )
    p.expanded = true
    assert_equal( p.is_expanded?, true )

    pp = RBA::LayerPropertiesNode::new
    assert_equal( pp == p, false )
    assert_equal( pp != p, true )
    assert_equal( pp.is_expanded?, false )
    pp = p.dup
    assert_equal( pp == p, true )
    assert_equal( pp != p, false )
    assert_equal( pp.is_expanded?, true )

  end

  # direct replacement of objects and attributes
  def test_3

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    mw.close_all

    mw.load_layout( ENV["TESTSRC"] + "/testdata/gds/t11.gds", RBA::LoadLayoutOptions::new, "", 1 ) 
    mw.load_layout( ENV["TESTSRC"] + "/testdata/gds/t10.gds", RBA::LoadLayoutOptions::new, "", 2 ) 

    cv = mw.current_view
    assert_equal( lnodes_str( "", cv.begin_layers ), "1/0@1\n2/0@1\n1/0@2\n2/0@2\n3/0@2\n3/1@2\n4/0@2\n5/0@2\n6/0@2\n6/1@2\n7/0@2\n8/0@2\n8/1@2\n" )

    cv.clear_layers

    pos = cv.end_layers
    assert_equal( pos.current.is_valid?, false )

    cv.insert_layer( pos, RBA::LayerProperties::new )
    assert_equal( pos.current.is_valid?, true )
    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n" )
    assert_equal( lnodes_str2(cv), "*/*@*" )

    assert_equal( cv.begin_layers.current.name, "" )
    assert_equal( cv.begin_layers.current.visible?, true )
    assert_equal( cv.begin_layers.current.dither_pattern, -1 )
    assert_equal( cv.begin_layers.current.line_style, -1 )
    assert_equal( cv.begin_layers.current.valid?, true )
    assert_equal( cv.begin_layers.current.transparent?, false )

    # test LayerPropertiesNodeRef
    pos.current.name = "NAME"
    pos.current.visible = false
    pos.current.fill_color = 0xff012345
    pos.current.frame_color = 0xff123456
    pos.current.fill_brightness = 42
    pos.current.frame_brightness = 17
    pos.current.dither_pattern = 4
    pos.current.line_style = 3
    pos.current.valid = false
    pos.current.transparent = true
    pos.current.marked = false
    pos.current.xfill = false
    pos.current.width = 2
    pos.current.animation = 2

    assert_equal( cv.begin_layers.current.name, "NAME" )
    assert_equal( cv.begin_layers.current.visible?, false )
    assert_equal( cv.begin_layers.current.fill_color, 0xff012345 )
    assert_equal( cv.begin_layers.current.frame_color, 0xff123456 )
    assert_equal( cv.begin_layers.current.fill_brightness, 42 )
    assert_equal( cv.begin_layers.current.frame_brightness, 17 )
    assert_equal( cv.begin_layers.current.dither_pattern, 4 )
    assert_equal( cv.begin_layers.current.line_style, 3 )
    assert_equal( cv.begin_layers.current.valid?, false )
    assert_equal( cv.begin_layers.current.transparent?, true )
    assert_equal( cv.begin_layers.current.marked?, false )
    assert_equal( cv.begin_layers.current.xfill?, false )
    assert_equal( cv.begin_layers.current.width, 2 )
    assert_equal( cv.begin_layers.current.animation, 2 )

    new_p = RBA::LayerProperties::new
    new_p.source = "1/0@1"
    assert_equal( new_p.flat.source, "1/0@1" )
    assert_equal( new_p == new_p.flat, true )
    assert_equal( new_p != new_p.flat, false )
    new_p_ref = pos.current.add_child(new_p)
    assert_equal( new_p_ref.layer_index, cv.cellview(0).layout.layer(1, 0) )
    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n" )
    assert_equal( lnodes_str2(cv), "*/*@*\n1/0@1" )

    p = pos.current.add_child
    p.source = "1/0@2"
    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  1/0@2\n" )
    assert_equal( lnodes_str2(cv), "*/*@*\n1/0@1\n1/0@2" )
    assert_equal( p.layer_index, cv.cellview(1).layout.layer(1, 0) )
    assert_equal( p.bbox.to_s, "(-1.4,1.8;25.16,3.8)" )
    assert_equal( p.view == cv, true )
    assert_equal( p.list_index, 0 )

    l12_new = RBA::LayerProperties::new
    l12_new.source = "@* #1..2"
    assert_equal(l12_new.flat.source, "*/*@* #1..2")
    assert_equal(pos.first_child.current.is_valid?, true)
    assert_equal(pos.last_child.current.is_valid?, false)
    pos.first_child.next.current.assign(l12_new)
    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  */*@* #1..2\n" )
    assert_equal( lnodes_str2(cv), "*/*@*\n1/0@1\n*/*@* #1..2" )

    pos.first_child.next_sibling(1).current.source = "@* #3..4"
    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  1/0@1\n  */*@* #3..4\n" )
    assert_equal( lnodes_str2(cv), "*/*@*\n1/0@1\n*/*@* #3..4" )

    pos.first_child.to_sibling(1).next_sibling(-1).current.source = "7/0"
    assert_equal( lnodes_str( "", cv.begin_layers ), "*/*@*\n  7/0@1\n  */*@* #3..4\n" )
    assert_equal( lnodes_str2(cv), "*/*@*\n7/0@1\n*/*@* #3..4" )
    assert_equal( lnodes_str3(cv, 0), "*/*@*\n7/0@1\n*/*@* #3..4" )
    assert_equal( lnodes_str3(cv, 1), "" )

    nn = RBA::LayerPropertiesNode::new
    nn.source = "TOP"

    nn1 = RBA::LayerPropertiesNode::new
    nn1.source = "nn1"

    nn2 = RBA::LayerProperties::new
    nn2.source = "nn1"
    nn1.add_child(nn2)

    nn.add_child(nn1)

    pos.current.assign(nn)
    assert_equal( pos.current.id, nn.id )
    assert_equal( lnodes_str( "", cv.begin_layers ), "TOP@1\n  nn1@1\n    nn1@1\n" )
    assert_equal( lnodes_str2(cv), "TOP@1\nnn1@1\nnn1@1" )

    mw.close_all

  end

  # propagation of "real" attributes through the hierarchy
  def test_4

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    mw.close_all

    mw.load_layout( ENV["TESTSRC"] + "/testdata/gds/t11.gds", 1 ) 
    mw.load_layout( ENV["TESTSRC"] + "/testdata/gds/t10.gds", 2 ) 

    cv = mw.current_view

    cv.clear_layers

    pos = cv.end_layers
    assert_equal( pos.current.is_valid?, false )

    cv.insert_layer(pos, RBA::LayerProperties::new)

    new_p = RBA::LayerProperties::new
    new_p.source = "1/0@1"
    pos.current.add_child(new_p)

    assert_equal(pos.current.visible?(true), true)
    assert_equal(pos.current.visible?(false), true)
    assert_equal(pos.first_child.current.visible?(true), true)
    assert_equal(pos.first_child.current.visible?(false), true)
    pos.current.visible = false
    assert_equal(pos.current.visible?(true), false)
    assert_equal(pos.current.visible?(false), false)
    assert_equal(pos.first_child.current.visible?(true), false)
    assert_equal(pos.first_child.current.visible?(false), true)

    mw.close_all

  end

  # delete method of iterator
  def test_5

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    mw.close_all

    mw.load_layout( ENV["TESTSRC"] + "/testdata/gds/t11.gds", 1 ) 

    cv = mw.current_view
    cv.clear_layers

    new_p = RBA::LayerProperties::new
    new_p.source = "1/0@1"
    cv.insert_layer( 0, cv.end_layers, new_p )

    new_p = RBA::LayerProperties::new
    new_p.source = "2/0@1"
    cv.insert_layer( 0, cv.end_layers, new_p )

    pos = cv.begin_layers

    assert_equal(lnodes_str("", cv.begin_layers), "1/0@1\n2/0@1\n")
    assert_equal(pos.at_end?, false)

    pos.current.delete

    assert_equal(lnodes_str("", cv.begin_layers), "2/0@1\n")
    assert_equal(pos.at_end?, false)

    pos.current.delete

    assert_equal(lnodes_str("", cv.begin_layers), "")
    assert_equal(pos.at_end?, true)

    pos.current.delete

    assert_equal(lnodes_str("", cv.begin_layers), "")
    assert_equal(pos.at_end?, true)

    mw.close_all

  end

  # custom stipples and line styles
  def test_6

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    mw.close_all

    mw.load_layout( ENV["TESTSRC"] + "/testdata/gds/t11.gds", 1 ) 

    cv = mw.current_view

    cv.clear_stipples

    assert_equal(cv.get_stipple(0), "*\n")

    index = cv.add_stipple("something", [ 0x1, 0x2, 0x4, 0x8 ], 4)
    assert_equal(cv.get_stipple(index), "...*\n..*.\n.*..\n*...\n")

    cv.remove_stipple(index)
    assert_equal(cv.get_stipple(index), "*\n")

    index = cv.add_stipple("something", ".**.\n*..*\n.*.*\n*.*.")
    assert_equal(cv.get_stipple(index), ".**.\n*..*\n.*.*\n*.*.\n")

    cv.clear_stipples
    assert_equal(cv.get_stipple(index), "*\n")

    cv.clear_line_styles

    assert_equal(cv.get_line_style(0), "")

    index = cv.add_line_style("something", 0x5, 4)
    assert_equal(cv.get_line_style(index), "*.*.")

    cv.remove_line_style(index)
    assert_equal(cv.get_line_style(index), "")

    index = cv.add_line_style("something", ".**.*..*")
    assert_equal(cv.get_line_style(index), ".**.*..*")

    cv.clear_line_styles
    assert_equal(cv.get_line_style(index), "")

    mw.close_all

  end

  # dynamic update of tree with LayerPropertiesNodeRef
  def test_7

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    mw.close_all

    mw.load_layout( ENV["TESTSRC"] + "/testdata/gds/t11.gds", 1 ) 

    lv = mw.current_view
    lv.clear_layers

    lp = RBA::LayerProperties::new
    lp.source = "A@*"
     
    lp = lv.insert_layer(lv.begin_layers, lp)
     
    lpp = RBA::LayerProperties::new
    lpp = lp.add_child(lpp)
    lpp.source = "A.1"
     
    lppp = RBA::LayerProperties::new
    lppp = lpp.add_child(lppp)
    lppp.source = "A.1.1@*"
     
    lppp = RBA::LayerProperties::new
    lppp = lpp.add_child(lppp)
    li = lv.begin_layers # lp
    li.down_first_child # lpp
    li.down_first_child # before lppp
    li.next # lppp
    li.current.source = "X"
    assert_equal(lppp.source, "X@1")
    lppp.source = "A.1.2"
    assert_equal(li.current.source, "A.1.2@1")
     
    lpp = RBA::LayerProperties::new
    lpp = lp.add_child(lpp)
    lpp.source = "A.2@*"
     
    lppp = RBA::LayerProperties::new
    lppp = lpp.add_child(lppp)
    lppp.source = "A.2.1"
    lppp_saved = lppp

    lppp = RBA::LayerProperties::new
    lppp = lpp.add_child(lppp)
    lppp.source = "A.2.2@*"

    assert_equal(lnodes_str("", lv.begin_layers), 
      "A@*\n" +
      "  A.1@1\n" +
      "    A.1.1@1\n" +
      "    A.1.2@1\n" +
      "  A.2@*\n" +
      "    A.2.1@1\n" +
      "    A.2.2@*\n"
    )

    lp.source = "B@2"

    assert_equal(lnodes_str("", lv.begin_layers), 
      "B@2\n" +
      "  A.1@1\n" +
      "    A.1.1@1\n" +
      "    A.1.2@1\n" +
      "  A.2@2\n" +
      "    A.2.1@1\n" +
      "    A.2.2@2\n"
    )

    lppp_saved.delete

    assert_equal(lnodes_str("", lv.begin_layers), 
      "B@2\n" +
      "  A.1@1\n" +
      "    A.1.1@1\n" +
      "    A.1.2@1\n" +
      "  A.2@2\n" +
      "    A.2.2@2\n"
    )

    assert_equal(lppp.source, "A.2.2@*")

  end

end

load("test_epilogue.rb")
