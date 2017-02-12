
$:.push(File::dirname($0))

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

class RBA::Annotation
  def to_s
    "p1=" + p1.to_s + ", p2=" + p2.to_s + ", fmt=" + fmt + ", fmt_x=" + fmt_x + ", fmt_y=" + fmt_y +
      ", style=" + style.to_s + ", outline=" + outline.to_s + ", snap=" + snap?.to_s + ", ac=" + angle_constraint.to_s
  end
end

class Ant_TestClass < TestBase

  # Annotation object
  def test_1

    a = RBA::Annotation::new
    assert_equal( a.to_s, "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=0, outline=0, snap=true, ac=5" )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 3 )
    assert_equal( a.to_s, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=1, outline=2, snap=false, ac=3" )

    b = a.dup
    assert_equal( b.to_s, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=1, outline=2, snap=false, ac=3" )
    a = RBA::Annotation::new

    assert_equal( a == b, false )
    assert_equal( a != b, true )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 3 )
    assert_equal( a == b, true )
    assert_equal( a != b, false )

    c = a.transformed( RBA::DTrans::new( RBA::DPoint::new( 10.0, 20.0 ) ) )
    assert_equal( c.to_s, "p1=11,22, p2=13,24, fmt=a, fmt_x=b, fmt_y=c, style=1, outline=2, snap=false, ac=3" )

    c = a.transformed_cplx( RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DPoint::new( 11.0, 22.0 ) ) ) )
    assert_equal( c.to_s, "p1=12,24, p2=14,26, fmt=a, fmt_x=b, fmt_y=c, style=1, outline=2, snap=false, ac=3" )

    a.fmt = "X=$X,Y=$Y,D=$D,A=$A,P=$P,Q=$Q,U=$U,V=$V"
    a.fmt_x = "$(X*1000)+$(Y*1e3)"
    assert_equal( a.text == "X=2,Y=2,D=2.8284271,A=4e-06,P=3,Q=4,U=1,V=2" || a.text == "X=2,Y=2,D=2.8284271,A=4e-006,P=3,Q=4,U=1,V=2", true )
    assert_equal( a.text_x, "2000+2000" )
    assert_equal( a.text_y, "c" )

  end

  # Test LayoutView integration
  def test_2

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

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 3 )
    assert_equal( a.is_valid?, false )
    lv.insert_annotation( a )
    assert_equal( aac, 2 )
    assert_equal( ac, [] )
    assert_equal( asc, 0 )
    assert_equal( a.is_valid?, true )
    assert_equal( lv.annotation( -1 ).is_valid?, false )
    assert_equal( lv.annotation( a.id ).is_valid?, true )
    assert_equal( lv.annotation( a.id ).to_s, "p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=1, outline=2, snap=false, ac=3" )

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=0, outline=0, snap=true, ac=5;p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=1, outline=2, snap=false, ac=3" )

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
    lv.each_annotation { |a| arr.push( a.to_s ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=0, outline=0, snap=true, ac=5;p1=1,2, p2=3,4, fmt=u, fmt_x=b, fmt_y=c, style=1, outline=2, snap=false, ac=3" )

    lv.erase_annotation(id)

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=0, outline=0, snap=true, ac=5" )

    lv.clear_annotations

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s ) }
    assert_equal( arr.join( ";" ), "" )

    mw.close_all

  end

  # Test LayoutView integration with live updates
  def test_3

    mw = RBA::Application::instance.main_window
    mw.close_all
    mw.create_layout( 0 )
    lv = mw.current_view

    a0 = RBA::Annotation::new
    lv.insert_annotation( a0 )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 3 )
    lv.insert_annotation( a )

    arr = []
    lv.each_annotation { |x| arr.push( x.to_s ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=0, outline=0, snap=true, ac=5;p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=1, outline=2, snap=false, ac=3" )

    a.fmt = "u"

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=0, outline=0, snap=true, ac=5;p1=1,2, p2=3,4, fmt=u, fmt_x=b, fmt_y=c, style=1, outline=2, snap=false, ac=3" )

    a.delete

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=0, outline=0, snap=true, ac=5" )

    a0.delete

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s ) }
    assert_equal( arr.join( ";" ), "" )

    a0 = RBA::Annotation::new
    lv.insert_annotation( a0 )

    a = annot_obj( RBA::DPoint::new( 1.0, 2.0 ), RBA::DPoint::new( 3.0, 4.0 ), "a", "b", "c", 1, 2, false, 3 )
    lv.insert_annotation( a )

    arr = []
    lv.each_annotation { |x| arr.push( x.to_s ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=$D, fmt_x=$X, fmt_y=$Y, style=0, outline=0, snap=true, ac=5;p1=1,2, p2=3,4, fmt=a, fmt_x=b, fmt_y=c, style=1, outline=2, snap=false, ac=3" )

    lv.each_annotation { |x| x.fmt = "q" }

    arr = []
    lv.each_annotation { |x| arr.push( x.to_s ) }
    assert_equal( arr.join( ";" ), "p1=0,0, p2=0,0, fmt=q, fmt_x=$X, fmt_y=$Y, style=0, outline=0, snap=true, ac=5;p1=1,2, p2=3,4, fmt=q, fmt_x=b, fmt_y=c, style=1, outline=2, snap=false, ac=3" )

    lv.each_annotation { |x| x.delete }

    arr = []
    lv.each_annotation { |a| arr.push( a.to_s ) }
    assert_equal( arr.join( ";" ), "" )

    mw.close_all

  end

end

load("test_epilogue.rb")

