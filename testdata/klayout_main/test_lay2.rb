
RBA::Application::instance.main_window.views.times do |v|
  view = RBA::Application::instance.main_window.view(v)
  tc = []
  view.cellviews.times { |cv| tc << view.cellview(cv).layout.top_cell.name }
  puts tc.join(";")
end

