
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
*  @file gsiDeclQGraphicsSceneDragDropEvent.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QGraphicsSceneDragDropEvent>
#include <QEvent>
#include <QMimeData>
#include <QPoint>
#include <QPointF>
#include <QWidget>
#include "gsiQt.h"
#include "gsiQtWidgetsCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QGraphicsSceneDragDropEvent

// void QGraphicsSceneDragDropEvent::acceptProposedAction()


static void _init_f_acceptProposedAction_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<void > ();
}

static void _call_f_acceptProposedAction_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneDragDropEvent *)cls)->acceptProposedAction ();
}


// QFlags<Qt::MouseButton> QGraphicsSceneDragDropEvent::buttons()


static void _init_f_buttons_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QFlags<Qt::MouseButton> > ();
}

static void _call_f_buttons_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QFlags<Qt::MouseButton> > ((QFlags<Qt::MouseButton>)((QGraphicsSceneDragDropEvent *)cls)->buttons ());
}


// Qt::DropAction QGraphicsSceneDragDropEvent::dropAction()


static void _init_f_dropAction_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<qt_gsi::Converter<Qt::DropAction>::target_type > ();
}

static void _call_f_dropAction_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<qt_gsi::Converter<Qt::DropAction>::target_type > ((qt_gsi::Converter<Qt::DropAction>::target_type)qt_gsi::CppToQtAdaptor<Qt::DropAction>(((QGraphicsSceneDragDropEvent *)cls)->dropAction ()));
}


// const QMimeData *QGraphicsSceneDragDropEvent::mimeData()


static void _init_f_mimeData_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<const QMimeData * > ();
}

static void _call_f_mimeData_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<const QMimeData * > ((const QMimeData *)((QGraphicsSceneDragDropEvent *)cls)->mimeData ());
}


// QFlags<Qt::KeyboardModifier> QGraphicsSceneDragDropEvent::modifiers()


static void _init_f_modifiers_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QFlags<Qt::KeyboardModifier> > ();
}

static void _call_f_modifiers_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QFlags<Qt::KeyboardModifier> > ((QFlags<Qt::KeyboardModifier>)((QGraphicsSceneDragDropEvent *)cls)->modifiers ());
}


// QPointF QGraphicsSceneDragDropEvent::pos()


static void _init_f_pos_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QPointF > ();
}

static void _call_f_pos_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QPointF > ((QPointF)((QGraphicsSceneDragDropEvent *)cls)->pos ());
}


// QFlags<Qt::DropAction> QGraphicsSceneDragDropEvent::possibleActions()


static void _init_f_possibleActions_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QFlags<Qt::DropAction> > ();
}

static void _call_f_possibleActions_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QFlags<Qt::DropAction> > ((QFlags<Qt::DropAction>)((QGraphicsSceneDragDropEvent *)cls)->possibleActions ());
}


// Qt::DropAction QGraphicsSceneDragDropEvent::proposedAction()


static void _init_f_proposedAction_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<qt_gsi::Converter<Qt::DropAction>::target_type > ();
}

static void _call_f_proposedAction_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<qt_gsi::Converter<Qt::DropAction>::target_type > ((qt_gsi::Converter<Qt::DropAction>::target_type)qt_gsi::CppToQtAdaptor<Qt::DropAction>(((QGraphicsSceneDragDropEvent *)cls)->proposedAction ()));
}


// QPointF QGraphicsSceneDragDropEvent::scenePos()


static void _init_f_scenePos_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QPointF > ();
}

static void _call_f_scenePos_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QPointF > ((QPointF)((QGraphicsSceneDragDropEvent *)cls)->scenePos ());
}


// QPoint QGraphicsSceneDragDropEvent::screenPos()


static void _init_f_screenPos_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QPoint > ();
}

static void _call_f_screenPos_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QPoint > ((QPoint)((QGraphicsSceneDragDropEvent *)cls)->screenPos ());
}


// void QGraphicsSceneDragDropEvent::setButtons(QFlags<Qt::MouseButton> buttons)


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
  ((QGraphicsSceneDragDropEvent *)cls)->setButtons (arg1);
}


// void QGraphicsSceneDragDropEvent::setDropAction(Qt::DropAction action)


static void _init_f_setDropAction_1760 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("action");
  decl->add_arg<const qt_gsi::Converter<Qt::DropAction>::target_type & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setDropAction_1760 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<Qt::DropAction>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<Qt::DropAction>::target_type & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneDragDropEvent *)cls)->setDropAction (qt_gsi::QtToCppAdaptor<Qt::DropAction>(arg1).cref());
}


// void QGraphicsSceneDragDropEvent::setMimeData(const QMimeData *data)


static void _init_f_setMimeData_2168 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("data");
  decl->add_arg<const QMimeData * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setMimeData_2168 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QMimeData *arg1 = gsi::arg_reader<const QMimeData * >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneDragDropEvent *)cls)->setMimeData (arg1);
}


// void QGraphicsSceneDragDropEvent::setModifiers(QFlags<Qt::KeyboardModifier> modifiers)


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
  ((QGraphicsSceneDragDropEvent *)cls)->setModifiers (arg1);
}


// void QGraphicsSceneDragDropEvent::setPos(const QPointF &pos)


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
  ((QGraphicsSceneDragDropEvent *)cls)->setPos (arg1);
}


// void QGraphicsSceneDragDropEvent::setPossibleActions(QFlags<Qt::DropAction> actions)


static void _init_f_setPossibleActions_2456 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("actions");
  decl->add_arg<QFlags<Qt::DropAction> > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setPossibleActions_2456 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QFlags<Qt::DropAction> arg1 = gsi::arg_reader<QFlags<Qt::DropAction> >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneDragDropEvent *)cls)->setPossibleActions (arg1);
}


// void QGraphicsSceneDragDropEvent::setProposedAction(Qt::DropAction action)


static void _init_f_setProposedAction_1760 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("action");
  decl->add_arg<const qt_gsi::Converter<Qt::DropAction>::target_type & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setProposedAction_1760 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<Qt::DropAction>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<Qt::DropAction>::target_type & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneDragDropEvent *)cls)->setProposedAction (qt_gsi::QtToCppAdaptor<Qt::DropAction>(arg1).cref());
}


// void QGraphicsSceneDragDropEvent::setScenePos(const QPointF &pos)


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
  ((QGraphicsSceneDragDropEvent *)cls)->setScenePos (arg1);
}


// void QGraphicsSceneDragDropEvent::setScreenPos(const QPoint &pos)


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
  ((QGraphicsSceneDragDropEvent *)cls)->setScreenPos (arg1);
}


// void QGraphicsSceneDragDropEvent::setSource(QWidget *source)


static void _init_f_setSource_1315 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("source");
  decl->add_arg<QWidget * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setSource_1315 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QWidget *arg1 = gsi::arg_reader<QWidget * >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QGraphicsSceneDragDropEvent *)cls)->setSource (arg1);
}


// QWidget *QGraphicsSceneDragDropEvent::source()


static void _init_f_source_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QWidget * > ();
}

static void _call_f_source_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QWidget * > ((QWidget *)((QGraphicsSceneDragDropEvent *)cls)->source ());
}


namespace gsi
{

static gsi::Methods methods_QGraphicsSceneDragDropEvent () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericMethod ("acceptProposedAction", "@brief Method void QGraphicsSceneDragDropEvent::acceptProposedAction()\n", false, &_init_f_acceptProposedAction_0, &_call_f_acceptProposedAction_0);
  methods += new qt_gsi::GenericMethod (":buttons", "@brief Method QFlags<Qt::MouseButton> QGraphicsSceneDragDropEvent::buttons()\n", true, &_init_f_buttons_c0, &_call_f_buttons_c0);
  methods += new qt_gsi::GenericMethod (":dropAction", "@brief Method Qt::DropAction QGraphicsSceneDragDropEvent::dropAction()\n", true, &_init_f_dropAction_c0, &_call_f_dropAction_c0);
  methods += new qt_gsi::GenericMethod (":mimeData", "@brief Method const QMimeData *QGraphicsSceneDragDropEvent::mimeData()\n", true, &_init_f_mimeData_c0, &_call_f_mimeData_c0);
  methods += new qt_gsi::GenericMethod (":modifiers", "@brief Method QFlags<Qt::KeyboardModifier> QGraphicsSceneDragDropEvent::modifiers()\n", true, &_init_f_modifiers_c0, &_call_f_modifiers_c0);
  methods += new qt_gsi::GenericMethod (":pos", "@brief Method QPointF QGraphicsSceneDragDropEvent::pos()\n", true, &_init_f_pos_c0, &_call_f_pos_c0);
  methods += new qt_gsi::GenericMethod (":possibleActions", "@brief Method QFlags<Qt::DropAction> QGraphicsSceneDragDropEvent::possibleActions()\n", true, &_init_f_possibleActions_c0, &_call_f_possibleActions_c0);
  methods += new qt_gsi::GenericMethod (":proposedAction", "@brief Method Qt::DropAction QGraphicsSceneDragDropEvent::proposedAction()\n", true, &_init_f_proposedAction_c0, &_call_f_proposedAction_c0);
  methods += new qt_gsi::GenericMethod (":scenePos", "@brief Method QPointF QGraphicsSceneDragDropEvent::scenePos()\n", true, &_init_f_scenePos_c0, &_call_f_scenePos_c0);
  methods += new qt_gsi::GenericMethod (":screenPos", "@brief Method QPoint QGraphicsSceneDragDropEvent::screenPos()\n", true, &_init_f_screenPos_c0, &_call_f_screenPos_c0);
  methods += new qt_gsi::GenericMethod ("setButtons|buttons=", "@brief Method void QGraphicsSceneDragDropEvent::setButtons(QFlags<Qt::MouseButton> buttons)\n", false, &_init_f_setButtons_2602, &_call_f_setButtons_2602);
  methods += new qt_gsi::GenericMethod ("setDropAction|dropAction=", "@brief Method void QGraphicsSceneDragDropEvent::setDropAction(Qt::DropAction action)\n", false, &_init_f_setDropAction_1760, &_call_f_setDropAction_1760);
  methods += new qt_gsi::GenericMethod ("setMimeData|mimeData=", "@brief Method void QGraphicsSceneDragDropEvent::setMimeData(const QMimeData *data)\n", false, &_init_f_setMimeData_2168, &_call_f_setMimeData_2168);
  methods += new qt_gsi::GenericMethod ("setModifiers|modifiers=", "@brief Method void QGraphicsSceneDragDropEvent::setModifiers(QFlags<Qt::KeyboardModifier> modifiers)\n", false, &_init_f_setModifiers_3077, &_call_f_setModifiers_3077);
  methods += new qt_gsi::GenericMethod ("setPos|pos=", "@brief Method void QGraphicsSceneDragDropEvent::setPos(const QPointF &pos)\n", false, &_init_f_setPos_1986, &_call_f_setPos_1986);
  methods += new qt_gsi::GenericMethod ("setPossibleActions|possibleActions=", "@brief Method void QGraphicsSceneDragDropEvent::setPossibleActions(QFlags<Qt::DropAction> actions)\n", false, &_init_f_setPossibleActions_2456, &_call_f_setPossibleActions_2456);
  methods += new qt_gsi::GenericMethod ("setProposedAction|proposedAction=", "@brief Method void QGraphicsSceneDragDropEvent::setProposedAction(Qt::DropAction action)\n", false, &_init_f_setProposedAction_1760, &_call_f_setProposedAction_1760);
  methods += new qt_gsi::GenericMethod ("setScenePos|scenePos=", "@brief Method void QGraphicsSceneDragDropEvent::setScenePos(const QPointF &pos)\n", false, &_init_f_setScenePos_1986, &_call_f_setScenePos_1986);
  methods += new qt_gsi::GenericMethod ("setScreenPos|screenPos=", "@brief Method void QGraphicsSceneDragDropEvent::setScreenPos(const QPoint &pos)\n", false, &_init_f_setScreenPos_1916, &_call_f_setScreenPos_1916);
  methods += new qt_gsi::GenericMethod ("setSource|source=", "@brief Method void QGraphicsSceneDragDropEvent::setSource(QWidget *source)\n", false, &_init_f_setSource_1315, &_call_f_setSource_1315);
  methods += new qt_gsi::GenericMethod (":source", "@brief Method QWidget *QGraphicsSceneDragDropEvent::source()\n", true, &_init_f_source_c0, &_call_f_source_c0);
  return methods;
}

gsi::Class<QGraphicsSceneEvent> &qtdecl_QGraphicsSceneEvent ();

gsi::Class<QGraphicsSceneDragDropEvent> decl_QGraphicsSceneDragDropEvent (qtdecl_QGraphicsSceneEvent (), "QtWidgets", "QGraphicsSceneDragDropEvent_Native",
  methods_QGraphicsSceneDragDropEvent (),
  "@hide\n@alias QGraphicsSceneDragDropEvent");

GSI_QTWIDGETS_PUBLIC gsi::Class<QGraphicsSceneDragDropEvent> &qtdecl_QGraphicsSceneDragDropEvent () { return decl_QGraphicsSceneDragDropEvent; }

}


class QGraphicsSceneDragDropEvent_Adaptor : public QGraphicsSceneDragDropEvent, public qt_gsi::QtObjectBase
{
public:

  virtual ~QGraphicsSceneDragDropEvent_Adaptor();

  //  [adaptor ctor] QGraphicsSceneDragDropEvent::QGraphicsSceneDragDropEvent(QEvent::Type type)
  QGraphicsSceneDragDropEvent_Adaptor() : QGraphicsSceneDragDropEvent()
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [adaptor ctor] QGraphicsSceneDragDropEvent::QGraphicsSceneDragDropEvent(QEvent::Type type)
  QGraphicsSceneDragDropEvent_Adaptor(QEvent::Type type) : QGraphicsSceneDragDropEvent(type)
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [adaptor impl] QEvent *QGraphicsSceneDragDropEvent::clone()
  QEvent * cbs_clone_c0_0() const
  {
    return QGraphicsSceneDragDropEvent::clone();
  }

  virtual QEvent * clone() const
  {
    if (cb_clone_c0_0.can_issue()) {
      return cb_clone_c0_0.issue<QGraphicsSceneDragDropEvent_Adaptor, QEvent *>(&QGraphicsSceneDragDropEvent_Adaptor::cbs_clone_c0_0);
    } else {
      return QGraphicsSceneDragDropEvent::clone();
    }
  }

  //  [adaptor impl] void QGraphicsSceneDragDropEvent::setAccepted(bool accepted)
  void cbs_setAccepted_864_0(bool accepted)
  {
    QGraphicsSceneDragDropEvent::setAccepted(accepted);
  }

  virtual void setAccepted(bool accepted)
  {
    if (cb_setAccepted_864_0.can_issue()) {
      cb_setAccepted_864_0.issue<QGraphicsSceneDragDropEvent_Adaptor, bool>(&QGraphicsSceneDragDropEvent_Adaptor::cbs_setAccepted_864_0, accepted);
    } else {
      QGraphicsSceneDragDropEvent::setAccepted(accepted);
    }
  }

  gsi::Callback cb_clone_c0_0;
  gsi::Callback cb_setAccepted_864_0;
};

QGraphicsSceneDragDropEvent_Adaptor::~QGraphicsSceneDragDropEvent_Adaptor() { }

//  Constructor QGraphicsSceneDragDropEvent::QGraphicsSceneDragDropEvent(QEvent::Type type) (adaptor class)

static void _init_ctor_QGraphicsSceneDragDropEvent_Adaptor_1565 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("type", true, "QEvent::None");
  decl->add_arg<const qt_gsi::Converter<QEvent::Type>::target_type & > (argspec_0);
  decl->set_return_new<QGraphicsSceneDragDropEvent_Adaptor> ();
}

static void _call_ctor_QGraphicsSceneDragDropEvent_Adaptor_1565 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<QEvent::Type>::target_type & arg1 = args ? gsi::arg_reader<const qt_gsi::Converter<QEvent::Type>::target_type & >() (args, heap) : gsi::arg_maker<const qt_gsi::Converter<QEvent::Type>::target_type & >() (qt_gsi::CppToQtReadAdaptor<QEvent::Type>(heap, QEvent::None), heap);
  ret.write<QGraphicsSceneDragDropEvent_Adaptor *> (new QGraphicsSceneDragDropEvent_Adaptor (qt_gsi::QtToCppAdaptor<QEvent::Type>(arg1).cref()));
}


// QEvent *QGraphicsSceneDragDropEvent::clone()

static void _init_cbs_clone_c0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QEvent * > ();
}

static void _call_cbs_clone_c0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QEvent * > ((QEvent *)((QGraphicsSceneDragDropEvent_Adaptor *)cls)->cbs_clone_c0_0 ());
}

static void _set_callback_cbs_clone_c0_0 (void *cls, const gsi::Callback &cb)
{
  ((QGraphicsSceneDragDropEvent_Adaptor *)cls)->cb_clone_c0_0 = cb;
}


// void QGraphicsSceneDragDropEvent::setAccepted(bool accepted)

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
  ((QGraphicsSceneDragDropEvent_Adaptor *)cls)->cbs_setAccepted_864_0 (arg1);
}

static void _set_callback_cbs_setAccepted_864_0 (void *cls, const gsi::Callback &cb)
{
  ((QGraphicsSceneDragDropEvent_Adaptor *)cls)->cb_setAccepted_864_0 = cb;
}


namespace gsi
{

gsi::Class<QGraphicsSceneDragDropEvent> &qtdecl_QGraphicsSceneDragDropEvent ();

static gsi::Methods methods_QGraphicsSceneDragDropEvent_Adaptor () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QGraphicsSceneDragDropEvent::QGraphicsSceneDragDropEvent(QEvent::Type type)\nThis method creates an object of class QGraphicsSceneDragDropEvent.", &_init_ctor_QGraphicsSceneDragDropEvent_Adaptor_1565, &_call_ctor_QGraphicsSceneDragDropEvent_Adaptor_1565);
  methods += new qt_gsi::GenericMethod ("clone", "@brief Virtual method QEvent *QGraphicsSceneDragDropEvent::clone()\nThis method can be reimplemented in a derived class.", true, &_init_cbs_clone_c0_0, &_call_cbs_clone_c0_0);
  methods += new qt_gsi::GenericMethod ("clone", "@hide", true, &_init_cbs_clone_c0_0, &_call_cbs_clone_c0_0, &_set_callback_cbs_clone_c0_0);
  methods += new qt_gsi::GenericMethod ("setAccepted", "@brief Virtual method void QGraphicsSceneDragDropEvent::setAccepted(bool accepted)\nThis method can be reimplemented in a derived class.", false, &_init_cbs_setAccepted_864_0, &_call_cbs_setAccepted_864_0);
  methods += new qt_gsi::GenericMethod ("setAccepted", "@hide", false, &_init_cbs_setAccepted_864_0, &_call_cbs_setAccepted_864_0, &_set_callback_cbs_setAccepted_864_0);
  return methods;
}

gsi::Class<QGraphicsSceneDragDropEvent_Adaptor> decl_QGraphicsSceneDragDropEvent_Adaptor (qtdecl_QGraphicsSceneDragDropEvent (), "QtWidgets", "QGraphicsSceneDragDropEvent",
  methods_QGraphicsSceneDragDropEvent_Adaptor (),
  "@qt\n@brief Binding of QGraphicsSceneDragDropEvent");

}

