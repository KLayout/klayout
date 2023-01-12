
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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


#ifndef _HDR_gsiEnums
#define _HDR_gsiEnums

#include "gsiDecl.h"
#include "tlString.h"
#include "tlVariant.h"

#if defined(HAVE_QT)
#  include <QFlags>
#endif

namespace gsi
{
 
template <class E> class Enum;

/**
 *  @brief The basic enum adaptor class 
 *  We will later bind this class to an enum. Binding is automatically resolved by GSI.
 *  The adaptor provides a client-side object which connects to the enum features.
 *  It employs the declaration class to retrieve the constants defined.
 */
template <class E>
class EnumAdaptor
{
public:
  EnumAdaptor () : m_e (E (0)) { }
  EnumAdaptor (E e) : m_e (e) { }
  EnumAdaptor (int e) : m_e (E (e)) { }

  EnumAdaptor (const std::string &e) 
  {
    const Enum<E> *ecls = dynamic_cast<const Enum<E> *> (cls_decl<E> ());
    tl_assert (ecls != 0);
    m_e = ecls->enum_from_string (e);
  }

  E &value () { return m_e; }
  const E &value () const { return m_e; }

  int to_int () const { return int (m_e); }
  tl::Variant to_variant () const { return tl::Variant (int (m_e)); }

  std::string to_string () const
  {
    const Enum<E> *ecls = dynamic_cast<const Enum<E> *> (cls_decl<E> ());
    tl_assert (ecls != 0);
    return ecls->enum_to_string (m_e);
  }

private:
  E m_e;
};

/**
 *  @brief A single specification for an enum value
 */
template <class E>
struct EnumSpec
{
  EnumSpec (const std::string &s, E e, const std::string &d)
    : str (s), evalue (e), doc (d)
  {
  }

  std::string str;
  E evalue;
  std::string doc;
};

/**
 *  @brief A method implementation which represents an enum constant
 *  This method is called by the client side to retrieve an enum constant.
 */
template <class E>
class EnumConst
  : public MethodBase
{
public:
  EnumConst (const std::string &name, E evalue, const std::string &doc)
    : MethodBase (name, doc, true, true), m_evalue (evalue)
  {
  }

  virtual void initialize () 
  {
    set_return<E> ();
  }
    
  virtual void call (void * /*obj*/, SerialArgs & /*args*/, SerialArgs &ret) const 
  {
    ret.write<E> (m_evalue);
  }

  virtual MethodBase *clone () const
  {
    return new EnumConst (*this);
  }

private:
  E m_evalue;
};

/**
 *  @brief A list of constants
 *  This list is passed to the declaration class to represent the list of constants 
 *  in the enum.
 *  Building of the list is facilitated by the "gsi::enum_const" function. The use
 *  case is supposed to be this:
 *
 *  @code
 *  gsi::enum_const("a", Enum::a, "doc...") +
 *  gsi::enum_const("b", Enum::b) ...
 *  @endcode
 */
template <class E>
class EnumSpecs
{
public:
  typedef typename std::vector<EnumSpec<E> >::const_iterator iterator;

  EnumSpecs (const std::string &estr, E evalue, const std::string &doc)
  {
    m_specs.push_back (EnumSpec<E> (estr, evalue, doc));
  }

  iterator begin () const
  {
    return m_specs.begin ();
  }

  iterator end () const
  {
    return m_specs.end ();
  }

  EnumSpecs &operator+ (const EnumSpecs &other) 
  {
    return operator+= (other);
  }

  EnumSpecs &operator+= (const EnumSpecs &other) 
  {
    m_specs.insert (m_specs.end (), other.m_specs.begin (), other.m_specs.end ());
    return *this;
  }

  E enum_from_string (const std::string &s) const
  {
    for (typename std::vector<EnumSpec<E> >::const_iterator spec = m_specs.begin (); spec != m_specs.end (); ++spec) {
      if (spec->str == s) {
        return spec->evalue;
      }
    }

    tl::Extractor ex (s.c_str ());
    ex.test ("#");
    int e = 0;
    if (ex.try_read (e)) {
      return E (e);
    } else {
      return E ();
    }
  }

  std::string enum_to_string (E e) const
  {
    for (typename std::vector<EnumSpec<E> >::const_iterator spec = m_specs.begin (); spec != m_specs.end (); ++spec) {
      if (spec->evalue == e) {
        return spec->str;
      }
    }
    return std::string (tl::sprintf ("#%d", int (e)));
  }

  std::string enum_to_string_inspect (E e) const
  {
    for (typename std::vector<EnumSpec<E> >::const_iterator spec = m_specs.begin (); spec != m_specs.end (); ++spec) {
      if (spec->evalue == e) {
        return spec->str + tl::sprintf (" (%d)", int (e));
      }
    }
    return std::string ("(not a valid enum value)");
  }

  static std::string enum_to_string_ext (const E *e)
  {
    const Enum<E> *ecls = dynamic_cast<const Enum<E> *> (cls_decl<E> ());
    tl_assert (ecls != 0);
    return ecls->enum_to_string (*e);
  }

  static std::string enum_to_string_inspect_ext (const E *e)
  {
    const Enum<E> *ecls = dynamic_cast<const Enum<E> *> (cls_decl<E> ());
    tl_assert (ecls != 0);
    return ecls->enum_to_string_inspect (*e);
  }

  static int enum_to_int (const E *e)
  {
    return int (*e); 
  }

  static bool enum_eq (const E *e, const E &other) 
  {
    return *e == other;
  }

  static bool enum_eq_with_int (const E *e, int other)
  {
    return int (*e) == other;
  }

  static bool enum_ne (const E *e, const E &other)
  {
    return *e != other;
  }

  static bool enum_ne_with_int (const E *e, int other)
  {
    return int (*e) != other;
  }

  static bool enum_lt (const E *e, const E &other)
  {
    return *e < other;
  }

  static bool enum_lt_with_int (const E *e, int other)
  {
    return int (*e) < other;
  }

  static E *new_enum_from_int (int i)
  {
    return new E (E (i));
  }

  static E *new_enum_from_string (const std::string &s)
  {
    const Enum<E> *ecls = dynamic_cast<const Enum<E> *> (cls_decl<E> ());
    tl_assert (ecls != 0);
    return new E (ecls->enum_from_string (s));
  }

  gsi::Methods methods () const
  {
    gsi::Methods m = 
      gsi::constructor ("new", &new_enum_from_int, gsi::arg("i"), "@brief Creates an enum from an integer value") +
      gsi::constructor ("new", &new_enum_from_string, gsi::arg("s"), "@brief Creates an enum from a string value") +
      gsi::method_ext ("to_s", &enum_to_string_ext, "@brief Gets the symbolic string from an enum") +
      gsi::method_ext ("inspect", &enum_to_string_inspect_ext, "@brief Converts an enum to a visual string") +
      gsi::method_ext ("to_i", &enum_to_int, "@brief Gets the integer value from the enum") +
      gsi::method_ext ("==", &enum_eq, gsi::arg("other"), "@brief Compares two enums") +
      gsi::method_ext ("==", &enum_eq_with_int, gsi::arg("other"), "@brief Compares an enum with an integer value") +
      gsi::method_ext ("!=", &enum_ne, gsi::arg("other"), "@brief Compares two enums for inequality") +
      gsi::method_ext ("!=", &enum_ne_with_int, gsi::arg("other"), "@brief Compares an enum with an integer for inequality") +
      gsi::method_ext ("<", &enum_lt, gsi::arg("other"), "@brief Returns true if the first enum is less (in the enum symbol order) than the second") +
      gsi::method_ext ("<", &enum_lt_with_int, gsi::arg("other"), "@brief Returns true if the enum is less (in the enum symbol order) than the integer value");

    return m + defs ();
  }

  gsi::Methods defs () const
  {
    gsi::Methods m;
    for (typename std::vector<EnumSpec<E> >::const_iterator spec = m_specs.begin (); spec != m_specs.end (); ++spec) {
      m += gsi::Methods (new EnumConst<E> (spec->str, spec->evalue, spec->doc));
    }
    return m;
  }

private:
  std::vector<EnumSpec<E> > m_specs;
};

/**
 *  @brief A helper function to build declaration lists
 *  See \EnumSpecs for details.
 */
template <class E>
EnumSpecs<E> enum_const (const std::string &estr, E evalue, const std::string &doc = std::string ())
{
  return EnumSpecs<E> (estr, evalue, doc);
}

/**
 *  @brief A helper class for the enum implementation
 */
template <class E>
class EnumImpl
{
public:
  EnumImpl (const EnumSpecs<E> &specs)
    : m_specs (specs)
  {
  }

  gsi::Methods defs () const
  {
    return m_specs.defs ();
  }

  std::string enum_to_string (E e) const
  {
    return m_specs.enum_to_string (e);
  }

  std::string enum_to_string_inspect (E e) const
  {
    return m_specs.enum_to_string_inspect (e);
  }

  E enum_from_string (const std::string &e) const
  {
    return m_specs.enum_from_string (e);
  }

  const EnumSpecs<E> &specs () const
  {
    return m_specs;
  }

private:
  EnumSpecs<E> m_specs;
};

/**
 *  @brief The basic declaration class
 *  This is a gsi::Class specialization which implements an enum declaration.
 *  The use is this:
 *
 *  @code
 *  gsi::Enum<E> e_enum ("A description", 
 *    gsi::enum_const ("a", E::a, "description of a") + 
 *    gsi::enum_const ("b", E::b) + 
 *    ...
 *  );
 *  @endcode
 */
template <class E>
class Enum
  : public Class<EnumAdaptor<E>, E>, public EnumImpl<E>
{
public:
  Enum (const std::string &module, const std::string &name, const EnumSpecs<E> &specs, const std::string &doc = std::string ())
    : Class<EnumAdaptor<E>, E> (module, name, specs.methods (), doc), EnumImpl<E> (specs)
  {
  }
};

/**
 *  @brief An enum declaration as a child class
 *
 *  @code
 *  gsi::EnumIn<P, E> e_enum ("A description",
 *    gsi::enum_const ("a", E::a, "description of a") +
 *    gsi::enum_const ("b", E::b) +
 *    ...
 *  );
 *  @endcode
 */
template <class P, class E>
class EnumIn
  : public Enum<E>
{
public:
  EnumIn (const std::string &module, const std::string &name, const EnumSpecs<E> &specs, const std::string &doc = std::string ())
    : Enum<E> (module, name, specs, doc)
  {
  }

  virtual bool consolidate () const
  {
    //  TODO: ugly const cast
    ClassBase *non_const_pcls = const_cast<ClassBase *> (cls_decl<P> ());
    non_const_pcls->add_child_class (this);

    //  no longer required as it is a child now.
    return false;
  }
};

#if defined(HAVE_QT)

template <class E>
class QFlagsClass;

/**
 *  @brief An adaptor class for the QFlags<E> template
 */
template <class E>
class QFlagsAdaptor
{
public:
  QFlagsAdaptor () : m_qf () { }
  QFlagsAdaptor (E e) : m_qf (e) { }
  QFlagsAdaptor (const QFlags<E> &qf) : m_qf (qf) { }
  QFlagsAdaptor (int i) : m_qf (i) { }

  QFlagsAdaptor (const std::string &s) 
  {
    const QFlagsClass<E> *ecls = dynamic_cast<const QFlagsClass<E> *> (cls_decl<QFlags<E> > ());
    tl_assert (ecls != 0);
    m_qf = ecls->qflags_from_string (s);
  }

  QFlags<E> &value () { return m_qf; }
  const QFlags<E> &value () const { return m_qf; }

private:
  QFlags<E> m_qf;
};

/**
 *  @brief A class creating an automatic binding for QFlags templates
 *
 *  Instantiating this class will provide a new client-side class as
 *  an adaptor for the QFlags template. The use scenario is that:
 *
 *  @code
 *  enum E { ... };
 *
 *  //  the enum declaration (mandatory)
 *  Enum<E> enum_decl ("E", ...);
 *
 *  //  provide additional declarations for QFlags<E> (optional)
 *  QFlagsClass<E> qflags_decl ("QFlags_E", "documentation");
 *  @endcode
 *
 *  The documentation is optional. In addition to the new class "QFlags_E", this 
 *  declaration will also provide an "or" operator between two enums of type E
 *  which will render a QFlags object.
 */
template <class E>
class QFlagsClass
  : public Class<QFlagsAdaptor<E>, QFlags<E> >
{
public:
#if QT_VERSION >= 0x050000
  typedef typename QFlags<E>::Int int_repr;
#else
  typedef int int_repr;
#endif

  QFlagsClass (const std::string &module, const std::string &name, const std::string &doc = std::string ())
    : Class<QFlagsAdaptor<E>, QFlags<E> > (module, name, methods (), doc),
      m_enum_ext (ext_methods ())
  {
  }

  static QFlags<E> *new_from_i (int_repr i)
  {
    return new QFlags<E> (i);
  }

  static QFlags<E> *new_from_e (E e)
  {
    return new QFlags<E> (e);
  }

  static QFlags<E> *new_from_s (const std::string &s)
  {
    QFlags<E> flags;

    const Enum<E> *ecls = dynamic_cast<const Enum<E> *> (cls_decl<E> ());
    tl_assert (ecls != 0);

    tl::Extractor ex (s.c_str ());
    while (! ex.at_end ()) {

      bool any = false;
      for (typename EnumSpecs<E>::iterator s = ecls->specs ().begin (); s != ecls->specs ().end () && !any; ++s) {
        if (ex.test (s->str.c_str ())) {
          flags |= E (s->evalue);
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

    return new QFlags<E> (flags);
  }

  static std::string to_s (const QFlags<E> *self)
  {
    std::string res;

    const Enum<E> *ecls = dynamic_cast<const Enum<E> *> (cls_decl<E> ());
    tl_assert (ecls != 0);
    for (typename EnumSpecs<E>::iterator s = ecls->specs ().begin (); s != ecls->specs ().end (); ++s) {
      if (self->testFlag (s->evalue)) {
        if (! res.empty ()) {
          res += "|";
        }
        res += s->str;
      }
    }

    return res;
  }

  static int_repr to_i (const QFlags<E> *self)
  {
    return int_repr (*self);
  }

  static bool test_flag (const QFlags<E> *self, E e)
  {
    return self->testFlag (e);
  }

  static std::string inspect (const QFlags<E> *self)
  {
    return to_s (self) + tl::sprintf(" (%u)", (unsigned int) (int_repr (*self)));
  }

  static QFlags<E> invert (const QFlags<E> *self)
  {
    return ~*self;
  }

  static QFlags<E> or_op (const QFlags<E> *self, const QFlags<E> &other)
  {
    return *self | other;
  }

  static QFlags<E> or_op_with_e (const QFlags<E> *self, E e)
  {
    return *self | e;
  }

  static QFlags<E> and_op (const QFlags<E> *self, const QFlags<E> &other)
  {
    return *self & other;
  }

  static QFlags<E> and_op_with_e (const QFlags<E> *self, E e)
  {
    return *self & e;
  }

  static QFlags<E> xor_op (const QFlags<E> *self, const QFlags<E> &other)
  {
    return *self ^ other;
  }

  static QFlags<E> xor_op_with_e (const QFlags<E> *self, E e)
  {
    return *self ^ e;
  }

  static bool not_equal_with_i (const QFlags<E> *self, int_repr i)
  {
    return int_repr (*self) != i;
  }

  static bool not_equal (const QFlags<E> *self, const QFlags<E> &other)
  {
    //  See equal() for an explanation why we compare int's.
    return int_repr (*self) != int_repr (other);
  }

  static bool equal_with_i (const QFlags<E> *self, int_repr i)
  {
    return int_repr (*self) == i;
  }

  static bool equal (const QFlags<E> *self, const QFlags<E> &other)
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
      gsi::constructor ("new", &new_from_e, gsi::arg ("e"), "@brief Creates a flag set from an enum") +
      gsi::method_ext ("to_s", &to_s, "@brief Converts the flag set to a string") +
      gsi::method_ext ("to_i", &to_i, "@brief Converts the flag set to an integer") +
      gsi::method_ext ("testFlag", &test_flag, gsi::arg ("flag"), "@brief Tests whether the flag set contains the given flag") +
      gsi::method_ext ("inspect", &inspect, "@brief Converts the flag set to a visual string") +
      gsi::method_ext ("|", &or_op, gsi::arg ("other"), "@brief Computes the union of two flag sets") +
      gsi::method_ext ("|", &or_op_with_e, gsi::arg ("flag"), "@brief Adds the given flag to the flag set and returns the new flag set") +
      gsi::method_ext ("&", &and_op, gsi::arg ("other"), "@brief Computes the intersection between the two flag sets") +
      gsi::method_ext ("&", &and_op_with_e, gsi::arg ("flag"), "@brief Tests whether the given flag is contained in the flag set and returns a null flag set if not") +
      gsi::method_ext ("^", &xor_op, gsi::arg ("other"), "@brief Computes the exclusive-or between the flag set and the other flag set") +
      gsi::method_ext ("^", &xor_op_with_e, gsi::arg ("flag"), "@brief Inverts the given flag in the flag set and returns the new flag set") +
      gsi::method_ext ("==", &equal_with_i, gsi::arg ("other"), "@brief Returns true if the flag set equals the given integer value") +
      gsi::method_ext ("==", &equal, gsi::arg ("i"), "@brief Returns true if the flag set equals the given other flag set") +
      gsi::method_ext ("!=", &not_equal_with_i, gsi::arg ("other"), "@brief Returns true if the flag set is not equal to the given integer value") +
      gsi::method_ext ("!=", &not_equal, gsi::arg ("i"), "@brief Returns true if the flag set is not equal to the given other flag set") +
      gsi::method_ext ("~", &invert, "@brief Returns the inverted flag set");     
  }

  static QFlags<E> e_or_e (const E *self, const E &other)
  {
    return QFlags<E> (*self) | other;
  }

  static QFlags<E> e_or_ee (const E *self, const QFlags<E> &other)
  {
    return QFlags<E> (*self) | other;
  }

  static gsi::Methods ext_methods ()
  {
    return 
      gsi::method_ext ("|", &e_or_e, gsi::arg ("other"), "@brief Creates a flag set by combining the two flags") +
      gsi::method_ext ("|", &e_or_ee, gsi::arg ("other"), "@brief Combines the flag and the flag set");
  }

private:
  gsi::ClassExt<E> m_enum_ext;
};

#endif

}

#endif

