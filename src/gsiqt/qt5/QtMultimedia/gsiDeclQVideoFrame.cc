
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
*  @file gsiDeclQVideoFrame.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QVideoFrame>
#include <QAbstractVideoBuffer>
#include <QImage>
#include <QSize>
#include "gsiQt.h"
#include "gsiQtMultimediaCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QVideoFrame

//  Constructor QVideoFrame::QVideoFrame()


static void _init_ctor_QVideoFrame_0 (qt_gsi::GenericStaticMethod *decl)
{
  decl->set_return_new<QVideoFrame> ();
}

static void _call_ctor_QVideoFrame_0 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QVideoFrame *> (new QVideoFrame ());
}


//  Constructor QVideoFrame::QVideoFrame(QAbstractVideoBuffer *buffer, const QSize &size, QVideoFrame::PixelFormat format)


static void _init_ctor_QVideoFrame_6975 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("buffer");
  decl->add_arg<QAbstractVideoBuffer * > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("size");
  decl->add_arg<const QSize & > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("format");
  decl->add_arg<const qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type & > (argspec_2);
  decl->set_return_new<QVideoFrame> ();
}

static void _call_ctor_QVideoFrame_6975 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QAbstractVideoBuffer *arg1 = gsi::arg_reader<QAbstractVideoBuffer * >() (args, heap);
  const QSize &arg2 = gsi::arg_reader<const QSize & >() (args, heap);
  const qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type & arg3 = gsi::arg_reader<const qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type & >() (args, heap);
  ret.write<QVideoFrame *> (new QVideoFrame (arg1, arg2, qt_gsi::QtToCppAdaptor<QVideoFrame::PixelFormat>(arg3).cref()));
}


//  Constructor QVideoFrame::QVideoFrame(int bytes, const QSize &size, int bytesPerLine, QVideoFrame::PixelFormat format)


static void _init_ctor_QVideoFrame_5773 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("bytes");
  decl->add_arg<int > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("size");
  decl->add_arg<const QSize & > (argspec_1);
  static gsi::ArgSpecBase argspec_2 ("bytesPerLine");
  decl->add_arg<int > (argspec_2);
  static gsi::ArgSpecBase argspec_3 ("format");
  decl->add_arg<const qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type & > (argspec_3);
  decl->set_return_new<QVideoFrame> ();
}

static void _call_ctor_QVideoFrame_5773 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  const QSize &arg2 = gsi::arg_reader<const QSize & >() (args, heap);
  int arg3 = gsi::arg_reader<int >() (args, heap);
  const qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type & arg4 = gsi::arg_reader<const qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type & >() (args, heap);
  ret.write<QVideoFrame *> (new QVideoFrame (arg1, arg2, arg3, qt_gsi::QtToCppAdaptor<QVideoFrame::PixelFormat>(arg4).cref()));
}


//  Constructor QVideoFrame::QVideoFrame(const QImage &image)


static void _init_ctor_QVideoFrame_1877 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("image");
  decl->add_arg<const QImage & > (argspec_0);
  decl->set_return_new<QVideoFrame> ();
}

static void _call_ctor_QVideoFrame_1877 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QImage &arg1 = gsi::arg_reader<const QImage & >() (args, heap);
  ret.write<QVideoFrame *> (new QVideoFrame (arg1));
}


//  Constructor QVideoFrame::QVideoFrame(const QVideoFrame &other)


static void _init_ctor_QVideoFrame_2388 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QVideoFrame & > (argspec_0);
  decl->set_return_new<QVideoFrame> ();
}

static void _call_ctor_QVideoFrame_2388 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QVideoFrame &arg1 = gsi::arg_reader<const QVideoFrame & >() (args, heap);
  ret.write<QVideoFrame *> (new QVideoFrame (arg1));
}


// QMap<QString, QVariant> QVideoFrame::availableMetaData()


static void _init_f_availableMetaData_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QMap<QString, QVariant> > ();
}

static void _call_f_availableMetaData_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QMap<QString, QVariant> > ((QMap<QString, QVariant>)((QVideoFrame *)cls)->availableMetaData ());
}


// const unsigned char *QVideoFrame::bits()


static void _init_f_bits_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<const unsigned char * > ();
}

static void _call_f_bits_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<const unsigned char * > ((const unsigned char *)((QVideoFrame *)cls)->bits ());
}


// const unsigned char *QVideoFrame::bits(int plane)


static void _init_f_bits_c767 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("plane");
  decl->add_arg<int > (argspec_0);
  decl->set_return<const unsigned char * > ();
}

static void _call_f_bits_c767 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  ret.write<const unsigned char * > ((const unsigned char *)((QVideoFrame *)cls)->bits (arg1));
}


// int QVideoFrame::bytesPerLine()


static void _init_f_bytesPerLine_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_bytesPerLine_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QVideoFrame *)cls)->bytesPerLine ());
}


// int QVideoFrame::bytesPerLine(int plane)


static void _init_f_bytesPerLine_c767 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("plane");
  decl->add_arg<int > (argspec_0);
  decl->set_return<int > ();
}

static void _call_f_bytesPerLine_c767 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  int arg1 = gsi::arg_reader<int >() (args, heap);
  ret.write<int > ((int)((QVideoFrame *)cls)->bytesPerLine (arg1));
}


// qint64 QVideoFrame::endTime()


static void _init_f_endTime_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<qint64 > ();
}

static void _call_f_endTime_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<qint64 > ((qint64)((QVideoFrame *)cls)->endTime ());
}


// QVideoFrame::FieldType QVideoFrame::fieldType()


static void _init_f_fieldType_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<qt_gsi::Converter<QVideoFrame::FieldType>::target_type > ();
}

static void _call_f_fieldType_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<qt_gsi::Converter<QVideoFrame::FieldType>::target_type > ((qt_gsi::Converter<QVideoFrame::FieldType>::target_type)qt_gsi::CppToQtAdaptor<QVideoFrame::FieldType>(((QVideoFrame *)cls)->fieldType ()));
}


// QVariant QVideoFrame::handle()


static void _init_f_handle_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QVariant > ();
}

static void _call_f_handle_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QVariant > ((QVariant)((QVideoFrame *)cls)->handle ());
}


// QAbstractVideoBuffer::HandleType QVideoFrame::handleType()


static void _init_f_handleType_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<qt_gsi::Converter<QAbstractVideoBuffer::HandleType>::target_type > ();
}

static void _call_f_handleType_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<qt_gsi::Converter<QAbstractVideoBuffer::HandleType>::target_type > ((qt_gsi::Converter<QAbstractVideoBuffer::HandleType>::target_type)qt_gsi::CppToQtAdaptor<QAbstractVideoBuffer::HandleType>(((QVideoFrame *)cls)->handleType ()));
}


// int QVideoFrame::height()


static void _init_f_height_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_height_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QVideoFrame *)cls)->height ());
}


// bool QVideoFrame::isMapped()


static void _init_f_isMapped_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<bool > ();
}

static void _call_f_isMapped_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<bool > ((bool)((QVideoFrame *)cls)->isMapped ());
}


// bool QVideoFrame::isReadable()


static void _init_f_isReadable_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<bool > ();
}

static void _call_f_isReadable_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<bool > ((bool)((QVideoFrame *)cls)->isReadable ());
}


// bool QVideoFrame::isValid()


static void _init_f_isValid_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<bool > ();
}

static void _call_f_isValid_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<bool > ((bool)((QVideoFrame *)cls)->isValid ());
}


// bool QVideoFrame::isWritable()


static void _init_f_isWritable_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<bool > ();
}

static void _call_f_isWritable_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<bool > ((bool)((QVideoFrame *)cls)->isWritable ());
}


// bool QVideoFrame::map(QAbstractVideoBuffer::MapMode mode)


static void _init_f_map_3233 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("mode");
  decl->add_arg<const qt_gsi::Converter<QAbstractVideoBuffer::MapMode>::target_type & > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_map_3233 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<QAbstractVideoBuffer::MapMode>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<QAbstractVideoBuffer::MapMode>::target_type & >() (args, heap);
  ret.write<bool > ((bool)((QVideoFrame *)cls)->map (qt_gsi::QtToCppAdaptor<QAbstractVideoBuffer::MapMode>(arg1).cref()));
}


// QAbstractVideoBuffer::MapMode QVideoFrame::mapMode()


static void _init_f_mapMode_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<qt_gsi::Converter<QAbstractVideoBuffer::MapMode>::target_type > ();
}

static void _call_f_mapMode_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<qt_gsi::Converter<QAbstractVideoBuffer::MapMode>::target_type > ((qt_gsi::Converter<QAbstractVideoBuffer::MapMode>::target_type)qt_gsi::CppToQtAdaptor<QAbstractVideoBuffer::MapMode>(((QVideoFrame *)cls)->mapMode ()));
}


// int QVideoFrame::mappedBytes()


static void _init_f_mappedBytes_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_mappedBytes_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QVideoFrame *)cls)->mappedBytes ());
}


// QVariant QVideoFrame::metaData(const QString &key)


static void _init_f_metaData_c2025 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("key");
  decl->add_arg<const QString & > (argspec_0);
  decl->set_return<QVariant > ();
}

static void _call_f_metaData_c2025 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  ret.write<QVariant > ((QVariant)((QVideoFrame *)cls)->metaData (arg1));
}


// QVideoFrame &QVideoFrame::operator =(const QVideoFrame &other)


static void _init_f_operator_eq__2388 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QVideoFrame & > (argspec_0);
  decl->set_return<QVideoFrame & > ();
}

static void _call_f_operator_eq__2388 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QVideoFrame &arg1 = gsi::arg_reader<const QVideoFrame & >() (args, heap);
  ret.write<QVideoFrame & > ((QVideoFrame &)((QVideoFrame *)cls)->operator = (arg1));
}


// bool QVideoFrame::operator!=(const QVideoFrame &other)


static void _init_f_operator_excl__eq__c2388 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QVideoFrame & > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_operator_excl__eq__c2388 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QVideoFrame &arg1 = gsi::arg_reader<const QVideoFrame & >() (args, heap);
  ret.write<bool > ((bool)((QVideoFrame *)cls)->operator!= (arg1));
}


// bool QVideoFrame::operator==(const QVideoFrame &other)


static void _init_f_operator_eq__eq__c2388 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("other");
  decl->add_arg<const QVideoFrame & > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_operator_eq__eq__c2388 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QVideoFrame &arg1 = gsi::arg_reader<const QVideoFrame & >() (args, heap);
  ret.write<bool > ((bool)((QVideoFrame *)cls)->operator== (arg1));
}


// QVideoFrame::PixelFormat QVideoFrame::pixelFormat()


static void _init_f_pixelFormat_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type > ();
}

static void _call_f_pixelFormat_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type > ((qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type)qt_gsi::CppToQtAdaptor<QVideoFrame::PixelFormat>(((QVideoFrame *)cls)->pixelFormat ()));
}


// int QVideoFrame::planeCount()


static void _init_f_planeCount_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_planeCount_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QVideoFrame *)cls)->planeCount ());
}


// void QVideoFrame::setEndTime(qint64 time)


static void _init_f_setEndTime_986 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("time");
  decl->add_arg<qint64 > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setEndTime_986 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  qint64 arg1 = gsi::arg_reader<qint64 >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QVideoFrame *)cls)->setEndTime (arg1);
}


// void QVideoFrame::setFieldType(QVideoFrame::FieldType)


static void _init_f_setFieldType_2529 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("arg1");
  decl->add_arg<const qt_gsi::Converter<QVideoFrame::FieldType>::target_type & > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setFieldType_2529 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<QVideoFrame::FieldType>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<QVideoFrame::FieldType>::target_type & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QVideoFrame *)cls)->setFieldType (qt_gsi::QtToCppAdaptor<QVideoFrame::FieldType>(arg1).cref());
}


// void QVideoFrame::setMetaData(const QString &key, const QVariant &value)


static void _init_f_setMetaData_4036 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("key");
  decl->add_arg<const QString & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("value");
  decl->add_arg<const QVariant & > (argspec_1);
  decl->set_return<void > ();
}

static void _call_f_setMetaData_4036 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  const QVariant &arg2 = gsi::arg_reader<const QVariant & >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QVideoFrame *)cls)->setMetaData (arg1, arg2);
}


// void QVideoFrame::setStartTime(qint64 time)


static void _init_f_setStartTime_986 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("time");
  decl->add_arg<qint64 > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setStartTime_986 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  qint64 arg1 = gsi::arg_reader<qint64 >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QVideoFrame *)cls)->setStartTime (arg1);
}


// QSize QVideoFrame::size()


static void _init_f_size_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QSize > ();
}

static void _call_f_size_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QSize > ((QSize)((QVideoFrame *)cls)->size ());
}


// qint64 QVideoFrame::startTime()


static void _init_f_startTime_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<qint64 > ();
}

static void _call_f_startTime_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<qint64 > ((qint64)((QVideoFrame *)cls)->startTime ());
}


// void QVideoFrame::unmap()


static void _init_f_unmap_0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<void > ();
}

static void _call_f_unmap_0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QVideoFrame *)cls)->unmap ();
}


// int QVideoFrame::width()


static void _init_f_width_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<int > ();
}

static void _call_f_width_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<int > ((int)((QVideoFrame *)cls)->width ());
}


// static QImage::Format QVideoFrame::imageFormatFromPixelFormat(QVideoFrame::PixelFormat format)


static void _init_f_imageFormatFromPixelFormat_2758 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("format");
  decl->add_arg<const qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type & > (argspec_0);
  decl->set_return<qt_gsi::Converter<QImage::Format>::target_type > ();
}

static void _call_f_imageFormatFromPixelFormat_2758 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type & >() (args, heap);
  ret.write<qt_gsi::Converter<QImage::Format>::target_type > ((qt_gsi::Converter<QImage::Format>::target_type)qt_gsi::CppToQtAdaptor<QImage::Format>(QVideoFrame::imageFormatFromPixelFormat (qt_gsi::QtToCppAdaptor<QVideoFrame::PixelFormat>(arg1).cref())));
}


// static QVideoFrame::PixelFormat QVideoFrame::pixelFormatFromImageFormat(QImage::Format format)


static void _init_f_pixelFormatFromImageFormat_1733 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("format");
  decl->add_arg<const qt_gsi::Converter<QImage::Format>::target_type & > (argspec_0);
  decl->set_return<qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type > ();
}

static void _call_f_pixelFormatFromImageFormat_1733 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<QImage::Format>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<QImage::Format>::target_type & >() (args, heap);
  ret.write<qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type > ((qt_gsi::Converter<QVideoFrame::PixelFormat>::target_type)qt_gsi::CppToQtAdaptor<QVideoFrame::PixelFormat>(QVideoFrame::pixelFormatFromImageFormat (qt_gsi::QtToCppAdaptor<QImage::Format>(arg1).cref())));
}



namespace gsi
{

static gsi::Methods methods_QVideoFrame () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QVideoFrame::QVideoFrame()\nThis method creates an object of class QVideoFrame.", &_init_ctor_QVideoFrame_0, &_call_ctor_QVideoFrame_0);
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QVideoFrame::QVideoFrame(QAbstractVideoBuffer *buffer, const QSize &size, QVideoFrame::PixelFormat format)\nThis method creates an object of class QVideoFrame.", &_init_ctor_QVideoFrame_6975, &_call_ctor_QVideoFrame_6975);
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QVideoFrame::QVideoFrame(int bytes, const QSize &size, int bytesPerLine, QVideoFrame::PixelFormat format)\nThis method creates an object of class QVideoFrame.", &_init_ctor_QVideoFrame_5773, &_call_ctor_QVideoFrame_5773);
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QVideoFrame::QVideoFrame(const QImage &image)\nThis method creates an object of class QVideoFrame.", &_init_ctor_QVideoFrame_1877, &_call_ctor_QVideoFrame_1877);
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QVideoFrame::QVideoFrame(const QVideoFrame &other)\nThis method creates an object of class QVideoFrame.", &_init_ctor_QVideoFrame_2388, &_call_ctor_QVideoFrame_2388);
  methods += new qt_gsi::GenericMethod ("availableMetaData", "@brief Method QMap<QString, QVariant> QVideoFrame::availableMetaData()\n", true, &_init_f_availableMetaData_c0, &_call_f_availableMetaData_c0);
  methods += new qt_gsi::GenericMethod ("bits", "@brief Method const unsigned char *QVideoFrame::bits()\n", true, &_init_f_bits_c0, &_call_f_bits_c0);
  methods += new qt_gsi::GenericMethod ("bits", "@brief Method const unsigned char *QVideoFrame::bits(int plane)\n", true, &_init_f_bits_c767, &_call_f_bits_c767);
  methods += new qt_gsi::GenericMethod ("bytesPerLine", "@brief Method int QVideoFrame::bytesPerLine()\n", true, &_init_f_bytesPerLine_c0, &_call_f_bytesPerLine_c0);
  methods += new qt_gsi::GenericMethod ("bytesPerLine", "@brief Method int QVideoFrame::bytesPerLine(int plane)\n", true, &_init_f_bytesPerLine_c767, &_call_f_bytesPerLine_c767);
  methods += new qt_gsi::GenericMethod (":endTime", "@brief Method qint64 QVideoFrame::endTime()\n", true, &_init_f_endTime_c0, &_call_f_endTime_c0);
  methods += new qt_gsi::GenericMethod (":fieldType", "@brief Method QVideoFrame::FieldType QVideoFrame::fieldType()\n", true, &_init_f_fieldType_c0, &_call_f_fieldType_c0);
  methods += new qt_gsi::GenericMethod ("handle", "@brief Method QVariant QVideoFrame::handle()\n", true, &_init_f_handle_c0, &_call_f_handle_c0);
  methods += new qt_gsi::GenericMethod ("handleType", "@brief Method QAbstractVideoBuffer::HandleType QVideoFrame::handleType()\n", true, &_init_f_handleType_c0, &_call_f_handleType_c0);
  methods += new qt_gsi::GenericMethod ("height", "@brief Method int QVideoFrame::height()\n", true, &_init_f_height_c0, &_call_f_height_c0);
  methods += new qt_gsi::GenericMethod ("isMapped?", "@brief Method bool QVideoFrame::isMapped()\n", true, &_init_f_isMapped_c0, &_call_f_isMapped_c0);
  methods += new qt_gsi::GenericMethod ("isReadable?", "@brief Method bool QVideoFrame::isReadable()\n", true, &_init_f_isReadable_c0, &_call_f_isReadable_c0);
  methods += new qt_gsi::GenericMethod ("isValid?", "@brief Method bool QVideoFrame::isValid()\n", true, &_init_f_isValid_c0, &_call_f_isValid_c0);
  methods += new qt_gsi::GenericMethod ("isWritable?", "@brief Method bool QVideoFrame::isWritable()\n", true, &_init_f_isWritable_c0, &_call_f_isWritable_c0);
  methods += new qt_gsi::GenericMethod ("map", "@brief Method bool QVideoFrame::map(QAbstractVideoBuffer::MapMode mode)\n", false, &_init_f_map_3233, &_call_f_map_3233);
  methods += new qt_gsi::GenericMethod ("mapMode", "@brief Method QAbstractVideoBuffer::MapMode QVideoFrame::mapMode()\n", true, &_init_f_mapMode_c0, &_call_f_mapMode_c0);
  methods += new qt_gsi::GenericMethod ("mappedBytes", "@brief Method int QVideoFrame::mappedBytes()\n", true, &_init_f_mappedBytes_c0, &_call_f_mappedBytes_c0);
  methods += new qt_gsi::GenericMethod ("metaData", "@brief Method QVariant QVideoFrame::metaData(const QString &key)\n", true, &_init_f_metaData_c2025, &_call_f_metaData_c2025);
  methods += new qt_gsi::GenericMethod ("assign", "@brief Method QVideoFrame &QVideoFrame::operator =(const QVideoFrame &other)\n", false, &_init_f_operator_eq__2388, &_call_f_operator_eq__2388);
  methods += new qt_gsi::GenericMethod ("!=", "@brief Method bool QVideoFrame::operator!=(const QVideoFrame &other)\n", true, &_init_f_operator_excl__eq__c2388, &_call_f_operator_excl__eq__c2388);
  methods += new qt_gsi::GenericMethod ("==", "@brief Method bool QVideoFrame::operator==(const QVideoFrame &other)\n", true, &_init_f_operator_eq__eq__c2388, &_call_f_operator_eq__eq__c2388);
  methods += new qt_gsi::GenericMethod ("pixelFormat", "@brief Method QVideoFrame::PixelFormat QVideoFrame::pixelFormat()\n", true, &_init_f_pixelFormat_c0, &_call_f_pixelFormat_c0);
  methods += new qt_gsi::GenericMethod ("planeCount", "@brief Method int QVideoFrame::planeCount()\n", true, &_init_f_planeCount_c0, &_call_f_planeCount_c0);
  methods += new qt_gsi::GenericMethod ("setEndTime|endTime=", "@brief Method void QVideoFrame::setEndTime(qint64 time)\n", false, &_init_f_setEndTime_986, &_call_f_setEndTime_986);
  methods += new qt_gsi::GenericMethod ("setFieldType|fieldType=", "@brief Method void QVideoFrame::setFieldType(QVideoFrame::FieldType)\n", false, &_init_f_setFieldType_2529, &_call_f_setFieldType_2529);
  methods += new qt_gsi::GenericMethod ("setMetaData", "@brief Method void QVideoFrame::setMetaData(const QString &key, const QVariant &value)\n", false, &_init_f_setMetaData_4036, &_call_f_setMetaData_4036);
  methods += new qt_gsi::GenericMethod ("setStartTime|startTime=", "@brief Method void QVideoFrame::setStartTime(qint64 time)\n", false, &_init_f_setStartTime_986, &_call_f_setStartTime_986);
  methods += new qt_gsi::GenericMethod ("size", "@brief Method QSize QVideoFrame::size()\n", true, &_init_f_size_c0, &_call_f_size_c0);
  methods += new qt_gsi::GenericMethod (":startTime", "@brief Method qint64 QVideoFrame::startTime()\n", true, &_init_f_startTime_c0, &_call_f_startTime_c0);
  methods += new qt_gsi::GenericMethod ("unmap", "@brief Method void QVideoFrame::unmap()\n", false, &_init_f_unmap_0, &_call_f_unmap_0);
  methods += new qt_gsi::GenericMethod ("width", "@brief Method int QVideoFrame::width()\n", true, &_init_f_width_c0, &_call_f_width_c0);
  methods += new qt_gsi::GenericStaticMethod ("imageFormatFromPixelFormat", "@brief Static method QImage::Format QVideoFrame::imageFormatFromPixelFormat(QVideoFrame::PixelFormat format)\nThis method is static and can be called without an instance.", &_init_f_imageFormatFromPixelFormat_2758, &_call_f_imageFormatFromPixelFormat_2758);
  methods += new qt_gsi::GenericStaticMethod ("pixelFormatFromImageFormat", "@brief Static method QVideoFrame::PixelFormat QVideoFrame::pixelFormatFromImageFormat(QImage::Format format)\nThis method is static and can be called without an instance.", &_init_f_pixelFormatFromImageFormat_1733, &_call_f_pixelFormatFromImageFormat_1733);
  return methods;
}

gsi::Class<QVideoFrame> decl_QVideoFrame ("QtMultimedia", "QVideoFrame",
  methods_QVideoFrame (),
  "@qt\n@brief Binding of QVideoFrame");


GSI_QTMULTIMEDIA_PUBLIC gsi::Class<QVideoFrame> &qtdecl_QVideoFrame () { return decl_QVideoFrame; }

}


//  Implementation of the enum wrapper class for QVideoFrame::FieldType
namespace qt_gsi
{

static gsi::Enum<QVideoFrame::FieldType> decl_QVideoFrame_FieldType_Enum ("QtMultimedia", "QVideoFrame_FieldType",
    gsi::enum_const ("ProgressiveFrame", QVideoFrame::ProgressiveFrame, "@brief Enum constant QVideoFrame::ProgressiveFrame") +
    gsi::enum_const ("TopField", QVideoFrame::TopField, "@brief Enum constant QVideoFrame::TopField") +
    gsi::enum_const ("BottomField", QVideoFrame::BottomField, "@brief Enum constant QVideoFrame::BottomField") +
    gsi::enum_const ("InterlacedFrame", QVideoFrame::InterlacedFrame, "@brief Enum constant QVideoFrame::InterlacedFrame"),
  "@qt\n@brief This class represents the QVideoFrame::FieldType enum");

static gsi::QFlagsClass<QVideoFrame::FieldType > decl_QVideoFrame_FieldType_Enums ("QtMultimedia", "QVideoFrame_QFlags_FieldType",
  "@qt\n@brief This class represents the QFlags<QVideoFrame::FieldType> flag set");

//  Inject the declarations into the parent
static gsi::ClassExt<QVideoFrame> inject_QVideoFrame_FieldType_Enum_in_parent (decl_QVideoFrame_FieldType_Enum.defs ());
static gsi::ClassExt<QVideoFrame> decl_QVideoFrame_FieldType_Enum_as_child (decl_QVideoFrame_FieldType_Enum, "FieldType");
static gsi::ClassExt<QVideoFrame> decl_QVideoFrame_FieldType_Enums_as_child (decl_QVideoFrame_FieldType_Enums, "QFlags_FieldType");

}


//  Implementation of the enum wrapper class for QVideoFrame::PixelFormat
namespace qt_gsi
{

static gsi::Enum<QVideoFrame::PixelFormat> decl_QVideoFrame_PixelFormat_Enum ("QtMultimedia", "QVideoFrame_PixelFormat",
    gsi::enum_const ("Format_Invalid", QVideoFrame::Format_Invalid, "@brief Enum constant QVideoFrame::Format_Invalid") +
    gsi::enum_const ("Format_ARGB32", QVideoFrame::Format_ARGB32, "@brief Enum constant QVideoFrame::Format_ARGB32") +
    gsi::enum_const ("Format_ARGB32_Premultiplied", QVideoFrame::Format_ARGB32_Premultiplied, "@brief Enum constant QVideoFrame::Format_ARGB32_Premultiplied") +
    gsi::enum_const ("Format_RGB32", QVideoFrame::Format_RGB32, "@brief Enum constant QVideoFrame::Format_RGB32") +
    gsi::enum_const ("Format_RGB24", QVideoFrame::Format_RGB24, "@brief Enum constant QVideoFrame::Format_RGB24") +
    gsi::enum_const ("Format_RGB565", QVideoFrame::Format_RGB565, "@brief Enum constant QVideoFrame::Format_RGB565") +
    gsi::enum_const ("Format_RGB555", QVideoFrame::Format_RGB555, "@brief Enum constant QVideoFrame::Format_RGB555") +
    gsi::enum_const ("Format_ARGB8565_Premultiplied", QVideoFrame::Format_ARGB8565_Premultiplied, "@brief Enum constant QVideoFrame::Format_ARGB8565_Premultiplied") +
    gsi::enum_const ("Format_BGRA32", QVideoFrame::Format_BGRA32, "@brief Enum constant QVideoFrame::Format_BGRA32") +
    gsi::enum_const ("Format_BGRA32_Premultiplied", QVideoFrame::Format_BGRA32_Premultiplied, "@brief Enum constant QVideoFrame::Format_BGRA32_Premultiplied") +
    gsi::enum_const ("Format_BGR32", QVideoFrame::Format_BGR32, "@brief Enum constant QVideoFrame::Format_BGR32") +
    gsi::enum_const ("Format_BGR24", QVideoFrame::Format_BGR24, "@brief Enum constant QVideoFrame::Format_BGR24") +
    gsi::enum_const ("Format_BGR565", QVideoFrame::Format_BGR565, "@brief Enum constant QVideoFrame::Format_BGR565") +
    gsi::enum_const ("Format_BGR555", QVideoFrame::Format_BGR555, "@brief Enum constant QVideoFrame::Format_BGR555") +
    gsi::enum_const ("Format_BGRA5658_Premultiplied", QVideoFrame::Format_BGRA5658_Premultiplied, "@brief Enum constant QVideoFrame::Format_BGRA5658_Premultiplied") +
    gsi::enum_const ("Format_AYUV444", QVideoFrame::Format_AYUV444, "@brief Enum constant QVideoFrame::Format_AYUV444") +
    gsi::enum_const ("Format_AYUV444_Premultiplied", QVideoFrame::Format_AYUV444_Premultiplied, "@brief Enum constant QVideoFrame::Format_AYUV444_Premultiplied") +
    gsi::enum_const ("Format_YUV444", QVideoFrame::Format_YUV444, "@brief Enum constant QVideoFrame::Format_YUV444") +
    gsi::enum_const ("Format_YUV420P", QVideoFrame::Format_YUV420P, "@brief Enum constant QVideoFrame::Format_YUV420P") +
    gsi::enum_const ("Format_YV12", QVideoFrame::Format_YV12, "@brief Enum constant QVideoFrame::Format_YV12") +
    gsi::enum_const ("Format_UYVY", QVideoFrame::Format_UYVY, "@brief Enum constant QVideoFrame::Format_UYVY") +
    gsi::enum_const ("Format_YUYV", QVideoFrame::Format_YUYV, "@brief Enum constant QVideoFrame::Format_YUYV") +
    gsi::enum_const ("Format_NV12", QVideoFrame::Format_NV12, "@brief Enum constant QVideoFrame::Format_NV12") +
    gsi::enum_const ("Format_NV21", QVideoFrame::Format_NV21, "@brief Enum constant QVideoFrame::Format_NV21") +
    gsi::enum_const ("Format_IMC1", QVideoFrame::Format_IMC1, "@brief Enum constant QVideoFrame::Format_IMC1") +
    gsi::enum_const ("Format_IMC2", QVideoFrame::Format_IMC2, "@brief Enum constant QVideoFrame::Format_IMC2") +
    gsi::enum_const ("Format_IMC3", QVideoFrame::Format_IMC3, "@brief Enum constant QVideoFrame::Format_IMC3") +
    gsi::enum_const ("Format_IMC4", QVideoFrame::Format_IMC4, "@brief Enum constant QVideoFrame::Format_IMC4") +
    gsi::enum_const ("Format_Y8", QVideoFrame::Format_Y8, "@brief Enum constant QVideoFrame::Format_Y8") +
    gsi::enum_const ("Format_Y16", QVideoFrame::Format_Y16, "@brief Enum constant QVideoFrame::Format_Y16") +
    gsi::enum_const ("Format_Jpeg", QVideoFrame::Format_Jpeg, "@brief Enum constant QVideoFrame::Format_Jpeg") +
    gsi::enum_const ("Format_CameraRaw", QVideoFrame::Format_CameraRaw, "@brief Enum constant QVideoFrame::Format_CameraRaw") +
    gsi::enum_const ("Format_AdobeDng", QVideoFrame::Format_AdobeDng, "@brief Enum constant QVideoFrame::Format_AdobeDng") +
    gsi::enum_const ("NPixelFormats", QVideoFrame::NPixelFormats, "@brief Enum constant QVideoFrame::NPixelFormats") +
    gsi::enum_const ("Format_User", QVideoFrame::Format_User, "@brief Enum constant QVideoFrame::Format_User"),
  "@qt\n@brief This class represents the QVideoFrame::PixelFormat enum");

static gsi::QFlagsClass<QVideoFrame::PixelFormat > decl_QVideoFrame_PixelFormat_Enums ("QtMultimedia", "QVideoFrame_QFlags_PixelFormat",
  "@qt\n@brief This class represents the QFlags<QVideoFrame::PixelFormat> flag set");

//  Inject the declarations into the parent
static gsi::ClassExt<QVideoFrame> inject_QVideoFrame_PixelFormat_Enum_in_parent (decl_QVideoFrame_PixelFormat_Enum.defs ());
static gsi::ClassExt<QVideoFrame> decl_QVideoFrame_PixelFormat_Enum_as_child (decl_QVideoFrame_PixelFormat_Enum, "PixelFormat");
static gsi::ClassExt<QVideoFrame> decl_QVideoFrame_PixelFormat_Enums_as_child (decl_QVideoFrame_PixelFormat_Enums, "QFlags_PixelFormat");

}

