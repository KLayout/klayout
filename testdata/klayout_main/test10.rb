
lv = RBA::LayoutView::new
lv.load_layout($input)
lv.resize(800, 500)
lv.save_screenshot("test10_screenshot.png")  # smoke test

puts lv.active_cellview.layout.top_cell.dbbox.to_s

