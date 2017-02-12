
$:.push(File::dirname($0))

load("test_prologue.rb")

class LAYMarkers_TestClass < TestBase

  def test_1

    app = RBA::Application.instance
    mw = app.main_window
    mw.create_layout(0)
    cv = mw.current_view

    m = RBA::Marker.new(cv)
    m.set(RBA::DBox.new(1, 2, 3, 4))
    m.set(RBA::DText.new("T", RBA::DTrans.new(4)))
    m.set(RBA::DPolygon.new([RBA::DPoint.new(1, 2), RBA::DPoint.new(3, 4), RBA::DPoint.new(5, 6)]))
    m.set(RBA::DPath.new([RBA::DPoint.new(1, 2), RBA::DPoint.new(3, 4), RBA::DPoint.new(5, 6)], 0.5))
    m.set_box(RBA::DBox.new(1, 2, 3, 4))
    m.set_text(RBA::DText.new("T", RBA::DTrans.new(4)))
    m.set_polygon(RBA::DPolygon.new([RBA::DPoint.new(1, 2), RBA::DPoint.new(3, 4), RBA::DPoint.new(5, 7)]))
    m.set_path(RBA::DPath.new([RBA::DPoint.new(1, 2), RBA::DPoint.new(3, 4), RBA::DPoint.new(5, 7)], 0.5))

    m.color = 0xff812345
    assert_equal(m.color, 0xff812345)
    assert_equal(m.has_color?, true)
    m.reset_color
    assert_equal(m.has_color?, false)
    m.color = 0xff4123c5

    m.frame_color = 0xffa23456
    assert_equal(m.frame_color, 0xffa23456)
    assert_equal(m.has_frame_color?, true)
    m.reset_frame_color
    assert_equal(m.has_frame_color?, false)
    m.frame_color = 0xffa23456

    m.line_width = 2
    assert_equal(m.line_width, 2)

    m.vertex_size = 3
    assert_equal(m.vertex_size, 3)

    m.halo = 1
    assert_equal(m.halo, 1)

    m.dither_pattern = 15
    assert_equal(m.dither_pattern, 15)

    # Keep the marker alive after GC.start:
    # $marker = m

  end

end

load("test_epilogue.rb")
