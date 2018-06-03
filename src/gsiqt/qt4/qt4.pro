
TEMPLATE = subdirs

SUBDIRS = \
  QtCore \
  QtGui \
  QtXml \
  QtSql \
  QtNetwork \
  QtDesigner

# This is weired, but true: because QSignalMapper (inside QtCore) has a QWidget
# argument, we need to enforce linking of QtCore against QtGui so this GSI declaration
# is available there
# TODO:
QtCore.depends += QtGui
