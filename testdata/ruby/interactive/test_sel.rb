
app = RBA::Application.instance
mw = app.main_window

menu = mw.menu

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


class RBA::InstElement
  def to_s
    "#{self.cell_inst.cell_index} [#{self.specific_cplx_trans.to_s}]"
  end
end

class RBA::ObjectInstPath 
  def to_s
    path = []
    self.each_inst { |p| path.push( p ) }
    pp = "  cv_index=#{self.cv_index}\n" +
         "  cell_index=#{self.cell_index}\n" + 
         "  trans=#{self.trans.to_s}\n" +
         "  path=" + path.join( ":" )
    if self.is_cell_inst? 
      return pp
    else
      return pp + "\n  layer=#{self.layer}\n" +
                  "  shape.bbox=#{self.shape.bbox}" 
    end
  end
end


$f8_handler = MenuHandler.new( "each_selected", "F8" ) do

  cv = RBA::Application.instance.main_window.current_view

  res = "<html><body><pre>"
  if cv.has_object_selection?
    cv.each_object_selected do |s|
      res += "object {\n" + s.to_s + "\n}\n"
    end
  else
    res += "No objects selected."
  end
  res += "</pre></body></html>"
  browser = RBA::BrowserDialog::new
  browser.set_source(RBA::BrowserSource::new_html(res))
  browser.load("int:index.html")
  browser.exec

end

menu.insert_item("@toolbar.end", "rb_test2", $f8_handler)

puts "-----"

app.exec


