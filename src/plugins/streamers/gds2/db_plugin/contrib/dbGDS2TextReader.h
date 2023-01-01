
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


#ifndef HDR_dbGDS2ReaderText
#define HDR_dbGDS2ReaderText

#include "dbPluginCommon.h"
#include "dbGDS2Reader.h"
#include <sstream>


namespace db
{
  
/**
 *  @brief Generic base class of GDS2 Text reader exceptions
 */
class DB_PLUGIN_PUBLIC GDS2ReaderTextException
  : public ReaderException 
{
public:
  GDS2ReaderTextException (const std::string &msg, size_t n, const std::string &cell)
    : ReaderException (tl::sprintf (tl::to_string (tr ("%s (line number=%ld, cell=%s)")).c_str (), msg.c_str (),  n, cell.c_str ()))
  { }
};

/**
 *  @brief The GDS2 text format stream reader
 */
class DB_PLUGIN_PUBLIC GDS2ReaderText
  : public GDS2ReaderBase
{

public:
  /**
   *  @brief Construct a stream reader object
   *
   *  @param s The stream delegate from which to read stream data from
   */
  GDS2ReaderText(tl::InputStream &s, int _iChunkSize = 1024);

  /**
   *  @brief Destructor
   */
  ~GDS2ReaderText();

  /**
   *  @brief Format
   */
  const char *format () const { return "GDS2Text"; }

protected:
  virtual void init (const LoadLayoutOptions &options);

private:
  tl::TextInputStream sStream;
  std::string sExtractedValue;
  std::string sExtractedArguments;
  tl::AbsoluteProgress mProgress;
  short storedRecId;
  tl::Extractor reader;
  std::vector<GDS2XY> xyData;

  virtual std::string path () const;
  const char *get_string ();
  void get_string (std::string &s) const;
  int get_int ();
  short get_short ();
  unsigned short get_ushort ();
  double get_double();
  short get_record();
  void unget_record (short rec_id);
  void get_time (unsigned int *mod_time, unsigned int *access_time);
  GDS2XY *get_xy_data (unsigned int &xy_length);
  void progress_checkpoint ();
  short siExtractData(std::string &sInput, std::string &sToken, std::string &sArguments);

  /**
   *  @brief append XY datas into the aulpoints vector for later use
   */
  void vConvertToXY(const std::string &_sArg);

  void error (const std::string &txt);
  void warn (const std::string &txt, int warn_level = 1);
};

}
#endif

