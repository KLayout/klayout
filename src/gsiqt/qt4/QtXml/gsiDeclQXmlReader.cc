
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
*  @file gsiDeclQXmlReader.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QXmlReader>
#include <QXmlContentHandler>
#include <QXmlDTDHandler>
#include <QXmlDeclHandler>
#include <QXmlEntityResolver>
#include <QXmlErrorHandler>
#include <QXmlInputSource>
#include <QXmlLexicalHandler>
#include "gsiQt.h"
#include "gsiQtXmlCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QXmlReader

// QXmlDTDHandler *QXmlReader::DTDHandler()


static void _init_f_DTDHandler_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QXmlDTDHandler * > ();
}

static void _call_f_DTDHandler_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QXmlDTDHandler * > ((QXmlDTDHandler *)((QXmlReader *)cls)->DTDHandler ());
}


// QXmlContentHandler *QXmlReader::contentHandler()


static void _init_f_contentHandler_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QXmlContentHandler * > ();
}

static void _call_f_contentHandler_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QXmlContentHandler * > ((QXmlContentHandler *)((QXmlReader *)cls)->contentHandler ());
}


// QXmlDeclHandler *QXmlReader::declHandler()


static void _init_f_declHandler_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QXmlDeclHandler * > ();
}

static void _call_f_declHandler_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QXmlDeclHandler * > ((QXmlDeclHandler *)((QXmlReader *)cls)->declHandler ());
}


// QXmlEntityResolver *QXmlReader::entityResolver()


static void _init_f_entityResolver_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QXmlEntityResolver * > ();
}

static void _call_f_entityResolver_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QXmlEntityResolver * > ((QXmlEntityResolver *)((QXmlReader *)cls)->entityResolver ());
}


// QXmlErrorHandler *QXmlReader::errorHandler()


static void _init_f_errorHandler_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QXmlErrorHandler * > ();
}

static void _call_f_errorHandler_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QXmlErrorHandler * > ((QXmlErrorHandler *)((QXmlReader *)cls)->errorHandler ());
}


// bool QXmlReader::feature(const QString &name, bool *ok)


static void _init_f_feature_c2967 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("name");
  decl->add_arg<const QString & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("ok", true, "0");
  decl->add_arg<bool * > (argspec_1);
  decl->set_return<bool > ();
}

static void _call_f_feature_c2967 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  bool *arg2 = args ? gsi::arg_reader<bool * >() (args, heap) : gsi::arg_maker<bool * >() (0, heap);
  ret.write<bool > ((bool)((QXmlReader *)cls)->feature (arg1, arg2));
}


// bool QXmlReader::hasFeature(const QString &name)


static void _init_f_hasFeature_c2025 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("name");
  decl->add_arg<const QString & > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_hasFeature_c2025 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  ret.write<bool > ((bool)((QXmlReader *)cls)->hasFeature (arg1));
}


// bool QXmlReader::hasProperty(const QString &name)


static void _init_f_hasProperty_c2025 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("name");
  decl->add_arg<const QString & > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_hasProperty_c2025 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  ret.write<bool > ((bool)((QXmlReader *)cls)->hasProperty (arg1));
}


// QXmlLexicalHandler *QXmlReader::lexicalHandler()


static void _init_f_lexicalHandler_c0 (qt_gsi::GenericMethod *decl)
{
  decl->set_return<QXmlLexicalHandler * > ();
}

static void _call_f_lexicalHandler_c0 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QXmlLexicalHandler * > ((QXmlLexicalHandler *)((QXmlReader *)cls)->lexicalHandler ());
}


// bool QXmlReader::parse(const QXmlInputSource *input)


static void _init_f_parse_2856 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("input");
  decl->add_arg<const QXmlInputSource * > (argspec_0);
  decl->set_return<bool > ();
}

static void _call_f_parse_2856 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QXmlInputSource *arg1 = gsi::arg_reader<const QXmlInputSource * >() (args, heap);
  ret.write<bool > ((bool)((QXmlReader *)cls)->parse (arg1));
}


// void *QXmlReader::property(const QString &name, bool *ok)


static void _init_f_property_c2967 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("name");
  decl->add_arg<const QString & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("ok", true, "0");
  decl->add_arg<bool * > (argspec_1);
  decl->set_return<void * > ();
}

static void _call_f_property_c2967 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  bool *arg2 = args ? gsi::arg_reader<bool * >() (args, heap) : gsi::arg_maker<bool * >() (0, heap);
  ret.write<void * > ((void *)((QXmlReader *)cls)->property (arg1, arg2));
}


// void QXmlReader::setContentHandler(QXmlContentHandler *handler)


static void _init_f_setContentHandler_2441 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("handler");
  decl->add_arg<QXmlContentHandler * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setContentHandler_2441 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QXmlContentHandler *arg1 = gsi::arg_reader<QXmlContentHandler * >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QXmlReader *)cls)->setContentHandler (arg1);
}


// void QXmlReader::setDTDHandler(QXmlDTDHandler *handler)


static void _init_f_setDTDHandler_1930 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("handler");
  decl->add_arg<QXmlDTDHandler * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setDTDHandler_1930 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QXmlDTDHandler *arg1 = gsi::arg_reader<QXmlDTDHandler * >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QXmlReader *)cls)->setDTDHandler (arg1);
}


// void QXmlReader::setDeclHandler(QXmlDeclHandler *handler)


static void _init_f_setDeclHandler_2086 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("handler");
  decl->add_arg<QXmlDeclHandler * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setDeclHandler_2086 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QXmlDeclHandler *arg1 = gsi::arg_reader<QXmlDeclHandler * >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QXmlReader *)cls)->setDeclHandler (arg1);
}


// void QXmlReader::setEntityResolver(QXmlEntityResolver *handler)


static void _init_f_setEntityResolver_2495 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("handler");
  decl->add_arg<QXmlEntityResolver * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setEntityResolver_2495 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QXmlEntityResolver *arg1 = gsi::arg_reader<QXmlEntityResolver * >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QXmlReader *)cls)->setEntityResolver (arg1);
}


// void QXmlReader::setErrorHandler(QXmlErrorHandler *handler)


static void _init_f_setErrorHandler_2232 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("handler");
  decl->add_arg<QXmlErrorHandler * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setErrorHandler_2232 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QXmlErrorHandler *arg1 = gsi::arg_reader<QXmlErrorHandler * >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QXmlReader *)cls)->setErrorHandler (arg1);
}


// void QXmlReader::setFeature(const QString &name, bool value)


static void _init_f_setFeature_2781 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("name");
  decl->add_arg<const QString & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("value");
  decl->add_arg<bool > (argspec_1);
  decl->set_return<void > ();
}

static void _call_f_setFeature_2781 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  bool arg2 = gsi::arg_reader<bool >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QXmlReader *)cls)->setFeature (arg1, arg2);
}


// void QXmlReader::setLexicalHandler(QXmlLexicalHandler *handler)


static void _init_f_setLexicalHandler_2416 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("handler");
  decl->add_arg<QXmlLexicalHandler * > (argspec_0);
  decl->set_return<void > ();
}

static void _call_f_setLexicalHandler_2416 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  QXmlLexicalHandler *arg1 = gsi::arg_reader<QXmlLexicalHandler * >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QXmlReader *)cls)->setLexicalHandler (arg1);
}


// void QXmlReader::setProperty(const QString &name, void *value)


static void _init_f_setProperty_2973 (qt_gsi::GenericMethod *decl)
{
  static gsi::ArgSpecBase argspec_0 ("name");
  decl->add_arg<const QString & > (argspec_0);
  static gsi::ArgSpecBase argspec_1 ("value");
  decl->add_arg<void * > (argspec_1);
  decl->set_return<void > ();
}

static void _call_f_setProperty_2973 (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  tl::Heap heap;
  const QString &arg1 = gsi::arg_reader<const QString & >() (args, heap);
  void *arg2 = gsi::arg_reader<void * >() (args, heap);
  __SUPPRESS_UNUSED_WARNING(ret);
  ((QXmlReader *)cls)->setProperty (arg1, arg2);
}



namespace gsi
{

static gsi::Methods methods_QXmlReader () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericMethod ("DTDHandler", "@brief Method QXmlDTDHandler *QXmlReader::DTDHandler()\n", true, &_init_f_DTDHandler_c0, &_call_f_DTDHandler_c0);
  methods += new qt_gsi::GenericMethod (":contentHandler", "@brief Method QXmlContentHandler *QXmlReader::contentHandler()\n", true, &_init_f_contentHandler_c0, &_call_f_contentHandler_c0);
  methods += new qt_gsi::GenericMethod (":declHandler", "@brief Method QXmlDeclHandler *QXmlReader::declHandler()\n", true, &_init_f_declHandler_c0, &_call_f_declHandler_c0);
  methods += new qt_gsi::GenericMethod (":entityResolver", "@brief Method QXmlEntityResolver *QXmlReader::entityResolver()\n", true, &_init_f_entityResolver_c0, &_call_f_entityResolver_c0);
  methods += new qt_gsi::GenericMethod (":errorHandler", "@brief Method QXmlErrorHandler *QXmlReader::errorHandler()\n", true, &_init_f_errorHandler_c0, &_call_f_errorHandler_c0);
  methods += new qt_gsi::GenericMethod ("feature", "@brief Method bool QXmlReader::feature(const QString &name, bool *ok)\n", true, &_init_f_feature_c2967, &_call_f_feature_c2967);
  methods += new qt_gsi::GenericMethod ("hasFeature", "@brief Method bool QXmlReader::hasFeature(const QString &name)\n", true, &_init_f_hasFeature_c2025, &_call_f_hasFeature_c2025);
  methods += new qt_gsi::GenericMethod ("hasProperty", "@brief Method bool QXmlReader::hasProperty(const QString &name)\n", true, &_init_f_hasProperty_c2025, &_call_f_hasProperty_c2025);
  methods += new qt_gsi::GenericMethod (":lexicalHandler", "@brief Method QXmlLexicalHandler *QXmlReader::lexicalHandler()\n", true, &_init_f_lexicalHandler_c0, &_call_f_lexicalHandler_c0);
  methods += new qt_gsi::GenericMethod ("parse", "@brief Method bool QXmlReader::parse(const QXmlInputSource *input)\n", false, &_init_f_parse_2856, &_call_f_parse_2856);
  methods += new qt_gsi::GenericMethod ("property", "@brief Method void *QXmlReader::property(const QString &name, bool *ok)\n", true, &_init_f_property_c2967, &_call_f_property_c2967);
  methods += new qt_gsi::GenericMethod ("setContentHandler|contentHandler=", "@brief Method void QXmlReader::setContentHandler(QXmlContentHandler *handler)\n", false, &_init_f_setContentHandler_2441, &_call_f_setContentHandler_2441);
  methods += new qt_gsi::GenericMethod ("setDTDHandler", "@brief Method void QXmlReader::setDTDHandler(QXmlDTDHandler *handler)\n", false, &_init_f_setDTDHandler_1930, &_call_f_setDTDHandler_1930);
  methods += new qt_gsi::GenericMethod ("setDeclHandler|declHandler=", "@brief Method void QXmlReader::setDeclHandler(QXmlDeclHandler *handler)\n", false, &_init_f_setDeclHandler_2086, &_call_f_setDeclHandler_2086);
  methods += new qt_gsi::GenericMethod ("setEntityResolver|entityResolver=", "@brief Method void QXmlReader::setEntityResolver(QXmlEntityResolver *handler)\n", false, &_init_f_setEntityResolver_2495, &_call_f_setEntityResolver_2495);
  methods += new qt_gsi::GenericMethod ("setErrorHandler|errorHandler=", "@brief Method void QXmlReader::setErrorHandler(QXmlErrorHandler *handler)\n", false, &_init_f_setErrorHandler_2232, &_call_f_setErrorHandler_2232);
  methods += new qt_gsi::GenericMethod ("setFeature", "@brief Method void QXmlReader::setFeature(const QString &name, bool value)\n", false, &_init_f_setFeature_2781, &_call_f_setFeature_2781);
  methods += new qt_gsi::GenericMethod ("setLexicalHandler|lexicalHandler=", "@brief Method void QXmlReader::setLexicalHandler(QXmlLexicalHandler *handler)\n", false, &_init_f_setLexicalHandler_2416, &_call_f_setLexicalHandler_2416);
  methods += new qt_gsi::GenericMethod ("setProperty", "@brief Method void QXmlReader::setProperty(const QString &name, void *value)\n", false, &_init_f_setProperty_2973, &_call_f_setProperty_2973);
  return methods;
}

gsi::Class<QXmlReader> decl_QXmlReader ("QtXml", "QXmlReader",
  methods_QXmlReader (),
  "@qt\n@brief Binding of QXmlReader");


GSI_QTXML_PUBLIC gsi::Class<QXmlReader> &qtdecl_QXmlReader () { return decl_QXmlReader; }

}

