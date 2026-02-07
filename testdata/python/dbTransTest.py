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

import pya
import unittest
import sys

class DBTransTests(unittest.TestCase):

  # Transformation basics
  def test_1_DTrans(self):

    a = pya.DTrans()
    b = pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ))
    c = pya.DTrans( 3, True, pya.DPoint( 17, 5 ))
    d = pya.DTrans( pya.DPoint( 17, 5 ))
    e = pya.DTrans( pya.DTrans.M135 )
    e2 = pya.DTrans.from_itrans( pya.Trans.M135 )
    f = pya.DTrans( pya.Trans( pya.Trans.M135, pya.Point( 17, 5 )) )

    self.assertEqual( str(a), "r0 0,0" )
    self.assertEqual( str(pya.DTrans.from_s(str(a))), str(a) )
    self.assertEqual( str(b), "m135 17,5" )
    self.assertEqual( str(c), "m135 17,5" )
    self.assertEqual( str(d), "r0 17,5" )
    self.assertEqual( str(e), "m135 0,0" )
    self.assertEqual( str(e2), "m135 0,0" )
    self.assertEqual( str(f), "m135 17,5" )
    self.assertEqual( str(pya.DTrans.from_s(str(f))), str(f) )

    self.assertEqual( str(b.trans( pya.DPoint( 1, 0 ))), "17,4" )

    self.assertEqual( a == b, False )
    self.assertEqual( a == a, True )
    self.assertEqual( a != b, True )
    self.assertEqual( a != a, False )
    self.assertEqual( (d * e) == b, True )
    self.assertEqual( (e * d) == b, False )

    i = c.inverted()

    self.assertEqual( str(i), "m135 5,17" )
    self.assertEqual( (i * b) == a, True )
    self.assertEqual( (b * i) == a, True )

    c = pya.DTrans( 3, True, pya.DPoint( 17, 5 ))
    self.assertEqual( str(c), "m135 17,5" )
    c.disp = pya.DPoint(1, 7)
    self.assertEqual( str(c), "m135 1,7" )
    c.angle = 1
    self.assertEqual( str(c), "m45 1,7" )
    c.rot = 3
    self.assertEqual( str(c), "r270 1,7" )
    c.mirror = True
    self.assertEqual( str(c), "m135 1,7" )

    self.assertEqual( str(pya.Trans(pya.Trans.R180, 5,-7)), "r180 5,-7" )
    self.assertEqual( str(pya.Trans(pya.Trans.R180, pya.Point(5,-7))), "r180 5,-7" )
    self.assertEqual( str(pya.Trans(pya.Trans.R180, pya.Vector(5,-7))), "r180 5,-7" )
    self.assertEqual( str(pya.Trans(pya.Trans.R180, pya.DVector(5,-7))), "r180 5,-7" )
    self.assertEqual( str(pya.Trans(pya.Trans.R180)), "r180 0,0" )

    self.assertEqual( str(e.trans( pya.Edge(0, 1, 2, 3) )), "(-3,-2;-1,0)" )
    self.assertEqual( str(( e * pya.Edge(0, 1, 2, 3) )), "(-3,-2;-1,0)" )
    self.assertEqual( str(e.trans( pya.Box(0, 1, 2, 3) )), "(-3,-2;-1,0)" )
    self.assertEqual( str(( e * pya.Box(0, 1, 2, 3) )), "(-3,-2;-1,0)" )
    self.assertEqual( str(e.trans( pya.Text("text", pya.Vector(0, 1)) )), "('text',m135 -1,0)" )
    self.assertEqual( str(( e * pya.Text("text", pya.Vector(0, 1)) )), "('text',m135 -1,0)" )
    self.assertEqual( str(e.trans( pya.Polygon( [ pya.Point(0, 1), pya.Point(2, -3), pya.Point(4, 5) ] ) )), "(-5,-4;-1,0;3,-2)" )
    self.assertEqual( str(( e * pya.Polygon( [ pya.Point(0, 1), pya.Point(2, -3), pya.Point(4, 5) ] ) )), "(-5,-4;-1,0;3,-2)" )
    self.assertEqual( str(e.trans( pya.Path( [ pya.Point(0, 1), pya.Point(2, 3) ], 10 ) )), "(-1,0;-3,-2) w=10 bx=0 ex=0 r=false" )
    self.assertEqual( str(( e * pya.Path( [ pya.Point(0, 1), pya.Point(2, 3) ], 10 ) )), "(-1,0;-3,-2) w=10 bx=0 ex=0 r=false" )

    # Constructor variations
    self.assertEqual( str(pya.Trans()), "r0 0,0" )
    self.assertEqual( str(pya.Trans(1)), "r90 0,0" )
    self.assertEqual( str(pya.Trans(2, True)), "m90 0,0" )
    self.assertEqual( str(pya.Trans(2, True, pya.Vector(100, 200))), "m90 100,200" )
    self.assertEqual( str(pya.Trans(2, True, 100, 200)), "m90 100,200" )
    self.assertEqual( str(pya.Trans(pya.Vector(100, 200))), "r0 100,200" )
    self.assertEqual( str(pya.Trans(100, 200)), "r0 100,200" )
    self.assertEqual( str(pya.Trans(pya.Trans(100, 200), 10, 20)), "r0 110,220" )
    self.assertEqual( str(pya.Trans(pya.Trans(100, 200), pya.Vector(10, 20))), "r0 110,220" )

    self.assertEqual( str(pya.DTrans()), "r0 0,0" )
    self.assertEqual( str(pya.DTrans(1)), "r90 0,0" )
    self.assertEqual( str(pya.DTrans(2, True)), "m90 0,0" )
    self.assertEqual( str(pya.DTrans(2, True, pya.DVector(0.1, 0.2))), "m90 0.1,0.2" )
    self.assertEqual( str(pya.DTrans(2, True, 0.1, 0.2)), "m90 0.1,0.2" )
    self.assertEqual( str(pya.DTrans(pya.DVector(0.1, 0.2))), "r0 0.1,0.2" )
    self.assertEqual( str(pya.DTrans(0.1, 0.2)), "r0 0.1,0.2" )
    self.assertEqual( str(pya.DTrans(pya.DTrans(0.1, 0.2), 0.01, 0.02)), "r0 0.11,0.22" )
    self.assertEqual( str(pya.DTrans(pya.DTrans(0.1, 0.2), pya.DVector(0.01, 0.02))), "r0 0.11,0.22" )

  # Magnification basics
  def test_2_DTrans(self):

    a = pya.DTrans()
    b = pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ))
    ma = pya.DCplxTrans( a, 0.5 )
    mb = pya.DCplxTrans( b, 2.0 )
    u = pya.DCplxTrans( a )

    self.assertEqual( str(ma), "r0 *0.5 0,0" )
    self.assertEqual( str(mb), "m135 *2 17,5" )

    self.assertEqual( ma == mb, False )
    self.assertEqual( ma == ma, True )
    self.assertEqual( ma != mb, True )
    self.assertEqual( ma != ma, False )

    i = mb.inverted()

    self.assertEqual( str(i), "m135 *0.5 2.5,8.5" )
    self.assertEqual( str(pya.DCplxTrans.from_s(str(i))), str(i) )
    self.assertEqual( i * mb == u, True )
    self.assertEqual( mb * i == u, True )

    self.assertEqual( str(mb.trans( pya.DPoint( 1, 0 ))), "17,3" )
    self.assertEqual( str(mb.ctrans(2)), "4.0" )
    self.assertEqual( str(i.ctrans(2)), "1.0" )

  # Complex transformation specials
  def test_3_DTrans(self):

    c = pya.DCplxTrans( 5.0, -7.0 )
    self.assertEqual( str(c), "r0 *1 5,-7" )

    c = pya.DCplxTrans( pya.DCplxTrans.M135 )
    self.assertEqual( str(c), "m135 *1 0,0" )
    self.assertEqual( c.is_unity(), False )
    self.assertEqual( c.is_ortho(), True )
    self.assertEqual( c.is_mag(), False )
    self.assertEqual( c.is_mirror(), True )
    self.assertEqual( c.rot(), pya.DCplxTrans.M135.rot() )
    self.assertEqual( str(c.s_trans()), "m135 0,0" )
    self.assertAlmostEqual( c.angle, 270 )

    self.assertEqual( str(c.trans( pya.DEdge(0, 1, 2, 3) )), "(-3,-2;-1,0)" )
    self.assertEqual( str(( c * pya.DEdge(0, 1, 2, 3) )), "(-3,-2;-1,0)" )
    self.assertEqual( str(c.trans( pya.DBox(0, 1, 2, 3) )), "(-3,-2;-1,0)" )
    self.assertEqual( str(( c * pya.DBox(0, 1, 2, 3) )), "(-3,-2;-1,0)" )
    self.assertEqual( str(c.trans( pya.DText("text", pya.DVector(0, 1)) )), "('text',m135 -1,0)" )
    self.assertEqual( str(( c * pya.DText("text", pya.DVector(0, 1)) )), "('text',m135 -1,0)" )
    self.assertEqual( str(c.trans( pya.DPolygon( [ pya.DPoint(0, 1), pya.DPoint(2, -3), pya.DPoint(4, 5) ] ) )), "(-5,-4;-1,0;3,-2)" )
    self.assertEqual( str(( c * pya.DPolygon( [ pya.DPoint(0, 1), pya.DPoint(2, -3), pya.DPoint(4, 5) ] ) )), "(-5,-4;-1,0;3,-2)" )
    self.assertEqual( str(c.trans( pya.DPath( [ pya.DPoint(0, 1), pya.DPoint(2, 3) ], 10 ) )), "(-1,0;-3,-2) w=10 bx=0 ex=0 r=false" )
    self.assertEqual( str(( c * pya.DPath( [ pya.DPoint(0, 1), pya.DPoint(2, 3) ], 10 ) )), "(-1,0;-3,-2) w=10 bx=0 ex=0 r=false" )

    c = pya.DCplxTrans.from_itrans( pya.CplxTrans.M135 )
    self.assertEqual( str(c), "m135 *1 0,0" )

    c = pya.DCplxTrans( 1.5 )
    self.assertEqual( str(c), "r0 *1.5 0,0" )
    self.assertEqual( c.is_unity(), False )
    self.assertEqual( c.is_ortho(), True )
    self.assertEqual( c.is_mag(), True )
    self.assertEqual( c.is_mirror(), False )
    self.assertEqual( c.rot(), pya.DCplxTrans.R0.rot() )
    self.assertEqual( str(c.s_trans()), "r0 0,0" )
    self.assertAlmostEqual( c.angle, 0 )

    c = pya.DCplxTrans( 0.75, 45, True, 2.5, -12.5 )
    self.assertEqual( str(c), "m22.5 *0.75 2.5,-12.5" )
    c = pya.DCplxTrans( 0.75, 45, True, pya.DPoint( 2.5, -12.5 ) )
    self.assertEqual( str(c), "m22.5 *0.75 2.5,-12.5" )
    self.assertEqual( c.is_unity(), False )
    self.assertEqual( c.is_ortho(), False )
    self.assertEqual( c.is_mag(), True )
    self.assertEqual( c.rot(), pya.DCplxTrans.M0.rot() )
    self.assertEqual( str(c.s_trans()), "m0 2.5,-12.5" )
    self.assertAlmostEqual( c.angle, 45 )

    self.assertEqual( str(c.ctrans( 5 )), "3.75" )
    self.assertEqual( str(c.trans( pya.DPoint( 12, 16 ) )), "17.3492424049,-14.6213203436" )

    self.assertEqual( str(pya.DCplxTrans()), "r0 *1 0,0" )
    self.assertEqual( pya.DCplxTrans().is_unity(), True )
    self.assertEqual( (c * c.inverted()).is_unity(), True )

    c.mirror = False
    self.assertEqual( str(c), "r45 *0.75 2.5,-12.5" )
    c.mag = 1.5
    self.assertEqual( str(c), "r45 *1.5 2.5,-12.5" )
    c.disp = pya.DPoint( -1.0, 5.5 )
    self.assertEqual( str(c), "r45 *1.5 -1,5.5" )
    self.assertEqual( c.mag, 1.5 )
    c.angle = 60
    self.assertEqual( str(c), "r60 *1.5 -1,5.5" )
    self.assertEqual( ("%g" % c.angle), "60" )

    # Constructor variations
    self.assertEqual( str(pya.ICplxTrans()), "r0 *1 0,0" )
    self.assertEqual( str(pya.ICplxTrans(1.5)), "r0 *1.5 0,0" )
    self.assertEqual( str(pya.ICplxTrans(pya.Trans(1, False, 10, 20), 1.5)), "r90 *1.5 10,20" )
    self.assertEqual( str(pya.ICplxTrans(pya.Trans(1, False, 10, 20))), "r90 *1 10,20" )
    self.assertEqual( str(pya.ICplxTrans(1.5, 80, True, pya.Vector(100, 200))), "m40 *1.5 100,200" )
    self.assertEqual( str(pya.ICplxTrans(1.5, 80, True, 100, 200)), "m40 *1.5 100,200" )
    self.assertEqual( str(pya.ICplxTrans(pya.Vector(100, 200))), "r0 *1 100,200" )
    self.assertEqual( str(pya.ICplxTrans(100, 200)), "r0 *1 100,200" )
    self.assertEqual( str(pya.ICplxTrans(pya.ICplxTrans(100, 200))), "r0 *1 100,200" )
    self.assertEqual( str(pya.ICplxTrans(pya.ICplxTrans(100, 200), 1.5)), "r0 *1.5 150,300" )
    self.assertEqual( str(pya.ICplxTrans(pya.ICplxTrans(100, 200), 1.5, pya.Vector(10, 20))), "r0 *1.5 160,320" )
    self.assertEqual( str(pya.ICplxTrans(pya.ICplxTrans(100, 200), 1.5, 10, 20)), "r0 *1.5 160,320" )

    self.assertEqual( str(pya.DCplxTrans()), "r0 *1 0,0" )
    self.assertEqual( str(pya.DCplxTrans(1.5)), "r0 *1.5 0,0" )
    self.assertEqual( str(pya.DCplxTrans(pya.DTrans(1, False, 0.01, 0.02), 1.5)), "r90 *1.5 0.01,0.02" )
    self.assertEqual( str(pya.DCplxTrans(pya.DTrans(1, False, 0.01, 0.02))), "r90 *1 0.01,0.02" )
    self.assertEqual( str(pya.DCplxTrans(1.5, 80, True, pya.DVector(0.1, 0.2))), "m40 *1.5 0.1,0.2" )
    self.assertEqual( str(pya.DCplxTrans(1.5, 80, True, 0.1, 0.2)), "m40 *1.5 0.1,0.2" )
    self.assertEqual( str(pya.DCplxTrans(pya.DVector(0.1, 0.2))), "r0 *1 0.1,0.2" )
    self.assertEqual( str(pya.DCplxTrans(0.1, 0.2)), "r0 *1 0.1,0.2" )
    self.assertEqual( str(pya.DCplxTrans(pya.DCplxTrans(0.1, 0.2))), "r0 *1 0.1,0.2" )
    self.assertEqual( str(pya.DCplxTrans(pya.DCplxTrans(0.1, 0.2), 1.5)), "r0 *1.5 0.15,0.3" )
    self.assertEqual( str(pya.DCplxTrans(pya.DCplxTrans(0.1, 0.2), 1.5, pya.DVector(0.01, 0.02))), "r0 *1.5 0.16,0.32" )
    self.assertEqual( str(pya.DCplxTrans(pya.DCplxTrans(0.1, 0.2), 1.5, 0.01, 0.02)), "r0 *1.5 0.16,0.32" )

  # Transformation basics
  def test_1_Trans(self):

    a = pya.Trans()
    b = pya.Trans( pya.Trans.M135, pya.Point( 17, 5 ))
    c = pya.Trans( 3, True, pya.Point( 17, 5 ))
    d = pya.Trans( pya.Point( 17, 5 ))
    e = pya.Trans( pya.Trans.M135 )
    e2 = pya.Trans.from_dtrans( pya.DTrans.M135 )
    f = pya.Trans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 )) )

    self.assertEqual( str(a), "r0 0,0" )
    self.assertEqual( str(pya.Trans.from_s(str(a))), str(a) )
    self.assertEqual( str(b), "m135 17,5" )
    self.assertEqual( str(c), "m135 17,5" )
    self.assertEqual( str(d), "r0 17,5" )
    self.assertEqual( str(e), "m135 0,0" )
    self.assertEqual( str(e2), "m135 0,0" )
    self.assertEqual( str(f), "m135 17,5" )
    self.assertEqual( str(pya.Trans.from_s(str(f))), str(f) )

    self.assertEqual( str(b.trans( pya.Point( 1, 0 ))), "17,4" )

    self.assertEqual( a == b, False )
    self.assertEqual( a == a, True )
    self.assertEqual( a != b, True )
    self.assertEqual( a != a, False )
    self.assertEqual( (d * e) == b, True )
    self.assertEqual( (e * d) == b, False )

    i = c.inverted()

    self.assertEqual( str(i), "m135 5,17" )
    self.assertEqual( (i * b) == a, True )
    self.assertEqual( (b * i) == a, True )

    c = pya.Trans( 3, True, pya.Point( 17, 5 ))
    self.assertEqual( str(c), "m135 17,5" )
    c.disp = pya.Point(1, 7)
    self.assertEqual( str(c), "m135 1,7" )
    c.angle = 1
    self.assertEqual( str(c), "m45 1,7" )
    c.rot = 3
    self.assertEqual( str(c), "r270 1,7" )
    c.mirror = True
    self.assertEqual( str(c), "m135 1,7" )

    self.assertEqual( str(e.trans( pya.Edge(0, 1, 2, 3) )), "(-3,-2;-1,0)" )
    self.assertEqual( str(( e * pya.Edge(0, 1, 2, 3) )), "(-3,-2;-1,0)" )
    self.assertEqual( str(e.trans( pya.Box(0, 1, 2, 3) )), "(-3,-2;-1,0)" )
    self.assertEqual( str(( e * pya.Box(0, 1, 2, 3) )), "(-3,-2;-1,0)" )
    self.assertEqual( str(e.trans( pya.Text("text", pya.Vector(0, 1)) )), "('text',m135 -1,0)" )
    self.assertEqual( str(( e * pya.Text("text", pya.Vector(0, 1)) )), "('text',m135 -1,0)" )
    self.assertEqual( str(e.trans( pya.Polygon( [ pya.Point(0, 1), pya.Point(2, -3), pya.Point(4, 5) ] ) )), "(-5,-4;-1,0;3,-2)" )
    self.assertEqual( str(( e * pya.Polygon( [ pya.Point(0, 1), pya.Point(2, -3), pya.Point(4, 5) ] ) )), "(-5,-4;-1,0;3,-2)" )
    self.assertEqual( str(e.trans( pya.Path( [ pya.Point(0, 1), pya.Point(2, 3) ], 10 ) )), "(-1,0;-3,-2) w=10 bx=0 ex=0 r=false" )
    self.assertEqual( str(( e * pya.Path( [ pya.Point(0, 1), pya.Point(2, 3) ], 10 ) )), "(-1,0;-3,-2) w=10 bx=0 ex=0 r=false" )

  # Complex transformation basics
  def test_2_Trans(self):

    a = pya.Trans()
    b = pya.Trans( pya.Trans.M135, pya.Point( 17, 5 ))
    ma = pya.CplxTrans( a, 0.5 )
    mb = pya.CplxTrans( b, 2.0 )
    u = pya.CplxTrans( a )

    self.assertEqual( str(ma), "r0 *0.5 0,0" )
    self.assertEqual( str(mb), "m135 *2 17,5" )

    self.assertEqual( ma == mb, False )
    self.assertEqual( ma == ma, True )
    self.assertEqual( ma != mb, True )
    self.assertEqual( ma != ma, False )

    self.assertEqual( str(mb.inverted()), "m135 *0.5 2.5,8.5" )

    i = mb.dup()
    i.invert()

    self.assertEqual( str(i), "m135 *0.5 2.5,8.5" )
    self.assertEqual( i * mb == u, True )
    self.assertEqual( mb * i == u, True )

    self.assertEqual( str(mb.trans( pya.Point( 1, 0 ))), "17,3" )
    self.assertEqual( str(mb.ctrans(2)), "4.0" )
    self.assertEqual( str(i.ctrans(2)), "1.0" )

  # Complex transformation specials
  def test_3_Trans(self):

    c = pya.CplxTrans( 5, -7 )
    self.assertEqual( str(c), "r0 *1 5,-7" )
    self.assertEqual( str(pya.CplxTrans.from_s(str(c))), str(c) )

    c = pya.CplxTrans( pya.CplxTrans.M135 )
    self.assertEqual( str(c), "m135 *1 0,0" )
    self.assertEqual( c.is_unity(), False )
    self.assertEqual( c.is_ortho(), True )
    self.assertEqual( c.is_mag(), False )
    self.assertEqual( c.is_mirror(), True )
    self.assertEqual( c.rot(), pya.CplxTrans.M135.rot() )
    self.assertEqual( str(c.s_trans()), "m135 0,0" )
    self.assertAlmostEqual( c.angle, 270 )

    c = pya.CplxTrans.from_dtrans( pya.DCplxTrans.M135 )
    self.assertEqual( str(c), "m135 *1 0,0" )

    c = pya.CplxTrans( 1.5 )
    self.assertEqual( str(c), "r0 *1.5 0,0" )
    self.assertEqual( c.is_unity(), False )
    self.assertEqual( c.is_ortho(), True )
    self.assertEqual( c.is_mag(), True )
    self.assertEqual( c.is_mirror(), False )
    self.assertEqual( c.rot(), pya.CplxTrans.R0.rot() )
    self.assertEqual( str(c.s_trans()), "r0 0,0" )
    self.assertAlmostEqual( c.angle, 0 )

    c = pya.CplxTrans( 0.75, 45, True, 2.5, -12.5 )
    self.assertEqual( str(c), "m22.5 *0.75 2.5,-12.5" )
    self.assertEqual( str(pya.CplxTrans.from_s(str(c))), str(c) )
    c = pya.CplxTrans( 0.75, 45, True, pya.DPoint( 2.5, -12.5 ) )
    self.assertEqual( str(c), "m22.5 *0.75 2.5,-12.5" )
    self.assertEqual( c.is_unity(), False )
    self.assertEqual( c.is_ortho(), False )
    self.assertEqual( c.is_mag(), True )
    self.assertEqual( c.rot(), pya.CplxTrans.M0.rot() )
    self.assertEqual( str(c.s_trans()), "m0 3,-13" )
    self.assertAlmostEqual( c.angle, 45 )

    self.assertEqual( str(c.ctrans( 5 )), "3.75" )
    self.assertEqual( str(c.trans( pya.Point( 12, 16 ) )), "17.3492424049,-14.6213203436" )

    self.assertEqual( str(pya.CplxTrans()), "r0 *1 0,0" )
    self.assertEqual( pya.CplxTrans().is_unity(), True )
    self.assertEqual( (c.inverted() * c).is_unity(), True )

    c.mirror = False
    self.assertEqual( str(c), "r45 *0.75 2.5,-12.5" )
    c.mag = 1.5
    self.assertEqual( str(c), "r45 *1.5 2.5,-12.5" )
    c.disp = pya.DPoint( -1.0, 5.5 )
    self.assertEqual( str(c), "r45 *1.5 -1,5.5" )
    self.assertEqual( c.mag, 1.5 )
    c.angle = 60
    self.assertEqual( str(c), "r60 *1.5 -1,5.5" )
    self.assertEqual( ("%g" % c.angle), "60" )

  # Complex transformation types
  def test_4_Trans(self):

    a = pya.Trans()
    m = pya.CplxTrans( a, 1.1 )
    da = pya.DTrans()
    dm = pya.DCplxTrans( da, 1.1 )

    self.assertEqual( str(m), "r0 *1.1 0,0" )
    self.assertEqual( str(pya.DCplxTrans.from_s(str(m))), str(m) )
    self.assertEqual( str(m.trans( pya.Point( 5, -7 ))), "5.5,-7.7" )

    im = pya.ICplxTrans( a, 0.5 )
    im_old = im.dup()

    self.assertEqual( str(im), "r0 *0.5 0,0" )
    self.assertEqual( str(pya.ICplxTrans.from_s(str(im))), str(im) )
    self.assertEqual( str(im.trans( pya.Point( 5, -7 ))), "3,-4" )

    im = pya.ICplxTrans(m)
    self.assertEqual( str(im), "r0 *1.1 0,0" )
    self.assertEqual( str(im.trans( pya.Point( 5, -7 ))), "6,-8" )

    im = pya.ICplxTrans(dm)
    self.assertEqual( str(im), "r0 *1.1 0,0" )
    self.assertEqual( str(im.trans( pya.Point( 5, -7 ))), "6,-8" )

    im.assign(im_old)
    self.assertEqual( str(im), "r0 *0.5 0,0" )
    self.assertEqual( str(im.trans( pya.Point( 5, -7 ))), "3,-4" )

    self.assertEqual( str(pya.ICplxTrans(5,-7)), "r0 *1 5,-7" )

    self.assertEqual( str(pya.ICplxTrans(pya.ICplxTrans.R180, 1.5, 5,-7)), "r180 *1.5 5,-7" )
    self.assertEqual( str(pya.ICplxTrans(pya.ICplxTrans.R180, 1.5, pya.Point(5,-7))), "r180 *1.5 5,-7" )
    self.assertEqual( str(pya.ICplxTrans(pya.ICplxTrans.R180, 1.5, pya.Vector(5,-7))), "r180 *1.5 5,-7" )
    self.assertEqual( str(pya.ICplxTrans(pya.ICplxTrans.R180, 1.5, pya.DVector(5,-7))), "r180 *1.5 5,-7" )
    self.assertEqual( str(pya.ICplxTrans(pya.ICplxTrans.R180, 1.5)), "r180 *1.5 0,0" )

    c = pya.ICplxTrans.from_dtrans( pya.DCplxTrans.M135 )
    self.assertEqual( str(c), "m135 *1 0,0" )
    c = pya.ICplxTrans.from_trans( pya.CplxTrans.M135 )
    self.assertEqual( str(c), "m135 *1 0,0" )

  # Fuzzy compare
  def test_5_Trans_FuzzyCompare(self):

    t1 = pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ))
    t2 = pya.DTrans( pya.DTrans.M135, pya.DPoint( 17 + 1e-7, 5 ))
    t3 = pya.DTrans( pya.DTrans.M135, pya.DPoint( 17 + 1e-4, 5 ))
    t4a = pya.DTrans( pya.DTrans.M135, pya.DPoint( 18, 5 ))
    t4b = pya.DTrans( pya.DTrans.R90, pya.DPoint( 18, 5 ))

    self.assertEqual(t1 == t2, True)
    self.assertEqual(t1 != t2, False)
    self.assertEqual(t1 < t2, False)
    self.assertEqual(t2 < t1, False)

    self.assertEqual(t1 == t3, False)
    self.assertEqual(t1 != t3, True)
    self.assertEqual(t1 < t3, True)
    self.assertEqual(t3 < t1, False)

    self.assertEqual(t1 == t4a, False)
    self.assertEqual(t1 != t4a, True)
    self.assertEqual(t1 < t4a, True)
    self.assertEqual(t4a < t1, False)

    self.assertEqual(t1 == t4b, False)
    self.assertEqual(t1 != t4b, True)
    self.assertEqual(t1 < t4b, False)
    self.assertEqual(t4b < t1, True)

  # Complex trans fuzzy compare
  def test_5_CplxTrans_FuzzyCompare(self):

    t1 = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ) ), 1.0)
    t2a = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17 + 1e-7, 5 ) ), 1.0)
    t2b = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ) ), 1.0 + 1e-11)
    t2c = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ) ), 1.0)
    t2c.angle = t2c.angle + 1e-11
    t3a = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17 + 1e-4, 5 ) ), 1.0)
    t3b = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ) ), 1.0 + 1e-4)
    t3c = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ) ), 1.0)
    t3c.angle = t3c.angle + 1e-4
    t4 = pya.DCplxTrans( pya.DTrans( pya.DTrans.R90, pya.DPoint( 18, 5 ) ), 1.0)

    self.assertEqual(t1 == t2a, True)
    self.assertEqual(t1 != t2a, False)
    self.assertEqual(t1 < t2a, False)
    self.assertEqual(t2a < t1, False)

    self.assertEqual(t1 == t2b, True)
    self.assertEqual(t1 != t2b, False)
    self.assertEqual(t1 < t2b, False)
    self.assertEqual(t2b < t1, False)

    self.assertEqual(t1 == t2c, True)
    self.assertEqual(t1 != t2c, False)
    self.assertEqual(t1 < t2c, False)
    self.assertEqual(t2c < t1, False)

    self.assertEqual(t1 == t3a, False)
    self.assertEqual(t1 != t3a, True)
    self.assertEqual(t1 < t3a, True)
    self.assertEqual(t3a < t1, False)

    self.assertEqual(t1 == t3b, False)
    self.assertEqual(t1 != t3b, True)
    self.assertEqual(t1 < t3b, False)
    self.assertEqual(t3b < t1, True)

    self.assertEqual(t1 == t3c, False)
    self.assertEqual(t1 != t3c, True)
    self.assertEqual(t1 < t3c, True)
    self.assertEqual(t3c < t1, False)

    self.assertEqual(t3a == t3b, False)
    self.assertEqual(t3a != t3b, True)
    self.assertEqual(t3a < t3b, False)
    self.assertEqual(t3b < t3a, True)

    self.assertEqual(t3a == t3c, False)
    self.assertEqual(t3a != t3c, True)
    self.assertEqual(t3a < t3c, False)
    self.assertEqual(t3c < t3a, True)

    self.assertEqual(t3b == t3c, False)
    self.assertEqual(t3b != t3c, True)
    self.assertEqual(t3b < t3c, True)
    self.assertEqual(t3c < t3b, False)

    self.assertEqual(t1 == t4, False)
    self.assertEqual(t1 != t4, True)
    self.assertEqual(t1 < t4, True)
    self.assertEqual(t4 < t1, False)

  # Hash values
  def test_5_Trans_Hash(self):

    t1 = pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ))
    t2 = pya.DTrans( pya.DTrans.M135, pya.DPoint( 17 + 1e-7, 5 ))
    t3 = pya.DTrans( pya.DTrans.M135, pya.DPoint( 17 + 1e-4, 5 ))
    t4a = pya.DTrans( pya.DTrans.M135, pya.DPoint( 18, 5 ))
    t4b = pya.DTrans( pya.DTrans.R90, pya.DPoint( 18, 5 ))

    self.assertEqual(t1.hash() == t2.hash(), True)
    self.assertEqual(t1.hash() == t3.hash(), False)
    self.assertEqual(t1.hash() == t4a.hash(), False)
    self.assertEqual(t1.hash() == t4b.hash(), False)
    self.assertEqual(hash(t1) == hash(t2), True)
    self.assertEqual(hash(t1) == hash(t3), False)
    self.assertEqual(hash(t1) == hash(t4a), False)
    self.assertEqual(hash(t1) == hash(t4b), False)
    self.assertEqual(t1.__hash__() == t2.__hash__(), True)
    self.assertEqual(t1.__hash__() == t3.__hash__(), False)
    self.assertEqual(t1.__hash__() == t4a.__hash__(), False)
    self.assertEqual(t1.__hash__() == t4b.__hash__(), False)

    # Transformations can't be used as hash keys currently
    if False:

      h = { t1: "t1", t3: "t3", t4a: "t4a", t4b: "t4b" }

      self.assertEqual(h[t1], "t1")
      self.assertEqual(h[t2], "t1")
      self.assertEqual(h[t3], "t3")
      self.assertEqual(h[t4a], "t4a")
      self.assertEqual(h[t4b], "t4b")

  # Complex trans hash values
  def test_5_CplxTrans_Hash(self):

    t1 = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ) ), 1.0)
    t2a = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17 + 1e-7, 5 ) ), 1.0)
    t2b = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ) ), 1.0 + 1e-11)
    t2c = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ) ), 1.0)
    t2c.angle = t2c.angle + 1e-11
    t3a = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17 + 1e-4, 5 ) ), 1.0)
    t3b = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ) ), 1.0 + 1e-4)
    t3c = pya.DCplxTrans( pya.DTrans( pya.DTrans.M135, pya.DPoint( 17, 5 ) ), 1.0)
    t3c.angle = t3c.angle + 1e-4
    t4 = pya.DCplxTrans( pya.DTrans( pya.DTrans.R90, pya.DPoint( 18, 5 ) ), 1.0)

    self.assertEqual(t1.hash() == t2a.hash(), True)
    self.assertEqual(t1.hash() == t2b.hash(), True)
    self.assertEqual(t1.hash() == t2c.hash(), True)
    self.assertEqual(t1.hash() == t3a.hash(), False)
    self.assertEqual(t1.hash() == t3b.hash(), False)
    self.assertEqual(t1.hash() == t3c.hash(), False)
    self.assertEqual(t3a.hash() == t3b.hash(), False)
    self.assertEqual(t3a.hash() == t3c.hash(), False)
    self.assertEqual(t3b.hash() == t3c.hash(), False)
    self.assertEqual(t1.hash() == t4.hash(), False)

    # Transformations can't be used as hash keys currently
    if False:

      h = { t1: "t1", t3a: "t3a", t3b: "t3b", t3c: "t3c", t4: "t4" }

      self.assertEqual(h[t1], "t1")
      self.assertEqual(h[t2a], "t1")
      self.assertEqual(h[t2b], "t1")
      self.assertEqual(h[t2c], "t1")
      self.assertEqual(h[t3a], "t3a")
      self.assertEqual(h[t3b], "t3b")
      self.assertEqual(h[t3c], "t3c")
      self.assertEqual(h[t4], "t4")


# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBTransTests)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)

