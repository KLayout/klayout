
TEMPLATE = subdirs
SUBDIRS = klayout_main

equals(HAVE_RUBY, "1") {

  # klayout_main_tests requires Ruby to run, hence HAVE_RUBY
  SUBDIRS += tests
  tests.depends += klayout_main

}

