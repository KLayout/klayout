
include($$PWD/klayout.pri)

TEMPLATE = lib

win32 {

  dlltarget.path = $$PREFIX
  INSTALLS += dlltarget

  # to avoid the major version being appended to the dll name - in this case -lxyz won't link it again
  # because the library is called xyx0.dll.
  CONFIG += skip_target_version_ext

} else {

  target.path = $$PREFIX
  INSTALLS += target

}
