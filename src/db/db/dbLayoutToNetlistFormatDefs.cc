
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#include "dbLayoutToNetlistFormatDefs.h"

namespace db
{

namespace l2n_std_format
{
  static const std::string long_version_key ("version");
  static const std::string long_description_key ("description");
  static const std::string long_top_key ("top");
  static const std::string long_unit_key ("unit");
  static const std::string long_layer_key ("layer");
  static const std::string long_connect_key ("connect");
  static const std::string long_global_key ("global");
  static const std::string long_circuit_key ("circuit");
  static const std::string long_net_key ("net");
  static const std::string long_device_key ("device");
  static const std::string long_polygon_key ("polygon");
  static const std::string long_rect_key ("rect");
  static const std::string long_terminal_key ("terminal");
  static const std::string long_abstract_key ("abstract");
  static const std::string long_param_key ("param");
  static const std::string long_location_key ("location");
  static const std::string long_rotation_key ("rotation");
  static const std::string long_mirror_key ("mirror");
  static const std::string long_scale_key ("scale");
  static const std::string long_pin_key ("pin");

  static const std::string short_version_key ("V");
  static const std::string short_description_key ("B");
  static const std::string short_top_key ("W");
  static const std::string short_unit_key ("U");
  static const std::string short_layer_key ("L");
  static const std::string short_connect_key ("C");
  static const std::string short_global_key ("G");
  static const std::string short_circuit_key ("X");
  static const std::string short_net_key ("N");
  static const std::string short_device_key ("D");
  static const std::string short_polygon_key ("Q");
  static const std::string short_rect_key ("R");
  static const std::string short_terminal_key ("T");
  static const std::string short_abstract_key ("A");
  static const std::string short_param_key ("E");
  static const std::string short_location_key ("Y");
  static const std::string short_rotation_key ("O");
  static const std::string short_mirror_key ("M");
  static const std::string short_scale_key ("S");
  static const std::string short_pin_key ("P");

  template<> const std::string &keys<false>::version_key = long_version_key;
  template<> const std::string &keys<false>::description_key = long_description_key;
  template<> const std::string &keys<false>::top_key = long_top_key;
  template<> const std::string &keys<false>::unit_key = long_unit_key;
  template<> const std::string &keys<false>::layer_key = long_layer_key;
  template<> const std::string &keys<false>::connect_key = long_connect_key;
  template<> const std::string &keys<false>::global_key = long_global_key;
  template<> const std::string &keys<false>::circuit_key = long_circuit_key;
  template<> const std::string &keys<false>::net_key = long_net_key;
  template<> const std::string &keys<false>::device_key = long_device_key;
  template<> const std::string &keys<false>::polygon_key = long_polygon_key;
  template<> const std::string &keys<false>::rect_key = long_rect_key;
  template<> const std::string &keys<false>::terminal_key = long_terminal_key;
  template<> const std::string &keys<false>::abstract_key = long_abstract_key;
  template<> const std::string &keys<false>::param_key = long_param_key;
  template<> const std::string &keys<false>::location_key = long_location_key;
  template<> const std::string &keys<false>::rotation_key = long_rotation_key;
  template<> const std::string &keys<false>::mirror_key = long_mirror_key;
  template<> const std::string &keys<false>::scale_key = long_scale_key;
  template<> const std::string &keys<false>::pin_key = long_pin_key;

  template<> const std::string &keys<true>::version_key = short_version_key;
  template<> const std::string &keys<true>::description_key = short_description_key;
  template<> const std::string &keys<true>::top_key = short_top_key;
  template<> const std::string &keys<true>::unit_key = short_unit_key;
  template<> const std::string &keys<true>::layer_key = short_layer_key;
  template<> const std::string &keys<true>::connect_key = short_connect_key;
  template<> const std::string &keys<true>::global_key = short_global_key;
  template<> const std::string &keys<true>::circuit_key = short_circuit_key;
  template<> const std::string &keys<true>::net_key = short_net_key;
  template<> const std::string &keys<true>::device_key = short_device_key;
  template<> const std::string &keys<true>::polygon_key = short_polygon_key;
  template<> const std::string &keys<true>::rect_key = short_rect_key;
  template<> const std::string &keys<true>::terminal_key = short_terminal_key;
  template<> const std::string &keys<true>::abstract_key = short_abstract_key;
  template<> const std::string &keys<true>::param_key = short_param_key;
  template<> const std::string &keys<true>::location_key = short_location_key;
  template<> const std::string &keys<true>::rotation_key = short_rotation_key;
  template<> const std::string &keys<true>::mirror_key = short_mirror_key;
  template<> const std::string &keys<true>::scale_key = short_scale_key;
  template<> const std::string &keys<true>::pin_key = short_pin_key;
}

}
