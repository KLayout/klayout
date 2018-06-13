
TEMPLATE = subdirs

SUBDIRS = db_plugin lay_plugin unit_tests

lay_plugin.depends += db_plugin
unit_tests.depends += db_plugin

