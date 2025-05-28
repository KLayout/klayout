
TEMPLATE = subdirs

# Automatically include all sub-folders, but not the .pro file
SUBDIR_LIST = $$files($$PWD/*)
SUBDIR_LIST -= $$PWD/streamers.pro

SUBDIRS = $$SUBDIR_LIST
