
TEMPLATE = subdirs
SUBDIRS = ext unit_tests

unit_tests.depends += ext

