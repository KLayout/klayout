
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


#ifndef HDR_dbCIF
#define HDR_dbCIF

#include "dbPoint.h"

#include "tlException.h"
#include "tlInternational.h"
#include "tlString.h"
#include "tlAssert.h"

#include <string>
#include <vector>

namespace db
{

/**
 *  @brief The diagnostics interface for reporting problems in the reader or writer
 */
class CIFDiagnostics
{
public:
  virtual ~CIFDiagnostics ();

  /**
   *  @brief Issue an error with positional information
   */
  virtual void error (const std::string &txt) = 0;

  /**
   *  @brief Issue a warning with positional information
   */
  virtual void warn (const std::string &txt) = 0;
};

}

#endif

