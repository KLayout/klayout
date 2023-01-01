
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

#include <algorithm>

namespace lay
{

Viewport::Viewport ()
  : m_width (0), m_height (0)
{
  //  .. nothing yet .. 
}

Viewport::Viewport (unsigned int width, unsigned int height, const db::DBox &target_box)
  : m_width (width), m_height (height)
{
  set_box (target_box);
}

void 
Viewport::set_size (unsigned int w, unsigned int h)
{
  m_width = w;
  m_height = h;
  set_box (m_target_box);
}

void 
Viewport::set_global_trans (const db::DCplxTrans &trans)
{
  if (! trans.equal (m_global_trans)) {
    db::DBox b = box ();
    m_global_trans = trans;
    set_box (b);
  }
}

void 
Viewport::set_trans (const db::DCplxTrans &trans)
{
  m_trans = trans;
  m_target_box = trans * db::DBox (0.0, 0.0, double (width ()), double (height ()));
}

void 
Viewport::set_box (const db::DBox &in_box)
{
  m_target_box = in_box;

  db::DBox box = m_global_trans * in_box;

  //  use double arithmetics to avoid overflows
  double w = box.right () - box.left ();
  double h = box.top () - box.bottom ();
  double fx = w / double (std::max (width (), (unsigned int) 1));
  double fy = h / double (std::max (height (), (unsigned int) 1));
  double f = std::max (fx, fy);

  //  as a safety measure we treat the zero factor case somewhat more graceful
  if (f < 1e-13) {
    f = 0.001; // default mag. factor
  }

  double mx = double (box.right ()) + double (box.left ());
  double my = double (box.top ()) + double (box.bottom ());

  //  use only integer shift vectors. That enables a partial update of the image.
  double dx = floor (0.5 + (mx / f - double (width ())) * 0.5);
  double dy = floor (0.5 + (my / f - double (height ())) * 0.5);

  //  preserve the angle and mirror properties of the transformation
  m_trans = db::DCplxTrans (1.0 / f /*mag*/, 0.0 /*angle*/, false, db::DVector (-dx, -dy)) * m_global_trans;
}

db::DBox 
Viewport::box () const
{
  db::DPoint p1 = trans ().inverted () * db::DPoint (0, 0);
  db::DPoint p2 = trans ().inverted () * db::DPoint (width (), height ());
  return db::DBox (p1, p2);
}

}

