

app = RBA::Application.instance
mw = app.main_window

#  create a new layout 
mw.create_layout( 0 )
view = mw.current_view

#  create a new layer in that layout
layout = view.cellview( 0 ).layout 

layer_ids = []

256.times { |l|

  linfo = RBA::LayerInfo.new 
  layer_id = layout.insert_layer( linfo )

  #  create a layer view for that layer
  ln = RBA::LayerPropertiesNode::new
  ln.dither_pattern = 0
  if l >= 128 
    #  red colors for the positive values
    c = (l - 128) * 2 * 0x10000
  elsif l > 0
    #  blue colors for the negative values
    c = (128 - l) * 2
  else 
    c = 0xff
  end
  ln.fill_color = c
  ln.frame_color = c
  ln.width = 1
  ln.source_layer_index = layer_id
  view.insert_layer( view.end_layers, ln )

  layer_ids.push( layer_id )

}

#  replicate last layer to allow values of 256 (mapped to 255) ..
layer_ids.push( layer_ids[255] )

#  create a top cell and start the recursion on this
topcell_id = layout.add_cell( "top" )
topcell = layout.cell( topcell_id )

#  create an image

nx = 500
ny = 500
radius = 100
x = -nx / 2
nx.times { 
  y = -ny / 2
  ny.times { 
    r = Math::sqrt( x * x + y * y ) * Math::PI * 2.0 / radius
    if r.abs < 1e-6
      v = 1.0
    else
      v = Math::sin( r ) / r
    end
    vi = ((v + 1.0) * 128.0 + 0.5).to_i
    box = RBA::Box::new_lbrt( x * 100, y * 100, (x + 1) * 100, (y + 1) * 100 )
    topcell.shapes( layer_ids[vi] ).insert_box( box )
    y += 1
  }
  x += 1
}

#  select his cell as the top cell, fit all and switch on all hierarchy levels
view.select_cell_path( [topcell_id], 0 )
view.update_content
view.zoom_fit
view.max_hier

#  run the application
RBA::Application.instance.exec


