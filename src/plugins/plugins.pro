TEMPLATE = subdirs

# Automatically include all sub-folders, but not the .pro file
SUBDIR_LIST = $$files($$PWD/*)
SUBDIR_LIST -= $$PWD/plugins.pro

SUBDIRS = $$SUBDIR_LIST
