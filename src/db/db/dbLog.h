
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

#ifndef _HDR_dbLog
#define _HDR_dbLog

#include "dbCommon.h"

#include <string>

namespace db
{

/**
 *  @brief An enum describing the severity for a log entry
 */
enum Severity {
  NoSeverity = 0,   //  unspecific
  Info = 1,         //  information only
  Warning = 2,      //  a warning
  Error = 3         //  an error
};

/**
 *  @brief A class representing one log entry
 */
struct LogEntryData
{
  LogEntryData (Severity s, const std::string &m) : severity (s), msg (m) { }
  LogEntryData () : severity (NoSeverity) { }

  Severity severity;
  std::string msg;
};

}

#endif
