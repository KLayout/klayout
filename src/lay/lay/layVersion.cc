
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


#include "layVersion.h"

#include <string>

namespace lay 
{

static std::string s_exe_name;
static std::string s_name;
static std::string s_version;
static std::string s_subversion;
static std::string s_about_text;

const char *
Version::exe_name ()
{
  return s_exe_name.c_str ();
}

const char *
Version::name ()
{
  return s_name.c_str ();
}

const char *
Version::version ()
{
  return s_version.c_str ();
}

const char *
Version::subversion ()
{
  return s_subversion.c_str ();
}

const char *
Version::about_text ()
{
  return s_about_text.c_str ();
}

void
Version::set_exe_name (const char *s)
{
  s_exe_name = s;
}

void
Version::set_name (const char *s)
{
  s_name = s;
}

void
Version::set_version (const char *s)
{
  s_version = s;
}

void
Version::set_subversion (const char *s)
{
  s_subversion = s;
}

void
Version::set_about_text (const char *s)
{
  s_about_text = s;
}

}

