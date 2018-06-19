
TEMPLATE = subdirs
SUBDIRS = gsi gsi_test unit_tests

gsi_test.depends += gsi
unit_tests.depends += gsi_test

