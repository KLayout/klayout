
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


#ifndef HDR_layBitmapsToImage
#define HDR_layBitmapsToImage

#include "layViewOp.h"
#include "tlThreads.h"

#include <vector>

namespace tl
{
  class PixelBuffer;
  class BitmapBuffer;
}

namespace lay
{

class DitherPattern;
class LineStyles;
class Bitmap;

/**
 *  @brief This function converts the given set of bitmaps to a PixelBuffer
 *
 *  This function uses the set of bitmaps in "pbitmaps" with the given set
 *  of view operands in "view_ops" and converts these into the QImage 
 *  with the given width and height.
 *  The "view_ops" and "pbitmaps" vectors must have the same size.
 *  The QImage must be initialized to the given width and height.
 *  If the tl::Mutex pointer is not 0, the mutex is locked between operations
 *  if the bitmap is accessed. The set of dither pattern specifies any custom
 *  pattern that are used bz the view operands.
 *  The "use_bitmap_index" parameter specifies whether the bitmap_index
 *  parameter of the operators is being used to map a operator to a certain
 *  bitmap.
 *  "dpr" is the device pixel ratio and will be used to scale line widths,
 *  stipple pattern and line styles.
 */
LAYBASIC_PUBLIC void
bitmaps_to_image (const std::vector <lay::ViewOp> &view_ops, 
                  const std::vector <lay::Bitmap *> &pbitmaps,
                  const lay::DitherPattern &dp, 
                  const lay::LineStyles &ls,
                  double dpr,
                  tl::PixelBuffer *pimage, unsigned int width, unsigned int height,
                  bool use_bitmap_index,
                  tl::Mutex *mutex);

/**
 *  @brief This function converts the given set of bitmaps to a BitmapBuffer
 *
 *  This is the monochrome version of the previous bitmaps_to_image function.
 */
LAYBASIC_PUBLIC void
bitmaps_to_image (const std::vector <lay::ViewOp> &view_ops,
                  const std::vector <lay::Bitmap *> &pbitmaps,
                  const lay::DitherPattern &dp,
                  const lay::LineStyles &ls,
                  double dpr,
                  tl::BitmapBuffer *pimage, unsigned int width, unsigned int height,
                  bool use_bitmap_index,
                  tl::Mutex *mutex);

/**
 *  @brief Convert a lay::Bitmap to a unsigned char * data field to be passed to tl::BitmapBuffer
 *
 *  This function converts the bitmap given the view_op into a raw byte data
 *  field that can be passed to a QBitmap constructor. The data field is not 
 *  cleared by the bits rather or'ed to the existing bits.
 */
LAYBASIC_PUBLIC void
bitmap_to_bitmap (const lay::ViewOp &view_op, const lay::Bitmap &bitmap,
                  unsigned char *data,
                  unsigned int width, unsigned int height,
                  const lay::DitherPattern &dp,
                  const lay::LineStyles &ls,
                  double dpr);

} // namespace lay

#endif

