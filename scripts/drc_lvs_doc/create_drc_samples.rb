
# Run with:
# ./klayout -z -r ./create_drc_samples.rb -t -c klayoutrc_drc_samples

class QRCGenerator

  def res_path 
    "src/doc"
  end

  def img_path
    "doc/images"
  end

  def initialize
    @path = res_path + "/" + "docDRCLVSResources.qrc"
    @file = File.open(@path, "w")
    @file.puts("<RCC>")
    @file.puts(" <qresource prefix=\"/help/images\">")
  end

  def <<(str)
    @file.puts(str)
  end

  def finish
    @file.puts(" </qresource>")
    @file.puts("</RCC>")
    @file.close
    puts "---> resource file written to #{@path}"
  end

  def self.instance
    @@inst ||= QRCGenerator::new
    @@inst
  end

end

def run_demo(gen, cmd, out)

  res_path = QRCGenerator::instance.res_path
  img_path = QRCGenerator::instance.img_path

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

  view.zoom_box(RBA::DBox::new(-2.0, -2.0, 8.0, 9.0))
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
    labels1 = labels(1, 0)
    labels = labels1
    (#{cmd}).data
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
  elsif data.is_a?(RBA::Texts)
    cell.shapes(lout).insert(data)
  end

  view.update_content
  view.save_image(res_path + "/" + img_path + "/" + out, 500, 500)

  puts "---> written #{res_path}/#{img_path}/#{out}"

  mw.close_all

  QRCGenerator::instance << "  <file alias=\"#{out}\">#{img_path}/#{out}</file>"

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
      RBA::Point::new(0, 0),
      RBA::Point::new(2000, 0),
      RBA::Point::new(2000, 2000),
      RBA::Point::new(0, 2000)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(2000, 2000),
      RBA::Point::new(4000, 2000),
      RBA::Point::new(4000, 4000),
      RBA::Point::new(2000, 4000)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new( 500, 4000),
      RBA::Point::new(2500, 4000),
      RBA::Point::new(2500, 6000),
      RBA::Point::new( 500, 6000)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.width(1.0)", "drc_width5.png"
run_demo gen, "input.width(1.0, without_touching_corners)", "drc_width6.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(0, 0),
      RBA::Point::new(3000, 0),
      RBA::Point::new(3000, 4000),
      RBA::Point::new(5000, 4000),
      RBA::Point::new(5000, 7000),
      RBA::Point::new(2000, 7000),
      RBA::Point::new(2000, 2000),
      RBA::Point::new(0, 2000)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.drc(width < 2.0)", "drc_width1u.png"
run_demo gen, "input.drc(width(projection) < 2.0)", "drc_width2u.png"
run_demo gen, "input.drc(width(projection) > 2.0)", "drc_width3u.png"
run_demo gen, "input.drc(width(projection) == 2.0)", "drc_width4u.png"
run_demo gen, "input.drc(width(projection) != 2.0)", "drc_width5u.png"
run_demo gen, "input.drc(1.0 < width(projection) <= 3.0)", "drc_width6u.png"

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

run_demo gen, "input.drc(space(euclidian) < 1.2)", "drc_space1u.png"
run_demo gen, "input.drc(notch(euclidian) < 1.2)", "drc_space2u.png"
run_demo gen, "input.drc(isolated(euclidian) < 1.2)", "drc_space3u.png"

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
run_demo gen, "input1.drc(separation(input2, euclidian) < 1.2)", "drc_separation1u.png"
run_demo gen, "input1.drc((separation(input2) >= 1.2).first_edges)", "drc_separation1un.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(1000, 0),
      RBA::Point::new(1000, 6000),
      RBA::Point::new(2000, 6000),
      RBA::Point::new(2000, 0),
    ];
    s2.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(5000, 2000),
      RBA::Point::new(5000, 4000),
      RBA::Point::new(6000, 4000),
      RBA::Point::new(6000, 2000),
    ];
    s2.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(3000, 0),
      RBA::Point::new(3000, 6000),
      RBA::Point::new(4000, 6000),
      RBA::Point::new(4000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input1.sep(input2, 1.2, projection)", "drc_separation2.png"
run_demo gen, "input1.sep(input2, 1.2, projection, not_opposite)", "drc_separation3.png"
run_demo gen, "input1.sep(input2, 1.2, projection, only_opposite)", "drc_separation4.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(0, 3000),
      RBA::Point::new(0, 4000),
      RBA::Point::new(1000, 4000),
      RBA::Point::new(1000, 3000)
    ]
    s2.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(3000, 3000),
      RBA::Point::new(3000, 4000),
      RBA::Point::new(4000, 4000),
      RBA::Point::new(4000, 3000)
    ]
    s2.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(6000, 3000),
      RBA::Point::new(6000, 4000),
      RBA::Point::new(7000, 4000),
      RBA::Point::new(7000, 3000)
    ]
    s2.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(4500, 1500),
      RBA::Point::new(4500, 2500),
      RBA::Point::new(5500, 2500),
      RBA::Point::new(5500, 1500)
    ]
    s2.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(1500, 3000),
      RBA::Point::new(1500, 4000),
      RBA::Point::new(2500, 4000),
      RBA::Point::new(2500, 3000)
    ]
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(3000, 4500),
      RBA::Point::new(3000, 5500),
      RBA::Point::new(4000, 5500),
      RBA::Point::new(4000, 4500)
    ]
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(3000, 1500),
      RBA::Point::new(3000, 2500),
      RBA::Point::new(4000, 2500),
      RBA::Point::new(4000, 1500)
    ]
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(4500, 3000),
      RBA::Point::new(4500, 4000),
      RBA::Point::new(5500, 4000),
      RBA::Point::new(5500, 3000)
    ]
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input1.sep(input2, 1.0, projection)", "drc_separation5.png"
run_demo gen, "input1.sep(input2, 1.0, projection,\n" +
              "           one_side_allowed)", "drc_separation6.png"
run_demo gen, "input1.sep(input2, 1.0, projection,\n" + 
              "           two_sides_allowed)", "drc_separation7.png"
run_demo gen, "input1.sep(input2, 1.0, projection,\n" +
              "           two_opposite_sides_allowed)", "drc_separation8.png"
run_demo gen, "input1.sep(input2, 1.0, projection,\n" + 
              "           two_connected_sides_allowed)", "drc_separation9.png"
run_demo gen, "input1.sep(input2, 1.0, projection,\n" +
              "           three_sides_allowed)", "drc_separation10.png"
run_demo gen, "input1.sep(input2, 1.0, projection,\n" +
              "           one_side_allowed,\n" + 
              "           two_opposite_sides_allowed)", "drc_separation11.png"

class Gen
  def produce(s1, s2)
    pts = [ 
      RBA::Point::new(0, 0),
      RBA::Point::new(2000, 0),
      RBA::Point::new(2000, 1500),
      RBA::Point::new(0, 1500)
    ];
    s2.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(2000, 1500),
      RBA::Point::new(3500, 1500),
      RBA::Point::new(3500, 3500),
      RBA::Point::new(2000, 3500)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(1000, 3500),
      RBA::Point::new(3000, 3500),
      RBA::Point::new(3000, 5000),
      RBA::Point::new(1000, 5000)
    ];
    s2.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(1000, 5500),
      RBA::Point::new(3000, 5500),
      RBA::Point::new(3000, 7000),
      RBA::Point::new(1000, 7000)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input1.sep(input2, 1.0)", "drc_separation12.png"
run_demo gen, "input1.sep(input2, 1.0, without_touching_corners)", "drc_separation13.png"
run_demo gen, "input1.sep(input2, 1.0, without_touching_edges)", "drc_separation14.png"

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

run_demo gen, "input1.drc(enclosing(input2) < 2.0.um)", "drc_enc1u.png"
run_demo gen, "input1.drc(enclosing(input2,\n"+
              "                     projection) < 2.0.um)", "drc_enc2u.png"

class Gen
  def produce(s1, s2)
    s1.insert(RBA::Box::new(3000, 0, 6000, 6000))
    s2.insert(RBA::Box::new(0, 1000, 6000, 7000))
  end
end

gen = Gen::new

run_demo gen, "input1.enclosed(input2, 2.0.um)", "drc_encd1.png"
run_demo gen, "input1.enclosed(input2, 2.0.um, projection)", "drc_encd2.png"

run_demo gen, "input1.drc(enclosed(input2) < 2.0.um)", "drc_encd1u.png"
run_demo gen, "input1.drc(enclosed(input2,\n"+
              "                    projection) < 2.0.um)", "drc_encd2u.png"


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

run_demo gen, "input1.drc(overlap(input2) < 2.0.um)", "drc_overlap1u.png"
run_demo gen, "input1.drc(overlap(input2,\n"+
              "                   projection) < 2.0.um)", "drc_overlap2u.png"


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
      RBA::Point::new(1000, 0),
      RBA::Point::new(1000, 5000),
      RBA::Point::new(2000, 5000),
      RBA::Point::new(2000, 7000),
      RBA::Point::new(4000, 7000),
      RBA::Point::new(4000, 5000),
      RBA::Point::new(5000, 5000),
      RBA::Point::new(5000, 0),
      RBA::Point::new(4000, 0),
      RBA::Point::new(4000, 1000),
      RBA::Point::new(2000, 1000),
      RBA::Point::new(2000, 0)
    ];
    s1.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input.edges", "drc_edge_modes1.png"
run_demo gen, "input.edges(convex)", "drc_edge_modes2.png"
run_demo gen, "input.edges(concave)", "drc_edge_modes3.png"
run_demo gen, "input.edges(step)", "drc_edge_modes4.png"
run_demo gen, "input.edges(step_in)", "drc_edge_modes5.png"
run_demo gen, "input.edges(step_out)", "drc_edge_modes6.png"

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
    pts = [ 
      RBA::Point::new(0   , 0   ),
      RBA::Point::new(0   , 3000),
      RBA::Point::new(2000, 3000),
      RBA::Point::new(2000, 5000),
      RBA::Point::new(6000, 5000),
      RBA::Point::new(6000, 0   )
    ];
    s1.insert(RBA::Polygon::new(pts))
    s2.insert(RBA::Box::new(0, 0, 4000, 6000))
  end
end

gen = Gen::new

run_demo gen, "input1.edges.inside(input2)", "drc_inside_ep.png"
run_demo gen, "input1.edges.inside(input2.edges)", "drc_inside_ee.png"
run_demo gen, "input1.edges.not_inside(input2)", "drc_not_inside_ep.png"
run_demo gen, "input1.edges.not_inside(input2.edges)", "drc_not_inside_ee.png"
run_demo gen, "input1.edges.outside(input2)", "drc_outside_ep.png"
run_demo gen, "input1.edges.outside(input2.edges)", "drc_outside_ee.png"
run_demo gen, "input1.edges.not_outside(input2)", "drc_not_outside_ep.png"
run_demo gen, "input1.edges.not_outside(input2.edges)", "drc_not_outside_ee.png"

class Gen
  def produce(s1, s2)
    s1.insert(RBA::Box::new(-1000, 0, 1000, 2000))
    s1.insert(RBA::Box::new(2000, 0, 4000, 2000))
    s1.insert(RBA::Box::new(5000, 0, 7000, 2000))
    s1.insert(RBA::Box::new(-1000, 3000, 1000, 5000))
    s1.insert(RBA::Box::new(2000, 3000, 4000, 5000))
    s1.insert(RBA::Box::new(5000, 3000, 7000, 5000))
    s2.insert(RBA::Box::new(0, 500, 1000, 1500))
    s2.insert(RBA::Box::new(2500, 500, 3500, 1500))
    s2.insert(RBA::Box::new(4000, 500, 5000, 1500))
    s2.insert(RBA::Box::new(6250, 500, 7250, 1500))
    s2.insert(RBA::Box::new(-500, 5000, 500, 6000))
    s2.insert(RBA::Box::new(2500, 3500, 3500, 4500))
    s2.insert(RBA::Box::new(6250, 4750, 7250, 5750))
  end
end

gen = Gen::new

run_demo gen, "input1.covering(input2)", "drc_covering.png"
run_demo gen, "input1.not_covering(input2)", "drc_not_covering.png"

class Gen
  def produce(s1, s2)
    s1.insert(RBA::Box::new(0, 5000, 2000, 7000))
    s1.insert(RBA::Box::new(4000, 5000, 6000, 7000))
    s1.insert(RBA::Box::new(0, 0, 2000, 2000))
    s1.insert(RBA::Box::new(4000, 0, 6000, 2000))
    s2.insert(RBA::Box::new(2000, 6000, 4000, 7000))
    s2.insert(RBA::Box::new(4500, 5500, 5500, 6500))
    s2.insert(RBA::Box::new(5000, 4000, 6000, 5000))
    s2.insert(RBA::Polygon::new(
      [ RBA::Point::new(2000, 0),    RBA::Point::new(2000, 3000), 
        RBA::Point::new(1000, 3000), RBA::Point::new(1000, 2000), 
        RBA::Point::new(0, 2000),    RBA::Point::new(0, 5000), 
        RBA::Point::new(1000, 5000),    RBA::Point::new(1000, 4000), 
        RBA::Point::new(3000, 4000), RBA::Point::new(3000, 0) ]
    ))
  end
end

gen = Gen::new

run_demo gen, "input1.interacting(input2, 1)", "drc_interacting2.png"
run_demo gen, "input1.interacting(input2, 1, 1)", "drc_interacting3.png"
run_demo gen, "input1.interacting(input2, 2..)", "drc_interacting4.png"
run_demo gen, "input1.interacting(input2, 1..2)", "drc_interacting5.png"

run_demo gen, "input1.not_interacting(input2, 1)", "drc_not_interacting2.png"
run_demo gen, "input1.not_interacting(input2, 1, 1)", "drc_not_interacting3.png"
run_demo gen, "input1.not_interacting(input2, 2..)", "drc_not_interacting4.png"
run_demo gen, "input1.not_interacting(input2, 1..2)", "drc_not_interacting5.png"

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
      RBA::Point::new(1000, 1000),
      RBA::Point::new(1000, 2000),
      RBA::Point::new(2000, 2000),
      RBA::Point::new(2000, 1000)
    ];
    s1.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(1000, 1000),
      RBA::Point::new(1000, 7000),
      RBA::Point::new(6000, 7000),
      RBA::Point::new(6000, 1000),
      RBA::Point::new(5000, 1000),
      RBA::Point::new(5000, 6000),
      RBA::Point::new(2000, 6000),
      RBA::Point::new(2000, 1000)
    ];
    s2.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "input1.sized(1.um, steps(1), size_inside(input2))", "drc_sized_inside1.png"
run_demo gen, "input1.sized(2.um, steps(2), size_inside(input2))", "drc_sized_inside2.png"
run_demo gen, "input1.sized(3.um, steps(3), size_inside(input2))", "drc_sized_inside3.png"
run_demo gen, "input1.sized(10.um, steps(10), size_inside(input2))", "drc_sized_inside4.png"

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


class Gen
  def produce(s1, s2)
    s1.insert(RBA::Text::new("ABC", RBA::Trans::new(RBA::Vector::new(0, 2000))))
    s1.insert(RBA::Text::new("A", RBA::Trans::new(RBA::Vector::new(0, 6000))))
    s1.insert(RBA::Text::new("XYZ", RBA::Trans::new(RBA::Vector::new(4000, 2000))))
    s1.insert(RBA::Text::new("A*", RBA::Trans::new(RBA::Vector::new(4000, 6000))))
  end
end

gen = Gen::new

run_demo gen, "labels.texts(\"A*\")", "drc_texts1.png"
run_demo gen, "labels.texts(text(\"A*\"))", "drc_texts2.png"

class Gen
  def produce(s1, s2)
    s1.insert(RBA::Text::new("T1", RBA::Trans::new(RBA::Vector::new(0, 2000))))
    s1.insert(RBA::Text::new("T2", RBA::Trans::new(RBA::Vector::new(2000, 2000))))
    s1.insert(RBA::Text::new("T3", RBA::Trans::new(RBA::Vector::new(4000, 2000))))
    pts = [ 
      RBA::Point::new(2000, 0),
      RBA::Point::new(2000, 4000),
      RBA::Point::new(6000, 4000),
      RBA::Point::new(6000, 0)
    ];
    s2.insert(RBA::Polygon::new(pts))
  end
end

gen = Gen::new

run_demo gen, "labels & input2", "drc_textpoly1.png"
run_demo gen, "labels - input2", "drc_textpoly2.png"


class Gen
  def produce(s1, s2)
    s1.insert(RBA::Polygon::new(RBA::Box::new(0, 0, 6000, 2000)))
    s2.insert(RBA::Polygon::new(RBA::Box::new(3500, 3000, 6000, 4000)))
    s2.insert(RBA::Polygon::new(RBA::Box::new(0, 5000, 2500, 7000)))
    s2.insert(RBA::Polygon::new(RBA::Box::new(3500, 5000, 6000, 7000)))
  end
end

gen = Gen::new

run_demo gen, "input1.separation(input2, 3.5.um, projection, \n                  shielded)", "drc_shielded1.png"
run_demo gen, "input1.separation(input2, 3.5.um, projection, \n                  transparent)", "drc_shielded2.png"


class Gen
  def produce(s1, s2)
    s1.insert(RBA::Polygon::new(RBA::Box::new(0, 0, 2500, 2000)))
    s1.insert(RBA::Polygon::new(RBA::Box::new(3500, 0, 6000, 2000)))
    s2.insert(RBA::Polygon::new(RBA::Box::new(3500, 0, 6000, 2000)))
    s2.insert(RBA::Polygon::new(RBA::Box::new(0, 5000, 2500, 7000)))
    s2.insert(RBA::Polygon::new(RBA::Box::new(3500, 5000, 6000, 7000)))
  end
end

gen = Gen::new

run_demo gen, "input1.separation(input2, 3.5.um, projection, \n                  shielded)", "drc_shielded3.png"
run_demo gen, "input1.separation(input2, 3.5.um, projection, \n                  transparent)", "drc_shielded4.png"


QRCGenerator::instance.finish

