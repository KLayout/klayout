
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

/**
*  @file gsiDeclQAccessibleTextSelectionEvent.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QAccessibleTextSelectionEvent>
#include <QAccessibleInterface>
#include <QObject>
#include "gsiQt.h"
#include "gsiQtGuiCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QAccessibleTextSelectionEvent

// int QAccessibleTextSelectionEvent::selectionEnd()


static void _init_f_selectionEnd_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_selectionEnd_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QAccessibleTextSelectionEvent *)cls)->selectionEnd ());
}


// int QAccessibleTextSelectionEvent::selectionStart()


static void _init_f_selectionStart_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_selectionStart_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QAccessibleTextSelectionEvent *)cls)->selectionStart ());
}


// void QAccessibleTextSelectionEvent::setSelection(int start, int end)


static void _init_f_setSelection_1426 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("start");
  decl->add_arg<int > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("end");
  decl->add_arg<int > (argspec_1);
  decl->set_return<void > ();
}

static void _call_f_setSelection_1426 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  int arg2 = gsi::arg_reader<int >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QAccessibleTextSelectionEvent *)cls)->setSelection (arg1, arg2);
}


namespace gsi
{

static gsi::Methods methods_QAccessibleTextSelectionEvent () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericMethod ("selectionEnd", "@brief Method int QAccessibleTextSelectionEvent::selectionEnd()\n", true, &_init_f_selectionEnd_c0, &_call_f_selectionEnd_c0);
  methods += new qt_gsi::GenericMethod ("selectionStart", "@brief Method int QAccessibleTextSelectionEvent::selectionStart()\n", true, &_init_f_selectionStart_c0, &_call_f_selectionStart_c0);
  methods += new qt_gsi::GenericMethod ("setSelection", "@brief Method void QAccessibleTextSelectionEvent::setSelection(int start, int end)\n", false, &_init_f_setSelection_1426, &_call_f_setSelection_1426);
  return methods;
}

gsi::Class<QAccessibleTextCursorEvent> &qtdecl_QAccessibleTextCursorEvent ();

gsi::Class<QAccessibleTextSelectionEvent> decl_QAccessibleTextSelectionEvent (qtdecl_QAccessibleTextCursorEvent (), "QtGui", "QAccessibleTextSelectionEvent_Native",
  methods_QAccessibleTextSelectionEvent (),
  "@hide\n@alias QAccessibleTextSelectionEvent");

GSI_QTGUI_PUBLIC gsi::Class<QAccessibleTextSelectionEvent> &qtdecl_QAccessibleTextSelectionEvent () { return decl_QAccessibleTextSelectionEvent; }

}


class QAccessibleTextSelectionEvent_Adaptor : public QAccessibleTextSelectionEvent, public qt_gsi::QtObjectBase
{
public:

  virtual ~QAccessibleTextSelectionEvent_Adaptor();

  //  [adaptor ctor] QAccessibleTextSelectionEvent::QAccessibleTextSelectionEvent(QObject *obj, int start, int end)
  QAccessibleTextSelectionEvent_Adaptor(QObject *obj, int start, int end) : QAccessibleTextSelectionEvent(obj, start, end)
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [adaptor ctor] QAccessibleTextSelectionEvent::QAccessibleTextSelectionEvent(QAccessibleInterface *iface, int start, int end)
  QAccessibleTextSelectionEvent_Adaptor(QAccessibleInterface *iface, int start, int end) : QAccessibleTextSelectionEvent(iface, start, end)
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [adaptor impl] QAccessibleInterface *QAccessibleTextSelectionEvent::accessibleInterface()
  QAccessibleInterface * cbs_accessibleInterface_c0_0() const
  {
    return QAccessibleTextSelectionEvent::accessibleInterface();
  }

  virtual QAccessibleInterface * accessibleInterface() const
  {
    if (cb_accessibleInterface_c0_0.can_issue()) {
      return cb_accessibleInterface_c0_0.issue<QAccessibleTextSelectionEvent_Adaptor, QAccessibleInterface *>(&QAccessibleTextSelectionEvent_Adaptor::cbs_accessibleInterface_c0_0);
    } else {
      return QAccessibleTextSelectionEvent::accessibleInterface();
    }
  }

  gsi::Callback cb_accessibleInterface_c0_0;
};

QAccessibleTextSelectionEvent_Adaptor::~QAccessibleTextSelectionEvent_Adaptor() { }

//  Constructor QAccessibleTextSelectionEvent::QAccessibleTextSelectionEvent(QObject *obj, int start, int end) (adaptor class)

static void _init_ctor_QAccessibleTextSelectionEvent_Adaptor_2620 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("obj");
  decl->add_arg<QObject * > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("start");
  decl->add_arg<int > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("end");
  decl->add_arg<int > (argspec_2);
  decl->set_return_new<QAccessibleTextSelectionEvent_Adaptor> ();
}

static void _call_ctor_QAccessibleTextSelectionEvent_Adaptor_2620 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QObject *arg1 = gsi::arg_reader<QObject * >() (args, heap);
  int arg2 = gsi::arg_reader<int >() (args, heap);
  int arg3 = gsi::arg_reader<int >() (args, heap);
  ret.write<QAccessibleTextSelectionEvent_Adaptor *> (new QAccessibleTextSelectionEvent_Adaptor (arg1, arg2, arg3));
}


//  Constructor QAccessibleTextSelectionEvent::QAccessibleTextSelectionEvent(QAccessibleInterface *iface, int start, int end) (adaptor class)

static void _init_ctor_QAccessibleTextSelectionEvent_Adaptor_3940 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("iface");
  decl->add_arg<QAccessibleInterface * > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("start");
  decl->add_arg<int > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("end");
  decl->add_arg<int > (argspec_2);
  decl->set_return_new<QAccessibleTextSelectionEvent_Adaptor> ();
}

static void _call_ctor_QAccessibleTextSelectionEvent_Adaptor_3940 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QAccessibleInterface *arg1 = gsi::arg_reader<QAccessibleInterface * >() (args, heap);
  int arg2 = gsi::arg_reader<int >() (args, heap);
  int arg3 = gsi::arg_reader<int >() (args, heap);
  ret.write<QAccessibleTextSelectionEvent_Adaptor *> (new QAccessibleTextSelectionEvent_Adaptor (arg1, arg2, arg3));
}


// QAccessibleInterface *QAccessibleTextSelectionEvent::accessibleInterface()

static void _init_cbs_accessibleInterface_c0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QAccessibleInterface * > ();
}

static void _call_cbs_accessibleInterface_c0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QAccessibleInterface * > ((QAccessibleInterface *)((QAccessibleTextSelectionEvent_Adaptor *)cls)->cbs_accessibleInterface_c0_0 ());
}

static void _set_callback_cbs_accessibleInterface_c0_0 (void *cls, const gsi::Callback &cb)
{
  ((QAccessibleTextSelectionEvent_Adaptor *)cls)->cb_accessibleInterface_c0_0 = cb;
}


namespace gsi
{

gsi::Class<QAccessibleTextSelectionEvent> &qtdecl_QAccessibleTextSelectionEvent ();

static gsi::Methods methods_QAccessibleTextSelectionEvent_Adaptor () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QAccessibleTextSelectionEvent::QAccessibleTextSelectionEvent(QObject *obj, int start, int end)\nThis method creates an object of class QAccessibleTextSelectionEvent.", &_init_ctor_QAccessibleTextSelectionEvent_Adaptor_2620, &_call_ctor_QAccessibleTextSelectionEvent_Adaptor_2620);
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QAccessibleTextSelectionEvent::QAccessibleTextSelectionEvent(QAccessibleInterface *iface, int start, int end)\nThis method creates an object of class QAccessibleTextSelectionEvent.", &_init_ctor_QAccessibleTextSelectionEvent_Adaptor_3940, &_call_ctor_QAccessibleTextSelectionEvent_Adaptor_3940);
  methods += new qt_gsi::GenericMethod ("accessibleInterface", "@brief Virtual method QAccessibleInterface *QAccessibleTextSelectionEvent::accessibleInterface()\nThis method can be reimplemented in a derived class.", true, &_init_cbs_accessibleInterface_c0_0, &_call_cbs_accessibleInterface_c0_0);
  methods += new qt_gsi::GenericMethod ("accessibleInterface", "@hide", true, &_init_cbs_accessibleInterface_c0_0, &_call_cbs_accessibleInterface_c0_0, &_set_callback_cbs_accessibleInterface_c0_0);
  return methods;
}

gsi::Class<QAccessibleTextSelectionEvent_Adaptor> decl_QAccessibleTextSelectionEvent_Adaptor (qtdecl_QAccessibleTextSelectionEvent (), "QtGui", "QAccessibleTextSelectionEvent",
  methods_QAccessibleTextSelectionEvent_Adaptor (),
  "@qt\n@brief Binding of QAccessibleTextSelectionEvent");

}

