
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


#include "antConfig.h"

namespace ant
{

// ------------------------------------------------------------
//  Helper functions to get and set the configuration

std::string 
ACConverter::to_string (const lay::angle_constraint_type &m)
{
  if (m == lay::AC_Any) {
    return "any";
  } else if (m == lay::AC_Diagonal) {
    return "diagonal";
  } else if (m == lay::AC_Ortho) {
    return "ortho";
  } else if (m == lay::AC_Horizontal) {
    return "horizontal";
  } else if (m == lay::AC_Vertical) {
    return "vertical";
  } else if (m == lay::AC_Global) {
    return "global";
  } else {
    return "";
  }
}

void 
ACConverter::from_string (const std::string &tt, lay::angle_constraint_type &m)
{
  std::string t (tl::trim (tt));
  if (t == "any") {
    m = lay::AC_Any;
  } else if (t == "diagonal") {
    m = lay::AC_Diagonal;
  } else if (t == "ortho") {
    m = lay::AC_Ortho;
  } else if (t == "horizontal") {
    m = lay::AC_Horizontal;
  } else if (t == "vertical") {
    m = lay::AC_Vertical;
  } else if (t == "global") {
    m = lay::AC_Global;
  } else {
    m = lay::AC_Any;
  }
}

std::string 
StyleConverter::to_string (ant::Object::style_type s)
{
  if (s == ant::Object::STY_ruler) {
    return "ruler";
  } else if (s == ant::Object::STY_arrow_end) {
    return "arrow_end";
  } else if (s == ant::Object::STY_arrow_start) {
    return "arrow_start";
  } else if (s == ant::Object::STY_arrow_both) {
    return "arrow_both";
  } else if (s == ant::Object::STY_cross_start) {
    return "cross_start";
  } else if (s == ant::Object::STY_cross_end) {
    return "cross_end";
  } else if (s == ant::Object::STY_cross_both) {
    return "cross_both";
  } else if (s == ant::Object::STY_line) {
    return "line";
  } else {
    return "";
  }
}

void  
StyleConverter::from_string (const std::string &tt, ant::Object::style_type &s)
{
  std::string t (tl::trim (tt));
  if (t == "ruler") {
    s = ant::Object::STY_ruler;
  } else if (t == "arrow_end") {
    s = ant::Object::STY_arrow_end;
  } else if (t == "arrow_start") {
    s = ant::Object::STY_arrow_start;
  } else if (t == "arrow_both") {
    s = ant::Object::STY_arrow_both;
  } else if (t == "cross_start") {
    s = ant::Object::STY_cross_start;
  } else if (t == "cross_end") {
    s = ant::Object::STY_cross_end;
  } else if (t == "cross_both") {
    s = ant::Object::STY_cross_both;
  } else if (t == "line") {
    s = ant::Object::STY_line;
  } else {
    s = ant::Object::STY_ruler;
  }
}

std::string  
OutlineConverter::to_string (ant::Object::outline_type o)
{
  if (o == ant::Object::OL_diag) {
    return "diag";
  } else if (o == ant::Object::OL_xy) {
    return "xy";
  } else if (o == ant::Object::OL_diag_xy) {
    return "diag_xy";
  } else if (o == ant::Object::OL_yx) {
    return "yx";
  } else if (o == ant::Object::OL_diag_yx) {
    return "diag_yx";
  } else if (o == ant::Object::OL_box) {
    return "box";
  } else if (o == ant::Object::OL_ellipse) {
    return "ellipse";
  } else if (o == ant::Object::OL_radius) {
    return "radius";
  } else if (o == ant::Object::OL_angle) {
    return "angle";
  } else {
    return "";
  }
}

void   
OutlineConverter::from_string (const std::string &s, ant::Object::outline_type &o)
{
  std::string t (tl::trim (s));
  if (t == "diag") {
    o = ant::Object::OL_diag;
  } else if (t == "xy") {
    o = ant::Object::OL_xy;
  } else if (t == "diag_xy") {
    o = ant::Object::OL_diag_xy;
  } else if (t == "yx") {
    o = ant::Object::OL_yx;
  } else if (t == "diag_yx") {
    o = ant::Object::OL_diag_yx;
  } else if (t == "box") {
    o = ant::Object::OL_box;
  } else if (t == "ellipse") {
    o = ant::Object::OL_ellipse;
  } else if (t == "radius") {
    o = ant::Object::OL_radius;
  } else if (t == "angle") {
    o = ant::Object::OL_angle;
  } else {
    o = ant::Object::OL_diag;
  }
}

std::string
PositionConverter::to_string (ant::Object::position_type p)
{
  if (p == ant::Object::POS_auto) {
    return "auto";
  } else if (p == ant::Object::POS_p1) {
    return "p1";
  } else if (p == ant::Object::POS_p2) {
    return "p2";
  } else if (p == ant::Object::POS_center) {
    return "center";
  } else {
    return "";
  }
}

void
PositionConverter::from_string (const std::string &s, ant::Object::position_type &p)
{
  std::string t (tl::trim (s));
  if (t == "auto") {
    p = ant::Object::POS_auto;
  } else if (t == "p1") {
    p = ant::Object::POS_p1;
  } else if (t == "p2") {
    p = ant::Object::POS_p2;
  } else if (t == "center") {
    p = ant::Object::POS_center;
  } else {
    p = ant::Object::POS_auto;
  }
}

std::string
AlignmentConverter::to_string (ant::Object::alignment_type a)
{
  if (a == ant::Object::AL_auto) {
    return "auto";
  } else if (a == ant::Object::AL_center) {
    return "center";
  } else if (a == ant::Object::AL_down) {
    return "down";
  } else if (a == ant::Object::AL_up) {
    return "up";
  } else {
    return "";
  }
}

void
AlignmentConverter::from_string (const std::string &s, ant::Object::alignment_type &a)
{
  std::string t (tl::trim (s));
  if (t == "auto") {
    a = ant::Object::AL_auto;
  } else if (t == "center") {
    a = ant::Object::AL_center;
  } else if (t == "down") {
    a = ant::Object::AL_down;
  } else if (t == "up") {
    a = ant::Object::AL_up;
  } else {
    a = ant::Object::AL_auto;
  }
}

std::string
RulerModeConverter::to_string (ant::Template::ruler_mode_type m)
{
  if (m == ant::Template::RulerNormal) {
    return "normal";
  } else if (m == ant::Template::RulerSingleClick) {
    return "single_click";
  } else if (m == ant::Template::RulerAutoMetric) {
    return "auto_metric";
  } else if (m == ant::Template::RulerMultiSegment) {
    return "multi_segment";
  } else if (m == ant::Template::RulerThreeClicks) {
    return "angle";
  } else {
    return "normal";
  }
}

void
RulerModeConverter::from_string (const std::string &s, ant::Template::ruler_mode_type &a)
{
  std::string t (tl::trim (s));
  if (t == "normal") {
    a = ant::Template::RulerNormal;
  } else if (t == "single_click") {
    a = ant::Template::RulerSingleClick;
  } else if (t == "auto_metric") {
    a = ant::Template::RulerAutoMetric;
  } else if (t == "multi_segment") {
    a = ant::Template::RulerMultiSegment;
  } else if (t == "angle") {
    a = ant::Template::RulerThreeClicks;
  } else {
    a = ant::Template::RulerNormal;
  }
}

std::string
TemplatesConverter::to_string (const std::vector <ant::Template> &t)
{
  return ant::Template::to_string (t);
}

void 
TemplatesConverter::from_string (const std::string &s, std::vector <ant::Template> &t)
{
  t = ant::Template::from_string (s);
}

// ------------------------------------------------------------
//  Declaration of the configuration options

const std::string cfg_max_number_of_rulers ("rulers");
const std::string cfg_ruler_snap_range ("ruler-snap-range");
const std::string cfg_ruler_color ("ruler-color");
const std::string cfg_ruler_halo ("ruler-halo");
const std::string cfg_ruler_snap_mode ("ruler-snap-mode");
const std::string cfg_ruler_obj_snap ("ruler-obj-snap");
const std::string cfg_ruler_grid_snap ("ruler-grid-snap");
const std::string cfg_ruler_grid_micron ("grid-micron");
const std::string cfg_ruler_templates ("ruler-templates-v2");
const std::string cfg_current_ruler_template ("current-ruler-template");

} // namespace ant

