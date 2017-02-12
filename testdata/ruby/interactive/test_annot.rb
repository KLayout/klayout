
app = RBA::Application.instance
mw = app.main_window

menu = mw.menu

class MenuHandler < RBA::Action
  def initialize( t, k, &action ) 
    self.title = t
    self.shortcut = k
    puts title()
    puts shortcut()
    @action = action
  end
  def triggered 
    @action.call( self ) 
  end
private
  @action
end

$f7_handler = MenuHandler.new( "clear_annotations", "F7" ) {
  RBA::Application.instance.main_window.current_view.clear_annotations
}

$f8_handler = MenuHandler.new( "each_annotation", "F8" ) {
  res = "<html><body><pre>"

  RBA::Application.instance.main_window.current_view.each_annotation do |a|
    res += "{\n"
    res += "  a.p1=#{a.p1}\n"
    res += "  a.p2=#{a.p2}\n"
    res += "  a.style=#{a.style}\n"
    res += "  a.outline=#{a.outline}\n"
    res += "  a.angle_constraint=#{a.angle_constraint}\n"
    res += "  a.snap=#{a.snap?}\n"
    res += "  a.text=#{a.text}\n"
    res += "  a.text_x=#{a.text_x}\n"
    res += "  a.text_y=#{a.text_y}\n"
    res += "}\n"
  end

  res += "</pre></body></html>"
  browser = RBA::BrowserDialog::new
  browser.set_source(RBA::BrowserSource::new_html(res))
  browser.load("int:index.html")
  browser.exec
}

$f9_handler = MenuHandler.new( "insert_ruler", "F8" ) {
  ruler = RBA::Annotation::new
  ruler.p1 = RBA::DPoint::new_xy( 1.0, 5.0 )
  ruler.p2 = RBA::DPoint::new_xy( 10.0, 5.0 )
  RBA::Application.instance.main_window.current_view.insert_annotation( ruler )
}

menu.insert_item("@toolbar.end", "rb_test1", $f7_handler)
menu.insert_item("@toolbar.end", "rb_test2", $f8_handler)
menu.insert_item("@toolbar.end", "rb_test3", $f9_handler)

puts "-----"

app.exec


