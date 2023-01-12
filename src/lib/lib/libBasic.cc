
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


#include "libBasicText.h"
#include "libBasicArc.h"
#include "libBasicCircle.h"
#include "libBasicEllipse.h"
#include "libBasicPie.h"
#include "libBasicDonut.h"
#include "libBasicRoundPath.h"
#include "libBasicRoundPolygon.h"
#include "libBasicStrokedPolygon.h"
#include "dbLibrary.h"

namespace lib
{

/**
 *  @brief Declaration of the Basic library 
 */
class Basic
  : public db::Library
{
public:
  Basic ()
    : db::Library ()
  {
    //  basic initialization
    set_name ("Basic");
    set_description ("Basic layout objects");

    //  register all the PCells:
    layout ().register_pcell ("TEXT", new BasicText ());
    layout ().register_pcell ("CIRCLE", new BasicCircle ());
    layout ().register_pcell ("ELLIPSE", new BasicEllipse ());
    layout ().register_pcell ("PIE", new BasicPie ());
    layout ().register_pcell ("ARC", new BasicArc ());
    layout ().register_pcell ("DONUT", new BasicDonut ());
    layout ().register_pcell ("ROUND_PATH", new BasicRoundPath ());
    layout ().register_pcell ("ROUND_POLYGON", new BasicRoundPolygon ());
    layout ().register_pcell ("STROKED_BOX", new BasicStrokedPolygon (true));
    layout ().register_pcell ("STROKED_POLYGON", new BasicStrokedPolygon (false));
  }
};

//  register the library
static tl::RegisteredClass<db::Library> basic_lib_registration (new Basic ());

}

