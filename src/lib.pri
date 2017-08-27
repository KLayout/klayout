
include($$PWD/klayout.pri)

TEMPLATE = lib

win32 {
  dlltarget.path = $$PREFIX
  INSTALLS += dlltarget
} else {
  target.path = $$PREFIX
  INSTALLS += target
}
