
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
  const char *l2n_magic_string_cstr = "#%l2n-klayout";

  DB_PUBLIC std::string LongKeys::l2n_magic_string (l2n_magic_string_cstr);
  DB_PUBLIC std::string ShortKeys::l2n_magic_string (l2n_magic_string_cstr);

  DB_PUBLIC std::string LongKeys::version_key ("version");
  DB_PUBLIC std::string LongKeys::description_key ("description");
  DB_PUBLIC std::string LongKeys::top_key ("top");
  DB_PUBLIC std::string LongKeys::unit_key ("unit");
  DB_PUBLIC std::string LongKeys::layer_key ("layer");
  DB_PUBLIC std::string LongKeys::class_key ("class");
  DB_PUBLIC std::string LongKeys::connect_key ("connect");
  DB_PUBLIC std::string LongKeys::global_key ("global");
  DB_PUBLIC std::string LongKeys::circuit_key ("circuit");
  DB_PUBLIC std::string LongKeys::net_key ("net");
  DB_PUBLIC std::string LongKeys::name_key ("name");
  DB_PUBLIC std::string LongKeys::device_key ("device");
  DB_PUBLIC std::string LongKeys::polygon_key ("polygon");
  DB_PUBLIC std::string LongKeys::rect_key ("rect");
  DB_PUBLIC std::string LongKeys::terminal_key ("terminal");
  DB_PUBLIC std::string LongKeys::abstract_key ("abstract");
  DB_PUBLIC std::string LongKeys::param_key ("param");
  DB_PUBLIC std::string LongKeys::location_key ("location");
  DB_PUBLIC std::string LongKeys::rotation_key ("rotation");
  DB_PUBLIC std::string LongKeys::mirror_key ("mirror");
  DB_PUBLIC std::string LongKeys::scale_key ("scale");
  DB_PUBLIC std::string LongKeys::pin_key ("pin");

  //  A, B, C, D, E, G, I, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y
  DB_PUBLIC std::string ShortKeys::version_key ("V");
  DB_PUBLIC std::string ShortKeys::description_key ("B");
  DB_PUBLIC std::string ShortKeys::top_key ("W");
  DB_PUBLIC std::string ShortKeys::unit_key ("U");
  DB_PUBLIC std::string ShortKeys::layer_key ("L");
  DB_PUBLIC std::string ShortKeys::class_key ("K");
  DB_PUBLIC std::string ShortKeys::connect_key ("C");
  DB_PUBLIC std::string ShortKeys::global_key ("G");
  DB_PUBLIC std::string ShortKeys::circuit_key ("X");
  DB_PUBLIC std::string ShortKeys::net_key ("N");
  DB_PUBLIC std::string ShortKeys::name_key ("I");
  DB_PUBLIC std::string ShortKeys::device_key ("D");
  DB_PUBLIC std::string ShortKeys::polygon_key ("Q");
  DB_PUBLIC std::string ShortKeys::rect_key ("R");
  DB_PUBLIC std::string ShortKeys::terminal_key ("T");
  DB_PUBLIC std::string ShortKeys::abstract_key ("A");
  DB_PUBLIC std::string ShortKeys::param_key ("E");
  DB_PUBLIC std::string ShortKeys::location_key ("Y");
  DB_PUBLIC std::string ShortKeys::rotation_key ("O");
  DB_PUBLIC std::string ShortKeys::mirror_key ("M");
  DB_PUBLIC std::string ShortKeys::scale_key ("S");
  DB_PUBLIC std::string ShortKeys::pin_key ("P");
}

}
