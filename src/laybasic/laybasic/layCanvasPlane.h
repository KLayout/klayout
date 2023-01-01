
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



#ifndef HDR_layCanvasPlane
#define HDR_layCanvasPlane

#include "laybasicCommon.h"

#include "dbEdge.h"
#include "dbText.h"
#include "dbHershey.h"

#include <stdint.h>

#include <vector>

namespace lay {

const double render_epsilon = 1e-6;

/**
 *  @brief A generic canvas plane
 *
 *  This is the base class for lay::Bitmap for example.
 *  It is used by the renderer to identify the rendering target.
 */
class LAYBASIC_PUBLIC CanvasPlane 
{
public:
  /**
   *  @brief Destructor
   */
  virtual ~CanvasPlane ();

  /**
   *  @brief Clear the plane
   */
  virtual void clear () = 0;

  /**
   *  @brief Fill method 
   *
   *  This method is supposed to draw a small rectangle of 1 pixel
   *
   *  @param x,y The coordinate where to put the pixel (lower left corner)
   */
  virtual void pixel (unsigned int y, unsigned int x) = 0;
};

} // namespace lay

#endif


