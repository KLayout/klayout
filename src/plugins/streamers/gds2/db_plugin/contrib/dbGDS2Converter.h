
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


#ifndef HDR_dbGDS2_Conv
#define HDR_dbGDS2_Conv

namespace db
{

/**
 *  @brief A class to switch GDS2 markers from binary to text
 *
 */ 
class Gds2ConstantConverter
{
private:
  bool bIsInitialized;

  std::map<std::string, short> string_short_conversion_map;
  std::map<short, std::string> short_string_conversion_map;

  /**
   *  @brief Initialize the structure
   *
   */ 
  void vInitialize();

public:

  Gds2ConstantConverter():bIsInitialized(false){vInitialize();};

  /**
   *  @brief Return the corresponding text marker
   *
   */ 
  const char* to_char   (short sConstValue);
  
  /**
   *  @brief Return the corresponding short marker
   *
   */ 
  short          to_short  (const char * cstrConstName);
}

static gds2_converter;

}

#endif

