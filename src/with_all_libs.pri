
INCLUDEPATH += $$RBA_INC $$PYA_INC $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC $$LYM_INC $$LAYBASIC_INC $$LAYVIEW_INC $$ANT_INC $$IMG_INC $$EDT_INC $$LIB_INC $$VERSION_INC
DEPENDPATH += $$RBA_INC $$PYA_INC $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC $$LYM_INC $$LAYBASIC_INC $$LAYVIEW_INC $$ANT_INC $$IMG_INC $$EDT_INC $$LIB_INC $$VERSION_INC

LIBS += "$$PYTHONLIBFILE" "$$RUBYLIBFILE" -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db -lklayout_rdb -lklayout_lym -lklayout_laybasic -lklayout_layview -lklayout_ant -lklayout_img -lklayout_edt -lklayout_lib

!equals(HAVE_QT, "0") {

  INCLUDEPATH += $$LAYUI_INC $$LAY_INC
  DEPENDPATH += $$LAYUI_INC $$LAY_INC

  LIBS += -L$$DESTDIR -lklayout_layui -lklayout_lay

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
