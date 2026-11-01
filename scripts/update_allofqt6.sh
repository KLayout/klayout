#!/bin/bash -e

qt=/opt/qt/6.6.1

for mod in \
  QtCore \
  QtCore5Compat \
  QtDesigner \
  QtGui \
  QtMultimedia \
  QtNetwork \
  QtPrintSupport \
  QtSql \
  QtSvg \
  QtUiTools \
  QtWidgets \
  QtXml; do

  echo "Running $mod .."
  ./update_allofqt.rb mkqtdecl6/$mod/allofqt.cpp $qt/include/$mod $mod mkqtdecl6/$mod/allofqt.cpp

done


