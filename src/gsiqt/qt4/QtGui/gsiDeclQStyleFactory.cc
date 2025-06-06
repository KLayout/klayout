
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
*  @file gsiDeclQStyleFactory.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QStyleFactory>
#include <QStyle>
#include "gsiQt.h"
#include "gsiQtGuiCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QStyleFactory

//  Constructor QStyleFactory::QStyleFactory()


static void _init_ctor_QStyleFactory_0 (qt_gsi::GenericStaticMethod *decl)
{
  decl->set_return_new<QStyleFactory> ();
}

static void _call_ctor_QStyleFactory_0 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QStyleFactory *> (new QStyleFactory ());
}


// static QStyle *QStyleFactory::create(const QString &)


static void _init_f_create_2025 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<const QString & > (argspec_0);
  decl->set_return<QStyle * > ();
}

static void _call_f_create_2025 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  ret.write<QStyle * > ((QStyle *)QStyleFactory::create (arg1));
}


// static QStringList QStyleFactory::keys()


static void _init_f_keys_0 (qt_gsi::GenericStaticMethod *decl)
{
  decl->set_return<QStringList > ();
}

static void _call_f_keys_0 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QStringList > ((QStringList)QStyleFactory::keys ());
}



namespace gsi
{

static gsi::Methods methods_QStyleFactory () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QStyleFactory::QStyleFactory()\nThis method creates an object of class QStyleFactory.", &_init_ctor_QStyleFactory_0, &_call_ctor_QStyleFactory_0);
  methods += new qt_gsi::GenericStaticMethod ("qt_create", "@brief Static method QStyle *QStyleFactory::create(const QString &)\nThis method is static and can be called without an instance.", &_init_f_create_2025, &_call_f_create_2025);
  methods += new qt_gsi::GenericStaticMethod ("keys", "@brief Static method QStringList QStyleFactory::keys()\nThis method is static and can be called without an instance.", &_init_f_keys_0, &_call_f_keys_0);
  return methods;
}

gsi::Class<QStyleFactory> decl_QStyleFactory ("QtGui", "QStyleFactory",
  methods_QStyleFactory (),
  "@qt\n@brief Binding of QStyleFactory");


GSI_QTGUI_PUBLIC gsi::Class<QStyleFactory> &qtdecl_QStyleFactory () { return decl_QStyleFactory; }

}

