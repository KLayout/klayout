

class Screenshot

  def initialize( index, view )

    @index = index
    @small_file = "screenshot.small.#{@index}.png"
    @large_file = "screenshot.large.#{@index}.png"

    view.save_image( @small_file, 80, 60 )
    view.save_image( @large_file, 800, 600 )
    @box = view.box

  end

  def box
    return @box
  end

  def html_snippet
    s = ""
    l = sprintf( "%.3f", @box.left.to_s )
    r = sprintf( "%.3f", @box.right.to_s )
    b = sprintf( "%.3f", @box.bottom.to_s )
    t = sprintf( "%.3f", @box.top.to_s )
    s += "<a style=\"font-size:10px\" href=\"int:navigate?#{@index}\">#{l},#{b} <br/>"
    s += "#{r},#{t}</a><br/>"
    s += "<a href=\"int:enlarged?#{@large_file}\"><img src=\"file:#{@small_file}\" width=\"80\" height=\"60\"/></a>"
    return s
  end

end

class Browser < RBA::BrowserDialog

  def initialize
    @browser_source = Server.new
    self.set_source( @browser_source )
    self.set_home( "int:index" )
    self.show
  end

  def add
    @browser_source.add
    self.show
    self.load( "int:index" )
    self.reload 
  end

end

class Server < RBA::BrowserSource

  def initialize
    @screenshots = []
  end

  def add
    view = RBA::Application::instance.main_window.current_view
    if view != nil
      @screenshots.push( Screenshot.new( @screenshots.size, view ) )
    end
  end

  def get( url )
    url = url.sub( /^int:/, "" )
    if url == "index"
      return self.index
    elsif url =~ /^enlarged\?(.*)/
      return enlarged( $1 )
    elsif url =~ /^navigate\?(.*)/
      navigate( $1.to_i )
    end
    return ""
  end

  def navigate( index )
    view = RBA::Application::instance.main_window.current_view
    if view != nil
      view.zoom_box( @screenshots[index].box )
    end
  end

  def enlarged( img )
    return "<a href=\"int:index\">Index Page</a><br/><img src=\"file:#{img}\"/>"
  end

  def index
    r = []
    r.push( "<html><h1>Screenshot gallery</h1><p><p>" )
    r.push( "<table>" )
    r.push( "<tr>" )
    n = 0
    @screenshots.each { |s|
      r.push( "<td>" )
      r.push( s.html_snippet )
      r.push( "</td>" )
      n += 1
      if n % 5 == 0
        r.push( "</tr><tr>" )
      end
    }
    r.push( "</tr>" )
    r.push( "</table>" )
    return r.join( "" )
  end

end

# ------------------------------------------------------------------
#  Declare and register the menu items 

app = RBA::Application::instance
mw = app.main_window
menu = mw.menu

$browser = nil

class AddScreenshot < RBA::Action

  def initialize
    self.title = "Add Screenshot"
  end

  def triggered
    if $browser == nil
      $browser = Browser.new 
    end
    $browser.add
  end

end

class ShowGallery < RBA::Action

  def initialize
    self.title = "Show Gallery"
  end

  def triggered
    if $browser != nil
      $browser.show
    end
  end

end

$add_screenshot_action = AddScreenshot.new
$show_gallery_action = ShowGallery.new

menu.insert_separator( "@toolbar.end", "tb_sep" )
menu.insert_item( "@toolbar.end", "add_screenshot", $add_screenshot_action )
menu.insert_item( "@toolbar.end", "show_gallery", $show_gallery_action )

app.exec

