
class MenuAction < RBA::Action
  def initialize( title, shortcut, &action ) 
    self.title = title
    self.shortcut = shortcut
    @action = action
  end
  def triggered 
    @action.call( self ) 
  end
private
  @action
end

# HINT: this simple implementation does not account 
# for variant building - the shapes and instances are simply taken
# out from the original shapes and therefore might disappear somewhere else.

$f7_handler = MenuAction.new( "Flatten", "F7" ) do 

  app = RBA::Application.instance
  mw = app.main_window

  lv = mw.current_view
  if lv == nil
    raise "Flatten: No view selected"
  end

  # start transaction for "undo"
  lv.transaction( "Flatten" )

  begin

    # because objects might be referenced multiple times (thru different hierarchy paths)
    # we must first copy and then delete them

    # copy & transform them
    lv.each_object_selected do |sel|

      cv = lv.cellview( sel.cv_index ) 
      target = cv.cell 

      # only if not flat already ..
      if target.cell_index != sel.cell_index

        source = cv.layout.cell( sel.cell_index )

        if sel.is_cell_inst? 

          # copy and transform
          new_inst = target.insert( sel.inst )
          target.transform( new_inst, sel.source_trans )

        else

          # copy and transform
          target_shapes = target.shapes( sel.layer )
          new_shape = target_shapes.insert( sel.shape )
          target_shapes.transform( new_shape, sel.source_trans )
          
        end

      end

    end

    # delete the objects
    # HINT: since it is possible that a certain object is used multiple times, we need to test
    # each reference, if it is still valid (i.e. the object has not been deleted yet).
    lv.each_object_selected do |sel|

      cv = lv.cellview( sel.cv_index ) 
      target = cv.cell 

      # only if not flat already ..
      if target.cell_index != sel.cell_index

        source = cv.layout.cell( sel.cell_index )

        if sel.is_cell_inst? 
          if source.is_valid?( sel.inst )
            source.erase( sel.inst )
          end
        else
          if source.shapes( sel.layer ).is_valid?( sel.shape )
            source.shapes( sel.layer ).erase( sel.shape )
          end
        end

      end

    end

  ensure

    # always execute that code:

    # commit transaction
    lv.commit

    # clear selection and cancel all other edit operations, so 
    # nothing refers to shapes that might have been deleted.
    lv.cancel

  end

end

# HINT: this simple implementation does not account 
# for variant building - the shapes and instances are simply taken
# out from the original shapes and therefore might disappear somewhere else.

$f8_handler = MenuAction.new( "Propagate", "F8" ) do 

  app = RBA::Application.instance
  mw = app.main_window

  lv = mw.current_view
  if lv == nil
    raise "Propagate: No view selected"
  end

  # start transaction for "undo"
  lv.transaction( "Propagate" )

  begin

    # because objects might be referenced multiple times (thru different hierarchy paths)
    # we must first copy and then delete them

    # copy & transform them
    lv.each_object_selected do |sel|

      cv = lv.cellview( sel.cv_index ) 

      # only if not flat already ..
      if cv.cell_index != sel.cell_index

        source = cv.layout.cell( sel.cell_index )

        if sel.is_cell_inst? 

          # copy and transform
          if sel.path_length <= 2
            target = cv.cell 
          else
            target = cv.layout.cell( sel.path_nth( sel.path_length - 3 ).cell_inst.cell_index )
          end
          new_inst = target.insert( sel.inst )
          target.transform( new_inst, sel.path_nth( sel.path_length - 2 ).specific_cplx_trans )

        else

          # copy and transform
          if sel.path_length <= 1
            target = cv.cell 
          else
            target = cv.layout.cell( sel.path_nth( sel.path_length - 2 ).cell_inst.cell_index )
          end
          target_shapes = target.shapes( sel.layer )
          new_shape = target_shapes.insert( sel.shape )
          target_shapes.transform( new_shape, sel.path_nth( sel.path_length - 1 ).specific_cplx_trans )
          
        end

      end

    end

    # delete the objects
    # HINT: since it is possible that a certain object is used multiple times, we need to test
    # each reference, if it is still valid (i.e. the object has not been deleted yet).
    lv.each_object_selected do |sel|

      cv = lv.cellview( sel.cv_index ) 
      target = cv.cell 

      # only if not flat already ..
      if target.cell_index != sel.cell_index

        source = cv.layout.cell( sel.cell_index )

        if sel.is_cell_inst? 
          if source.is_valid?( sel.inst )
            source.erase( sel.inst )
          end
        else
          if source.shapes( sel.layer ).is_valid?( sel.shape )
            source.shapes( sel.layer ).erase( sel.shape )
          end
        end

      end

    end

  ensure

    # always execute that code:

    # commit transaction
    lv.commit

    # clear selection and cancel all other edit operations, so 
    # nothing refers to shapes that might have been deleted.
    lv.cancel

  end

end

app = RBA::Application.instance
mw = app.main_window

menu = mw.menu
menu.insert_separator("@toolbar.end", "name")
menu.insert_item("@toolbar.end", "rba_flatten", $f7_handler)
menu.insert_item("@toolbar.end", "rba_propagate", $f8_handler)

app.exec


