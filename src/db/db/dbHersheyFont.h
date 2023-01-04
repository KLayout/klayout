
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


#ifndef HDR_dbHersheyFont
#define HDR_dbHersheyFont

namespace db {

enum Font { NoFont = -1, DefaultFont = 0, GothicFont, SansSerifFont, StickFont, TimesItalicFont, TimesThinFont, TimesFont, NFonts };
enum HAlign { NoHAlign = -1, HAlignCenter = 1, HAlignLeft = 0, HAlignRight = 2 };
enum VAlign { NoVAlign = -1, VAlignCenter = 1, VAlignTop = 0, VAlignBottom = 2 };

}

#endif

