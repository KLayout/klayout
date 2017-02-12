
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



#ifndef HDR_dbStream
#define HDR_dbStream

#include "dbCommon.h"

#include "tlClassRegistry.h"

#include <string>
#include <vector>

namespace tl
{
  class InputStream;
}

namespace db
{

class ReaderBase;
class WriterBase;

/**
 *  @brief A stream format declaration
 */
class DB_PUBLIC StreamFormatDeclaration 
{
public:
  /**
   *  @brief Constructor
   */
  StreamFormatDeclaration () { }

  /**
   *  @brief Destructor
   */
  virtual ~StreamFormatDeclaration () { }

  /**
   *  @brief Obtain the format name
   */
  virtual std::string format_name () const = 0;

  /**
   *  @brief Obtain the format description
   */
  virtual std::string format_desc () const = 0;

  /**
   *  @brief Obtain the (long) format description
   */
  virtual std::string format_title () const = 0;

  /**
   *  @brief Obtain the file dialog format contribution
   */
  virtual std::string file_format () const = 0;

  /**
   *  @brief Auto-detect this format from the stream
   */
  virtual bool detect (tl::InputStream &s) const = 0;

  /**
   *  @brief Create the reader
   */
  virtual ReaderBase *create_reader (tl::InputStream &s) const = 0;

  /**
   *  @brief Returns true, when the format supports import (has a reader)
   */
  virtual bool can_read () const = 0;

  /**
   *  @brief Create the reader
   */
  virtual WriterBase *create_writer () const = 0;

  /**
   *  @brief Returns true, when the format supports export (has a writer)
   */
  virtual bool can_write () const = 0;
};

}

namespace tl
{
  //  make registration available to external DLL's
  template class DB_PUBLIC tl::RegisteredClass<db::StreamFormatDeclaration>;
}

#endif


