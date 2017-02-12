
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


//  since the stream readers are provided as plugins, we need to force linking 
//  of the objects by using the macros provided when we build tools from common.a.

#include "dbOASIS.h"
#include "dbGDS2.h"
#include "dbCIF.h"
#include "dbDXF.h"
#include "contrib/dbGDS2Text.h"

namespace db
{

FORCE_LINK_OASIS
FORCE_LINK_GDS2
FORCE_LINK_GDS2_TXT
FORCE_LINK_CIF
FORCE_LINK_DXF

}

