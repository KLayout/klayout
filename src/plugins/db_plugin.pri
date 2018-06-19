
include($$PWD/../klayout.pri)

TEMPLATE = lib

INCLUDEPATH += $$DB_INC $$TL_INC $$GSI_INC $$PWD/common
DEPENDPATH += $$DB_INC $$TL_INC $$GSI_INC $$PWD/common
LIBS += -L$$DESTDIR/.. -lklayout_db -lklayout_tl -lklayout_gsi

DEFINES += MAKE_DB_PLUGIN_LIBRARY

win32 {

  dlltarget.path = $$PREFIX/db_plugins
  INSTALLS += dlltarget

  # to avoid the major version being appended to the dll name - in this case -lxyz won't link it again
  # because the library is called xyx0.dll.
  CONFIG += skip_target_version_ext

} else {

  target.path = $$PREFIX/db_plugins
  INSTALLS += target

}
