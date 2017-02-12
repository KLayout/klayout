TEMPLATE = subdirs

SUBDIR_LIST = $$files($$PWD/*)
SUBDIR_LIST -= $$PWD/plugins.pro

SUBDIRS = $$SUBDIR_LIST
