
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
*  @file gsiDeclQStyleOptionTabWidgetFrame.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QStyleOptionTabWidgetFrame>
#include <QWidget>
#include "gsiQt.h"
#include "gsiQtWidgetsCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QStyleOptionTabWidgetFrame

//  Constructor QStyleOptionTabWidgetFrame::QStyleOptionTabWidgetFrame()


static void _init_ctor_QStyleOptionTabWidgetFrame_0 (qt_gsi::GenericStaticMethod *decl)
{
  decl->set_return_new<QStyleOptionTabWidgetFrame> ();
}

static void _call_ctor_QStyleOptionTabWidgetFrame_0 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QStyleOptionTabWidgetFrame *> (new QStyleOptionTabWidgetFrame ());
}


//  Constructor QStyleOptionTabWidgetFrame::QStyleOptionTabWidgetFrame(const QStyleOptionTabWidgetFrame &other)


static void _init_ctor_QStyleOptionTabWidgetFrame_3938 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QStyleOptionTabWidgetFrame & > (argspec_0);
  decl->set_return_new<QStyleOptionTabWidgetFrame> ();
}

static void _call_ctor_QStyleOptionTabWidgetFrame_3938 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QStyleOptionTabWidgetFrame &arg1 = gsi::arg_reader<const QStyleOptionTabWidgetFrame & >() (args, heap);
  ret.write<QStyleOptionTabWidgetFrame *> (new QStyleOptionTabWidgetFrame (arg1));
}


// QStyleOptionTabWidgetFrame &QStyleOptionTabWidgetFrame::operator=(const QStyleOptionTabWidgetFrame &)


static void _init_f_operator_eq__3938 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<const QStyleOptionTabWidgetFrame & > (argspec_0);
  decl->set_return<QStyleOptionTabWidgetFrame & > ();
}

static void _call_f_operator_eq__3938 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QStyleOptionTabWidgetFrame &arg1 = gsi::arg_reader<const QStyleOptionTabWidgetFrame & >() (args, heap);
  ret.write<QStyleOptionTabWidgetFrame & > ((QStyleOptionTabWidgetFrame &)((QStyleOptionTabWidgetFrame *)cls)->operator= (arg1));
}



namespace gsi
{

static gsi::Methods methods_QStyleOptionTabWidgetFrame () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QStyleOptionTabWidgetFrame::QStyleOptionTabWidgetFrame()\nThis method creates an object of class QStyleOptionTabWidgetFrame.", &_init_ctor_QStyleOptionTabWidgetFrame_0, &_call_ctor_QStyleOptionTabWidgetFrame_0);
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QStyleOptionTabWidgetFrame::QStyleOptionTabWidgetFrame(const QStyleOptionTabWidgetFrame &other)\nThis method creates an object of class QStyleOptionTabWidgetFrame.", &_init_ctor_QStyleOptionTabWidgetFrame_3938, &_call_ctor_QStyleOptionTabWidgetFrame_3938);
  methods += new qt_gsi::GenericMethod ("assign", "@brief Method QStyleOptionTabWidgetFrame &QStyleOptionTabWidgetFrame::operator=(const QStyleOptionTabWidgetFrame &)\n", false, &_init_f_operator_eq__3938, &_call_f_operator_eq__3938);
  return methods;
}

gsi::Class<QStyleOption> &qtdecl_QStyleOption ();

gsi::Class<QStyleOptionTabWidgetFrame> decl_QStyleOptionTabWidgetFrame (qtdecl_QStyleOption (), "QtWidgets", "QStyleOptionTabWidgetFrame",
  methods_QStyleOptionTabWidgetFrame (),
  "@qt\n@brief Binding of QStyleOptionTabWidgetFrame");


GSI_QTWIDGETS_PUBLIC gsi::Class<QStyleOptionTabWidgetFrame> &qtdecl_QStyleOptionTabWidgetFrame () { return decl_QStyleOptionTabWidgetFrame; }

}

