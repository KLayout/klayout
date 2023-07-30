
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
#include "layLayerProperties.h"
#include "layLayoutViewBase.h"

namespace gsi
{

static const std::vector<db::DCplxTrans> &get_trans (const lay::LayerProperties *n, bool real)
{
  return n->source (real).trans ();
}

static const std::vector<db::DCplxTrans> &get_trans_1 (const lay::LayerProperties *n)
{
  return n->source (true).trans ();
}

static void set_trans (lay::LayerProperties *n, const std::vector<db::DCplxTrans> &trans)
{
  lay::ParsedLayerSource s (n->source (false));
  s.set_trans (trans);
  n->set_source (s);
}

static int get_cellview (const lay::LayerProperties *n, bool real)
{
  return n->source (real).cv_index ();
}

static int get_cellview_1 (const lay::LayerProperties *n)
{
  return n->source (true).cv_index ();
}

static void set_cellview (lay::LayerProperties *n, int index)
{
  lay::ParsedLayerSource s (n->source (false));
  s.cv_index (index);
  n->set_source (s);
}

static int get_layer_index (const lay::LayerProperties *n, bool real)
{
  return n->source (real).layer_index ();
}

static int get_layer_index_1 (const lay::LayerProperties *n)
{
  return n->source (true).layer_index ();
}

static void set_layer_index (lay::LayerProperties *n, int index)
{
  lay::ParsedLayerSource s (n->source (false));
  s.layer_index (index);
  n->set_source (s);
}

static int get_layer (const lay::LayerProperties *n, bool real)
{
  return n->source (real).layer ();
}

static int get_layer_1 (const lay::LayerProperties *n)
{
  return n->source (true).layer ();
}

static void set_layer (lay::LayerProperties *n, int index)
{
  lay::ParsedLayerSource s (n->source (false));
  s.layer (index);
  n->set_source (s);
}

static int get_datatype (const lay::LayerProperties *n, bool real)
{
  return n->source (real).datatype ();
}

static int get_datatype_1 (const lay::LayerProperties *n)
{
  return n->source (true).datatype ();
}

static void set_datatype (lay::LayerProperties *n, int index)
{
  lay::ParsedLayerSource s (n->source (false));
  s.datatype (index);
  n->set_source (s);
}

static std::string get_name (const lay::LayerProperties *n, bool real)
{
  return n->source (real).name ();
}

static std::string get_name_1 (const lay::LayerProperties *n)
{
  return n->source (true).name ();
}

static bool has_name (const lay::LayerProperties *n, bool real)
{
  return n->source (real).has_name ();
}

static bool has_name_1 (const lay::LayerProperties *n)
{
  return n->source (true).has_name ();
}

static void set_name (lay::LayerProperties *n, const std::string &name)
{
  lay::ParsedLayerSource s (n->source (false));
  s.name (name);
  n->set_source (s);
}

static void clear_name (lay::LayerProperties *n)
{
  lay::ParsedLayerSource s (n->source (false));
  s.clear_name ();
  n->set_source (s);
}

static int get_upper_hier_level (const lay::LayerProperties *n, bool real)
{
  return n->source (real).hier_levels ().to_level ();
}

static int get_upper_hier_level_1 (const lay::LayerProperties *n)
{
  return n->source (true).hier_levels ().to_level ();
}

static bool get_upper_hier_level_relative (const lay::LayerProperties *n, bool real)
{
  return n->source (real).hier_levels ().to_level_relative ();
}

static bool get_upper_hier_level_relative_1 (const lay::LayerProperties *n)
{
  return n->source (true).hier_levels ().to_level_relative ();
}

static int get_upper_hier_level_mode (const lay::LayerProperties *n, bool real)
{
  return int (n->source (real).hier_levels ().to_level_mode ());
}

static int get_upper_hier_level_mode_1 (const lay::LayerProperties *n)
{
  return int (n->source (true).hier_levels ().to_level_mode ());
}

static void set_upper_hier_level1 (lay::LayerProperties *n, int level)
{
  lay::ParsedLayerSource s (n->source (false));
  lay::HierarchyLevelSelection h (s.hier_levels ());
  h.set_to_level (level, false, lay::HierarchyLevelSelection::absolute);
  s.set_hier_levels (h);
  n->set_source (s);
}

static void set_upper_hier_level2 (lay::LayerProperties *n, int level, bool relative)
{
  lay::ParsedLayerSource s (n->source (false));
  lay::HierarchyLevelSelection h (s.hier_levels ());
  h.set_to_level (level, relative, lay::HierarchyLevelSelection::absolute);
  s.set_hier_levels (h);
  n->set_source (s);
}

static void set_upper_hier_level3 (lay::LayerProperties *n, int level, bool relative, int mode)
{
  lay::ParsedLayerSource s (n->source (false));
  lay::HierarchyLevelSelection h (s.hier_levels ());
  h.set_to_level (level, relative, lay::HierarchyLevelSelection::level_mode_type (mode));
  s.set_hier_levels (h);
  n->set_source (s);
}

static bool get_has_upper_hier_level (const lay::LayerProperties *n, bool real)
{
  return n->source (real).hier_levels ().has_to_level ();
}

static bool get_has_upper_hier_level_1 (const lay::LayerProperties *n)
{
  return n->source (true).hier_levels ().has_to_level ();
}

static void clear_upper_hier_level (lay::LayerProperties *n)
{
  lay::ParsedLayerSource s (n->source (false));
  lay::HierarchyLevelSelection h (s.hier_levels ());
  h.clear_to_level ();
  s.set_hier_levels (h);
  n->set_source (s);
}

static int get_lower_hier_level (const lay::LayerProperties *n, bool real)
{
  return n->source (real).hier_levels ().from_level ();
}

static int get_lower_hier_level_1 (const lay::LayerProperties *n)
{
  return n->source (true).hier_levels ().from_level ();
}

static bool get_lower_hier_level_relative (const lay::LayerProperties *n, bool real)
{
  return n->source (real).hier_levels ().from_level_relative ();
}

static bool get_lower_hier_level_relative_1 (const lay::LayerProperties *n)
{
  return n->source (true).hier_levels ().from_level_relative ();
}

static int get_lower_hier_level_mode (const lay::LayerProperties *n, bool real)
{
  return int (n->source (real).hier_levels ().from_level_mode ());
}

static int get_lower_hier_level_mode_1 (const lay::LayerProperties *n)
{
  return int (n->source (true).hier_levels ().from_level_mode ());
}

static void set_lower_hier_level1 (lay::LayerProperties *n, int level)
{
  lay::ParsedLayerSource s (n->source (false));
  lay::HierarchyLevelSelection h (s.hier_levels ());
  h.set_from_level (level, false, lay::HierarchyLevelSelection::absolute);
  s.set_hier_levels (h);
  n->set_source (s);
}

static void set_lower_hier_level2 (lay::LayerProperties *n, int level, bool relative)
{
  lay::ParsedLayerSource s (n->source (false));
  lay::HierarchyLevelSelection h (s.hier_levels ());
  h.set_from_level (level, relative, lay::HierarchyLevelSelection::absolute);
  s.set_hier_levels (h);
  n->set_source (s);
}

static void set_lower_hier_level3 (lay::LayerProperties *n, int level, bool relative, int mode)
{
  lay::ParsedLayerSource s (n->source (false));
  lay::HierarchyLevelSelection h (s.hier_levels ());
  h.set_from_level (level, relative, lay::HierarchyLevelSelection::level_mode_type (mode));
  s.set_hier_levels (h);
  n->set_source (s);
}

static bool get_has_lower_hier_level (const lay::LayerProperties *n, bool real)
{
  return n->source (real).hier_levels ().has_from_level ();
}

static bool get_has_lower_hier_level_1 (const lay::LayerProperties *n)
{
  return n->source (true).hier_levels ().has_from_level ();
}

static void clear_lower_hier_level (lay::LayerProperties *n)
{
  lay::ParsedLayerSource s (n->source (false));
  lay::HierarchyLevelSelection h (s.hier_levels ());
  h.clear_from_level ();
  s.set_hier_levels (h);
  n->set_source (s);
}

static tl::color_t get_eff_frame_color_1 (const lay::LayerProperties *n)
{
  return n->eff_frame_color (true);
}

static tl::color_t get_eff_fill_color_1 (const lay::LayerProperties *n)
{
  return n->eff_fill_color (true);
}

static tl::color_t get_frame_color_1 (const lay::LayerProperties *n)
{
  return n->frame_color (true);
}

static tl::color_t get_fill_color_1 (const lay::LayerProperties *n)
{
  return n->fill_color (true);
}

static bool get_has_frame_color_1 (const lay::LayerProperties *n)
{
  return n->has_frame_color (true);
}

static bool get_has_fill_color_1 (const lay::LayerProperties *n)
{
  return n->has_fill_color (true);
}

static int get_frame_brightness_1 (const lay::LayerProperties *n)
{
  return n->frame_brightness (true);
}

static int get_fill_brightness_1 (const lay::LayerProperties *n)
{
  return n->fill_brightness (true);
}

static unsigned int get_eff_dither_pattern_1 (const lay::LayerProperties *n)
{
  return n->eff_dither_pattern (true);
}

static int get_dither_pattern_1 (const lay::LayerProperties *n)
{
  return n->dither_pattern (true);
}

static bool get_has_dither_pattern_1 (const lay::LayerProperties *n)
{
  return n->has_dither_pattern (true);
}

static unsigned int get_eff_line_style_1 (const lay::LayerProperties *n)
{
  return n->eff_line_style (true);
}

static int get_line_style_1 (const lay::LayerProperties *n)
{
  return n->line_style (true);
}

static bool get_has_line_style_1 (const lay::LayerProperties *n)
{
  return n->has_line_style (true);
}

static bool get_valid_1 (const lay::LayerProperties *n)
{
  return n->valid (true);
}

static bool get_visible_1 (const lay::LayerProperties *n)
{
  return n->visible (true);
}

static bool get_marked_1 (const lay::LayerProperties *n)
{
  return n->marked (true);
}

static bool get_xfill_1 (const lay::LayerProperties *n)
{
  return n->xfill (true);
}

static int get_width_1 (const lay::LayerProperties *n)
{
  return n->width (true);
}

static int get_animation_1 (const lay::LayerProperties *n)
{
  return n->animation (true);
}

static bool get_transparent_1 (const lay::LayerProperties *n)
{
  return n->transparent (true);
}

static std::string source_string_1 (const lay::LayerProperties *n)
{
  return n->source_string (true);
}

Class<lay::LayerProperties> decl_LayerProperties ("lay", "LayerProperties",
  method ("==", &lay::LayerProperties::operator==, gsi::arg ("other"),
    "@brief Equality \n"
    "\n"
    "@param other The other object to compare against"
  ) +
  method ("!=", &lay::LayerProperties::operator!=, gsi::arg ("other"),
    "@brief Inequality \n"
    "\n"
    "@param other The other object to compare against"
  ) +
  method ("flat", &lay::LayerProperties::flat, 
    "@brief Returns the \"flattened\" (effective) layer properties entry for this node\n"
    "\n"
    "This method returns a \\LayerProperties object that is not embedded into a hierarchy.\n"
    "This object represents the effective layer properties for the given node. In particular, "
    "all 'local' properties are identical to the 'real' properties. Such an object can be "
    "used as a basis for manipulations."
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +
  method ("eff_frame_color", &lay::LayerProperties::eff_frame_color, gsi::arg ("real"),
    "@brief Gets the effective frame color \n"
    "\n"
    "The effective frame color is computed from the frame color brightness and the\n"
    "frame color.\n"
    "\n"
    "@param real Set to true to return the real instead of local value"
  ) +
  method_ext ("eff_frame_color", &get_eff_frame_color_1, 
    "@brief Gets the effective frame color\n"
    "\n"
    "This method is a convenience method for \"eff_frame_color(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("eff_fill_color", &lay::LayerProperties::eff_fill_color, gsi::arg ("real"),
    "@brief Gets the effective fill color\n"
    "\n"
    "The effective fill color is computed from the frame color brightness and the\n"
    "frame color.\n"
    "\n"
    "@param real Set to true to return the real instead of local value"
  ) +
  method_ext ("eff_fill_color", &get_eff_fill_color_1, 
    "@brief Gets the effective fill color\n"
    "\n"
    "This method is a convenience method for \"eff_fill_color(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("frame_color", &lay::LayerProperties::frame_color, gsi::arg ("real"),
    "@brief Gets the frame color\n"
    "\n"
    "This method may return an invalid color if the color is not set.\n"
    "\n"
    "@param real Set to true to return the real instead of local value"
  ) +
  method_ext ("frame_color", &get_frame_color_1, 
    "@brief Gets the frame color\n"
    "\n"
    "This method is a convenience method for \"frame_color(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("frame_color=", &lay::LayerProperties::set_frame_color, gsi::arg ("color"),
    "@brief Sets the frame color to the given value\n"
    "\n"
    "The color is a 32bit value encoding the blue value in the lower 8 bits, "
    "the green value in the next 8 bits and the red value in the 8 bits above that."
  ) +
  method ("clear_frame_color", &lay::LayerProperties::clear_frame_color, 
    "@brief Resets the frame color \n"
  ) +
  method ("has_frame_color?", &lay::LayerProperties::has_frame_color, gsi::arg ("real"),
    "@brief True, if the frame color is set\n"
  ) +
  method_ext ("has_frame_color?", &get_has_frame_color_1, 
    "@brief True, if the frame color is set\n"
    "\n"
    "This method is a convenience method for \"has_frame_color?(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("fill_color", &lay::LayerProperties::fill_color, gsi::arg ("real"),
    "@brief Gets the fill color\n"
    "\n"
    "This method may return an invalid color if the color is not set.\n"
    "\n"
    "@param real Set to true to return the real instead of local value"
  ) +
  method_ext ("fill_color", &get_fill_color_1, 
    "@brief Gets the fill color\n"
    "\n"
    "This method is a convenience method for \"fill_color(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("fill_color=", &lay::LayerProperties::set_fill_color, gsi::arg ("color"),
    "@brief Sets the fill color to the given value\n"
    "\n"
    "The color is a 32bit value encoding the blue value in the lower 8 bits, "
    "the green value in the next 8 bits and the red value in the 8 bits above that."
  ) +
  method ("clear_fill_color", &lay::LayerProperties::clear_fill_color, 
    "@brief Resets the fill color\n"
  ) +
  method ("has_fill_color?", &lay::LayerProperties::has_fill_color, gsi::arg ("real"),
    "@brief True, if the fill color is set\n"
  ) +
  method_ext ("has_fill_color?", &get_has_fill_color_1, 
    "@brief True, if the fill color is set\n"
    "\n"
    "This method is a convenience method for \"has_fill_color?(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("frame_brightness=", &lay::LayerProperties::set_frame_brightness, gsi::arg ("brightness"),
    "@brief Sets the frame brightness\n"
    "\n"
    "For neutral brightness set this value to 0. For darker colors set it to a negative "
    "value (down to -255), for brighter colors to a positive value (up to 255)\n"
  ) +
  method ("frame_brightness", &lay::LayerProperties::frame_brightness, gsi::arg ("real"),
    "@brief Gets the frame brightness value\n"
    "\n"
    "If the brightness is not set, this method may return an invalid value\n"
    "\n"
    "@param real Set to true to return the real instead of local value"
  ) +
  method_ext ("frame_brightness", &get_frame_brightness_1, 
    "@brief Gets the frame brightness value\n"
    "\n"
    "This method is a convenience method for \"frame_brightness(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("fill_brightness=", &lay::LayerProperties::set_fill_brightness, gsi::arg ("brightness"),
    "@brief Sets the fill brightness\n"
    "\n"
    "For neutral brightness set this value to 0. For darker colors set it to a negative "
    "value (down to -255), for brighter colors to a positive value (up to 255)\n"
  ) +
  method ("fill_brightness", &lay::LayerProperties::fill_brightness, gsi::arg ("real"),
    "@brief Gets the fill brightness value\n"
    "\n"
    "If the brightness is not set, this method may return an invalid value\n"
    "\n"
    "@param real Set to true to return the real instead of local value"
  ) +
  method_ext ("fill_brightness", &get_fill_brightness_1, 
    "@brief Gets the fill brightness value\n"
    "\n"
    "This method is a convenience method for \"fill_brightness(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("dither_pattern=", &lay::LayerProperties::set_dither_pattern, gsi::arg ("index"),
    "@brief Sets the dither pattern index\n"
    "\n"
    "The dither pattern index must be one of the valid indices.\n"
    "The first indices are reserved for built-in pattern, the following ones are custom pattern.\n"
    "Index 0 is always solid filled and 1 is always the hollow filled pattern.\n"
    "For custom pattern see \\LayoutView#add_stipple.\n"
  ) +
  method ("eff_dither_pattern", &lay::LayerProperties::eff_dither_pattern, gsi::arg ("real"),
    "@brief Gets the effective dither pattern index\n"
    "\n"
    "The effective dither pattern index is always a valid index, even if no dither pattern "
    "is set."
    "\n"
    "@param real Set to true to return the real instead of local value"
  ) +
  method_ext ("eff_dither_pattern", &get_eff_dither_pattern_1, 
    "@brief Gets the effective dither pattern index\n"
    "\n"
    "This method is a convenience method for \"eff_dither_pattern(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("dither_pattern", &lay::LayerProperties::dither_pattern, gsi::arg ("real"),
    "@brief Gets the dither pattern index\n"
    "\n"
    "This method may deliver an invalid dither pattern index if it is not set.\n"
    "\n"
    "@param real Set to true to return the real instead of local value"
  ) +
  method_ext ("dither_pattern", &get_dither_pattern_1, 
    "@brief Gets the dither pattern index\n"
    "\n"
    "This method is a convenience method for \"dither_pattern(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("clear_dither_pattern", &lay::LayerProperties::clear_dither_pattern, 
    "@brief Clears the dither pattern\n"
  ) +
  method ("has_dither_pattern?", &lay::LayerProperties::has_dither_pattern, gsi::arg ("real"),
    "@brief True, if the dither pattern is set\n"
  ) +
  method_ext ("has_dither_pattern?", &get_has_dither_pattern_1, 
    "@brief True, if the dither pattern is set\n"
    "\n"
    "This method is a convenience method for \"has_dither_pattern?(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("line_style=", &lay::LayerProperties::set_line_style, gsi::arg ("index"),
    "@brief Sets the line style index\n"
    "\n"
    "The line style index must be one of the valid indices.\n"
    "The first indices are reserved for built-in pattern, the following ones are custom pattern.\n"
    "Index 0 is always solid filled.\n"
    "For custom line styles see \\LayoutView#add_line_style.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("eff_line_style", &lay::LayerProperties::eff_line_style,  gsi::arg ("real"),
    "@brief Gets the effective line style index\n"
    "\n"
    "The effective line style index is always a valid index, even if no line style "
    "is set. In that case, a default style index will be returned.\n"
    "\n"
    "@param real Set to true to return the real instead of local value\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("eff_line_style", &get_eff_line_style_1,
    "@brief Gets the line style index\n"
    "\n"
    "This method is a convenience method for \"eff_line_style(true)\"\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("line_style", &lay::LayerProperties::line_style, gsi::arg ("real"),
    "@brief Gets the line style index\n"
    "\n"
    "This method may deliver an invalid line style index if it is not set (see \\has_line_style?).\n"
    "\n"
    "@param real Set to true to return the real instead of local value"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("line_style", &get_line_style_1,
    "@brief Gets the line style index\n"
    "\n"
    "This method is a convenience method for \"line_style(true)\"\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("clear_line_style", &lay::LayerProperties::clear_line_style,
    "@brief Clears the line style\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("has_line_style?", &lay::LayerProperties::has_line_style, gsi::arg ("real"),
    "@brief Gets a value indicating whether the line style is set\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("has_line_style?", &get_has_line_style_1,
    "@brief True, if the line style is set\n"
    "\n"
    "This method is a convenience method for \"has_line_style?(true)\"\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("valid=", &lay::LayerProperties::set_valid, gsi::arg ("valid"),
    "@brief Sets the validity state\n"
  ) +
  method ("valid?", &lay::LayerProperties::valid, gsi::arg ("real"),
    "@brief Gets the validity state\n"
  ) +
  method_ext ("valid?", &get_valid_1, 
    "@brief Gets the validity state\n"
    "\n"
    "This method is a convenience method for \"valid?(true)\"\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  method ("visible=", &lay::LayerProperties::set_visible, gsi::arg ("visible"),
    "@brief Sets the visibility state\n"
  ) +
  method ("visible?", &lay::LayerProperties::visible, gsi::arg ("real"),
    "@brief Gets the visibility state\n"
  ) +
  method_ext ("visible?", &get_visible_1, 
    "@brief Gets the visibility state\n"
    "\n"
    "This method is a convenience method for \"visible?(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("transparent=", &lay::LayerProperties::set_transparent, gsi::arg ("transparent"),
    "@brief Sets the transparency state\n"
  ) +
  method ("transparent?", &lay::LayerProperties::transparent, gsi::arg ("real"),
    "@brief Gets the transparency state\n"
  ) +
  method_ext ("transparent?", &get_transparent_1, 
    "@brief Gets the transparency state\n"
    "\n"
    "This method is a convenience method for \"transparent?(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("width=", &lay::LayerProperties::set_width, gsi::arg ("width"),
    "@brief Sets the line width to the given width\n"
  ) +
  method ("width", &lay::LayerProperties::width, gsi::arg ("real"),
    "@brief Gets the line width\n"
  ) +
  method_ext ("width", &get_width_1, 
    "@brief Gets the line width\n"
    "\n"
    "This method is a convenience method for \"width(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("marked=", &lay::LayerProperties::set_marked, gsi::arg ("marked"),
    "@brief Sets the marked state\n"
  ) +
  method ("marked?", &lay::LayerProperties::marked, gsi::arg ("real"),
    "@brief Gets the marked state\n"
  ) +
  method_ext ("marked?", &get_marked_1,
    "@brief Gets the marked state\n"
    "\n"
    "This method is a convenience method for \"marked?(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("xfill=", &lay::LayerProperties::set_xfill, gsi::arg ("xfill"),
    "@brief Sets a value indicating whether shapes are drawn with a cross\n"
    "\n"
    "This attribute has been introduced in version 0.25.\n"
  ) +
  method ("xfill?", &lay::LayerProperties::xfill, gsi::arg ("real"),
    "@brief Gets a value indicating whether shapes are drawn with a cross\n"
    "\n"
    "This attribute has been introduced in version 0.25.\n"
  ) +
  method_ext ("xfill?", &get_xfill_1,
    "@brief Gets a value indicating whether shapes are drawn with a cross\n"
    "\n"
    "This method is a convenience method for \"xfill?(true)\"\n"
    "\n"
    "This attribute has been introduced in version 0.25.\n"
  ) +
  method ("animation=", &lay::LayerProperties::set_animation, gsi::arg ("animation"),
    "@brief Sets the animation state\n"
    "\n"
    "See the description of the \\animation method for details about the animation state"
  ) +
  method ("animation", &lay::LayerProperties::animation, gsi::arg ("real"),
    "@brief Gets the animation state\n"
    "\n"
    "The animation state is an integer either being 0 (static), 1 (scrolling), 2 (blinking) "
    "or 3 (inversely blinking)"
  ) +
  method_ext ("animation", &get_animation_1, 
    "@brief Gets the animation state\n"
    "\n"
    "This method is a convenience method for \"animation(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("name=", &lay::LayerProperties::set_name, gsi::arg ("name"),
    "@brief Sets the name to the given string\n"
  ) +
  method ("name", &lay::LayerProperties::name, 
    "@brief Gets the name\n"
  ) +
  method_ext ("trans", &get_trans, gsi::arg ("real"),
    "@brief Gets the transformations that the layer is transformed with\n"
    "\n"
    "The transformations returned by this accessor is the one used for displaying this layer. "
    "The layout is transformed with each of these transformations before it is drawn.\n\n"
    "If \"real\" is true, the effective value is returned."
  ) +
  method_ext ("trans", &get_trans_1, 
    "@brief Gets the transformations that the layer is transformed with\n"
    "\n"
    "This method is a convenience method for \"trans(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("trans=", &set_trans, gsi::arg ("trans_vector"),
    "@brief Sets the transformations that the layer is transformed with\n"
    "\n"
    "See \\trans for a description of the transformations."
  ) +
  method_ext ("source_cellview", &get_cellview, gsi::arg ("real"),
    "@brief Gets the cellview index that this layer refers to\n"
    "\n"
    "If \"real\" is true, the effective value is returned."
  ) +
  method_ext ("source_cellview", &get_cellview_1, 
    "@brief Gets the cellview index that this layer refers to\n"
    "\n"
    "This method is a convenience method for \"source_cellview(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("source_cellview=", &set_cellview, gsi::arg ("cellview_index"),
    "@brief Sets the cellview index that this layer refers to\n"
    "\n"
    "See \\cellview for a description of the transformations."
  ) +
  method_ext ("source_layer_index", &get_layer_index, gsi::arg ("real"),
    "@brief Gets the layer index that the shapes are taken from\n"
    "\n"
    "If the layer index is positive, the shapes drawn are taken from this layer rather than "
    "searched for by layer and datatype. This property is stronger than the layer/datatype or "
    "name specification.\n\n"
    "A different method is \\layer_index which indicates the ID of the layer actually used. "
    "While \"source_layer_index\" is one of several ways to address the layer drawn, \"layer_index\" is the ID (index) "
    "of the layer matching the source specification and is >= 0 if such a layer is found.\n\n"
    "If \"real\" is true, the effective value is returned."
  ) +
  method_ext ("source_layer_index", &get_layer_index_1, 
    "@brief Gets the stream layer that the shapes are taken from\n"
    "\n"
    "This method is a convenience method for \"source_layer_index(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("source_layer_index=", &set_layer_index, gsi::arg ("index"),
    "@brief Sets the layer index specification that the shapes are taken from\n"
    "\n"
    "See \\source_layer_index for a description of this property."
  ) +
  method_ext ("source_layer", &get_layer, gsi::arg ("real"),
    "@brief Gets the stream layer that the shapes are taken from\n"
    "\n"
    "If the layer is positive, the actual layer is looked up by this stream layer. If a name or "
    "layer index is specified, the stream layer is not used.\n\n"
    "If \"real\" is true, the effective value is returned."
  ) +
  method_ext ("source_layer", &get_layer_1, 
    "@brief Gets the stream layer that the shapes are taken from\n"
    "\n"
    "This method is a convenience method for \"source_layer(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("source_layer=", &set_layer, gsi::arg ("layer"),
    "@brief Sets the stream layer that the shapes are taken from\n"
    "\n"
    "See \\source_layer for a description of this property"
  ) +
  method_ext ("source_datatype", &get_datatype, gsi::arg ("real"),
    "@brief Gets the stream datatype that the shapes are taken from\n"
    "\n"
    "If the datatype is positive, the actual layer is looked up by this stream datatype. If a name or "
    "layer index is specified, the stream datatype is not used.\n\n"
    "If \"real\" is true, the effective value is returned."
  ) +
  method_ext ("source_datatype", &get_datatype_1, 
    "@brief Gets the stream datatype that the shapes are taken from\n"
    "\n"
    "This method is a convenience method for \"source_datatype(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("source_datatype=", &set_datatype, gsi::arg ("datatype"),
    "@brief Sets the stream datatype that the shapes are taken from\n"
    "\n"
    "See \\datatype for a description of this property"
  ) +
  method_ext ("clear_source_name", &clear_name, 
    "@brief Removes any stream layer name specification from this layer\n"
  ) +
  method_ext ("source_name", &get_name, gsi::arg ("real"),
    "@brief Gets the stream name that the shapes are taken from\n"
    "\n"
    "If the name is non-empty, the actual layer is looked up by this stream layer name. If a "
    "layer index (see \\layer_index) is specified, the stream datatype is not used.\n"
    "A name is only meaningful for OASIS files.\n\n"
    "If \"real\" is true, the effective value is returned."
  ) +
  method_ext ("source_name", &get_name_1, 
    "@brief Gets the stream name that the shapes are taken from\n"
    "\n"
    "This method is a convenience method for \"source_name(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("has_source_name?", &has_name, gsi::arg ("real"),
    "@brief Gets a value indicating whether a stream layer name is specified for this layer\n"
    "\n"
    "If \"real\" is true, the effective value is returned."
  ) +
  method_ext ("has_source_name?", &has_name_1, 
    "@brief Gets a value indicating whether a stream layer name is specified for this layer\n"
    "\n"
    "This method is a convenience method for \"has_source_name?(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("source_name=", &set_name, gsi::arg ("name"),
    "@brief Sets the stream layer name that the shapes are taken from\n"
    "\n"
    "See \\name for a description of this property"
  ) +
  method_ext ("upper_hier_level", &get_upper_hier_level, gsi::arg ("real"),
    "@brief Gets the upper hierarchy level shown\n"
    "\n"
    "This is the hierarchy level at which the drawing starts. "
    "This property is only meaningful, if \\has_upper_hier_level is true. "
    "The hierarchy level can be relative in which case, 0 refers to the context cell's level. "
    "A mode can be specified for the hierarchy level which is 0 for absolute, 1 for minimum "
    "of specified level and set level and 2 for maximum of specified level and set level. "
  ) +
  method_ext ("upper_hier_level", &get_upper_hier_level_1, 
    "@brief Gets the upper hierarchy level shown\n"
    "\n"
    "This method is a convenience method for \"upper_hier_level(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("upper_hier_level_relative?|#upper_hier_level_relative", &get_upper_hier_level_relative, gsi::arg ("real"),
    "@brief Gets a value indicating whether if the upper hierarchy level is relative.\n"
    "\n"
    "See \\upper_hier_level for a description of this property.\n"
    "\n"
    "This method has been introduced in version 0.19.\n"
  ) +
  method_ext ("upper_hier_level_relative?|#upper_hier_level_relative", &get_upper_hier_level_relative_1,
    "@brief Gets a value indicating whether the upper hierarchy level is relative.\n"
    "\n"
    "This method is a convenience method for \"upper_hier_level_relative(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("upper_hier_level_mode", &get_upper_hier_level_mode, gsi::arg ("real"),
    "@brief Gets the mode for the upper hierarchy level.\n"
    "@param real If true, the computed value is returned, otherwise the local node value\n"
    "\n"
    "The mode value can be 0 (value is given by \\upper_hier_level), 1 for \"minimum value\" and 2 for \"maximum value\".\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  method_ext ("upper_hier_level_mode", &get_upper_hier_level_mode_1, 
    "@brief Gets the mode for the upper hierarchy level.\n"
    "\n"
    "This method is a convenience method for \"upper_hier_level_mode(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("upper_hier_level=", &set_upper_hier_level1, gsi::arg ("level"),
    "@brief Sets a upper hierarchy level\n"
    "\n"
    "If this method is called, the upper hierarchy level is enabled. "
    "See \\upper_hier_level for a description of this property.\n"
  ) +
  method_ext ("set_upper_hier_level", &set_upper_hier_level2, gsi::arg ("level"), gsi::arg ("relative"),
    "@brief Sets the upper hierarchy level and if it is relative to the context cell\n"
    "\n"
    "If this method is called, the upper hierarchy level is enabled. "
    "See \\upper_hier_level for a description of this property.\n"
    "\n"
    "This method has been introduced in version 0.19.\n"
  ) +
  method_ext ("set_upper_hier_level", &set_upper_hier_level3, gsi::arg ("level"), gsi::arg ("relative"), gsi::arg ("mode"),
    "@brief Sets the upper hierarchy level, if it is relative to the context cell and the mode\n"
    "\n"
    "If this method is called, the upper hierarchy level is enabled. "
    "See \\upper_hier_level for a description of this property.\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  method_ext ("has_upper_hier_level?", &get_has_upper_hier_level, gsi::arg ("real"),
    "@brief Gets a value indicating whether an upper hierarchy level is explicitly specified\n"
    "\n"
    "If \"real\" is true, the effective value is returned."
  ) +
  method_ext ("has_upper_hier_level?", &get_has_upper_hier_level_1, 
    "@brief Gets a value indicating whether an upper hierarchy level is explicitly specified\n"
    "\n"
    "This method is a convenience method for \"has_upper_hier_level?(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("clear_upper_hier_level", &clear_upper_hier_level, 
    "@brief Clears the upper hierarchy level specification\n"
    "\n"
    "See \\has_upper_hier_level for a description of this property"
  ) +
  method_ext ("lower_hier_level", &get_lower_hier_level, gsi::arg ("real"),
    "@brief Gets the lower hierarchy level shown\n"
    "\n"
    "This is the hierarchy level at which the drawing starts. "
    "This property is only meaningful, if \\has_lower_hier_level is true. "
    "The hierarchy level can be relative in which case, 0 refers to the context cell's level. "
    "A mode can be specified for the hierarchy level which is 0 for absolute, 1 for minimum "
    "of specified level and set level and 2 for maximum of specified level and set level. "
  ) +
  method_ext ("lower_hier_level", &get_lower_hier_level_1, 
    "@brief Gets the lower hierarchy level shown\n"
    "\n"
    "This method is a convenience method for \"lower_hier_level(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("lower_hier_level_relative?|#lower_hier_level_relative", &get_lower_hier_level_relative, gsi::arg ("real"),
    "@brief Gets a value indicating whether the lower hierarchy level is relative.\n"
    "\n"
    "See \\lower_hier_level for a description of this property.\n"
    "\n"
    "This method has been introduced in version 0.19.\n"
  ) +
  method_ext ("lower_hier_level_relative?|#lower_hier_level_relative", &get_lower_hier_level_relative_1,
    "@brief Gets a value indicating whether the upper hierarchy level is relative.\n"
    "\n"
    "This method is a convenience method for \"lower_hier_level_relative(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("lower_hier_level_mode", &get_lower_hier_level_mode, 
    "@brief Gets the mode for the lower hierarchy level.\n"
    "@param real If true, the computed value is returned, otherwise the local node value\n"
    "\n"
    "The mode value can be 0 (value is given by \\lower_hier_level), 1 for \"minimum value\" and 2 for \"maximum value\".\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  method_ext ("lower_hier_level_mode", &get_lower_hier_level_mode_1, 
    "@brief Gets the mode for the lower hierarchy level.\n"
    "\n"
    "This method is a convenience method for \"lower_hier_level_mode(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("lower_hier_level=", &set_lower_hier_level1, gsi::arg ("level"),
    "@brief Sets the lower hierarchy level\n"
    "\n"
    "If this method is called, the lower hierarchy level is enabled. "
    "See \\lower_hier_level for a description of this property.\n"
  ) +
  method_ext ("set_lower_hier_level", &set_lower_hier_level2, gsi::arg ("level"), gsi::arg ("relative"),
    "@brief Sets the lower hierarchy level and if it is relative to the context cell\n"
    "\n"
    "If this method is called, the lower hierarchy level is enabled. "
    "See \\lower_hier_level for a description of this property.\n"
    "\n"
    "This method has been introduced in version 0.19.\n"
  ) +
  method_ext ("set_lower_hier_level", &set_lower_hier_level3, gsi::arg ("level"), gsi::arg ("relative"), gsi::arg ("mode"),
    "@brief Sets the lower hierarchy level, whether it is relative to the context cell and the mode\n"
    "\n"
    "If this method is called, the lower hierarchy level is enabled. "
    "See \\lower_hier_level for a description of this property.\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  method_ext ("has_lower_hier_level?", &get_has_lower_hier_level, gsi::arg ("real"),
    "@brief Gets a value indicating whether a lower hierarchy level is explicitly specified\n"
    "\n"
    "If \"real\" is true, the effective value is returned."
  ) +
  method_ext ("has_lower_hier_level?", &get_has_lower_hier_level_1, 
    "@brief Gets a value indicating whether a lower hierarchy level is explicitly specified\n"
    "\n"
    "This method is a convenience method for \"has_lower_hier_level?(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method_ext ("clear_lower_hier_level", &clear_lower_hier_level, 
    "@brief Clears the lower hierarchy level specification\n"
    "\n"
    "See \\has_lower_hier_level for a description of this property"
  ) +
  method ("source", &lay::LayerProperties::source_string, gsi::arg ("real"),
    "@brief Gets the source specification \n"
    "\n"
    "This method delivers the source specification as a string\n"
    "\n"
    "@param real Set to true to return the computed instead of local value"
  ) +
  method_ext ("source", &source_string_1, 
    "@brief Gets the source specification \n"
    "\n"
    "This method is a convenience method for \"source(true)\"\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  method ("source=", (void (lay::LayerProperties::*) (const std::string &s)) &lay::LayerProperties::set_source, gsi::arg ("s"),
    "@brief Loads the source specification from a string\n"
    "\n"
    "Sets the source specification to the given string. The source specification may contain "
    "the cellview index, the source layer (given by layer/datatype or layer name), transformation, "
    "property selector etc.\n"
    "This method throws an exception if the specification is not valid. \n"
  ) +
  method ("cellview", (int (lay::LayerProperties::*) () const) &lay::LayerProperties::cellview_index, 
    "@brief Gets the the cellview index\n"
    "\n"
    "This is the index of the actual cellview to use. Basically, this method returns \\source_cellview in \"real\" mode. "
    "The result may be different, if the cellview is not valid for example. In this case, a negative value is returned. "
  ) +
  method ("layer_index", (int (lay::LayerProperties::*) () const) &lay::LayerProperties::layer_index, 
    "@brief Gets the the layer index\n"
    "\n"
    "This is the index of the actual layer used. The source specification given by \\source_layer, "
    "\\source_datatype, \\source_name is evaluated and the corresponding "
    "layer is looked up in the layout object. If a \\source_layer_index is specified, this layer index "
    "is taken as the layer index to use."
  ),
  "@brief The layer properties structure\n"
  "\n"
  "The layer properties encapsulate the settings relevant for\n"
  "the display and source of a layer.\n"
  "\n"
  "Each attribute is present in two incarnations: local and real.\n"
  "\"real\" refers to the effective attribute after collecting the \n"
  "attributes from the parents to the leaf property node.\n"
  "In the spirit of this distinction, all read accessors\n"
  "are present in \"local\" and \"real\" form. The read accessors take\n"
  "a boolean parameter \"real\" that must be set to true, if the real\n"
  "value shall be returned.\n"
  "\n"
  "\"brightness\" is a index that indicates how much to make the\n"
  "color brighter to darker rendering the effective color \n"
  "(\\eff_frame_color, \\eff_fill_color). It's value is roughly between\n"
  "-255 and 255.\n"
);

static lay::LayerPropertiesNodeRef add_child (lay::LayerPropertiesNode *node, const lay::LayerProperties *child)
{
  const lay::LayerPropertiesNode *lp = dynamic_cast<const lay::LayerPropertiesNode *> (child);
  if (lp) {
    node->insert_child (node->end_children (), *lp);
  } else {
    node->insert_child (node->end_children (), lay::LayerPropertiesNode (*child));
  }

  const lay::LayerPropertiesNodeRef *ref = dynamic_cast<lay::LayerPropertiesNodeRef *> (node);
  if (ref && ref->is_valid ()) {
    return lay::LayerPropertiesNodeRef (ref->iter ().last_child ().next_sibling (-1));
  } else {
    return lay::LayerPropertiesNodeRef (&node->end_children ()[-1]);
  }
}

static lay::LayerPropertiesNodeRef add_child0 (lay::LayerPropertiesNode *node)
{
  lay::LayerProperties lp_empty;
  return add_child (node, &lp_empty);
}

static void clear_children (lay::LayerPropertiesNode *node)
{
  node->clear_children ();
}

Class<lay::LayerPropertiesNode> decl_LayerPropertiesNode (
  decl_LayerProperties, 
  "lay", "LayerPropertiesNode",
  method ("==", &lay::LayerPropertiesNode::operator==, gsi::arg ("other"),
    "@brief Equality \n"
    "\n"
    "@param other The other object to compare against"
  ) +
  method ("!=", &lay::LayerPropertiesNode::operator!=, gsi::arg ("other"),
    "@brief Inequality \n"
    "\n"
    "@param other The other object to compare against"
  ) +
  method ("flat", &lay::LayerPropertiesNode::flat,
    "@brief return the \"flattened\" (effective) layer properties node for this node\n"
    "\n"
    "This method returns a \\LayerPropertiesNode object that is not embedded into a hierarchy.\n"
    "This object represents the effective layer properties for the given node. In particular, "
    "all 'local' properties are identical to the 'real' properties. Such an object can be "
    "used as a basis for manipulations.\n"
    "\n"
    "Unlike the name suggests, this node will still contain a hierarchy of nodes below if the original "
    "node did so."
  ) +
  method ("is_expanded?", &lay::LayerPropertiesNode::expanded,
    "@brief Gets a value indicating whether the layer tree node is expanded.\n"
    "This predicate has been introduced in version 0.28.6."
  ) +
  method ("expanded=", &lay::LayerPropertiesNode::set_expanded, gsi::arg ("ex"),
    "@brief Set a value indicating whether the layer tree node is expanded.\n"
    "Setting this value to 'true' will expand (open) the tree node. Setting it to 'false' will collapse the node.\n"
    "\n"
    "This predicate has been introduced in version 0.28.6."
  ) +
  method_ext ("add_child", &add_child0,
    "@brief Add a child entry\n"
    "@return A reference to the node created\n"
    "This method allows building a layer properties tree by adding children to node objects. "
    "It returns a reference to the node object created which is a freshly initialized one.\n"
    "\n"
    "The parameterless version of this method was introduced in version 0.25."
  ) +
  method_ext ("add_child", &add_child, gsi::arg ("child"),
    "@brief Add a child entry\n"
    "@return A reference to the node created\n"
    "This method allows building a layer properties tree by adding children to node objects. "
    "It returns a reference to the node object created.\n"
    "\n"
    "This method was introduced in version 0.22."
  ) +
  method_ext ("clear_children", &clear_children,
    "@brief Clears all children\n"
    "This method was introduced in version 0.22."
  ) +
  method ("has_children?", &lay::LayerPropertiesNode::has_children, 
    "@brief Test, if there are children\n"
  ) +
  method ("bbox", &lay::LayerPropertiesNode::bbox, 
    "@brief Compute the bbox of this layer\n"
    "\n"
    "This takes the layout and path definition (supported by the\n"
    "given default layout or path, if no specific is given).\n"
    "The node must have been attached to a view to make this\n"
    "operation possible.\n"
    "\n"
    "@return A bbox in micron units\n"
  ) +
  method ("list_index", &lay::LayerPropertiesNode::list_index,
    "@brief Gets the index of the layer properties list that the node lives in\n"
  ) +
  method ("id", &lay::LayerPropertiesNode::id,
    "@brief Obtain the unique ID\n"
    "\n"
    "Each layer properties node object has a unique ID that is created \n"
    "when a new LayerPropertiesNode object is instantiated. The ID is\n"
    "copied when the object is copied. The ID can be used to identify the\n"
    "object irregardless of its content.\n"
  ),
  "@brief A layer properties node structure\n"
  "\n"
  "This class is derived from \\LayerProperties. Objects of this class are used\n"
  "in the hierarchy of layer views that are arranged in a tree while the \\LayerProperties\n"
  "object reflects the properties of a single node."
);

void assign1 (lay::LayerPropertiesNodeRef *ref, const lay::LayerPropertiesNode &other)
{
  ref->assign (other);
}

void assign2 (lay::LayerPropertiesNodeRef *ref, const lay::LayerProperties &other)
{
  ref->assign_lp (other);
}

static lay::LayerPropertiesNode lp_dup (const lay::LayerPropertiesNodeRef *ref)
{
  if (ref->target ()) {
    return *ref->target ();
  } else {
    return lay::LayerPropertiesNode ();
  }
}

Class<lay::LayerPropertiesNodeRef> decl_LayerPropertiesNodeRef (
  decl_LayerPropertiesNode,
  "lay", "LayerPropertiesNodeRef",
  method ("delete", &lay::LayerPropertiesNodeRef::erase,
    "@brief Erases the current node and all child nodes\n"
    "\n"
    "After erasing the node, the reference will become invalid."
  ) +
  method_ext ("assign", &assign1, gsi::arg ("other"),
    "@brief Assigns the contents of the 'other' object to self.\n"
    "\n"
    "This version accepts a \\LayerPropertiesNode object and allows modification of the layer node's hierarchy. "
    "Assignment will reconfigure the layer node in the view."
  ) +
  method_ext ("assign", &assign2, gsi::arg ("other"),
    "@brief Assigns the contents of the 'other' object to self.\n"
    "\n"
    "This version accepts a \\LayerProperties object. Assignment will change the properties of the layer in the "
    "view."
  ) +
  method_ext ("dup", &lp_dup,
    "@brief Creates a \\LayerPropertiesNode object as a copy of the content of this node.\n"
    "This method is mainly provided for backward compatibility with 0.24 and before."
  ) +
  method ("is_valid?", &lay::LayerPropertiesNodeRef::is_valid,
    "@brief Returns true, if the reference points to a valid layer properties node\n"
    "\n"
    "Invalid references behave like ordinary \\LayerPropertiesNode objects but without "
    "the ability to update the view upon changes of attributes."
  ),
  "@brief A class representing a reference to a layer properties node\n"
  "\n"
  "This object is returned by the layer properties iterator's current method (\\LayerPropertiesIterator#current). "
  "A reference behaves like a layer properties node, but changes in the node are reflected in the view it is attached to.\n"
  "\n"
  "A typical use case for references is this:\n"
  "\n"
  "@code\n"
  "# Hides a layers of a view\n"
  "view = RBA::LayoutView::current\n"
  "view.each_layer do |lref|\n"
  "  # lref is a LayerPropertiesNodeRef object\n"
  "  lref.visible = false\n"
  "end\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.25.\n"
);

static lay::LayerPropertiesNodeRef current (const lay::LayerPropertiesConstIterator *iter)
{
  return lay::LayerPropertiesNodeRef (*iter);
}

Class<lay::LayerPropertiesConstIterator> decl_LayerPropertiesIterator (
  "lay", "LayerPropertiesIterator",
  method ("!=", &lay::LayerPropertiesConstIterator::operator!=, gsi::arg ("other"),
    "@brief Inequality\n"
    "\n"
    "@param other The other object to compare against"
  ) +
  method ("==", &lay::LayerPropertiesConstIterator::operator==, gsi::arg ("other"),
    "@brief Equality\n"
    "\n"
    "@param other The other object to compare against"
    "\n"
    "Returns true, if self and other point to the same layer properties node. Caution: this does "
    "not imply that both layer properties nodes sit in the same tab. Just their position in the tree is compared."
  ) +
  method ("<", &lay::LayerPropertiesConstIterator::operator<, gsi::arg ("other"),
    "@brief Comparison\n"
    "\n"
    "@param other The other object to compare against\n"
    "\n"
    "@return true, if self points to an object that comes before other\n"
  ) +
  method ("at_top?", &lay::LayerPropertiesConstIterator::at_top, 
    "@brief At-the-top property\n"
    "\n"
    "This predicate is true if there is no parent node above the node addressed by self.\n"
  ) +
  method ("at_end?", &lay::LayerPropertiesConstIterator::at_end, 
    "@brief At-the-end property\n"
    "\n"
    "This predicate is true if the iterator is at the end of either all elements or\n"
    "at the end of the child list (if \\down_last_child or \\down_first_child is used to iterate).\n"
  ) +
  method ("is_null?", &lay::LayerPropertiesConstIterator::is_null, 
    "@brief \"is null\" predicate\n"
    "\n"
    "This predicate is true if the iterator is \"null\". Such an iterator can be\n"
    "created with the default constructor or by moving a top-level iterator up.\n"
  ) +
  method ("next", &lay::LayerPropertiesConstIterator::operator++, 
    "@brief Increment operator\n"
    "\n"
    "The iterator will be incremented to point to the next layer entry. It will descend "
    "into the hierarchy to address child nodes if there are any."
  ) +
  method ("up", &lay::LayerPropertiesConstIterator::up, 
    "@brief Move up\n"
    "\n"
    "The iterator is moved to point to the current element's parent.\n"
    "If the current element does not have a parent, the iterator will\n"
    "become a null iterator.\n"
  ) +
  method ("next_sibling", &lay::LayerPropertiesConstIterator::next_sibling, gsi::arg ("n"),
    "@brief Move to the next sibling by a given distance\n"
    "\n"
    "\n"
    "The iterator is moved to the nth next sibling of the current element. Use negative distances to move backward.\n"
  ) +
  method ("to_sibling", &lay::LayerPropertiesConstIterator::to_sibling, gsi::arg ("n"),
    "@brief Move to the sibling with the given index\n"
    "\n"
    "\n"
    "The iterator is moved to the nth sibling by selecting the nth child in the current node's parent.\n"
  ) +
  method ("num_siblings", &lay::LayerPropertiesConstIterator::num_siblings, 
    "@brief Return the number of siblings\n"
    "\n"
    "The count includes the current element. More precisely, this property delivers the number of children "
    "of the current node's parent."
  ) +
  method ("down_first_child", &lay::LayerPropertiesConstIterator::down_first_child, 
    "@brief Move to the first child\n"
    "\n"
    "This method moves to the first child of the current element. If there is\n"
    "no child, \\at_end? will be true. Even then, the iterator is sitting at the \n"
    "the child level and \\up can be used to move back.\n"
  ) +
  method ("down_last_child", &lay::LayerPropertiesConstIterator::down_last_child, 
    "@brief Move to the last child\n"
    "\n"
    "This method moves behind the last child of the current element. \\at_end? will be\n"
    "true then. Even then, the iterator points to the child level and \\up \n"
    "can be used to move back.\n"
    "\n"
    "Despite the name, the iterator does not address the last child, but the position after that child. "
    "To actually get the iterator for the last child, use down_last_child and next_sibling(-1)."
  ) +
  method_ext ("current", &current,
    "@brief Returns a reference to the layer properties node that the iterator points to\n"
    "\n"
    "Starting with version 0.25, the returned object can be manipulated and the changes will be "
    "reflected in the view immediately.\n"
  ) +
  method ("parent", &lay::LayerPropertiesConstIterator::parent, 
    "@brief Returns the iterator pointing to the parent node\n"
    "\n"
    "This method will return an iterator pointing to the parent element.\n"
    "If there is no parent, the returned iterator will be a null iterator.\n"
  ) +
  method ("first_child", &lay::LayerPropertiesConstIterator::first_child, 
    "@brief Returns the iterator pointing to the first child\n"
    "\n"
    "If there is no children, the iterator will be a valid insert point but not\n"
    "point to any valid element. It will report \\at_end? = true.\n"
  ) +
  method ("last_child", &lay::LayerPropertiesConstIterator::last_child, 
    "@brief Returns the iterator pointing behind the last child\n"
    "\n"
    "The iterator will be a valid insert point but not\n"
    "point to any valid element. It will report \\at_end? = true.\n"
    "\n"
    "Despite the name, the iterator does not address the last child, but the position after that child. "
    "To actually get the iterator for the last child, use last_child and call next_sibling(-1) on that iterator.\n"
  ) +
  method ("child_index", &lay::LayerPropertiesConstIterator::child_index, 
    "@brief Returns the index of the child within the parent\n"
    "\n"
    "This method returns the index of that the properties node the iterator points to in the list\n"
    "of children of its parent. If the element does not have a parent, the \n"
    "index of the element in the global list is returned.\n"
  ),  
  "@brief Layer properties iterator\n"
  "\n"
  "This iterator provides a flat view for the layers in the layer tree if used with the next method. In this mode "
  "it will descend into the hierarchy and deliver node by node as a linear (flat) sequence.\n"
  "\n"
  "The iterator can also be used to navigate through the node hierarchy using \\next_sibling, \\down_first_child, \\parent etc.\n"
  "\n"
  "The iterator also plays an important role for manipulating the layer properties tree, i.e. by specifying "
  "insertion points in the tree for the \\LayoutView class."
  "\n"
);


}
