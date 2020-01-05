
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#include <QPair>
#include <QString>
#include <QByteArray>
#include <QPoint>
#include <QColor>
#include <QSize>
#include <QAbstractMessageHandler>

//  NOTE: this is required because HitTestAccuracy is defined here, but goes into Qt
//  namespace which is in QtCore ... this introduces a dependency of QtCore GSI lib on QtGui.
#include <QTextDocument>

class Qt_Namespace { };
class QVariant_Namespace { };

namespace gsi_qt
{

// ---------------------------------------------------------------------------
//  QVariant::Type implementation
//  (this type is not created automatically since QVariant is implemented implicitly)

//  A dummy namespace "QVariant"
gsi::Class<QVariant_Namespace> decl_QVariant_Namespace ("QtCore", "QVariant",
  gsi::Methods(),
  "@qt\n@brief This class represents the QVariant namespace");

static gsi::Enum<QVariant::Type> decl_QVariant_Type_Enum ("QtCore", "QVariant_Type",
    gsi::enum_const ("Invalid", QVariant::Invalid, "@brief Enum constant QVariant::Invalid") +
    gsi::enum_const ("Bool", QVariant::Bool, "@brief Enum constant QVariant::Bool") +
    gsi::enum_const ("Int", QVariant::Int, "@brief Enum constant QVariant::Int") +
    gsi::enum_const ("UInt", QVariant::UInt, "@brief Enum constant QVariant::UInt") +
    gsi::enum_const ("LongLong", QVariant::LongLong, "@brief Enum constant QVariant::LongLong") +
    gsi::enum_const ("ULongLong", QVariant::ULongLong, "@brief Enum constant QVariant::ULongLong") +
    gsi::enum_const ("Double", QVariant::Double, "@brief Enum constant QVariant::Double") +
    gsi::enum_const ("Char", QVariant::Char, "@brief Enum constant QVariant::Char") +
    gsi::enum_const ("Map", QVariant::Map, "@brief Enum constant QVariant::Map") +
    gsi::enum_const ("List", QVariant::List, "@brief Enum constant QVariant::List") +
    gsi::enum_const ("String", QVariant::String, "@brief Enum constant QVariant::String") +
    gsi::enum_const ("StringList", QVariant::StringList, "@brief Enum constant QVariant::StringList") +
    gsi::enum_const ("ByteArray", QVariant::ByteArray, "@brief Enum constant QVariant::ByteArray") +
    gsi::enum_const ("BitArray", QVariant::BitArray, "@brief Enum constant QVariant::BitArray") +
    gsi::enum_const ("Date", QVariant::Date, "@brief Enum constant QVariant::Date") +
    gsi::enum_const ("Time", QVariant::Time, "@brief Enum constant QVariant::Time") +
    gsi::enum_const ("DateTime", QVariant::DateTime, "@brief Enum constant QVariant::DateTime") +
    gsi::enum_const ("Url", QVariant::Url, "@brief Enum constant QVariant::Url") +
    gsi::enum_const ("Locale", QVariant::Locale, "@brief Enum constant QVariant::Locale") +
    gsi::enum_const ("Rect", QVariant::Rect, "@brief Enum constant QVariant::Rect") +
    gsi::enum_const ("RectF", QVariant::RectF, "@brief Enum constant QVariant::RectF") +
    gsi::enum_const ("Size", QVariant::Size, "@brief Enum constant QVariant::Size") +
    gsi::enum_const ("SizeF", QVariant::SizeF, "@brief Enum constant QVariant::SizeF") +
    gsi::enum_const ("Line", QVariant::Line, "@brief Enum constant QVariant::Line") +
    gsi::enum_const ("LineF", QVariant::LineF, "@brief Enum constant QVariant::LineF") +
    gsi::enum_const ("Point", QVariant::Point, "@brief Enum constant QVariant::Point") +
    gsi::enum_const ("PointF", QVariant::PointF, "@brief Enum constant QVariant::PointF") +
    gsi::enum_const ("RegExp", QVariant::RegExp, "@brief Enum constant QVariant::RegExp") +
    gsi::enum_const ("Hash", QVariant::Hash, "@brief Enum constant QVariant::Hash") +
    gsi::enum_const ("LastCoreType", QVariant::LastCoreType, "@brief Enum constant QVariant::LastCoreType") +
    gsi::enum_const ("Font", QVariant::Font, "@brief Enum constant QVariant::Font") +
    gsi::enum_const ("Pixmap", QVariant::Pixmap, "@brief Enum constant QVariant::Pixmap") +
    gsi::enum_const ("Brush", QVariant::Brush, "@brief Enum constant QVariant::Brush") +
    gsi::enum_const ("Color", QVariant::Color, "@brief Enum constant QVariant::Color") +
    gsi::enum_const ("Palette", QVariant::Palette, "@brief Enum constant QVariant::Palette") +
    gsi::enum_const ("Icon", QVariant::Icon, "@brief Enum constant QVariant::Icon") +
    gsi::enum_const ("Image", QVariant::Image, "@brief Enum constant QVariant::Image") +
    gsi::enum_const ("Polygon", QVariant::Polygon, "@brief Enum constant QVariant::Polygon") +
    gsi::enum_const ("Region", QVariant::Region, "@brief Enum constant QVariant::Region") +
    gsi::enum_const ("Bitmap", QVariant::Bitmap, "@brief Enum constant QVariant::Bitmap") +
    gsi::enum_const ("Cursor", QVariant::Cursor, "@brief Enum constant QVariant::Cursor") +
    gsi::enum_const ("SizePolicy", QVariant::SizePolicy, "@brief Enum constant QVariant::SizePolicy") +
    gsi::enum_const ("KeySequence", QVariant::KeySequence, "@brief Enum constant QVariant::KeySequence") +
    gsi::enum_const ("Pen", QVariant::Pen, "@brief Enum constant QVariant::Pen") +
    gsi::enum_const ("TextLength", QVariant::TextLength, "@brief Enum constant QVariant::TextLength") +
    gsi::enum_const ("TextFormat", QVariant::TextFormat, "@brief Enum constant QVariant::TextFormat") +
    gsi::enum_const ("Matrix", QVariant::Matrix, "@brief Enum constant QVariant::Matrix") +
    gsi::enum_const ("Transform", QVariant::Transform, "@brief Enum constant QVariant::Transform") +
    gsi::enum_const ("Matrix4x4", QVariant::Matrix4x4, "@brief Enum constant QVariant::Matrix4x4") +
    gsi::enum_const ("Vector2D", QVariant::Vector2D, "@brief Enum constant QVariant::Vector2D") +
    gsi::enum_const ("Vector3D", QVariant::Vector3D, "@brief Enum constant QVariant::Vector3D") +
    gsi::enum_const ("Vector4D", QVariant::Vector4D, "@brief Enum constant QVariant::Vector4D") +
    gsi::enum_const ("Quaternion", QVariant::Quaternion, "@brief Enum constant QVariant::Quaternion") +
    gsi::enum_const ("LastGuiType", QVariant::LastGuiType, "@brief Enum constant QVariant::LastGuiType") +
    gsi::enum_const ("UserType", QVariant::UserType, "@brief Enum constant QVariant::UserType") +
    gsi::enum_const ("LastType", QVariant::LastType, "@brief Enum constant QVariant::LastType"),
  "@qt\n@brief This class represents the QVariant::Type enum");

static gsi::QFlagsClass<QVariant::Type> decl_QVariant_Type_Enums ("QtCore", "QVariant_QFlags_Type",
  "@qt\n@brief This class represents the QFlags<QVariant::Type> flag set");

//  Inject the declarations into the parent
static gsi::ClassExt<QVariant_Namespace> inject_QVariant_Type_Enum_in_parent (decl_QVariant_Type_Enum.defs ());
static gsi::ClassExt<QVariant_Namespace> decl_QVariant_Type_Enum_as_child (decl_QVariant_Type_Enum, "Type");
static gsi::ClassExt<QVariant_Namespace> decl_QVariant_Type_Enums_as_child (decl_QVariant_Type_Enums, "QFlags_Type");

// ------------------------------------------------------------
//  Declarations for QPair<QString, QString>

gsi::Class<QPair<QString, QString> > decl_QString_QPair ("QtCore", "QPair_QString_QString",
  qt_gsi::pair_decl<QString, QString>::methods (),
  "@qt\\n@brief Represents a QPair<QString, QString>"
);

// ------------------------------------------------------------
//  Declarations for QPair<QByteArray, QByteArray>

gsi::Class<QPair<QByteArray, QByteArray> > decl_QByteArray_QPair ("QtCore", "QPair_QByteArray_QByteArray",
  qt_gsi::pair_decl<QByteArray, QByteArray>::methods (),
  "@qt\\n@brief Represents a QPair<QString, QString>"
);

// ------------------------------------------------------------
//  Declarations for QPair<double, double>

gsi::Class<QPair<double, double> > decl_double_QPair ("QtCore", "QPair_double_double",
  qt_gsi::pair_decl<double, double>::methods (),
  "@qt\\n@brief Represents a QPair<double, double>"
);

// ------------------------------------------------------------
//  Declarations for QPair<double, QPointf>

gsi::Class<QPair<double, QPointF> > decl_double_QPointF_QPair ("QtCore", "QPair_double_QPointF",
  qt_gsi::pair_decl<double, QPointF>::methods (),
  "@qt\\n@brief Represents a QPair<double, QPointF>"
);

// ------------------------------------------------------------
//  Declarations for QPair<int, int>

gsi::Class<QPair<int, int> > decl_int_int_QPair ("QtCore", "QPair_int_int",
  qt_gsi::pair_decl<int, int>::methods (),
  "@qt\\n@brief Represents a QPair<int, int>"
);

// ------------------------------------------------------------
//  Declarations for QPair<QString, QSizeF>

gsi::Class<QPair<QString, QSizeF> > decl_QString_QSizeF_QPair ("QtCore", "QPair_QString_QSizeF",
  qt_gsi::pair_decl<QString, QSizeF>::methods (),
  "@qt\\n@brief Represents a QPair<QString, QSizeF>"
);

// ------------------------------------------------------------
//  Declarations for QPair<double, QVariant>

gsi::Class<QPair<double, QVariant> > decl_double_QVariant_QPair ("QtCore", "QPair_double_QVariant",
  qt_gsi::pair_decl<double, QVariant>::methods (),
  "@qt\\n@brief Represents a QPair<double, QVariant>"
);

// ---------------------------------------------------------------------------
//  QtMsgType implementation
//  (this type is not created automatically since QtMsgType is not within a namespace)
//  TODO: automate this within mkqtdecl5.sh

static gsi::Enum<QtMsgType> decl_QtMsgType ("QtCore", "QtMsgType",
  gsi::enum_const ("QtDebugMsg", QtDebugMsg, "@brief Enum constant QtDebugMsg of QtMsgType") +
  gsi::enum_const ("QtWarningMsg", QtWarningMsg, "@brief Enum constant QtWarningMsg of QtMsgType") +
  gsi::enum_const ("QtCriticalMsg", QtCriticalMsg, "@brief Enum constant QtCriticalMsg of QtMsgType") +
  gsi::enum_const ("QtFatalMsg", QtFatalMsg, "@brief Enum constant QtFatalMsg of QtMsgType") +
  gsi::enum_const ("QtInfoMsg", QtInfoMsg, "@brief Enum constant QtInfoMsg of QtMsgType") +
  gsi::enum_const ("QtSystemMsg", QtSystemMsg, "@brief Enum constant QtSystemMsg of QtMsgType"),
  "@qt\n@brief Binding of QtMsgType"
);

// ---------------------------------------------------------------------------
//  QUrlTwoFlags<QUrl::UrlFormattingOption, QUrl::ComponentFormattingOption> bindings

template <class A, class B>
class QUrlTwoFlagsClass;

/**
 *  @brief An adaptor class for the QUrlTwoFlags<A, B> template derived from QFlagsAdaptor<E> template
 */
template <class A, class B>
class QUrlTwoFlagsAdaptor
{
public:
  QUrlTwoFlagsAdaptor () : m_qf () { }
  QUrlTwoFlagsAdaptor (A e) : m_qf (e) { }
  QUrlTwoFlagsAdaptor (B e) : m_qf (e) { }
  QUrlTwoFlagsAdaptor (QFlags<A> e) : m_qf (e) { }
  QUrlTwoFlagsAdaptor (QFlags<B> e) : m_qf (e) { }
  QUrlTwoFlagsAdaptor (const QUrlTwoFlags<A, B> &qf) : m_qf (qf) { }
  QUrlTwoFlagsAdaptor (int i) : m_qf (i) { }

  QUrlTwoFlagsAdaptor (const std::string &s)
  {
    const QUrlTwoFlagsClass<A, B> *ecls = dynamic_cast<const QUrlTwoFlagsClass<A, B> *> (gsi::cls_decl<QUrlTwoFlags<A, B> > ());
    tl_assert (ecls != 0);
    m_qf = ecls->qflags_from_string (s);
  }

  QUrlTwoFlags<A, B> &value () { return m_qf; }
  const QUrlTwoFlags<A, B> &value () const { return m_qf; }

private:
  QUrlTwoFlags<A, B> m_qf;
};

/**
 *  @brief A binding for QUrlTwoFlags<A, B> derived from QFlags binding
 */
template <class A, class B>
class QUrlTwoFlagsClass
  : public gsi::Class<QUrlTwoFlagsAdaptor<A, B>, QUrlTwoFlags<A, B> >
{
public:
  typedef typename QFlags<A>::Int int_repr;

  QUrlTwoFlagsClass (const std::string &module, const std::string &name, const std::string &doc = std::string ())
    : gsi::Class<QUrlTwoFlagsAdaptor<A, B>, QUrlTwoFlags<A, B> > (module, name, methods (), doc)
  {
  }

  static QUrlTwoFlags<A, B> *new_from_i (int_repr i)
  {
    return new QUrlTwoFlags<A, B> (i);
  }

  static QUrlTwoFlags<A, B> *new_from_e1 (A e)
  {
    return new QUrlTwoFlags<A, B> (e);
  }

  static QUrlTwoFlags<A, B> *new_from_e1f (QFlags<A> e)
  {
    return new QUrlTwoFlags<A, B> (e);
  }

  static QUrlTwoFlags<A, B> *new_from_e2 (B e)
  {
    return new QUrlTwoFlags<A, B> (e);
  }

  static QUrlTwoFlags<A, B> *new_from_e2f (QFlags<B> e)
  {
    return new QUrlTwoFlags<A, B> (e);
  }

  static QUrlTwoFlags<A, B> *new_from_s (const std::string &s)
  {
    QUrlTwoFlags<A, B> flags;

    const gsi::Enum<A> *acls = dynamic_cast<const gsi::Enum<A> *> (gsi::cls_decl<A> ());
    const gsi::Enum<B> *bcls = dynamic_cast<const gsi::Enum<B> *> (gsi::cls_decl<B> ());
    tl_assert (acls != 0 && bcls != 0);

    tl::Extractor ex (s.c_str ());
    while (! ex.at_end ()) {

      bool any = false;

      for (typename gsi::EnumSpecs<A>::iterator s = acls->specs ().begin (); s != acls->specs ().end () && !any; ++s) {
        if (ex.test (s->str.c_str ())) {
          flags |= A (s->evalue);
          any = true;
        }
      }

      for (typename gsi::EnumSpecs<B>::iterator s = bcls->specs ().begin (); s != bcls->specs ().end () && !any; ++s) {
        if (ex.test (s->str.c_str ())) {
          flags |= B (s->evalue);
          any = true;
        }
      }

      if (any) {
        ex.test ("|");
        ex.test (",");
      } else {
        break;
      }

    }

    return new QUrlTwoFlags<A, B> (flags);
  }

  static std::string to_s (const QUrlTwoFlags<A, B> *self)
  {
    std::string res;

    const gsi::Enum<A> *acls = dynamic_cast<const gsi::Enum<A> *> (gsi::cls_decl<A> ());
    const gsi::Enum<B> *bcls = dynamic_cast<const gsi::Enum<B> *> (gsi::cls_decl<B> ());
    tl_assert (acls != 0 && bcls != 0);

    for (typename gsi::EnumSpecs<A>::iterator s = acls->specs ().begin (); s != acls->specs ().end (); ++s) {
      if (self->testFlag (s->evalue)) {
        if (! res.empty ()) {
          res += "|";
        }
        res += s->str;
      }
    }

    for (typename gsi::EnumSpecs<B>::iterator s = bcls->specs ().begin (); s != bcls->specs ().end (); ++s) {
      if (self->testFlag (s->evalue)) {
        if (! res.empty ()) {
          res += "|";
        }
        res += s->str;
      }
    }

    return res;
  }

  static int_repr to_i (const QUrlTwoFlags<A, B> *self)
  {
    return int_repr (*self);
  }

  static bool test_flag1 (const QUrlTwoFlags<A, B> *self, A e)
  {
    return self->testFlag (e);
  }

  static bool test_flag2 (const QUrlTwoFlags<A, B> *self, B e)
  {
    return self->testFlag (e);
  }

  static std::string inspect (const QUrlTwoFlags<A, B> *self)
  {
    return to_s (self) + tl::sprintf(" (%u)", (unsigned int) (int_repr (*self)));
  }

  static QUrlTwoFlags<A, B> invert (const QUrlTwoFlags<A, B> *self)
  {
    return ~*self;
  }

  static QUrlTwoFlags<A, B> or_op (const QUrlTwoFlags<A, B> *self, const QUrlTwoFlags<A, B> &other)
  {
    return *self | other;
  }

  static QUrlTwoFlags<A, B> or_op_with_e1 (const QUrlTwoFlags<A, B> *self, A e)
  {
    return *self | e;
  }

  static QUrlTwoFlags<A, B> or_op_with_e2 (const QUrlTwoFlags<A, B> *self, B e)
  {
    return *self | e;
  }

  static QUrlTwoFlags<A, B> and_op (const QUrlTwoFlags<A, B> *self, const QUrlTwoFlags<A, B> &other)
  {
    return *self & other;
  }

  static QUrlTwoFlags<A, B> and_op_with_e1 (const QUrlTwoFlags<A, B> *self, A e)
  {
    return *self & e;
  }

  static QUrlTwoFlags<A, B> and_op_with_e2 (const QUrlTwoFlags<A, B> *self, B e)
  {
    return *self & e;
  }

  static QUrlTwoFlags<A, B> xor_op (const QUrlTwoFlags<A, B> *self, const QUrlTwoFlags<A, B> &other)
  {
    return *self ^ other;
  }

  static QUrlTwoFlags<A, B> xor_op_with_e1 (const QUrlTwoFlags<A, B> *self, A e)
  {
    return *self ^ e;
  }

  static QUrlTwoFlags<A, B> xor_op_with_e2 (const QUrlTwoFlags<A, B> *self, B e)
  {
    return *self ^ e;
  }

  static bool not_equal_with_i (const QUrlTwoFlags<A, B> *self, int_repr i)
  {
    return int_repr (*self) != i;
  }

  static bool not_equal (const QUrlTwoFlags<A, B> *self, const QUrlTwoFlags<A, B> &other)
  {
    //  See equal() for an explanation why we compare int's.
    return int_repr (*self) != int_repr (other);
  }

  static bool equal_with_i (const QUrlTwoFlags<A, B> *self, int_repr i)
  {
    return int_repr (*self) == i;
  }

  static bool equal (const QUrlTwoFlags<A, B> *self, const QUrlTwoFlags<A, B> &other)
  {
    //  NOTE: in order to avoid ambiguities with non-explicit constructors of objects taking a QFlag as an argument,
    //  we compare int's explicitly. An example for such an ambiguity is QSurfaceFormat in Qt 5.5.1 which takes a QFlags<FormatOption>
    //  object in a non-explicit constructor.
    return int_repr (*self) == int_repr (other);
  }

  static gsi::Methods methods ()
  {
    return
      gsi::constructor ("new", &new_from_i, gsi::arg ("i"), "@brief Creates a flag set from an integer value") +
      gsi::constructor ("new", &new_from_s, gsi::arg ("s"), "@brief Creates a flag set from a string") +
      gsi::constructor ("new", &new_from_e1, gsi::arg ("e"), "@brief Creates a flag set from an enum") +
      gsi::constructor ("new", &new_from_e1f, gsi::arg ("e"), "@brief Creates a flag set from a QFlags set") +
      gsi::constructor ("new", &new_from_e2, gsi::arg ("e"), "@brief Creates a flag set from an enum") +
      gsi::constructor ("new", &new_from_e2f, gsi::arg ("e"), "@brief Creates a flag set from a QFlags set") +
      gsi::method_ext ("to_s", &to_s, "@brief Converts the flag set to a string") +
      gsi::method_ext ("to_i", &to_i, "@brief Converts the flag set to an integer") +
      gsi::method_ext ("testFlag1", &test_flag1, gsi::arg ("flag"), "@brief Tests whether the flag set contains the given flag") +
      gsi::method_ext ("testFlag2", &test_flag2, gsi::arg ("flag"), "@brief Tests whether the flag set contains the given flag") +
      gsi::method_ext ("inspect", &inspect, "@brief Converts the flag set to a visual string") +
      gsi::method_ext ("|", &or_op, gsi::arg ("other"), "@brief Computes the union of two flag sets") +
      gsi::method_ext ("|", &or_op_with_e1, gsi::arg ("flag"), "@brief Adds the given flag to the flag set and returns the new flag set") +
      gsi::method_ext ("|", &or_op_with_e2, gsi::arg ("flag"), "@brief Adds the given flag to the flag set and returns the new flag set") +
      gsi::method_ext ("&", &and_op, gsi::arg ("other"), "@brief Computes the intersection between the two flag sets") +
      gsi::method_ext ("&", &and_op_with_e1, gsi::arg ("flag"), "@brief Tests whether the given flag is contained in the flag set and returns a null flag set if not") +
      gsi::method_ext ("&", &and_op_with_e2, gsi::arg ("flag"), "@brief Tests whether the given flag is contained in the flag set and returns a null flag set if not") +
      gsi::method_ext ("^", &xor_op, gsi::arg ("other"), "@brief Computes the exclusive-or between the flag set and the other flag set") +
      gsi::method_ext ("^", &xor_op_with_e1, gsi::arg ("flag"), "@brief Inverts the given flag in the flag set and returns the new flag set") +
      gsi::method_ext ("^", &xor_op_with_e2, gsi::arg ("flag"), "@brief Inverts the given flag in the flag set and returns the new flag set") +
      gsi::method_ext ("==", &equal_with_i, gsi::arg ("other"), "@brief Returns true if the flag set equals the given integer value") +
      gsi::method_ext ("==", &equal, gsi::arg ("i"), "@brief Returns true if the flag set equals the given other flag set") +
      gsi::method_ext ("!=", &not_equal_with_i, gsi::arg ("other"), "@brief Returns true if the flag set is not equal to the given integer value") +
      gsi::method_ext ("!=", &not_equal, gsi::arg ("i"), "@brief Returns true if the flag set is not equal to the given other flag set") +
      gsi::method_ext ("~", &invert, "@brief Returns the inverted flag set");
  }
};

static QUrlTwoFlagsClass<QUrl::UrlFormattingOption, QUrl::ComponentFormattingOption> decl_QUrlTwoFlags ("QtCore", "QUrl_FormattingOptions", "@qt\n@brief Binding of QUrl::FormattingOptions");

//  inject as QUrl::FormattingOptions
static gsi::ClassExt<QUrl> decl_QUrlTwoFlags_as_child (decl_QUrlTwoFlags, "FormattingOptions", "@qt\n@brief Binding of QUrl::FormattingOptions");

// ---------------------------------------------------------------------------
//  Add declarations for Qt constants and propagate into QtCore space

static int _qt_version () { return QT_VERSION; }
static std::string _qt_version_str () { return QT_VERSION_STR; }

static gsi::ClassExt<Qt_Namespace> decl_QtCore_constants (
  gsi::constant ("QT_VERSION", _qt_version,
    "@brief QT_VERSION constant\n"
    "This is the Qt version used at build time. "
    "Note that the script binding may be derived from an older version so effectively the API may be older."
  ) +
  gsi::constant ("QT_VERSION_STR", _qt_version_str,
    "@brief QT_VERSION_STR constant\n"
    "This is the Qt version used at build time. "
    "Note that the script binding may be derived from an older version so effectively the API may be older."
  )
);

}
