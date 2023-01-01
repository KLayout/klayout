
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

#include "gsiDecl.h"
#include "tlPixelBuffer.h"

#if defined(HAVE_QT)
#  include <QColor>
#endif

namespace gsi
{

#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)

static void fill_with_qcolor (tl::PixelBuffer *pb, QColor c)
{
  pb->fill (c.rgb ());
}

//  NOTE: we add the Qt-Bindings-related methods of tl::PixelBuffer and tl::BitmapBuffer
//  here instead of inside gsiDeclTl.
//  Reasoning: by doing so, gsi and tl remain independent of the Qt binding library which
//  is good for lean applications such as strmrun

ClassExt<tl::PixelBuffer> decl_PixelBuffer (
  gsi::method_ext ("fill", &fill_with_qcolor, gsi::arg ("color"),
    "@brief Fills the pixel buffer with the given QColor\n"
  ) +
  gsi::method ("to_qimage", &tl::PixelBuffer::to_image_copy,
    "@brief Converts the pixel buffer to a \\QImage object"
  ) +
  gsi::method ("from_qimage", &tl::PixelBuffer::from_image, gsi::arg ("qimage"),
    "@brief Creates a pixel buffer object from a QImage object\n"
  )
);

ClassExt<tl::BitmapBuffer> decl_BitmapBuffer (
  gsi::method ("to_qimage", &tl::BitmapBuffer::to_image_copy,
    "@brief Converts the pixel buffer to a \\QImage object"
  ) +
  gsi::method ("from_qimage", &tl::BitmapBuffer::from_image, gsi::arg ("qimage"),
    "@brief Creates a pixel buffer object from a QImage object\n"
  )
);

#endif

}
