
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#ifndef HDR_lstrFormat
#define HDR_lstrFormat

#include "dbPluginCommon.h"
#include "dbLoadLayoutOptions.h"
#include "dbSaveLayoutOptions.h"

namespace lstr
{

/**
 *  @brief Structure that holds the LStream specific options for the reader
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC ReaderOptions
  : public db::FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  ReaderOptions ();

  /**
   *  @brief If not empty, this string specifies a key under which the bbox from the stream is stored in the cells
   */
  std::string bbox_meta_info_key;

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual db::FormatSpecificReaderOptions *clone () const
  {
    return new ReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("LStream");
    return n;
  }
};

/**
 *  @brief Structure that holds the OASIS specific options for the Writer
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC WriterOptions
  : public db::FormatSpecificWriterOptions
{
public:
  /**
   *  @brief The constructor
   */
  WriterOptions ();

  /**
   *  @brief OASIS writer compression level
   *
   *  This level describes how hard the OASIS writer will try to compress the shapes
   *  using shape arrays. Building shape arrays may take some time and requires some memory.
   *    0 - no shape array building
   *    1 - nearest neighbor shape array formation 
   *    2++ - enhanced shape array search algorithm using 2nd and further neighbor distances as well
   */
  int compression_level; 

  /**
   *  @brief Recompressions
   *
   *  If the recompression flag is true, existing shape arrays will be resolved and 
   *  put into the compressor again (may take longer).
   */
  bool recompress;

  /**
   *  @brief Permissive mode
   *
   *  In permissive mode, a warning is issued for certain cases rather than
   *  an error. For example paths/circles with odd diameter (rounded).
   */
  bool permissive;

  /** 
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual db::FormatSpecificWriterOptions *clone () const
  {
    return new WriterOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual const std::string &format_name () const
  {
    static std::string n ("LStream");
    return n;
  }
};

}

#endif
