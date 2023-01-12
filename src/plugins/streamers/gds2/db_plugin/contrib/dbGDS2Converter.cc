
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
#include "dbGDS2Writer.h"
#include "dbGDS2Converter.h"
#include "dbGDS2.h"
#include "dbStream.h"

#include "tlClassRegistry.h"

#include <string>
#include <map>

namespace db
{

const char* Gds2ConstantConverter::to_char(short _sConstValue)
{
  std::map<short, std::string>::iterator it;

  it = short_string_conversion_map.find(_sConstValue);

  if(it!= short_string_conversion_map.end()){
    return it->second.c_str();
  }
  return "";
}

short Gds2ConstantConverter::to_short   (const char * _cstrConstName)
{
  std::map<std::string, short>::iterator it;

  it = string_short_conversion_map.find(_cstrConstName);

  if(it!= string_short_conversion_map.end()){
    return it->second;
  }
  return 0;
}

void db::Gds2ConstantConverter::vInitialize()
  {
    if(!bIsInitialized)
    {
      short_string_conversion_map[sHEADER        ] = "HEADER";
      short_string_conversion_map[sBGNLIB        ] = "BGNLIB";
      short_string_conversion_map[sLIBNAME       ] = "LIBNAME";
      short_string_conversion_map[sUNITS         ] =  "UNITS";
      short_string_conversion_map[sENDLIB        ] = "ENDLIB";
      short_string_conversion_map[sBGNSTR        ] = "BGNSTR";
      short_string_conversion_map[sSTRNAME       ] = "STRNAME";
      short_string_conversion_map[sENDSTR        ] = "ENDSTR";
      short_string_conversion_map[sBOUNDARY      ] = "BOUNDARY";
      short_string_conversion_map[sPATH          ] = "PATH";
      short_string_conversion_map[sSREF          ] = "SREF";
      short_string_conversion_map[sAREF          ] = "AREF";
      short_string_conversion_map[sTEXT          ] = "TEXT";
      short_string_conversion_map[sLAYER         ] = "LAYER";
      short_string_conversion_map[sDATATYPE      ] = "DATATYPE";
      short_string_conversion_map[sWIDTH         ] = "WIDTH";
      short_string_conversion_map[sXY            ] = "XY";
      short_string_conversion_map[sENDEL         ] = "ENDEL";
      short_string_conversion_map[sSNAME         ] = "SNAME";
      short_string_conversion_map[sCOLROW        ] = "COLROW";
      short_string_conversion_map[sTEXTNODE      ] = "TEXTNODE";
      short_string_conversion_map[sNODE          ] = "NODE";
      short_string_conversion_map[sTEXTTYPE      ] = "TEXTTYPE";
      short_string_conversion_map[sPRESENTATION  ] = "PRESENTATION";
      short_string_conversion_map[sSTRING        ] = "STRING";
      short_string_conversion_map[sSTRANS        ] = "STRANS";
      short_string_conversion_map[sMAG           ] = "MAG";
      short_string_conversion_map[sANGLE         ] = "ANGLE";
      short_string_conversion_map[sREFLIBS       ] = "REFLIBS";
      short_string_conversion_map[sFONTS         ] = "FONTS";
      short_string_conversion_map[sPATHTYPE      ] = "PATHTYPE";
      short_string_conversion_map[sGENERATIONS   ] = "GENERATIONS";
      short_string_conversion_map[sATTRTABLE     ] = "ATTRTABLE";
      short_string_conversion_map[sSTYPTABLE     ] = "STYPTABLE";
      short_string_conversion_map[sSTRTYPE       ] = "STRTYPE";
      short_string_conversion_map[sELFLAGS       ] = "ELFLAGS";
      short_string_conversion_map[sELKEY         ] = "ELKEY";
      short_string_conversion_map[sNODETYPE      ] = "NODETYPE";
      short_string_conversion_map[sPROPATTR      ] = "PROPATTR";
      short_string_conversion_map[sPROPVALUE     ] = "PROPVALUE";
      short_string_conversion_map[sBOX           ] = "BOX";
      short_string_conversion_map[sBOXTYPE       ] = "BOXTYPE";
      short_string_conversion_map[sPLEX          ] = "PLEX";
      short_string_conversion_map[sBGNEXTN       ] = "BGNEXTN";
      short_string_conversion_map[sENDEXTN       ] = "ENDEXTN";
      short_string_conversion_map[sTAPENUM       ] = "TAPENUM";
      short_string_conversion_map[sTAPECODE      ] = "TAPECODE";
      short_string_conversion_map[sSTRCLASS      ] = "STRCLASS";
      short_string_conversion_map[sRESERVED      ] = "RESERVED";
      short_string_conversion_map[sFORMAT        ] = "FORMAT";
      short_string_conversion_map[sMASK          ] = "MASK";
      short_string_conversion_map[sENDMASKS      ] = "ENDMASKS";
      short_string_conversion_map[sLIBDIRSIZE    ] = "LIBDIRSIZE";
      short_string_conversion_map[sSRFNAME       ] = "SRFNAME";
      
      for(std::map<short, std::string>::iterator it = short_string_conversion_map.begin(); it != short_string_conversion_map.end(); ++ it)
      {
        string_short_conversion_map.insert(std::pair<std::string, short>(it->second, it->first));
      }

      bIsInitialized = true;
    }
  }
}

