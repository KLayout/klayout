
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


#include "gsiQt.h"

#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QPoint>
#include <QtGui/QColor>
#include <QtNetwork/QHostAddress>

// ------------------------------------------------------------
//  Generic declarations for a pair

namespace qt_gsi
{

template <class A, class B>
struct pair_decl
{
  static typename qt_gsi::Converter<A>::target_type pair_first(const QPair<A, B> *pair)
  {
    return qt_gsi::Converter<A>::toc (pair->first);
  }

  static typename qt_gsi::Converter<B>::target_type pair_second(const QPair<A, B> *pair)
  {
    return qt_gsi::Converter<B>::toc (pair->second);
  }

  static void pair_set_first(QPair<A, B> *pair, const typename qt_gsi::Converter<A>::target_type &s)
  {
    pair->first = qt_gsi::Converter<A>::toq (s);
  }

  static void pair_set_second(QPair<A, B> *pair, const typename qt_gsi::Converter<B>::target_type &s)
  {
    pair->second = qt_gsi::Converter<B>::toq (s);
  }

  static bool pair_equal(const QPair<A, B> *pair, const QPair<A, B> &other)
  {
    return *pair == other;
  }

  /* Not available for all types:
  static bool pair_less(const QPair<A, B> *pair, const QPair<A, B> &other)
  {
    return *pair < other;
  }
  */

  static QPair<A, B> *pair_default_ctor()
  {
    return new QPair<A, B>();
  }

  static QPair<A, B> *pair_ctor(const typename qt_gsi::Converter<A>::target_type &first, const typename qt_gsi::Converter<B>::target_type &second)
  {
    return new QPair<A, B>(qt_gsi::Converter<A>::toq (first), qt_gsi::Converter<B>::toq (second));
  }

  static gsi::Methods methods ()
  {
    return
      gsi::constructor("new", &pair_default_ctor, "@brief Creates a new pair") +
      gsi::constructor("new", &pair_ctor, "@brief Creates a new pair from the given arguments\n@args first, second") +
      gsi::method_ext("first", &pair_first, "@brief Returns the first element of the pair\n") +
      gsi::method_ext("first=", &pair_set_first, "@brief Sets the first element of the pair\n@args first") +
      gsi::method_ext("second", &pair_second, "@brief Returns the second element of the pair\n") +
      gsi::method_ext("second=", &pair_set_second, "@brief Sets the second element of the pair\n@args second") +
      gsi::method_ext("==", &pair_equal, "@brief Returns true if self is equal to the other pair\n@args other") 
      // not available for all types:
      // gsi::method_ext("<", &pair_less, "@brief Returns true if self is less than the other pair\n@args other")
    ;
  }
};

}

// ------------------------------------------------------------
//  Declarations for QPair<QString, QString>

gsi::Class<QPair<QString, QString> > decl_QString_QPair ("QStringPair",
  qt_gsi::pair_decl<QString, QString>::methods (),
  "@qt\\n@brief Represents a QPair<QString, QString>"
);

// ------------------------------------------------------------
//  Declarations for QPair<QByteArray, QByteArray>

gsi::Class<QPair<QByteArray, QByteArray> > decl_QByteArray_QPair ("QByteArrayPair",
  qt_gsi::pair_decl<QByteArray, QByteArray>::methods (),
  "@qt\\n@brief Represents a QPair<QString, QString>"
);

// ------------------------------------------------------------
//  Declarations for QPair<double, double>

gsi::Class<QPair<double, double> > decl_double_QPair ("QDoublePair",
  qt_gsi::pair_decl<double, double>::methods (),
  "@qt\\n@brief Represents a QPair<double, double>"
);

// ------------------------------------------------------------
//  Declarations for QPair<double, QPointf>

gsi::Class<QPair<double, QPointF> > decl_double_QPointF_QPair ("QDoublePointFPair",
  qt_gsi::pair_decl<double, QPointF>::methods (),
  "@qt\\n@brief Represents a QPair<double, QPointF>"
);

// ------------------------------------------------------------
//  Declarations for QPair<double, QColor>

gsi::Class<QPair<double, QColor> > decl_double_QColor_QPair ("QDoubleColorPair",
  qt_gsi::pair_decl<double, QColor>::methods (),
  "@qt\\n@brief Represents a QPair<double, QColor>"
);

// ------------------------------------------------------------
//  Declarations for QPair<QHostAddress, int>

gsi::Class<QPair<QHostAddress, int> > decl_QHostAddress_int_QPair ("QHostAddressIntPair",
  qt_gsi::pair_decl<QHostAddress, int>::methods (),
  "@qt\\n@brief Represents a QPair<QHostAddress, int>"
);

// ------------------------------------------------------------
//  Some helper functions

namespace qt_gsi
{

std::vector<std::string> 
to_string_vector (const QStringList &sl)
{
  std::vector<std::string> sv;
  sv.reserve (sl.size ());
  for (QStringList::const_iterator s = sl.begin (); s != sl.end (); ++s) {
    sv.push_back (tl::to_string (*s));
  }
  return sv;
}

QStringList
to_string_list (const std::vector<std::string> &sv)
{
  QStringList sl;
  for (std::vector<std::string>::const_iterator s = sv.begin (); s != sv.end (); ++s) {
    sl.push_back (tl::to_qstring (*s));
  }
  return sl;
}

}

