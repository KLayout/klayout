
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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
#include <QHostAddress>
#include <QUrl>

namespace gsi_qt
{

// ------------------------------------------------------------
//  Declarations for QPair<QHostAddress, int>

gsi::Class<QPair<QHostAddress, int> > decl_QHostAddress_int_QPair ("QtNetwork", "QPair_QHostAddress_int",
  qt_gsi::pair_decl<QHostAddress, int>::methods (),
  "@qt\\n@brief Represents a QPair<QHostAddress, int>"
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

static QUrlTwoFlagsClass<QUrl::UrlFormattingOption, QUrl::ComponentFormattingOption> decl_QUrlTwoFlags ("QtNetwork", "QUrl_FormattingOptions", "@brief Binding of QUrl::FormattingOptions");

//  inject as QUrl::FormattingOptions
static gsi::ClassExt<QUrl> decl_QUrlTwoFlags_as_child (decl_QUrlTwoFlags, "FormattingOptions", "@brief Binding of QUrl::FormattingOptions");

}
