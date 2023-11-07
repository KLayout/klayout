
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

#include "dbLayoutVsSchematicFormatDefs.h"

namespace db
{

namespace lvs_std_format
{
  const char *lvs_magic_string_cstr = "#%lvsdb-klayout";
  DB_PUBLIC std::string ShortKeys::lvs_magic_string (lvs_magic_string_cstr);
  DB_PUBLIC std::string LongKeys::lvs_magic_string (lvs_magic_string_cstr);

  DB_PUBLIC std::string LongKeys::reference_key ("reference");
  DB_PUBLIC std::string LongKeys::layout_key ("layout");
  DB_PUBLIC std::string LongKeys::xref_key ("xref");
  DB_PUBLIC std::string LongKeys::log_key ("log");
  DB_PUBLIC std::string LongKeys::log_entry_key ("entry");

  DB_PUBLIC std::string LongKeys::mismatch_key ("mismatch");
  DB_PUBLIC std::string LongKeys::match_key ("match");
  DB_PUBLIC std::string LongKeys::nomatch_key ("nomatch");
  DB_PUBLIC std::string LongKeys::warning_key ("warning");
  DB_PUBLIC std::string LongKeys::skipped_key ("skipped");

  //  H, J, L, M, S, X, Z, 0, 1

  DB_PUBLIC std::string ShortKeys::reference_key ("H");
  DB_PUBLIC std::string ShortKeys::layout_key ("J");
  DB_PUBLIC std::string ShortKeys::xref_key ("Z");
  DB_PUBLIC std::string ShortKeys::log_key ("L");
  DB_PUBLIC std::string ShortKeys::log_entry_key ("M");

  DB_PUBLIC std::string ShortKeys::mismatch_key ("0");
  DB_PUBLIC std::string ShortKeys::match_key ("1");
  DB_PUBLIC std::string ShortKeys::nomatch_key ("X");
  DB_PUBLIC std::string ShortKeys::warning_key ("W");
  DB_PUBLIC std::string ShortKeys::skipped_key ("S");
}

}
