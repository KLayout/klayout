
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
*  @file gsiDeclQTime.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QTime>
#include "gsiQt.h"
#include "gsiQtCoreCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QTime

//  Constructor QTime::QTime()


static void _init_ctor_QTime_0 (qt_gsi::GenericStaticMethod *decl)
{
  decl->set_return_new<QTime> ();
}

static void _call_ctor_QTime_0 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QTime *> (new QTime ());
}


//  Constructor QTime::QTime(int h, int m, int s, int ms)


static void _init_ctor_QTime_2744 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("h");
  decl->add_arg<int > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("m");
  decl->add_arg<int > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("s", true, "0");
  decl->add_arg<int > (argspec_2);
  static gsi::ArgSpecBase argspec_3 ("ms", true, "0");
  decl->add_arg<int > (argspec_3);
  decl->set_return_new<QTime> ();
}

static void _call_ctor_QTime_2744 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  int arg2 = gsi::arg_reader<int >() (args, heap);
  int arg3 = args ? gsi::arg_reader<int >() (args, heap) : gsi::arg_maker<int >() (0, heap);
  int arg4 = args ? gsi::arg_reader<int >() (args, heap) : gsi::arg_maker<int >() (0, heap);
  ret.write<QTime *> (new QTime (arg1, arg2, arg3, arg4));
}


// QTime QTime::addMSecs(int ms)


static void _init_f_addMSecs_c767 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("ms");
  decl->add_arg<int > (argspec_0);
  decl->set_return<QTime > ();
}

static void _call_f_addMSecs_c767 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  ret.write<QTime > ((QTime)((QTime *)cls)->addMSecs (arg1));
}


// QTime QTime::addSecs(int secs)


static void _init_f_addSecs_c767 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("secs");
  decl->add_arg<int > (argspec_0);
  decl->set_return<QTime > ();
}

static void _call_f_addSecs_c767 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  ret.write<QTime > ((QTime)((QTime *)cls)->addSecs (arg1));
}


// int QTime::hour()


static void _init_f_hour_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_hour_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QTime *)cls)->hour ());
}


// bool QTime::isNull()


static void _init_f_isNull_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<bool > ();
}

static void _call_f_isNull_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<bool > ((bool)((QTime *)cls)->isNull ());
}


// bool QTime::isValid()


static void _init_f_isValid_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<bool > ();
}

static void _call_f_isValid_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<bool > ((bool)((QTime *)cls)->isValid ());
}


// int QTime::minute()


static void _init_f_minute_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_minute_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QTime *)cls)->minute ());
}


// int QTime::msec()


static void _init_f_msec_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_msec_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QTime *)cls)->msec ());
}


// int QTime::msecsSinceStartOfDay()


static void _init_f_msecsSinceStartOfDay_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_msecsSinceStartOfDay_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QTime *)cls)->msecsSinceStartOfDay ());
}


// int QTime::msecsTo(QTime t)


static void _init_f_msecsTo_c916 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("t");
  decl->add_arg<QTime > (argspec_0);
  decl->set_return<int > ();
}

static void _call_f_msecsTo_c916 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QTime arg1 = gsi::arg_reader<QTime >() (args, heap);
  ret.write<int > ((int)((QTime *)cls)->msecsTo (arg1));
}


// int QTime::second()


static void _init_f_second_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_second_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QTime *)cls)->second ());
}


// int QTime::secsTo(QTime t)


static void _init_f_secsTo_c916 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("t");
  decl->add_arg<QTime > (argspec_0);
  decl->set_return<int > ();
}

static void _call_f_secsTo_c916 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QTime arg1 = gsi::arg_reader<QTime >() (args, heap);
  ret.write<int > ((int)((QTime *)cls)->secsTo (arg1));
}


// bool QTime::setHMS(int h, int m, int s, int ms)


static void _init_f_setHMS_2744 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("h");
  decl->add_arg<int > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("m");
  decl->add_arg<int > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("s");
  decl->add_arg<int > (argspec_2);
  static gsi::ArgSpecBase argspec_3 ("ms", true, "0");
  decl->add_arg<int > (argspec_3);
  decl->set_return<bool > ();
}

static void _call_f_setHMS_2744 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  int arg2 = gsi::arg_reader<int >() (args, heap);
  int arg3 = gsi::arg_reader<int >() (args, heap);
  int arg4 = args ? gsi::arg_reader<int >() (args, heap) : gsi::arg_maker<int >() (0, heap);
  ret.write<bool > ((bool)((QTime *)cls)->setHMS (arg1, arg2, arg3, arg4));
}


// QString QTime::toString(Qt::DateFormat f)


static void _init_f_toString_c1748 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("f", true, "Qt::TextDate");
  decl->add_arg<const qt_gsi::Converter<Qt::DateFormat>::target_type & > (argspec_0);
  decl->set_return<QString > ();
}

static void _call_f_toString_c1748 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<Qt::DateFormat>::target_type & arg1 = args ? gsi::arg_reader<const qt_gsi::Converter<Qt::DateFormat>::target_type & >() (args, heap) : gsi::arg_maker<const qt_gsi::Converter<Qt::DateFormat>::target_type & >() (qt_gsi::CppToQtReadAdaptor<Qt::DateFormat>(heap, Qt::TextDate), heap);
  ret.write<QString > ((QString)((QTime *)cls)->toString (qt_gsi::QtToCppAdaptor<Qt::DateFormat>(arg1).cref()));
}


// QString QTime::toString(const QString &format)


static void _init_f_toString_c2025 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("format");
  decl->add_arg<const QString & > (argspec_0);
  decl->set_return<QString > ();
}

static void _call_f_toString_c2025 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  ret.write<QString > ((QString)((QTime *)cls)->toString (arg1));
}


// QString QTime::toString(QStringView format)


static void _init_f_toString_c1559 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("format");
  decl->add_arg<QStringView > (argspec_0);
  decl->set_return<QString > ();
}

static void _call_f_toString_c1559 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QStringView arg1 = gsi::arg_reader<QStringView >() (args, heap);
  ret.write<QString > ((QString)((QTime *)cls)->toString (arg1));
}


// static QTime QTime::currentTime()


static void _init_f_currentTime_0 (qt_gsi::GenericStaticMethod *decl)
{
  decl->set_return<QTime > ();
}

static void _call_f_currentTime_0 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QTime > ((QTime)QTime::currentTime ());
}


// static QTime QTime::fromMSecsSinceStartOfDay(int msecs)


static void _init_f_fromMSecsSinceStartOfDay_767 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("msecs");
  decl->add_arg<int > (argspec_0);
  decl->set_return<QTime > ();
}

static void _call_f_fromMSecsSinceStartOfDay_767 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  ret.write<QTime > ((QTime)QTime::fromMSecsSinceStartOfDay (arg1));
}


// static QTime QTime::fromString(QStringView string, Qt::DateFormat format)


static void _init_f_fromString_3199 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("string");
  decl->add_arg<QStringView > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("format", true, "Qt::TextDate");
  decl->add_arg<const qt_gsi::Converter<Qt::DateFormat>::target_type & > (argspec_1);
  decl->set_return<QTime > ();
}

static void _call_f_fromString_3199 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QStringView arg1 = gsi::arg_reader<QStringView >() (args, heap);
  const qt_gsi::Converter<Qt::DateFormat>::target_type & arg2 = args ? gsi::arg_reader<const qt_gsi::Converter<Qt::DateFormat>::target_type & >() (args, heap) : gsi::arg_maker<const qt_gsi::Converter<Qt::DateFormat>::target_type & >() (qt_gsi::CppToQtReadAdaptor<Qt::DateFormat>(heap, Qt::TextDate), heap);
  ret.write<QTime > ((QTime)QTime::fromString (arg1, qt_gsi::QtToCppAdaptor<Qt::DateFormat>(arg2).cref()));
}


// static QTime QTime::fromString(QStringView string, QStringView format)


static void _init_f_fromString_3010 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("string");
  decl->add_arg<QStringView > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("format");
  decl->add_arg<QStringView > (argspec_1);
  decl->set_return<QTime > ();
}

static void _call_f_fromString_3010 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QStringView arg1 = gsi::arg_reader<QStringView >() (args, heap);
  QStringView arg2 = gsi::arg_reader<QStringView >() (args, heap);
  ret.write<QTime > ((QTime)QTime::fromString (arg1, arg2));
}


// static QTime QTime::fromString(const QString &string, QStringView format)


static void _init_f_fromString_3476 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("string");
  decl->add_arg<const QString & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("format");
  decl->add_arg<QStringView > (argspec_1);
  decl->set_return<QTime > ();
}

static void _call_f_fromString_3476 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  QStringView arg2 = gsi::arg_reader<QStringView >() (args, heap);
  ret.write<QTime > ((QTime)QTime::fromString (arg1, arg2));
}


// static QTime QTime::fromString(const QString &string, Qt::DateFormat format)


static void _init_f_fromString_3665 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("string");
  decl->add_arg<const QString & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("format", true, "Qt::TextDate");
  decl->add_arg<const qt_gsi::Converter<Qt::DateFormat>::target_type & > (argspec_1);
  decl->set_return<QTime > ();
}

static void _call_f_fromString_3665 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  const qt_gsi::Converter<Qt::DateFormat>::target_type & arg2 = args ? gsi::arg_reader<const qt_gsi::Converter<Qt::DateFormat>::target_type & >() (args, heap) : gsi::arg_maker<const qt_gsi::Converter<Qt::DateFormat>::target_type & >() (qt_gsi::CppToQtReadAdaptor<Qt::DateFormat>(heap, Qt::TextDate), heap);
  ret.write<QTime > ((QTime)QTime::fromString (arg1, qt_gsi::QtToCppAdaptor<Qt::DateFormat>(arg2).cref()));
}


// static QTime QTime::fromString(const QString &string, const QString &format)


static void _init_f_fromString_3942 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("string");
  decl->add_arg<const QString & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("format");
  decl->add_arg<const QString & > (argspec_1);
  decl->set_return<QTime > ();
}

static void _call_f_fromString_3942 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  const QString &arg2 = gsi::arg_reader<const QString & >() (args, heap);
  ret.write<QTime > ((QTime)QTime::fromString (arg1, arg2));
}


// static bool QTime::isValid(int h, int m, int s, int ms)


static void _init_f_isValid_2744 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("h");
  decl->add_arg<int > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("m");
  decl->add_arg<int > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("s");
  decl->add_arg<int > (argspec_2);
  static gsi::ArgSpecBase argspec_3 ("ms", true, "0");
  decl->add_arg<int > (argspec_3);
  decl->set_return<bool > ();
}

static void _call_f_isValid_2744 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  int arg2 = gsi::arg_reader<int >() (args, heap);
  int arg3 = gsi::arg_reader<int >() (args, heap);
  int arg4 = args ? gsi::arg_reader<int >() (args, heap) : gsi::arg_maker<int >() (0, heap);
  ret.write<bool > ((bool)QTime::isValid (arg1, arg2, arg3, arg4));
}



namespace gsi
{

static gsi::Methods methods_QTime () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QTime::QTime()\nThis method creates an object of class QTime.", &_init_ctor_QTime_0, &_call_ctor_QTime_0);
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QTime::QTime(int h, int m, int s, int ms)\nThis method creates an object of class QTime.", &_init_ctor_QTime_2744, &_call_ctor_QTime_2744);
  methods += new qt_gsi::GenericMethod ("addMSecs", "@brief Method QTime QTime::addMSecs(int ms)\n", true, &_init_f_addMSecs_c767, &_call_f_addMSecs_c767);
  methods += new qt_gsi::GenericMethod ("addSecs", "@brief Method QTime QTime::addSecs(int secs)\n", true, &_init_f_addSecs_c767, &_call_f_addSecs_c767);
  methods += new qt_gsi::GenericMethod ("hour", "@brief Method int QTime::hour()\n", true, &_init_f_hour_c0, &_call_f_hour_c0);
  methods += new qt_gsi::GenericMethod ("isNull?", "@brief Method bool QTime::isNull()\n", true, &_init_f_isNull_c0, &_call_f_isNull_c0);
  methods += new qt_gsi::GenericMethod ("isValid?", "@brief Method bool QTime::isValid()\n", true, &_init_f_isValid_c0, &_call_f_isValid_c0);
  methods += new qt_gsi::GenericMethod ("minute", "@brief Method int QTime::minute()\n", true, &_init_f_minute_c0, &_call_f_minute_c0);
  methods += new qt_gsi::GenericMethod ("msec", "@brief Method int QTime::msec()\n", true, &_init_f_msec_c0, &_call_f_msec_c0);
  methods += new qt_gsi::GenericMethod ("msecsSinceStartOfDay", "@brief Method int QTime::msecsSinceStartOfDay()\n", true, &_init_f_msecsSinceStartOfDay_c0, &_call_f_msecsSinceStartOfDay_c0);
  methods += new qt_gsi::GenericMethod ("msecsTo", "@brief Method int QTime::msecsTo(QTime t)\n", true, &_init_f_msecsTo_c916, &_call_f_msecsTo_c916);
  methods += new qt_gsi::GenericMethod ("second", "@brief Method int QTime::second()\n", true, &_init_f_second_c0, &_call_f_second_c0);
  methods += new qt_gsi::GenericMethod ("secsTo", "@brief Method int QTime::secsTo(QTime t)\n", true, &_init_f_secsTo_c916, &_call_f_secsTo_c916);
  methods += new qt_gsi::GenericMethod ("setHMS", "@brief Method bool QTime::setHMS(int h, int m, int s, int ms)\n", false, &_init_f_setHMS_2744, &_call_f_setHMS_2744);
  methods += new qt_gsi::GenericMethod ("toString", "@brief Method QString QTime::toString(Qt::DateFormat f)\n", true, &_init_f_toString_c1748, &_call_f_toString_c1748);
  methods += new qt_gsi::GenericMethod ("toString", "@brief Method QString QTime::toString(const QString &format)\n", true, &_init_f_toString_c2025, &_call_f_toString_c2025);
  methods += new qt_gsi::GenericMethod ("toString", "@brief Method QString QTime::toString(QStringView format)\n", true, &_init_f_toString_c1559, &_call_f_toString_c1559);
  methods += new qt_gsi::GenericStaticMethod ("currentTime", "@brief Static method QTime QTime::currentTime()\nThis method is static and can be called without an instance.", &_init_f_currentTime_0, &_call_f_currentTime_0);
  methods += new qt_gsi::GenericStaticMethod ("fromMSecsSinceStartOfDay", "@brief Static method QTime QTime::fromMSecsSinceStartOfDay(int msecs)\nThis method is static and can be called without an instance.", &_init_f_fromMSecsSinceStartOfDay_767, &_call_f_fromMSecsSinceStartOfDay_767);
  methods += new qt_gsi::GenericStaticMethod ("fromString", "@brief Static method QTime QTime::fromString(QStringView string, Qt::DateFormat format)\nThis method is static and can be called without an instance.", &_init_f_fromString_3199, &_call_f_fromString_3199);
  methods += new qt_gsi::GenericStaticMethod ("fromString", "@brief Static method QTime QTime::fromString(QStringView string, QStringView format)\nThis method is static and can be called without an instance.", &_init_f_fromString_3010, &_call_f_fromString_3010);
  methods += new qt_gsi::GenericStaticMethod ("fromString", "@brief Static method QTime QTime::fromString(const QString &string, QStringView format)\nThis method is static and can be called without an instance.", &_init_f_fromString_3476, &_call_f_fromString_3476);
  methods += new qt_gsi::GenericStaticMethod ("fromString", "@brief Static method QTime QTime::fromString(const QString &string, Qt::DateFormat format)\nThis method is static and can be called without an instance.", &_init_f_fromString_3665, &_call_f_fromString_3665);
  methods += new qt_gsi::GenericStaticMethod ("fromString", "@brief Static method QTime QTime::fromString(const QString &string, const QString &format)\nThis method is static and can be called without an instance.", &_init_f_fromString_3942, &_call_f_fromString_3942);
  methods += new qt_gsi::GenericStaticMethod ("isValid?", "@brief Static method bool QTime::isValid(int h, int m, int s, int ms)\nThis method is static and can be called without an instance.", &_init_f_isValid_2744, &_call_f_isValid_2744);
  return methods;
}

gsi::Class<QTime> decl_QTime ("QtCore", "QTime",
  methods_QTime (),
  "@qt\n@brief Binding of QTime");


GSI_QTCORE_PUBLIC gsi::Class<QTime> &qtdecl_QTime () { return decl_QTime; }

}
