
# ---------------------------------------------------------------------------
#  A class that simplifies the creation of new menu entries by
#  allowing a more "Ruby-style" callbacks 

class MenuHandler < RBA::Action
  def initialize( t, k, &action ) 
    self.title = t
    self.shortcut = k
    @action = action
  end
  def triggered 
    @action.call( self ) 
  end
private
  @action
end

# ---------------------------------------------------------------------------
#  The base class for objects that inhabit the arena

class GameObject
  
  #  Constructor: each object must have a position
  def initialize( x, y )
    @x = x
    @y = y
  end

  #  Helper method: create a cell ("game" is the game controller object)
  def create_cell( game, name )
    if game.layout.has_cell?( name )
      @cell_index = game.layout.cell_by_name( name )
    else
      @cell_index = game.layout.add_cell( name )
      build_cell( game )
    end
  end

  #  Instantiate our cell in the top cell ("game" is the game controller object)
  def instantiate( game )
    t = RBA::Trans::new_u( RBA::Point::new_xy( @x*1000, @y*1000 ) )
    inst = RBA::CellInstArray::new_inst( @cell_index, t )
    game.topcell.insert( inst )
  end

  #  Predicate telling if we can move
  #  Reimplemented by the derived classes
  def can_move?( level, x, y )
    return true
  end

  #  Check, if we are at the given position
  def is_at?( x, y )
    return x == @x && y == @y
  end

  #  Predicate, telling if we are at the guy
  def is_guy? 
    return false
  end

  #  Predicate, telling if we are an obstacle (i.e. a piece of the wall)
  def is_obstacle? 
    return false
  end

  #  Predicate, telling if we are a target 
  def is_target? 
    return false
  end

  #  Predicate, telling if we are a diamond (the "load" to move around) 
  def is_diamond?
    return false
  end

end

# ---------------------------------------------------------------------------
#  A piece of the wall

class Wall < GameObject

  def construct( game )
    create_cell( game, "wall" )
  end

  def build_cell( game )

    lay1 = game.create_layer( "wall.1", 0x800000, 0x404040, 0 )

    ystep = 125
    width = 250
    (1000 / ystep).times { |n|
      x = (n % 2 == 1) ? -width / 2 : 0
      while x < 1000
        brick = RBA::Box::new_lbrt( x < 0 ? 0 : x, n * ystep, x + width > 1000 ? 1000 : x + width, (n + 1) * ystep )
        game.layout.cell( @cell_index ).shapes( lay1 ).insert_box( brick )
        x += width
      end
    }

  end

  def is_obstacle?
    return true
  end

end

# ---------------------------------------------------------------------------
#  A target

class Target < GameObject

  def construct( game )
    create_cell( game, "target" )
  end

  def build_cell( game )

    lay2 = game.create_layer( "target.2", 0x008000, 0x008000, 0 )
    lay1 = game.create_layer( "target.1", 0x40ff40, 0x00ff00, 0 )

    [ [ 0, 50, lay1 ], [ 50, 100, lay2 ], [ 100, 150, lay1 ], [ 150, 200, lay2 ],
      [ 200, 250, lay1 ], [ 250, 300, lay2 ], [ 300, 350, lay1 ] ].each { |r|
      pointlist = []
      n = 32
      n.times { |a| 
        x = 500 + r[1] * Math::cos( (2 * Math::PI * a) / n )
        y = 500 + r[1] * Math::sin( (2 * Math::PI * a) / n )
        pointlist.push( RBA::Point::new_xy( x, y ) )
      }
      shape = RBA::Polygon::new_p( pointlist )
      if r[0] > 0 
        pointlist = []
        n = 32
        n.times { |a| 
          x = 500 + r[0] * Math::cos( (2 * Math::PI * a) / n )
          y = 500 + r[0] * Math::sin( (2 * Math::PI * a) / n )
          pointlist.push( RBA::Point::new_xy( x, y ) )
        }
        shape.insert_hole( pointlist )
      end
      game.layout.cell( @cell_index ).shapes( r[2] ).insert_polygon( shape )
    }

  end

  def is_target? 
    return true
  end

end

# ---------------------------------------------------------------------------
#  A diamond

class Diamond < GameObject

  def construct( game )
    create_cell( game, "diamond" )
  end

  def build_cell( game )

    lay1 = game.create_layer( "diamond.1", 0x80ff80, 0x404040, 0 )

    pts = [ [ [ 300, 900 ], [ 700, 900 ], [ 600, 870 ], [ 400, 870 ] ],
            [ [ 700, 900 ], [ 900, 730 ], [ 680, 800 ], [ 600, 870 ] ],
            [ [ 680, 800 ], [ 900, 730 ], [ 660, 520 ], [ 600, 720 ] ],
            [ [ 660, 520 ], [ 600, 720 ], [ 400, 720 ], [ 340, 520 ] ],
            [ [ 320, 800 ], [ 100, 730 ], [ 340, 520 ], [ 400, 720 ] ],
            [ [ 300, 900 ], [ 100, 730 ], [ 320, 800 ], [ 400, 870 ] ],
            [ [ 400, 870 ], [ 600, 870 ], [ 680, 800 ], [ 600, 720 ], [ 400, 720 ], [ 320, 800 ] ],
            [ [ 100, 730 ], [ 500, 125 ], [ 340, 520 ] ],
            [ [ 340, 520 ], [ 500, 125 ], [ 660, 520 ] ],
            [ [ 660, 520 ], [ 500, 125 ], [ 900, 730 ] ] ]

    pts.each { |pp|
      pointlist = []
      pp.each { |p| pointlist.push( RBA::Point::new_xy( p[0], p[1] ) ) }
      shape = RBA::Polygon::new_p( pointlist )
      game.layout.cell( @cell_index ).shapes( lay1 ).insert_polygon( shape )
    }

  end

  def can_move?( level, x, y )
    level.each_object { |o|
      if o.is_at?( @x + x, @y + y )
        if o.is_obstacle? || o.is_diamond?
          return false
        end
      end
    }
    return true
  end

  def move( level, x, y )
    @x += x
    @y += y
    @in_target = false
    level.each_object { |o|
      if o.is_target? && o.is_at?( @x, @y )
        @in_target = true
      end
    }
  end

  def is_diamond?
    return true
  end

  def in_target?
    return @in_target
  end

end

# ---------------------------------------------------------------------------
#  The guy

class Guy < GameObject

  def construct( game )
    create_cell( game, "guy" )
  end

  def build_cell( game )

    lay1 = game.create_layer( "guy.1", 0x806000, 0x604000, 0 )

    pts = [ [ [ 400, 880 ], [ 420, 940 ], [ 580, 940 ], [ 600, 880 ], [ 550, 750 ], [ 450, 750 ] ],
            [ [ 350, 740 ], [ 630, 740 ], [ 710, 640 ], [ 710, 350 ], [ 630, 350 ], [ 630, 610 ],
              [ 620, 610 ], [ 620, 100 ], [ 700, 100 ], [ 700, 50 ], [ 505, 50 ], [ 505, 400 ],
              [ 495, 400 ], [ 495, 50 ], [ 300, 50 ], [ 300, 100 ], [ 380, 100 ], [ 380, 610 ],
              [ 370, 610 ], [ 370, 350 ], [ 290, 350 ], [ 290, 640 ] ] ]

    pts.each { |pp|
      pointlist = []
      pp.each { |p| pointlist.push( RBA::Point::new_xy( p[0], p[1] ) ) }
      shape = RBA::Polygon::new_p( pointlist )
      game.layout.cell( @cell_index ).shapes( lay1 ).insert_polygon( shape )
    }


  end

  def can_move?( level, x, y )
    level.each_object { |o|
      if o.is_at?( @x + x, @y + y )
        if o.is_obstacle? 
          return false
        elsif o.is_diamond? && !o.can_move?( level, x, y )
          return false
        end
      end
    }
    return true
  end

  def move( level, x, y)
    @x += x
    @y += y
    level.each_object { |o|
      if o.is_at?( @x, @y ) && o.is_diamond?
        o.move( level, x, y )
      end
    }
    return true
  end

  def is_guy?
    return true
  end

end

# ---------------------------------------------------------------------------
#  The arena which is inhabitated by GameObjects

class Level 
  
  def initialize()

    #  This is one example for an arena
    arena = [ 
      '   ####',
      '####  #',
      '#     ####',
      '# $ #  . ##',
      '#  #   .  #',
      '## #$$#.  #',
      '##    #####',
      '# @ ###',
      '#   #',
      '#####',
    ]
  
    @objs = []

    y = arena.size - 1
    arena.each { |l| 
      x = 0
      l.split("").each { |o|
        if o == '#' 
          @objs.push( Wall.new( x, y ) )
        elsif o == '.'
          @objs.push( Target.new( x, y ) )
        elsif o == '$'
          @objs.push( Diamond.new( x, y ) )
        elsif o == '@'
          @guy = Guy.new( x, y )
          @objs.push( @guy )
        end
        x += 1
      }
      y -= 1
    }

  end

  #  iterate over all objects in the arena
  def each_object( &action )
    @objs.each { |o| action.call( o ) }
  end

  #  get the object representing the guy
  def guy
    return @guy
  end

end

# ---------------------------------------------------------------------------
#  The game controller

class Game

  def initialize()

    #  Get the reference to the application object, the main window and the menu
    app = RBA::Application.instance
    mw = app.main_window
    menu = mw.menu

    #  create the menu handlers
    #  IMPORTANT: in order to keep the references (which is not done on C++ side)
    #  we need to assign the reference to member variables
    @down_handler  = MenuHandler.new( "Down", "Down" )   { move( 0, -1 ) }
    @left_handler  = MenuHandler.new( "Left", "Left" )   { move( -1, 0 ) }
    @right_handler = MenuHandler.new( "Right", "Right" ) { move( 1, 0 ) }
    @up_handler    = MenuHandler.new( "Up", "Up" )       { move( 0, 1 ) }
    @restart_handler = MenuHandler.new( "Restart", "" )  { restart }

    #  add new menu entries into the toolbar and bind them to our action handlers
    menu.insert_separator("@toolbar.end", "name")
    menu.insert_item("@toolbar.end", "sokoban_down", @down_handler)
    menu.insert_item("@toolbar.end", "sokoban_left", @left_handler)
    menu.insert_item("@toolbar.end", "sokoban_right", @right_handler)
    menu.insert_item("@toolbar.end", "sokoban_up", @up_handler)
    menu.insert_item("@toolbar.end", "sokoban_restart", @restart_handler)

    #  create a new layout and store a reference to it's view objects, layout handle
    #  and a reference to the top cell
    mw.create_layout( "", 0 )
    @view = mw.current_view
    @layout = @view.cellview( 0 ).layout 
    @topcell = @layout.add_cell( "game" )

    #  initialize the layer list: so far we do not have layers
    @layers = {}

    #  create and initialize some dummy objects so it is guaranteed that the layers are
    #  created in the right order.
    dummy_objs = [ Wall.new( 0, 0 ), Target.new( 0, 0 ), Diamond.new( 0, 0 ), Guy.new( 0, 0 ) ]
    dummy_objs.each { |o| o.construct( self ) }

    #  instantiate the level and create 
    @level = Level.new
    @level.each_object { |o| o.construct( self ) }
    @level.each_object { |o| o.instantiate( self ) }

    #  set up the viewer window: select the new cell for top cell, update cell hierarchy browser
    #  and layer list, fit all and show all levels of hierarchy
    @view.select_cell_path( [@topcell], 0 )
    @view.update_content
    @view.zoom_fit
    @view.max_hier

  end

  #  start over 
  def restart
    @level = Level.new
    @level.each_object { |o| o.construct( self ) }
    redraw
  end

  #  refresh the layout with the current arena setup
  def redraw

    #  IMPORTANT: always stop the redraw thread before applying changes
    @view.stop_redraw

    #  empty the top cell and recreate the instances to the game objects 
    #  so they appear at their position
    topcell.clear_insts
    @level.each_object { |o| o.instantiate( self ) }
    @view.select_cell_path( [@topcell], 0 )

    #  force an update and redraw of the content
    @view.update_content
    RBA::Application.instance.main_window.redraw

  end

  #  move the guy by the specified distance
  def move( dx, dy )

    #  IMPORTANT: because the user may have closed the view panel or the layout, 
    #  we need to check, if we still have a valid object
    if ! @view.destroyed?

      #  check, if we can move the guy and do so.
      if @level.guy.can_move?( @level, dx, dy )
        @level.guy.move( @level, dx, dy )
      end

      #  update the arena view
      redraw

      #  check, if all objects have been moved into their targets
      all_in_target = true
      @level.each_object { |o| 
        if o.is_diamond? && !o.in_target?
          all_in_target = false
        end
      }
      if all_in_target
        RBA::MessageBox::info( "Done", "Congratulations! Level done.", RBA::MessageBox::b_ok )
        @level = Level.new
        @level.each_object { |o| o.construct( self ) }
        redraw
      end

    end

  end

  #  retrieve the top cell handle
  def topcell 
    return @layout.cell( @topcell )
  end

  #  retrieve the layout handle
  def layout 
    return @layout
  end

  #  create a layer with the given properties
  def create_layer( name, color, frame_color, stipple )

    if @layers[name] == nil 

      linfo = RBA::LayerInfo.new 
      lid = @layout.insert_layer( linfo )
      @layers[name] = lid

      lpp = @view.end_layers
      ln = RBA::LayerPropertiesNode::new
      ln.dither_pattern = stipple
      ln.fill_color = color
      ln.frame_color = frame_color
      ln.width = 1
      ln.source_layer_index = lid
      @view.insert_layer( lpp, ln )

    else
      lid = @layers[name]
    end

    return lid

  end

end

# ---------------------------------------------------------------------------
#  Main application

#  instantiate the game controller
$game = Game.new

#  run the UI
app = RBA::Application.instance.exec


