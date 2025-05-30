
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
*  @file gsiDeclQGraphicsSceneResizeEvent.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QGraphicsSceneResizeEvent>
#include <QSizeF>
#include <QWidget>
#include "gsiQt.h"
#include "gsiQtGuiCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QGraphicsSceneResizeEvent

// QSizeF QGraphicsSceneResizeEvent::newSize()


static void _init_f_newSize_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QSizeF > ();
}

static void _call_f_newSize_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QSizeF > ((QSizeF)((QGraphicsSceneResizeEvent *)cls)->newSize ());
}


// QSizeF QGraphicsSceneResizeEvent::oldSize()


static void _init_f_oldSize_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QSizeF > ();
}

static void _call_f_oldSize_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QSizeF > ((QSizeF)((QGraphicsSceneResizeEvent *)cls)->oldSize ());
}


// void QGraphicsSceneResizeEvent::setNewSize(const QSizeF &size)


static void _init_f_setNewSize_1875 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("size");
  decl->add_arg<const QSizeF & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setNewSize_1875 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QSizeF &arg1 = gsi::arg_reader<const QSizeF & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneResizeEvent *)cls)->setNewSize (arg1);
}


// void QGraphicsSceneResizeEvent::setOldSize(const QSizeF &size)


static void _init_f_setOldSize_1875 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("size");
  decl->add_arg<const QSizeF & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setOldSize_1875 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QSizeF &arg1 = gsi::arg_reader<const QSizeF & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneResizeEvent *)cls)->setOldSize (arg1);
}


namespace gsi
{

static gsi::Methods methods_QGraphicsSceneResizeEvent () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericMethod (":newSize", "@brief Method QSizeF QGraphicsSceneResizeEvent::newSize()\n", true, &_init_f_newSize_c0, &_call_f_newSize_c0);
  methods += new qt_gsi::GenericMethod (":oldSize", "@brief Method QSizeF QGraphicsSceneResizeEvent::oldSize()\n", true, &_init_f_oldSize_c0, &_call_f_oldSize_c0);
  methods += new qt_gsi::GenericMethod ("setNewSize|newSize=", "@brief Method void QGraphicsSceneResizeEvent::setNewSize(const QSizeF &size)\n", false, &_init_f_setNewSize_1875, &_call_f_setNewSize_1875);
  methods += new qt_gsi::GenericMethod ("setOldSize|oldSize=", "@brief Method void QGraphicsSceneResizeEvent::setOldSize(const QSizeF &size)\n", false, &_init_f_setOldSize_1875, &_call_f_setOldSize_1875);
  return methods;
}

gsi::Class<QGraphicsSceneEvent> &qtdecl_QGraphicsSceneEvent ();

gsi::Class<QGraphicsSceneResizeEvent> decl_QGraphicsSceneResizeEvent (qtdecl_QGraphicsSceneEvent (), "QtGui", "QGraphicsSceneResizeEvent_Native",
  methods_QGraphicsSceneResizeEvent (),
  "@hide\n@alias QGraphicsSceneResizeEvent");

GSI_QTGUI_PUBLIC gsi::Class<QGraphicsSceneResizeEvent> &qtdecl_QGraphicsSceneResizeEvent () { return decl_QGraphicsSceneResizeEvent; }

}


class QGraphicsSceneResizeEvent_Adaptor : public QGraphicsSceneResizeEvent, public qt_gsi::QtObjectBase
{
public:

  virtual ~QGraphicsSceneResizeEvent_Adaptor();

  //  [adaptor ctor] QGraphicsSceneResizeEvent::QGraphicsSceneResizeEvent()
  QGraphicsSceneResizeEvent_Adaptor() : QGraphicsSceneResizeEvent()
  {
    qt_gsi::QtObjectBase::init (this);
  }

  
};

QGraphicsSceneResizeEvent_Adaptor::~QGraphicsSceneResizeEvent_Adaptor() { }

//  Constructor QGraphicsSceneResizeEvent::QGraphicsSceneResizeEvent() (adaptor class)

static void _init_ctor_QGraphicsSceneResizeEvent_Adaptor_0 (qt_gsi::GenericStaticMethod *decl)
{
  decl->set_return_new<QGraphicsSceneResizeEvent_Adaptor> ();
}

static void _call_ctor_QGraphicsSceneResizeEvent_Adaptor_0 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QGraphicsSceneResizeEvent_Adaptor *> (new QGraphicsSceneResizeEvent_Adaptor ());
}


namespace gsi
{

gsi::Class<QGraphicsSceneResizeEvent> &qtdecl_QGraphicsSceneResizeEvent ();

static gsi::Methods methods_QGraphicsSceneResizeEvent_Adaptor () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QGraphicsSceneResizeEvent::QGraphicsSceneResizeEvent()\nThis method creates an object of class QGraphicsSceneResizeEvent.", &_init_ctor_QGraphicsSceneResizeEvent_Adaptor_0, &_call_ctor_QGraphicsSceneResizeEvent_Adaptor_0);
  return methods;
}

gsi::Class<QGraphicsSceneResizeEvent_Adaptor> decl_QGraphicsSceneResizeEvent_Adaptor (qtdecl_QGraphicsSceneResizeEvent (), "QtGui", "QGraphicsSceneResizeEvent",
  methods_QGraphicsSceneResizeEvent_Adaptor (),
  "@qt\n@brief Binding of QGraphicsSceneResizeEvent");

}

