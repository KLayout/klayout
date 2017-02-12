

# ---------------------------------------------------------------------------
#  The HTML browser object

class Browser < RBA::BrowserDialog

  def open
    @browser_source = Server.new( self )
    self.set_source( @browser_source )
    self.set_home( "int:index" )
    self.show
  end

  def closed
    @browser_source.close
    @browser_source = nil
  end

end

# ---------------------------------------------------------------------------
#  The data provider for the browser 

class Server < RBA::BrowserSource

  #  set up the data provider
  def initialize( browser )

    #  this is an arbitrary list of locations to go to
    @boxes = [ RBA::DBox::new_lbrt( -10.0, 110.0, 10.0, 130.0 ),
               RBA::DBox::new_lbrt( 10.0, 100.0, 12.0, 103.0 ),
               RBA::DBox::new_lbrt( 22.0, -10.0, 23.0, -11.0 ) ]

    @marker = nil
    @visited = []
    @boxes.size.times { @visited.push( false ) }
    @browser = browser

  end

  #  on close, destroy the marker 
  def close
    if @marker != nil
      @marker.destroy
    end
    @marker = nil
  end

  #  reimplementation of the data provider's main function
  def get( url )

    url = url.sub( /^int:/, "" )
    if url == "index"
      #  deliver the index page
      return self.index
    elsif url =~ /^zoom_to\?(\d+)/
      #  navigate to the specified position
      navigate( $1.to_i )
    end

    #  default behaviour: do not deliver anything - do not set the browser to a new location
    return ""

  end

  #  deliver the index page
  def index
    r = "<html><h1>Locations to select</h1><p><p>"
    @boxes.size.times { |index|
      if @visited[index]
        #  visited locations are shown in red color
        r += "<a style=\"color:#ff0000\" href=\"int:zoom_to?#{index}\">Zoom to location " + @boxes[index].to_s + "</a><p>"
      else
        r += "<a href=\"int:zoom_to?#{index}\">Zoom to location " + @boxes[index].to_s + "</a></b><p>"
      end
    }
    return r
  end

  #  go to a certain location
  def navigate( loc_index )

    if loc_index < @boxes.size

      #  mark this location as visited
      @visited[loc_index] = true

      #  place the marker on the current view. If no view is opened, this may be nil!
      view = RBA::Application::instance.main_window.current_view
      if view != nil

        #  zoom to the specified position (put 10 micron space around that location to 
        #  get the displayed rectangle)
        box = @boxes[loc_index]
        view.zoom_box( box.enlarged( RBA::DPoint::new_xy( 10.0, 10.0 ) ) )

        #  before creating a new marker, delete the current marker unless it was destroyed already 
        #  (this is a recommended safty measure)
        if @marker != nil && !@marker.destroyed
          @marker.destroy
        end

        #  create a new marker that shows the box selected
        @marker = RBA::Marker::new( view )
        @marker.set_box( box )

      end

      #  force a reload of the page (necessary, since the color of the links may have changed)
      @browser.reload 

    end

  end

end

# ---------------------------------------------------------------------------
#  The menu handler that opens the browser

class StartBrowser < RBA::Action

  def initialize
    self.title = "Browser"
    @browser = nil
    @browser_source = nil
  end

  def triggered
    if @browser == nil
      @browser = Browser.new
    end
    @browser.open
  end

end

#  register the menu item and attach a StartBrowser action 
app = RBA::Application::instance
mw = app.main_window
menu = mw.menu

$start_browser_action = StartBrowser.new

menu.insert_separator( "@toolbar.end", "tb_sep" )
menu.insert_item( "@toolbar.end", "start_browser", $start_browser_action )

#  run the application
app.exec

