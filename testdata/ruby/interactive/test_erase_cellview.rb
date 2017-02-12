
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



$f8_handler = MenuHandler.new( "erase current cellview", "F8" ) do

  view = RBA::Application.instance.main_window.current_view
  view.erase_cellview( view.active_cellview_index )

end

menu.insert_item("@toolbar.end", "rb_test2", $f8_handler)

puts "-----"

app.exec


