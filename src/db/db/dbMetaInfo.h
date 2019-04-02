
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


#ifndef HDR_dbMetaInfo
#define HDR_dbMetaInfo

#include "dbCommon.h"
#include "dbMemStatistics.h"
#include <string>

namespace db
{

/**
 *  @brief A structure describing the meta information from the reader
 *
 *  In the meta information block, the reader provides additional information
 *  about the file and content etc.
 *  "name" is a unique name that can be used to identify the information.
 *  "description" is a "speaking" description of the information.
 *  "value" is the value of the specific part of meta information.
 */
struct DB_PUBLIC MetaInfo
{
  MetaInfo (const std::string &n, const std::string &d, const std::string &v)
    : name (n), description (d), value (v)
  {
    //  .. nothing else ..
  }

  MetaInfo ()
  {
    //  .. nothing else ..
  }

  std::string name;
  std::string description;
  std::string value;
};

/**
 *  @brief Collect memory statistics
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const MetaInfo &v, bool no_self, void *parent)
{
  db::mem_stat (stat, purpose, cat, v.name, no_self, parent);
  db::mem_stat (stat, purpose, cat, v.description, no_self, parent);
  db::mem_stat (stat, purpose, cat, v.value, no_self, parent);
}

}

#endif

