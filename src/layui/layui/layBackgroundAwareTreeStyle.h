
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#if defined(HAVE_QT)

#ifndef HDR_layBackgroundAwareTreeStyle
#define HDR_layBackgroundAwareTreeStyle

#include "layuiCommon.h"

#include <QProxyStyle>

namespace lay
{

/**
 *  @brief A style tailoring the drawing of the branch indicator
 *  This proxy style is making the branch indicator a triangle and aware of the
 *  palette of the tree.
 *  The default Gtk style is not, hence making the background dark means the
 *  triangles become invisible.
 */
class LAYUI_PUBLIC BackgroundAwareTreeStyle
  : public QProxyStyle
{
public:
  BackgroundAwareTreeStyle (QStyle *org_style);
  void drawPrimitive (PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const;
};

} // namespace lay

#endif

#endif  //  defined(HAVE_QT)
