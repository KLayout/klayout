
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



#include "layViewport.h"
#include "layRubberBox.h"
#include "layRenderer.h"

namespace lay
{

// -------------------------------------------------------------
//  RubberBox implementation

RubberBox::RubberBox (lay::ViewObjectUI *widget, unsigned int color, const db::DPoint &p1, const db::DPoint &p2)
  : lay::ViewObject (widget, false /*not static*/), 
    m_p1 (p1), m_p2 (p2), m_color (color), m_stipple (0)
{ }

void 
RubberBox::set_color (unsigned int color)
{
  if (m_color != color) {
    m_color = color;
    redraw ();
  }
}

void 
RubberBox::set_stipple (unsigned int s)
{
  if (m_stipple != s) {
    m_stipple = s;
    redraw ();
  }
}

void 
RubberBox::render (const Viewport &vp, ViewObjectCanvas &canvas)
{ 
  lay::Renderer &r = canvas.renderer ();
  int lw = int (0.5 + 1.0 / r.resolution ());
  lay::CanvasPlane *plane = canvas.plane (lay::ViewOp (m_color, lay::ViewOp::Copy, 0, m_stipple, 0, lay::ViewOp::Rect, lw));
  if (plane) {
    r.draw (vp.trans () * db::DBox (m_p1, m_p2), 0, plane, 0, 0);
  }
}

void
RubberBox::set_points (const db::DPoint &p1, const db::DPoint &p2)
{ 
  if (m_p1 != p1 || m_p2 != p2) {
    m_p1 = p1;
    m_p2 = p2;
    redraw ();
  }
}

}

