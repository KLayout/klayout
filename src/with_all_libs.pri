
INCLUDEPATH += $$RBA_INC $$PYA_INC $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC $$LYM_INC $$LAYBASIC_INC $$LAY_INC $$ANT_INC $$IMG_INC $$EDT_INC $$LIB_INC $$VERSION_INC
DEPENDPATH += $$RBA_INC $$PYA_INC $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC $$LYM_INC $$LAYBASIC_INC $$LAY_INC $$ANT_INC $$IMG_INC $$EDT_INC $$LIB_INC $$VERSION_INC

LIBS += "$$PYTHONLIBFILE" "$$RUBYLIBFILE" -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db -lklayout_rdb -lklayout_lib

!equals(HAVE_QT, "0") {

  INCLUDEPATH += $$LYM_INC $$LAYBASIC_INC $$LAY_INC $$ANT_INC $$IMG_INC $$EDT_INC
  DEPENDPATH += $$LYM_INC $$LAYBASIC_INC $$LAY_INC $$ANT_INC $$IMG_INC $$EDT_INC

  LIBS += -L$$DESTDIR -lklayout_lym -lklayout_laybasic -lklayout_lay -lklayout_ant -lklayout_img -lklayout_edt

  # Note: this accounts for UI-generated headers placed into the output folders in
  # shadow builds:
  INCLUDEPATH += $$DESTDIR/laybasic $$DESTDIR/lay $$DESTDIR/ext
  DEPENDPATH += $$DESTDIR/laybasic $$DESTDIR/lay $$DESTDIR/ext

  equals(HAVE_QTBINDINGS, "1") {
    LIBS += -lklayout_qtbasic -lklayout_QtGui -lklayout_QtCore
  }

}

equals(HAVE_RUBY, "1") {
  LIBS += -lklayout_rba
} else {
  LIBS += -lklayout_rbastub
}

equals(HAVE_PYTHON, "1") {
  LIBS += -lklayout_pya
} else {
  LIBS += -lklayout_pyastub
}

!equals(HAVE_QT, "0") {
  equals(HAVE_RUBY, "1") {
    # DRC is only available with Ruby
    INCLUDEPATH += $$DRC_INC $$LVS_INC
    DEPENDPATH += $$DRC_INC $$LVS_INC
    LIBS += -lklayout_drc -lklayout_lvs
  }
}

msvc {
  LIBS += user32.lib
}
