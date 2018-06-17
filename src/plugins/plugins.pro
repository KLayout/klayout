TEMPLATE = subdirs

# Automatically include all sub-folders, but not the .pro file
SUBDIR_LIST = $$files($$PWD/*)
SUBDIR_LIST -= $$PWD/plugins.pro
SUBDIR_LIST -= $$PWD/db_plugin.pri
SUBDIR_LIST -= $$PWD/lay_plugin.pri
SUBDIR_LIST -= $$PWD/common

SUBDIRS = $$SUBDIR_LIST
