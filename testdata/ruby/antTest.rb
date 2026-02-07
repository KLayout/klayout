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

def annot_obj( p1, p2, fmt, fmt_x, fmt_y, style, outline, snap, ac )
  a = RBA::Annotation::new
  a.p1 = p1
  a.p2 = p2
  a.fmt = fmt
  a.fmt_x = fmt_x
  a.fmt_y = fmt_y
  a.style = style
  a.outline = outline
  a.snap = snap
  a.angle_constraint = ac
  return a
end

def annot_obj2( pts, fmt, fmt_x, fmt_y, style, outline, snap, ac )
  a = RBA::Annotation::new
  a.points = pts
  a.fmt = fmt
  a.fmt_x = fmt_x
  a.fmt_y = fmt_y
  a.style = style
  a.outline = outline
  a.snap = snap
  a.angle_constraint = ac
  return a
end

class RBA::Annotation
  def style_to_s(s)
    if s == StyleRuler
      "ruler"
    elsif s == StyleArrowEnd
      "arrow_end"
    elsif s == StyleArrowStart
      "arrow_start"
    elsif s == StyleArrowBoth
      "arrow_both"
    elsif s == StyleLine
      "line"
    elsif s == StyleCrossEnd
      "cross_end"
    elsif s == StyleCrossStart
      "cross_start"
    elsif s == StyleCrossBoth
      "cross_both"
    else
      ""
    end
  end
  def outline_to_s(s)
    if s == OutlineDiag
      "diag"
    elsif s == OutlineXY
      "xy"
    elsif s == OutlineDiagXY
      "diag_xy"
    elsif s == OutlineYX
      "yx"
    elsif s == OutlineDiagYX
      "diag_yx"
    elsif s == OutlineBox
      "diag_box"
    elsif s == OutlineEllipse
      "ellipse"
    elsif s == OutlineAngle
      "angle"
    elsif s == OutlineRadius
      "radius"
    else
      ""
    end
  end
  def ac_to_s(s)
    if s == AngleAny
      "any"
    elsif s == AngleDiagonal
      "diagonal"
    elsif s == AngleDiagonalOnly
      "diagonal_only"
    elsif s == AngleOrtho
      "ortho"
    elsif s == AngleHorizontal
      "horizontal"
    elsif s == AngleVertical
      "vertical"
    elsif s == AngleGlobal
      "global"
    else
      ""
    end
  end
  def pos_to_s(s)
    if s == PositionAuto
      "auto"
    elsif s == PositionP1
      "p1"
    elsif s == PositionP2
      "p2"
    elsif s == PositionCenter
      "center"
    else
      ""
    end
  end
  def align_to_s(s)
    if s == AlignAuto
      "auto"
    elsif s == AlignCenter
      "center"
    elsif s == AlignUp
      "up"
    elsif s == AlignDown
      "down"
    else
      ""
    end
  end
  def to_s_debug
    s = "p1=" + p1.to_s + ", p2=" + p2.to_s + ", fmt=" + fmt + ", fmt_x=" + fmt_x + ", fmt_y=" + fmt_y +
      ", style=" + self.style_to_s(style) + ", outline=" + self.outline_to_s(outline) + ", snap=" + snap?.to_s + ", ac=" + self.ac_to_s(angle_constraint)
    if category != ""
      s += ", category=" + self.category
    end
    if main_position != PositionAuto
      s += ", main_position=" + self.pos_to_s(main_position)
    end
    if main_xalign != AlignAuto
      s += ", main_xalign=" + self.align_to_s(main_xalign)
    end
    if main_yalign != AlignAuto
      s += ", main_yalign=" + self.align_to_s(main_yalign)
    end
    if xlabel_xalign != AlignAuto
      s += ", xlabel_xalign=" + self.align_to_s(xlabel_xalign)
    end
    if xlabel_yalign != AlignAuto
      s += ", xlabel_yalign=" + self.align_to_s(xlabel_yalign)
    end
    if ylabel_xalign != AlignAuto
      s += ", ylabel_xalign=" + self.align_to_s(ylabel_xalign)
    end
    if ylabel_yalign != AlignAuto
      s += ", ylabel_yalign=" + self.align_to_s(ylabel_yalign)
    end
    s
  end
end

class Ant_TestClass < TestBase

  # Annotation object
  def test_1

    if !RBA.constants.member?(:Application)
      return
    end

    a = RBA::Annotation::new
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag, snap=true, ac=global" )

    a.outline = RBA::Annotation::OutlineXY
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=xy, snap=true, ac=global" )
    a.outline = RBA::Annotation::OutlineDiagXY
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag_xy, snap=true, ac=global" )
    a.outline = RBA::Annotation::OutlineYX
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=yx, snap=true, ac=global" )
    a.outline = RBA::Annotation::OutlineDiagYX
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag_yx, snap=true, ac=global" )
    a.outline = RBA::Annotation::OutlineBox
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag_box, snap=true, ac=global" )
    a.outline = RBA::Annotation::OutlineEllipse
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=ellipse, snap=true, ac=global" )
    a.outline = RBA::Annotation::OutlineAngle
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=angle, snap=true, ac=global" )
    a.outline = RBA::Annotation::OutlineRadius
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=radius, snap=true, ac=global" )
    a.outline = RBA::Annotation::OutlineDiag
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag, snap=true, ac=global" )

    a.style = RBA::Annotation::StyleArrowEnd
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=arrow_end, outline=diag, snap=true, ac=global" )
    a.style = RBA::Annotation::StyleArrowStart
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=arrow_start, outline=diag, snap=true, ac=global" )
    a.style = RBA::Annotation::StyleArrowBoth
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=arrow_both, outline=diag, snap=true, ac=global" )
    a.style = RBA::Annotation::StyleLine
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=line, outline=diag, snap=true, ac=global" )
    a.style = RBA::Annotation::StyleCrossEnd
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=cross_end, outline=diag, snap=true, ac=global" )
    a.style = RBA::Annotation::StyleCrossStart
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=cross_start, outline=diag, snap=true, ac=global" )
    a.style = RBA::Annotation::StyleCrossBoth
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=cross_both, outline=diag, snap=true, ac=global" )
    a.style = RBA::Annotation::StyleRuler
    assert_equal( a.to_s_debug, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag, snap=true, ac=global" )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )

    a = annot_obj2( [ RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ) ], "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )

    b = a.dup
    assert_equal( b.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    a = RBA::Annotation::new

    assert_equal( a == b, false )
    assert_equal( a != b, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a == b, true )
    assert_equal( a != b, false )

    c = a.transformed( RBA::DTrans::new( RBA::DPoint::new( 10.0, 20.0 ) ) )
    assert_equal( c.to_s_debug, "p1=11,22, p2=13,24, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )

    c = a.transformed_cplx( RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DPoint::new( 11.0, 22.0 ) ) ) )
    assert_equal( c.to_s_debug, "p1=12,24, p2=14,26, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )

    a.fmt = "X=$X,Y=$Y,D=$D,A=$A,P=$P,Q=$Q,U=$U,V=$V"
    a.fmt_x = "$(X*1000)+$(Y*1e3)"
    assert_equal( a.text == "X=2,Y=2,D=2.8284271,A=4e-06,P=3,Q=4,U=1,V=2" || a.text == "X=2,Y=2,D=2.8284271,A=4e-006,P=3,Q=4,U=1,V=2", true )
    assert_equal( a.text_x, "2000+2000" )
    assert_equal( a.text_y, "c" )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 3 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=diagonal_only" )
    aa = a.dup
    aa.category = "abc"
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=diagonal_only, category=abc" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.main_position = RBA::Annotation::PositionP1
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, main_position=p1" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.main_position = RBA::Annotation::PositionP2
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, main_position=p2" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.main_position = RBA::Annotation::PositionCenter
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, main_position=center" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.main_xalign = RBA::Annotation::AlignCenter
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, main_xalign=center" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.main_xalign = RBA::Annotation::AlignLeft
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, main_xalign=down" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.main_xalign = RBA::Annotation::AlignRight
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, main_xalign=up" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.main_yalign = RBA::Annotation::AlignCenter
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, main_yalign=center" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.main_yalign = RBA::Annotation::AlignLeft
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, main_yalign=down" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.main_yalign = RBA::Annotation::AlignRight
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, main_yalign=up" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.xlabel_xalign = RBA::Annotation::AlignCenter
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, xlabel_xalign=center" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.xlabel_xalign = RBA::Annotation::AlignLeft
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, xlabel_xalign=down" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.xlabel_xalign = RBA::Annotation::AlignRight
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, xlabel_xalign=up" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.xlabel_yalign = RBA::Annotation::AlignCenter
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, xlabel_yalign=center" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.xlabel_yalign = RBA::Annotation::AlignLeft
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, xlabel_yalign=down" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.xlabel_yalign = RBA::Annotation::AlignRight
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, xlabel_yalign=up" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.ylabel_xalign = RBA::Annotation::AlignCenter
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, ylabel_xalign=center" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.ylabel_xalign = RBA::Annotation::AlignLeft
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, ylabel_xalign=down" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.ylabel_xalign = RBA::Annotation::AlignRight
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, ylabel_xalign=up" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.ylabel_yalign = RBA::Annotation::AlignCenter
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, ylabel_yalign=center" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.ylabel_yalign = RBA::Annotation::AlignLeft
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, ylabel_yalign=down" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )
    aa = a.dup
    aa.ylabel_yalign = RBA::Annotation::AlignRight
    assert_equal( aa.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal, ylabel_yalign=up" )
    assert_equal( a == aa, false )
    a = aa.dup
    assert_equal( a == aa, true )

  end

  # Test LayoutView integration
  def test_2

    if !RBA.constants.member?(:Application)
      return
    end

    mw = RBA::Application::instance.main_window
    mw.close_all
    mw.create_layout( 0 )
    lv = mw.current_view

    aac = 0
    ac = []
    asc = 0
    lv.on_annotations_changed { aac += 1 }
    lv.on_annotation_changed { |id| ac << id }
    lv.on_annotation_selection_changed { asc += 1 }

    lv.insert_annotation( RBA::Annotation::new )
    assert_equal( aac, 1 )
    assert_equal( ac, [] )
    assert_equal( asc, 0 )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.is_valid?, false )
    lv.insert_annotation( a )
    assert_equal( aac, 2 )
    assert_equal( ac, [] )
    assert_equal( asc, 0 )
    assert_equal( a.is_valid?, true )
    assert_equal( lv.annotation( -1 ).is_valid?, false )
    assert_equal( lv.annotation( a.id ).is_valid?, true )
    assert_equal( lv.annotation( a.id ).to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s_debug ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag, snap=true, ac=global;p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )

    id = nil
    lv.each_annotation { |a| id = a.id }
    assert_equal( a.is_valid?, true )
    a.detach
    assert_equal( a.is_valid?, false )
    a.fmt = "u"
    assert_equal( aac, 2 )
    assert_equal( ac, [ ] )
    assert_equal( asc, 0 )
    lv.replace_annotation(id, a)
    # NOTE: currently, a spontaneous event is also emitted from the "list changed" event
    # even if something on the properties has changed.
    assert_equal( aac, 3 )
    assert_equal( ac, [ id ] )
    # This is also a side effect of the properties changes - may be removed in the future
    assert_equal( asc, 1 )

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s_debug ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag, snap=true, ac=global;p1=1,2, p2=3,4, fmt=u, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )

    lv.erase_annotation(id)

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s_debug ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag, snap=true, ac=global" )

    lv.clear_annotations

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s_debug ) }
    assert_equal( arr.join( ";" ), "" )

    mw.close_all

  end

  # Test LayoutView integration with live updates
  def test_3

    if !RBA.constants.member?(:Application)
      return
    end

    mw = RBA::Application::instance.main_window
    mw.close_all
    mw.create_layout( 0 )
    lv = mw.current_view

    a0 = RBA::Annotation::new
    lv.insert_annotation( a0 )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    lv.insert_annotation( a )

    arr = []
    lv.each_annotation { |x| arr.push( x.to_s_debug ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag, snap=true, ac=global;p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )

    a.fmt = "u"

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s_debug ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag, snap=true, ac=global;p1=1,2, p2=3,4, fmt=u, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )

    a.delete

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s_debug ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag, snap=true, ac=global" )

    a0.delete

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s_debug ) }
    assert_equal( arr.join( ";" ), "" )

    a0 = RBA::Annotation::new
    lv.insert_annotation( a0 )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    lv.insert_annotation( a )

    arr = []
    lv.each_annotation { |x| arr.push( x.to_s_debug ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag, snap=true, ac=global;p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )

    lv.each_annotation { |x| x.fmt = "q" }

    arr = []
    lv.each_annotation { |x| arr.push( x.to_s_debug ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=q, fmt_x=$X, fmt_y=$Y, style=ruler, outline=diag, snap=true, ac=global;p1=1,2, p2=3,4, fmt=q, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal" )

    lv.each_annotation { |x| x.delete }

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s_debug ) }
    assert_equal( arr.join( ";" ), "" )

    mw.close_all

  end

  # each_annotation_selected
  def test_4

    if !RBA.constants.member?(:Application)
      return
    end

    mw = RBA::Application::instance.main_window
    mw.close_all
    mw.create_layout( 0 )
    lv = mw.current_view

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 4 )
    assert_equal( a.is_valid?, false )
    lv.insert_annotation( a )

    mw.cm_select_all

    arr = []
    lv.each_annotation_selected { |a| arr.push(a) }
    assert_equal(arr.size, 1)
    assert_equal(arr[0].to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal")

    arr[0].p1 = RBA::DPoint::new(11, 12)

    arr = []
    lv.each_annotation_selected { |a| arr.push(a) }
    assert_equal(arr.size, 1)
    assert_equal(arr[0].to_s_debug, "p1=11,12, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal")

    mw.close_all

  end

  # annotation template registration
  def test_5

    lv = RBA::LayoutView::new
    lv.set_config("ruler-templates-v2", "mode=normal,title=Ruler,category=_ruler,fmt=$D,fmt_x=$X,fmt_y=$Y,position=auto,xalign=auto,yalign=auto,xlabel_xalign=auto,xlabel_yalign=auto,ylabel_xalign=auto,ylabel_yalign=auto,style=ruler,outline=diag,snap=true,angle_constraint=global")
    tpl = lv.annotation_templates
    assert_equal(tpl.size, 1)

    lv.unregister_annotation_templates("")

    tpl = lv.annotation_templates
    assert_equal(tpl.size, 0)

    a = RBA::Annotation::new
    a.fmt = "U"
    a.category = "42"
    lv.register_annotation_template(a, "X", RBA::Annotation::RulerModeSingleClick)

    tpl = lv.annotation_templates
    assert_equal(tpl.size, 1)
    assert_equal(tpl[0].size, 3)
    (a, title, mode) = tpl[0]
    assert_equal(a.fmt, "U")
    assert_equal(a.category, "42")
    assert_equal(title, "X")
    assert_equal(mode, RBA::Annotation::RulerModeSingleClick)

    lv.unregister_annotation_templates("X")
    tpl = lv.annotation_templates
    assert_equal(tpl.size, 1)

    lv.unregister_annotation_templates("42")
    tpl = lv.annotation_templates
    assert_equal(tpl.size, 0)

  end

  # multi-segment rulers
  def test_6

    a = annot_obj2( [ RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ) ], "a", "b", "c", 1, 2, false, 4)
    assert_equal(a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=horizontal")
    assert_equal(a.to_s, "id=-1,x1=1,y1=2,x2=3,y2=4,category='',fmt=a,fmt_x=b,fmt_y=c,position=auto,xalign=auto,yalign=auto,xlabel_xalign=auto,xlabel_yalign=auto,ylabel_xalign=auto,ylabel_yalign=auto,style=arrow_end,outline=diag_xy,snap=false,angle_constraint=horizontal")
    aa = RBA::Annotation::from_s(a.to_s)
    assert_equal(aa.to_s, a. to_s)

    a = annot_obj2( [ RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 1.5, 2.5 ), RBA::DPoint::new( 3.0, 4.0 ) ], "a", "b", "c", 1, 2, false, 3)
    assert_equal(a.to_s_debug, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=arrow_end, outline=diag_xy, snap=false, ac=diagonal_only")
    assert_equal(a.to_s, "id=-1,pt=1:2,pt=1.5:2.5,pt=3:4,category='',fmt=a,fmt_x=b,fmt_y=c,position=auto,xalign=auto,yalign=auto,xlabel_xalign=auto,xlabel_yalign=auto,ylabel_xalign=auto,ylabel_yalign=auto,style=arrow_end,outline=diag_xy,snap=false,angle_constraint=diagonal_only")
    assert_equal(a.segments, 2)
    assert_equal(a.seg_p1(0).to_s, "1,2")
    assert_equal(a.seg_p2(0).to_s, "1.5,2.5")
    assert_equal(a.seg_p1(1).to_s, "1.5,2.5")
    assert_equal(a.seg_p2(1).to_s, "3,4")
    a.fmt_x = "$X"
    a.fmt_y = "$(Y*2)"
    a.fmt = "$D"
    assert_equal(a.text_x(0), "0.5")
    assert_equal(a.text_x(1), "1.5")
    assert_equal(a.text_y(0), "1")
    assert_equal(a.text_y(1), "3")
    assert_equal(a.text(0), "0.70710678")
    assert_equal(a.text(1), "2.1213203")
    aa = RBA::Annotation::from_s(a.to_s)
    assert_equal(aa.to_s, a. to_s)

    assert_equal(a.points.collect { |p| p.to_s }.join(";"), "1,2;1.5,2.5;3,4")

    a.p1 = RBA::DPoint::new(1.1, 2.1)
    assert_equal(a.points.collect { |p| p.to_s }.join(";"), "1.1,2.1;1.5,2.5;3,4")

    a.p2 = RBA::DPoint::new(3.1, 4.1)
    assert_equal(a.points.collect { |p| p.to_s }.join(";"), "1.1,2.1;1.5,2.5;3.1,4.1")

    a.points = [ RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 2.0, 3.0 ) ]
    assert_equal(a.points.collect { |p| p.to_s }.join(";"), "1,2;2,3")
    assert_equal(a.segments, 1)

    a.points = [ RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 1.0, 2.0 ) ]
    assert_equal(a.points.collect { |p| p.to_s }.join(";"), "1,2")
    assert_equal(a.segments, 1)

  end

end

load("test_epilogue.rb")
