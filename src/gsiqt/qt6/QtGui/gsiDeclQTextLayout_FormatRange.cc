
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
*  @file gsiDeclQTextLayout_FormatRange.cc 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

#include <QTextLayout>
#include "gsiQt.h"
#include "gsiQtGuiCommon.h"
#include <memory>

// -----------------------------------------------------------------------
// struct QTextLayout::FormatRange

//  Constructor QTextLayout::FormatRange::FormatRange()


static void _init_ctor_QTextLayout_FormatRange_0 (qt_gsi::GenericStaticMethod *decl)
{
  decl->set_return_new<QTextLayout::FormatRange> ();
}

static void _call_ctor_QTextLayout_FormatRange_0 (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) 
{
  __SUPPRESS_UNUSED_WARNING(args);
  ret.write<QTextLayout::FormatRange *> (new QTextLayout::FormatRange ());
}



namespace gsi
{

static gsi::Methods methods_QTextLayout_FormatRange () {
  gsi::Methods methods;
  methods += new qt_gsi::GenericStaticMethod ("new", "@brief Constructor QTextLayout::FormatRange::FormatRange()\nThis method creates an object of class QTextLayout::FormatRange.", &_init_ctor_QTextLayout_FormatRange_0, &_call_ctor_QTextLayout_FormatRange_0);
  return methods;
}

gsi::Class<QTextLayout::FormatRange> decl_QTextLayout_FormatRange ("QtGui", "QTextLayout_FormatRange",
  methods_QTextLayout_FormatRange (),
  "@qt\n@brief Binding of QTextLayout::FormatRange");

gsi::ClassExt<QTextLayout> decl_QTextLayout_FormatRange_as_child (decl_QTextLayout_FormatRange, "FormatRange");

GSI_QTGUI_PUBLIC gsi::Class<QTextLayout::FormatRange> &qtdecl_QTextLayout_FormatRange () { return decl_QTextLayout_FormatRange; }

}

