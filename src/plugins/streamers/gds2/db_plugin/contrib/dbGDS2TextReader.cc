
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


#include "dbGDS2Reader.h"
#include "dbGDS2Converter.h"
#include "dbGDS2.h"
#include "dbGDS2TextReader.h"
#include "dbArray.h"

#include "tlException.h"
#include "tlString.h"
#include "tlClassRegistry.h"

#include <limits>
#include <cctype>

namespace db
{

GDS2ReaderText::GDS2ReaderText(tl::InputStream &s, int /*ignored*/)
  : GDS2ReaderBase(), sStream(s),
    mProgress (tl::to_string (tr ("Reading GDS2 text file")), 10000),
    storedRecId (0)
{
  mProgress.set_format (tl::to_string (tr ("%.0f MB")));
  mProgress.set_unit (1024 * 1024);
}

GDS2ReaderText::~GDS2ReaderText()
{
  //  .. nothing yet ..
}
  
void
GDS2ReaderText::init (const db::LoadLayoutOptions &options)
{
  GDS2ReaderBase::init (options);
  storedRecId = 0;
}

void
GDS2ReaderText::unget_record (short rec_id)
{
  storedRecId = rec_id;
}

short 
GDS2ReaderText::get_record ()
{
  short siValueToReturn = 0;

  if (storedRecId) {

    siValueToReturn = storedRecId;
    storedRecId = 0;

  } else {

    std::string nextLine;

    sExtractedArguments.clear ();
    xyData.clear ();

    do {   

      if (sExtractedValue.empty()) {

        if (sStream.at_end ()) {
          error ("Unexpected end of file");
          return 0;
        }

        std::string sLine = sStream.get_line ();

        const char *cp = sLine.c_str();
        while (*cp && isspace (*cp)) {
          ++cp;
        }

        if (*cp != '#') {
          sExtractedValue += cp;
        }

      }

      if (!sExtractedValue.empty ()) {

        // Save data, so that they can be re-evaluated later, as for now on we might only look for the end of the element
        nextLine = sExtractedValue;

        std::string s1, sNewArgument;
        short siLocalvalue = siExtractData(sExtractedValue, s1, sNewArgument);
        
        // In the function below, we treat  sXY identifier in some different manner, in order to gain some speed, as it require special actions

        if(siLocalvalue) {

          // If the extracted data does contain an identifier
          if(!siValueToReturn) {
            // If we do not have stored an identifier
            siValueToReturn = siLocalvalue;
                                
            if(siValueToReturn == sXY) {
              vConvertToXY(sNewArgument);    
            } else {
              // save extracted arguments
              if (! sExtractedArguments.empty ()) {
                sExtractedArguments.append(" ");
              }
              sExtractedArguments.append(sNewArgument);
            }

            // Special case for the end of librarie detection
            if(siLocalvalue == sENDLIB) {
              sExtractedValue.clear ();
              sExtractedArguments.clear ();
              break;
            }
          } else {
            // Unget as next line
            sExtractedValue = nextLine;
            break;
          }

        } else {
          if(siValueToReturn == sXY) {
            vConvertToXY(sNewArgument);
          }
        }

      }

    } while (true);

  }

  reader = tl::Extractor (sExtractedArguments.c_str ());
  return siValueToReturn;
}

void 
GDS2ReaderText::vConvertToXY(const std::string &_sArg)
{
  tl::Extractor ex (_sArg.c_str ());
  long x = 0, y = 0;
  if (ex.try_read (x) && ex.test (":") && ex.try_read (y)) {

    xyData.push_back (GDS2XY ());

    xyData.back ().x [0] = (x >> 24);
    xyData.back ().x [1] = (x >> 16);
    xyData.back ().x [2] = (x >> 8);
    xyData.back ().x [3] = x;
    
    xyData.back ().y [0] = (y >> 24);
    xyData.back ().y [1] = (y >> 16);
    xyData.back ().y [2] = (y >> 8);
    xyData.back ().y [3] = y;

  }
}

short 
GDS2ReaderText::siExtractData(std::string &_sInput, std::string &_sToken, std::string &_sArguments)
{
  short token = 0;

  std::string input;
  input.swap (_sInput);

  tl::Extractor ex (input.c_str ());
  if (! ex.at_end ()) {

    if (isalpha (*ex) && ex.try_read_word (_sToken, "")) {
      token = db::gds2_converter.to_short(_sToken.c_str());
      if (! token) {
        error ("Unexpected token '" + _sToken + "'");
      }
    }

    if (! ex.at_end ()) {

      if (! _sArguments.empty ()) {
        _sArguments.append (" ");
      }

      const char *rem = ex.skip ();

      if (token == sSTRING || token == sPROPVALUE) {

        //  take rest of line to allow ; in strings.
        _sArguments.append (rem);

      } else {

        const char *semicolon = strchr (rem, ';');
        if (semicolon) {
          _sInput = semicolon + 1;
          _sArguments.append (std::string (rem, 0, semicolon - rem));
        } else {
          _sArguments.append (rem);
        }

      }
        
    }

  }

  return token;
}

const char *
GDS2ReaderText::get_string ()
{
  return reader.skip ();
}

double
GDS2ReaderText::get_double ()
{
  double x = 0;
  if (! reader.try_read (x)) {
    error (tl::to_string (tr ("Expected a floating-point number")));
  }
  return x;
}

void
GDS2ReaderText::get_string (std::string &s) const
{
  //  TODO: get rid of this const_cast hack
  s = (const_cast<GDS2ReaderText *> (this))->reader.skip ();
}

int 
GDS2ReaderText::get_int ()
{
  int x = 0;
  if (! reader.try_read (x)) {
    error (tl::to_string (tr ("Expected an integer number")));
  }
  return x;
}

short 
GDS2ReaderText::get_short ()
{
  int x = 0;
  if (! reader.try_read (x)) {
    error (tl::to_string (tr ("Expected an integer number")));
  }
  if (x < std::numeric_limits<short>::min() || x > std::numeric_limits<short>::max ()) {
    error (tl::to_string (tr ("Value out of range for 16bit signed integer")));
  }
  return x;
}

unsigned short 
GDS2ReaderText::get_ushort ()
{
  unsigned int x = 0;
  if (! reader.try_read (x)) {
    error (tl::to_string (tr ("Expected an integer number")));
  }
  if (x > std::numeric_limits<unsigned short>::max ()) {
    error (tl::to_string (tr ("Value out of range for 16bit unsigned integer")));
  }
  return x;
}

std::string
GDS2ReaderText::path () const
{
  return sStream.source ();
}

void 
GDS2ReaderText::error (const std::string &msg)
{
  throw GDS2ReaderTextException (msg, int(sStream.line_number()), cellname().c_str ());
}

void 
GDS2ReaderText::warn (const std::string &msg, int wl)
{
  if (warn_level () < wl) {
    return;
  }

  // TODO: compress
  tl::warn << msg 
           << tl::to_string (tr (", line number=")) << sStream.line_number()
           << tl::to_string (tr (", cell=")) << cellname ().c_str ()
           << ")";
}

void 
GDS2ReaderText::get_time (unsigned int *mod_time, unsigned int *access_time)
{
  if (! reader.try_read (mod_time [1])) {
    return;
  }
  reader.test ("/") || reader.test (":");
  if (! reader.try_read (mod_time [2])) {
    return;
  }
  reader.test ("/") || reader.test (":");
  if (! reader.try_read (mod_time [0])) {
    return;
  }
  reader.test ("/") || reader.test (":");
  if (! reader.try_read (mod_time [3])) {
    return;
  }
  reader.test ("/") || reader.test (":");
  if (! reader.try_read (mod_time [4])) {
    return;
  }
  reader.test ("/") || reader.test (":");
  if (! reader.try_read (mod_time [5])) {
    return;
  }

  if (! reader.try_read (access_time [1])) {
    return;
  }
  reader.test ("/") || reader.test (":");
  if (! reader.try_read (access_time [2])) {
    return;
  }
  reader.test ("/") || reader.test (":");
  if (! reader.try_read (access_time [0])) {
    return;
  }
  reader.test ("/") || reader.test (":");
  if (! reader.try_read (access_time [3])) {
    return;
  }
  reader.test ("/") || reader.test (":");
  if (! reader.try_read (access_time [4])) {
    return;
  }
  reader.test ("/") || reader.test (":");
  if (! reader.try_read (access_time [5])) {
    return;
  }
}

GDS2XY * 
GDS2ReaderText::get_xy_data (unsigned int &xy_length)
{
  xy_length = (unsigned int) xyData.size ();
  return xyData.empty () ? 0 : &xyData.front ();
}

void 
GDS2ReaderText::progress_checkpoint ()
{
  mProgress.set (sStream.raw_stream ().pos ());
}

}  // end namespace db

