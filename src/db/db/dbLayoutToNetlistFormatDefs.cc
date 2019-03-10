
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
  template<> DB_PUBLIC const std::string keys<false>::version_key ("version");
  template<> DB_PUBLIC const std::string keys<false>::description_key ("description");
  template<> DB_PUBLIC const std::string keys<false>::top_key ("top");
  template<> DB_PUBLIC const std::string keys<false>::unit_key ("unit");
  template<> DB_PUBLIC const std::string keys<false>::layer_key ("layer");
  template<> DB_PUBLIC const std::string keys<false>::connect_key ("connect");
  template<> DB_PUBLIC const std::string keys<false>::global_key ("global");
  template<> DB_PUBLIC const std::string keys<false>::circuit_key ("circuit");
  template<> DB_PUBLIC const std::string keys<false>::net_key ("net");
  template<> DB_PUBLIC const std::string keys<false>::name_key ("name");
  template<> DB_PUBLIC const std::string keys<false>::device_key ("device");
  template<> DB_PUBLIC const std::string keys<false>::polygon_key ("polygon");
  template<> DB_PUBLIC const std::string keys<false>::rect_key ("rect");
  template<> DB_PUBLIC const std::string keys<false>::terminal_key ("terminal");
  template<> DB_PUBLIC const std::string keys<false>::abstract_key ("abstract");
  template<> DB_PUBLIC const std::string keys<false>::param_key ("param");
  template<> DB_PUBLIC const std::string keys<false>::location_key ("location");
  template<> DB_PUBLIC const std::string keys<false>::rotation_key ("rotation");
  template<> DB_PUBLIC const std::string keys<false>::mirror_key ("mirror");
  template<> DB_PUBLIC const std::string keys<false>::scale_key ("scale");
  template<> DB_PUBLIC const std::string keys<false>::pin_key ("pin");

  template<> DB_PUBLIC const std::string keys<true>::version_key ("V");
  template<> DB_PUBLIC const std::string keys<true>::description_key ("B");
  template<> DB_PUBLIC const std::string keys<true>::top_key ("W");
  template<> DB_PUBLIC const std::string keys<true>::unit_key ("U");
  template<> DB_PUBLIC const std::string keys<true>::layer_key ("L");
  template<> DB_PUBLIC const std::string keys<true>::connect_key ("C");
  template<> DB_PUBLIC const std::string keys<true>::global_key ("G");
  template<> DB_PUBLIC const std::string keys<true>::circuit_key ("X");
  template<> DB_PUBLIC const std::string keys<true>::net_key ("N");
  template<> DB_PUBLIC const std::string keys<true>::name_key ("I");
  template<> DB_PUBLIC const std::string keys<true>::device_key ("D");
  template<> DB_PUBLIC const std::string keys<true>::polygon_key ("Q");
  template<> DB_PUBLIC const std::string keys<true>::rect_key ("R");
  template<> DB_PUBLIC const std::string keys<true>::terminal_key ("T");
  template<> DB_PUBLIC const std::string keys<true>::abstract_key ("A");
  template<> DB_PUBLIC const std::string keys<true>::param_key ("E");
  template<> DB_PUBLIC const std::string keys<true>::location_key ("Y");
  template<> DB_PUBLIC const std::string keys<true>::rotation_key ("O");
  template<> DB_PUBLIC const std::string keys<true>::mirror_key ("M");
  template<> DB_PUBLIC const std::string keys<true>::scale_key ("S");
  template<> DB_PUBLIC const std::string keys<true>::pin_key ("P");
}

}
