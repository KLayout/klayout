
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
*  @file gsiDeclQSpacerItem.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QSpacerItem>
#include <QLayout>
#include <QRect>
#include <QSize>
#include <QSizePolicy>
#include <QWidget>
#include "gsiQt.h"
#include "gsiQtWidgetsCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QSpacerItem

// void QSpacerItem::changeSize(int w, int h, QSizePolicy::Policy hData, QSizePolicy::Policy vData)


static void _init_f_changeSize_5794 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("w");
  decl->add_arg<int > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("h");
  decl->add_arg<int > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("hData", true, "QSizePolicy::Minimum");
  decl->add_arg<const qt_gsi::Converter<QSizePolicy::Policy>::target_type & > (argspec_2);
  static gsi::ArgSpecBase argspec_3 ("vData", true, "QSizePolicy::Minimum");
  decl->add_arg<const qt_gsi::Converter<QSizePolicy::Policy>::target_type & > (argspec_3);
  decl->set_return<void > ();
}

static void _call_f_changeSize_5794 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  int arg2 = gsi::arg_reader<int >() (args, heap);
  const qt_gsi::Converter<QSizePolicy::Policy>::target_type & arg3 = args ? gsi::arg_reader<const qt_gsi::Converter<QSizePolicy::Policy>::target_type & >() (args, heap) : gsi::arg_maker<const qt_gsi::Converter<QSizePolicy::Policy>::target_type & >() (qt_gsi::CppToQtReadAdaptor<QSizePolicy::Policy>(heap, QSizePolicy::Minimum), heap);
  const qt_gsi::Converter<QSizePolicy::Policy>::target_type & arg4 = args ? gsi::arg_reader<const qt_gsi::Converter<QSizePolicy::Policy>::target_type & >() (args, heap) : gsi::arg_maker<const qt_gsi::Converter<QSizePolicy::Policy>::target_type & >() (qt_gsi::CppToQtReadAdaptor<QSizePolicy::Policy>(heap, QSizePolicy::Minimum), heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QSpacerItem *)cls)->changeSize (arg1, arg2, qt_gsi::QtToCppAdaptor<QSizePolicy::Policy>(arg3).cref(), qt_gsi::QtToCppAdaptor<QSizePolicy::Policy>(arg4).cref());
}


// QFlags<Qt::Orientation> QSpacerItem::expandingDirections()


static void _init_f_expandingDirections_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QFlags<Qt::Orientation> > ();
}

static void _call_f_expandingDirections_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QFlags<Qt::Orientation> > ((QFlags<Qt::Orientation>)((QSpacerItem *)cls)->expandingDirections ());
}


// QRect QSpacerItem::geometry()


static void _init_f_geometry_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QRect > ();
}

static void _call_f_geometry_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QRect > ((QRect)((QSpacerItem *)cls)->geometry ());
}


// bool QSpacerItem::isEmpty()


static void _init_f_isEmpty_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<bool > ();
}

static void _call_f_isEmpty_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<bool > ((bool)((QSpacerItem *)cls)->isEmpty ());
}


// QSize QSpacerItem::maximumSize()


static void _init_f_maximumSize_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QSize > ();
}

static void _call_f_maximumSize_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QSize > ((QSize)((QSpacerItem *)cls)->maximumSize ());
}


// QSize QSpacerItem::minimumSize()


static void _init_f_minimumSize_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QSize > ();
}

static void _call_f_minimumSize_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QSize > ((QSize)((QSpacerItem *)cls)->minimumSize ());
}


// void QSpacerItem::setGeometry(const QRect &)


static void _init_f_setGeometry_1792 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<const QRect & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setGeometry_1792 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QRect &arg1 = gsi::arg_reader<const QRect & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QSpacerItem *)cls)->setGeometry (arg1);
}


// QSize QSpacerItem::sizeHint()


static void _init_f_sizeHint_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QSize > ();
}

static void _call_f_sizeHint_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QSize > ((QSize)((QSpacerItem *)cls)->sizeHint ());
}


// QSizePolicy QSpacerItem::sizePolicy()


static void _init_f_sizePolicy_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QSizePolicy > ();
}

static void _call_f_sizePolicy_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QSizePolicy > ((QSizePolicy)((QSpacerItem *)cls)->sizePolicy ());
}


// QSpacerItem *QSpacerItem::spacerItem()


static void _init_f_spacerItem_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QSpacerItem * > ();
}

static void _call_f_spacerItem_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QSpacerItem * > ((QSpacerItem *)((QSpacerItem *)cls)->spacerItem ());
}


namespace gsi
{

static gsi::Methods methods_QSpacerItem () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericMethod ("changeSize", "@brief Method void QSpacerItem::changeSize(int w, int h, QSizePolicy::Policy hData, QSizePolicy::Policy vData)\n", false, &_init_f_changeSize_5794, &_call_f_changeSize_5794);
  methods += new qt_gsi::GenericMethod ("expandingDirections", "@brief Method QFlags<Qt::Orientation> QSpacerItem::expandingDirections()\nThis is a reimplementation of QLayoutItem::expandingDirections", true, &_init_f_expandingDirections_c0, &_call_f_expandingDirections_c0);
  methods += new qt_gsi::GenericMethod (":geometry", "@brief Method QRect QSpacerItem::geometry()\nThis is a reimplementation of QLayoutItem::geometry", true, &_init_f_geometry_c0, &_call_f_geometry_c0);
  methods += new qt_gsi::GenericMethod ("isEmpty?", "@brief Method bool QSpacerItem::isEmpty()\nThis is a reimplementation of QLayoutItem::isEmpty", true, &_init_f_isEmpty_c0, &_call_f_isEmpty_c0);
  methods += new qt_gsi::GenericMethod ("maximumSize", "@brief Method QSize QSpacerItem::maximumSize()\nThis is a reimplementation of QLayoutItem::maximumSize", true, &_init_f_maximumSize_c0, &_call_f_maximumSize_c0);
  methods += new qt_gsi::GenericMethod ("minimumSize", "@brief Method QSize QSpacerItem::minimumSize()\nThis is a reimplementation of QLayoutItem::minimumSize", true, &_init_f_minimumSize_c0, &_call_f_minimumSize_c0);
  methods += new qt_gsi::GenericMethod ("setGeometry|geometry=", "@brief Method void QSpacerItem::setGeometry(const QRect &)\nThis is a reimplementation of QLayoutItem::setGeometry", false, &_init_f_setGeometry_1792, &_call_f_setGeometry_1792);
  methods += new qt_gsi::GenericMethod ("sizeHint", "@brief Method QSize QSpacerItem::sizeHint()\nThis is a reimplementation of QLayoutItem::sizeHint", true, &_init_f_sizeHint_c0, &_call_f_sizeHint_c0);
  methods += new qt_gsi::GenericMethod ("sizePolicy", "@brief Method QSizePolicy QSpacerItem::sizePolicy()\n", true, &_init_f_sizePolicy_c0, &_call_f_sizePolicy_c0);
  methods += new qt_gsi::GenericMethod ("spacerItem", "@brief Method QSpacerItem *QSpacerItem::spacerItem()\nThis is a reimplementation of QLayoutItem::spacerItem", false, &_init_f_spacerItem_0, &_call_f_spacerItem_0);
  return methods;
}

gsi::Class<QLayoutItem> &qtdecl_QLayoutItem ();

gsi::Class<QSpacerItem> decl_QSpacerItem (qtdecl_QLayoutItem (), "QtWidgets", "QSpacerItem_Native",
  methods_QSpacerItem (),
  "@hide\n@alias QSpacerItem");

GSI_QTWIDGETS_PUBLIC gsi::Class<QSpacerItem> &qtdecl_QSpacerItem () { return decl_QSpacerItem; }

}


class QSpacerItem_Adaptor : public QSpacerItem, public qt_gsi::QtObjectBase
{
public:

  virtual ~QSpacerItem_Adaptor();

  //  [adaptor ctor] QSpacerItem::QSpacerItem(int w, int h, QSizePolicy::Policy hData, QSizePolicy::Policy vData)
  QSpacerItem_Adaptor(int w, int h) : QSpacerItem(w, h)
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [adaptor ctor] QSpacerItem::QSpacerItem(int w, int h, QSizePolicy::Policy hData, QSizePolicy::Policy vData)
  QSpacerItem_Adaptor(int w, int h, QSizePolicy::Policy hData) : QSpacerItem(w, h, hData)
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [adaptor ctor] QSpacerItem::QSpacerItem(int w, int h, QSizePolicy::Policy hData, QSizePolicy::Policy vData)
  QSpacerItem_Adaptor(int w, int h, QSizePolicy::Policy hData, QSizePolicy::Policy vData) : QSpacerItem(w, h, hData, vData)
  {
    qt_gsi::QtObjectBase::init (this);
  }

  //  [adaptor impl] QFlags<QSizePolicy::ControlType> QSpacerItem::controlTypes()
  QFlags<QSizePolicy::ControlType> cbs_controlTypes_c0_0() const
  {
    return QSpacerItem::controlTypes();
  }

  virtual QFlags<QSizePolicy::ControlType> controlTypes() const
  {
    if (cb_controlTypes_c0_0.can_issue()) {
      return cb_controlTypes_c0_0.issue<QSpacerItem_Adaptor, QFlags<QSizePolicy::ControlType> >(&QSpacerItem_Adaptor::cbs_controlTypes_c0_0);
    } else {
      return QSpacerItem::controlTypes();
    }
  }

  //  [adaptor impl] QFlags<Qt::Orientation> QSpacerItem::expandingDirections()
  QFlags<Qt::Orientation> cbs_expandingDirections_c0_0() const
  {
    return QSpacerItem::expandingDirections();
  }

  virtual QFlags<Qt::Orientation> expandingDirections() const
  {
    if (cb_expandingDirections_c0_0.can_issue()) {
      return cb_expandingDirections_c0_0.issue<QSpacerItem_Adaptor, QFlags<Qt::Orientation> >(&QSpacerItem_Adaptor::cbs_expandingDirections_c0_0);
    } else {
      return QSpacerItem::expandingDirections();
    }
  }

  //  [adaptor impl] QRect QSpacerItem::geometry()
  QRect cbs_geometry_c0_0() const
  {
    return QSpacerItem::geometry();
  }

  virtual QRect geometry() const
  {
    if (cb_geometry_c0_0.can_issue()) {
      return cb_geometry_c0_0.issue<QSpacerItem_Adaptor, QRect>(&QSpacerItem_Adaptor::cbs_geometry_c0_0);
    } else {
      return QSpacerItem::geometry();
    }
  }

  //  [adaptor impl] bool QSpacerItem::hasHeightForWidth()
  bool cbs_hasHeightForWidth_c0_0() const
  {
    return QSpacerItem::hasHeightForWidth();
  }

  virtual bool hasHeightForWidth() const
  {
    if (cb_hasHeightForWidth_c0_0.can_issue()) {
      return cb_hasHeightForWidth_c0_0.issue<QSpacerItem_Adaptor, bool>(&QSpacerItem_Adaptor::cbs_hasHeightForWidth_c0_0);
    } else {
      return QSpacerItem::hasHeightForWidth();
    }
  }

  //  [adaptor impl] int QSpacerItem::heightForWidth(int)
  int cbs_heightForWidth_c767_0(int arg1) const
  {
    return QSpacerItem::heightForWidth(arg1);
  }

  virtual int heightForWidth(int arg1) const
  {
    if (cb_heightForWidth_c767_0.can_issue()) {
      return cb_heightForWidth_c767_0.issue<QSpacerItem_Adaptor, int, int>(&QSpacerItem_Adaptor::cbs_heightForWidth_c767_0, arg1);
    } else {
      return QSpacerItem::heightForWidth(arg1);
    }
  }

  //  [adaptor impl] void QSpacerItem::invalidate()
  void cbs_invalidate_0_0()
  {
    QSpacerItem::invalidate();
  }

  virtual void invalidate()
  {
    if (cb_invalidate_0_0.can_issue()) {
      cb_invalidate_0_0.issue<QSpacerItem_Adaptor>(&QSpacerItem_Adaptor::cbs_invalidate_0_0);
    } else {
      QSpacerItem::invalidate();
    }
  }

  //  [adaptor impl] bool QSpacerItem::isEmpty()
  bool cbs_isEmpty_c0_0() const
  {
    return QSpacerItem::isEmpty();
  }

  virtual bool isEmpty() const
  {
    if (cb_isEmpty_c0_0.can_issue()) {
      return cb_isEmpty_c0_0.issue<QSpacerItem_Adaptor, bool>(&QSpacerItem_Adaptor::cbs_isEmpty_c0_0);
    } else {
      return QSpacerItem::isEmpty();
    }
  }

  //  [adaptor impl] QLayout *QSpacerItem::layout()
  QLayout * cbs_layout_0_0()
  {
    return QSpacerItem::layout();
  }

  virtual QLayout * layout()
  {
    if (cb_layout_0_0.can_issue()) {
      return cb_layout_0_0.issue<QSpacerItem_Adaptor, QLayout *>(&QSpacerItem_Adaptor::cbs_layout_0_0);
    } else {
      return QSpacerItem::layout();
    }
  }

  //  [adaptor impl] QSize QSpacerItem::maximumSize()
  QSize cbs_maximumSize_c0_0() const
  {
    return QSpacerItem::maximumSize();
  }

  virtual QSize maximumSize() const
  {
    if (cb_maximumSize_c0_0.can_issue()) {
      return cb_maximumSize_c0_0.issue<QSpacerItem_Adaptor, QSize>(&QSpacerItem_Adaptor::cbs_maximumSize_c0_0);
    } else {
      return QSpacerItem::maximumSize();
    }
  }

  //  [adaptor impl] int QSpacerItem::minimumHeightForWidth(int)
  int cbs_minimumHeightForWidth_c767_0(int arg1) const
  {
    return QSpacerItem::minimumHeightForWidth(arg1);
  }

  virtual int minimumHeightForWidth(int arg1) const
  {
    if (cb_minimumHeightForWidth_c767_0.can_issue()) {
      return cb_minimumHeightForWidth_c767_0.issue<QSpacerItem_Adaptor, int, int>(&QSpacerItem_Adaptor::cbs_minimumHeightForWidth_c767_0, arg1);
    } else {
      return QSpacerItem::minimumHeightForWidth(arg1);
    }
  }

  //  [adaptor impl] QSize QSpacerItem::minimumSize()
  QSize cbs_minimumSize_c0_0() const
  {
    return QSpacerItem::minimumSize();
  }

  virtual QSize minimumSize() const
  {
    if (cb_minimumSize_c0_0.can_issue()) {
      return cb_minimumSize_c0_0.issue<QSpacerItem_Adaptor, QSize>(&QSpacerItem_Adaptor::cbs_minimumSize_c0_0);
    } else {
      return QSpacerItem::minimumSize();
    }
  }

  //  [adaptor impl] void QSpacerItem::setGeometry(const QRect &)
  void cbs_setGeometry_1792_0(const QRect &arg1)
  {
    QSpacerItem::setGeometry(arg1);
  }

  virtual void setGeometry(const QRect &arg1)
  {
    if (cb_setGeometry_1792_0.can_issue()) {
      cb_setGeometry_1792_0.issue<QSpacerItem_Adaptor, const QRect &>(&QSpacerItem_Adaptor::cbs_setGeometry_1792_0, arg1);
    } else {
      QSpacerItem::setGeometry(arg1);
    }
  }

  //  [adaptor impl] QSize QSpacerItem::sizeHint()
  QSize cbs_sizeHint_c0_0() const
  {
    return QSpacerItem::sizeHint();
  }

  virtual QSize sizeHint() const
  {
    if (cb_sizeHint_c0_0.can_issue()) {
      return cb_sizeHint_c0_0.issue<QSpacerItem_Adaptor, QSize>(&QSpacerItem_Adaptor::cbs_sizeHint_c0_0);
    } else {
      return QSpacerItem::sizeHint();
    }
  }

  //  [adaptor impl] QSpacerItem *QSpacerItem::spacerItem()
  QSpacerItem * cbs_spacerItem_0_0()
  {
    return QSpacerItem::spacerItem();
  }

  virtual QSpacerItem * spacerItem()
  {
    if (cb_spacerItem_0_0.can_issue()) {
      return cb_spacerItem_0_0.issue<QSpacerItem_Adaptor, QSpacerItem *>(&QSpacerItem_Adaptor::cbs_spacerItem_0_0);
    } else {
      return QSpacerItem::spacerItem();
    }
  }

  //  [adaptor impl] QWidget *QSpacerItem::widget()
  QWidget * cbs_widget_0_0()
  {
    return QSpacerItem::widget();
  }

  virtual QWidget * widget()
  {
    if (cb_widget_0_0.can_issue()) {
      return cb_widget_0_0.issue<QSpacerItem_Adaptor, QWidget *>(&QSpacerItem_Adaptor::cbs_widget_0_0);
    } else {
      return QSpacerItem::widget();
    }
  }

  gsi::Callback cb_controlTypes_c0_0;
  gsi::Callback cb_expandingDirections_c0_0;
  gsi::Callback cb_geometry_c0_0;
  gsi::Callback cb_hasHeightForWidth_c0_0;
  gsi::Callback cb_heightForWidth_c767_0;
  gsi::Callback cb_invalidate_0_0;
  gsi::Callback cb_isEmpty_c0_0;
  gsi::Callback cb_layout_0_0;
  gsi::Callback cb_maximumSize_c0_0;
  gsi::Callback cb_minimumHeightForWidth_c767_0;
  gsi::Callback cb_minimumSize_c0_0;
  gsi::Callback cb_setGeometry_1792_0;
  gsi::Callback cb_sizeHint_c0_0;
  gsi::Callback cb_spacerItem_0_0;
  gsi::Callback cb_widget_0_0;
};

QSpacerItem_Adaptor::~QSpacerItem_Adaptor() { }

//  Constructor QSpacerItem::QSpacerItem(int w, int h, QSizePolicy::Policy hData, QSizePolicy::Policy vData) (adaptor class)

static void _init_ctor_QSpacerItem_Adaptor_5794 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("w");
  decl->add_arg<int > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("h");
  decl->add_arg<int > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("hData", true, "QSizePolicy::Minimum");
  decl->add_arg<const qt_gsi::Converter<QSizePolicy::Policy>::target_type & > (argspec_2);
  static gsi::ArgSpecBase argspec_3 ("vData", true, "QSizePolicy::Minimum");
  decl->add_arg<const qt_gsi::Converter<QSizePolicy::Policy>::target_type & > (argspec_3);
  decl->set_return_new<QSpacerItem_Adaptor> ();
}

static void _call_ctor_QSpacerItem_Adaptor_5794 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  int arg2 = gsi::arg_reader<int >() (args, heap);
  const qt_gsi::Converter<QSizePolicy::Policy>::target_type & arg3 = args ? gsi::arg_reader<const qt_gsi::Converter<QSizePolicy::Policy>::target_type & >() (args, heap) : gsi::arg_maker<const qt_gsi::Converter<QSizePolicy::Policy>::target_type & >() (qt_gsi::CppToQtReadAdaptor<QSizePolicy::Policy>(heap, QSizePolicy::Minimum), heap);
  const qt_gsi::Converter<QSizePolicy::Policy>::target_type & arg4 = args ? gsi::arg_reader<const qt_gsi::Converter<QSizePolicy::Policy>::target_type & >() (args, heap) : gsi::arg_maker<const qt_gsi::Converter<QSizePolicy::Policy>::target_type & >() (qt_gsi::CppToQtReadAdaptor<QSizePolicy::Policy>(heap, QSizePolicy::Minimum), heap);
  ret.write<QSpacerItem_Adaptor *> (new QSpacerItem_Adaptor (arg1, arg2, qt_gsi::QtToCppAdaptor<QSizePolicy::Policy>(arg3).cref(), qt_gsi::QtToCppAdaptor<QSizePolicy::Policy>(arg4).cref()));
}


// QFlags<QSizePolicy::ControlType> QSpacerItem::controlTypes()

static void _init_cbs_controlTypes_c0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QFlags<QSizePolicy::ControlType> > ();
}

static void _call_cbs_controlTypes_c0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QFlags<QSizePolicy::ControlType> > ((QFlags<QSizePolicy::ControlType>)((QSpacerItem_Adaptor *)cls)->cbs_controlTypes_c0_0 ());
}

static void _set_callback_cbs_controlTypes_c0_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_controlTypes_c0_0 = cb;
}


// QFlags<Qt::Orientation> QSpacerItem::expandingDirections()

static void _init_cbs_expandingDirections_c0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QFlags<Qt::Orientation> > ();
}

static void _call_cbs_expandingDirections_c0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QFlags<Qt::Orientation> > ((QFlags<Qt::Orientation>)((QSpacerItem_Adaptor *)cls)->cbs_expandingDirections_c0_0 ());
}

static void _set_callback_cbs_expandingDirections_c0_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_expandingDirections_c0_0 = cb;
}


// QRect QSpacerItem::geometry()

static void _init_cbs_geometry_c0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QRect > ();
}

static void _call_cbs_geometry_c0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QRect > ((QRect)((QSpacerItem_Adaptor *)cls)->cbs_geometry_c0_0 ());
}

static void _set_callback_cbs_geometry_c0_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_geometry_c0_0 = cb;
}


// bool QSpacerItem::hasHeightForWidth()

static void _init_cbs_hasHeightForWidth_c0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<bool > ();
}

static void _call_cbs_hasHeightForWidth_c0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<bool > ((bool)((QSpacerItem_Adaptor *)cls)->cbs_hasHeightForWidth_c0_0 ());
}

static void _set_callback_cbs_hasHeightForWidth_c0_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_hasHeightForWidth_c0_0 = cb;
}


// int QSpacerItem::heightForWidth(int)

static void _init_cbs_heightForWidth_c767_0 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<int > (argspec_0);
  decl->set_return<int > ();
}

static void _call_cbs_heightForWidth_c767_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = args.read<int > (heap);
  ret.write<int > ((int)((QSpacerItem_Adaptor *)cls)->cbs_heightForWidth_c767_0 (arg1));
}

static void _set_callback_cbs_heightForWidth_c767_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_heightForWidth_c767_0 = cb;
}


// void QSpacerItem::invalidate()

static void _init_cbs_invalidate_0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<void > ();
}

static void _call_cbs_invalidate_0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QSpacerItem_Adaptor *)cls)->cbs_invalidate_0_0 ();
}

static void _set_callback_cbs_invalidate_0_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_invalidate_0_0 = cb;
}


// bool QSpacerItem::isEmpty()

static void _init_cbs_isEmpty_c0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<bool > ();
}

static void _call_cbs_isEmpty_c0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<bool > ((bool)((QSpacerItem_Adaptor *)cls)->cbs_isEmpty_c0_0 ());
}

static void _set_callback_cbs_isEmpty_c0_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_isEmpty_c0_0 = cb;
}


// QLayout *QSpacerItem::layout()

static void _init_cbs_layout_0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QLayout * > ();
}

static void _call_cbs_layout_0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QLayout * > ((QLayout *)((QSpacerItem_Adaptor *)cls)->cbs_layout_0_0 ());
}

static void _set_callback_cbs_layout_0_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_layout_0_0 = cb;
}


// QSize QSpacerItem::maximumSize()

static void _init_cbs_maximumSize_c0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QSize > ();
}

static void _call_cbs_maximumSize_c0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QSize > ((QSize)((QSpacerItem_Adaptor *)cls)->cbs_maximumSize_c0_0 ());
}

static void _set_callback_cbs_maximumSize_c0_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_maximumSize_c0_0 = cb;
}


// int QSpacerItem::minimumHeightForWidth(int)

static void _init_cbs_minimumHeightForWidth_c767_0 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<int > (argspec_0);
  decl->set_return<int > ();
}

static void _call_cbs_minimumHeightForWidth_c767_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = args.read<int > (heap);
  ret.write<int > ((int)((QSpacerItem_Adaptor *)cls)->cbs_minimumHeightForWidth_c767_0 (arg1));
}

static void _set_callback_cbs_minimumHeightForWidth_c767_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_minimumHeightForWidth_c767_0 = cb;
}


// QSize QSpacerItem::minimumSize()

static void _init_cbs_minimumSize_c0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QSize > ();
}

static void _call_cbs_minimumSize_c0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QSize > ((QSize)((QSpacerItem_Adaptor *)cls)->cbs_minimumSize_c0_0 ());
}

static void _set_callback_cbs_minimumSize_c0_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_minimumSize_c0_0 = cb;
}


// void QSpacerItem::setGeometry(const QRect &)

static void _init_cbs_setGeometry_1792_0 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<const QRect & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_cbs_setGeometry_1792_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QRect &arg1 = args.read<const QRect & > (heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QSpacerItem_Adaptor *)cls)->cbs_setGeometry_1792_0 (arg1);
}

static void _set_callback_cbs_setGeometry_1792_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_setGeometry_1792_0 = cb;
}


// QSize QSpacerItem::sizeHint()

static void _init_cbs_sizeHint_c0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QSize > ();
}

static void _call_cbs_sizeHint_c0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QSize > ((QSize)((QSpacerItem_Adaptor *)cls)->cbs_sizeHint_c0_0 ());
}

static void _set_callback_cbs_sizeHint_c0_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_sizeHint_c0_0 = cb;
}


// QSpacerItem *QSpacerItem::spacerItem()

static void _init_cbs_spacerItem_0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QSpacerItem * > ();
}

static void _call_cbs_spacerItem_0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QSpacerItem * > ((QSpacerItem *)((QSpacerItem_Adaptor *)cls)->cbs_spacerItem_0_0 ());
}

static void _set_callback_cbs_spacerItem_0_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_spacerItem_0_0 = cb;
}


// QWidget *QSpacerItem::widget()

static void _init_cbs_widget_0_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QWidget * > ();
}

static void _call_cbs_widget_0_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QWidget * > ((QWidget *)((QSpacerItem_Adaptor *)cls)->cbs_widget_0_0 ());
}

static void _set_callback_cbs_widget_0_0 (void *cls, const gsi::Callback &cb)
{
  ((QSpacerItem_Adaptor *)cls)->cb_widget_0_0 = cb;
}


namespace gsi
{

gsi::Class<QSpacerItem> &qtdecl_QSpacerItem ();

static gsi::Methods methods_QSpacerItem_Adaptor () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QSpacerItem::QSpacerItem(int w, int h, QSizePolicy::Policy hData, QSizePolicy::Policy vData)\nThis method creates an object of class QSpacerItem.", &_init_ctor_QSpacerItem_Adaptor_5794, &_call_ctor_QSpacerItem_Adaptor_5794);
  methods += new qt_gsi::GenericMethod ("controlTypes", "@brief Virtual method QFlags<QSizePolicy::ControlType> QSpacerItem::controlTypes()\nThis method can be reimplemented in a derived class.", true, &_init_cbs_controlTypes_c0_0, &_call_cbs_controlTypes_c0_0);
  methods += new qt_gsi::GenericMethod ("controlTypes", "@hide", true, &_init_cbs_controlTypes_c0_0, &_call_cbs_controlTypes_c0_0, &_set_callback_cbs_controlTypes_c0_0);
  methods += new qt_gsi::GenericMethod ("expandingDirections", "@brief Virtual method QFlags<Qt::Orientation> QSpacerItem::expandingDirections()\nThis method can be reimplemented in a derived class.", true, &_init_cbs_expandingDirections_c0_0, &_call_cbs_expandingDirections_c0_0);
  methods += new qt_gsi::GenericMethod ("expandingDirections", "@hide", true, &_init_cbs_expandingDirections_c0_0, &_call_cbs_expandingDirections_c0_0, &_set_callback_cbs_expandingDirections_c0_0);
  methods += new qt_gsi::GenericMethod ("geometry", "@brief Virtual method QRect QSpacerItem::geometry()\nThis method can be reimplemented in a derived class.", true, &_init_cbs_geometry_c0_0, &_call_cbs_geometry_c0_0);
  methods += new qt_gsi::GenericMethod ("geometry", "@hide", true, &_init_cbs_geometry_c0_0, &_call_cbs_geometry_c0_0, &_set_callback_cbs_geometry_c0_0);
  methods += new qt_gsi::GenericMethod ("hasHeightForWidth", "@brief Virtual method bool QSpacerItem::hasHeightForWidth()\nThis method can be reimplemented in a derived class.", true, &_init_cbs_hasHeightForWidth_c0_0, &_call_cbs_hasHeightForWidth_c0_0);
  methods += new qt_gsi::GenericMethod ("hasHeightForWidth", "@hide", true, &_init_cbs_hasHeightForWidth_c0_0, &_call_cbs_hasHeightForWidth_c0_0, &_set_callback_cbs_hasHeightForWidth_c0_0);
  methods += new qt_gsi::GenericMethod ("heightForWidth", "@brief Virtual method int QSpacerItem::heightForWidth(int)\nThis method can be reimplemented in a derived class.", true, &_init_cbs_heightForWidth_c767_0, &_call_cbs_heightForWidth_c767_0);
  methods += new qt_gsi::GenericMethod ("heightForWidth", "@hide", true, &_init_cbs_heightForWidth_c767_0, &_call_cbs_heightForWidth_c767_0, &_set_callback_cbs_heightForWidth_c767_0);
  methods += new qt_gsi::GenericMethod ("invalidate", "@brief Virtual method void QSpacerItem::invalidate()\nThis method can be reimplemented in a derived class.", false, &_init_cbs_invalidate_0_0, &_call_cbs_invalidate_0_0);
  methods += new qt_gsi::GenericMethod ("invalidate", "@hide", false, &_init_cbs_invalidate_0_0, &_call_cbs_invalidate_0_0, &_set_callback_cbs_invalidate_0_0);
  methods += new qt_gsi::GenericMethod ("isEmpty", "@brief Virtual method bool QSpacerItem::isEmpty()\nThis method can be reimplemented in a derived class.", true, &_init_cbs_isEmpty_c0_0, &_call_cbs_isEmpty_c0_0);
  methods += new qt_gsi::GenericMethod ("isEmpty", "@hide", true, &_init_cbs_isEmpty_c0_0, &_call_cbs_isEmpty_c0_0, &_set_callback_cbs_isEmpty_c0_0);
  methods += new qt_gsi::GenericMethod ("layout", "@brief Virtual method QLayout *QSpacerItem::layout()\nThis method can be reimplemented in a derived class.", false, &_init_cbs_layout_0_0, &_call_cbs_layout_0_0);
  methods += new qt_gsi::GenericMethod ("layout", "@hide", false, &_init_cbs_layout_0_0, &_call_cbs_layout_0_0, &_set_callback_cbs_layout_0_0);
  methods += new qt_gsi::GenericMethod ("maximumSize", "@brief Virtual method QSize QSpacerItem::maximumSize()\nThis method can be reimplemented in a derived class.", true, &_init_cbs_maximumSize_c0_0, &_call_cbs_maximumSize_c0_0);
  methods += new qt_gsi::GenericMethod ("maximumSize", "@hide", true, &_init_cbs_maximumSize_c0_0, &_call_cbs_maximumSize_c0_0, &_set_callback_cbs_maximumSize_c0_0);
  methods += new qt_gsi::GenericMethod ("minimumHeightForWidth", "@brief Virtual method int QSpacerItem::minimumHeightForWidth(int)\nThis method can be reimplemented in a derived class.", true, &_init_cbs_minimumHeightForWidth_c767_0, &_call_cbs_minimumHeightForWidth_c767_0);
  methods += new qt_gsi::GenericMethod ("minimumHeightForWidth", "@hide", true, &_init_cbs_minimumHeightForWidth_c767_0, &_call_cbs_minimumHeightForWidth_c767_0, &_set_callback_cbs_minimumHeightForWidth_c767_0);
  methods += new qt_gsi::GenericMethod ("minimumSize", "@brief Virtual method QSize QSpacerItem::minimumSize()\nThis method can be reimplemented in a derived class.", true, &_init_cbs_minimumSize_c0_0, &_call_cbs_minimumSize_c0_0);
  methods += new qt_gsi::GenericMethod ("minimumSize", "@hide", true, &_init_cbs_minimumSize_c0_0, &_call_cbs_minimumSize_c0_0, &_set_callback_cbs_minimumSize_c0_0);
  methods += new qt_gsi::GenericMethod ("setGeometry", "@brief Virtual method void QSpacerItem::setGeometry(const QRect &)\nThis method can be reimplemented in a derived class.", false, &_init_cbs_setGeometry_1792_0, &_call_cbs_setGeometry_1792_0);
  methods += new qt_gsi::GenericMethod ("setGeometry", "@hide", false, &_init_cbs_setGeometry_1792_0, &_call_cbs_setGeometry_1792_0, &_set_callback_cbs_setGeometry_1792_0);
  methods += new qt_gsi::GenericMethod ("sizeHint", "@brief Virtual method QSize QSpacerItem::sizeHint()\nThis method can be reimplemented in a derived class.", true, &_init_cbs_sizeHint_c0_0, &_call_cbs_sizeHint_c0_0);
  methods += new qt_gsi::GenericMethod ("sizeHint", "@hide", true, &_init_cbs_sizeHint_c0_0, &_call_cbs_sizeHint_c0_0, &_set_callback_cbs_sizeHint_c0_0);
  methods += new qt_gsi::GenericMethod ("spacerItem", "@brief Virtual method QSpacerItem *QSpacerItem::spacerItem()\nThis method can be reimplemented in a derived class.", false, &_init_cbs_spacerItem_0_0, &_call_cbs_spacerItem_0_0);
  methods += new qt_gsi::GenericMethod ("spacerItem", "@hide", false, &_init_cbs_spacerItem_0_0, &_call_cbs_spacerItem_0_0, &_set_callback_cbs_spacerItem_0_0);
  methods += new qt_gsi::GenericMethod ("widget", "@brief Virtual method QWidget *QSpacerItem::widget()\nThis method can be reimplemented in a derived class.", false, &_init_cbs_widget_0_0, &_call_cbs_widget_0_0);
  methods += new qt_gsi::GenericMethod ("widget", "@hide", false, &_init_cbs_widget_0_0, &_call_cbs_widget_0_0, &_set_callback_cbs_widget_0_0);
  return methods;
}

gsi::Class<QSpacerItem_Adaptor> decl_QSpacerItem_Adaptor (qtdecl_QSpacerItem (), "QtWidgets", "QSpacerItem",
  methods_QSpacerItem_Adaptor (),
  "@qt\n@brief Binding of QSpacerItem");

}

