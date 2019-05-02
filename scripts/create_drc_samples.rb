
# Run with:
# ./klayout -z -r ./create_drc_samples.rb -t -c klayoutrc_drc_samples

def run_demo(gen, cmd, out)

  img_path = "src/lay/lay/doc/images"

  mw = RBA::Application::instance::main_window

  cv = mw.create_layout(0)
  ly = cv.layout
  view = mw.current_view

  l0 = ly.insert_layer(RBA::LayerInfo::new(0, 0))
  l1 = ly.insert_layer(RBA::LayerInfo::new(1, 0))
  l2 = ly.insert_layer(RBA::LayerInfo::new(2, 0))
  lout_poly = ly.insert_layer(RBA::LayerInfo::new(101, 0))
  lout = ly.insert_layer(RBA::LayerInfo::new(100, 0))
  cell = ly.create_cell("TOP")
  view.select_cell(0, cell.cell_index)

  lp = RBA::LayerProperties::new
  lp.fill_color = lp.frame_color = 0xffffff
  lp.source_layer = 0
  lp.source_datatype = 0
  lp.dither_pattern = 5
  lp.width = 0
  lp.name = "TXT"
  view.insert_layer(view.end_layers, lp)

  lp = RBA::LayerProperties::new
  lp.fill_color = lp.frame_color = 0xff8080
  lp.source_layer = 1
  lp.source_datatype = 0
  lp.dither_pattern = 5
  lp.name = "IN1"
  view.insert_layer(view.end_layers, lp)

  lp = RBA::LayerProperties::new
  lp.fill_color = lp.frame_color = 0x8080ff
  lp.source_layer = 2
  lp.source_datatype = 0
  lp.dither_pattern = 9
  lp.name = "IN2"
  view.insert_layer(view.end_layers, lp)

  lp = RBA::LayerProperties::new
  lp.fill_color = lp.frame_color = 0x404040
  lp.source_layer = 101
  lp.source_datatype = 0
  lp.width = 0
  lp.dither_pattern = 0
  lp.transparent = true
  lp.name = "OUT_POLY"
  view.insert_layer(view.end_layers, lp)

  lp = RBA::LayerProperties::new
  lp.fill_color = lp.frame_color = 0xffffff
  lp.source_layer = 100
  lp.source_datatype = 0
  lp.dither_pattern = 1
  lp.width = 3
  lp.name = "OUT"
  view.insert_layer(view.end_layers, lp)

  gen.produce(cell.shapes(l1), cell.shapes(l2))

  view.zoom_box(RBA::DBox::new(-2.0, -1.0, 8.0, 9.0))
  view.max_hier

  t = RBA::Text::new(cmd, -1500, 8500)
  t.valign = 0
  t.font = 0
  t.size = 0.2 / ly.dbu
  cell.shapes(l0).insert(t)

  eng = DRC::DRCEngine::new
  data = eng.instance_eval(<<SCRIPT)
    silent
    input1 = input(1, 0)
    input = input1
    input2 = input(2, 0)
    #{cmd}.data
SCRIPT

  if data.is_a?(RBA::Region)
    cell.shapes(lout_poly).insert(data)
    cell.shapes(lout).insert(data)
  elsif data.is_a?(RBA::Edges)
    data.each do |e|
      v = e.p2 - e.p1
      if v.length > 0
        a1 = RBA::ICplxTrans::new(300.0 / v.length, 30, false, 0, 0).trans(v)
        a2 = RBA::ICplxTrans::new(300.0 / v.length, -30, false, 0, 0).trans(v)
        cell.shapes(lout).insert(RBA::Edge::new(e.p2 - a1, e.p2))
        cell.shapes(lout).insert(RBA::Edge::new(e.p2 - a2, e.p2))
      end
      cell.shapes(lout).insert(e)
    end
  elsif data.is_a?(RBA::EdgePairs)
    cell.shapes(lout_poly).insert_as_polygons(data, 1)
    cell.shapes(lout).insert(data.edges)
  end

  view.update_content
  view.save_image(img_path + "/" + out, 400, 400)

  puts "---> written #{img_path}/#{out}"

  mw.close_all

end

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(0, 0),
      RBA::Point::new(3000, 0),
      RBA::Point::new(3000, 4000),
      RBA::Point::new(5000, 4000),
      RBA::Point::new(5000, 7000),
      RBA::Point::new(2000, 7000),
      RBA::Point::new(2000, 3000),
      RBA::Point::new(0, 3000)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.width(1.2, euclidian)", "drc_width1.png"
run_demo gen, "input.width(1.2, projection)", "drc_width2.png"
run_demo gen, "input.width(1.2, square)", "drc_width3.png"
run_demo gen, "input.width(1.2, whole_edges)", "drc_width4.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(4000, 0),
      RBA::Point::new(4000, 1000),
      RBA::Point::new(6000, 1000),
      RBA::Point::new(6000, 2000),
      RBA::Point::new(4000, 2000),
      RBA::Point::new(4000, 6000),
      RBA::Point::new(7000, 6000),
      RBA::Point::new(7000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(0, 4000),
      RBA::Point::new(0, 7000),
      RBA::Point::new(3000, 7000),
      RBA::Point::new(3000, 4000)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.space(1.2, euclidian)", "drc_space1.png"
run_demo gen, "input.notch(1.2, euclidian)", "drc_space2.png"
run_demo gen, "input.isolated(1.2, euclidian)", "drc_space3.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(4000, 0),
      RBA::Point::new(4000, 1000),
      RBA::Point::new(6000, 1000),
      RBA::Point::new(6000, 2000),
      RBA::Point::new(4000, 2000),
      RBA::Point::new(4000, 6000),
      RBA::Point::new(7000, 6000),
      RBA::Point::new(7000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(0, 0),
      RBA::Point::new(0, 3000),
      RBA::Point::new(3000, 3000),
      RBA::Point::new(3000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(0, 4000),
      RBA::Point::new(0, 7000),
      RBA::Point::new(3000, 7000),
      RBA::Point::new(3000, 4000)
    ];
    s2.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input1.separation(input2, 1.2, euclidian)", "drc_separation1.png"

# ...

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(5000, 0),
      RBA::Point::new(5000, 2000),
      RBA::Point::new(4000, 2000),
      RBA::Point::new(4000, 5000),
      RBA::Point::new(3000, 5000),
      RBA::Point::new(3000, 7000),
      RBA::Point::new(6000, 7000),
      RBA::Point::new(6000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(0, 0),
      RBA::Point::new(0, 7000),
      RBA::Point::new(2000, 7000),
      RBA::Point::new(2000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.sized(1.8, 0.0).raw.merged(2)", "drc_raw1.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(0, 0),
      RBA::Point::new(0, 4000),
      RBA::Point::new(3000, 7000),
      RBA::Point::new(3000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(3000, 0),
      RBA::Point::new(3000, 7000),
      RBA::Point::new(6000, 4000),
      RBA::Point::new(6000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.raw.sized(-0.5)", "drc_raw2.png"
run_demo gen, "input.sized(-0.5)", "drc_raw3.png"



class Gen
  def produce(s1, s2)
    s1.insert(RBA::Box::new(0, 1000, 6000, 7000))
    s2.insert(RBA::Box::new(3000, 0, 6000, 6000))
  end
end

gen = Gen::new

run_demo gen, "input1.enclosing(input2, 2.0.um)", "drc_enc1.png"
run_demo gen, "input1.enclosing(input2, 2.0.um, projection)", "drc_enc2.png"


class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(0, 1000),
      RBA::Point::new(0, 7000),
      RBA::Point::new(7000, 7000),
      RBA::Point::new(7000, 3000),
      RBA::Point::new(4000, 3000),
      RBA::Point::new(4000, 5000),
      RBA::Point::new(2000, 5000),
      RBA::Point::new(2000, 2000),
      RBA::Point::new(1000, 2000),
      RBA::Point::new(1000, 1000)
    ];
    s2.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(2000, 0),
      RBA::Point::new(2000, 6000),
      RBA::Point::new(6000, 6000),
      RBA::Point::new(6000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input1.overlap(input2, 2.0.um)", "drc_overlap1.png"
run_demo gen, "input1.overlap(input2, 2.0.um, projection)", "drc_overlap2.png"


class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(1000, 0),
      RBA::Point::new(1000, 2000),
      RBA::Point::new(4000, 2000),
      RBA::Point::new(4000, 5000),
      RBA::Point::new(1000, 5000),
      RBA::Point::new(1000, 7000),
      RBA::Point::new(5000, 7000),
      RBA::Point::new(5000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.edges.start_segments(1.um)", "drc_start_segments1.png"
run_demo gen, "input.edges.start_segments(0, 0.5)", "drc_start_segments2.png"

run_demo gen, "input.edges.end_segments(1.um)", "drc_end_segments1.png"
run_demo gen, "input.edges.end_segments(0, 0.5)", "drc_end_segments2.png"

run_demo gen, "input.edges.centers(1.um)", "drc_centers1.png"
run_demo gen, "input.edges.centers(0, 0.5)", "drc_centers2.png"

run_demo gen, "input.edges.with_length(2.0)\n   .extended(:out => 1.0)", "drc_extended1.png"
run_demo gen, "input.edges.with_length(0, 3.5)\n   .extended(:out => 1.0)", "drc_extended2.png"
run_demo gen, "input.edges.with_length(0, 3.5)\n   .extended(:out => 1.0, :joined => true)", "drc_extended3.png"
run_demo gen, "input.edges.with_length(2.0)\n   .extended(0.0, -0.5, 1.0, -0.5)", "drc_extended4.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(2000, 0),
      RBA::Point::new(2000, 2000),
      RBA::Point::new(4000, 2000),
      RBA::Point::new(4000, 6000),
      RBA::Point::new(5000, 6000),
      RBA::Point::new(5000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(0, 3000),
      RBA::Point::new(3000, 3000),
      RBA::Point::new(3000, 4000),
      RBA::Point::new(1000, 4000),
      RBA::Point::new(1000, 7000),
      RBA::Point::new(0000, 7000)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.extents", "drc_extents1.png"
run_demo gen, "input.edges.extents(0.1)", "drc_extents2.png"


class Gen
  def produce(s1, s2)
    s1.insert(RBA::Box::new(0, 1000, 2000, 3000))
    s1.insert(RBA::Box::new(4000, 1000, 6000, 3000))
    s1.insert(RBA::Box::new(4000, 5000, 6000, 7000))
    s1.insert(RBA::Box::new(0, 4000, 2000, 6000))
    s2.insert(RBA::Box::new(1000, 0, 6000, 4000))
  end
end

gen = Gen::new

run_demo gen, "input1.inside(input2)", "drc_inside.png"
run_demo gen, "input1.not_inside(input2)", "drc_not_inside.png"
run_demo gen, "input1.outside(input2)", "drc_outside.png"
run_demo gen, "input1.not_outside(input2)", "drc_not_outside.png"
run_demo gen, "input1.interacting(input2)", "drc_interacting.png"
run_demo gen, "input1.not_interacting(input2)", "drc_not_interacting.png"
run_demo gen, "input1.overlapping(input2)", "drc_overlapping.png"
run_demo gen, "input1.not_overlapping(input2)", "drc_not_overlapping.png"
run_demo gen, "input1.and(input2)", "drc_and1.png"
run_demo gen, "input1.or(input2)", "drc_or1.png"
run_demo gen, "input1.xor(input2)", "drc_xor1.png"
run_demo gen, "input1.not(input2)", "drc_not1.png"
run_demo gen, "input1.join(input2)", "drc_join1.png"
run_demo gen, "input1.edges.and(input2.edges)", "drc_and2.png"
run_demo gen, "input1.edges.or(input2.edges)", "drc_or2.png"
run_demo gen, "input1.edges.xor(input2.edges)", "drc_xor2.png"
run_demo gen, "input1.edges.not(input2.edges)", "drc_not2.png"
run_demo gen, "input1.edges.join(input2.edges)", "drc_join2.png"
run_demo gen, "input1.edges.and(input2)", "drc_and3.png"
run_demo gen, "input1.edges.not(input2)", "drc_not3.png"
run_demo gen, "input1.edges.inside_part(input2)", "drc_inside_part.png"
run_demo gen, "input1.edges.outside_part(input2)", "drc_outside_part.png"

class Gen
  def produce(s1, s2)
    s1.insert(RBA::Box::new(0, 1000, 2000, 3000))
    s1.insert(RBA::Box::new(4000, 1000, 6000, 3000))
    s1.insert(RBA::Box::new(4000, 5000, 6000, 7000))
    s1.insert(RBA::Box::new(0, 4000, 2000, 6000))
    s2.insert(RBA::Box::new(4000, 1000, 6000, 3000))
    s2.insert(RBA::Box::new(1000, 0, 3000, 2000))
  end
end

gen = Gen::new

run_demo gen, "input1.in(input2)", "drc_in.png"
run_demo gen, "input1.not_in(input2)", "drc_not_in.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(0, 0),
      RBA::Point::new(0, 4000),
      RBA::Point::new(2000, 4000),
      RBA::Point::new(2000, 7000),
      RBA::Point::new(6000, 7000),
      RBA::Point::new(6000, 0)
    ];
    h1 = [ 
      RBA::Point::new(1000, 1000),
      RBA::Point::new(1000, 3000),
      RBA::Point::new(3000, 3000),
      RBA::Point::new(3000, 1000)
    ];
    h2 = [ 
      RBA::Point::new(3000, 4000),
      RBA::Point::new(3000, 6000),
      RBA::Point::new(5000, 6000),
      RBA::Point::new(5000, 4000)
    ];
    poly = RBA::Polygon::new(pts)
    poly.insert_hole(h1)
    poly.insert_hole(h2)
    s1.insert(poly)
  end
end

gen = Gen::new

run_demo gen, "input.hulls", "drc_hulls.png"
run_demo gen, "input.holes", "drc_holes.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(1000, 1000),
      RBA::Point::new(4000, 1000),
      RBA::Point::new(4000, 3000),
      RBA::Point::new(1000, 3000)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(0, 0),
      RBA::Point::new(0, 5000),
      RBA::Point::new(3000, 5000),
      RBA::Point::new(3000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(2000, 2000),
      RBA::Point::new(6000, 2000),
      RBA::Point::new(6000, 7000),
      RBA::Point::new(2000, 7000)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.merged", "drc_merged1.png"
run_demo gen, "input.merged(1)", "drc_merged2.png"
run_demo gen, "input.merged(2)", "drc_merged3.png"
run_demo gen, "input.merged(3)", "drc_merged4.png"


class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(3000, 0),
      RBA::Point::new(3000, 2000),
      RBA::Point::new(5000, 2000),
      RBA::Point::new(5000, 6000),
      RBA::Point::new(6000, 6000),
      RBA::Point::new(6000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(1000, 3000),
      RBA::Point::new(4000, 3000),
      RBA::Point::new(4000, 4000),
      RBA::Point::new(2000, 4000),
      RBA::Point::new(2000, 7000),
      RBA::Point::new(1000, 7000)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.moved(-1.5.um, 0.5.um)", "drc_moved1.png"
run_demo gen, "input.rotated(15)", "drc_rotated1.png"
run_demo gen, "input.scaled(0.75)", "drc_scaled1.png"
run_demo gen, "t = RBA::DCplxTrans::new(0.75, 180.0, true, 7.0, 1.0)\ninput.transformed(t)", "drc_transformed1.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(2000, 1000),
      RBA::Point::new(2000, 1000),
      RBA::Point::new(4000, 5000),
      RBA::Point::new(4000, 1000)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.sized(1.um)", "drc_sized1.png"
run_demo gen, "input.sized(1.5.um, 0.5.um)", "drc_sized2.png"
run_demo gen, "input.sized(1.um, diamond_limit)", "drc_sized3.png"
run_demo gen, "input.sized(1.um, octagon_limit)", "drc_sized4.png"
run_demo gen, "input.sized(1.um, square_limit)", "drc_sized5.png"
run_demo gen, "input.sized(1.um, acute_limit)", "drc_sized6.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(0, 0),
      RBA::Point::new(0, 1000),
      RBA::Point::new(4000, 5000),
      RBA::Point::new(4000, 7000),
      RBA::Point::new(6000, 5000),
      RBA::Point::new(6000, 0),
      RBA::Point::new(5000, 0),
      RBA::Point::new(5000, 4000),
      RBA::Point::new(1000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.edges.with_angle(45.degree)", "drc_with_angle1.png"
run_demo gen, "h = input.edges.with_angle(0)\nv = input.edges.with_angle(90)\n(h + v)", "drc_with_angle2.png"
run_demo gen, "input.with_angle(45.degree)", "drc_with_angle3.png"
run_demo gen, "input.without_angle(45.1.degree, 315.degree)", "drc_with_angle4.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(0, 0),
      RBA::Point::new(0, 7000),
      RBA::Point::new(1000, 7000),
      RBA::Point::new(1000, 1000),
      RBA::Point::new(5000, 1000),
      RBA::Point::new(5000, 7000),
      RBA::Point::new(6000, 7000),
      RBA::Point::new(6000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.rounded_corners(1.um, 2.um, 16)", "drc_rounded_corners.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(1000, 0),
      RBA::Point::new(1000, 2000),
      RBA::Point::new(4000, 2000),
      RBA::Point::new(4000, 5000),
      RBA::Point::new(1000, 5000),
      RBA::Point::new(1000, 7000),
      RBA::Point::new(5000, 7000),
      RBA::Point::new(5000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.middle.sized(0.1)", "drc_middle1.png"

run_demo gen, "input.extent_refs(:center).sized(0.1)", "drc_extent_refs1.png"

run_demo gen, "input.extent_refs(:bottom, as_edges)", "drc_extent_refs10.png"
run_demo gen, "input.extent_refs(:top, as_edges)", "drc_extent_refs11.png"
run_demo gen, "input.extent_refs(:left, as_edges)", "drc_extent_refs12.png"
run_demo gen, "input.extent_refs(:right, as_edges)", "drc_extent_refs13.png"

run_demo gen, "input.extent_refs(:bottom_left).sized(0.1)", "drc_extent_refs20.png"
run_demo gen, "input.extent_refs(:bottom_center).sized(0.1)", "drc_extent_refs21.png"
run_demo gen, "input.extent_refs(:bottom_right).sized(0.1)", "drc_extent_refs22.png"
run_demo gen, "input.extent_refs(:right_center).sized(0.1)", "drc_extent_refs23.png"
run_demo gen, "input.extent_refs(:left_center).sized(0.1)", "drc_extent_refs24.png"
run_demo gen, "input.extent_refs(:top_left).sized(0.1)", "drc_extent_refs25.png"
run_demo gen, "input.extent_refs(:top_center).sized(0.1)", "drc_extent_refs26.png"
run_demo gen, "input.extent_refs(:top_right).sized(0.1)", "drc_extent_refs27.png"

run_demo gen, "input.extent_refs(0.25, 0.75).sized(0.1)", "drc_extent_refs30.png"
run_demo gen, "input.extent_refs(0.25, 0.75, 0.5, 1.0)", "drc_extent_refs31.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(0, 0),
      RBA::Point::new(0, 8000),
      RBA::Point::new(4000, 4000),
      RBA::Point::new(4000, 2000),
      RBA::Point::new(6000, 2000),
      RBA::Point::new(6000, 0),
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.corners.sized(0.1)", "drc_corners1.png"
run_demo gen, "input.corners(90.0).sized(0.1)", "drc_corners2.png"
run_demo gen, "input.corners(-90.0 .. -45.0).sized(0.1)", "drc_corners3.png"

