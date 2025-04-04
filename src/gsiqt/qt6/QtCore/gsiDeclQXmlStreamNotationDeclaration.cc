
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
*  @file gsiDeclQXmlStreamNotationDeclaration.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QXmlStreamNotationDeclaration>
#include "gsiQt.h"
#include "gsiQtCoreCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// class QXmlStreamNotationDeclaration
  static bool QXmlStreamNotationDeclaration_operator_eq(const QXmlStreamNotationDeclaration *a, const QXmlStreamNotationDeclaration &b) {
    return *a == b;
  }
  static bool QXmlStreamNotationDeclaration_operator_ne(const QXmlStreamNotationDeclaration *a, const QXmlStreamNotationDeclaration &b) {
    return !(*a == b);
  }

//  Constructor QXmlStreamNotationDeclaration::QXmlStreamNotationDeclaration()


static void _init_ctor_QXmlStreamNotationDeclaration_0 (qt_gsi::GenericStaticMethod *decl)
{
  decl->set_return_new<QXmlStreamNotationDeclaration> ();
}

static void _call_ctor_QXmlStreamNotationDeclaration_0 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QXmlStreamNotationDeclaration *> (new QXmlStreamNotationDeclaration ());
}



namespace gsi
{

static gsi::Methods methods_QXmlStreamNotationDeclaration () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QXmlStreamNotationDeclaration::QXmlStreamNotationDeclaration()\nThis method creates an object of class QXmlStreamNotationDeclaration.", &_init_ctor_QXmlStreamNotationDeclaration_0, &_call_ctor_QXmlStreamNotationDeclaration_0);
  return methods;
}

gsi::Class<QXmlStreamNotationDeclaration> decl_QXmlStreamNotationDeclaration ("QtCore", "QXmlStreamNotationDeclaration",
  gsi::method_ext("==", &QXmlStreamNotationDeclaration_operator_eq, gsi::arg ("other"), "@brief Method bool QXmlStreamNotationDeclaration::operator==(const QXmlStreamNotationDeclaration &) const") +
  gsi::method_ext("!=", &QXmlStreamNotationDeclaration_operator_ne, gsi::arg ("other"), "@brief Method bool QXmlStreamNotationDeclaration::operator!=(const QXmlStreamNotationDeclaration &) const") 
+
  methods_QXmlStreamNotationDeclaration (),
  "@qt\n@brief Binding of QXmlStreamNotationDeclaration");


GSI_QTCORE_PUBLIC gsi::Class<QXmlStreamNotationDeclaration> &qtdecl_QXmlStreamNotationDeclaration () { return decl_QXmlStreamNotationDeclaration; }

}

