
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


#include "gsiDecl.h"

namespace gsi
{

// ---------------------------------------------------------------
//  Access to the internal information of GSI

static int t_void () { return T_void; }
static int t_bool () { return T_bool; }
static int t_char () { return T_char; }
static int t_schar () { return T_schar; }
static int t_uchar () { return T_uchar; }
static int t_short () { return T_short; }
static int t_ushort () { return T_ushort; }
static int t_int () { return T_int; }
static int t_uint () { return T_uint; }
static int t_long () { return T_long; }
static int t_ulong () { return T_ulong; }
static int t_longlong () { return T_longlong; }
static int t_ulonglong () { return T_ulonglong; }
#if defined(HAVE_64BIT_COORD)
static int t_int128 () { return T_int128; }
#endif
static int t_double () { return T_double; }
static int t_float () { return T_float; }
static int t_var () { return T_var; }
static int t_string () { return T_string; }
static int t_byte_array () { return T_byte_array; }
static int t_void_ptr () { return T_void_ptr; }
static int t_object () { return T_object; }
static int t_vector () { return T_vector; }
static int t_map () { return T_map; }

static int type (const ArgType *t)
{
  return t->type ();
}

static const std::string &arg_name (const ArgType *t)
{
  static std::string empty;
  return t->spec () ? t->spec ()->name () : empty;
}

static bool has_default_value (const ArgType *t)
{
  return t->spec () && t->spec ()->has_default ();
}

static tl::Variant default_value (const ArgType *t)
{
  tl::Variant empty;
  return t->spec () ? t->spec ()->default_value () : empty;
}

Class<ArgType> decl_ArgType ("tl", "ArgType",
  gsi::method ("TypeVoid", &t_void) +
  gsi::method ("TypeBool", &t_bool) +
  gsi::method ("TypeChar", &t_char) +
  gsi::method ("TypeSChar", &t_schar) +
  gsi::method ("TypeUChar", &t_uchar) +
  gsi::method ("TypeShort", &t_short) +
  gsi::method ("TypeUShort", &t_ushort) +
  gsi::method ("TypeInt", &t_int) +
  gsi::method ("TypeUInt", &t_uint) +
  gsi::method ("TypeLong", &t_long) +
  gsi::method ("TypeULong", &t_ulong) +
  gsi::method ("TypeLongLong", &t_longlong) +
  gsi::method ("TypeULongLong", &t_ulonglong) +
#if defined(HAVE_64BIT_COORD)
  gsi::method ("TypeInt128|#t_int128", &t_int128) +
#endif
  gsi::method ("TypeDouble", &t_double) +
  gsi::method ("TypeFloat", &t_float) +
  gsi::method ("TypeVar", &t_var) +
  gsi::method ("TypeByteArray", &t_byte_array) +
  gsi::method ("TypeString", &t_string) +
  gsi::method ("TypeVoidPtr", &t_void_ptr) +
  gsi::method ("TypeObject", &t_object) +
  gsi::method ("TypeVector", &t_vector) +
  gsi::method ("TypeMap", &t_map) +
  gsi::method_ext ("type", &type,
    "@brief Return the basic type (see t_.. constants)\n"
  ) +
  gsi::method ("inner", &ArgType::inner,
    "@brief Returns the inner ArgType object (i.e. value of a vector/map)\n"
    "Starting with version 0.22, this method replaces the is_vector method.\n"
  ) +
  gsi::method ("inner_k", &ArgType::inner_k,
    "@brief Returns the inner ArgType object (i.e. key of a map)\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::method ("pass_obj?", &ArgType::pass_obj,
    "@brief True, if the ownership over an object represented by this type is passed to the receiver\n"
    "In case of the return type, a value of true indicates, that the object is a freshly created one and "
    "the receiver has to take ownership of the object.\n\n"
    "This method has been introduced in version 0.24."
  ) +
  gsi::method ("is_ref?", &ArgType::is_ref,
    "@brief True, if the type is a reference to the given type\n"
    "Starting with version 0.22 there are more methods that describe the "
    "type of reference and is_ref? only applies to non-const reference (in C++: 'X &').\n"
  ) +
  gsi::method ("is_cref?", &ArgType::is_cref,
    "@brief True, if the type is a const reference to the given type\n"
    "This property indicates that the argument is a const reference (in C++: 'const X &').\n"
  ) +
  gsi::method ("is_ptr?", &ArgType::is_ptr,
    "@brief True, if the type is a non-const pointer to the given type\n"
    "This property indicates that the argument is a non-const pointer (in C++: 'X *').\n"
  ) +
  gsi::method ("is_cptr?", &ArgType::is_cptr,
    "@brief True, if the type is a const pointer to the given type\n"
    "This property indicates that the argument is a const pointer (in C++: 'const X *').\n"
  ) +
  gsi::method ("is_iter?", &ArgType::is_iter,
    "@brief (Return value only) True, if the return value is an iterator rendering the given type\n"
  ) +
  gsi::method ("cls", &ArgType::cls,
    "@brief Specifies the class for t_object.. types\n"
  ) +
  gsi::method ("to_s", &ArgType::to_string,
    "@brief Convert to a string\n"
  ) +
  gsi::method_ext ("has_default?", &has_default_value,
    "@brief Returns true, if a default value is specified for this argument\n"
    "Applies to arguments only. This method has been introduced in version 0.24."
  ) +
  gsi::method_ext ("default", &default_value,
    "@brief Returns the default value or nil is there is no default value\n"
    "Applies to arguments only. This method has been introduced in version 0.24."
  ) +
  gsi::method_ext ("name", &arg_name,
    "@brief Returns the name for this argument or an empty string if the argument is not named\n"
    "Applies to arguments only. This method has been introduced in version 0.24."
  ) +
  gsi::method ("==", &ArgType::operator==,
    "@brief Equality of two types\n"
  ) +
  gsi::method ("!=", &ArgType::operator!=,
    "@brief Inequality of two types\n"
  ),
  "@hide"
);

static const std::string &_syn_name (const gsi::MethodBase::MethodSynonym *syn)
{
  return syn->name;
}

static bool _syn_deprecated (const gsi::MethodBase::MethodSynonym *syn)
{
  return syn->deprecated;
}

static bool _syn_is_predicate (const gsi::MethodBase::MethodSynonym *syn)
{
  return syn->is_predicate;
}

static bool _syn_is_getter (const gsi::MethodBase::MethodSynonym *syn)
{
  return syn->is_getter;
}

static bool _syn_is_setter (const gsi::MethodBase::MethodSynonym *syn)
{
  return syn->is_setter;
}

Class<MethodBase::MethodSynonym> decl_MethodOverload ("tl", "MethodOverload",
  gsi::method_ext ("name", &_syn_name,
    "@brief The name of this overload\n"
    "This is the raw, unadorned name. I.e. no question mark suffix for predicates, no "
    "equal character suffix for setters etc.\n"
  ) +
  gsi::method_ext ("deprecated?", &_syn_deprecated,
    "@brief A value indicating that this overload is deprecated\n"
  ) +
  gsi::method_ext ("is_getter?", &_syn_is_getter,
    "@brief A value indicating that this overload is a property getter\n"
  ) +
  gsi::method_ext ("is_setter?", &_syn_is_setter,
    "@brief A value indicating that this overload is a property setter\n"
  ) +
  gsi::method_ext ("is_predicate?", &_syn_is_predicate,
    "@brief A value indicating that this overload is a predicate\n"
  ),
  "@hide"
);

Class<MethodBase> decl_Method ("tl", "Method",
  gsi::iterator ("each_argument", &MethodBase::begin_arguments, &MethodBase::end_arguments,
    "@brief Iterate over all arguments of this method\n"
  ) +
  gsi::method ("ret_type", (const gsi::ArgType &(MethodBase::*)() const) &MethodBase::ret_type,
    "@brief The return type of this method\n"
  ) +
  gsi::method ("is_protected?", &MethodBase::is_protected,
    "@brief True, if this method is protected\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) +
  gsi::iterator ("each_overload", &MethodBase::begin_synonyms, &MethodBase::end_synonyms,
    "@brief This iterator delivers the synonyms (overloads).\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) +
  gsi::method ("accepts_num_args", &MethodBase::compatible_with_num_args,
    "@brief True, if this method is compatible with the given number of arguments\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) +
  gsi::method ("is_const?", &MethodBase::is_const,
    "@brief True, if this method does not alter the object\n"
  ) +
  gsi::method ("is_static?", &MethodBase::is_static,
    "@brief True, if this method is static (a class method)\n"
  ) +
  gsi::method ("is_constructor?", &MethodBase::is_constructor,
    "@brief True, if this method is a constructor\n"
    "Static methods that return new objects are constructors.\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("is_signal?", &MethodBase::is_signal,
    "@brief True, if this method is a signal\n"
    "\n"
    "Signals replace events for version 0.25. is_event? is no longer available."
  ) +
  gsi::method ("name", &MethodBase::combined_name,
    "@brief The name string of the method\n"
    "A method may have multiple names (aliases). The name string delivers all of them in a combined way.\n"
    "\n"
    "The names are separated by pipe characters (|). A trailing star (*) indicates that the method is protected.\n"
    "\n"
    "Names may be prefixed by a colon (:) to indicate a property getter. This colon does not appear in the "
    "method name.\n"
    "\n"
    "A hash prefix indicates that a specific alias is deprecated.\n"
    "\n"
    "Names may be suffixed by a question mark (?) to indicate a predicate or a equal character (=) to indicate "
    "a property setter. Depending on the preferences of the language, these characters may appear in the "
    "method names of not - in Python they don't, in Ruby they will be part of the method name.\n"
    "\n"
    "The backslash character is used inside the names to escape these special characters.\n"
    "\n"
    "The preferred method of deriving the overload is to iterate then using \\each_overload.\n"
  ) +
  gsi::method ("primary_name", &MethodBase::primary_name,
    "@brief The primary name of the method\n"
    "The primary name is the first name of a sequence of aliases.\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) +
  gsi::method ("doc", &MethodBase::doc,
    "@brief The documentation string for this method\n"
  ),
  "@hide"
);

Class<ClassBase> decl_Class ("tl", "Class",
  gsi::iterator ("each_class", &ClassBase::begin_classes, &ClassBase::end_classes,
    "@brief Iterate over all classes\n"
  ) +
  gsi::iterator ("each_method", &ClassBase::begin_methods, &ClassBase::end_methods,
    "@brief Iterate over all methods of this class\n"
  ) +
  gsi::iterator ("each_child_class", &ClassBase::begin_child_classes, &ClassBase::end_child_classes,
    "@brief Iterate over all child classes defined within this class\n"
  ) +
  gsi::method ("parent", &ClassBase::parent,
    "@brief The parent of the class\n"
  ) +
  gsi::method ("name", &ClassBase::name,
    "@brief The name of the class\n"
  ) +
  gsi::method ("module", &ClassBase::module,
    "@brief The name of module where the class lives\n"
  ) +
  gsi::method ("base", &ClassBase::base,
    "@brief The base class or nil if the class does not have a base class\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method ("can_copy?", &ClassBase::can_copy,
    "@brief True if the class offers assignment\n"
  ) +
  gsi::method ("can_destroy?", &ClassBase::can_destroy,
    "@brief True if the class offers a destroy method\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method ("doc", &ClassBase::doc,
    "@brief The documentation string for this class\n"
  ),
  "@hide"
);

}
