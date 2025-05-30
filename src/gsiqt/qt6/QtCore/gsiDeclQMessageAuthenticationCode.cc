
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
*  @file gsiDeclQMessageAuthenticationCode.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QMessageAuthenticationCode>
#include <QIODevice>
#include "gsiQt.h"
#include "gsiQtCoreCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QMessageAuthenticationCode

//  Constructor QMessageAuthenticationCode::QMessageAuthenticationCode(QCryptographicHash::Algorithm method, const QByteArray &key)


static void _init_ctor_QMessageAuthenticationCode_5532 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("method");
  decl->add_arg<const qt_gsi::Converter<QCryptographicHash::Algorithm>::target_type & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("key", true, "QByteArray()");
  decl->add_arg<const QByteArray & > (argspec_1);
  decl->set_return_new<QMessageAuthenticationCode> ();
}

static void _call_ctor_QMessageAuthenticationCode_5532 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<QCryptographicHash::Algorithm>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<QCryptographicHash::Algorithm>::target_type & >() (args, heap);
  const QByteArray &arg2 = args ? gsi::arg_reader<const QByteArray & >() (args, heap) : gsi::arg_maker<const QByteArray & >() (QByteArray(), heap);
  ret.write<QMessageAuthenticationCode *> (new QMessageAuthenticationCode (qt_gsi::QtToCppAdaptor<QCryptographicHash::Algorithm>(arg1).cref(), arg2));
}


// void QMessageAuthenticationCode::addData(const char *data, qsizetype length)


static void _init_f_addData_3065 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("data");
  decl->add_arg<const char * > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("length");
  decl->add_arg<qsizetype > (argspec_1);
  decl->set_return<void > ();
}

static void _call_f_addData_3065 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const char *arg1 = gsi::arg_reader<const char * >() (args, heap);
  qsizetype arg2 = gsi::arg_reader<qsizetype >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QMessageAuthenticationCode *)cls)->addData (arg1, arg2);
}


// void QMessageAuthenticationCode::addData(const QByteArray &data)


static void _init_f_addData_2309 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("data");
  decl->add_arg<const QByteArray & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_addData_2309 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QByteArray &arg1 = gsi::arg_reader<const QByteArray & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QMessageAuthenticationCode *)cls)->addData (arg1);
}


// bool QMessageAuthenticationCode::addData(QIODevice *device)


static void _init_f_addData_1447 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("device");
  decl->add_arg<QIODevice * > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_addData_1447 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QIODevice *arg1 = gsi::arg_reader<QIODevice * >() (args, heap);
  ret.write<bool > ((bool)((QMessageAuthenticationCode *)cls)->addData (arg1));
}


// void QMessageAuthenticationCode::reset()


static void _init_f_reset_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<void > ();
}

static void _call_f_reset_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QMessageAuthenticationCode *)cls)->reset ();
}


// QByteArray QMessageAuthenticationCode::result()


static void _init_f_result_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QByteArray > ();
}

static void _call_f_result_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QByteArray > ((QByteArray)((QMessageAuthenticationCode *)cls)->result ());
}


// void QMessageAuthenticationCode::setKey(const QByteArray &key)


static void _init_f_setKey_2309 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("key");
  decl->add_arg<const QByteArray & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setKey_2309 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QByteArray &arg1 = gsi::arg_reader<const QByteArray & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QMessageAuthenticationCode *)cls)->setKey (arg1);
}


// static QByteArray QMessageAuthenticationCode::hash(const QByteArray &message, const QByteArray &key, QCryptographicHash::Algorithm method)


static void _init_f_hash_7733 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("message");
  decl->add_arg<const QByteArray & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("key");
  decl->add_arg<const QByteArray & > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("method");
  decl->add_arg<const qt_gsi::Converter<QCryptographicHash::Algorithm>::target_type & > (argspec_2);
  decl->set_return<QByteArray > ();
}

static void _call_f_hash_7733 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QByteArray &arg1 = gsi::arg_reader<const QByteArray & >() (args, heap);
  const QByteArray &arg2 = gsi::arg_reader<const QByteArray & >() (args, heap);
  const qt_gsi::Converter<QCryptographicHash::Algorithm>::target_type & arg3 = gsi::arg_reader<const qt_gsi::Converter<QCryptographicHash::Algorithm>::target_type & >() (args, heap);
  ret.write<QByteArray > ((QByteArray)QMessageAuthenticationCode::hash (arg1, arg2, qt_gsi::QtToCppAdaptor<QCryptographicHash::Algorithm>(arg3).cref()));
}



namespace gsi
{

static gsi::Methods methods_QMessageAuthenticationCode () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QMessageAuthenticationCode::QMessageAuthenticationCode(QCryptographicHash::Algorithm method, const QByteArray &key)\nThis method creates an object of class QMessageAuthenticationCode.", &_init_ctor_QMessageAuthenticationCode_5532, &_call_ctor_QMessageAuthenticationCode_5532);
  methods += new qt_gsi::GenericMethod ("addData", "@brief Method void QMessageAuthenticationCode::addData(const char *data, qsizetype length)\n", false, &_init_f_addData_3065, &_call_f_addData_3065);
  methods += new qt_gsi::GenericMethod ("addData", "@brief Method void QMessageAuthenticationCode::addData(const QByteArray &data)\n", false, &_init_f_addData_2309, &_call_f_addData_2309);
  methods += new qt_gsi::GenericMethod ("addData", "@brief Method bool QMessageAuthenticationCode::addData(QIODevice *device)\n", false, &_init_f_addData_1447, &_call_f_addData_1447);
  methods += new qt_gsi::GenericMethod ("reset", "@brief Method void QMessageAuthenticationCode::reset()\n", false, &_init_f_reset_0, &_call_f_reset_0);
  methods += new qt_gsi::GenericMethod ("result", "@brief Method QByteArray QMessageAuthenticationCode::result()\n", true, &_init_f_result_c0, &_call_f_result_c0);
  methods += new qt_gsi::GenericMethod ("setKey", "@brief Method void QMessageAuthenticationCode::setKey(const QByteArray &key)\n", false, &_init_f_setKey_2309, &_call_f_setKey_2309);
  methods += new qt_gsi::GenericStaticMethod ("hash", "@brief Static method QByteArray QMessageAuthenticationCode::hash(const QByteArray &message, const QByteArray &key, QCryptographicHash::Algorithm method)\nThis method is static and can be called without an instance.", &_init_f_hash_7733, &_call_f_hash_7733);
  return methods;
}

gsi::Class<QMessageAuthenticationCode> decl_QMessageAuthenticationCode ("QtCore", "QMessageAuthenticationCode",
  methods_QMessageAuthenticationCode (),
  "@qt\n@brief Binding of QMessageAuthenticationCode");


GSI_QTCORE_PUBLIC gsi::Class<QMessageAuthenticationCode> &qtdecl_QMessageAuthenticationCode () { return decl_QMessageAuthenticationCode; }

}

