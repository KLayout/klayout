
include($$PWD/../klayout.pri)

TEMPLATE = lib

INCLUDEPATH += $$DB_INC $$TL_INC $$GSI_INC $$LAYBASIC_INC $$LAY_INC $$PWD/common
DEPENDPATH += $$DB_INC $$TL_INC $$GSI_INC $$LAYBASIC_INC $$LAY_INC $$PWD/common
LIBS += -L$$DESTDIR/.. -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_lay

DEFINES += MAKE_LAY_PLUGIN_LIBRARY

win32 {

  dlltarget.path = $$PREFIX/lay_plugins
  INSTALLS += dlltarget

  # to avoid the major version being appended to the dll name - in this case -lxyz won't link it again
  # because the library is called xyx0.dll.
  CONFIG += skip_target_version_ext

} else {

  target.path = $$PREFIX/lay_plugins
  INSTALLS += target

}
