
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
*  @file gsiDeclQModelRoleData.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QModelRoleData>
#include "gsiQt.h"
#include "gsiQtCoreCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QModelRoleData

//  Constructor QModelRoleData::QModelRoleData(int role)


static void _init_ctor_QModelRoleData_767 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("role");
  decl->add_arg<int > (argspec_0);
  decl->set_return_new<QModelRoleData> ();
}

static void _call_ctor_QModelRoleData_767 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  ret.write<QModelRoleData *> (new QModelRoleData (arg1));
}


// void QModelRoleData::clearData()


static void _init_f_clearData_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<void > ();
}

static void _call_f_clearData_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QModelRoleData *)cls)->clearData ();
}


// QVariant &QModelRoleData::data()


static void _init_f_data_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QVariant & > ();
}

static void _call_f_data_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QVariant & > ((QVariant &)((QModelRoleData *)cls)->data ());
}


// const QVariant &QModelRoleData::data()


static void _init_f_data_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<const QVariant & > ();
}

static void _call_f_data_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<const QVariant & > ((const QVariant &)((QModelRoleData *)cls)->data ());
}


// int QModelRoleData::role()


static void _init_f_role_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_role_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QModelRoleData *)cls)->role ());
}



namespace gsi
{

static gsi::Methods methods_QModelRoleData () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QModelRoleData::QModelRoleData(int role)\nThis method creates an object of class QModelRoleData.", &_init_ctor_QModelRoleData_767, &_call_ctor_QModelRoleData_767);
  methods += new qt_gsi::GenericMethod ("clearData", "@brief Method void QModelRoleData::clearData()\n", false, &_init_f_clearData_0, &_call_f_clearData_0);
  methods += new qt_gsi::GenericMethod ("data", "@brief Method QVariant &QModelRoleData::data()\n", false, &_init_f_data_0, &_call_f_data_0);
  methods += new qt_gsi::GenericMethod ("data", "@brief Method const QVariant &QModelRoleData::data()\n", true, &_init_f_data_c0, &_call_f_data_c0);
  methods += new qt_gsi::GenericMethod ("role", "@brief Method int QModelRoleData::role()\n", true, &_init_f_role_c0, &_call_f_role_c0);
  return methods;
}

gsi::Class<QModelRoleData> decl_QModelRoleData ("QtCore", "QModelRoleData",
  methods_QModelRoleData (),
  "@qt\n@brief Binding of QModelRoleData");


GSI_QTCORE_PUBLIC gsi::Class<QModelRoleData> &qtdecl_QModelRoleData () { return decl_QModelRoleData; }

}

