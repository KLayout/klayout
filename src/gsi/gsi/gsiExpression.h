
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


#ifndef HDR_gsiExpression
#define HDR_gsiExpression

#include "tlExpression.h"

#include "gsiCommon.h"

#include <string>
#include <map>

namespace gsi 
{

class GSI_PUBLIC ClassBase;
struct NoAdaptorTag;
template <class T, class A> class GSI_PUBLIC_TEMPLATE Class;

/**
 *  @brief The implementation delegate for the VariantUserClass<T>
 */
class GSI_PUBLIC VariantUserClassImpl
  : public tl::EvalClass
{
public:
  VariantUserClassImpl ();
  ~VariantUserClassImpl ();

  bool equal_impl (void *, void *) const;
  bool less_impl (void *, void *) const;
  tl::Variant to_variant_impl (void *) const;
  std::string to_string_impl (void *) const;
  int to_int_impl (void *) const;
  double to_double_impl (void *) const;

  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args) const;

  void initialize (const gsi::ClassBase *cls, const tl::VariantUserClassBase *self, const tl::VariantUserClassBase *object_cls, bool is_const);

private:
  const gsi::ClassBase *mp_cls;
  const tl::VariantUserClassBase *mp_self, *mp_object_cls;
  bool m_is_const;

  virtual void execute_gsi (const tl::ExpressionParserContext &context, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args) const;

  bool has_method (const std::string &method) const;
};

/**
 *  @brief Initialize GSI objects for expressions
 *
 *  This function must be called initially to enable GSI objects into expressions.
 */
GSI_PUBLIC void
initialize_expressions ();

}

#endif

