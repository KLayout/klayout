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

def a2s( a )
  sa = []
  a.each { |x| sa.push( x.to_s ) }
  return sa.join(";")
end

def sh2s( sh )
  sa = []
  sh.each do |s|
    sa.push( s.polygon.to_s )
  end
  return sa.join(";")
end

def p2ee( pa )
  ee = []
  pa.each do |p|
    p.each_edge { |e| ee.push( e ) }
  end
  return ee
end

class DBBoolean_TestClass < TestBase

  # simple merge
  def test_1

    ep = RBA::EdgeProcessor::new
    ee = ep.simple_merge_p2e( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 2000, 1000 ),
      RBA::Point::new( 2000, 2000 ),
      RBA::Point::new( 1000, 2000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ] )

    assert_equal( a2s(ee), "(0,0;0,1000);(1000,0;0,0);(1000,1000;1000,0);(0,1000;1000,1000);(1000,1000;1000,2000);(2000,1000;1000,1000);(2000,2000;2000,1000);(1000,2000;2000,2000)" )

    ee = ep.simple_merge_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 2000, 1000 ),
      RBA::Point::new( 2000, 2000 ),
      RBA::Point::new( 1000, 2000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], true, true )

    assert_equal( a2s(ee), "(0,0;0,1000;1000,1000;1000,0);(1000,1000;1000,2000;2000,2000;2000,1000)" )

    ee = ep.simple_merge_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 2000, 1000 ),
      RBA::Point::new( 2000, 2000 ),
      RBA::Point::new( 1000, 2000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], true, false )

    assert_equal( a2s(ee), "(0,0;0,1000;1000,1000;1000,2000;2000,2000;2000,1000;1000,1000;1000,0)" )

    ee = ep.simple_merge_e2e( [ 
      RBA::Edge::new( 0, 0, 0, 1000 ),
      RBA::Edge::new( 0, 1000, 1000, 1000 ),
      RBA::Edge::new( 1000, 1000, 1000, 0 ),
      RBA::Edge::new( 1000, 0, 0, 0 ),
      RBA::Edge::new( 0, 900, 0, 0 ),
      RBA::Edge::new( 900, 900, 0, 900 ),
      RBA::Edge::new( 900, 0, 900, 900 ),
      RBA::Edge::new( 0, 0, 900, 0 ),
    ] )

    assert_equal( a2s(ee), "(900,0;900,900);(1000,0;900,0);(1000,1000;1000,0);(0,900;0,1000);(900,900;0,900);(0,1000;1000,1000)" )

    ee = ep.simple_merge_e2p( [ 
      RBA::Edge::new( 0, 0, 0, 1000 ),
      RBA::Edge::new( 0, 1000, 1000, 1000 ),
      RBA::Edge::new( 1000, 1000, 1000, 0 ),
      RBA::Edge::new( 1000, 0, 0, 0 ),
      RBA::Edge::new( 100, 900, 100, 100 ),
      RBA::Edge::new( 900, 900, 100, 900 ),
      RBA::Edge::new( 900, 100, 900, 900 ),
      RBA::Edge::new( 100, 100, 900, 100 ),
    ], true, true )

    assert_equal( a2s(ee), "(0,0;0,900;100,900;100,100;900,100;900,900;0,900;0,1000;1000,1000;1000,0)" )

    ee = ep.simple_merge_e2p( [ 
      RBA::Edge::new( 0, 0, 0, 1000 ),
      RBA::Edge::new( 0, 1000, 1000, 1000 ),
      RBA::Edge::new( 1000, 1000, 1000, 0 ),
      RBA::Edge::new( 1000, 0, 0, 0 ),
      RBA::Edge::new( 100, 900, 100, 100 ),
      RBA::Edge::new( 900, 900, 100, 900 ),
      RBA::Edge::new( 900, 100, 900, 900 ),
      RBA::Edge::new( 100, 100, 900, 100 ),
    ], false, true )

    assert_equal( a2s(ee), "(0,0;0,1000;1000,1000;1000,0/100,100;900,100;900,900;100,900)" )

  end

  # merge
  def test_2

    ep = RBA::EdgeProcessor::new
    ee = ep.merge_p2e( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 2000, 1000 ),
      RBA::Point::new( 2000, 2000 ),
      RBA::Point::new( 1000, 2000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 0 )

    assert_equal( a2s(ee), "(0,0;0,1000);(1000,0;0,0);(1000,1000;1000,0);(0,1000;1000,1000);(1000,1000;1000,2000);(2000,1000;1000,1000);(2000,2000;2000,1000);(1000,2000;2000,2000)" )

    ep = RBA::EdgeProcessor::new
    ee = ep.merge_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 2000 ),
      RBA::Point::new( 2000, 2000 ),
      RBA::Point::new( 2000, 1000 )
    ] ) ], 0, true, false )

    assert_equal( a2s(ee), "(0,0;0,1000;1000,1000;1000,2000;2000,2000;2000,1000;1000,1000;1000,0)" )

    ep = RBA::EdgeProcessor::new
    ee = ep.merge_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 2000 ),
      RBA::Point::new( 2000, 2000 ),
      RBA::Point::new( 2000, 1000 )
    ] ) ], 0, true, true )

    assert_equal( a2s(ee), "(0,0;0,1000;1000,1000;1000,0);(1000,1000;1000,2000;2000,2000;2000,1000)" )

    ep = RBA::EdgeProcessor::new
    ee = ep.merge_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 100 ),
      RBA::Point::new( 1000, 100 ),
      RBA::Point::new( 1000, 0 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 0, 900 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 900 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 100, 1000 ),
      RBA::Point::new( 100, 0 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 1000, 0 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 900, 1000 ),
      RBA::Point::new( 900, 0 )
    ] ) ], 0, true, false )

    assert_equal( a2s(ee), "(0,0;0,900;100,900;100,100;900,100;900,900;0,900;0,1000;1000,1000;1000,0)" )

    ep = RBA::EdgeProcessor::new
    ee = ep.merge_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 100 ),
      RBA::Point::new( 1000, 100 ),
      RBA::Point::new( 1000, 0 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 0, 900 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 900 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 100, 1000 ),
      RBA::Point::new( 100, 0 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 1000, 0 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 900, 1000 ),
      RBA::Point::new( 900, 0 )
    ] ) ], 0, false, false )

    assert_equal( a2s(ee), "(0,0;0,1000;1000,1000;1000,0/100,100;900,100;900,900;100,900)" )

    ep = RBA::EdgeProcessor::new
    ee = ep.merge_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 100 ),
      RBA::Point::new( 1000, 100 ),
      RBA::Point::new( 1000, 0 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 0, 900 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 900 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 100, 1000 ),
      RBA::Point::new( 100, 0 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 1000, 0 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 900, 1000 ),
      RBA::Point::new( 900, 0 )
    ] ) ], 1, false, false )

    assert_equal( a2s(ee), "(0,0;0,100;100,100;100,0);(900,0;900,100;1000,100;1000,0);(0,900;0,1000;100,1000;100,900);(900,900;900,1000;1000,1000;1000,900)" )

    ep = RBA::EdgeProcessor::new
    ee = ep.merge_p2e( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 100 ),
      RBA::Point::new( 1000, 100 ),
      RBA::Point::new( 1000, 0 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 0, 900 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 900 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 100, 1000 ),
      RBA::Point::new( 100, 0 )
    ] ),
    RBA::Polygon::new( [
      RBA::Point::new( 1000, 0 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 900, 1000 ),
      RBA::Point::new( 900, 0 )
    ] ) ], 1 )

    assert_equal( a2s(ee), "(0,0;0,100);(100,0;0,0);(100,100;100,0);(900,0;900,100);(1000,0;900,0);(1000,100;1000,0);(0,100;100,100);(900,100;1000,100);(0,900;0,1000);(100,900;0,900);(100,1000;100,900);(900,900;900,1000);(1000,900;900,900);(1000,1000;1000,900);(0,1000;100,1000);(900,1000;1000,1000)" )

  end

  # size
  def test_3

    ep = RBA::EdgeProcessor::new

    ee = ep.size_p2e( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 100, 100, 0 )

    assert_equal( a2s(ee), "(0,-100;-100,0);(1000,-100;0,-100);(1100,0;1000,-100);(-100,0;-100,1000);(1100,1000;1100,0);(-100,1000;0,1100);(1000,1100;1100,1000);(0,1100;1000,1100)" )

    ee = ep.size_p2e( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 100, 100, 2 )

    assert_equal( a2s(ee), "(-100,-100;-100,1100);(1100,-100;-100,-100);(1100,1100;1100,-100);(-100,1100;1100,1100)" )

    ee = ep.size_p2e( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 100, 2 )

    assert_equal( a2s(ee), "(-100,-100;-100,1100);(1100,-100;-100,-100);(1100,1100;1100,-100);(-100,1100;1100,1100)" )

    ee = ep.size_p2e( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], -100, 2 )

    assert_equal( a2s(ee), "(100,100;100,900);(900,100;100,100);(900,900;900,100);(100,900;900,900)" )

    ee = ep.size_p2e( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 0, 100, 2 )

    assert_equal( a2s(ee), "(0,-100;0,1100);(1000,-100;0,-100);(1000,1100;1000,-100);(0,1100;1000,1100)" )

    ee = ep.size_p2e( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 100, 0, 2 )

    assert_equal( a2s(ee), "(-100,0;-100,1000);(1100,0;-100,0);(1100,1000;1100,0);(-100,1000;1100,1000)" )

    ee = ep.size_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 100, 100, 0, true, true )

    assert_equal( a2s(ee), "(0,-100;-100,0;-100,1000;0,1100;1000,1100;1100,1000;1100,0;1000,-100)" )

    ee = ep.size_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 100, 100, 2, true, true )

    assert_equal( a2s(ee), "(-100,-100;-100,1100;1100,1100;1100,-100)" )

    ee = ep.size_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 100, 2, true, true )

    assert_equal( a2s(ee), "(-100,-100;-100,1100;1100,1100;1100,-100)" )

    ee = ep.size_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], -100, 2, true, true )

    assert_equal( a2s(ee), "(100,100;100,900;900,900;900,100)" )

    ee = ep.size_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 0, 100, 2, true, true )

    assert_equal( a2s(ee), "(0,-100;0,1100;1000,1100;1000,-100)" )

    ee = ep.size_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 100, 0, 2, true, true )

    assert_equal( a2s(ee), "(-100,0;-100,1000;1100,1000;1100,0)" )

  end

  # boolean
  def test_4

    ep = RBA::EdgeProcessor::new

    ee = ep.boolean_p2e( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 
    [ RBA::Polygon::new( [
      RBA::Point::new( 100, 100 ),
      RBA::Point::new( 100, 1100 ),
      RBA::Point::new( 1100, 1100 ),
      RBA::Point::new( 1100, 100 )
    ] ) ], 
    RBA::EdgeProcessor::mode_or() )

    assert_equal( a2s(ee), "(0,0;0,1000);(1000,0;0,0);(1000,100;1000,0);(1100,100;1000,100);(1100,1100;1100,100);(0,1000;100,1000);(100,1000;100,1100);(100,1100;1100,1100)" )

    ee = ep.boolean_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 
    [ RBA::Polygon::new( [
      RBA::Point::new( 100, 100 ),
      RBA::Point::new( 100, 1100 ),
      RBA::Point::new( 1100, 1100 ),
      RBA::Point::new( 1100, 100 )
    ] ) ], 
    RBA::EdgeProcessor::mode_or(), false, true )

    assert_equal( a2s(ee), "(0,0;0,1000;100,1000;100,1100;1100,1100;1100,100;1000,100;1000,0)" )

    ee = ep.boolean_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 
    [ RBA::Polygon::new( [
      RBA::Point::new( 100, 100 ),
      RBA::Point::new( 100, 1100 ),
      RBA::Point::new( 1100, 1100 ),
      RBA::Point::new( 1100, 100 )
    ] ) ], 
    RBA::EdgeProcessor::mode_and(), false, true )

    assert_equal( a2s(ee), "(100,100;100,1000;1000,1000;1000,100)" )

    ee = ep.boolean_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 
    [ RBA::Polygon::new( [
      RBA::Point::new( 100, 100 ),
      RBA::Point::new( 100, 1100 ),
      RBA::Point::new( 1100, 1100 ),
      RBA::Point::new( 1100, 100 )
    ] ) ], 
    RBA::EdgeProcessor::mode_anotb(), false, true )

    assert_equal( a2s(ee), "(0,0;0,1000;100,1000;100,100;1000,100;1000,0)" )

    ee = ep.boolean_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 
    [ RBA::Polygon::new( [
      RBA::Point::new( 100, 100 ),
      RBA::Point::new( 100, 1100 ),
      RBA::Point::new( 1100, 1100 ),
      RBA::Point::new( 1100, 100 )
    ] ) ], 
    RBA::EdgeProcessor::mode_bnota(), false, true )

    assert_equal( a2s(ee), "(1000,100;1000,1000;100,1000;100,1100;1100,1100;1100,100)" )

    ee = ep.boolean_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 
    [ RBA::Polygon::new( [
      RBA::Point::new( 100, 100 ),
      RBA::Point::new( 100, 1100 ),
      RBA::Point::new( 1100, 1100 ),
      RBA::Point::new( 1100, 100 )
    ] ) ], 
    RBA::EdgeProcessor::mode_xor(), false, true )

    assert_equal( a2s(ee), "(0,0;0,1000;100,1000;100,100;1000,100;1000,0);(1000,100;1000,1000;100,1000;100,1100;1100,1100;1100,100)" )

    ee = ep.boolean_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 
    [ RBA::Polygon::new( [
      RBA::Point::new( 100, 100 ),
      RBA::Point::new( 100, 1100 ),
      RBA::Point::new( 1100, 1100 ),
      RBA::Point::new( 1100, 100 )
    ] ) ], 
    RBA::EdgeProcessor::mode_xor(), false, false )

    assert_equal( a2s(ee), "(0,0;0,1000;100,1000;100,1100;1100,1100;1100,100;1000,100;1000,0/100,100;1000,100;1000,1000;100,1000)" )

    ee = ep.boolean_p2p( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ], 
    [ RBA::Polygon::new( [
      RBA::Point::new( 100, 100 ),
      RBA::Point::new( 100, 1100 ),
      RBA::Point::new( 1100, 1100 ),
      RBA::Point::new( 1100, 100 )
    ] ) ], 
    RBA::EdgeProcessor::mode_xor(), true, false )

    assert_equal( a2s(ee), "(0,0;0,1000;100,1000;100,100;1000,100;1000,1000;100,1000;100,1100;1100,1100;1100,100;1000,100;1000,0)" )

    ee = ep.boolean_e2e( p2ee( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ] ), 
    p2ee( [ RBA::Polygon::new( [
      RBA::Point::new( 100, 100 ),
      RBA::Point::new( 100, 1100 ),
      RBA::Point::new( 1100, 1100 ),
      RBA::Point::new( 1100, 100 )
    ] ) ] ), 
    RBA::EdgeProcessor::mode_or() )

    assert_equal( a2s(ee), "(0,0;0,1000);(1000,0;0,0);(1000,100;1000,0);(1100,100;1000,100);(1100,1100;1100,100);(0,1000;100,1000);(100,1000;100,1100);(100,1100;1100,1100)" )

    ee = ep.boolean_e2p( p2ee( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ] ), 
    p2ee( [ RBA::Polygon::new( [
      RBA::Point::new( 100, 100 ),
      RBA::Point::new( 100, 1100 ),
      RBA::Point::new( 1100, 1100 ),
      RBA::Point::new( 1100, 100 )
    ] ) ] ), 
    RBA::EdgeProcessor::mode_or(), false, true )

    assert_equal( a2s(ee), "(0,0;0,1000;100,1000;100,1100;1100,1100;1100,100;1000,100;1000,0)" )

    ee = ep.boolean_e2p( p2ee( [ RBA::Polygon::new( [
      RBA::Point::new( 0, 0 ),
      RBA::Point::new( 0, 1000 ),
      RBA::Point::new( 1000, 1000 ),
      RBA::Point::new( 1000, 0 )
    ] ) ] ), 
    p2ee( [ RBA::Polygon::new( [
      RBA::Point::new( 100, 100 ),
      RBA::Point::new( 100, 1100 ),
      RBA::Point::new( 1100, 1100 ),
      RBA::Point::new( 1100, 100 )
    ] ) ] ), 
    RBA::EdgeProcessor::mode_xor(), true, false )

    assert_equal( a2s(ee), "(0,0;0,1000;100,1000;100,100;1000,100;1000,1000;100,1000;100,1100;1100,1100;1100,100;1000,100;1000,0)" )

  end

  # shape merge
  def test_10

    lay = RBA::Layout::new
    cid_top = lay.add_cell( "TOP" )
    cid_a = lay.add_cell( "A" )
    li = RBA::LayerInfo::new
    lid_a = lay.insert_layer( li )
    lid_b = lay.insert_layer( li )
    lay.cell( cid_top ).insert( RBA::CellInstArray::new( cid_a, RBA::Trans::new( 0, 0 ) ) )
    lay.cell( cid_top ).insert( RBA::CellInstArray::new( cid_a, RBA::Trans::new( 200, 200 ) ) )
    lay.cell( cid_a ).shapes( lid_a ).insert( RBA::Box::new( 0, 0, 1000, 1000 ) )
    lay.cell( cid_a ).shapes( lid_b ).insert( RBA::Box::new( 100, 100, 300, 300 ) )

    sp = RBA::ShapeProcessor::new

    sh = RBA::Shapes::new

    sp.merge( lay, lay.cell( cid_top ), lid_a, sh, true, 0, false, true )
    assert_equal( sh2s(sh), "(0,0;0,1000;200,1000;200,1200;1200,1200;1200,200;1000,200;1000,0)" )

    sp.merge( lay, lay.cell( cid_top ), lid_a, sh, true, 1, false, true )
    assert_equal( sh2s(sh), "(200,200;200,1000;1000,1000;1000,200)" )

    sp.merge( lay, lay.cell( cid_top ), lid_a, sh, false, 0, false, true )
    assert_equal( sh2s(sh), "" )

    sp.merge( lay, lay.cell( cid_a ), lid_a, sh, false, 0, false, true )
    assert_equal( sh2s(sh), "(0,0;0,1000;1000,1000;1000,0)" )

    sp.merge( lay, lay.cell( cid_top ), lid_b, sh, true, 0, false, true )
    assert_equal( sh2s(sh), "(100,100;100,300;300,300;300,100);(300,300;300,500;500,500;500,300)" )

    sp.merge( lay, lay.cell( cid_top ), lid_b, sh, true, 0, false, false )
    assert_equal( sh2s(sh), "(100,100;100,300;300,300;300,500;500,500;500,300;300,300;300,100)" )

    sha = []
    lay.cell( cid_a ).shapes( lid_b ).each { |s| sha.push( s ) }
    ee = sp.merge( sha, 0 )

    assert_equal( a2s(ee), "(100,100;100,300);(300,100;100,100);(300,300;300,100);(100,300;300,300)" )

    ee = sp.merge_to_polygon( sha, 0, true, false )

    assert_equal( a2s(ee), "(100,100;100,300;300,300;300,100)" )

    sha = []
    tt = []
    lay.cell( cid_a ).shapes( lid_a ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 0, 0 ) ) ) }
    lay.cell( cid_a ).shapes( lid_a ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 200, 200 ) ) ) }
    ee = sp.merge( sha, tt, 0 )

    assert_equal( a2s(ee), "(0,0;0,1000);(1000,0;0,0);(1000,200;1000,0);(1200,200;1000,200);(1200,1200;1200,200);(0,1000;200,1000);(200,1000;200,1200);(200,1200;1200,1200)" )

    sha = []
    tt = []
    lay.cell( cid_a ).shapes( lid_a ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 0, 0 ) ) ) }
    lay.cell( cid_a ).shapes( lid_a ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 200, 200 ) ) ) }
    ee = sp.merge_to_polygon( sha, tt, 0, true, false )

    assert_equal( a2s(ee), "(0,0;0,1000;200,1000;200,1200;1200,1200;1200,200;1000,200;1000,0)" )

    sha = []
    tt = []
    lay.cell( cid_a ).shapes( lid_a ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 0, 0 ) ) ) }
    lay.cell( cid_a ).shapes( lid_a ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 200, 200 ) ) ) }
    ee = sp.merge_to_polygon( sha, tt, 1, true, false )

    assert_equal( a2s(ee), "(200,200;200,1000;1000,1000;1000,200)" )

    sha = []
    tt = []
    lay.cell( cid_a ).shapes( lid_a ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 0, 0 ) ) ) }
    lay.cell( cid_a ).shapes( lid_a ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 200, 200 ) ) ) }
    ee = sp.merge( sha, tt, 1 )

    assert_equal( a2s(ee), "(200,200;200,1000);(1000,200;200,200);(1000,1000;1000,200);(200,1000;1000,1000)" )

    sha = []
    tt = []
    lay.cell( cid_a ).shapes( lid_b ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 0, 0 ) ) ) }
    lay.cell( cid_a ).shapes( lid_b ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 200, 200 ) ) ) }
    ee = sp.merge_to_polygon( sha, tt, 0, true, false )

    assert_equal( a2s(ee), "(100,100;100,300;300,300;300,500;500,500;500,300;300,300;300,100)" )

    sha = []
    tt = []
    lay.cell( cid_a ).shapes( lid_b ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 0, 0 ) ) ) }
    lay.cell( cid_a ).shapes( lid_b ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 200, 200 ) ) ) }
    ee = sp.merge_to_polygon( sha, tt, 0, true, true )

    assert_equal( a2s(ee), "(100,100;100,300;300,300;300,100);(300,300;300,500;500,500;500,300)" )

  end

  # shape boolean
  def test_11

    lay = RBA::Layout::new
    cid_top = lay.add_cell( "TOP" )
    cid_a = lay.add_cell( "A" )
    li = RBA::LayerInfo::new
    lid_a = lay.insert_layer( li )
    lid_b = lay.insert_layer( li )
    lay.cell( cid_top ).insert( RBA::CellInstArray::new( cid_a, RBA::Trans::new( 0, 0 ) ) )
    lay.cell( cid_top ).insert( RBA::CellInstArray::new( cid_a, RBA::Trans::new( 200, 200 ) ) )
    lay.cell( cid_a ).shapes( lid_a ).insert( RBA::Box::new( 0, 0, 1000, 1000 ) )
    lay.cell( cid_a ).shapes( lid_b ).insert( RBA::Box::new( 100, 100, 300, 300 ) )

    sp = RBA::ShapeProcessor::new

    sh = RBA::Shapes::new

    sp.boolean( lay, lay.cell( cid_top ), lid_a, lay, lay.cell( cid_top ), lid_b, sh, RBA::EdgeProcessor::mode_and(), true, false, true )
    assert_equal( sh2s(sh), "(100,100;100,300;300,300;300,100);(300,300;300,500;500,500;500,300)" )

    sp.boolean( lay, lay.cell( cid_top ), lid_a, lay, lay.cell( cid_top ), lid_b, sh, RBA::EdgeProcessor::mode_and(), true, false, false )
    assert_equal( sh2s(sh), "(100,100;100,300;300,300;300,500;500,500;500,300;300,300;300,100)" )

    sp.boolean( lay, lay.cell( cid_top ), lid_a, lay, lay.cell( cid_top ), lid_b, sh, RBA::EdgeProcessor::mode_and(), false, false, true )
    assert_equal( sh2s(sh), "" )

    sp.boolean( lay, lay.cell( cid_a ), lid_a, lay, lay.cell( cid_a ), lid_b, sh, RBA::EdgeProcessor::mode_and(), true, false, true )
    assert_equal( sh2s(sh), "(100,100;100,300;300,300;300,100)" )

    sp.boolean( lay, lay.cell( cid_a ), lid_a, lay, lay.cell( cid_a ), lid_b, sh, RBA::EdgeProcessor::mode_xor(), true, false, true )
    assert_equal( sh2s(sh), "(0,0;0,1000;1000,1000;1000,0/100,100;300,100;300,300;100,300)" )

    sp.boolean( lay, lay.cell( cid_a ), lid_a, lay, lay.cell( cid_a ), lid_b, sh, RBA::EdgeProcessor::mode_xor(), true, true, true )
    assert_equal( sh2s(sh), "(0,0;0,300;100,300;100,100;300,100;300,300;0,300;0,1000;1000,1000;1000,0)" )

    sha = []
    tta = []
    lay.cell( cid_a ).shapes( lid_a ).each { |s| sha.push( s ); tta.push( RBA::CplxTrans::new( RBA::Trans::new( 0, 0 ) ) ) }

    shb = []
    ttb = []
    lay.cell( cid_a ).shapes( lid_b ).each { |s| shb.push( s ); ttb.push( RBA::CplxTrans::new( RBA::Trans::new( 100, 100 ) ) ) }

    ee = sp.boolean( sha, shb, RBA::EdgeProcessor::mode_and() )
    assert_equal( a2s(ee), "(100,100;100,300);(300,100;100,100);(300,300;300,100);(100,300;300,300)" )

    ee = sp.boolean( sha, shb, RBA::EdgeProcessor::mode_or() )
    assert_equal( a2s(ee), "(0,0;0,1000);(1000,0;0,0);(1000,1000;1000,0);(0,1000;1000,1000)" )

    ee = sp.boolean( sha, tta, shb, ttb, RBA::EdgeProcessor::mode_and() )
    assert_equal( a2s(ee), "(200,200;200,400);(400,200;200,200);(400,400;400,200);(200,400;400,400)" )

    ee = sp.boolean( sha, tta, shb, ttb, RBA::EdgeProcessor::mode_or() )
    assert_equal( a2s(ee), "(0,0;0,1000);(1000,0;0,0);(1000,1000;1000,0);(0,1000;1000,1000)" )

    ee = sp.boolean_to_polygon( sha, shb, RBA::EdgeProcessor::mode_and(), true, false )
    assert_equal( a2s(ee), "(100,100;100,300;300,300;300,100)" )

    ee = sp.boolean_to_polygon( sha, shb, RBA::EdgeProcessor::mode_or(), true, false )
    assert_equal( a2s(ee), "(0,0;0,1000;1000,1000;1000,0)" )

    ee = sp.boolean_to_polygon( sha, tta, shb, ttb, RBA::EdgeProcessor::mode_and(), true, false )
    assert_equal( a2s(ee), "(200,200;200,400;400,400;400,200)" )

    ee = sp.boolean_to_polygon( sha, tta, shb, ttb, RBA::EdgeProcessor::mode_or(), true, false )
    assert_equal( a2s(ee), "(0,0;0,1000;1000,1000;1000,0)" )

    ee = sp.boolean_to_polygon( sha, tta, shb, ttb, RBA::EdgeProcessor::mode_xor(), true, false )
    assert_equal( a2s(ee), "(0,0;0,400;200,400;200,200;400,200;400,400;0,400;0,1000;1000,1000;1000,0)" )

    ee = sp.boolean_to_polygon( sha, tta, shb, ttb, RBA::EdgeProcessor::mode_xor(), false, false )
    assert_equal( a2s(ee), "(0,0;0,1000;1000,1000;1000,0/200,200;400,200;400,400;200,400)" )

    ee = sp.boolean_to_polygon( sha, shb, RBA::EdgeProcessor::mode_xor(), false, true )
    assert_equal( a2s(ee), "(0,0;0,1000;1000,1000;1000,0/100,100;300,100;300,300;100,300)" )

    ee = sp.boolean_to_polygon( sha, shb, RBA::EdgeProcessor::mode_xor(), true, true )
    assert_equal( a2s(ee), "(0,0;0,300;100,300;100,100;300,100;300,300;0,300;0,1000;1000,1000;1000,0)" )

    shb = []
    ttb = []
    lay.cell( cid_a ).shapes( lid_b ).each { |s| shb.push( s ); ttb.push( RBA::CplxTrans::new( RBA::Trans::new( 900, 900 ) ) ) }

    ee = sp.boolean_to_polygon( sha, tta, shb, ttb, RBA::EdgeProcessor::mode_or(), true, false )
    assert_equal( a2s(ee), "(0,0;0,1000;1000,1000;1000,1200;1200,1200;1200,1000;1000,1000;1000,0)" )

    ee = sp.boolean_to_polygon( sha, tta, shb, ttb, RBA::EdgeProcessor::mode_or(), true, true )
    assert_equal( a2s(ee), "(0,0;0,1000;1000,1000;1000,0);(1000,1000;1000,1200;1200,1200;1200,1000)" )

  end

  # shape sizing
  def test_12

    lay = RBA::Layout::new
    cid_top = lay.add_cell( "TOP" )
    cid_a = lay.add_cell( "A" )
    li = RBA::LayerInfo::new
    lid_a = lay.insert_layer( li )
    lid_b = lay.insert_layer( li )
    lay.cell( cid_top ).insert( RBA::CellInstArray::new( cid_a, RBA::Trans::new( 0, 0 ) ) )
    lay.cell( cid_top ).insert( RBA::CellInstArray::new( cid_a, RBA::Trans::new( 200, 200 ) ) )
    lay.cell( cid_a ).shapes( lid_a ).insert( RBA::Box::new( 0, 0, 1000, 1000 ) )
    lay.cell( cid_a ).shapes( lid_b ).insert( RBA::Box::new( 100, 100, 300, 300 ) )

    sp = RBA::ShapeProcessor::new

    sh = RBA::Shapes::new

    sp.size( lay, lay.cell( cid_top ), lid_b, sh, 100, 100, 2, true, false, true )
    assert_equal( sh2s(sh), "(0,0;0,400;200,400;200,600;600,600;600,200;400,200;400,0)" )

    sp.size( lay, lay.cell( cid_top ), lid_b, sh, 0, 100, 2, true, false, true )
    assert_equal( sh2s(sh), "(100,0;100,400;300,400;300,600;500,600;500,200;300,200;300,0)" )

    sp.size( lay, lay.cell( cid_top ), lid_b, sh, 100, 2, true, false, true )
    assert_equal( sh2s(sh), "(0,0;0,400;200,400;200,600;600,600;600,200;400,200;400,0)" )

    sp.size( lay, lay.cell( cid_top ), lid_b, sh, 100, 100, 2, false, false, true )
    assert_equal( sh2s(sh), "" )

    sp.size( lay, lay.cell( cid_top ), lid_b, sh, 100, 100, 0, true, false, true )
    assert_equal( sh2s(sh), "(100,0;0,100;0,300;100,400;200,400;200,500;300,600;500,600;600,500;600,300;500,200;400,200;400,100;300,0)" )

    sp.size( lay, lay.cell( cid_top ), lid_b, sh, 0, 0, true, false, true )
    assert_equal( sh2s(sh), "(100,100;100,300;300,300;300,100);(300,300;300,500;500,500;500,300)" )

    sp.size( lay, lay.cell( cid_top ), lid_b, sh, 0, 0, true, false, false )
    assert_equal( sh2s(sh), "(100,100;100,300;300,300;300,500;500,500;500,300;300,300;300,100)" )

    sha = []
    lay.cell( cid_a ).shapes( lid_a ).each { |s| sha.push( s ) }

    ee = sp.size( sha, 100, 0 )
    assert_equal( a2s(ee), "(0,-100;-100,0);(1000,-100;0,-100);(1100,0;1000,-100);(-100,0;-100,1000);(1100,1000;1100,0);(-100,1000;0,1100);(1000,1100;1100,1000);(0,1100;1000,1100)" )

    ee = sp.size( sha, 100, 2 )
    assert_equal( a2s(ee), "(-100,-100;-100,1100);(1100,-100;-100,-100);(1100,1100;1100,-100);(-100,1100;1100,1100)" )

    ee = sp.size( sha, 100, 0, 2 )
    assert_equal( a2s(ee), "(-100,0;-100,1000);(1100,0;-100,0);(1100,1000;1100,0);(-100,1000;1100,1000)" )

    ee = sp.size_to_polygon( sha, 100, 0, true, false )
    assert_equal( a2s(ee), "(0,-100;-100,0;-100,1000;0,1100;1000,1100;1100,1000;1100,0;1000,-100)" )

    ee = sp.size_to_polygon( sha, 100, 2, true, false )
    assert_equal( a2s(ee), "(-100,-100;-100,1100;1100,1100;1100,-100)" )

    ee = sp.size_to_polygon( sha, 100, 0, 2, true, false )
    assert_equal( a2s(ee), "(-100,0;-100,1000;1100,1000;1100,0)" )

    sha = []
    tt = []
    lay.cell( cid_a ).shapes( lid_b ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 0, 0 ) ) ) }
    lay.cell( cid_a ).shapes( lid_b ).each { |s| sha.push( s ); tt.push( RBA::CplxTrans::new( RBA::Trans::new( 200, 200 ) ) ) }

    ee = sp.size( sha, tt, 100, 2 )
    assert_equal( a2s(ee), "(0,0;0,400);(400,0;0,0);(400,200;400,0);(600,200;400,200);(600,600;600,200);(0,400;200,400);(200,400;200,600);(200,600;600,600)" )

    ee = sp.size( sha, tt, 0, 100, 2 )
    assert_equal( a2s(ee), "(100,0;100,400);(300,0;100,0);(300,200;300,0);(500,200;300,200);(500,600;500,200);(100,400;300,400);(300,400;300,600);(300,600;500,600)" )

    ee = sp.size_to_polygon( sha, tt, 0, 2, true, false )
    assert_equal( a2s(ee), "(100,100;100,300;300,300;300,500;500,500;500,300;300,300;300,100)" )

    ee = sp.size_to_polygon( sha, tt, 0, 2, true, true )
    assert_equal( a2s(ee), "(100,100;100,300;300,300;300,100);(300,300;300,500;500,500;500,300)" )

    ee = sp.size_to_polygon( sha, tt, 0, 0, 2, true, false )
    assert_equal( a2s(ee), "(100,100;100,300;300,300;300,500;500,500;500,300;300,300;300,100)" )

    ee = sp.size_to_polygon( sha, tt, 0, 0, 2, true, true )
    assert_equal( a2s(ee), "(100,100;100,300;300,300;300,100);(300,300;300,500;500,500;500,300)" )

  end

end

load("test_epilogue.rb")


