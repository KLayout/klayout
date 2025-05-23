
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
*  @file gsiDeclQSystemSemaphore.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QSystemSemaphore>
#include "gsiQt.h"
#include "gsiQtCoreCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QSystemSemaphore

//  Constructor QSystemSemaphore::QSystemSemaphore(const QString &key, int initialValue, QSystemSemaphore::AccessMode mode)


static void _init_ctor_QSystemSemaphore_5769 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("key");
  decl->add_arg<const QString & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("initialValue", true, "0");
  decl->add_arg<int > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("mode", true, "QSystemSemaphore::Open");
  decl->add_arg<const qt_gsi::Converter<QSystemSemaphore::AccessMode>::target_type & > (argspec_2);
  decl->set_return_new<QSystemSemaphore> ();
}

static void _call_ctor_QSystemSemaphore_5769 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  int arg2 = args ? gsi::arg_reader<int >() (args, heap) : gsi::arg_maker<int >() (0, heap);
  const qt_gsi::Converter<QSystemSemaphore::AccessMode>::target_type & arg3 = args ? gsi::arg_reader<const qt_gsi::Converter<QSystemSemaphore::AccessMode>::target_type & >() (args, heap) : gsi::arg_maker<const qt_gsi::Converter<QSystemSemaphore::AccessMode>::target_type & >() (qt_gsi::CppToQtReadAdaptor<QSystemSemaphore::AccessMode>(heap, QSystemSemaphore::Open), heap);
  ret.write<QSystemSemaphore *> (new QSystemSemaphore (arg1, arg2, qt_gsi::QtToCppAdaptor<QSystemSemaphore::AccessMode>(arg3).cref()));
}


// bool QSystemSemaphore::acquire()


static void _init_f_acquire_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<bool > ();
}

static void _call_f_acquire_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<bool > ((bool)((QSystemSemaphore *)cls)->acquire ());
}


// QSystemSemaphore::SystemSemaphoreError QSystemSemaphore::error()


static void _init_f_error_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<qt_gsi::Converter<QSystemSemaphore::SystemSemaphoreError>::target_type > ();
}

static void _call_f_error_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<qt_gsi::Converter<QSystemSemaphore::SystemSemaphoreError>::target_type > ((qt_gsi::Converter<QSystemSemaphore::SystemSemaphoreError>::target_type)qt_gsi::CppToQtAdaptor<QSystemSemaphore::SystemSemaphoreError>(((QSystemSemaphore *)cls)->error ()));
}


// QString QSystemSemaphore::errorString()


static void _init_f_errorString_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QString > ();
}

static void _call_f_errorString_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QString > ((QString)((QSystemSemaphore *)cls)->errorString ());
}


// QString QSystemSemaphore::key()


static void _init_f_key_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QString > ();
}

static void _call_f_key_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QString > ((QString)((QSystemSemaphore *)cls)->key ());
}


// bool QSystemSemaphore::release(int n)


static void _init_f_release_767 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("n", true, "1");
  decl->add_arg<int > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_release_767 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = args ? gsi::arg_reader<int >() (args, heap) : gsi::arg_maker<int >() (1, heap);
  ret.write<bool > ((bool)((QSystemSemaphore *)cls)->release (arg1));
}


// void QSystemSemaphore::setKey(const QString &key, int initialValue, QSystemSemaphore::AccessMode mode)


static void _init_f_setKey_5769 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("key");
  decl->add_arg<const QString & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("initialValue", true, "0");
  decl->add_arg<int > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("mode", true, "QSystemSemaphore::Open");
  decl->add_arg<const qt_gsi::Converter<QSystemSemaphore::AccessMode>::target_type & > (argspec_2);
  decl->set_return<void > ();
}

static void _call_f_setKey_5769 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  int arg2 = args ? gsi::arg_reader<int >() (args, heap) : gsi::arg_maker<int >() (0, heap);
  const qt_gsi::Converter<QSystemSemaphore::AccessMode>::target_type & arg3 = args ? gsi::arg_reader<const qt_gsi::Converter<QSystemSemaphore::AccessMode>::target_type & >() (args, heap) : gsi::arg_maker<const qt_gsi::Converter<QSystemSemaphore::AccessMode>::target_type & >() (qt_gsi::CppToQtReadAdaptor<QSystemSemaphore::AccessMode>(heap, QSystemSemaphore::Open), heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QSystemSemaphore *)cls)->setKey (arg1, arg2, qt_gsi::QtToCppAdaptor<QSystemSemaphore::AccessMode>(arg3).cref());
}


// static QString QSystemSemaphore::tr(const char *sourceText, const char *disambiguation, int n)


static void _init_f_tr_4013 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("sourceText");
  decl->add_arg<const char * > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("disambiguation", true, "nullptr");
  decl->add_arg<const char * > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("n", true, "-1");
  decl->add_arg<int > (argspec_2);
  decl->set_return<QString > ();
}

static void _call_f_tr_4013 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const char *arg1 = gsi::arg_reader<const char * >() (args, heap);
  const char *arg2 = args ? gsi::arg_reader<const char * >() (args, heap) : gsi::arg_maker<const char * >() (nullptr, heap);
  int arg3 = args ? gsi::arg_reader<int >() (args, heap) : gsi::arg_maker<int >() (-1, heap);
  ret.write<QString > ((QString)QSystemSemaphore::tr (arg1, arg2, arg3));
}



namespace gsi
{

static gsi::Methods methods_QSystemSemaphore () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QSystemSemaphore::QSystemSemaphore(const QString &key, int initialValue, QSystemSemaphore::AccessMode mode)\nThis method creates an object of class QSystemSemaphore.", &_init_ctor_QSystemSemaphore_5769, &_call_ctor_QSystemSemaphore_5769);
  methods += new qt_gsi::GenericMethod ("acquire", "@brief Method bool QSystemSemaphore::acquire()\n", false, &_init_f_acquire_0, &_call_f_acquire_0);
  methods += new qt_gsi::GenericMethod ("error", "@brief Method QSystemSemaphore::SystemSemaphoreError QSystemSemaphore::error()\n", true, &_init_f_error_c0, &_call_f_error_c0);
  methods += new qt_gsi::GenericMethod ("errorString", "@brief Method QString QSystemSemaphore::errorString()\n", true, &_init_f_errorString_c0, &_call_f_errorString_c0);
  methods += new qt_gsi::GenericMethod (":key", "@brief Method QString QSystemSemaphore::key()\n", true, &_init_f_key_c0, &_call_f_key_c0);
  methods += new qt_gsi::GenericMethod ("release", "@brief Method bool QSystemSemaphore::release(int n)\n", false, &_init_f_release_767, &_call_f_release_767);
  methods += new qt_gsi::GenericMethod ("setKey", "@brief Method void QSystemSemaphore::setKey(const QString &key, int initialValue, QSystemSemaphore::AccessMode mode)\n", false, &_init_f_setKey_5769, &_call_f_setKey_5769);
  methods += new qt_gsi::GenericStaticMethod ("tr", "@brief Static method QString QSystemSemaphore::tr(const char *sourceText, const char *disambiguation, int n)\nThis method is static and can be called without an instance.", &_init_f_tr_4013, &_call_f_tr_4013);
  return methods;
}

gsi::Class<QSystemSemaphore> decl_QSystemSemaphore ("QtCore", "QSystemSemaphore",
  methods_QSystemSemaphore (),
  "@qt\n@brief Binding of QSystemSemaphore");


GSI_QTCORE_PUBLIC gsi::Class<QSystemSemaphore> &qtdecl_QSystemSemaphore () { return decl_QSystemSemaphore; }

}


//  Implementation of the enum wrapper class for QSystemSemaphore::AccessMode
namespace qt_gsi
{

static gsi::Enum<QSystemSemaphore::AccessMode> decl_QSystemSemaphore_AccessMode_Enum ("QtCore", "QSystemSemaphore_AccessMode",
    gsi::enum_const ("Open", QSystemSemaphore::Open, "@brief Enum constant QSystemSemaphore::Open") +
    gsi::enum_const ("Create", QSystemSemaphore::Create, "@brief Enum constant QSystemSemaphore::Create"),
  "@qt\n@brief This class represents the QSystemSemaphore::AccessMode enum");

static gsi::QFlagsClass<QSystemSemaphore::AccessMode > decl_QSystemSemaphore_AccessMode_Enums ("QtCore", "QSystemSemaphore_QFlags_AccessMode",
  "@qt\n@brief This class represents the QFlags<QSystemSemaphore::AccessMode> flag set");

//  Inject the declarations into the parent
static gsi::ClassExt<QSystemSemaphore> inject_QSystemSemaphore_AccessMode_Enum_in_parent (decl_QSystemSemaphore_AccessMode_Enum.defs ());
static gsi::ClassExt<QSystemSemaphore> decl_QSystemSemaphore_AccessMode_Enum_as_child (decl_QSystemSemaphore_AccessMode_Enum, "AccessMode");
static gsi::ClassExt<QSystemSemaphore> decl_QSystemSemaphore_AccessMode_Enums_as_child (decl_QSystemSemaphore_AccessMode_Enums, "QFlags_AccessMode");

}


//  Implementation of the enum wrapper class for QSystemSemaphore::SystemSemaphoreError
namespace qt_gsi
{

static gsi::Enum<QSystemSemaphore::SystemSemaphoreError> decl_QSystemSemaphore_SystemSemaphoreError_Enum ("QtCore", "QSystemSemaphore_SystemSemaphoreError",
    gsi::enum_const ("NoError", QSystemSemaphore::NoError, "@brief Enum constant QSystemSemaphore::NoError") +
    gsi::enum_const ("PermissionDenied", QSystemSemaphore::PermissionDenied, "@brief Enum constant QSystemSemaphore::PermissionDenied") +
    gsi::enum_const ("KeyError", QSystemSemaphore::KeyError, "@brief Enum constant QSystemSemaphore::KeyError") +
    gsi::enum_const ("AlreadyExists", QSystemSemaphore::AlreadyExists, "@brief Enum constant QSystemSemaphore::AlreadyExists") +
    gsi::enum_const ("NotFound", QSystemSemaphore::NotFound, "@brief Enum constant QSystemSemaphore::NotFound") +
    gsi::enum_const ("OutOfResources", QSystemSemaphore::OutOfResources, "@brief Enum constant QSystemSemaphore::OutOfResources") +
    gsi::enum_const ("UnknownError", QSystemSemaphore::UnknownError, "@brief Enum constant QSystemSemaphore::UnknownError"),
  "@qt\n@brief This class represents the QSystemSemaphore::SystemSemaphoreError enum");

static gsi::QFlagsClass<QSystemSemaphore::SystemSemaphoreError > decl_QSystemSemaphore_SystemSemaphoreError_Enums ("QtCore", "QSystemSemaphore_QFlags_SystemSemaphoreError",
  "@qt\n@brief This class represents the QFlags<QSystemSemaphore::SystemSemaphoreError> flag set");

//  Inject the declarations into the parent
static gsi::ClassExt<QSystemSemaphore> inject_QSystemSemaphore_SystemSemaphoreError_Enum_in_parent (decl_QSystemSemaphore_SystemSemaphoreError_Enum.defs ());
static gsi::ClassExt<QSystemSemaphore> decl_QSystemSemaphore_SystemSemaphoreError_Enum_as_child (decl_QSystemSemaphore_SystemSemaphoreError_Enum, "SystemSemaphoreError");
static gsi::ClassExt<QSystemSemaphore> decl_QSystemSemaphore_SystemSemaphoreError_Enums_as_child (decl_QSystemSemaphore_SystemSemaphoreError_Enums, "QFlags_SystemSemaphoreError");

}

