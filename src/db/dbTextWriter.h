
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


#ifndef HDR_dbTextWriter
#define HDR_dbTextWriter

#include "dbCommon.h"

#include "tlException.h"
#include "tlStream.h"
#include "tlInternational.h"
#include "tlProgress.h"
#include "tlString.h"

#include "dbStreamLayers.h"

namespace db
{

class Layout;

/**
 *  @brief A Text writer abstraction
 */

class DB_PUBLIC TextWriter
{
public:
  /**
   *  @brief Instantiate the writer
   */
  TextWriter (tl::OutputStream &stream);

  /**
   *  @brief Write the layout object
   */
  void write (const db::Layout &layout);

protected:
  TextWriter &operator<< (const std::string &s);
  TextWriter &operator<< (const char *s);
  TextWriter &operator<< (int64_t n);
  TextWriter &operator<< (int32_t n);
  TextWriter &operator<< (double d);
  TextWriter &operator<< (db::Point p);
  TextWriter &operator<< (db::Vector p);
  const char *endl ();
  
private:
  tl::OutputStream &m_stream;
  
  void write_props (const db::Layout &layout, size_t prop_id);
};

} // namespace db

#endif

