
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


#ifndef HDR_antConfig
#define HDR_antConfig

#include "antCommon.h"

#include <string>

#include "antService.h"
#include "laySnap.h"

namespace ant
{

/** 
 *  @brief Declaration of the configuration names
 */
extern ANT_PUBLIC const std::string cfg_max_number_of_rulers;
extern ANT_PUBLIC const std::string cfg_ruler_snap_range;
extern ANT_PUBLIC const std::string cfg_ruler_color;
extern ANT_PUBLIC const std::string cfg_ruler_halo;
extern ANT_PUBLIC const std::string cfg_ruler_snap_mode;
extern ANT_PUBLIC const std::string cfg_ruler_obj_snap;
extern ANT_PUBLIC const std::string cfg_ruler_grid_snap;
extern ANT_PUBLIC const std::string cfg_ruler_grid_micron;
extern ANT_PUBLIC const std::string cfg_ruler_templates;
extern ANT_PUBLIC const std::string cfg_current_ruler_template;

// ------------------------------------------------------------
//  Helper functions to get and set the configuration

struct ACConverter 
{
  std::string to_string (const lay::angle_constraint_type &m);
  void from_string (const std::string &s, lay::angle_constraint_type &m);
};

struct StyleConverter 
{
  std::string to_string (ant::Object::style_type s);
  void from_string (const std::string &s, ant::Object::style_type &style);
};

struct OutlineConverter 
{
  std::string to_string (ant::Object::outline_type s);
  void from_string (const std::string &s, ant::Object::outline_type &outline);
};

struct PositionConverter
{
  std::string to_string (ant::Object::position_type p);
  void from_string (const std::string &s, ant::Object::position_type &pos);
};

struct AlignmentConverter
{
  std::string to_string (ant::Object::alignment_type a);
  void from_string (const std::string &s, ant::Object::alignment_type &a);
};

struct RulerModeConverter
{
  std::string to_string (ant::Template::ruler_mode_type a);
  void from_string (const std::string &s, ant::Template::ruler_mode_type &a);
};

struct TemplatesConverter
{
  std::string to_string (const std::vector <ant::Template> &t);
  void from_string (const std::string &s, std::vector <ant::Template> &t);
};

}

#endif

