# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2025 Matthias Koefferlein
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
import sys

def astr(a):
  astr = []
  for i in a:
    astr.append(str(i))
  return "[" + ", ".join(astr) + "]"

class DBPolygonTests(unittest.TestCase):

  # DPolygon basics
  def test_1_DPolygon(self):

    a = pya.DPolygon()
    self.assertEqual( str(a), "()" )
    self.assertEqual( str(pya.DPolygon.from_s(str(a))), str(a) )
    self.assertEqual( a.is_box(), False )

    b = a.dup()
    a = pya.DPolygon( [ pya.DPoint( 0, 1 ), pya.DPoint( 1, 5 ), pya.DPoint( 5, 5 ) ] )
    self.assertEqual( str(a), "(0,1;1,5;5,5)" )
    self.assertEqual( str(a * 2), "(0,2;2,10;10,10)" )
    self.assertEqual( str(pya.DPolygon.from_s(str(a))), str(a) )
    self.assertEqual( a.is_box(), False )
    self.assertEqual( a.num_points_hull(), 3 )
    c = a.dup()

    self.assertEqual( a == b, False )
    self.assertEqual( a == c, True )
    self.assertEqual( a != b, True )
    self.assertEqual( a != c, False )

    a = pya.DPolygon( pya.DBox( 5, -10, 20, 15 ) )
    self.assertEqual( a.is_box(), True )
    self.assertEqual( str(a), "(5,-10;5,15;20,15;20,-10)" )
    self.assertEqual( str(pya.Polygon(a)), "(5,-10;5,15;20,15;20,-10)" )
    self.assertEqual( a.num_points_hull(), 4 )
    self.assertEqual( a.area(), 15*25 )
    self.assertEqual( a.perimeter(), 80 )
    self.assertEqual( a.inside( pya.DPoint( 10, 0 ) ), True )
    self.assertEqual( a.inside( pya.DPoint( 5, 0 ) ), True )
    self.assertEqual( a.inside( pya.DPoint( 30, 0 ) ), False )

    arr = []
    for p in a.each_point_hull():
      arr.append( str(p) )
    self.assertEqual( arr, ["5,-10", "5,15", "20,15", "20,-10"] )

    b = a.dup()

    self.assertEqual( str(a.moved( pya.DPoint( 0, 1 ) )), "(5,-9;5,16;20,16;20,-9)" )
    self.assertEqual( str(a.moved( 0, 1 )), "(5,-9;5,16;20,16;20,-9)" )
    aa = a.dup()
    aa.move( 1, 0 )
    self.assertEqual( str(aa), "(6,-10;6,15;21,15;21,-10)" )
    a.move( pya.DPoint( 1, 0 ) )
    self.assertEqual( str(a), "(6,-10;6,15;21,15;21,-10)" )

    b = b.transformed( pya.DTrans( pya.DTrans.R0, pya.DPoint( 1, 0 )) )
    self.assertEqual( str(b), "(6,-10;6,15;21,15;21,-10)" )

    m = pya.DCplxTrans( pya.DTrans(), 1.5 )
    self.assertEqual( type(a.transformed(m)).__name__, "DPolygon" )
    self.assertEqual( str(a.transformed(m)), "(9,-15;9,22.5;31.5,22.5;31.5,-15)" )

    m = pya.VCplxTrans( 1000.0 )
    self.assertEqual( type(a.transformed(m)).__name__, "Polygon" )
    self.assertEqual( str(a.transformed(m)), "(6000,-10000;6000,15000;21000,15000;21000,-10000)" )

    a.hull = [ pya.DPoint( 0, 1 ), pya.DPoint( 1, 1 ), pya.DPoint( 1, 5 ) ]
    self.assertEqual( str(a.bbox()), "(0,1;1,5)" )

    self.assertEqual( a.holes(), 0 )
    a.insert_hole( [ pya.DPoint( 1, 2 ), pya.DPoint( 2, 2 ), pya.DPoint( 2, 6 ) ] )
    self.assertEqual( str(a), "(0,1;1,5;1,1/1,2;2,2;2,6)" )
    self.assertEqual( str(pya.DPolygon.from_s(str(a))), str(a) )
    self.assertEqual( a.area(), 0 )
    self.assertEqual( a.num_points_hole(0), 3 )
    self.assertEqual( a.holes(), 1 )
    self.assertEqual( str(a.point_hull(1)), "1,5" )
    self.assertEqual( str(a.point_hull(0)), "0,1" )
    self.assertEqual( str(a.point_hull(100)), "0,0" )
    self.assertEqual( str(a.point_hole(0, 100)), "0,0" )
    self.assertEqual( str(a.point_hole(0, 1)), "2,2" )
    self.assertEqual( str(a.point_hole(1, 1)), "0,0" )
    a.compress(False);
    self.assertEqual( str(a), "(0,1;1,5;1,1/1,2;2,2;2,6)" )
    a.compress(True);
    self.assertEqual( str(a), "(0,1;1,5;1,1/1,2;2,2;2,6)" )

    b = a.dup()
    b.assign_hole(0, pya.DBox( 10, 20, 20, 60 ))
    self.assertEqual( str(b), "(0,1;1,5;1,1/10,20;20,20;20,60;10,60)" )
    b.insert_hole(pya.DBox( 10, 20, 20, 60 ))
    self.assertEqual( str(b), "(0,1;1,5;1,1/10,20;20,20;20,60;10,60/10,20;20,20;20,60;10,60)" )
    self.assertEqual( b.is_box(), False )

    b = a.dup()
    b.assign_hole(0, [ pya.DPoint( 10, 20 ), pya.DPoint( 20, 20 ), pya.DPoint( 20, 60 ) ])
    self.assertEqual( str(b), "(0,1;1,5;1,1/10,20;20,20;20,60)" )
    b.assign_hole(1, [ pya.DPoint( 15, 25 ), pya.DPoint( 25, 25 ), pya.DPoint( 25, 65 ) ])
    self.assertEqual( str(b), "(0,1;1,5;1,1/10,20;20,20;20,60)" )
    b.insert_hole( [ pya.DPoint( 1, 2 ), pya.DPoint( 2, 2 ), pya.DPoint( 2, 6 ) ] )
    self.assertEqual( str(b), "(0,1;1,5;1,1/10,20;20,20;20,60/1,2;2,2;2,6)" )
    b.sort_holes()
    self.assertEqual( str(b), "(0,1;1,5;1,1/1,2;2,2;2,6/10,20;20,20;20,60)" )
    b.assign_hole(0, [ pya.DPoint( 15, 25 ), pya.DPoint( 25, 25 ), pya.DPoint( 25, 65 ) ])
    self.assertEqual( str(b), "(0,1;1,5;1,1/15,25;25,25;25,65/10,20;20,20;20,60)" )

    arr = []
    for p in a.each_point_hole(0):
      arr.append( str(p) )

    self.assertEqual( arr, ["1,2", "2,2", "2,6"] )

    arr = []
    for p in a.each_edge():
      arr.append( str(p) )
    self.assertEqual( arr, ["(0,1;1,5)", "(1,5;1,1)", "(1,1;0,1)", "(1,2;2,2)", "(2,2;2,6)", "(2,6;1,2)"] )

    # Ellipse constructor
    p = pya.DPolygon.ellipse( pya.DBox(-10000, -20000, 30000, 40000), 200 )
    self.assertEqual(p.num_points(), 200)
    self.assertEqual(str(p.bbox()), "(-10000,-20000;30000,40000)")
    self.assertEqual(int(p.area()), 1884645544)    # roughly box.area*PI/4
    
    p = pya.DPolygon.ellipse( pya.DBox(-10000, -20000, 30000, 40000), 4 )
    self.assertEqual(str(p), "(10000,-20000;-10000,10000;10000,40000;30000,10000)")

  # Polygon basics
  def test_1_Polygon(self):

    a = pya.Polygon()
    self.assertEqual( str(a), "()" )
    self.assertEqual( str(pya.Polygon.from_s(str(a))), str(a) )
    self.assertEqual( a.is_box(), False )

    b = a.dup() 
    a = pya.Polygon( [ pya.Point( 0, 1 ), pya.Point( 1, 5 ), pya.Point( 5, 5 ) ] )
    self.assertEqual( str(a), "(0,1;1,5;5,5)" )
    self.assertEqual( str(a * 2), "(0,2;2,10;10,10)" )
    self.assertEqual( str(pya.Polygon.from_s(str(a))), str(a) )
    self.assertEqual( a.num_points_hull(), 3 )
    c = a.dup() 

    self.assertEqual( a == b, False )
    self.assertEqual( a == c, True )
    self.assertEqual( a != b, True )
    self.assertEqual( a != c, False )

    a = pya.Polygon( pya.Box( 5, -10, 20, 15 ) )
    self.assertEqual( a.is_box(), True )
    self.assertEqual( str(a), "(5,-10;5,15;20,15;20,-10)" )
    self.assertEqual( str(pya.DPolygon(a)), "(5,-10;5,15;20,15;20,-10)" )
    self.assertEqual( a.num_points_hull(), 4 )
    self.assertEqual( a.area(), 15*25 )
    self.assertEqual( a.perimeter(), 80 )
    self.assertEqual( a.inside( pya.Point( 10, 0 ) ), True )
    self.assertEqual( a.inside( pya.Point( 5, 0 ) ), True )
    self.assertEqual( a.inside( pya.Point( 30, 0 ) ), False )

    arr = []
    for p in a.each_point_hull():
      arr.append(str(p))
    self.assertEqual( arr, ["5,-10", "5,15", "20,15", "20,-10"] )

    b = a.dup()

    self.assertEqual( str(a.moved( pya.Point( 0, 1 ) )), "(5,-9;5,16;20,16;20,-9)" )
    self.assertEqual( str(a.moved( 0, 1 )), "(5,-9;5,16;20,16;20,-9)" )
    aa = a.dup()
    aa.move( 1, 0 )
    self.assertEqual( str(aa), "(6,-10;6,15;21,15;21,-10)" )
    a.move( pya.Point( 1, 0 ) )
    self.assertEqual( str(a), "(6,-10;6,15;21,15;21,-10)" )

    b = b.transformed( pya.Trans( pya.Trans.R0, pya.Point( 1, 0 )) )
    self.assertEqual( str(b), "(6,-10;6,15;21,15;21,-10)" )

    m = pya.CplxTrans( pya.Trans(), 1.5 )
    self.assertEqual( str(a.transformed(m)), "(9,-15;9,22.5;31.5,22.5;31.5,-15)" )
    self.assertEqual( str(a.transformed(pya.ICplxTrans(m))), "(9,-15;9,23;32,23;32,-15)" )

    a.hull = [ pya.Point( 0, 1 ), pya.Point( 1, 1 ), pya.Point( 1, 5 ) ]
    self.assertEqual( str(a.bbox()), "(0,1;1,5)" )

    self.assertEqual( a.holes(), 0 )
    a.insert_hole( [ pya.Point( 1, 2 ), pya.Point( 2, 2 ), pya.Point( 2, 6 ) ] )
    self.assertEqual( str(a), "(0,1;1,5;1,1/1,2;2,2;2,6)" )
    self.assertEqual( str(pya.Polygon.from_s(str(a))), str(a) )
    self.assertEqual( a.area(), 0 )
    self.assertEqual( a.num_points_hole(0), 3 )
    self.assertEqual( a.holes(), 1 )
    self.assertEqual( str(a.point_hull(1)), "1,5" )
    self.assertEqual( str(a.point_hull(0)), "0,1" )
    self.assertEqual( str(a.point_hull(100)), "0,0" )
    self.assertEqual( str(a.point_hole(0, 100)), "0,0" )
    self.assertEqual( str(a.point_hole(0, 1)), "2,2" )
    self.assertEqual( str(a.point_hole(1, 1)), "0,0" )
    a.compress(False);
    self.assertEqual( str(a), "(0,1;1,5;1,1/1,2;2,2;2,6)" )
    a.compress(True);
    self.assertEqual( str(a), "(0,1;1,5;1,1/1,2;2,2;2,6)" )

    b = a.dup()
    b.assign_hole(0, pya.Box( 10, 20, 20, 60 ))
    self.assertEqual( str(b), "(0,1;1,5;1,1/10,20;20,20;20,60;10,60)" )
    self.assertEqual( b.is_box(), False )
    b.insert_hole(pya.Box( 10, 20, 20, 60 ))
    self.assertEqual( str(b), "(0,1;1,5;1,1/10,20;20,20;20,60;10,60/10,20;20,20;20,60;10,60)" )

    b = a.dup()
    b.assign_hole(0, [ pya.Point( 10, 20 ), pya.Point( 20, 20 ), pya.Point( 20, 60 ) ])
    self.assertEqual( str(b), "(0,1;1,5;1,1/10,20;20,20;20,60)" )
    b.assign_hole(1, [ pya.Point( 15, 25 ), pya.Point( 25, 25 ), pya.Point( 25, 65 ) ])
    self.assertEqual( str(b), "(0,1;1,5;1,1/10,20;20,20;20,60)" )
    b.insert_hole( [ pya.Point( 1, 2 ), pya.Point( 2, 2 ), pya.Point( 2, 6 ) ] )
    self.assertEqual( str(b), "(0,1;1,5;1,1/10,20;20,20;20,60/1,2;2,2;2,6)" )
    b.sort_holes()
    self.assertEqual( str(b), "(0,1;1,5;1,1/1,2;2,2;2,6/10,20;20,20;20,60)" )
    b.assign_hole(0, [ pya.Point( 15, 25 ), pya.Point( 25, 25 ), pya.Point( 25, 65 ) ])
    self.assertEqual( str(b), "(0,1;1,5;1,1/15,25;25,25;25,65/10,20;20,20;20,60)" )

    arr = []
    for p in a.each_point_hole(0):
      arr.append(str(p))
    self.assertEqual( arr, ["1,2", "2,2", "2,6"] )

    arr = []
    for p in a.each_edge():
      arr.append(str(p))
    self.assertEqual( arr, ["(0,1;1,5)", "(1,5;1,1)", "(1,1;0,1)", "(1,2;2,2)", "(2,2;2,6)", "(2,6;1,2)"] )

    a = pya.Polygon( [ pya.Point( 0, 1 ), pya.Point( 1, 5 ), pya.Point( 5, 5 ) ] )
    self.assertEqual( str(a), "(0,1;1,5;5,5)" )
    self.assertEqual( str(a.sized(2)), "(0,-2;-2,0;-1,7;7,7;8,5)" )
    self.assertEqual( str(a.sized(2, 2)), "(0,-2;-2,0;-1,7;7,7;8,5)" )
    aa = a.dup()
    a.size(2, 2)
    self.assertEqual( str(a), "(0,-2;-2,0;-1,7;7,7;8,5)" )
    a = aa.dup()
    a.size(2)
    self.assertEqual( str(a), "(0,-2;-2,0;-1,7;7,7;8,5)" )

    a = pya.Polygon( [ pya.Point( 0, 1 ), pya.Point( 1, 5 ), pya.Point( 5, 5 ) ] )
    self.assertEqual( str(a), "(0,1;1,5;5,5)" )
    self.assertEqual( str(a.sized(2, 0, 2)), "(-2,1;-1,5;7,5;2,1)" )
    a.size(2, 0, 2);
    self.assertEqual( str(a), "(-2,1;-1,5;7,5;2,1)" )

    a = pya.Polygon()
    self.assertEqual( str(a), "()" )

    # corner rounding
    a = pya.Polygon( [ pya.Point(0, 0), pya.Point(0, 2000), pya.Point(4000, 2000),
                             pya.Point(4000, 1000), pya.Point(2000, 1000), pya.Point(2000, 0) ] )
    ar = a.round_corners(100, 200, 8)
    self.assertEqual( str(ar), "(117,0;0,117;0,1883;117,2000;3883,2000;4000,1883;4000,1117;3883,1000;2059,1000;2000,941;2000,117;1883,0)" )
    ar = a.round_corners(200, 100, 32)
    self.assertEqual( str(ar), "(90,0;71,4;53,11;36,22;22,36;11,53;4,71;0,90;0,1910;4,1929;11,1947;22,1964;36,1978;53,1989;71,1996;90,2000;3910,2000;3929,1996;3947,1989;3964,1978;3978,1964;3989,1947;3996,1929;4000,1910;4000,1090;3996,1071;3989,1053;3978,1036;3964,1022;3947,1011;3929,1004;3910,1000;2180,1000;2142,992;2105,977;2073,955;2045,927;2023,895;2008,858;2000,820;2000,90;1996,71;1989,53;1978,36;1964,22;1947,11;1929,4;1910,0)" )

    # Minkowsky sums
    p = pya.Polygon( [ pya.Point.new(0, -100), pya.Point.new(0, -50), pya.Point.new(-100, -75), pya.Point.new(0, 100), pya.Point.new(50, 50), pya.Point.new(100, 75), pya.Point.new(100, 0), pya.Point.new(100, -50) ] )
    self.assertEqual(str(p.minkowsky_sum(pya.Edge.new(pya.Point.new(10, 10), pya.Point.new(210, 110)), True)), "(10,-90;10,-40;-90,-65;10,110;210,210;260,160;310,185;310,60)")
    self.assertEqual(str(p.minkowsky_sum([pya.Point.new(10, 10), pya.Point.new(10, 310), pya.Point.new(510, 310), pya.Point.new(510, 10), pya.Point.new(10, 10) ], False)), "(10,-90;10,-65;-90,-65;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90/110,110;410,110;410,210;110,210)")
    self.assertEqual(str(p.minkowsky_sum([pya.Point.new(10, 10), pya.Point.new(10, 310), pya.Point.new(510, 310), pya.Point.new(510, 10), pya.Point.new(10, 10) ], True)), "(10,-90;10,-65;-90,-65;-90,210;110,210;110,110;410,110;410,210;-90,210;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)")
    self.assertEqual(str(p.minkowsky_sum(pya.Box.new(pya.Point.new(10, 10), pya.Point.new(210, 110)), True)), "(10,-90;10,-65;-90,-65;-90,35;10,210;210,210;235,185;310,185;310,-40;210,-90)")
    self.assertEqual(str(p.minkowsky_sum(pya.Box.new(pya.Point.new(10, 10), pya.Point.new(210, 10)), True)), "(10,-90;10,-65;-90,-65;10,110;210,110;235,85;310,85;310,-40;210,-90)")
    self.assertEqual(str(p.minkowsky_sum(pya.Polygon.new(pya.Box.new(pya.Point.new(10, 10), pya.Point.new(210, 110))), True)), "(10,-90;10,-65;-90,-65;-90,35;10,210;210,210;235,185;310,185;310,-40;210,-90)")

    # Smoothing
    p = pya.Polygon( [ pya.Point.new(0, 0), pya.Point.new(10, 50), pya.Point.new(0, 100), pya.Point.new(200, 100), pya.Point.new(200, 0) ])
    self.assertEqual(str(p.smooth(5)), "(0,0;10,50;0,100;200,100;200,0)")
    self.assertEqual(str(p.smooth(15)), "(0,0;0,100;200,100;200,0)")
    p = pya.Polygon( [ pya.Point.new(0, 0), pya.Point.new(10, 50), pya.Point.new(10, 100), pya.Point.new(200, 100), pya.Point.new(200, 0) ])
    self.assertEqual(str(p.smooth(15, False)), "(0,0;10,100;200,100;200,0)")
    self.assertEqual(str(p.smooth(15, True)), "(0,0;10,50;10,100;200,100;200,0)")

    # Ellipse constructor
    p = pya.Polygon.ellipse( pya.Box(-10000, -20000, 30000, 40000), 200 )
    self.assertEqual(p.num_points(), 200)
    self.assertEqual(str(p.bbox()), "(-10000,-20000;30000,40000)")
    self.assertEqual(p.area(), 1884651158)    # roughly box.area*PI/4
    
    p = pya.Polygon.ellipse( pya.Box(-10000, -20000, 30000, 40000), 4 )
    self.assertEqual(str(p), "(10000,-20000;-10000,10000;10000,40000;30000,10000)")

  # Polygon parametrized edge iterator
  def test_2(self):

    hull =  [ pya.Point(0, 0),       pya.Point(6000, 0), 
              pya.Point(6000, 3000), pya.Point(0, 3000) ]
    hole1 = [ pya.Point(1000, 1000), pya.Point(2000, 1000), 
              pya.Point(2000, 2000), pya.Point(1000, 2000) ]
    hole2 = [ pya.Point(3000, 1000), pya.Point(4000, 1000), 
              pya.Point(4000, 2000), pya.Point(3000, 2000) ]
    poly = pya.Polygon(hull)
    poly.insert_hole(hole1)
    poly.insert_hole(hole2)
    
    es = []
    for e in poly.each_edge():
      es.append(str(e))
    self.assertEqual( "/".join(es), "(0,0;0,3000)/(0,3000;6000,3000)/(6000,3000;6000,0)/(6000,0;0,0)/(1000,1000;2000,1000)/(2000,1000;2000,2000)/(2000,2000;1000,2000)/(1000,2000;1000,1000)/(3000,1000;4000,1000)/(4000,1000;4000,2000)/(4000,2000;3000,2000)/(3000,2000;3000,1000)" )
    es = []
    for e in poly.each_edge(0):
      es.append(str(e))
    self.assertEqual( "/".join(es), "(0,0;0,3000)/(0,3000;6000,3000)/(6000,3000;6000,0)/(6000,0;0,0)" )
    es = []
    for e in poly.each_edge(1):
      es.append(str(e))
    self.assertEqual( "/".join(es), "(1000,1000;2000,1000)/(2000,1000;2000,2000)/(2000,2000;1000,2000)/(1000,2000;1000,1000)" )
    es = []
    for e in poly.each_edge(2):
      es.append(str(e))
    self.assertEqual( "/".join(es), "(3000,1000;4000,1000)/(4000,1000;4000,2000)/(4000,2000;3000,2000)/(3000,2000;3000,1000)" )
    es = []
    for e in poly.each_edge(3):
      es.append(str(e))
    self.assertEqual( "/".join(es), "" )

    hull =  [ pya.DPoint(0, 0),       pya.DPoint(6000, 0), 
              pya.DPoint(6000, 3000), pya.DPoint(0, 3000) ]
    hole1 = [ pya.DPoint(1000, 1000), pya.DPoint(2000, 1000), 
              pya.DPoint(2000, 2000), pya.DPoint(1000, 2000) ]
    hole2 = [ pya.DPoint(3000, 1000), pya.DPoint(4000, 1000), 
              pya.DPoint(4000, 2000), pya.DPoint(3000, 2000) ]
    poly = pya.DPolygon(hull)
    poly.insert_hole(hole1)
    poly.insert_hole(hole2)
    
    es = []
    for e in poly.each_edge():
      es.append(str(e))
    self.assertEqual( "/".join(es), "(0,0;0,3000)/(0,3000;6000,3000)/(6000,3000;6000,0)/(6000,0;0,0)/(1000,1000;2000,1000)/(2000,1000;2000,2000)/(2000,2000;1000,2000)/(1000,2000;1000,1000)/(3000,1000;4000,1000)/(4000,1000;4000,2000)/(4000,2000;3000,2000)/(3000,2000;3000,1000)" )
    es = []
    for e in poly.each_edge(0):
      es.append(str(e))
    self.assertEqual( "/".join(es), "(0,0;0,3000)/(0,3000;6000,3000)/(6000,3000;6000,0)/(6000,0;0,0)" )
    es = []
    for e in poly.each_edge(1):
      es.append(str(e))
    self.assertEqual( "/".join(es), "(1000,1000;2000,1000)/(2000,1000;2000,2000)/(2000,2000;1000,2000)/(1000,2000;1000,1000)" )
    es = []
    for e in poly.each_edge(2):
      es.append(str(e))
    self.assertEqual( "/".join(es), "(3000,1000;4000,1000)/(4000,1000;4000,2000)/(4000,2000;3000,2000)/(3000,2000;3000,1000)" )
    es = []
    for e in poly.each_edge(3):
      es.append(str(e))
    self.assertEqual( "/".join(es), "" )

  # raw mode polygons
  def test_2_Polygon(self):

    pts = [ pya.Point(0, 0) ]
    p = pya.Polygon(pts, False)
    self.assertEqual(str(p), "()")
    
    pts = [ pya.Point(0, 0) ]
    p = pya.Polygon(pts)
    self.assertEqual(str(p), "()")
    
    pts = [ pya.Point(0, 0) ]
    p = pya.Polygon(pts, True)
    self.assertEqual(str(p), "(0,0)")

    arr = []
    for e in p.each_edge():
      arr.append(str(e))
    self.assertEqual( arr, ["(0,0;0,0)"] )
    
    p = pya.Polygon(pya.Box(0, 0, 100, 100))
    self.assertEqual(str(p), "(0,0;0,100;100,100;100,0)")
    p.insert_hole( [ pya.Point(0, 0), pya.Point(10, 0) ] )
    # TODO: this isn't nice (empty hole):
    self.assertEqual(str(p), "(0,0;0,100;100,100;100,0/)")

    p = pya.Polygon(pya.Box(0, 0, 100, 100))
    p.insert_hole( [ pya.Point(0, 0), pya.Point(10, 0) ], True )
    self.assertEqual(str(p), "(0,0;0,100;100,100;100,0/0,0;10,0)")
    p.assign_hole(0, [ pya.Point(0, 0), pya.Point(10, 0) ] )
    self.assertEqual(str(p), "(0,0;0,100;100,100;100,0/)")
    p.assign_hole(0, [ pya.Point(0, 0), pya.Point(10, 0) ], True )
    self.assertEqual(str(p), "(0,0;0,100;100,100;100,0/0,0;10,0)")

    pts = [ pya.Point(0, 0), pya.Point(10, 0) ]
    p = pya.Polygon(pts, True)
    self.assertEqual(str(p), "(0,0;10,0)")
    # conversion of degenerated polygon to simple polygon is not supported currently:
    self.assertEqual(str(p.to_simple_polygon()), "()")
    self.assertEqual(str(pya.DPolygon(p)), "(0,0;10,0)")

    p.hull = []
    self.assertEqual(str(p), "()")
    
    p.hull = [ pya.Point(0, 0), pya.Point(10, 0) ]
    self.assertEqual(str(p), "(0,0;10,0)")

    p.assign_hull([ pya.Point(0, 0), pya.Point(10, 0) ], False)
    self.assertEqual(str(p), "()")

    p.assign_hull([ pya.Point(0, 0), pya.Point(10, 0) ], True)
    self.assertEqual(str(p), "(0,0;10,0)")

    arr = []
    for e in p.each_edge():
      arr.append(str(e))
    self.assertEqual( arr, ["(0,0;10,0)", "(10,0;0,0)"] )
    
    self.assertEqual(str(p.moved(1, 2)), "(1,2;11,2)")
    self.assertEqual(str(p.sized(2)), "(0,-2;0,2;10,2;10,-2)")
    self.assertEqual(str(p * 2), "(0,0;20,0)")
    self.assertEqual(str(p.transformed(pya.Trans(pya.Trans.R90))), "(0,0;0,10)")

    pp = p.dup()
    pp.transform(pya.Trans(pya.Trans.R90))
    self.assertEqual(str(pp), "(0,0;0,10)")
    
    p = pya.Polygon([ pya.Point(0, 0), pya.Point(0, 10) ], True)
    q = pya.Polygon([ pya.Point(1, 1), pya.Point(-9, 1) ], True)
    self.assertEqual(str(p.minkowsky_sum(q, False)), "(-9,1;-9,11;1,11;1,1)")
    
  # raw mode polygons
  def test_2_DPolygon(self):

    pts = [ pya.DPoint(0, 0) ]
    p = pya.DPolygon(pts, True)
    self.assertEqual(str(p), "(0,0)")

    arr = []
    for e in p.each_edge():
      arr.append(str(e))
    self.assertEqual( arr, ["(0,0;0,0)"] )
    
    p = pya.DPolygon(pya.DBox(0, 0, 100, 100))
    p.insert_hole( [ pya.DPoint(0, 0), pya.DPoint(10, 0) ], True )
    self.assertEqual(str(p), "(0,0;0,100;100,100;100,0/0,0;10,0)")
    p.assign_hole(0, [ pya.DPoint(0, 0), pya.DPoint(10, 0) ] )
    self.assertEqual(str(p), "(0,0;0,100;100,100;100,0/0,0;10,0)")
    p.assign_hole(0, [ pya.DPoint(0, 0), pya.DPoint(10, 0) ], True )
    self.assertEqual(str(p), "(0,0;0,100;100,100;100,0/0,0;10,0)")

    pts = [ pya.DPoint(0, 0), pya.DPoint(10, 0) ]
    p = pya.DPolygon(pts, True)
    self.assertEqual(str(p), "(0,0;10,0)")
    self.assertEqual(str(pya.Polygon(p)), "(0,0;10,0)")

    p.hull = []
    self.assertEqual(str(p), "()")
    
    p.hull = [ pya.DPoint(0, 0), pya.DPoint(10, 0) ]
    self.assertEqual(str(p), "(0,0;10,0)")

    p.assign_hull([ pya.DPoint(0, 0), pya.DPoint(10, 0) ], True)
    self.assertEqual(str(p), "(0,0;10,0)")

    arr = []
    for e in p.each_edge():
      arr.append(str(e))
    self.assertEqual( arr, ["(0,0;10,0)", "(10,0;0,0)"] )
    
    self.assertEqual(str(p.moved(1, 2)), "(1,2;11,2)")
    self.assertEqual(str(p.sized(2)), "(0,-2;0,2;10,2;10,-2)")
    self.assertEqual(str(p * 2), "(0,0;20,0)")
    self.assertEqual(str(p.transformed(pya.DTrans(pya.DTrans.R90))), "(0,0;0,10)")

    pp = p.dup()
    pp.transform(pya.DTrans(pya.DTrans.R90))
    self.assertEqual(str(pp), "(0,0;0,10)")
    
  # is_convex
  def test_IsConvex(self):

    self.assertEqual(pya.Polygon(pya.Box(0, 0, 10, 10)).is_convex(), True)

    p = pya.Polygon.from_s("(0,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    self.assertEqual(p.is_convex(), False)

  # polygon decomposition
  def test_PolygonDecompose(self):

    p = pya.Polygon.from_s("(0,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")

    self.assertEqual(astr(p.decompose_convex()), "[(0,10;0,30;10,30;10,10), (0,30;0,40;30,40;30,30), (30,10;30,40;40,40;40,10), (0,0;0,10;40,10;40,0)]")
    self.assertEqual(astr(p.decompose_convex(pya.Polygon.PO_any)), "[(0,10;0,30;10,30;10,10), (0,30;0,40;30,40;30,30), (30,10;30,40;40,40;40,10), (0,0;0,10;40,10;40,0)]")
    self.assertEqual(astr(p.decompose_convex(pya.Polygon.PO_horizontal)), "[(0,10;0,30;10,30;10,10), (0,30;0,40;40,40;40,30), (30,10;30,30;40,30;40,10), (0,0;0,10;40,10;40,0)]")
    self.assertEqual(astr(p.decompose_convex(pya.Polygon.PO_vertical)), "[(10,0;10,10;30,10;30,0), (0,0;0,40;10,40;10,0), (10,30;10,40;30,40;30,30), (30,0;30,40;40,40;40,0)]")
    self.assertEqual(astr(p.decompose_convex(pya.Polygon.PO_htrapezoids)), "[(0,10;0,30;10,30;10,10), (0,30;0,40;30,40;30,30), (30,10;30,40;40,40;40,10), (0,0;0,10;40,10;40,0)]")
    self.assertEqual(astr(p.decompose_convex(pya.Polygon.PO_vtrapezoids)), "[(10,0;10,10;30,10;30,0), (0,0;0,30;10,30;10,0), (0,30;0,40;30,40;30,30), (30,0;30,40;40,40;40,0)]")

    self.assertEqual(astr(p.decompose_trapezoids()), "[(0,0;0,10;40,10;40,0), (0,10;0,30;10,30;10,10), (30,10;30,30;40,30;40,10), (0,30;0,40;40,40;40,30)]")
    self.assertEqual(astr(p.decompose_trapezoids(pya.Polygon.TD_simple)), "[(0,0;0,10;40,10;40,0), (0,10;0,30;10,30;10,10), (30,10;30,30;40,30;40,10), (0,30;0,40;40,40;40,30)]")
    self.assertEqual(astr(p.decompose_trapezoids(pya.Polygon.TD_htrapezoids)), "[(0,10;0,30;10,30;10,10), (0,30;0,40;30,40;30,30), (30,10;30,40;40,40;40,10), (0,0;0,10;40,10;40,0)]")
    self.assertEqual(astr(p.decompose_trapezoids(pya.Polygon.TD_vtrapezoids)), "[(10,0;10,10;30,10;30,0), (0,0;0,30;10,30;10,0), (0,30;0,40;30,40;30,30), (30,0;30,40;40,40;40,0)]")

  # polygon decomposition
  def test_extractRad(self):

    ex = pya.SimplePolygon().extract_rad()
    self.assertEqual(repr(ex), "[]")

    sp = pya.SimplePolygon.from_s("(0,0;0,200000;300000,200000;300000,100000;100000,100000;100000,0)")

    sp = sp.round_corners(10000, 5000, 200)
    ex = sp.extract_rad()

    self.assertEqual(ex, [pya.SimplePolygon.from_s("(0,0;0,200000;300000,200000;300000,100000;100000,100000;100000,0)"), 10000.0, 5000.0, 200])

    ex = pya.Polygon().extract_rad()
    self.assertEqual(ex, [])

    sp = pya.Polygon.from_s("(0,0;0,300000;300000,300000;300000,0/100000,100000;200000,100000;200000,200000;100000,200000)")

    sp = sp.round_corners(10000, 5000, 200)
    ex = sp.extract_rad()

    self.assertEqual(ex, [pya.Polygon.from_s("(0,0;0,300000;300000,300000;300000,0/100000,100000;200000,100000;200000,200000;100000,200000)"), 10000.0, 5000.0, 200])

    # double coords too ...

    ex = pya.DSimplePolygon().extract_rad()
    self.assertEqual(ex, [])

    sp = pya.DSimplePolygon.from_s("(0,0;0,200000;300000,200000;300000,100000;100000,100000;100000,0)")

    sp = sp.round_corners(10000, 5000, 200)
    ex = sp.extract_rad()

    # round to integers for better comparison
    
    ex[0] = pya.SimplePolygon(ex[0])
    self.assertEqual(ex, [pya.SimplePolygon.from_s("(0,0;0,200000;300000,200000;300000,100000;100000,100000;100000,0)"), 10000.0, 5000.0, 200])

    ex = pya.DPolygon().extract_rad()
    self.assertEqual(ex, [])

    sp = pya.DPolygon.from_s("(0,0;0,300000;300000,300000;300000,0/100000,100000;200000,100000;200000,200000;100000,200000)")

    sp = sp.round_corners(10000, 5000, 200)
    ex = sp.extract_rad()

    # round to integers for better comparison
    ex[0] = pya.Polygon(ex[0])

    self.assertEqual(ex, [pya.Polygon.from_s("(0,0;0,300000;300000,300000;300000,0/100000,100000;200000,100000;200000,200000;100000,200000)"), 10000.0, 5000.0, 200])

  # fuzzy compare 
  def test_FuzzyCompare(self):

    p1 = pya.DPolygon.from_s("(0,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p2a = pya.DPolygon.from_s("(0.0000001,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p2b = pya.DPolygon.from_s("(0,0;0,40;40,40;40,0/10.0000001,10;30,10;30,30;10,30)")
    p3a = pya.DPolygon.from_s("(0.0001,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p3b = pya.DPolygon.from_s("(0,0;0,40;40,40;40,0/10.0001,10;30,10;30,30;10,30)")
    p4a = pya.DPolygon.from_s("(0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p4b = pya.DPolygon.from_s("(0,0;1,1;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p4c = pya.DPolygon.from_s("(0,0;0,40;40,40;40,0)")
    p4d = pya.DPolygon.from_s("(0,0;1,1;0,40;40,40;40,0/10,10;30,10;30,30;10,30/15,15;16,15;16,16;15,16)")

    self.assertEqual(p1 == p2a, True)
    self.assertEqual(p1 == p2b, True)
    self.assertEqual(p1 == p3a, False)
    self.assertEqual(p1 == p3b, False)
    self.assertEqual(p1 == p4a, False)
    self.assertEqual(p1 == p4b, False)
    self.assertEqual(p1 == p4c, False)
    self.assertEqual(p1 == p4d, False)
    self.assertEqual(p1 < p2a, False)
    self.assertEqual(p1 < p2b, False)
    self.assertEqual(p1 < p3a, True)
    self.assertEqual(p1 < p3b, True)
    self.assertEqual(p1 < p4a, False)
    self.assertEqual(p1 < p4b, True)
    self.assertEqual(p1 < p4c, False)
    self.assertEqual(p1 < p4d, True)
    self.assertEqual(p4a < p4b, True)
    self.assertEqual(p4a < p4c, False)
    self.assertEqual(p4a < p4d, True)
    self.assertEqual(p4b < p4c, False)
    self.assertEqual(p4b < p4d, True)
    self.assertEqual(p4c < p4d, True)
    self.assertEqual(p2a < p1, False)
    self.assertEqual(p2b < p1, False)
    self.assertEqual(p3a < p1, False)
    self.assertEqual(p3b < p1, False)
    self.assertEqual(p4a < p1, True)
    self.assertEqual(p4b < p1, False)
    self.assertEqual(p4c < p1, True)
    self.assertEqual(p4d < p1, False)
    self.assertEqual(p4b < p4a, False)
    self.assertEqual(p4c < p4a, True)
    self.assertEqual(p4d < p4a, False)
    self.assertEqual(p4c < p4b, True)
    self.assertEqual(p4d < p4b, False)
    self.assertEqual(p4d < p4c, False)

  # hash values 
  def test_HashValues(self):

    p1 = pya.DPolygon.from_s("(0,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p2a = pya.DPolygon.from_s("(0.0000001,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p2b = pya.DPolygon.from_s("(0,0;0,40;40,40;40,0/10.0000001,10;30,10;30,30;10,30)")
    p3a = pya.DPolygon.from_s("(0.0001,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p3b = pya.DPolygon.from_s("(0,0;0,40;40,40;40,0/10.0001,10;30,10;30,30;10,30)")
    p4a = pya.DPolygon.from_s("(0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p4b = pya.DPolygon.from_s("(0,0;1,1;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p4c = pya.DPolygon.from_s("(0,0;0,40;40,40;40,0)")
    p4d = pya.DPolygon.from_s("(0,0;1,1;0,40;40,40;40,0/10,10;30,10;30,30;10,30/15,15;16,15;16,16;15,16)")

    self.assertEqual(p1.hash() == p2a.hash(), True)
    self.assertEqual(p1.hash() == p2b.hash(), True)
    self.assertEqual(p1.hash() == p3a.hash(), False)
    self.assertEqual(p1.hash() == p3b.hash(), False)
    self.assertEqual(p1.hash() == p4a.hash(), False)
    self.assertEqual(p1.hash() == p4b.hash(), False)
    self.assertEqual(p1.hash() == p4c.hash(), False)
    self.assertEqual(p1.hash() == p4d.hash(), False)
    self.assertEqual(p4a.hash() == p4b.hash(), False)
    self.assertEqual(p4a.hash() == p4c.hash(), False)
    self.assertEqual(p4a.hash() == p4d.hash(), False)
    self.assertEqual(p4b.hash() == p4c.hash(), False)
    self.assertEqual(p4b.hash() == p4d.hash(), False)
    self.assertEqual(p4c.hash() == p4d.hash(), False)

    if False:

      # TODO: Currently, complex types are not allowed as hash keys:

      h = { p1: "p1", p3a: "p3a", p3b: "p3b", p4a: "p4a", p4b: "p4b", p4c: "p4c", p4d: "p4d" }

      self.assertEqual(h[p1], "p1")
      self.assertEqual(h[p2a], "p1")
      self.assertEqual(h[p2b], "p1")
      self.assertEqual(h[p3a], "p3a")
      self.assertEqual(h[p3b], "p3b")
      self.assertEqual(h[p4a], "p4a")
      self.assertEqual(h[p4b], "p4b")
      self.assertEqual(h[p4c], "p4c")
      self.assertEqual(h[p4d], "p4d")

  # touches predicate
  def test_touches(self):

    p1 = pya.Polygon(pya.Box(10, 20, 30, 40))
    self.assertEqual(p1.touches(pya.Polygon(pya.Box(30, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.Polygon(pya.Box(31, 20, 40, 50))), False)
    self.assertEqual(p1.touches(pya.Polygon(pya.Box(29, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.SimplePolygon(pya.Box(30, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.SimplePolygon(pya.Box(31, 20, 40, 50))), False)
    self.assertEqual(p1.touches(pya.SimplePolygon(pya.Box(29, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.Box(30, 20, 40, 50)), True)
    self.assertEqual(p1.touches(pya.Box(31, 20, 40, 50)), False)
    self.assertEqual(p1.touches(pya.Box(29, 20, 40, 50)), True)
    self.assertEqual(p1.touches(pya.Edge(30, 20, 40, 50)), True)
    self.assertEqual(p1.touches(pya.Edge(31, 20, 40, 50)), False)
    self.assertEqual(p1.touches(pya.Edge(29, 20, 40, 50)), True)

    p1 = pya.SimplePolygon(pya.Box(10, 20, 30, 40))
    self.assertEqual(p1.touches(pya.Polygon(pya.Box(30, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.Polygon(pya.Box(31, 20, 40, 50))), False)
    self.assertEqual(p1.touches(pya.Polygon(pya.Box(29, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.SimplePolygon(pya.Box(30, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.SimplePolygon(pya.Box(31, 20, 40, 50))), False)
    self.assertEqual(p1.touches(pya.SimplePolygon(pya.Box(29, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.Box(30, 20, 40, 50)), True)
    self.assertEqual(p1.touches(pya.Box(31, 20, 40, 50)), False)
    self.assertEqual(p1.touches(pya.Box(29, 20, 40, 50)), True)
    self.assertEqual(p1.touches(pya.Edge(30, 20, 40, 50)), True)
    self.assertEqual(p1.touches(pya.Edge(31, 20, 40, 50)), False)
    self.assertEqual(p1.touches(pya.Edge(29, 20, 40, 50)), True)

    p1 = pya.DPolygon(pya.DBox(10, 20, 30, 40))
    self.assertEqual(p1.touches(pya.DPolygon(pya.DBox(30, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.DPolygon(pya.DBox(31, 20, 40, 50))), False)
    self.assertEqual(p1.touches(pya.DPolygon(pya.DBox(29, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.DSimplePolygon(pya.DBox(30, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.DSimplePolygon(pya.DBox(31, 20, 40, 50))), False)
    self.assertEqual(p1.touches(pya.DSimplePolygon(pya.DBox(29, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.DBox(30, 20, 40, 50)), True)
    self.assertEqual(p1.touches(pya.DBox(31, 20, 40, 50)), False)
    self.assertEqual(p1.touches(pya.DBox(29, 20, 40, 50)), True)
    self.assertEqual(p1.touches(pya.DEdge(30, 20, 40, 50)), True)
    self.assertEqual(p1.touches(pya.DEdge(31, 20, 40, 50)), False)
    self.assertEqual(p1.touches(pya.DEdge(29, 20, 40, 50)), True)

    p1 = pya.DSimplePolygon(pya.DBox(10, 20, 30, 40))
    self.assertEqual(p1.touches(pya.DPolygon(pya.DBox(30, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.DPolygon(pya.DBox(31, 20, 40, 50))), False)
    self.assertEqual(p1.touches(pya.DPolygon(pya.DBox(29, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.DSimplePolygon(pya.DBox(30, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.DSimplePolygon(pya.DBox(31, 20, 40, 50))), False)
    self.assertEqual(p1.touches(pya.DSimplePolygon(pya.DBox(29, 20, 40, 50))), True)
    self.assertEqual(p1.touches(pya.DBox(30, 20, 40, 50)), True)
    self.assertEqual(p1.touches(pya.DBox(31, 20, 40, 50)), False)
    self.assertEqual(p1.touches(pya.DBox(29, 20, 40, 50)), True)
    self.assertEqual(p1.touches(pya.DEdge(30, 20, 40, 50)), True)
    self.assertEqual(p1.touches(pya.DEdge(31, 20, 40, 50)), False)
    self.assertEqual(p1.touches(pya.DEdge(29, 20, 40, 50)), True)

  def test_selfRef(self):

    # p1 is a reference to the new'd object:
    p1 = pya.Polygon(pya.Box(10, 20, 30, 40)).move(10, 20)
    self.assertEqual(str(p1), "(20,40;20,60;40,60;40,40)")

    pp = pya.Polygon(pya.Box(10, 20, 30, 40))
    p1 = pp.move(10, 20)
    self.assertEqual(str(p1), "(20,40;20,60;40,60;40,40)")
    self.assertEqual(str(pp), "(20,40;20,60;40,60;40,40)")
    pp.move(1, 2)

    # p1 and pp are the same object
    self.assertEqual(str(p1), "(21,42;21,62;41,62;41,42)")
    self.assertEqual(str(pp), "(21,42;21,62;41,62;41,42)")

  def test_voidMethodsReturnSelf(self):

    hull =  [ pya.Point(0, 0),       pya.Point(6000, 0), 
              pya.Point(6000, 3000), pya.Point(0, 3000) ]
    hole1 = [ pya.Point(1000, 1000), pya.Point(2000, 1000), 
              pya.Point(2000, 2000), pya.Point(1000, 2000) ]
    hole2 = [ pya.Point(3000, 1000), pya.Point(4000, 1000), 
              pya.Point(4000, 2000), pya.Point(3000, 2000) ]
    poly = pya.Polygon(hull).insert_hole(hole1).insert_hole(hole2)
    self.assertEqual(str(poly), "(0,0;0,3000;6000,3000;6000,0/1000,1000;2000,1000;2000,2000;1000,2000/3000,1000;4000,1000;4000,2000;3000,2000)")

  def test_argumentShortcuts(self):

    # implicit conversion to a Point array:
    poly = pya.Polygon([ (0,0), (0,1000), (1000,1000) ])
    self.assertEqual(str(poly), "(0,0;0,1000;1000,1000)")

    # issue 1651 - no binding to Box constructor
    poly = pya.Polygon([ (0,0), (0,1000), (1000,1000), (1000,0) ])
    self.assertEqual(str(poly), "(0,0;0,1000;1000,1000;1000,0)")

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBPolygonTests)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)

