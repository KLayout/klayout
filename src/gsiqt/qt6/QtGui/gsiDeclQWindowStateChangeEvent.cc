
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
*  @file gsiDeclQWindowStateChangeEvent.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QWindowStateChangeEvent>
#include "gsiQt.h"
#include "gsiQtGuiCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QWindowStateChangeEvent

// bool QWindowStateChangeEvent::isOverride()


static void _init_f_isOverride_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<bool > ();
}

static void _call_f_isOverride_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<bool > ((bool)((QWindowStateChangeEvent *)cls)->isOverride ());
}


// QFlags<Qt::WindowState> QWindowStateChangeEvent::oldState()


static void _init_f_oldState_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QFlags<Qt::WindowState> > ();
}

static void _call_f_oldState_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QFlags<Qt::WindowState> > ((QFlags<Qt::WindowState>)((QWindowStateChangeEvent *)cls)->oldState ());
}


namespace gsi
{

static gsi::Methods methods_QWindowStateChangeEvent () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericMethod ("isOverride?", "@brief Method bool QWindowStateChangeEvent::isOverride()\n", true, &_init_f_isOverride_c0, &_call_f_isOverride_c0);
  methods += new qt_gsi::GenericMethod ("oldState", "@brief Method QFlags<Qt::WindowState> QWindowStateChangeEvent::oldState()\n", true, &_init_f_oldState_c0, &_call_f_oldState_c0);
  return methods;
}

gsi::Class<QEvent> &qtdecl_QEvent ();

gsi::Class<QWindowStateChangeEvent> decl_QWindowStateChangeEvent (qtdecl_QEvent (), "QtGui", "QWindowStateChangeEvent_Native",
  methods_QWindowStateChangeEvent (),
  "@hide\n@alias QWindowStateChangeEvent");

GSI_QTGUI_PUBLIC gsi::Class<QWindowStateChangeEvent> &qtdecl_QWindowStateChangeEvent () { return decl_QWindowStateChangeEvent; }

}


class QWindowStateChangeEvent_Adaptor : public QWindowStateChangeEvent, public qt_gsi::QtObjectBase
{
public:

  virtual ~QWindowStateChangeEvent_Adaptor();

  //  [adaptor ctor] QWindowStateChangeEvent::QWindowStateChangeEvent(QFlags<Qt::WindowState> oldState, bool isOverride)
  QWindowStateChangeEvent_Adaptor(QFlags<Qt::WindowState> oldState) : QWindowStateChangeEvent(oldState)
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [adaptor ctor] QWindowStateChangeEvent::QWindowStateChangeEvent(QFlags<Qt::WindowState> oldState, bool isOverride)
  QWindowStateChangeEvent_Adaptor(QFlags<Qt::WindowState> oldState, bool isOverride) : QWindowStateChangeEvent(oldState, isOverride)
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [adaptor impl] void QWindowStateChangeEvent::setAccepted(bool accepted)
  void cbs_setAccepted_864_0(bool accepted)
  {
    QWindowStateChangeEvent::setAccepted(accepted);
  }

  virtual void setAccepted(bool accepted)
  {
    if (cb_setAccepted_864_0.can_issue()) {
      cb_setAccepted_864_0.issue<QWindowStateChangeEvent_Adaptor, bool>(&QWindowStateChangeEvent_Adaptor::cbs_setAccepted_864_0, accepted);
    } else {
      QWindowStateChangeEvent::setAccepted(accepted);
    }
  }

  gsi::Callback cb_setAccepted_864_0;
};

QWindowStateChangeEvent_Adaptor::~QWindowStateChangeEvent_Adaptor() { }

//  Constructor QWindowStateChangeEvent::QWindowStateChangeEvent(QFlags<Qt::WindowState> oldState, bool isOverride) (adaptor class)

static void _init_ctor_QWindowStateChangeEvent_Adaptor_3346 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("oldState");
  decl->add_arg<QFlags<Qt::WindowState> > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("isOverride", true, "false");
  decl->add_arg<bool > (argspec_1);
  decl->set_return_new<QWindowStateChangeEvent_Adaptor> ();
}

static void _call_ctor_QWindowStateChangeEvent_Adaptor_3346 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QFlags<Qt::WindowState> arg1 = gsi::arg_reader<QFlags<Qt::WindowState> >() (args, heap);
  bool arg2 = args ? gsi::arg_reader<bool >() (args, heap) : gsi::arg_maker<bool >() (false, heap);
  ret.write<QWindowStateChangeEvent_Adaptor *> (new QWindowStateChangeEvent_Adaptor (arg1, arg2));
}


// void QWindowStateChangeEvent::setAccepted(bool accepted)

static void _init_cbs_setAccepted_864_0 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("accepted");
  decl->add_arg<bool > (argspec_0);
  decl->set_return<void > ();
}

static void _call_cbs_setAccepted_864_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  bool arg1 = args.read<bool > (heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QWindowStateChangeEvent_Adaptor *)cls)->cbs_setAccepted_864_0 (arg1);
}

static void _set_callback_cbs_setAccepted_864_0 (void *cls, const gsi::Callback &cb)
{
  ((QWindowStateChangeEvent_Adaptor *)cls)->cb_setAccepted_864_0 = cb;
}


namespace gsi
{

gsi::Class<QWindowStateChangeEvent> &qtdecl_QWindowStateChangeEvent ();

static gsi::Methods methods_QWindowStateChangeEvent_Adaptor () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QWindowStateChangeEvent::QWindowStateChangeEvent(QFlags<Qt::WindowState> oldState, bool isOverride)\nThis method creates an object of class QWindowStateChangeEvent.", &_init_ctor_QWindowStateChangeEvent_Adaptor_3346, &_call_ctor_QWindowStateChangeEvent_Adaptor_3346);
  methods += new qt_gsi::GenericMethod ("setAccepted", "@brief Virtual method void QWindowStateChangeEvent::setAccepted(bool accepted)\nThis method can be reimplemented in a derived class.", false, &_init_cbs_setAccepted_864_0, &_call_cbs_setAccepted_864_0);
  methods += new qt_gsi::GenericMethod ("setAccepted", "@hide", false, &_init_cbs_setAccepted_864_0, &_call_cbs_setAccepted_864_0, &_set_callback_cbs_setAccepted_864_0);
  return methods;
}

gsi::Class<QWindowStateChangeEvent_Adaptor> decl_QWindowStateChangeEvent_Adaptor (qtdecl_QWindowStateChangeEvent (), "QtGui", "QWindowStateChangeEvent",
  methods_QWindowStateChangeEvent_Adaptor (),
  "@qt\n@brief Binding of QWindowStateChangeEvent");

}

