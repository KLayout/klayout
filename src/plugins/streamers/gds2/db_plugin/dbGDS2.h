
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


#ifndef HDR_dbGDS2
#define HDR_dbGDS2

#include <stdint.h>

namespace db
{

const short sHEADER       = 0x0002;
const short sBGNLIB       = 0x0102;
const short sLIBNAME      = 0x0206;
const short sUNITS        = 0x0305;
const short sENDLIB       = 0x0400;
const short sBGNSTR       = 0x0502;
const short sSTRNAME      = 0x0606;
const short sENDSTR       = 0x0700;
const short sBOUNDARY     = 0x0800;
const short sPATH         = 0x0900;
const short sSREF         = 0x0a00;
const short sAREF         = 0x0b00;
const short sTEXT         = 0x0c00;
const short sLAYER        = 0x0d02;
const short sDATATYPE     = 0x0e02;
const short sWIDTH        = 0x0f03;
const short sXY           = 0x1003;
const short sENDEL        = 0x1100;
const short sSNAME        = 0x1206;
const short sCOLROW       = 0x1302;
const short sTEXTNODE     = 0x1400;
const short sNODE         = 0x1500;
const short sTEXTTYPE     = 0x1602;
const short sPRESENTATION = 0x1701;
const short sSTRING       = 0x1906;
const short sSTRANS       = 0x1a01;
const short sMAG          = 0x1b05;
const short sANGLE        = 0x1c05;
const short sREFLIBS      = 0x1f06;
const short sFONTS        = 0x2006;
const short sPATHTYPE     = 0x2102;
const short sGENERATIONS  = 0x2202;
const short sATTRTABLE    = 0x2306;
const short sSTYPTABLE    = 0x2406;
const short sSTRTYPE      = 0x2502;
const short sELFLAGS      = 0x2601;
const short sELKEY        = 0x2703;
const short sNODETYPE     = 0x2a02;
const short sPROPATTR     = 0x2b02;
const short sPROPVALUE    = 0x2c06;
const short sBOX          = 0x2d00;
const short sBOXTYPE      = 0x2e02;
const short sPLEX         = 0x2f03;
const short sBGNEXTN      = 0x3003;
const short sENDEXTN      = 0x3103;
const short sTAPENUM      = 0x3202;
const short sTAPECODE     = 0x3302;
const short sSTRCLASS     = 0x3401;
const short sRESERVED     = 0x3503;
const short sFORMAT       = 0x3602;
const short sMASK         = 0x3706;
const short sENDMASKS     = 0x3800;
const short sLIBDIRSIZE   = 0x3902;
const short sSRFNAME      = 0x3a06;

// ------------------------------------------------------------------
//  Utilities for converting the bytes from the order GDS wants it to have

inline void gds2h (int16_t &s)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
  //  swap required 
  char x;
  char *d = (char *)&s;
  x = d[0]; d[0] = d[1]; d[1] = x;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
  //  .. no action required
#else
  //  generic solution
  s = (int16_t (((unsigned char *)&s) [0]) << 8) | int16_t (((unsigned char *)&s) [1]);
#endif
}

inline void gds2h (int32_t &i)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
  //  swap required 
  char x;
  char *d = (char *)&i;
  x = d[0]; d[0] = d[3]; d[3] = x;
  x = d[1]; d[1] = d[2]; d[2] = x;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
  //  .. no action required
#else
  i = (int32_t (((unsigned char *)&i) [0]) << 24) | (int32_t (((unsigned char *)&i) [1]) << 16) |
      (int32_t (((unsigned char *)&i) [2]) << 8)  |  int32_t (((unsigned char *)&i) [3]);
#endif
}

}

#endif

