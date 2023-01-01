
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



#ifndef HDR_rdbReader
#define HDR_rdbReader

#include "rdbCommon.h"

#include "tlStream.h"
#include "tlClassRegistry.h"

namespace rdb
{

class ReaderBase;
class Database;

/**
 *  @brief A RDB import format declaration
 */
class RDB_PUBLIC FormatDeclaration 
{
public:
  /**
   *  @brief Constructor
   */
  FormatDeclaration () { }

  /**
   *  @brief Destructor
   */
  virtual ~FormatDeclaration () { }

  /**
   *  @brief Obtain the format name
   */
  virtual std::string format_name () const = 0;

  /**
   *  @brief Obtain the format description
   */
  virtual std::string format_desc () const = 0;

  /**
   *  @brief Obtain the file dialog format contribution
   */
  virtual std::string file_format () const = 0;

  /**
   *  @brief Auto-detect this format from the given stream
   */
  virtual bool detect (tl::InputStream &stream) const = 0;

  /**
   *  @brief Create the reader
   */
  virtual ReaderBase *create_reader (tl::InputStream &s) const = 0;
};

/**
 *  @brief An utility method to match a file name against a given format
 */
extern bool match_filename_to_format (const std::string &fn, const std::string &fmt);

/**
 *  @brief Generic base class of reader exceptions
 */
class RDB_PUBLIC ReaderException
  : public tl::Exception 
{
public:
  ReaderException (const std::string &msg)
    : tl::Exception (msg)
  { }
};

/**
 *  @brief The generic reader base class
 */
class RDB_PUBLIC ReaderBase
{
public:
  ReaderBase () { }
  virtual ~ReaderBase () { }

  virtual void read (Database &db) = 0;
  virtual const char *format () const = 0;
};

/**
 *  @brief The generic reader 
 *
 *  This reader is supposed to fork to one of the specific readers
 *  depending on the format detected.
 */
class RDB_PUBLIC Reader
{
public: 
  /**
   *  @brief Construct a reader object
   *
   *  If no valid format can be detected, the constructor will throw 
   *  an exception. The stream must be opened already in order to allow
   *  format detection.
   *
   *  @param s The stream object from which to read stream data from
   */
  Reader (tl::InputStream &s);

  /**  
   *  @brief Destructor
   */
  ~Reader ();

  /** 
   *  @brief The basic read method 
   *
   *  This method will read the stream data and translate this to
   *  insert calls into the database object.
   *
   *  @param database The layout object to write to
   */
  void read (Database &database)
  {
    mp_actual_reader->read (database);
  }

  /**
   *  @brief Returns a format describing the file format found
   */
  const char *format () const
  {
    return mp_actual_reader->format ();
  }

private:
  ReaderBase *mp_actual_reader;
};

}

#endif

