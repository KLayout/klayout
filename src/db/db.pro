
TEMPLATE = subdirs
SUBDIRS = db unit_tests

unit_tests.depends += db

