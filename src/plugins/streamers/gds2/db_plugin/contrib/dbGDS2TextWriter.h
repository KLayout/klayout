
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


#ifndef HDR_dbGDS2WriterText
#define HDR_dbGDS2WriterText

#include "dbPluginCommon.h"
#include "dbGDS2WriterBase.h"
#include <sstream>
#include <climits>

namespace db
{


class DB_PLUGIN_PUBLIC GDS2WriterText
  : public db::GDS2WriterBase
{

public:
  GDS2WriterText();
  ~GDS2WriterText();

protected:
  /**
   *  @brief Write a byte
   */
  virtual void write_byte (unsigned char b);

  /**
   *  @brief Write a short
   */
  virtual void write_short (int16_t i);

  /**
   *  @brief Write a long
   */
  virtual void write_int (int32_t l);

  /**
   *  @brief Write a double
   */
  virtual void write_double (double d);

  /**
   *  @brief Write the time
   */
  virtual void write_time (const short *t);

  /**
   *  @brief Write a string
   */
  virtual void write_string (const char *t);

  /**
   *  @brief Write a string
   */
  virtual void write_string (const std::string &t);

  /**
   *  @brief Write the size of the record
   */
  virtual void write_record_size (int16_t i);

  /**
   *  @brief Write a record identifier
   */
  virtual void write_record (int16_t i);

  /**
   *  @brief Set the stream to write the data to
   */
  void set_stream (tl::OutputStream &stream)
  {
    pStream = &stream;
  }

  /**
   *  @brief Establish a checkpoint for progress reporting
   */
  void progress_checkpoint ();

private:
  tl::OutputStream *pStream;
  std::stringstream ssFormattingStream;
  short siCurrentRecord;
  bool  bIsXCoordinate;
  tl::AbsoluteProgress mProgress;
};


} // namespace db

#endif

