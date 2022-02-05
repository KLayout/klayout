
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
*  @file gsiDeclQStringEncoder.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QStringEncoder>
#include "gsiQt.h"
#include "gsiQtCoreCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QStringEncoder
  static QByteArray encode (QStringEncoder *encoder, const QString &in)
  {
    return encoder->encode (in); 
  }

//  Constructor QStringEncoder::QStringEncoder()


static void _init_ctor_QStringEncoder_0 (qt_gsi::GenericStaticMethod *decl)
{
  decl->set_return_new<QStringEncoder> ();
}

static void _call_ctor_QStringEncoder_0 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QStringEncoder *> (new QStringEncoder ());
}


//  Constructor QStringEncoder::QStringEncoder(QStringConverter::Encoding encoding, QFlags<QStringConverterBase::Flag> flags)


static void _init_ctor_QStringEncoder_6584 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("encoding");
  decl->add_arg<const qt_gsi::Converter<QStringConverter::Encoding>::target_type & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("flags", true, "QStringConverterBase::Flag::Default");
  decl->add_arg<QFlags<QStringConverterBase::Flag> > (argspec_1);
  decl->set_return_new<QStringEncoder> ();
}

static void _call_ctor_QStringEncoder_6584 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const qt_gsi::Converter<QStringConverter::Encoding>::target_type & arg1 = gsi::arg_reader<const qt_gsi::Converter<QStringConverter::Encoding>::target_type & >() (args, heap);
  QFlags<QStringConverterBase::Flag> arg2 = args ? gsi::arg_reader<QFlags<QStringConverterBase::Flag> >() (args, heap) : gsi::arg_maker<QFlags<QStringConverterBase::Flag> >() (QStringConverterBase::Flag::Default, heap);
  ret.write<QStringEncoder *> (new QStringEncoder (qt_gsi::QtToCppAdaptor<QStringConverter::Encoding>(arg1).cref(), arg2));
}


//  Constructor QStringEncoder::QStringEncoder(const char *name, QFlags<QStringConverterBase::Flag> flags)


static void _init_ctor_QStringEncoder_5292 (qt_gsi::GenericStaticMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("name");
  decl->add_arg<const char * > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("flags", true, "QStringConverterBase::Flag::Default");
  decl->add_arg<QFlags<QStringConverterBase::Flag> > (argspec_1);
  decl->set_return_new<QStringEncoder> ();
}

static void _call_ctor_QStringEncoder_5292 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const char *arg1 = gsi::arg_reader<const char * >() (args, heap);
  QFlags<QStringConverterBase::Flag> arg2 = args ? gsi::arg_reader<QFlags<QStringConverterBase::Flag> >() (args, heap) : gsi::arg_maker<QFlags<QStringConverterBase::Flag> >() (QStringConverterBase::Flag::Default, heap);
  ret.write<QStringEncoder *> (new QStringEncoder (arg1, arg2));
}


// qsizetype QStringEncoder::requiredSpace(qsizetype inputLength)


static void _init_f_requiredSpace_c1442 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("inputLength");
  decl->add_arg<qsizetype > (argspec_0);
  decl->set_return<qsizetype > ();
}

static void _call_f_requiredSpace_c1442 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  qsizetype arg1 = gsi::arg_reader<qsizetype >() (args, heap);
  ret.write<qsizetype > ((qsizetype)((QStringEncoder *)cls)->requiredSpace (arg1));
}



namespace gsi
{

static gsi::Methods methods_QStringEncoder () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QStringEncoder::QStringEncoder()\nThis method creates an object of class QStringEncoder.", &_init_ctor_QStringEncoder_0, &_call_ctor_QStringEncoder_0);
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QStringEncoder::QStringEncoder(QStringConverter::Encoding encoding, QFlags<QStringConverterBase::Flag> flags)\nThis method creates an object of class QStringEncoder.", &_init_ctor_QStringEncoder_6584, &_call_ctor_QStringEncoder_6584);
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QStringEncoder::QStringEncoder(const char *name, QFlags<QStringConverterBase::Flag> flags)\nThis method creates an object of class QStringEncoder.", &_init_ctor_QStringEncoder_5292, &_call_ctor_QStringEncoder_5292);
  methods += new qt_gsi::GenericMethod ("requiredSpace", "@brief Method qsizetype QStringEncoder::requiredSpace(qsizetype inputLength)\n", true, &_init_f_requiredSpace_c1442, &_call_f_requiredSpace_c1442);
  return methods;
}

gsi::Class<QStringConverter> &qtdecl_QStringConverter ();

gsi::Class<QStringEncoder> decl_QStringEncoder (qtdecl_QStringConverter (), "QtCore", "QStringEncoder",
  gsi::method_ext("encode", &encode, gsi::arg ("in"), "@brief Method QStringEncoder::DecodedData<QStringView> QStringEncoder::encode(QStringView in)\n") + 
  gsi::method_ext("()", &encode, gsi::arg ("in"), "@brief Method QStringEncoder::DecodedData<QStringView> QStringEncoder::operator()(QStringView in)\n")
+
  methods_QStringEncoder (),
  "@qt\n@brief Binding of QStringEncoder");


GSI_QTCORE_PUBLIC gsi::Class<QStringEncoder> &qtdecl_QStringEncoder () { return decl_QStringEncoder; }

}
