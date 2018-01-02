
puts "RBA::Application superclass " + RBA::Application.instance.class.superclass.to_s
puts "MainWindow is " + (RBA::Application.instance.main_window ? "there" : "not there")

