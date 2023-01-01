
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



#ifndef HDR_layRubberBox
#define HDR_layRubberBox

#include "layViewObject.h"

namespace lay
{

class LAYBASIC_PUBLIC RubberBox
  : public lay::ViewObject
{
public: 
  RubberBox (lay::ViewObjectUI *canvas, unsigned int color, const db::DPoint &p1, const db::DPoint &p2);

  void set_color (unsigned int color);
  void set_stipple (unsigned int s);
  void set_points (const db::DPoint &begin, const db::DPoint &end);

private:
  virtual void render (const Viewport &vp, ViewObjectCanvas &canvas);

  db::DPoint m_p1, m_p2;
  unsigned int m_color;
  unsigned int m_stipple;
};

}

#endif


