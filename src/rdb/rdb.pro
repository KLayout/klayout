
TEMPLATE = subdirs
SUBDIRS = rdb unit_tests

unit_tests.depends += rdb

