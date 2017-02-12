

#  create one level of recursion
#  unless the level is deeper than the maximum level, create a new cell and instantiate this in 
#  the given upper cell four times rendering the Sierpinski curve.

def create_recursion( layout, cell_id, layer_id, rec_level )

  if rec_level > 20

    #  max. recursion depth reached: just paint a straight line 
    edge = RBA::Edge.new_pp( RBA::Point::new_xy( 0, 0 ), RBA::Point::new_xy( 3000, 0 ) )
    layout.cell( cell_id ).shapes( layer_id ).insert_edge( edge ) 

  else
   
    #  create a new cell 
    new_cell = layout.add_cell( "level" + rec_level.to_s )
    cell = layout.cell( cell_id )

    #  and add four instances of it
    insts = [
      RBA::CellInstArray::new_inst_cplx( new_cell, RBA::CplxTrans::new_mrmu( 1.0 / 3.0, 0.0, false, RBA::DPoint::new_xy( 0.0, 0.0 ) ) ),
      RBA::CellInstArray::new_inst_cplx( new_cell, RBA::CplxTrans::new_mrmu( 1.0 / 3.0, 60.0, false, RBA::DPoint::new_xy( 1000.0, 0.0 ) ) ),
      RBA::CellInstArray::new_inst_cplx( new_cell, RBA::CplxTrans::new_mrmu( 1.0 / 3.0, -60.0, false, RBA::DPoint::new_xy( 1500.0, 1000 * Math::sin( Math::PI / 3.0 ) ) ) ),
      RBA::CellInstArray::new_inst_cplx( new_cell, RBA::CplxTrans::new_mrmu( 1.0 / 3.0, 0.0, false, RBA::DPoint::new_xy( 2000.0, 0.0 ) ) )
    ]
    insts.each { |i| cell.insert( i ) }
  
    #  recursively create new cells on this one
    create_recursion( layout, new_cell, layer_id, rec_level + 1 )

  end

end    

app = RBA::Application.instance
mw = app.main_window

#  create a new layout 
mw.create_layout( 0 )
view = mw.current_view

#  create a new layer in that layout
layout = view.cellview( 0 ).layout 
linfo = RBA::LayerInfo.new 
layer_id = layout.insert_layer( linfo )

#  create a layer view for that layer
ln = RBA::LayerPropertiesNode::new
ln.dither_pattern = 0
ln.fill_color = 0xffff00
ln.frame_color = 0xffff00
ln.width = 1
ln.source_layer_index = layer_id
view.insert_layer( view.end_layers, ln )

#  create a top cell and start the recursion on this
topcell_id = layout.add_cell( "top" )
create_recursion( layout, topcell_id, layer_id, 1 )

#  select his cell as the top cell, fit all and switch on all hierarchy levels
view.select_cell_path( [topcell_id], 0 )
view.update_content
view.zoom_fit
view.max_hier

#  run the application
RBA::Application.instance.exec


