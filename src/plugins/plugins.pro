TEMPLATE = subdirs

# Automatically include all sub-folders, but not the .pro file
SUBDIR_LIST = $$files($$PWD/*)
SUBDIR_LIST -= $$PWD/plugins.pro
SUBDIR_LIST -= $$PWD/plugin.pri
SUBDIR_LIST -= $$PWD/plugin_ut.pri

SUBDIRS = $$SUBDIR_LIST
