
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

#ifndef HDR_imgStream
#define HDR_imgStream

#include "imgCommon.h"

#include "imgObject.h"
#include "tlStream.h"

namespace img {
  
/**
 *  @brief An object streaming image data from or to files
 */
struct IMG_PUBLIC ImageStreamer
{
public:
  /**
   *  @brief The constructor
   */
  ImageStreamer () { }

  /**
   *  @brief Reads an image Object from a stream
   *
   *  This method returns a new'd object. It's the responsibility of the caller to delete the object.
   */
  static Object *read(tl::InputStream &stream);

  /**
   *  @brief Writes an image object to a stream
   */
  static void write (tl::OutputStream &stream, const img::Object &img);
};

}

#endif

