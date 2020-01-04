
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "layAbstractMenuProvider.h"

namespace lay
{

static AbstractMenuProvider *ms_instance = 0;

AbstractMenuProvider::AbstractMenuProvider ()
{
  if (! ms_instance) {
    ms_instance = this;
  }
}

AbstractMenuProvider::~AbstractMenuProvider ()
{
  if (ms_instance == this) {
    ms_instance = 0;
  }
}

AbstractMenuProvider *AbstractMenuProvider::instance () 
{
  return ms_instance;
}

}

