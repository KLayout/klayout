
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
*  @file gsiDeclQGraphicsSceneMouseEvent.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QGraphicsSceneMouseEvent>
#include <QPoint>
#include <QPointF>
#include <QWidget>
#include "gsiQt.h"
#include "gsiQtWidgetsCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QGraphicsSceneMouseEvent

// Qt::MouseButton QGraphicsSceneMouseEvent::button()


static void _init_f_button_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<qt_gsi::Converter<Qt::MouseButton>::target_type > ();
}

static void _call_f_button_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<qt_gsi::Converter<Qt::MouseButton>::target_type > ((qt_gsi::Converter<Qt::MouseButton>::target_type)qt_gsi::CppToQtAdaptor<Qt::MouseButton>(((QGraphicsSceneMouseEvent *)cls)->button ()));
}


// QPointF QGraphicsSceneMouseEvent::buttonDownPos(Qt::MouseButton button)


static void _init_f_buttonDownPos_c1906 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("button");
  decl->add_arg<const qt_gsi::Converter<Qt::MouseButton>::target_type & > (argspec_0);
  decl->set_return<QPointF > ();
}

static void _call_f_buttonDownPos_c1906 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<Qt::MouseButton>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<Qt::MouseButton>::target_type & >() (args, heap);
  ret.write<QPointF > ((QPointF)((QGraphicsSceneMouseEvent *)cls)->buttonDownPos (qt_gsi::QtToCppAdaptor<Qt::MouseButton>(arg1).cref()));
}


// QPointF QGraphicsSceneMouseEvent::buttonDownScenePos(Qt::MouseButton button)


static void _init_f_buttonDownScenePos_c1906 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("button");
  decl->add_arg<const qt_gsi::Converter<Qt::MouseButton>::target_type & > (argspec_0);
  decl->set_return<QPointF > ();
}

static void _call_f_buttonDownScenePos_c1906 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<Qt::MouseButton>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<Qt::MouseButton>::target_type & >() (args, heap);
  ret.write<QPointF > ((QPointF)((QGraphicsSceneMouseEvent *)cls)->buttonDownScenePos (qt_gsi::QtToCppAdaptor<Qt::MouseButton>(arg1).cref()));
}


// QPoint QGraphicsSceneMouseEvent::buttonDownScreenPos(Qt::MouseButton button)


static void _init_f_buttonDownScreenPos_c1906 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("button");
  decl->add_arg<const qt_gsi::Converter<Qt::MouseButton>::target_type & > (argspec_0);
  decl->set_return<QPoint > ();
}

static void _call_f_buttonDownScreenPos_c1906 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<Qt::MouseButton>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<Qt::MouseButton>::target_type & >() (args, heap);
  ret.write<QPoint > ((QPoint)((QGraphicsSceneMouseEvent *)cls)->buttonDownScreenPos (qt_gsi::QtToCppAdaptor<Qt::MouseButton>(arg1).cref()));
}


// QFlags<Qt::MouseButton> QGraphicsSceneMouseEvent::buttons()


static void _init_f_buttons_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QFlags<Qt::MouseButton> > ();
}

static void _call_f_buttons_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QFlags<Qt::MouseButton> > ((QFlags<Qt::MouseButton>)((QGraphicsSceneMouseEvent *)cls)->buttons ());
}


// QFlags<Qt::MouseEventFlag> QGraphicsSceneMouseEvent::flags()


static void _init_f_flags_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QFlags<Qt::MouseEventFlag> > ();
}

static void _call_f_flags_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QFlags<Qt::MouseEventFlag> > ((QFlags<Qt::MouseEventFlag>)((QGraphicsSceneMouseEvent *)cls)->flags ());
}


// QPointF QGraphicsSceneMouseEvent::lastPos()


static void _init_f_lastPos_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QPointF > ();
}

static void _call_f_lastPos_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QPointF > ((QPointF)((QGraphicsSceneMouseEvent *)cls)->lastPos ());
}


// QPointF QGraphicsSceneMouseEvent::lastScenePos()


static void _init_f_lastScenePos_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QPointF > ();
}

static void _call_f_lastScenePos_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QPointF > ((QPointF)((QGraphicsSceneMouseEvent *)cls)->lastScenePos ());
}


// QPoint QGraphicsSceneMouseEvent::lastScreenPos()


static void _init_f_lastScreenPos_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QPoint > ();
}

static void _call_f_lastScreenPos_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QPoint > ((QPoint)((QGraphicsSceneMouseEvent *)cls)->lastScreenPos ());
}


// QFlags<Qt::KeyboardModifier> QGraphicsSceneMouseEvent::modifiers()


static void _init_f_modifiers_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QFlags<Qt::KeyboardModifier> > ();
}

static void _call_f_modifiers_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QFlags<Qt::KeyboardModifier> > ((QFlags<Qt::KeyboardModifier>)((QGraphicsSceneMouseEvent *)cls)->modifiers ());
}


// QPointF QGraphicsSceneMouseEvent::pos()


static void _init_f_pos_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QPointF > ();
}

static void _call_f_pos_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QPointF > ((QPointF)((QGraphicsSceneMouseEvent *)cls)->pos ());
}


// QPointF QGraphicsSceneMouseEvent::scenePos()


static void _init_f_scenePos_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QPointF > ();
}

static void _call_f_scenePos_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QPointF > ((QPointF)((QGraphicsSceneMouseEvent *)cls)->scenePos ());
}


// QPoint QGraphicsSceneMouseEvent::screenPos()


static void _init_f_screenPos_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QPoint > ();
}

static void _call_f_screenPos_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QPoint > ((QPoint)((QGraphicsSceneMouseEvent *)cls)->screenPos ());
}


// void QGraphicsSceneMouseEvent::setButton(Qt::MouseButton button)


static void _init_f_setButton_1906 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("button");
  decl->add_arg<const qt_gsi::Converter<Qt::MouseButton>::target_type & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setButton_1906 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<Qt::MouseButton>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<Qt::MouseButton>::target_type & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setButton (qt_gsi::QtToCppAdaptor<Qt::MouseButton>(arg1).cref());
}


// void QGraphicsSceneMouseEvent::setButtonDownPos(Qt::MouseButton button, const QPointF &pos)


static void _init_f_setButtonDownPos_3784 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("button");
  decl->add_arg<const qt_gsi::Converter<Qt::MouseButton>::target_type & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("pos");
  decl->add_arg<const QPointF & > (argspec_1);
  decl->set_return<void > ();
}

static void _call_f_setButtonDownPos_3784 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<Qt::MouseButton>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<Qt::MouseButton>::target_type & >() (args, heap);
  const QPointF &arg2 = gsi::arg_reader<const QPointF & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setButtonDownPos (qt_gsi::QtToCppAdaptor<Qt::MouseButton>(arg1).cref(), arg2);
}


// void QGraphicsSceneMouseEvent::setButtonDownScenePos(Qt::MouseButton button, const QPointF &pos)


static void _init_f_setButtonDownScenePos_3784 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("button");
  decl->add_arg<const qt_gsi::Converter<Qt::MouseButton>::target_type & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("pos");
  decl->add_arg<const QPointF & > (argspec_1);
  decl->set_return<void > ();
}

static void _call_f_setButtonDownScenePos_3784 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<Qt::MouseButton>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<Qt::MouseButton>::target_type & >() (args, heap);
  const QPointF &arg2 = gsi::arg_reader<const QPointF & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setButtonDownScenePos (qt_gsi::QtToCppAdaptor<Qt::MouseButton>(arg1).cref(), arg2);
}


// void QGraphicsSceneMouseEvent::setButtonDownScreenPos(Qt::MouseButton button, const QPoint &pos)


static void _init_f_setButtonDownScreenPos_3714 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("button");
  decl->add_arg<const qt_gsi::Converter<Qt::MouseButton>::target_type & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("pos");
  decl->add_arg<const QPoint & > (argspec_1);
  decl->set_return<void > ();
}

static void _call_f_setButtonDownScreenPos_3714 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<Qt::MouseButton>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<Qt::MouseButton>::target_type & >() (args, heap);
  const QPoint &arg2 = gsi::arg_reader<const QPoint & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setButtonDownScreenPos (qt_gsi::QtToCppAdaptor<Qt::MouseButton>(arg1).cref(), arg2);
}


// void QGraphicsSceneMouseEvent::setButtons(QFlags<Qt::MouseButton> buttons)


static void _init_f_setButtons_2602 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("buttons");
  decl->add_arg<QFlags<Qt::MouseButton> > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setButtons_2602 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QFlags<Qt::MouseButton> arg1 = gsi::arg_reader<QFlags<Qt::MouseButton> >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setButtons (arg1);
}


// void QGraphicsSceneMouseEvent::setFlags(QFlags<Qt::MouseEventFlag>)


static void _init_f_setFlags_2858 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<QFlags<Qt::MouseEventFlag> > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setFlags_2858 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QFlags<Qt::MouseEventFlag> arg1 = gsi::arg_reader<QFlags<Qt::MouseEventFlag> >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setFlags (arg1);
}


// void QGraphicsSceneMouseEvent::setLastPos(const QPointF &pos)


static void _init_f_setLastPos_1986 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("pos");
  decl->add_arg<const QPointF & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setLastPos_1986 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QPointF &arg1 = gsi::arg_reader<const QPointF & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setLastPos (arg1);
}


// void QGraphicsSceneMouseEvent::setLastScenePos(const QPointF &pos)


static void _init_f_setLastScenePos_1986 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("pos");
  decl->add_arg<const QPointF & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setLastScenePos_1986 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QPointF &arg1 = gsi::arg_reader<const QPointF & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setLastScenePos (arg1);
}


// void QGraphicsSceneMouseEvent::setLastScreenPos(const QPoint &pos)


static void _init_f_setLastScreenPos_1916 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("pos");
  decl->add_arg<const QPoint & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setLastScreenPos_1916 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QPoint &arg1 = gsi::arg_reader<const QPoint & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setLastScreenPos (arg1);
}


// void QGraphicsSceneMouseEvent::setModifiers(QFlags<Qt::KeyboardModifier> modifiers)


static void _init_f_setModifiers_3077 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("modifiers");
  decl->add_arg<QFlags<Qt::KeyboardModifier> > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setModifiers_3077 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QFlags<Qt::KeyboardModifier> arg1 = gsi::arg_reader<QFlags<Qt::KeyboardModifier> >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setModifiers (arg1);
}


// void QGraphicsSceneMouseEvent::setPos(const QPointF &pos)


static void _init_f_setPos_1986 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("pos");
  decl->add_arg<const QPointF & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setPos_1986 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QPointF &arg1 = gsi::arg_reader<const QPointF & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setPos (arg1);
}


// void QGraphicsSceneMouseEvent::setScenePos(const QPointF &pos)


static void _init_f_setScenePos_1986 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("pos");
  decl->add_arg<const QPointF & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setScenePos_1986 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QPointF &arg1 = gsi::arg_reader<const QPointF & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setScenePos (arg1);
}


// void QGraphicsSceneMouseEvent::setScreenPos(const QPoint &pos)


static void _init_f_setScreenPos_1916 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("pos");
  decl->add_arg<const QPoint & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setScreenPos_1916 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QPoint &arg1 = gsi::arg_reader<const QPoint & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setScreenPos (arg1);
}


// void QGraphicsSceneMouseEvent::setSource(Qt::MouseEventSource source)


static void _init_f_setSource_2409 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("source");
  decl->add_arg<const qt_gsi::Converter<Qt::MouseEventSource>::target_type & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setSource_2409 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<Qt::MouseEventSource>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<Qt::MouseEventSource>::target_type & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneMouseEvent *)cls)->setSource (qt_gsi::QtToCppAdaptor<Qt::MouseEventSource>(arg1).cref());
}


// Qt::MouseEventSource QGraphicsSceneMouseEvent::source()


static void _init_f_source_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<qt_gsi::Converter<Qt::MouseEventSource>::target_type > ();
}

static void _call_f_source_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<qt_gsi::Converter<Qt::MouseEventSource>::target_type > ((qt_gsi::Converter<Qt::MouseEventSource>::target_type)qt_gsi::CppToQtAdaptor<Qt::MouseEventSource>(((QGraphicsSceneMouseEvent *)cls)->source ()));
}


namespace gsi
{

static gsi::Methods methods_QGraphicsSceneMouseEvent () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericMethod (":button", "@brief Method Qt::MouseButton QGraphicsSceneMouseEvent::button()\n", true, &_init_f_button_c0, &_call_f_button_c0);
  methods += new qt_gsi::GenericMethod ("buttonDownPos", "@brief Method QPointF QGraphicsSceneMouseEvent::buttonDownPos(Qt::MouseButton button)\n", true, &_init_f_buttonDownPos_c1906, &_call_f_buttonDownPos_c1906);
  methods += new qt_gsi::GenericMethod ("buttonDownScenePos", "@brief Method QPointF QGraphicsSceneMouseEvent::buttonDownScenePos(Qt::MouseButton button)\n", true, &_init_f_buttonDownScenePos_c1906, &_call_f_buttonDownScenePos_c1906);
  methods += new qt_gsi::GenericMethod ("buttonDownScreenPos", "@brief Method QPoint QGraphicsSceneMouseEvent::buttonDownScreenPos(Qt::MouseButton button)\n", true, &_init_f_buttonDownScreenPos_c1906, &_call_f_buttonDownScreenPos_c1906);
  methods += new qt_gsi::GenericMethod (":buttons", "@brief Method QFlags<Qt::MouseButton> QGraphicsSceneMouseEvent::buttons()\n", true, &_init_f_buttons_c0, &_call_f_buttons_c0);
  methods += new qt_gsi::GenericMethod (":flags", "@brief Method QFlags<Qt::MouseEventFlag> QGraphicsSceneMouseEvent::flags()\n", true, &_init_f_flags_c0, &_call_f_flags_c0);
  methods += new qt_gsi::GenericMethod (":lastPos", "@brief Method QPointF QGraphicsSceneMouseEvent::lastPos()\n", true, &_init_f_lastPos_c0, &_call_f_lastPos_c0);
  methods += new qt_gsi::GenericMethod (":lastScenePos", "@brief Method QPointF QGraphicsSceneMouseEvent::lastScenePos()\n", true, &_init_f_lastScenePos_c0, &_call_f_lastScenePos_c0);
  methods += new qt_gsi::GenericMethod (":lastScreenPos", "@brief Method QPoint QGraphicsSceneMouseEvent::lastScreenPos()\n", true, &_init_f_lastScreenPos_c0, &_call_f_lastScreenPos_c0);
  methods += new qt_gsi::GenericMethod (":modifiers", "@brief Method QFlags<Qt::KeyboardModifier> QGraphicsSceneMouseEvent::modifiers()\n", true, &_init_f_modifiers_c0, &_call_f_modifiers_c0);
  methods += new qt_gsi::GenericMethod (":pos", "@brief Method QPointF QGraphicsSceneMouseEvent::pos()\n", true, &_init_f_pos_c0, &_call_f_pos_c0);
  methods += new qt_gsi::GenericMethod (":scenePos", "@brief Method QPointF QGraphicsSceneMouseEvent::scenePos()\n", true, &_init_f_scenePos_c0, &_call_f_scenePos_c0);
  methods += new qt_gsi::GenericMethod (":screenPos", "@brief Method QPoint QGraphicsSceneMouseEvent::screenPos()\n", true, &_init_f_screenPos_c0, &_call_f_screenPos_c0);
  methods += new qt_gsi::GenericMethod ("setButton|button=", "@brief Method void QGraphicsSceneMouseEvent::setButton(Qt::MouseButton button)\n", false, &_init_f_setButton_1906, &_call_f_setButton_1906);
  methods += new qt_gsi::GenericMethod ("setButtonDownPos", "@brief Method void QGraphicsSceneMouseEvent::setButtonDownPos(Qt::MouseButton button, const QPointF &pos)\n", false, &_init_f_setButtonDownPos_3784, &_call_f_setButtonDownPos_3784);
  methods += new qt_gsi::GenericMethod ("setButtonDownScenePos", "@brief Method void QGraphicsSceneMouseEvent::setButtonDownScenePos(Qt::MouseButton button, const QPointF &pos)\n", false, &_init_f_setButtonDownScenePos_3784, &_call_f_setButtonDownScenePos_3784);
  methods += new qt_gsi::GenericMethod ("setButtonDownScreenPos", "@brief Method void QGraphicsSceneMouseEvent::setButtonDownScreenPos(Qt::MouseButton button, const QPoint &pos)\n", false, &_init_f_setButtonDownScreenPos_3714, &_call_f_setButtonDownScreenPos_3714);
  methods += new qt_gsi::GenericMethod ("setButtons|buttons=", "@brief Method void QGraphicsSceneMouseEvent::setButtons(QFlags<Qt::MouseButton> buttons)\n", false, &_init_f_setButtons_2602, &_call_f_setButtons_2602);
  methods += new qt_gsi::GenericMethod ("setFlags|flags=", "@brief Method void QGraphicsSceneMouseEvent::setFlags(QFlags<Qt::MouseEventFlag>)\n", false, &_init_f_setFlags_2858, &_call_f_setFlags_2858);
  methods += new qt_gsi::GenericMethod ("setLastPos|lastPos=", "@brief Method void QGraphicsSceneMouseEvent::setLastPos(const QPointF &pos)\n", false, &_init_f_setLastPos_1986, &_call_f_setLastPos_1986);
  methods += new qt_gsi::GenericMethod ("setLastScenePos|lastScenePos=", "@brief Method void QGraphicsSceneMouseEvent::setLastScenePos(const QPointF &pos)\n", false, &_init_f_setLastScenePos_1986, &_call_f_setLastScenePos_1986);
  methods += new qt_gsi::GenericMethod ("setLastScreenPos|lastScreenPos=", "@brief Method void QGraphicsSceneMouseEvent::setLastScreenPos(const QPoint &pos)\n", false, &_init_f_setLastScreenPos_1916, &_call_f_setLastScreenPos_1916);
  methods += new qt_gsi::GenericMethod ("setModifiers|modifiers=", "@brief Method void QGraphicsSceneMouseEvent::setModifiers(QFlags<Qt::KeyboardModifier> modifiers)\n", false, &_init_f_setModifiers_3077, &_call_f_setModifiers_3077);
  methods += new qt_gsi::GenericMethod ("setPos|pos=", "@brief Method void QGraphicsSceneMouseEvent::setPos(const QPointF &pos)\n", false, &_init_f_setPos_1986, &_call_f_setPos_1986);
  methods += new qt_gsi::GenericMethod ("setScenePos|scenePos=", "@brief Method void QGraphicsSceneMouseEvent::setScenePos(const QPointF &pos)\n", false, &_init_f_setScenePos_1986, &_call_f_setScenePos_1986);
  methods += new qt_gsi::GenericMethod ("setScreenPos|screenPos=", "@brief Method void QGraphicsSceneMouseEvent::setScreenPos(const QPoint &pos)\n", false, &_init_f_setScreenPos_1916, &_call_f_setScreenPos_1916);
  methods += new qt_gsi::GenericMethod ("setSource|source=", "@brief Method void QGraphicsSceneMouseEvent::setSource(Qt::MouseEventSource source)\n", false, &_init_f_setSource_2409, &_call_f_setSource_2409);
  methods += new qt_gsi::GenericMethod (":source", "@brief Method Qt::MouseEventSource QGraphicsSceneMouseEvent::source()\n", true, &_init_f_source_c0, &_call_f_source_c0);
  return methods;
}

gsi::Class<QGraphicsSceneEvent> &qtdecl_QGraphicsSceneEvent ();

gsi::Class<QGraphicsSceneMouseEvent> decl_QGraphicsSceneMouseEvent (qtdecl_QGraphicsSceneEvent (), "QtWidgets", "QGraphicsSceneMouseEvent_Native",
  methods_QGraphicsSceneMouseEvent (),
  "@hide\n@alias QGraphicsSceneMouseEvent");

GSI_QTWIDGETS_PUBLIC gsi::Class<QGraphicsSceneMouseEvent> &qtdecl_QGraphicsSceneMouseEvent () { return decl_QGraphicsSceneMouseEvent; }

}


class QGraphicsSceneMouseEvent_Adaptor : public QGraphicsSceneMouseEvent, public qt_gsi::QtObjectBase
{
public:

  virtual ~QGraphicsSceneMouseEvent_Adaptor();

  //  [adaptor ctor] QGraphicsSceneMouseEvent::QGraphicsSceneMouseEvent(QEvent::Type type)
  QGraphicsSceneMouseEvent_Adaptor() : QGraphicsSceneMouseEvent()
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [adaptor ctor] QGraphicsSceneMouseEvent::QGraphicsSceneMouseEvent(QEvent::Type type)
  QGraphicsSceneMouseEvent_Adaptor(QEvent::Type type) : QGraphicsSceneMouseEvent(type)
  {
    qt_gsi::QtObjectBase::init (this);
  }

  
};

QGraphicsSceneMouseEvent_Adaptor::~QGraphicsSceneMouseEvent_Adaptor() { }

//  Constructor QGraphicsSceneMouseEvent::QGraphicsSceneMouseEvent(QEvent::Type type) (adaptor class)

static void _init_ctor_QGraphicsSceneMouseEvent_Adaptor_1565 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("type", true, "QEvent::None");
  decl->add_arg<const qt_gsi::Converter<QEvent::Type>::target_type & > (argspec_0);
  decl->set_return_new<QGraphicsSceneMouseEvent_Adaptor> ();
}

static void _call_ctor_QGraphicsSceneMouseEvent_Adaptor_1565 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<QEvent::Type>::target_type & arg1 = args ? gsi::arg_reader<const qt_gsi::Converter<QEvent::Type>::target_type & >() (args, heap) : gsi::arg_maker<const qt_gsi::Converter<QEvent::Type>::target_type & >() (qt_gsi::CppToQtReadAdaptor<QEvent::Type>(heap, QEvent::None), heap);
  ret.write<QGraphicsSceneMouseEvent_Adaptor *> (new QGraphicsSceneMouseEvent_Adaptor (qt_gsi::QtToCppAdaptor<QEvent::Type>(arg1).cref()));
}


namespace gsi
{

gsi::Class<QGraphicsSceneMouseEvent> &qtdecl_QGraphicsSceneMouseEvent ();

static gsi::Methods methods_QGraphicsSceneMouseEvent_Adaptor () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QGraphicsSceneMouseEvent::QGraphicsSceneMouseEvent(QEvent::Type type)\nThis method creates an object of class QGraphicsSceneMouseEvent.", &_init_ctor_QGraphicsSceneMouseEvent_Adaptor_1565, &_call_ctor_QGraphicsSceneMouseEvent_Adaptor_1565);
  return methods;
}

gsi::Class<QGraphicsSceneMouseEvent_Adaptor> decl_QGraphicsSceneMouseEvent_Adaptor (qtdecl_QGraphicsSceneMouseEvent (), "QtWidgets", "QGraphicsSceneMouseEvent",
  methods_QGraphicsSceneMouseEvent_Adaptor (),
  "@qt\n@brief Binding of QGraphicsSceneMouseEvent");

}

