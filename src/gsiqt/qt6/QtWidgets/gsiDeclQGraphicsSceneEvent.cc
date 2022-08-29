
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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
*  @file gsiDeclQGraphicsSceneEvent.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QGraphicsSceneEvent>
#include <QEvent>
#include <QWidget>
#include "gsiQt.h"
#include "gsiQtWidgetsCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QGraphicsSceneEvent

// void QGraphicsSceneEvent::setTimestamp(quint64 ts)


static void _init_f_setTimestamp_1103 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("ts");
  decl->add_arg<quint64 > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setTimestamp_1103 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  quint64 arg1 = gsi::arg_reader<quint64 >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneEvent *)cls)->setTimestamp (arg1);
}


// void QGraphicsSceneEvent::setWidget(QWidget *widget)


static void _init_f_setWidget_1315 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("widget");
  decl->add_arg<QWidget * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setWidget_1315 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QWidget *arg1 = gsi::arg_reader<QWidget * >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneEvent *)cls)->setWidget (arg1);
}


// quint64 QGraphicsSceneEvent::timestamp()


static void _init_f_timestamp_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<quint64 > ();
}

static void _call_f_timestamp_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<quint64 > ((quint64)((QGraphicsSceneEvent *)cls)->timestamp ());
}


// QWidget *QGraphicsSceneEvent::widget()


static void _init_f_widget_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QWidget * > ();
}

static void _call_f_widget_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QWidget * > ((QWidget *)((QGraphicsSceneEvent *)cls)->widget ());
}


namespace gsi
{

static gsi::Methods methods_QGraphicsSceneEvent () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericMethod ("setTimestamp", "@brief Method void QGraphicsSceneEvent::setTimestamp(quint64 ts)\n", false, &_init_f_setTimestamp_1103, &_call_f_setTimestamp_1103);
  methods += new qt_gsi::GenericMethod ("setWidget|widget=", "@brief Method void QGraphicsSceneEvent::setWidget(QWidget *widget)\n", false, &_init_f_setWidget_1315, &_call_f_setWidget_1315);
  methods += new qt_gsi::GenericMethod ("timestamp", "@brief Method quint64 QGraphicsSceneEvent::timestamp()\n", true, &_init_f_timestamp_c0, &_call_f_timestamp_c0);
  methods += new qt_gsi::GenericMethod (":widget", "@brief Method QWidget *QGraphicsSceneEvent::widget()\n", true, &_init_f_widget_c0, &_call_f_widget_c0);
  return methods;
}

gsi::Class<QEvent> &qtdecl_QEvent ();

gsi::Class<QGraphicsSceneEvent> decl_QGraphicsSceneEvent (qtdecl_QEvent (), "QtWidgets", "QGraphicsSceneEvent_Native",
  methods_QGraphicsSceneEvent (),
  "@hide\n@alias QGraphicsSceneEvent");

GSI_QTWIDGETS_PUBLIC gsi::Class<QGraphicsSceneEvent> &qtdecl_QGraphicsSceneEvent () { return decl_QGraphicsSceneEvent; }

}


class QGraphicsSceneEvent_Adaptor : public QGraphicsSceneEvent, public qt_gsi::QtObjectBase
{
public:

  virtual ~QGraphicsSceneEvent_Adaptor();

  //  [adaptor ctor] QGraphicsSceneEvent::QGraphicsSceneEvent(QEvent::Type type)
  QGraphicsSceneEvent_Adaptor(QEvent::Type type) : QGraphicsSceneEvent(type)
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [adaptor impl] QEvent *QGraphicsSceneEvent::clone()
  QEvent * cbs_clone_c0_0() const
  {
    return QGraphicsSceneEvent::clone();
  }

  virtual QEvent * clone() const
  {
    if (cb_clone_c0_0.can_issue()) {
      return cb_clone_c0_0.issue<QGraphicsSceneEvent_Adaptor, QEvent *>(&QGraphicsSceneEvent_Adaptor::cbs_clone_c0_0);
    } else {
      return QGraphicsSceneEvent::clone();
    }
  }

  //  [adaptor impl] void QGraphicsSceneEvent::setAccepted(bool accepted)
  void cbs_setAccepted_864_0(bool accepted)
  {
    QGraphicsSceneEvent::setAccepted(accepted);
  }

  virtual void setAccepted(bool accepted)
  {
    if (cb_setAccepted_864_0.can_issue()) {
      cb_setAccepted_864_0.issue<QGraphicsSceneEvent_Adaptor, bool>(&QGraphicsSceneEvent_Adaptor::cbs_setAccepted_864_0, accepted);
    } else {
      QGraphicsSceneEvent::setAccepted(accepted);
    }
  }

  gsi::Callback cb_clone_c0_0;
  gsi::Callback cb_setAccepted_864_0;
};

QGraphicsSceneEvent_Adaptor::~QGraphicsSceneEvent_Adaptor() { }

//  Constructor QGraphicsSceneEvent::QGraphicsSceneEvent(QEvent::Type type) (adaptor class)

static void _init_ctor_QGraphicsSceneEvent_Adaptor_1565 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("type");
  decl->add_arg<const qt_gsi::Converter<QEvent::Type>::target_type & > (argspec_0);
  decl->set_return_new<QGraphicsSceneEvent_Adaptor> ();
}

static void _call_ctor_QGraphicsSceneEvent_Adaptor_1565 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<QEvent::Type>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<QEvent::Type>::target_type & >() (args, heap);
  ret.write<QGraphicsSceneEvent_Adaptor *> (new QGraphicsSceneEvent_Adaptor (qt_gsi::QtToCppAdaptor<QEvent::Type>(arg1).cref()));
}


// QEvent *QGraphicsSceneEvent::clone()

static void _init_cbs_clone_c0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QEvent * > ();
}

static void _call_cbs_clone_c0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QEvent * > ((QEvent *)((QGraphicsSceneEvent_Adaptor *)cls)->cbs_clone_c0_0 ());
}

static void _set_callback_cbs_clone_c0_0 (void *cls, const gsi::Callback &cb)
{
  ((QGraphicsSceneEvent_Adaptor *)cls)->cb_clone_c0_0 = cb;
}


// void QGraphicsSceneEvent::setAccepted(bool accepted)

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
  ((QGraphicsSceneEvent_Adaptor *)cls)->cbs_setAccepted_864_0 (arg1);
}

static void _set_callback_cbs_setAccepted_864_0 (void *cls, const gsi::Callback &cb)
{
  ((QGraphicsSceneEvent_Adaptor *)cls)->cb_setAccepted_864_0 = cb;
}


namespace gsi
{

gsi::Class<QGraphicsSceneEvent> &qtdecl_QGraphicsSceneEvent ();

static gsi::Methods methods_QGraphicsSceneEvent_Adaptor () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QGraphicsSceneEvent::QGraphicsSceneEvent(QEvent::Type type)\nThis method creates an object of class QGraphicsSceneEvent.", &_init_ctor_QGraphicsSceneEvent_Adaptor_1565, &_call_ctor_QGraphicsSceneEvent_Adaptor_1565);
  methods += new qt_gsi::GenericMethod ("clone", "@brief Virtual method QEvent *QGraphicsSceneEvent::clone()\nThis method can be reimplemented in a derived class.", true, &_init_cbs_clone_c0_0, &_call_cbs_clone_c0_0);
  methods += new qt_gsi::GenericMethod ("clone", "@hide", true, &_init_cbs_clone_c0_0, &_call_cbs_clone_c0_0, &_set_callback_cbs_clone_c0_0);
  methods += new qt_gsi::GenericMethod ("setAccepted", "@brief Virtual method void QGraphicsSceneEvent::setAccepted(bool accepted)\nThis method can be reimplemented in a derived class.", false, &_init_cbs_setAccepted_864_0, &_call_cbs_setAccepted_864_0);
  methods += new qt_gsi::GenericMethod ("setAccepted", "@hide", false, &_init_cbs_setAccepted_864_0, &_call_cbs_setAccepted_864_0, &_set_callback_cbs_setAccepted_864_0);
  return methods;
}

gsi::Class<QGraphicsSceneEvent_Adaptor> decl_QGraphicsSceneEvent_Adaptor (qtdecl_QGraphicsSceneEvent (), "QtWidgets", "QGraphicsSceneEvent",
  methods_QGraphicsSceneEvent_Adaptor (),
  "@qt\n@brief Binding of QGraphicsSceneEvent");

}
