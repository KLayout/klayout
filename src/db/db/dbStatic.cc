
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


#include "dbStatic.h"
#include "tlException.h"

#include <cstdio>

namespace db
{

// -----------------------------------------------------------
//  editable mode

DB_PUBLIC bool ms_editable = false;

void set_default_editable_mode (bool editable)
{
  ms_editable = editable;
}

void check_editable_mode (const char *f_str)
{
  if (! ms_editable) {
    throw tl::Exception (tl::to_string (tr ("Function '%s' is permitted only in editable mode")), f_str);
  }
}

void check_editable_mode ()
{
  if (! ms_editable) {
    throw tl::Exception (tl::to_string (tr ("Operation is permitted only in editable mode")));
  }
}

// -----------------------------------------------------------
//  number of points per circle

DB_PUBLIC unsigned int ms_num_circle_points = 32;

void set_num_circle_points (unsigned int n)
{
  ms_num_circle_points = n;
}

// -----------------------------------------------------------
//  undo enable 

DB_PUBLIC bool ms_transactions_enabled = true;

void enable_transactions (bool enable)
{
  ms_transactions_enabled = enable;
}

}

