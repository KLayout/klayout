
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

#include "dbLayoutVsSchematicFormatDefs.h"

namespace db
{

namespace lvs_std_format
{
  const char *lvs_magic_string_cstr = "#%lvsdb-klayout";
  template<> DB_PUBLIC const std::string keys<false>::lvs_magic_string (lvs_magic_string_cstr);
  template<> DB_PUBLIC const std::string keys<true>::lvs_magic_string (lvs_magic_string_cstr);

  template<> DB_PUBLIC const std::string keys<false>::reference_key ("reference");
  template<> DB_PUBLIC const std::string keys<false>::layout_key ("layout");
  template<> DB_PUBLIC const std::string keys<false>::xref_key ("xref");

  template<> DB_PUBLIC const std::string keys<false>::mismatch_key ("mismatch");
  template<> DB_PUBLIC const std::string keys<false>::match_key ("match");
  template<> DB_PUBLIC const std::string keys<false>::nomatch_key ("nomatch");
  template<> DB_PUBLIC const std::string keys<false>::warning_key ("warning");
  template<> DB_PUBLIC const std::string keys<false>::skipped_key ("skipped");

  template<> DB_PUBLIC const std::string keys<true>::reference_key ("H");
  template<> DB_PUBLIC const std::string keys<true>::layout_key ("J");
  template<> DB_PUBLIC const std::string keys<true>::xref_key ("Z");

  template<> DB_PUBLIC const std::string keys<true>::mismatch_key ("0");
  template<> DB_PUBLIC const std::string keys<true>::match_key ("1");
  template<> DB_PUBLIC const std::string keys<true>::nomatch_key ("X");
  template<> DB_PUBLIC const std::string keys<true>::warning_key ("W");
  template<> DB_PUBLIC const std::string keys<true>::skipped_key ("S");
}

}
