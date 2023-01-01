
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
#include "dbInstElement.h"

namespace gsi
{
  
static const db::Instance &inst (const db::InstElement *ie)
{
  return ie->inst_ptr;
}

static const db::CellInstArray *cell_inst (const db::InstElement *ie)
{
  return &ie->inst_ptr.cell_inst ();
}

static db::properties_id_type prop_id (const db::InstElement *ie)
{
  return ie->inst_ptr.prop_id ();
}

static db::Trans specific_trans (const db::InstElement *ie)
{
  return *ie->array_inst;
}

static db::ICplxTrans specific_cplx_trans (const db::InstElement *ie)
{
  return ie->inst_ptr.cell_inst ().complex_trans (*ie->array_inst);
}

static db::Trans array_member_trans (const db::InstElement *ie)
{
  return *ie->array_inst * ie->inst_ptr.front ().inverted ();
}

static long array_index_a (const db::InstElement *ie)
{
  return ie->array_inst.index_a ();
}

static long array_index_b (const db::InstElement *ie)
{
  return ie->array_inst.index_b ();
}

static db::InstElement *new_i (const db::Instance &i)
{
  return new db::InstElement (i);
}

static db::InstElement *new_v ()
{
  return new db::InstElement ();
}

static db::InstElement *new_iab (const db::Instance &i, unsigned long na, unsigned long nb)
{
  db::Vector a, b;
  unsigned long amax, bmax;
  if (i.is_regular_array (a, b, amax, bmax)) {
    return new db::InstElement (i, db::CellInstArray::iterator (i.front (), new db::regular_array_iterator <db::Coord> (a, b, na, na, nb, nb)));
  } else {
    return new_i (i);
  }
}

Class<db::InstElement> decl_InstElement ("db", "InstElement",
  gsi::constructor ("new", &new_v,
    "@brief Default constructor"
  ) +
  gsi::constructor ("new|#new_i", &new_i, gsi::arg ("inst"),
    "@brief Create an instance element from a single instance alone\n"
    "Starting with version 0.15, this method takes an \\Instance object (an instance reference) as the argument.\n"
  ) +
  gsi::constructor ("new|#new_iab", &new_iab, gsi::arg ("inst"), gsi::arg ("a_index"), gsi::arg ("b_index"),
    "@brief Create an instance element from an array instance pointing into a certain array member\n"
    "Starting with version 0.15, this method takes an \\Instance object (an instance reference) as the first argument.\n"
  ) +
  gsi::method_ext ("inst", &inst,
    "@brief Gets the \\Instance object held in this instance path element.\n"
    "\n"
    "This method has been added in version 0.24.\n"
  ) +
  gsi::method_ext ("cell_inst", &cell_inst,
    "@brief Accessor to the cell instance (array).\n"
    "\n"
    "This method is equivalent to \"self.inst.cell_inst\" and provided for convenience.\n"
  ) +
  gsi::method_ext ("prop_id", &prop_id,
    "@brief Accessor to the property attached to this instance.\n"
    "\n"
    "This method is equivalent to \"self.inst.prop_id\" and provided for convenience.\n"
  ) +
  gsi::method ("<", &db::InstElement::operator<, gsi::arg ("b"),
    "@brief Provides an order criterion for two InstElement objects\n"
    "Note: this operator is just provided to establish any order, not a particular one."
  ) +
  gsi::method ("!=", &db::InstElement::operator!=, gsi::arg ("b"),
    "@brief Inequality of two InstElement objects\n"
    "See the comments on the == operator.\n"
  ) +
  gsi::method ("==", &db::InstElement::operator==, gsi::arg ("b"),
    "@brief Equality of two InstElement objects\n"
    "Note: this operator returns true if both instance elements refer to the same instance, not just identical ones."
  ) +
  gsi::method_ext ("ia", &array_index_a,
    "@brief Returns the 'a' axis index for array instances\n"
    "For instance elements describing one member of an array, this attribute will deliver "
    "the a axis index addressed by this element. See \\ib and \\array_member_trans "
    "for further attributes applicable to array members.\n"
    "If the element is a plain instance and not an array member, this attribute is a negative value.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("ib", &array_index_b,
    "@brief Returns the 'b' axis index for array instances\n"
    "For instance elements describing one member of an array, this attribute will deliver "
    "the a axis index addressed by this element. See \\ia and \\array_member_trans "
    "for further attributes applicable to array members.\n"
    "If the element is a plain instance and not an array member, this attribute is a negative value.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("specific_trans", &specific_trans,
    "@brief Returns the specific transformation for this instance\n"
    "\n"
    "The specific transformation is the one applicable for the member selected from an array.\n"
    "This is the effective transformation applied for this array member. "
    "\\array_member_trans gives the transformation applied additionally to the instances' "
    "global transformation (in other words, specific_trans = array_member_trans * cell_inst.trans).\n"
    "This method delivers a simple transformation that does not include magnification components. To get "
    "these as well, use \\specific_cplx_trans."
  ) +
  gsi::method_ext ("specific_cplx_trans", &specific_cplx_trans,
    "@brief Returns the specific complex transformation for this instance\n"
    "\n"
    "The specific transformation is the one applicable for the member selected from an array.\n"
    "This is the effective transformation applied for this array member. "
    "\\array_member_trans gives the transformation applied additionally to the "
    "instances' global transformation (in other words, specific_cplx_trans = array_member_trans * cell_inst.cplx_trans).\n"
  ) +
  gsi::method_ext ("array_member_trans", &array_member_trans,
    "@brief Returns the transformation for this array member\n"
    "\n"
    "The array member transformation is the one applicable in addition to the global transformation for the member selected from an array.\n"
    "If this instance is not an array instance, the specific transformation is a unit transformation without displacement.\n"
  ),
  "@brief An element in an instantiation path\n"
  "\n"
  "This objects are used to reference a single instance in a instantiation path. The object is composed "
  "of a \\CellInstArray object (accessible through the \\cell_inst accessor) that describes the basic instance, which may be an array. The particular "
  "instance within the array can be further retrieved using the \\array_member_trans, \\specific_trans or \\specific_cplx_trans methods."
);

}  // namespace gsi
