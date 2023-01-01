
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


#include "libBasicText.h"
#include "dbGlyphs.h"

namespace lib
{

// --------------------------------------------------------------------------
//  Implementation

static const size_t p_text = 0;
static const size_t p_font_name = 1;
static const size_t p_layer = 2;
static const size_t p_magnification = 3;
static const size_t p_inverse = 4;
static const size_t p_bias = 5;
static const size_t p_char_spacing = 6;
static const size_t p_line_spacing = 7;
static const size_t p_eff_cell_width = 8;
static const size_t p_eff_cell_height = 9;
static const size_t p_eff_line_width = 10;
static const size_t p_eff_design_raster = 11;
static const size_t p_font = 12;
static const size_t p_total = 13;

BasicText::BasicText ()
{
  //  .. nothing yet ..
}

bool 
BasicText::can_create_from_shape (const db::Layout & /*layout*/, const db::Shape &shape, unsigned int /*layer*/) const
{
  return shape.is_text ();
}

db::Trans
BasicText::transformation_from_shape (const db::Layout & /*layout*/, const db::Shape &shape, unsigned int /*layer*/) const
{
  //  use the displacement to define the center of the circle
  return shape.text_trans ();
}

db::pcell_parameters_type
BasicText::parameters_from_shape (const db::Layout &layout, const db::Shape &shape, unsigned int layer) const
{
  //  use map_parameters to create defaults for the other parameters
  std::map<size_t, tl::Variant> nm;
  nm.insert (std::make_pair (p_layer, tl::Variant (layout.get_properties (layer))));
  nm.insert (std::make_pair (p_text, tl::Variant (shape.text_string ())));
  if (shape.text_size () > 0) {
    double hfont = 1.0;
    if (! db::TextGenerator::generators ().empty ()) {
      hfont = db::TextGenerator::generators ().front ().height () * db::TextGenerator::generators ().front ().dbu ();
    }
    nm.insert (std::make_pair (p_magnification, tl::Variant (layout.dbu () * shape.text_size () / hfont)));
  }
  return map_parameters (nm);
}

std::vector<db::PCellLayerDeclaration> 
BasicText::get_layer_declarations (const db::pcell_parameters_type &parameters) const
{
  std::vector<db::PCellLayerDeclaration> layers;
  if (parameters.size () > p_layer && parameters [p_layer].is_user<db::LayerProperties> ()) {
    db::LayerProperties lp = parameters [p_layer].to_user<db::LayerProperties> ();
    if (lp != db::LayerProperties ()) {
      layers.push_back (lp);
    }
  }
  return layers;
}

int
BasicText::get_font_index (const db::pcell_parameters_type &parameters) const
{
  int f = -1;
  if (parameters.size () > p_font) {
    f = parameters [p_font].to_int ();
  }
  if (f < 0 || f >= int (db::TextGenerator::generators ().size ())) {
    f = 0;
  }

  //  if just the index is given set the name and vice versa
  std::string fn = parameters [p_font_name].to_string ();
  if (! fn.empty ()) {
    for (std::vector<db::TextGenerator>::const_iterator i = db::TextGenerator::generators ().begin (); i != db::TextGenerator::generators ().end (); ++i) {
      if (i->name () == fn) {
        f = int (i - db::TextGenerator::generators ().begin ());
        break;
      }
    }
  }

  return f;
}

void 
BasicText::coerce_parameters (const db::Layout &layout, db::pcell_parameters_type &parameters) const
{
  //  Compute the read-only parameters

  if (parameters.size () < p_total || db::TextGenerator::generators ().empty ()) {
    return;
  }

  std::string t = parameters [p_text].to_string ();

  int f = get_font_index (parameters);
  const db::TextGenerator &font = db::TextGenerator::generators ()[f];

  parameters [p_font_name] = font.name ();
  parameters [p_font] = f;

  double m = parameters [p_magnification].to_double ();
  double b = parameters [p_bias].to_double ();
  parameters [p_eff_cell_width] = font.width () * layout.dbu () * m;
  parameters [p_eff_cell_height] = font.height () * layout.dbu () * m;
  parameters [p_eff_line_width] = font.line_width () * layout.dbu () * m + 2.0 * b;
  parameters [p_eff_design_raster] = font.design_grid () * layout.dbu () * m;
}

void 
BasicText::produce (const db::Layout &layout, const std::vector<unsigned int> &layer_ids, const db::pcell_parameters_type &parameters, db::Cell &cell) const
{
  if (parameters.size () < 6 || layer_ids.size () < 1 || db::TextGenerator::generators ().empty ()) {
    return;
  }

  int f = get_font_index (parameters);
  const db::TextGenerator &font = db::TextGenerator::generators ()[f];

  double m = parameters [p_magnification].to_double ();
  double b = parameters [p_bias].to_double ();
  bool inv = parameters [p_inverse].to_bool ();

  double dx = parameters [p_char_spacing].to_double ();
  double dy = parameters [p_line_spacing].to_double ();

  std::string t = parameters [p_text].to_string ();

  std::vector<db::Polygon> data;
  font.text (t, layout.dbu (), m, inv, b, dx, dy, data);

  for (std::vector<db::Polygon>::const_iterator d = data.begin (); d != data.end (); ++d) {
    cell.shapes (layer_ids [0]).insert (*d);
  }
}

std::string 
BasicText::get_display_name (const db::pcell_parameters_type &parameters) const
{
  std::string t;
  if (! parameters.empty ()) {
    t = parameters [p_text].to_string ();
  }
  return "TEXT(l=" + std::string (parameters [p_layer].to_string ()) +
              ",'" + t + "')";
}

std::vector<db::PCellParameterDeclaration> 
BasicText::get_parameter_declarations () const
{
  std::vector<db::PCellParameterDeclaration> parameters;

  //  parameter: text 
  tl_assert (parameters.size () == p_text);
  parameters.push_back (db::PCellParameterDeclaration ("text"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_string);
  parameters.back ().set_description (tl::to_string (tr ("Text")));
  parameters.back ().set_default ("");

  //  parameter: font name
  tl_assert (parameters.size () == p_font_name);
  parameters.push_back (db::PCellParameterDeclaration ("font_name"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_string);
  parameters.back ().set_description (tl::to_string (tr ("Font")));
  parameters.back ().set_default (0);

  std::vector<tl::Variant> choices;
  std::vector<std::string> choice_descriptions;
  for (std::vector<db::TextGenerator>::const_iterator f = db::TextGenerator::generators ().begin (); f != db::TextGenerator::generators ().end (); ++f) {
    choice_descriptions.push_back (f->description ());
    choices.push_back (f->name ());
  }
  parameters.back ().set_choices (choices);
  parameters.back ().set_choice_descriptions (choice_descriptions);

  //  parameter: layer
  tl_assert (parameters.size () == p_layer);
  parameters.push_back (db::PCellParameterDeclaration ("layer"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_layer);
  parameters.back ().set_description (tl::to_string (tr ("Layer")));

  //  parameter: magnification
  tl_assert (parameters.size () == p_magnification);
  parameters.push_back (db::PCellParameterDeclaration ("mag"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Magnification")));
  parameters.back ().set_default (1.0);

  //  parameter: inverse 
  tl_assert (parameters.size () == p_inverse);
  parameters.push_back (db::PCellParameterDeclaration ("inverse"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_boolean);
  parameters.back ().set_description (tl::to_string (tr ("Inverse")));
  parameters.back ().set_default (false);

  //  parameter: bias 
  tl_assert (parameters.size () == p_bias);
  parameters.push_back (db::PCellParameterDeclaration ("bias"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Bias")));
  parameters.back ().set_default (0.0);
  parameters.back ().set_unit (tl::to_string (tr ("micron")));

  //  parameter: character spacing 
  tl_assert (parameters.size () == p_char_spacing);
  parameters.push_back (db::PCellParameterDeclaration ("cspacing"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Additional character spacing")));
  parameters.back ().set_default (0.0);
  parameters.back ().set_unit (tl::to_string (tr ("micron")));

  //  parameter: line spacing 
  tl_assert (parameters.size () == p_line_spacing);
  parameters.push_back (db::PCellParameterDeclaration ("lspacing"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Additional line spacing")));
  parameters.back ().set_default (0.0);
  parameters.back ().set_unit (tl::to_string (tr ("micron")));

  //  parameter: effective cell width
  tl_assert (parameters.size () == p_eff_cell_width);
  parameters.push_back (db::PCellParameterDeclaration ("eff_cw"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Computed parameters\tCell width")));
  parameters.back ().set_default (0.0);
  parameters.back ().set_unit (tl::to_string (tr ("micron")));
  parameters.back ().set_readonly (true);

  //  parameter: effective cell height
  tl_assert (parameters.size () == p_eff_cell_height);
  parameters.push_back (db::PCellParameterDeclaration ("eff_ch"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Computed parameters\tCell height")));
  parameters.back ().set_default (0.0);
  parameters.back ().set_unit (tl::to_string (tr ("micron")));
  parameters.back ().set_readonly (true);

  //  parameter: effective line width
  tl_assert (parameters.size () == p_eff_line_width);
  parameters.push_back (db::PCellParameterDeclaration ("eff_lw"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Computed parameters\tLine width")));
  parameters.back ().set_default (0.0);
  parameters.back ().set_unit (tl::to_string (tr ("micron")));
  parameters.back ().set_readonly (true);

  //  parameter: effective design raster
  tl_assert (parameters.size () == p_eff_design_raster);
  parameters.push_back (db::PCellParameterDeclaration ("eff_dr"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Computed parameters\tDesign raster")));
  parameters.back ().set_default (0.0);
  parameters.back ().set_unit (tl::to_string (tr ("micron")));
  parameters.back ().set_readonly (true);

  //  parameter: font number
  //  This parameter is deprecated - it is used only if the font name is not
  //  given. It is provided for backward compatibility.
  tl_assert (parameters.size () == p_font);
  parameters.push_back (db::PCellParameterDeclaration ("font"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_int);
  parameters.back ().set_description (tl::to_string (tr ("Font")));
  parameters.back ().set_default (0);
  parameters.back ().set_hidden (true);

  return parameters;
}

}


