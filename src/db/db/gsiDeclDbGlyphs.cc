
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

#include "dbGlyphs.h"

namespace gsi
{

// -------------------------------------------------------------------
//  db::TextGenerator declarations

static double dwidth (const db::TextGenerator *gen)
{
  return gen->width () * gen->dbu ();
}

static double dheight (const db::TextGenerator *gen)
{
  return gen->height () * gen->dbu ();
}

static double dline_width (const db::TextGenerator *gen)
{
  return gen->line_width () * gen->dbu ();
}

static double ddesign_grid (const db::TextGenerator *gen)
{
  return gen->design_grid () * gen->dbu ();
}

static db::DBox dbackground (const db::TextGenerator *gen)
{
  return db::DBox (gen->background ()) * gen->dbu ();
}

static std::vector<const db::TextGenerator *> generators ()
{
  std::vector<const db::TextGenerator *> gg;
  for (std::vector<db::TextGenerator>::const_iterator g = db::TextGenerator::generators ().begin (); g != db::TextGenerator::generators ().end (); ++g) {
    gg.push_back (g.operator-> ());
  }
  return gg;
}

Class<db::TextGenerator> decl_TextGenerator ("db", "TextGenerator",
  method ("load_from_resource", &db::TextGenerator::load_from_resource, arg ("resource_path"),
    "@brief Loads the given resource data (as layout data) into the generator\n"
    "The resource path has to start with a colon, i.e. ':/my/resource.gds'. "
    "See the description of the class how the layout data is read."
  ) +
  method ("load_from_file", &db::TextGenerator::load_from_file, arg ("path"),
    "@brief Loads the given file into the generator\n"
    "See the description of the class how the layout data is read."
  ) +
  method ("description", &db::TextGenerator::description,
    "@brief Gets the description text of the generator\n"
    "The generator's description text is a human-readable text that is used to identify the generator (aka 'font') in "
    "user interfaces."
  ) +
  method ("name", &db::TextGenerator::name,
    "@brief Gets the name of the generator\n"
    "The generator's name is the basic key by which the generator is identified."
  ) +
  method ("text", &db::TextGenerator::text_as_region,
    arg ("text"),
    arg ("target_dbu"),
    arg ("mag", 1.0),
    arg ("inv", false),
    arg ("bias", 0.0),
    arg ("char_spacing", 0.0),
    arg ("line_spacing", 0.0),
    "@brief Gets the rendered text as a region\n"
    "@param text The text string\n"
    "@param target_dbu The database unit for which to produce the text\n"
    "@param mag The magnification (1.0 for original size)\n"
    "@param inv inverted rendering: if true, the glyphs are rendered inverse with the background box as the outer bounding box\n"
    "@param bias An additional bias to be applied (happens before inversion, can be negative)\n"
    "@param char_spacing Additional space between characters (in micron units)\n"
    "@param line_spacing Additional space between lines (in micron units)\n"
    "Various options can be specified to control the appearance of the text. See the description of the parameters. "
    "It's important to specify the target database unit in \\target_dbu to indicate what database unit shall be used to create the "
    "output for."
  ) +
  method ("glyph", &db::TextGenerator::glyph_as_region, arg ("char"),
    "@brief Gets the glyph of the given character as a region\n"
    "The region represents the glyph's outline and is delivered in the generator's database units ."
    "A more elaborate way to getting the text's outline is \\text."
  ) +
  method_ext ("dline_width", &dline_width,
    "@brief Gets the line width of the glyphs in micron units\n"
    "The line width is the intended (not necessarily precisely) line width of typical "
    "character lines (such as the bar of an 'I')."
  ) +
  method ("line_width", &db::TextGenerator::line_width,
    "@brief Gets the line width of the glyphs in the generator's database units\n"
    "The line width is the intended (not necessarily precisely) line width of typical "
    "character lines (such as the bar of an 'I'). "
    "A version that delivers this value in micrometer units is \\dline_width."
  ) +
  method_ext ("ddesign_grid", &ddesign_grid,
    "@brief Gets the design grid of the glyphs in micron units\n"
    "The design grid is the basic grid used when designing the glyphs. In most cases "
    "this grid is bigger than the database unit. "
  ) +
  method ("design_grid", &db::TextGenerator::design_grid,
    "@brief Gets the design grid of the glyphs in the generator's database units\n"
    "The design grid is the basic grid used when designing the glyphs. In most cases "
    "this grid is bigger than the database unit. "
    "A version that delivers this value in micrometer units is \\ddesign_grid."
  ) +
  method_ext ("dwidth", &dwidth,
    "@brief Gets the design width of the glyphs in micron units\n"
    "The width is the width of the rectangle occupied by each character. "
  ) +
  method ("width", &db::TextGenerator::width,
    "@brief Gets the design height of the glyphs in the generator's database units\n"
    "The width is the width of the rectangle occupied by each character. "
    "A version that delivers this value in micrometer units is \\dwidth."
  ) +
  method_ext ("dheight", &dheight,
    "@brief Gets the design height of the glyphs in micron units\n"
    "The height is the height of the rectangle occupied by each character. "
  ) +
  method ("height", &db::TextGenerator::height,
    "@brief Gets the design height of the glyphs in the generator's database units\n"
    "The height is the height of the rectangle occupied by each character. "
    "A version that delivers this value in micrometer units is \\dheight."
  ) +
  method_ext ("dbackground", &dbackground,
    "@brief Gets the background rectangle in micron units\n"
    "The background rectangle is the one that is used as background for inverted rendering."
  ) +
  method ("background", &db::TextGenerator::background,
    "@brief Gets the background rectangle of each glyph in the generator's database units\n"
    "The background rectangle is the one that is used as background for inverted rendering. "
    "A version that delivers this value in micrometer units is \\dbackground."
  ) +
  method ("dbu", &db::TextGenerator::dbu,
    "@brief Gets the basic database unit the design of the glyphs was made\n"
    "This database unit the basic resolution of the glyphs."
  ) +
  method ("generators", &generators,
    "@brief Gets the generators registered in the system\n"
    "This method delivers a list of generator objects that can be used to create texts."
  ) +
  method ("generator_by_name", &db::TextGenerator::generator_by_name, arg ("name"),
    "@brief Gets the text generator for a given name\n"
    "This method delivers the generator with the given name or nil if no such generator is registered."
  ) +
  method ("default_generator", &db::TextGenerator::default_generator,
    "@brief Gets the default text generator (a standard font)\n"
    "This method delivers the default generator or nil if no such generator is installed."
  ) +
  method ("set_font_paths", &db::TextGenerator::set_font_paths,
    "@brief Sets the paths where to look for font files\n"
    "This function sets the paths where to look for font files. After setting such a path, each font found will render a "
    "specific generator. The generator can be found under the font file's name. As the text generator is also the basis "
    "for the Basic.TEXT PCell, using this function also allows configuring custom fonts for this library cell.\n"
    "\n"
    "This method has been introduced in version 0.27.4."
  ) +
  method ("font_paths", &db::TextGenerator::font_paths,
    "@brief Gets the paths where to look for font files\n"
    "See \\set_font_paths for a description of this function.\n"
    "\n"
    "This method has been introduced in version 0.27.4."
  ),
  "@brief A text generator class\n"
  "\n"
  "A text generator is basically a way to produce human-readable text for labelling layouts. It's "
  "similar to the Basic.TEXT PCell, but more convenient to use in a scripting context.\n"
  "\n"
  "Generators can be constructed from font files (or resources) or one of the registered generators "
  "can be used.\n"
  "\n"
  "To create a generator from a font file proceed this way:\n"
  "@code\n"
  "gen = RBA::TextGenerator::new\n"
  "gen.load_from_file(\"myfont.gds\")\n"
  "region = gen.text(\"A TEXT\", 0.001)\n"
  "@/code\n"
  "\n"
  "This code produces a RBA::Region with a database unit of 0.001 micron. This region can be fed "
  "into a \\Shapes container to place it into a cell for example.\n"
  "\n"
  "By convention the font files must have two to three layers:\n"
  "\n"
  "@ul\n"
  "@li 1/0 for the actual data @/li\n"
  "@li 2/0 for the borders @/li\n"
  "@li 3/0 for an optional additional background @/li\n"
  "@/ul\n"
  "\n"
  "Currently, all glyphs must be bottom-left aligned at 0, 0. The\n"
  "border must be drawn in at least one glyph cell. The border is taken\n"
  "as the overall bbox of all borders.\n"
  "\n"
  "The glyph cells must be named with a single character or \"nnn\" where \"d\" is the\n"
  "ASCII code of the character (i.e. \"032\" for space). Allowed ASCII codes are 32 through 127.\n"
  "If a lower-case \"a\" character is defined, lower-case letters are supported.\n"
  "Otherwise, lowercase letters are mapped to uppercase letters.\n"
  "\n"
  "Undefined characters are left blank in the output.\n"
  "\n"
  "A comment cell can be defined (\"COMMENT\") which must hold one text in layer 1\n"
  "stating the comment, and additional descriptions such as line width:\n"
  "\n"
  "@ul\n"
  "@li \"line_width=<x>\": Specifies the intended line width in micron units @/li\n"
  "@li \"design_grid=<x>\": Specifies the intended design grid in micron units @/li\n"
  "@li any other text: The description string @/li\n"
  "@/ul\n"
  "\n"
  "Generators can be picked form a list of predefined generator. See \\generators, \\default_generator and "
  "\\generator_by_name for picking a generator from the list.\n"
  "\n"
  "This class has been introduced in version 0.25."
);

} // namespace gsi
