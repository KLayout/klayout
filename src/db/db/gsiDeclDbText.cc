
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
#include "gsiEnums.h"
#include "dbPoint.h"
#include "dbText.h"
#include "dbHash.h"

namespace gsi
{

// ---------------------------------------------------------------
//  text binding

template <class C>
struct text_defs 
{
  typedef typename C::coord_type coord_type;
  typedef typename C::box_type box_type;
  typedef typename C::point_type point_type;
  typedef typename C::vector_type vector_type;
  typedef db::simple_trans<coord_type> simple_trans_type;
  typedef db::complex_trans<coord_type, double> complex_trans_type;

  static C *from_string (const char *s)
  {
    tl::Extractor ex (s);
    std::unique_ptr<C> c (new C ());
    ex.read (*c.get ());
    return c.release ();
  }

  static C *new_v ()
  {
    return new C ();
  }

  static C *new_sxy (const char *s, coord_type x, coord_type y)
  {
    return new C (s, simple_trans_type (vector_type (x, y)));
  }

  static C *new_st (const char *s, const simple_trans_type &t)
  {
    return new C (s, t);
  }

  static C *new_sthf (const char *s, const simple_trans_type &t, coord_type h, int f)
  {
    return new C (s, t, h, db::Font (f));
  }

  static void set_x (C *t, coord_type x)
  {
    simple_trans_type tr = t->trans ();
    t->trans (simple_trans_type (tr.rot (), vector_type (x, tr.disp ().y ())));
  }

  static coord_type get_x (const C *t)
  {
    return t->trans ().disp ().x ();
  }

  static void set_y (C *t, coord_type y)
  {
    simple_trans_type tr = t->trans ();
    t->trans (simple_trans_type (tr.rot (), vector_type (tr.disp ().x (), y)));
  }

  static coord_type get_y (const C *t)
  {
    return t->trans ().disp ().y ();
  }

  static void set_font (C *t, int f)
  {
    t->font (db::Font (f));
  }

  static int get_font (C *t)
  {
    return t->font ();
  }

  static point_type get_pos (C *t)
  {
    return t->trans () * point_type ();
  }

  static box_type get_bbox (C *t)
  {
    point_type p = get_pos (t);
    return box_type (p, p);
  }

  static void set_halign (C *t, db::HAlign f)
  {
    t->halign (f);
  }

  static void set_halign_int (C *t, int f)
  {
    t->halign (db::HAlign (f));
  }

  static db::HAlign get_halign (C *t)
  {
    return t->halign ();
  }

  static void set_valign (C *t, db::VAlign f)
  {
    t->valign (f);
  }

  static void set_valign_int (C *t, int f)
  {
    t->valign (db::VAlign (f));
  }

  static db::VAlign get_valign (C *t)
  {
    return t->valign ();
  }

  static C moved (C *c, const vector_type &p)
  {
    return c->transformed (simple_trans_type (p));
  }

  static C &move (C *c, const vector_type &p)
  {
    c->transform (simple_trans_type (p));
    return *c;
  }

  static C moved_xy (C *c, coord_type dx, coord_type dy)
  {
    return c->transformed (simple_trans_type (vector_type (dx, dy)));
  }

  static C &move_xy (C *c, coord_type dx, coord_type dy)
  {
    c->transform (simple_trans_type (vector_type (dx, dy)));
    return *c;
  }

  static size_t hash_value (const C *box)
  {
    return std::hfunc (*box);
  }

  static gsi::Methods methods ()
  {
    return
    constructor ("new", &new_v, 
      "@brief Default constructor\n"
      "\n"
      "Creates a text with unit transformation and empty text."
    ) +
    constructor ("new", &new_st, gsi::arg ("string"), gsi::arg ("trans"),
      "@brief Constructor with string and transformation\n"
      "\n"
      "\n"
      "A string and a transformation is provided to this constructor. The transformation "
      "specifies the location and orientation of the text object."
    ) +
    constructor ("new", &new_sxy, gsi::arg ("string"), gsi::arg ("x"), gsi::arg ("y"),
      "@brief Constructor with string and location\n"
      "\n"
      "\n"
      "A string and a location is provided to this constructor. The location "
      "is specifies as a pair of x and y coordinates.\n"
      "\n"
      "This method has been introduced in version 0.23."
    ) +
    constructor ("new", &new_sthf, gsi::arg ("string"), gsi::arg ("trans"), gsi::arg ("height"), gsi::arg ("font"),
      "@brief Constructor with string, transformation, text height and font\n"
      "\n"
      "\n"
      "A string and a transformation is provided to this constructor. The transformation "
      "specifies the location and orientation of the text object. In addition, the text height "
      "and font can be specified."
    ) +
    method ("string=", (void (C::*) (const std::string &)) &C::string, gsi::arg ("text"),
      "@brief Assign a text string to this object\n"
    ) +
    method ("string", (const char *(C::*) () const) &C::string,
      "@brief Get the text string\n"
    ) +
    method_ext ("position", get_pos,
      "@brief Gets the position of the text\n"
      "\n"
      "This convenience method has been added in version 0.28."
    ) +
    method_ext ("bbox", get_bbox,
      "@brief Gets the bounding box of the text\n"
      "The bounding box of the text is a single point - the location of the text. "
      "Both points of the box are identical.\n"
      "\n"
      "This method has been added in version 0.28."
    ) +
    method_ext ("x=", set_x, gsi::arg ("x"),
      "@brief Sets the x location of the text\n"
      "\n"
      "This method has been introduced in version 0.23.\n"
    ) +
    method_ext ("x", get_x,
      "@brief Gets the x location of the text\n"
      "\n"
      "This method has been introduced in version 0.23.\n"
    ) +
    method_ext ("y=", set_y, gsi::arg ("y"),
      "@brief Sets the y location of the text\n"
      "\n"
      "This method has been introduced in version 0.23.\n"
    ) +
    method_ext ("y", get_y,
      "@brief Gets the y location of the text\n"
      "\n"
      "This method has been introduced in version 0.23.\n"
    ) +
    method ("trans=", (void (C::*) (const simple_trans_type &)) &C::trans, gsi::arg ("t"),
      "@brief Assign a transformation (text position and orientation) to this object\n"
    ) +
    method ("trans", (const simple_trans_type & (C::*) () const) &C::trans,
      "@brief Gets the transformation\n"
    ) +
    method ("size=", (void (C::*) (coord_type)) &C::size, gsi::arg ("s"),
      "@brief Sets the text height of this object\n"
    ) +
    method ("size", (coord_type (C::*) () const) &C::size,
      "@brief Gets the text height\n"
    ) +
    method_ext ("font=", &set_font, gsi::arg ("f"),
      "@brief Sets the font number\n"
      "The font number does not play a role for KLayout. This property is provided "
      "for compatibility with other systems which allow using different fonts for the text objects."
    ) +
    method_ext ("font", &get_font,
      "@brief Gets the font number\n"
      "See \\font= for a description of this property."
    ) +
    method_ext ("#halign=", &set_halign_int, gsi::arg ("a"),
      "@brief Sets the horizontal alignment\n"
      "\n"
      "This is the version accepting integer values. It's provided for backward compatibility.\n"
    ) +
    method_ext ("halign=", &set_halign, gsi::arg ("a"),
      "@brief Sets the horizontal alignment\n"
      "\n"
      "This property specifies how the text is aligned relative to the anchor point. "
      "\n"
      "This property has been introduced in version 0.22 and extended to enums in 0.28.\n"
    ) +
    method_ext ("halign", &get_halign,
      "@brief Gets the horizontal alignment\n"
      "\n"
      "See \\halign= for a description of this property.\n"
    ) +
    method_ext ("#valign=", &set_valign_int, gsi::arg ("a"),
      "@brief Sets the vertical alignment\n"
      "\n"
      "This is the version accepting integer values. It's provided for backward compatibility.\n"
    ) +
    method_ext ("valign=", &set_valign, gsi::arg ("a"),
      "@brief Sets the vertical alignment\n"
      "\n"
      "This property specifies how the text is aligned relative to the anchor point. "
      "\n"
      "This property has been introduced in version 0.22 and extended to enums in 0.28.\n"
    ) +
    method_ext ("valign", &get_valign,
      "@brief Gets the vertical alignment\n"
      "\n"
      "See \\valign= for a description of this property.\n"
    ) +
    method_ext ("move", &move, gsi::arg ("distance"),
      "@brief Moves the text by a certain distance (modifies self)\n"
      "\n"
      "\n"
      "Moves the text by a given offset and returns the moved\n"
      "text. Does not check for coordinate overflows.\n"
      "\n"
      "@param p The offset to move the text.\n"
      "\n"
      "@return A reference to this text object\n"
    ) +
    method_ext ("move", &move_xy, gsi::arg ("dx"), gsi::arg ("dy"),
      "@brief Moves the text by a certain distance (modifies self)\n"
      "\n"
      "\n"
      "Moves the text by a given distance in x and y direction and returns the moved\n"
      "text. Does not check for coordinate overflows.\n"
      "\n"
      "@param dx The x distance to move the text.\n"
      "@param dy The y distance to move the text.\n"
      "\n"
      "@return A reference to this text object\n"
      "\n"
      "This method was introduced in version 0.23."
    ) +
    method_ext ("moved", &moved, gsi::arg ("distance"),
      "@brief Returns the text moved by a certain distance (does not modify self)\n"
      "\n"
      "\n"
      "Moves the text by a given offset and returns the moved\n"
      "text. Does not modify *this. Does not check for coordinate\n"
      "overflows.\n"
      "\n"
      "@param p The offset to move the text.\n"
      "\n"
      "@return The moved text.\n"
    ) +
    method_ext ("moved", &moved_xy, gsi::arg ("dx"), gsi::arg ("dy"),
      "@brief Returns the text moved by a certain distance (does not modify self)\n"
      "\n"
      "\n"
      "Moves the text by a given offset and returns the moved\n"
      "text. Does not modify *this. Does not check for coordinate\n"
      "overflows.\n"
      "\n"
      "@param dx The x distance to move the text.\n"
      "@param dy The y distance to move the text.\n"
      "\n"
      "@return The moved text.\n"
      "\n"
      "This method was introduced in version 0.23."
    ) +
    method ("transformed", &C::template transformed<simple_trans_type>, gsi::arg ("t"),
      "@brief Transforms the text with the given simple transformation\n"
      "\n"
      "\n"
      "@param t The transformation to apply\n"
      "@return The transformed text\n"
    ) +
    method ("transformed", &C::template transformed<complex_trans_type>, gsi::arg ("t"),
      "@brief Transforms the text with the given complex transformation\n"
      "\n"
      "\n"
      "@param t The magnifying transformation to apply\n"
      "@return The transformed text (a DText now)\n"
    ) +
    method ("<", &C::less, gsi::arg ("t"),
      "@brief Less operator\n"
      "@param t The object to compare against\n"
      "This operator is provided to establish some, not necessarily a certain sorting order"
    ) +
    method ("==", &C::equal, gsi::arg ("text"),
      "@brief Equality\n"
      "\n"
      "\n"
      "Return true, if this text object and the given text are equal "
    ) +
    method ("!=", &C::not_equal, gsi::arg ("text"),
      "@brief Inequality\n"
      "\n"
      "\n"
      "Return true, if this text object and the given text are not equal "
    ) +
    method_ext ("hash", &hash_value,
      "@brief Computes a hash value\n"
      "Returns a hash value for the given text object. This method enables texts as hash keys.\n"
      "\n"
      "This method has been introduced in version 0.25.\n"
    ) +
    constructor ("from_s", &from_string, gsi::arg ("s"),
      "@brief Creates an object from a string\n"
      "Creates the object from a string representation (as returned by \\to_s)\n"
      "\n"
      "This method has been added in version 0.23.\n"
    ) +
    method ("to_s", &C::to_string, gsi::arg ("dbu", 0.0),
      "@brief Converts the object to a string.\n"
      "If a DBU is given, the output units will be micrometers.\n"
      "\n"
      "The DBU argument has been added in version 0.27.6.\n"
    );
  }
};

static db::Text *text_from_dtext (const db::DText &t)
{
  return new db::Text (t);
}

static db::DText text_to_dtext (const db::Text *t, double dbu)
{
  return db::DText (*t * dbu);
}

Class<db::Text> decl_Text ("db", "Text",
  constructor ("new", &text_from_dtext, gsi::arg ("dtext"),
    "@brief Creates an integer coordinate text from a floating-point coordinate text"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_dtext'."
  ) +
  method_ext ("to_dtype", &text_to_dtext, gsi::arg ("dbu", 1.0),
    "@brief Converts the text to a floating-point coordinate text"
    "\n"
    "The database unit can be specified to translate the integer-coordinate text into a floating-point coordinate "
    "text in micron units. The database unit is basically a scaling factor.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("transformed", &db::Text::transformed<db::ICplxTrans>, gsi::arg ("t"),
    "@brief Transform the text with the given complex transformation\n"
    "\n"
    "\n"
    "@param t The magnifying transformation to apply\n"
    "@return The transformed text (in this case an integer coordinate object now)\n"
    "\n"
    "This method has been introduced in version 0.18.\n"
  ) +
  text_defs<db::Text>::methods (),
  "@brief A text object\n"
  "\n"
  "A text object has a point (location), a text, a text transformation,\n"
  "a text size and a font id. Text size and font id are provided to be\n"
  "be able to render the text correctly.\n"
  "Text objects are used as labels (i.e. for pins) or to indicate a particular position.\n"
  "\n"
  "The \\Text class uses integer coordinates. A class that operates with floating-point coordinates "
  "is \\DText.\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

static db::DText *dtext_from_itext (const db::Text &t)
{
  return new db::DText (t);
}

static db::Text dtext_to_text (const db::DText *t, double dbu)
{
  return db::Text (*t * (1.0 / dbu));
}

Class<db::DText> decl_DText ("db", "DText",
  constructor ("new", &dtext_from_itext, gsi::arg ("Text"),
    "@brief Creates a floating-point coordinate text from an integer coordinate text\n"
    "\n"
    "This constructor has been introduced in version 0.25 and replaces the previous static method 'from_itext'."
  ) +
  method_ext ("to_itype", &dtext_to_text, gsi::arg ("dbu", 1.0),
    "@brief Converts the text to an integer coordinate text\n"
    "\n"
    "The database unit can be specified to translate the floating-point coordinate "
    "Text in micron units to an integer-coordinate text in database units. The text's "
    "coordinates will be divided by the database unit.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("transformed", &db::DText::transformed<db::VCplxTrans>, gsi::arg ("t"),
    "@brief Transforms the text with the given complex transformation\n"
    "\n"
    "\n"
    "@param t The magnifying transformation to apply\n"
    "@return The transformed text (in this case an integer coordinate text)\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  text_defs<db::DText>::methods (),
  "@brief A text object\n"
  "\n"
  "A text object has a point (location), a text, a text transformation,\n"
  "a text size and a font id. Text size and font id are provided to be\n"
  "be able to render the text correctly.\n"
  "Text objects are used as labels (i.e. for pins) or to indicate a particular position.\n"
  "\n"
  "The \\DText class uses floating-point coordinates. A class that operates with integer coordinates "
  "is \\Text.\n"
  "\n"
  "See @<a href=\"/programming/database_api.xml\">The Database API@</a> for more details about the "
  "database objects."
);

gsi::Enum<db::HAlign> decl_HAlign ("db", "HAlign",
  gsi::enum_const ("HAlignLeft", db::HAlignLeft,
    "@brief Left horizontal alignment\n"
  ) +
  gsi::enum_const ("HAlignCenter", db::HAlignCenter,
    "@brief Centered horizontal alignment\n"
  ) +
  gsi::enum_const ("HAlignRight", db::HAlignRight,
    "@brief Right horizontal alignment\n"
  ) +
  gsi::enum_const ("NoHAlign", db::NoHAlign,
    "@brief Undefined horizontal alignment\n"
  ),
  "@brief This class represents the horizontal alignment modes.\n"
  "This enum has been introduced in version 0.28."
);

gsi::Enum<db::VAlign> decl_VAlign ("db", "VAlign",
  gsi::enum_const ("VAlignBottom", db::VAlignBottom,
    "@brief Bottom vertical alignment\n"
  ) +
  gsi::enum_const ("VAlignCenter", db::VAlignCenter,
    "@brief Centered vertical alignment\n"
  ) +
  gsi::enum_const ("VAlignTop", db::VAlignTop,
    "@brief Top vertical alignment\n"
  ) +
  gsi::enum_const ("NoVAlign", db::NoVAlign,
    "@brief Undefined vertical alignment\n"
  ),
  "@brief This class represents the vertical alignment modes.\n"
  "This enum has been introduced in version 0.28."
);

//  Inject the alignment enums
gsi::ClassExt<db::Text> inject_Text_HAlign_in_parent (decl_HAlign.defs ());
gsi::ClassExt<db::DText> inject_DText_HAlign_in_parent (decl_HAlign.defs ());
gsi::ClassExt<db::Text> inject_Text_VAlign_in_parent (decl_VAlign.defs ());
gsi::ClassExt<db::DText> inject_DText_VAlign_in_parent (decl_VAlign.defs ());

}
