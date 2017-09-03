
TEMPLATE = subdirs
SUBDIRS = gsi unit_tests

gsi_test.depends += gsi
unit_tests.depends += gsi

