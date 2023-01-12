
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


#include "dbLayout.h"
#include "dbShape.h"
#include "tlStream.h"
#include "tlAssert.h"
#include "tlException.h"
#include "dbGDS2WriterBase.h"
#include "dbGDS2TextWriter.h"
#include "dbGDS2.h"
#include "dbGDS2Converter.h"
#include "dbSaveLayoutOptions.h"

#include <stdio.h>
#include <errno.h>

#include <limits>
#include <iomanip>

namespace db
{

GDS2WriterText::GDS2WriterText()
  : pStream(0),siCurrentRecord(0),bIsXCoordinate(true),
    mProgress (tl::to_string (tr ("Writing GDS2 text file")), 10000)
{
  mProgress.set_format (tl::to_string (tr ("%.0f MB")));
  mProgress.set_unit (1024 * 1024);
}

GDS2WriterText::~GDS2WriterText()
{
  // .. nothing yet ..
}

void 
GDS2WriterText::write_byte (unsigned char b)
{
  ssFormattingStream<<b<<" ";
}

void 
GDS2WriterText::write_short (int16_t i)
{
  ssFormattingStream<<i<<" ";
}


void 
GDS2WriterText::write_int (int32_t l)
{
  if(siCurrentRecord == sXY)
  {
    if(bIsXCoordinate)
    {
      ssFormattingStream<<l<<": ";
      bIsXCoordinate = false;
    }
    else
    {
      ssFormattingStream<<l<<std::endl;
      bIsXCoordinate = true;
    }
  }
  else
  {
    ssFormattingStream<<l<<" ";
  }
}


void 
GDS2WriterText::write_double (double d)
{
  ssFormattingStream<<d<<" ";
}


void 
GDS2WriterText::write_time (const short *time)
{
  //  time is an array of year/month/day hour/min/sec
  if (time[0] || time[1] || time[2]) {
    ssFormattingStream << time[1] << "/" << time[2] << "/" << time[0] << " " << time[3] << ":" << std::setfill('0') << std::setw(2) << time[4] << ":" << std::setfill('0') << std::setw(2) << time[5] << " ";
  }
}


void 
GDS2WriterText::write_string (const char *t)
{
    ssFormattingStream<<t;
}


void 
GDS2WriterText::write_string (const std::string &t)
{
  write_string(t.c_str());
}

void 
GDS2WriterText::write_record_size (int16_t /*i*/)
{
  // Nothing to do here
}


void 
GDS2WriterText::write_record (int16_t i)
{
  if(siCurrentRecord)
  {
    if(siCurrentRecord != sXY)
    {
      ssFormattingStream<<std::endl;
    }
  }
  
  switch((short)i)
  {
    case sBGNSTR:
    case sBOX:
    case sPATH:
    case sAREF:
    case sTEXT:
    case sBOUNDARY:
   {
      ssFormattingStream<<std::endl;
    }break; 
    default:
   {
    }break; 
  }

  // emit everyting we have so far
  pStream->put (ssFormattingStream.str().c_str(), ssFormattingStream.str().size() * sizeof(char));
  ssFormattingStream.str("");

  // produce record name
  ssFormattingStream<<db::gds2_converter.to_char((short)i)<<" ";
    

  switch((short)i)
  {
    case sENDLIB:
    {
      pStream->put (ssFormattingStream.str().c_str(), ssFormattingStream.str().size() * sizeof(char));
      ssFormattingStream.str("");
      siCurrentRecord = 0;
    } break;
    case sXY:
    {
      bIsXCoordinate = true;
      siCurrentRecord = (short)i;
    } break;
    default:
    {
      siCurrentRecord = (short)i;
    }
  }  
}

void 
GDS2WriterText::progress_checkpoint ()
{
  mProgress.set (pStream->pos ());
}


} // namespace db

