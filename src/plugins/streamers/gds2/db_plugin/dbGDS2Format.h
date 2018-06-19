
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#ifndef HDR_dbGDS2Format
#define HDR_dbGDS2Format

#include "dbSaveLayoutOptions.h"
#include "dbLoadLayoutOptions.h"

namespace db
{

/**
 *  @brief Structure that holds the GDS2 specific options for the reader
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class GDS2ReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  GDS2ReaderOptions ()
    : box_mode (1),
      allow_big_records (true),
      allow_multi_xy_records (true)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief How to treat BOX records
   *
   *  This property specifies how to treat BOX records. 
   *  Allowed values are 0 (ignore), 1 (treat as rectangles), 2 (treat as boundaries) or 3 (treat as errors).
   */
  unsigned int box_mode; 

  /**
   *  @brief Allow multiple big records
   *
   *  Setting this property to true allows to use up to 65535 bytes (instead of 32767) per record
   *  by treating the record length as unsigned short rather than signed short.
   *  This allows bigger polygons (up to ~8000 points) without having to use multiple XY records.
   */
  bool allow_big_records;

  /**
   *  @brief Allow multiple XY records in BOUNDARY elements for unlimited large polygons
   *
   *  Setting this property to true allows to unlimited polygons 
   *  by using multiple XY records. 
   */
  bool allow_multi_xy_records;

  /** 
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual FormatSpecificReaderOptions *clone () const
  {
    return new GDS2ReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("GDS2");
    return n;
  }
};

} // namespace db

#endif
