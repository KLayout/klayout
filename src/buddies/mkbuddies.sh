
for x in *.cc; do 
  xx=${x/.cc}
  echo "" >$xx/$xx.pro
  echo 'include($$PWD/../../klayout.pri)' >>$xx/$xx.pro
  echo "" >>$xx/$xx.pro
  echo "TEMPLATE = app" >>$xx/$xx.pro
  echo "" >>$xx/$xx.pro
  echo "TARGET = $xx" >>$xx/$xx.pro
  echo 'DESTDIR = $$OUT_PWD/../..' >>$xx/$xx.pro
  echo "" >> $xx/$xx.pro
  echo "SOURCES = $x" >>$xx/$xx.pro
  echo "" >> $xx/$xx.pro
  echo "INCLUDEPATH += ../../tl ../../db ../../gsi" >> $xx/$xx.pro
  echo "DEPENDPATH += ../../tl ../../db ../../gsi" >> $xx/$xx.pro
  echo 'LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi' >> $xx/$xx.pro
done

